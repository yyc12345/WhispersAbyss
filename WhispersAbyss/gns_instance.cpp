#include <sdkddkver.h>	// need by asio
#include "asio.hpp"
#include <steam/steamnetworkingsockets.h>
#include <steam/isteamnetworkingutils.h>
#include "gns_instance.hpp"
#include "gns_factory.hpp"

namespace WhispersAbyss {

	GnsInstance::GnsInstance(OutputHelper* output, IndexDistributor::Index_t index, GnsFactoryOperator* factory_oper, std::string& server) :
		mOutput(output), mModuleStatus(StateMachine::Ready), mStatusReporter(mModuleStatus),
		mIndex(index), mServerUrl(server), mFactoryOperator(factory_oper),
		mRecvMsgMutex(), mSendMsgMutex(),
		mRecvMsg(), mSendMsg(),
		mTdCtx(),
		mGnsConnection(k_HSteamNetConnection_Invalid), mGnsMessages(), mGnsBuffer()
	{
		std::thread([this]() -> void {
			// start transition
			StateMachine::TransitionInitializing transition(mModuleStatus);
			if (!transition.CanTransition()) return;

			// split url
			std::string address, port;
			size_t urlpos = mServerUrl.find(":");
			address = mServerUrl.substr(0, urlpos);
			port = mServerUrl.substr(urlpos + 1);

			// solve ip
			mOutput->Printf(OutputHelper::Component::GnsInstance, mIndex, "Resolving server address %s:%s", address.c_str(), port.c_str());
			asio::io_context ioContext;
			asio::ip::udp::resolver udpResolver(ioContext);
			asio::error_code ec;
			asio::ip::udp::resolver::results_type results = udpResolver.resolve(address, port, ec);
			bool is_success = false;
			if (!ec) {
				for (const auto& i : results) {
					auto endpoint = i.endpoint();
					std::string connection_string;
					connection_string = endpoint.address().to_string() + ":" + std::to_string(endpoint.port());
					mOutput->Printf(OutputHelper::Component::GnsInstance, mIndex, "Trying %s...", connection_string.c_str());
					if (ConnectGns(connection_string)) {
						// success
						is_success = true;
						transition.SetTransitionError(false);
						break;
					}
				}

				if (!is_success) {
					// failed
					this->InternalStop();
					transition.SetTransitionError(true);
					mOutput->Printf(OutputHelper::Component::GnsInstance, mIndex, "Run out of resolved address.");
				}
			} else {
				// failed
				this->InternalStop();
				transition.SetTransitionError(true);
				mOutput->Printf(OutputHelper::Component::GnsInstance, mIndex, "Fail to resolve hostname.");
			}

			// stop context
			ioContext.stop();

			// if success, start context worker
			if (is_success) {
				this->mTdCtx = std::jthread(std::bind(&GnsInstance::CtxWorker, this, std::placeholders::_1));
			}

		}).detach();

		mOutput->Printf(OutputHelper::Component::GnsInstance, mIndex, "Instance created.");
	}
	GnsInstance::~GnsInstance() {
		if (!mStatusReporter.IsInState(StateMachine::Stopped)) Stop();
		mStatusReporter.SpinUntil(StateMachine::Stopped);

		mOutput->Printf(OutputHelper::Component::GnsInstance, mIndex, "Instance disposed.");
	}

	void GnsInstance::InternalStop() {
		// stop steam interface
		mOutput->Printf(OutputHelper::Component::GnsInstance, mIndex, "Closing connection...");
		DisconnectGns();

		// stop ctx worker
		if (mTdCtx.joinable()) {
			mTdCtx.request_stop();
			mTdCtx.join();
		}

	}
	void GnsInstance::Stop() {
		std::thread([this]() -> void {
			// start transition
			StateMachine::TransitionStopping transition(mModuleStatus);
			if (!transition.CanTransition()) return;

			// call internal one
			this->InternalStop();
		}).detach();
	}

	void GnsInstance::Send(std::deque<CommonMessage>& msg_list) {
		if (!mStatusReporter.IsInState(StateMachine::Running)) return;

		// move msg
		// get size outside of bracket.
		size_t msg_list_size;
		{
			std::lock_guard locker(mSendMsgMutex);
			CommonOpers::MoveDeque(msg_list, mSendMsg);
			msg_list_size = mSendMsg.size();
		}
		CheckSize(msg_list_size, false);
	}

	void GnsInstance::Recv(std::deque<CommonMessage>& msg_list) {
		if (!mStatusReporter.IsInState(StateMachine::Running)) return;

		std::lock_guard locker(mRecvMsgMutex);
		CommonOpers::MoveDeque(mRecvMsg, msg_list);
	}

	void GnsInstance::CheckSize(size_t msg_size, bool is_recv) {
		const char* side = is_recv ? "Recv" : "Send";

		if (msg_size >= NUKE_CAPACITY) {
			mOutput->Printf(OutputHelper::Component::GnsInstance, mIndex, "%s message list reach nuke level: %zu. Nuke instance!!!", side, msg_size);
			Stop();
		} else if (msg_size >= WARNING_CAPACITY) {
			mOutput->Printf(OutputHelper::Component::GnsInstance, mIndex, "%s message list reach warning level: %zu", side, msg_size);
		}
	}

