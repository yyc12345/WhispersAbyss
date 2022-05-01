#include "../include/abyss_client.hpp"
#include "../include/global_define.hpp"
#include <steam/steamnetworkingsockets.h>
#include <steam/isteamnetworkingutils.h>

namespace WhispersAbyss {

	AbyssClient::AbyssClient(OutputHelper* output, const char* server, const char* username) :
		mIsRunning(false), mIsDead(false), mOutput(output),
		mRecvMsg(), mSendMsg(),
		mRecvMsgMutex(), mSendMsgMutex(), mSteamMutex(),
		mSteamSockets(NULL), mSteamConnection(k_HSteamNetConnection_Invalid), mSteamBuffer(),
		mUsername(username), mServerUrl(server),
		mStopCtx(false),
		mTdCtx(), mTdConnect() {
		// allocate fuck valve
		FuckValve::LockAbyssClientSingleton.lock();
		if (FuckValve::AbyssClientSingleton != NULL) {
			mOutput->FatalError("[Abyss] Fail to fuck Valve.");
		}
		FuckValve::AbyssClientSingleton = this;
		FuckValve::LockAbyssClientSingleton.unlock();

		// initialize steam lib
		SteamDatagramErrMsg err_msg;
		if (!GameNetworkingSockets_Init(nullptr, err_msg))
			mOutput->FatalError("GameNetworkingSockets_Init failed.  %s", err_msg);
		SteamNetworkingUtils()->SetDebugOutputFunction(
			k_ESteamNetworkingSocketsDebugOutputType_Msg,
			&FuckValve::ProcSteamDebugOutput
		);
		mSteamSockets = SteamNetworkingSockets();
	}

	AbyssClient::~AbyssClient() {
		// destroy steam work
		std::this_thread::sleep_for(std::chrono::milliseconds(500));
		GameNetworkingSockets_Kill();

		// release fuck valve
		FuckValve::LockAbyssClientSingleton.lock();
		FuckValve::AbyssClientSingleton = NULL;
		FuckValve::LockAbyssClientSingleton.unlock();
	}

	void AbyssClient::Start() {

		mStopCtx.store(false);

		// start connect worker
		mTdCtx = std::thread(&AbyssClient::CtxWorker, this);
		mTdConnect = std::thread(&AbyssClient::ConnectWorker, this);

		// notify main worker
		mIsRunning.store(true);
	}

	void AbyssClient::Stop() {

		DisconnectSteam();
		mStopCtx.store(true);

		if (mTdCtx.joinable())
			mTdCtx.join();
		if (mTdConnect.joinable())
			mTdConnect.join();

		// notify main worker
		mIsRunning.store(false);
	}

	void AbyssClient::CtxWorker() {
		std::deque<Bmmo::Messages::IMessage*> incoming_message, outbound_message;
		std::stringstream msg_stream;

		while (!mStopCtx.load()) {
			std::this_thread::sleep_for(std::chrono::milliseconds(10));

			// ==================================
			// poll new message
			// ready msg cache
			incoming_message.clear();
			RecvSteam(&incoming_message);
			// proc msg internally first
			for (auto it = incoming_message.begin(); it != incoming_message.end(); ++it) {
				InternalMsgProc(*it);
			}
			// after all messaged processed, push cache into list
			mRecvMsgMutex.lock();
			while (incoming_message.begin() != incoming_message.end()) {
				mRecvMsg.push_back(*incoming_message.begin());
				incoming_message.pop_front();
			}
			mRecvMsgMutex.unlock();

			// ==================================
			// poll status change
			mSteamSockets->RunCallbacks();

			// ==================================
			// process outbound message
			// pick from queue
			mSendMsgMutex.lock();
			while (mSendMsg.begin() != mSendMsg.end()) {
				outbound_message.push_back(*mSendMsg.begin());
				mSendMsg.pop_front();
			}
			mSendMsgMutex.unlock();
			// and send them
			while (outbound_message.begin() != outbound_message.end()) {
				auto msg = *outbound_message.begin();
				SendSteam(msg);
				delete msg;
				outbound_message.pop_front();
			}

		}
	}

	void AbyssClient::ConnectWorker() {
		// split url
		size_t urlpos = mServerUrl.find(":");
		std::string address = mServerUrl.substr(0, urlpos);
		std::string port = mServerUrl.substr(urlpos + 1);

		// solve ip
		mOutput->Printf("[Abyss] Resolving server address \"%s : %s\"", address.c_str(), port.c_str());
		asio::io_context ioContext;
		asio::ip::udp::resolver udpResolver(ioContext);
		asio::error_code ec;
		asio::ip::udp::resolver::results_type results = udpResolver.resolve(address, port, ec);
		if (!ec) {
			for (const auto& i : results) {
				auto endpoint = i.endpoint();
				std::string connection_string = endpoint.address().to_string() + ":" + std::to_string(endpoint.port());
				mOutput->Printf("[Abyss] Trying %s...", connection_string.c_str());
				if (ConnectSteam(&connection_string)) {
					break;	// success
				}
			}
		} else {
			mOutput->Printf("[Abyss] Fail to resolve hostname.");
		}

		ioContext.stop();
	}

