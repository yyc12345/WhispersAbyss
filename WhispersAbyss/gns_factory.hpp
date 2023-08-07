#pragma once

#include "output_helper.hpp"
#include "state_machine.hpp"
#include "others_helper.hpp"
#include "messages.hpp"
#include <steam/steamnetworkingtypes.h>
#include <steam/isteamnetworkingsockets.h>
#include <map>
#include <mutex>
#include <shared_mutex>
#include <deque>

namespace WhispersAbyss {

	using GnsUserData_t = int64;
	class GnsInstance;

	class GnsFactory {
		friend class GnsInstance;
	private:
		OutputHelper* mOutput;
		StateMachine::StateMachineCore mModuleStatus;

		std::jthread mTdDisposal;

		IndexDistributor mIndexDistributor;
		ISteamNetworkingSockets* mGnsSockets;

		std::map<GnsUserData_t, GnsInstance*> mRouterMap;
		// Lock shared when use router. Lock unique when change router.
		std::shared_mutex mRouterMutex;

		std::mutex mDisposalConnsMutex;
		std::deque<GnsInstance*> mDisposalConns;
	public:
		StateMachine::StateMachineReporter mModuleStatusReporter;

	public:
		GnsFactory(OutputHelper* output);
		~GnsFactory();

		void Start();
		void Stop();

		GnsInstance* GetConnections(std::string& server_url);
		void ReturnConnections(GnsInstance* conn);
	protected:
		void RegisterClient(GnsInstance* instance);
		void UnregisterClient(GnsInstance* instance);

	private:
		void DisposalWorker(std::stop_token st);

		void ProcDebugOutput(ESteamNetworkingSocketsDebugOutputType eType, const char* pszMsg);
		void ProcConnectionStatusChanged(SteamNetConnectionStatusChangedCallback_t* pInfo);
	};

}
