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
#include "../include/global_define.hpp"

namespace WhispersAbyss {


	class AbyssClient {
	public:
		std::atomic_bool mIsRunning;

		AbyssClient(OutputHelper* output, const char* server, const char* username);
		~AbyssClient();

		void Start();
		void Stop();

		void Send(std::deque<Bmmo::Messages::IMessage*>* manager_list);
		void Recv(std::deque<Bmmo::Messages::IMessage*>* manager_list);

		void HandleSteamDebugOutput(ESteamNetworkingSocketsDebugOutputType eType, const char* pszMsg);
		void HandleSteamConnectionStatusChanged(SteamNetConnectionStatusChangedCallback_t* pInfo);
	private:
		OutputHelper* mOutput;
		std::mutex mRecvMsgMutex, mSendMsgMutex;
		std::deque<Bmmo::Messages::IMessage*> mRecvMsg, mSendMsg;
		std::string mServerUrl, mUsername;
		std::thread mTdCtx, mTdConnect;
		std::atomic_bool mStopCtx;
		Bmmo::Messages::IMessage* CreateMessageFromStream(std::stringstream* data);
		void ConnectWorker();
		void CtxWorker();

		std::mutex mSteamMutex;
		ISteamNetworkingSockets* mSteamSockets;
		HSteamNetConnection mSteamConnection;
		ISteamNetworkingMessage* mSteamMessages[STEAM_MSG_CAPACITY];
		bool ConnectSteam(std::string* addrs);
		void CloseSteam();
	};

	namespace FuckValve {

		extern std::mutex LockAbyssClientSingleton;
		extern AbyssClient* AbyssClientSingleton;

		void ProcSteamDebugOutput(ESteamNetworkingSocketsDebugOutputType eType, const char* pszMsg);
		void ProcSteamConnectionStatusChanged(SteamNetConnectionStatusChangedCallback_t* pInfo);

	}
}