	void AbyssClient::Send(std::deque<Bmmo::Messages::IMessage*>* manager_list) {
		size_t msg_size;

		// push data
		mSendMsgMutex.lock();
		for (auto it = manager_list->begin(); it != manager_list->end(); ++it) {
			mSendMsg.push_back(*it);
			// do not clean manager_list
		}
		msg_size = mSendMsg.size();
		mSendMsgMutex.unlock();

		if (msg_size >= WARNING_CAPACITY)
			mOutput->Printf("[Abyss] Message list reach warning level: %d", msg_size);
		if (msg_size >= NUKE_CAPACITY) {
			mOutput->FatalError("[Abyss] Message list reach nuke level: %d. The program will be nuked!!!", msg_size);
		}
	}
	void WhispersAbyss::AbyssClient::Send(Bmmo::Messages::IMessage* single_msg) {
		mSendMsgMutex.lock();
		mSendMsg.push_back(single_msg);
		mSendMsgMutex.unlock();
	}

	void AbyssClient::Recv(std::deque<Bmmo::Messages::IMessage*>* manager_list) {
		mRecvMsgMutex.lock();
		while (mRecvMsg.begin() != mRecvMsg.end()) {
			manager_list->push_back(*mRecvMsg.begin());
			mRecvMsg.pop_front();
		}
		mRecvMsgMutex.unlock();
	}

	Bmmo::Messages::IMessage* AbyssClient::CreateMessageFromStream(std::stringstream* data) {
		// peek opcode
		Bmmo::OpCode code = Bmmo::Messages::IMessage::PeekOpCode(data);

		// check format
		Bmmo::Messages::IMessage* msg = NULL;
		switch (code) {
			case WhispersAbyss::Bmmo::OpCode::LoginRequest:
				msg = new Bmmo::Messages::LoginRequest();
				break;
			case WhispersAbyss::Bmmo::OpCode::LoginAccepted:
				msg = new Bmmo::Messages::LoginAccepted();
				break;
			case WhispersAbyss::Bmmo::OpCode::LoginDenied:
				msg = new Bmmo::Messages::LoginDenied();
				break;
			case WhispersAbyss::Bmmo::OpCode::PlayerDisconnected:
				msg = new Bmmo::Messages::PlayerDisconnected();
				break;
			case WhispersAbyss::Bmmo::OpCode::PlayerConnected:
				msg = new Bmmo::Messages::PlayerConnected();
				break;
			case WhispersAbyss::Bmmo::OpCode::BallState:
				msg = new Bmmo::Messages::BallState();
				break;
			case WhispersAbyss::Bmmo::OpCode::OwnedBallState:
				msg = new Bmmo::Messages::OwnedBallState();
				break;
			case WhispersAbyss::Bmmo::OpCode::Chat:
				msg = new Bmmo::Messages::Chat();
				break;
			case WhispersAbyss::Bmmo::OpCode::LevelFinish:
				msg = new Bmmo::Messages::LevelFinish();
				break;
			case WhispersAbyss::Bmmo::OpCode::LoginRequestV2:
				msg = new Bmmo::Messages::LoginRequestV2();
				break;
			case WhispersAbyss::Bmmo::OpCode::LoginAcceptedV2:
				msg = new Bmmo::Messages::LoginAcceptedV2();
				break;
			case WhispersAbyss::Bmmo::OpCode::PlayerConnectedV2:
				msg = new Bmmo::Messages::PlayerConnectedV2();
				break;
			case WhispersAbyss::Bmmo::OpCode::CheatState:
				msg = new Bmmo::Messages::CheatState();
				break;
			case WhispersAbyss::Bmmo::OpCode::OwnedCheatState:
				msg = new Bmmo::Messages::OwnedCheatState();
				break;
			case WhispersAbyss::Bmmo::OpCode::CheatToggle:
				msg = new Bmmo::Messages::CheatToggle();
				break;
			case WhispersAbyss::Bmmo::OpCode::OwnedCheatToggle:
				msg = new Bmmo::Messages::OwnedCheatToggle();
				break;
			case WhispersAbyss::Bmmo::OpCode::KeyboardInput:
			case WhispersAbyss::Bmmo::OpCode::Ping:
			case WhispersAbyss::Bmmo::OpCode::None:
				mOutput->Printf("[Abyss] Invalid OpCode %d.", code);
				break;
			default:
				mOutput->Printf("[Abyss] Unknow OpCode %d.", code);
				break;
		}

		if (msg != NULL) {
			if (!msg->Deserialize(data)) {
				mOutput->Printf("[Abyss] Fail to parse message with OpCode %d.", code);
				delete msg;
				msg = NULL;
			}
		}
		return msg;
	}

