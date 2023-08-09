#pragma once

#include "state_machine.hpp"
#include "others_helper.hpp"
#include "messages.hpp"
#include <steam/steamnetworkingtypes.h>
#include <steam/isteamnetworkingsockets.h>
#include <deque>
#include <mutex>
#include <string>

namespace WhispersAbyss {

	class GnsInstance;
	class GnsFactory;
	class GnsFactoryOperator;

	class GnsInstanceOperator {
	public:
		GnsInstanceOperator(GnsInstance* instance) : mInstance(instance) {}
		GnsInstanceOperator(const GnsInstanceOperator& rhs) : mInstance(rhs.mInstance) {}
		GnsInstanceOperator(GnsInstanceOperator&& rhs) noexcept : mInstance(rhs.mInstance) {}
		~GnsInstanceOperator() {}

		void HandleConnectionStatusChanged(SteamNetConnectionStatusChangedCallback_t* pInfo);
	private:
		GnsInstance* mInstance;
	};
	class GnsInstance {
		friend class GnsInstanceOperator;
	private:
		OutputHelper* mOutput;
		StateMachine::StateMachineCore mModuleStatus;
		GnsFactoryOperator* mFactoryOperator;

		std::mutex mRecvMsgMutex, mSendMsgMutex;
		std::deque<CommonMessage> mRecvMsg, mSendMsg;
		std::string mServerUrl;
		std::jthread mTdCtx;

		HSteamNetConnection mGnsConnection;
		ISteamNetworkingMessage* mGnsMessages[STEAM_MSG_CAPACITY];
		std::string mGnsBuffer;

	public:
		StateMachine::StateMachineReporter mStatusReporter;
		IndexDistributor::Index_t mIndex;

	public:
		GnsInstance(OutputHelper* output, IndexDistributor::Index_t index, GnsFactoryOperator* factory_oper, std::string& server);
		GnsInstance(const GnsInstance& rhs) = delete;
		GnsInstance(GnsInstance&& rhs) = delete;
		~GnsInstance();

		void Stop();

		void Send(std::deque<CommonMessage>& msg_list);
		void Recv(std::deque<CommonMessage>& msg_list);
		void CheckSize(size_t msg_size, bool is_recv);
	private:
		void InternalStop();
		void CtxWorker(std::stop_token st);

		bool ConnectGns(std::string& addrs);
		void RecvGns(std::deque<CommonMessage>& msg_list);
		void SendGns(std::deque<CommonMessage>& msg_list);
		void DisconnectGns();
	};
}


