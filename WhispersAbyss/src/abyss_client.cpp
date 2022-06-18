#include "../include/abyss_client.hpp"
#include "../include/global_define.hpp"
#include <steam/steamnetworkingsockets.h>
#include <steam/isteamnetworkingutils.h>

namespace WhispersAbyss {

	// init static variables
	ISteamNetworkingSockets* AbyssClient::smSteamSockets = NULL;
	uint64_t AbyssClient::smIndexDistributor = 0;

	AbyssClient::AbyssClient(OutputHelper* output, const char* server) :
		mIndex(smIndexDistributor++),

		mStatus(ModuleStatus::Initializing), mStopCtx(false),
		
		mOutput(output),
		mRecvMsg(), mSendMsg(),
		mRecvMsgMutex(), mSendMsgMutex(), 
		
		mSteamMutex(),
		mSteamConnection(k_HSteamNetConnection_Invalid), mSteamBuffer(),

		mServerUrl(server),
		mTdCtx(), mTdConnect(), mTdStop() {

		// check requirements
		if (smSteamSockets == NULL) throw std::logic_error("Call constructor of AbyssClient before AbyssClient::Init.");

		// when creating instance, we should connect server immediately
		// start connect worker and notify main worker
		mStopCtx.store(false);
		mStatus.store(ModuleStatus::Ready);
		mTdCtx = std::thread(&AbyssClient::CtxWorker, this);
		mTdConnect = std::thread(&AbyssClient::ConnectWorker, this);

		mOutput->Printf("[Abyss-#%ld] Connection instance created.", mIndex);
	}

	AbyssClient::~AbyssClient() {
		Bmmo::DeleteCachedMessage(&mRecvMsg);
		Bmmo::DeleteCachedMessage(&mSendMsg);

		mOutput->Printf("[Abyss-#%ld] Connection instance disposed.", mIndex);
	}

	void AbyssClient::Init(OutputHelper* output) {
		if (smSteamSockets != NULL) throw std::logic_error("Wrong call of AbyssClient::Init.");

		// setup fuck valve output
		FuckValve::LockValveOutput.lock();
		FuckValve::ValveOutput = output;
		FuckValve::LockValveOutput.unlock();

		// initialize steam lib
		SteamDatagramErrMsg err_msg;
		if (!GameNetworkingSockets_Init(nullptr, err_msg))
			output->FatalError("[Abyss] GameNetworkingSockets_Init failed.  %s", err_msg);
		SteamNetworkingUtils()->SetDebugOutputFunction(
			k_ESteamNetworkingSocketsDebugOutputType_Msg,
			&FuckValve::ProcSteamDebugOutput
		);
		smSteamSockets = SteamNetworkingSockets();

		FuckValve::ValveOutput->Printf("[Abyss] GameNetworkingSockets_Init done.");
	}

	void AbyssClient::Dispose() {
		if (smSteamSockets == NULL) throw std::logic_error("Wrong call of AbyssClient::Dispose.");

		// fuck valve output do not need to be removed
		// this operation is useless

		// destroy steam work
		std::this_thread::sleep_for(std::chrono::milliseconds(500));
		GameNetworkingSockets_Kill();

		FuckValve::ValveOutput->Printf("[Abyss] GameNetworkingSockets_Kill done.");
	}

	void AbyssClient::Stop() {
		// check requirement
		
		auto status = mStatus.load();
		if (status == ModuleStatus::Stopping || status == ModuleStatus::Stopped) return;
		mStatus.store(ModuleStatus::Stopping);

		// start stop worker
		mTdStop = std::thread(&AbyssClient::StopWorker, this);
	}