	void WhispersAbyss::AbyssClient::InternalMsgProc(Bmmo::Messages::IMessage* msg) {
		switch (msg->GetOpCode()) {
			case WhispersAbyss::Bmmo::OpCode::LoginDenied:
			{
				mOutput->Printf("[Abyss] Server refuse our login request.");
				DisconnectSteam();
				mIsDead.store(true);
				break;
			}
			case WhispersAbyss::Bmmo::OpCode::LoginAcceptedV2:
			{
				mOutput->Printf("[Abyss] Login successfully.");
				break;
			}
			case WhispersAbyss::Bmmo::OpCode::OwnedCheatToggle:
			{
				mOutput->Printf("[Abyss] Server order cheat toggle. Auto response.");
				auto cheat_msg = new Bmmo::Messages::CheatState();
				cheat_msg->mCheated = true;
				cheat_msg->mNotify = false;
				Send(cheat_msg);
				break;
			}
			case WhispersAbyss::Bmmo::OpCode::None:
			case WhispersAbyss::Bmmo::OpCode::LoginRequest:
			case WhispersAbyss::Bmmo::OpCode::LoginAccepted:
			case WhispersAbyss::Bmmo::OpCode::PlayerDisconnected:
			case WhispersAbyss::Bmmo::OpCode::PlayerConnected:
			case WhispersAbyss::Bmmo::OpCode::Ping:
			case WhispersAbyss::Bmmo::OpCode::BallState:
			case WhispersAbyss::Bmmo::OpCode::OwnedBallState:
			case WhispersAbyss::Bmmo::OpCode::KeyboardInput:
			case WhispersAbyss::Bmmo::OpCode::Chat:
			case WhispersAbyss::Bmmo::OpCode::LevelFinish:
			case WhispersAbyss::Bmmo::OpCode::LoginRequestV2:
			case WhispersAbyss::Bmmo::OpCode::PlayerConnectedV2:
			case WhispersAbyss::Bmmo::OpCode::CheatState:
			case WhispersAbyss::Bmmo::OpCode::CheatToggle:
			case WhispersAbyss::Bmmo::OpCode::OwnedCheatState:
			default:
				break;
		}
	}

#pragma region steam work

	bool AbyssClient::ConnectSteam(std::string* addrs) {
		std::lock_guard<std::mutex> lockGuard(mSteamMutex);

		if (mSteamConnection != k_HSteamNetConnection_Invalid) return false;	// already connected

		// parse addrs
		SteamNetworkingIPAddr server_address{};
		if (!server_address.ParseString(addrs->c_str())) {
			return false;
		}

		// gen status callback opt
		SteamNetworkingConfigValue_t opt{};
		opt.SetPtr(k_ESteamNetworkingConfig_Callback_ConnectionStatusChanged,
			(void*)FuckValve::ProcSteamConnectionStatusChanged);

		// connect
		mSteamConnection = mSteamSockets->ConnectByIPAddress(server_address, 1, &opt);
		if (mSteamConnection == k_HSteamNetConnection_Invalid)
			return false;

		return true;
	}

	void WhispersAbyss::AbyssClient::RecvSteam(std::deque<Bmmo::Messages::IMessage*>* msg_list) {
		std::lock_guard<std::mutex> lockGuard(mSteamMutex);

		if (mSteamConnection == k_HSteamNetConnection_Invalid) return;

		int msg_count = mSteamSockets->ReceiveMessagesOnConnection(mSteamConnection, mSteamMessages, STEAM_MSG_CAPACITY);
		if (msg_count < 0)
			mOutput->FatalError("[Abyss] Get minus number in getting GNS message list.");
		else if (msg_count > 0) {
			// process message
			for (int i = 0; i < msg_count; ++i) {
				// write to stream
				mSteamBuffer.str("");
				mSteamBuffer.clear();
				mSteamBuffer.write((const char*)mSteamMessages[i]->m_pData, mSteamMessages[i]->m_cbSize);

				// try parsing
				// and add into list
				auto msg = CreateMessageFromStream(&mSteamBuffer);
				if (msg != NULL) {
					msg_list->push_back(msg);
				}

				// release steam msg data
				mSteamMessages[i]->Release();
			}
		}
	}

