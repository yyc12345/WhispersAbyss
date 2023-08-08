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
	{}
	GnsInstance::~GnsInstance() {}

	void GnsInstance::Start() {
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
			mOutput->Printf("[Gns-#%" PRIu64 "] Resolving server address %s:%s", mIndex, address.c_str(), port.c_str());
			asio::io_context ioContext;
			asio::ip::udp::resolver udpResolver(ioContext);
			asio::error_code ec;
			asio::ip::udp::resolver::results_type results = udpResolver.resolve(address, port, ec);
			if (!ec) {
				bool flag_suc = false;
				for (const auto& i : results) {
					auto endpoint = i.endpoint();
					std::string connection_string;
					connection_string = endpoint.address().to_string() + ":" + std::to_string(endpoint.port());
					mOutput->Printf("[Gns-#%" PRIu64 "] Trying %s...", mIndex, connection_string.c_str());
					if (ConnectGns(connection_string)) {
						// success
						transition.SetTransitionError(false);
						flag_suc = true;
						break;
					}
				}

				if (!flag_suc) {
					// failed
					this->InternalStop();
					transition.SetTransitionError(true);
					mOutput->Printf("[Gns-#%" PRIu64 "] Run out of resolved address.", mIndex);
				}
			} else {
				// failed
				this->InternalStop();
				transition.SetTransitionError(true);
				mOutput->Printf("[Gns-#%" PRIu64 "] Fail to resolve hostname.", mIndex);
			}

			// stop context
			ioContext.stop();
		}).detach();
	}

	void GnsInstance::InternalStop() {
		// stop steam interface
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
		StateMachine::WorkBasedOnRunning work(mModuleStatus);
		if (!work.CanWork()) return;

		std::lock_guard locker(mSendMsgMutex);
		DequeOperations::MoveDeque(msg_list, mSendMsg);
	}

	void GnsInstance::Recv(std::deque<CommonMessage>& msg_list) {
		StateMachine::WorkBasedOnRunning work(mModuleStatus);
		if (!work.CanWork()) return;

		std::lock_guard locker(mRecvMsgMutex);
		DequeOperations::MoveDeque(mRecvMsg, msg_list);
	}

	void GnsInstance::CtxWorker(std::stop_token st) {
		std::deque<CommonMessage> incoming_message, outbound_message;

		while (!st.stop_requested()) {
			bool can_work = false;
			bool has_data = false;
			{
				StateMachine::WorkBasedOnRunning work(mModuleStatus);
				can_work = work.CanWork();
				if (work.CanWork()) {
					// ================= Message Sender =================
					// copy message to internal buffer
					{
						std::lock_guard locker(mSendMsgMutex);
						DequeOperations::MoveDeque(mSendMsg, outbound_message);
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
					{
						std::lock_guard locker(mRecvMsgMutex);
						DequeOperations::MoveDeque(mRecvMsg, incoming_message);
					}

				}
			}

			// if no work or no data
			// sleep
			if (!can_work || !has_data) {
				std::this_thread::sleep_for(SPIN_INTERVAL);
			}
		}

	}

	void GnsInstanceOperator::HandleConnectionStatusChanged(SteamNetConnectionStatusChangedCallback_t* pInfo) {
		auto state = pInfo->m_info.m_eState;

		switch (state) {
			case k_ESteamNetworkingConnectionState_Connecting:
			{
				mInstance->mOutput->Printf("[Gns-#%" PRIu64 "] Connecting...", mInstance->mIndex);
				break;
			}
			case k_ESteamNetworkingConnectionState_Connected:
			{
				mInstance->mOutput->Printf("[Gns-#%" PRIu64 "] Connected.", mInstance->mIndex);
				break;
			}
			case k_ESteamNetworkingConnectionState_ClosedByPeer:
			{
				mInstance->mOutput->Printf("[Gns-#%" PRIu64 "] Connection Lost. Reason %s(%d)",
					mInstance->mIndex,
					pInfo->m_info.m_szEndDebug,
					pInfo->m_info.m_eEndReason);

				// actively stop
				mInstance->Stop();
				break;
			}
			case k_ESteamNetworkingConnectionState_ProblemDetectedLocally:
			{
				mInstance->mOutput->Printf("[Gns-#%" PRIu64 "] Local Error. Reason %s(%d)",
					mInstance->mIndex,
					pInfo->m_info.m_szEndDebug,
					pInfo->m_info.m_eEndReason);

				// actively stop
				mInstance->Stop();
				break;
			}
			case k_ESteamNetworkingConnectionState_None:
			{
				mInstance->mOutput->Printf("[Gns-#%" PRIu64 "] Now in Disconnected status.", mInstance->mIndex);
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

		// set user custom data
		// IMPORTANT. related to callback dispatch
		auto token = mFactoryOperator->GetClientToken(this);
		SteamNetworkingConfigValue_t opt{};
		opt.SetInt64(
			k_ESteamNetworkingConfig_ConnectionUserData,
			token
		);
		// register client IMMEDIATELY
		mFactoryOperator->RegisterClient(token, this);

		// connect
		mGnsConnection = mFactoryOperator->GetGnsSockets()->ConnectByIPAddress(server_address, 1, &opt);
		if (mGnsConnection == k_HSteamNetConnection_Invalid) {
			// failed. unregister and return
			mFactoryOperator->UnregisterClient(token);
			return false;
		}

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
			mOutput->Printf("[Gns-#%" PRIu64 "] Closing connection...", mIndex);

			mFactoryOperator->GetGnsSockets()->CloseConnection(mGnsConnection, 0, "Goodbye from WhispersAbyss", false);
			mFactoryOperator->UnregisterClient(mFactoryOperator->GetClientToken(this));
			mGnsConnection = k_HSteamNetConnection_Invalid;

		}
	}

#pragma endregion

}