	void AbyssClient::CtxWorker() {
		std::deque<Bmmo::Message*> incoming_message, outbound_message;

		while (!mStopCtx.load()) {
			std::this_thread::sleep_for(std::chrono::milliseconds(10));

			// ==================================
			// poll new message
			// ready msg cache
			incoming_message.clear();
			RecvSteam(&incoming_message);
			// push cache into list
			mRecvMsgMutex.lock();
			Bmmo::MoveCachedMessage(&incoming_message, &mRecvMsg);
			mRecvMsgMutex.unlock();

			// ==================================
			// poll status change
			smSteamSockets->RunCallbacks();

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
				outbound_message.pop_front();
				SendSteam(msg);
			}

		}
	}

	void AbyssClient::ConnectWorker() {
		// split url
		size_t urlpos = mServerUrl.find(":");
		std::string address = mServerUrl.substr(0, urlpos);
		std::string port = mServerUrl.substr(urlpos + 1);

		// solve ip
		mOutput->Printf("[Abyss-#%ld] Resolving server address \"%s : %s\"", mIndex, address.c_str(), port.c_str());
		asio::io_context ioContext;
		asio::ip::udp::resolver udpResolver(ioContext);
		asio::error_code ec;
		asio::ip::udp::resolver::results_type results = udpResolver.resolve(address, port, ec);
		if (!ec) {
			for (const auto& i : results) {
				auto endpoint = i.endpoint();
				std::string connection_string = endpoint.address().to_string() + ":" + std::to_string(endpoint.port());
				mOutput->Printf("[Abyss-#%ld] Trying %s...", mIndex, connection_string.c_str());
				if (ConnectSteam(&connection_string)) {
					mStatus.store(ModuleStatus::Running);
					break;	// success
				}
			}
		} else {
			mOutput->Printf("[Abyss-#%ld] Fail to resolve hostname.", mIndex);
		}

		ioContext.stop();
	}

	void AbyssClient::StopWorker() {
		// stop steam interface and ctx worker
		DisconnectSteam();
		mStopCtx.store(true);

		// wait exit
		if (mTdCtx.joinable())
			mTdCtx.join();
		if (mTdConnect.joinable())
			mTdConnect.join();

		mStatus.store(ModuleStatus::Stopped);
	}

	void AbyssClient::Send(std::deque<Bmmo::Message*>* manager_list) {
		size_t msg_size;

		// push data
		mSendMsgMutex.lock();
		Bmmo::MoveCachedMessage(manager_list, &mSendMsg);
		msg_size = mSendMsg.size();
		mSendMsgMutex.unlock();

		if (msg_size >= WARNING_CAPACITY)
			mOutput->Printf("[Abyss-#%ld] Message list reach warning level: %d", mIndex, msg_size);
		if (msg_size >= NUKE_CAPACITY) {
			mOutput->FatalError("[Abyss-#%ld] Message list reach nuke level: %d. This abyss will be nuked!!!", mIndex, msg_size);
		}
	}

	void AbyssClient::Recv(std::deque<Bmmo::Message*>* manager_list) {
		mRecvMsgMutex.lock();
		Bmmo::MoveCachedMessage(&mRecvMsg, manager_list);
		mRecvMsgMutex.unlock();
	}

	void AbyssClient::HandleSteamConnectionStatusChanged(SteamNetConnectionStatusChangedCallback_t* pInfo) {
		auto state = pInfo->m_info.m_eState;

		switch (state) {
			case k_ESteamNetworkingConnectionState_Connecting:
			{
				mOutput->Printf("[Abyss-#%ld] Connecting...", mIndex);
				break;
			}
			case k_ESteamNetworkingConnectionState_Connected:
			{
				mOutput->Printf("[Abyss-#%ld] Connected.", mIndex);
				break;
			}
			case k_ESteamNetworkingConnectionState_ClosedByPeer:
			{
				mOutput->Printf("[Abyss-#%ld] Connection Lost. Reason %s(%d)",
					mIndex,
					pInfo->m_info.m_szEndDebug,
					pInfo->m_info.m_eEndReason);

				// actively stop
				Stop();
				break;
			}
			case k_ESteamNetworkingConnectionState_ProblemDetectedLocally:
			{
				mOutput->Printf("[Abyss-#%ld] Local Error. Reason %s(%d)",
					mIndex,
					pInfo->m_info.m_szEndDebug,
					pInfo->m_info.m_eEndReason);

				// actively stop
				Stop();
				break;
			}
			case k_ESteamNetworkingConnectionState_None:
			{
				mOutput->Printf("[Abyss-#%ld] Now in Disconnected status.", mIndex);
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
		mSteamConnection = smSteamSockets->ConnectByIPAddress(server_address, 1, &opt);
		if (mSteamConnection == k_HSteamNetConnection_Invalid)
			return false;

		FuckValve::RegisterClient(mSteamConnection, this);
		return true;
	}

	void WhispersAbyss::AbyssClient::RecvSteam(std::deque<Bmmo::Message*>* msg_list) {
		std::lock_guard<std::mutex> lockGuard(mSteamMutex);

		if (mSteamConnection == k_HSteamNetConnection_Invalid) return;

		int msg_count = smSteamSockets->ReceiveMessagesOnConnection(mSteamConnection, mSteamMessages, STEAM_MSG_CAPACITY);
		if (msg_count < 0)
			mOutput->FatalError("[Abyss-#%ld] Get minus number in getting GNS message list.", mIndex);
		else if (msg_count > 0) {
			// process message
			for (int i = 0; i < msg_count; ++i) {
				// parse data
				// and add into list
				auto msg = new Bmmo::Message();
				msg->ReadGnsData(mSteamMessages[i]->m_pData, mSteamMessages[i]->m_nFlags, mSteamMessages[i]->m_cbSize);
				msg_list->push_back(msg);

				// release steam msg data
				mSteamMessages[i]->Release();
			}
		}
	}

	void WhispersAbyss::AbyssClient::SendSteam(Bmmo::Message* msg) {
		std::lock_guard<std::mutex> lockGuard(mSteamMutex);

		if (mSteamConnection == k_HSteamNetConnection_Invalid) return;

		smSteamSockets->SendMessageToConnection(
			mSteamConnection,
			msg->GetData(),
			msg->GetDataLen(),
			msg->GetGnsSendFlag(),
			nullptr
		);

		delete msg;
	}

	void AbyssClient::DisconnectSteam() {
		std::lock_guard<std::mutex> lockGuard(mSteamMutex);
		
		if (mSteamConnection != k_HSteamNetConnection_Invalid) {
			mOutput->Printf("[Abyss-#%ld] Actively closing connection...", mIndex);
			smSteamSockets->CloseConnection(mSteamConnection, 0, "Goodbye from WhispersAbyss", true);
			//assert(connection == this->connection_);
			FuckValve::UnregisterClient(mSteamConnection);
			mSteamConnection = k_HSteamNetConnection_Invalid;
			//estate_ = k_ESteamNetworkingConnectionState_None;
		}
	}

	//void AbyssClient::HandleSteamDebugOutput(ESteamNetworkingSocketsDebugOutputType eType, const char* pszMsg) {
	//	if (eType == k_ESteamNetworkingSocketsDebugOutputType_Bug) {
	//		mOutput->FatalError(pszMsg);
	//	} else {
	//		mOutput->Printf(pszMsg);
	//	}
	//}

	namespace FuckValve {

		OutputHelper* ValveOutput = NULL;
		std::unordered_map<HSteamNetConnection, AbyssClient*> ValveClientRouter;
		std::mutex LockValveClientRouter, LockValveOutput;


		void ProcSteamDebugOutput(ESteamNetworkingSocketsDebugOutputType eType, const char* pszMsg) {
			std::lock_guard<std::mutex> lockGuard(LockValveOutput);

			if (eType == k_ESteamNetworkingSocketsDebugOutputType_Bug) {
				ValveOutput->FatalError(pszMsg);
			} else {
				ValveOutput->Printf(pszMsg);
			}
		}


		void RegisterClient(HSteamNetConnection conn, AbyssClient* instance) {
			std::lock_guard<std::mutex> lockGuard(LockValveClientRouter);
			ValveClientRouter.emplace(conn, instance);
		}

		void UnregisterClient(HSteamNetConnection conn) {
			std::lock_guard<std::mutex> lockGuard(LockValveClientRouter);
			ValveClientRouter.erase(conn);
		}

		void ProcSteamConnectionStatusChanged(SteamNetConnectionStatusChangedCallback_t* pInfo) {
			std::lock_guard<std::mutex> lockGuard(LockValveClientRouter);

			auto pair = ValveClientRouter.find(pInfo->m_hConn);
			if (pair != ValveClientRouter.end()) {
				pair->second->HandleSteamConnectionStatusChanged(pInfo);
			}
		}

	}

#pragma endregion

}