	void WhispersAbyss::AbyssClient::SendSteam(Bmmo::Messages::IMessage* msg) {
		std::lock_guard<std::mutex> lockGuard(mSteamMutex);

		if (mSteamConnection == k_HSteamNetConnection_Invalid) return;

		mSteamBuffer.str("");
		mSteamBuffer.clear();
		msg->Serialize(&mSteamBuffer);
		mSteamSockets->SendMessageToConnection(
			mSteamConnection,
			mSteamBuffer.str().c_str(),
			mSteamBuffer.str().size(),
			msg->GetMessageSendFlag(),
			nullptr
		);
	}

	void AbyssClient::DisconnectSteam() {
		std::lock_guard<std::mutex> lockGuard(mSteamMutex);

		if (mSteamConnection != k_HSteamNetConnection_Invalid) {
			mOutput->Printf("[Abyss] Actively closing connection...");
			mSteamSockets->CloseConnection(mSteamConnection, 0, "Goodbye from WhispersAbyss", true);
			//assert(connection == this->connection_);
			mSteamConnection = k_HSteamNetConnection_Invalid;
			//estate_ = k_ESteamNetworkingConnectionState_None;
		}
	}

	void AbyssClient::HandleSteamDebugOutput(ESteamNetworkingSocketsDebugOutputType eType, const char* pszMsg) {
		if (eType == k_ESteamNetworkingSocketsDebugOutputType_Bug) {
			mOutput->FatalError(pszMsg);
		} else {
			mOutput->Printf(pszMsg);
		}
	}

	void AbyssClient::HandleSteamConnectionStatusChanged(SteamNetConnectionStatusChangedCallback_t* pInfo) {
		auto state = pInfo->m_info.m_eState;

		switch (state) {
			case k_ESteamNetworkingConnectionState_Connecting:
			{
				mOutput->Printf("[Abyss] Connecting...");
				break;
			}
			case k_ESteamNetworkingConnectionState_Connected:
			{
				mOutput->Printf("[Abyss] Connected. Waiting for Login.");
				auto verfy_msg = new Bmmo::Messages::LoginRequestV2();
				verfy_msg->mNickname = mUsername.c_str();
				verfy_msg->mVersion.mMajor = 3;
				verfy_msg->mVersion.mMinor = 2;
				verfy_msg->mVersion.mSubminor = 2;
				verfy_msg->mVersion.mStage = Bmmo::PluginStage::Alpha;
				verfy_msg->mVersion.mBuild = 8;
				verfy_msg->mCheated = 1;

				Send(verfy_msg);
				break;
			}
			case k_ESteamNetworkingConnectionState_ClosedByPeer:
			{
				mOutput->Printf("[Abyss] Connection Lost. Reason %s(%d)",
					pInfo->m_info.m_szEndDebug,
					pInfo->m_info.m_eEndReason);

				mIsDead.store(true);
				break;
			}
			case k_ESteamNetworkingConnectionState_ProblemDetectedLocally:
			{
				mOutput->Printf("[Abyss] Local Error. Reason %s(%d)",
					pInfo->m_info.m_szEndDebug,
					pInfo->m_info.m_eEndReason);

				mIsDead.store(true);
				break;
			}
			case k_ESteamNetworkingConnectionState_None:
			{
				mOutput->Printf("[Abyss] Now in Disconnected status.");
				break;
			}
			case k_ESteamNetworkingConnectionState_FindingRoute:
			case k_ESteamNetworkingConnectionState_FinWait:
			case k_ESteamNetworkingConnectionState_Linger:
			case k_ESteamNetworkingConnectionState_Dead:
			case k_ESteamNetworkingConnectionState__Force32Bit:
			{
				break;
			}
			default:
				break;
		}
	}

	namespace FuckValve {

		AbyssClient* AbyssClientSingleton = NULL;
		std::mutex LockAbyssClientSingleton;

		void ProcSteamDebugOutput(ESteamNetworkingSocketsDebugOutputType eType, const char* pszMsg) {
			std::lock_guard<std::mutex> lockGuard(LockAbyssClientSingleton);

			if (AbyssClientSingleton != NULL)
				AbyssClientSingleton->HandleSteamDebugOutput(eType, pszMsg);
		}

		void ProcSteamConnectionStatusChanged(SteamNetConnectionStatusChangedCallback_t* pInfo) {
			std::lock_guard<std::mutex> lockGuard(LockAbyssClientSingleton);

			if (AbyssClientSingleton != NULL)
				AbyssClientSingleton->HandleSteamConnectionStatusChanged(pInfo);
		}

	}

#pragma endregion

}