	void GnsInstance::CtxWorker(std::stop_token st) {
		std::deque<CommonMessage> incoming_message, outbound_message;

		while (!st.stop_requested()) {
			// if not in work. spin until it can work.
			if (!mStatusReporter.IsInState(StateMachine::Running)) {
				std::this_thread::sleep_for(SPIN_INTERVAL);
				continue;
			}

			bool has_data = false;
			// ================= Message Sender =================
			// copy message to internal buffer
			{
				std::lock_guard locker(mSendMsgMutex);
				CommonOpers::MoveDeque(mSendMsg, outbound_message);
			}
			// process it if has message
			if (!outbound_message.empty()) {
				has_data = true;
				SendGns(outbound_message);
			}


			// ================= Message Receiver =================
			// receive msg and check size
			size_t count = incoming_message.size();
			RecvGns(incoming_message);
			if (count != incoming_message.size()) {
				has_data = true;
			}
			// try push message from internal buffer
			// and check size
			size_t msg_list_size;
			{
				std::lock_guard locker(mRecvMsgMutex);
				CommonOpers::MoveDeque(incoming_message, mRecvMsg);
				msg_list_size = mRecvMsg.size();
			}
			CheckSize(msg_list_size, true);

			// if this round no data. sleep more time
			if (!has_data) {
				std::this_thread::sleep_for(SPIN_INTERVAL);
				continue;
			}
		}

	}

	void GnsInstanceOperator::HandleConnectionStatusChanged(SteamNetConnectionStatusChangedCallback_t* pInfo) {
		// check whether this msg belong to me
		// because main handler may throw some callback not belong to this instance as a fallback mechanisim.
		if (pInfo->m_hConn != mInstance->mGnsConnection) return;

		switch (pInfo->m_info.m_eState) {
			case k_ESteamNetworkingConnectionState_Connecting:
			{
				mInstance->mOutput->Printf(OutputHelper::Component::GnsInstance, mInstance->mIndex, "Connecting...");
				break;
			}
			case k_ESteamNetworkingConnectionState_Connected:
			{
				mInstance->mOutput->Printf(OutputHelper::Component::GnsInstance,  mInstance->mIndex, "Connected.");
				break;
			}
			case k_ESteamNetworkingConnectionState_ClosedByPeer:
			{
				mInstance->mOutput->Printf(OutputHelper::Component::GnsInstance, mInstance->mIndex, 
					"Connection Lost. Reason %s(%d)",
					pInfo->m_info.m_szEndDebug,
					pInfo->m_info.m_eEndReason
				);

				// actively stop
				mInstance->Stop();
				break;
			}
			case k_ESteamNetworkingConnectionState_ProblemDetectedLocally:
			{
				mInstance->mOutput->Printf(OutputHelper::Component::GnsInstance, mInstance->mIndex, 
					"Local Error. Reason %s(%d)",
					pInfo->m_info.m_szEndDebug,
					pInfo->m_info.m_eEndReason
				);

				// actively stop
				mInstance->Stop();
				break;
			}
			case k_ESteamNetworkingConnectionState_None:
			{
				mInstance->mOutput->Printf(OutputHelper::Component::GnsInstance, mInstance->mIndex, "Now in Disconnected status.");
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

	bool GnsInstance::ConnectGns(std::string& addrs) {
		if (mGnsConnection != k_HSteamNetConnection_Invalid) return false;	// already connected

		// parse addrs
		SteamNetworkingIPAddr server_address{};
		if (!server_address.ParseString(addrs.c_str())) {
			return false;
		}

		// set empty opt
		SteamNetworkingConfigValue_t opt{};

		// connect
		mGnsConnection = mFactoryOperator->GetGnsSockets()->ConnectByIPAddress(server_address, 0, &opt);
		if (mGnsConnection == k_HSteamNetConnection_Invalid) {
			// failed. return.
			return false;
		}

		// register client
		mFactoryOperator->RegisterClient(mGnsConnection, this);

		return true;
	}

	void GnsInstance::RecvGns(std::deque<CommonMessage>& msg_list) {
		if (mGnsConnection == k_HSteamNetConnection_Invalid) return;

		int msg_count = mFactoryOperator->GetGnsSockets()->ReceiveMessagesOnConnection(mGnsConnection, mGnsMessages, STEAM_MSG_CAPACITY);
		// process message
		for (int i = 0; i < msg_count; ++i) {
			// parse data
			// and add into list
			CommonMessage msg;
			msg.SetGnsData(mGnsMessages[i]->m_pData, mGnsMessages[i]->m_nFlags, mGnsMessages[i]->m_cbSize);
			msg_list.emplace_back(std::move(msg));

			// release steam msg data
			mGnsMessages[i]->Release();
		}
	}

	void GnsInstance::SendGns(std::deque<CommonMessage>& msg_list) {
		if (mGnsConnection == k_HSteamNetConnection_Invalid) return;

		for (auto& msg : msg_list) {
			mFactoryOperator->GetGnsSockets()->SendMessageToConnection(
				mGnsConnection,
				msg.GetCommonData(),
				msg.GetCommonDataLen(),
				msg.GetGnsSendFlag(),
				nullptr
			);
		}
		msg_list.clear();
	}

	void GnsInstance::DisconnectGns() {
		if (mGnsConnection != k_HSteamNetConnection_Invalid) {
			mFactoryOperator->GetGnsSockets()->CloseConnection(mGnsConnection, 0, "Goodbye from WhispersAbyss", false);
			mFactoryOperator->UnregisterClient(mGnsConnection);
			mGnsConnection = k_HSteamNetConnection_Invalid;

		}
	}

#pragma endregion

}
