#pragma once

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

	// using GnsUserData_t = int64;

	class GnsFactory;
	class GnsInstance;
	class GnsInstanceOperator;

	class GnsFactoryOperator {
	public:
		GnsFactoryOperator(GnsFactory* factory) : mFactory(factory) {}
		GnsFactoryOperator(const GnsFactoryOperator& rhs) : mFactory(rhs.mFactory) {}
		GnsFactoryOperator(GnsFactoryOperator&& rhs) noexcept : mFactory(rhs.mFactory) {}
		~GnsFactoryOperator() {}

		ISteamNetworkingSockets* GetGnsSockets();

		void RegisterClient(HSteamNetConnection token, GnsInstance* instance);
		void UnregisterClient(HSteamNetConnection token);
	private:
		GnsFactory* mFactory;
	};
	class GnsFactory {
		friend class GnsFactoryOperator;
	private:
		OutputHelper* mOutput;
		StateMachine::StateMachineCore mModuleStatus;
		IndexDistributor mIndexDistributor;

		std::jthread mTdPoll;

		GnsFactoryOperator mSelfOperator;
		ISteamNetworkingSockets* mGnsSockets;

		std::map<HSteamNetConnection, GnsInstanceOperator> mRouterMap;
		// Lock shared when use router. Lock unique when change router.
		std::shared_mutex mRouterMutex;

		DisposalHelper<GnsInstance*> mDisposal;
	public:
		StateMachine::StateMachineReporter mStatusReporter;

	public:
		GnsFactory(OutputHelper* output);
		GnsFactory(const GnsFactory& rhs) = delete;
		GnsFactory(GnsFactory&& rhs) = delete;
		~GnsFactory();

		void Stop();

		GnsInstance* GetConnections(std::string& server_url);
		void ReturnConnections(GnsInstance* conn);
	protected:

	private:
		void ProcDebugOutput(ESteamNetworkingSocketsDebugOutputType eType, const char* pszMsg);
		void ProcConnectionStatusChanged(SteamNetConnectionStatusChangedCallback_t* pInfo);
	};

}
