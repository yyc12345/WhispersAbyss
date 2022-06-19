#pragma once

#include <sdkddkver.h>	// need by asio
#include "asio.hpp"
#include "output_helper.hpp"
#include <atomic>
#include <deque>
#include <mutex>
#include "bmmo_message.hpp"
#include <steam/steamnetworkingtypes.h>
#include <steam/isteamnetworkingsockets.h>
#include "global_define.hpp"

namespace WhispersAbyss {


	class AbyssClient {
	public:
		ModuleStatus mStatus;
		std::mutex mStatusMutex;
		ModuleStatus GetConnStatus();

		AbyssClient(OutputHelper* output, const char* server);
		~AbyssClient();

		static ISteamNetworkingSockets* smSteamSockets;
		static uint64_t smIndexDistributor;
		static void Init(OutputHelper* output);
		static void Dispose();

		void Stop();
		void Send(std::deque<Bmmo::Message*>* manager_list);
		void Recv(std::deque<Bmmo::Message*>* manager_list);

		void HandleSteamConnectionStatusChanged(SteamNetConnectionStatusChangedCallback_t* pInfo);

	private:
		uint64_t mIndex;
		OutputHelper* mOutput;
		std::mutex mRecvMsgMutex, mSendMsgMutex;
		std::deque<Bmmo::Message*> mRecvMsg, mSendMsg;
		std::string mServerUrl;
		std::thread mTdCtx, mTdConnect, mTdStop;
		std::atomic_bool mStopCtx;
		void ConnectWorker();
		void StopWorker();
		void CtxWorker();


		std::mutex mSteamMutex;
		HSteamNetConnection mSteamConnection;
		ISteamNetworkingMessage* mSteamMessages[STEAM_MSG_CAPACITY];
		std::string mSteamBuffer;
		bool ConnectSteam(std::string* addrs);
		void RecvSteam(std::deque<Bmmo::Message*>* msg_list);
		void SendSteam(Bmmo::Message* msg);
		void DisconnectSteam();
	};

	namespace FuckValve {

		extern OutputHelper* ValveOutput;
		extern std::unordered_map<HSteamNetConnection, AbyssClient*> ValveClientRouter;
		extern std::mutex LockValveClientRouter, LockValveOutput;

		void RegisterClient(HSteamNetConnection conn, AbyssClient* instance);
		void UnregisterClient(HSteamNetConnection conn);
		void ProcSteamDebugOutput(ESteamNetworkingSocketsDebugOutputType eType, const char* pszMsg);
		void ProcSteamConnectionStatusChanged(SteamNetConnectionStatusChangedCallback_t* pInfo);

	}
}


