#pragma once

#include "output_helper.hpp"
#include "state_machine.hpp"
#include "others_helper.hpp"
#include "messages.hpp"
#include <steam/steamnetworkingtypes.h>
#include <steam/isteamnetworkingsockets.h>
#include <deque>
#include <mutex>

namespace WhispersAbyss {

	class GnsFactory;

	class GnsInstance {
		friend class GnsFactory;
	private:
		OutputHelper* mOutput;
		StateMachine::StateMachineCore mModuleStatus;

		std::mutex mRecvMsgMutex, mSendMsgMutex;
		std::deque<CommonMessage> mRecvMsg, mSendMsg;
		std::string mServerUrl;
		std::thread mTdCtx;


	public:
		StateMachine::StateMachineReporter mStatusReporter;
		IndexDistributor::Index_t mIndex;

	public:
		GnsInstance(OutputHelper* output, std::string& server);
		~GnsInstance();

		void Start();
		void Stop();

		void Send(std::deque<CommonMessage>& manager_list);
		void Recv(std::deque<CommonMessage>& manager_list);
	protected:
		void HandleConnectionStatusChanged(SteamNetConnectionStatusChangedCallback_t* pInfo);
	private:
		void ConnectWorker();
		void StopWorker();
		void CtxWorker();

		//std::mutex mSteamMutex;
		//HSteamNetConnection mSteamConnection;
		//ISteamNetworkingMessage* mSteamMessages[STEAM_MSG_CAPACITY];
		//std::string mSteamBuffer;
		//bool ConnectSteam(std::string* addrs);
		//void RecvSteam(std::deque<Bmmo::Message*>* msg_list);
		//void SendSteam(Bmmo::Message* msg);
		//void DisconnectSteam();
	};
}


