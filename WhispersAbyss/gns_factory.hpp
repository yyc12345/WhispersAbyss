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

	using GnsUserData_t = int64;

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

		GnsUserData_t GetClientToken(GnsInstance* instance);
		void RegisterClient(GnsUserData_t token, GnsInstance* instance);
		void UnregisterClient(GnsUserData_t token);
	private:
		GnsFactory* mFactory;
	};
	class GnsFactory {
		friend class GnsFactoryOperator;
	private:
		OutputHelper* mOutput;
		StateMachine::StateMachineCore mModuleStatus;
		GnsFactoryOperator mSelfOperator;

		std::jthread mTdDisposal, mTdPoll;

		IndexDistributor mIndexDistributor;
		ISteamNetworkingSockets* mGnsSockets;

		std::map<GnsUserData_t, GnsInstanceOperator> mRouterMap;
		// Lock shared when use router. Lock unique when change router.
		std::shared_mutex mRouterMutex;

		std::mutex mDisposalConnsMutex;
		std::deque<GnsInstance*> mDisposalConns;
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
		void DisposalWorker(std::stop_token st);

		void ProcDebugOutput(ESteamNetworkingSocketsDebugOutputType eType, const char* pszMsg);
		void ProcConnectionStatusChanged(SteamNetConnectionStatusChangedCallback_t* pInfo);
	};

}
