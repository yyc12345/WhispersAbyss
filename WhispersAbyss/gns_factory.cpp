#include "gns_factory.hpp"
#include "gns_instance.hpp"
#include <steam/steamnetworkingsockets.h>
#include <steam/isteamnetworkingutils.h>
#include <functional>
#include <shared_mutex>

namespace WhispersAbyss {

#pragma region Init Kill works

	/// <summary>
	/// Lock shared when calling function. Lock unique when replacing function ptr.
	/// </summary>
	static std::shared_mutex s_GnsFuncPtrsMutex;
	static std::function<void(ESteamNetworkingSocketsDebugOutputType, const char*)> s_fpGnsDebug(nullptr);
	static std::function<void(SteamNetConnectionStatusChangedCallback_t*)> s_fpGnsStatusChanged(nullptr);
	static void ProcDebugOutput(ESteamNetworkingSocketsDebugOutputType eType, const char* pszMsg) {
		std::shared_lock locker(s_GnsFuncPtrsMutex);
		if (s_fpGnsDebug) s_fpGnsDebug(eType, pszMsg);
	}
	static void ProcConnectionStatusChanged(SteamNetConnectionStatusChangedCallback_t* pInfo) {
		std::shared_lock locker(s_GnsFuncPtrsMutex);
		if (s_fpGnsStatusChanged) s_fpGnsStatusChanged(pInfo);
	}

	GnsFactory::GnsFactory(OutputHelper* output) :
		mOutput(output), mModuleStatus(StateMachine::Ready), mStatusReporter(mModuleStatus), mSelfOperator(this),
		mIndexDistributor(), mGnsSockets(nullptr),
		mRouterMap(), mRouterMutex()
	{
		std::thread([this]() -> void {
			// start transition
			StateMachine::TransitionInitializing transition(mModuleStatus);
			if (!transition.CanTransition()) return;

			// initialize steam lib
			SteamDatagramErrMsg err_msg;
			if (!GameNetworkingSockets_Init(nullptr, err_msg)) {
				this->mOutput->FatalError(OutputHelper::Component::GnsFactory, NO_INDEX, "GameNetworkingSockets_Init failed. %s", err_msg);
				transition.SetTransitionError(true);
				return;
			}

			// bind to static variables
			{
				std::unique_lock locker(s_GnsFuncPtrsMutex);
				if (s_fpGnsDebug || s_fpGnsStatusChanged) {
					this->mOutput->FatalError(OutputHelper::Component::GnsFactory, NO_INDEX, "Multiple instance of GnsFactory!");
					transition.SetTransitionError(true);
					return;
				}

				s_fpGnsDebug = std::bind(&GnsFactory::ProcDebugOutput, this, std::placeholders::_1, std::placeholders::_2);
				s_fpGnsStatusChanged = std::bind(&GnsFactory::ProcConnectionStatusChanged, this, std::placeholders::_1);
			}

			// bind static functions to gns
			SteamNetworkingUtils()->SetDebugOutputFunction(
				k_ESteamNetworkingSocketsDebugOutputType_Msg,
				&WhispersAbyss::ProcDebugOutput
			);
			SteamNetworkingUtils()->SetGlobalCallback_SteamNetConnectionStatusChanged(
				&WhispersAbyss::ProcConnectionStatusChanged
			);

			// get sockets
			this->mGnsSockets = SteamNetworkingSockets();

			// start disposal
			this->mTdDisposal = std::jthread(std::bind(&GnsFactory::DisposalWorker, this, std::placeholders::_1));
			// start polling
			this->mTdPoll = std::jthread([this](std::stop_token st) -> void {
				while (!st.stop_requested()) {
					this->mGnsSockets->RunCallbacks();
					std::this_thread::sleep_for(std::chrono::milliseconds(SPIN_INTERVAL));
				}
			});

			this->mOutput->Printf(OutputHelper::Component::GnsFactory, NO_INDEX, "GameNetworkingSockets_Init done.");

			// end transition
			transition.SetTransitionError(false);
		}).detach();

		mOutput->Printf(OutputHelper::Component::GnsFactory, NO_INDEX, "Instance created.");
	}
	GnsFactory::~GnsFactory() {
		Stop();
		mStatusReporter.SpinUntil(StateMachine::Stopped);

		mOutput->Printf(OutputHelper::Component::GnsFactory, NO_INDEX, "Instance disposed.");
	}

	void GnsFactory::Stop() {
		std::thread([this]() -> void {
			// start transition
			StateMachine::TransitionStopping transition(mModuleStatus);
			if (!transition.CanTransition()) return;

			// stop polling
			if (this->mTdPoll.joinable()) {
				this->mTdPoll.request_stop();
				this->mTdPoll.join();
			}
			// set socket
			this->mGnsSockets = nullptr;

			// wait disposal worker exit
			if (this->mTdDisposal.joinable()) {
				this->mTdDisposal.request_stop();
				this->mTdDisposal.join();
			}

			// destroy steam work
			std::this_thread::sleep_for(std::chrono::milliseconds(500));
			GameNetworkingSockets_Kill();

			mOutput->Printf(OutputHelper::Component::GnsFactory, NO_INDEX, "GameNetworkingSockets_Kill done.");

		}).detach();
	}

#pragma endregion

#pragma region Callback dispatch

	void GnsFactory::ProcDebugOutput(ESteamNetworkingSocketsDebugOutputType eType, const char* pszMsg) {
		if (eType == k_ESteamNetworkingSocketsDebugOutputType_Bug) mOutput->FatalError(pszMsg);
		else mOutput->Printf(pszMsg);
	}
	void GnsFactory::ProcConnectionStatusChanged(SteamNetConnectionStatusChangedCallback_t* pInfo) {
		std::shared_lock locker(mRouterMutex);

		auto pair = mRouterMap.find(pInfo->m_info.m_nUserData);
		if (pair != mRouterMap.end()) {
			pair->second.HandleConnectionStatusChanged(pInfo);
		}
	}

#pragma endregion

#pragma region Factory Operator

	ISteamNetworkingSockets* GnsFactoryOperator::GetGnsSockets() {
		StateMachine::WorkBasedOnRunning work(mFactory->mModuleStatus);
		if (!work.CanWork()) {
			mFactory->mOutput->FatalError(OutputHelper::Component::GnsFactory, NO_INDEX, "Out of work time calling GnsFactoryOperator::GetGnsSockets()!");
			return nullptr;
		}

		return mFactory->mGnsSockets;
	}

	GnsUserData_t GnsFactoryOperator::GetClientToken(GnsInstance* instance) {
		return reinterpret_cast<intptr_t>(instance);
	}

	void GnsFactoryOperator::RegisterClient(GnsUserData_t token, GnsInstance* instance) {
		std::unique_lock locker(mFactory->mRouterMutex);
		mFactory->mRouterMap.emplace(token, GnsInstanceOperator(instance));
	}

	void GnsFactoryOperator::UnregisterClient(GnsUserData_t token) {
		std::unique_lock locker(mFactory->mRouterMutex);
		mFactory->mRouterMap.erase(token);
	}

#pragma endregion


	GnsInstance* GnsFactory::GetConnections(std::string& server_url) {
		StateMachine::WorkBasedOnRunning work(mModuleStatus);
		if (!work.CanWork()) {
			mOutput->FatalError(OutputHelper::Component::GnsFactory, NO_INDEX, "Out of work time calling GnsFactory::GetConnections()!");
			return nullptr;
		}

		GnsInstance* instance = new GnsInstance(
			mOutput, 
			mIndexDistributor.Get(),
			&mSelfOperator,
			server_url
		);
		return instance;
	}

	void GnsFactory::ReturnConnections(GnsInstance* conn) {
		StateMachine::WorkBasedOnRunning work(mModuleStatus);
		if (!work.CanWork()) {
			mOutput->FatalError(OutputHelper::Component::GnsFactory, NO_INDEX, "Out of work time calling GnsFactory::ReturnConnections()!");
			return;
		}

		std::lock_guard locker(mDisposalConnsMutex);
		mDisposalConns.push_back(conn);
	}
	
	void GnsFactory::DisposalWorker(std::stop_token st) {
		std::deque<GnsInstance*> cache;
		while (true) {
			// copy first
			{
				std::lock_guard<std::mutex> locker(mDisposalConnsMutex);
				DequeOperations::MoveDeque(mDisposalConns, cache);
			}

			// no item
			if (cache.empty()) {
				// quit if ordered.
				if (st.stop_requested()) return;
				// otherwise sleep
				std::this_thread::sleep_for(std::chrono::milliseconds(DISPOSAL_INTERVAL));
				continue;
			}

			// stop them one by one until all of them stopped.
			// then return index and free them.
			for (auto& ptr : cache) {
				ptr->Stop();
				ptr->mStatusReporter.SpinUntil(StateMachine::Stopped);
				mIndexDistributor.Return(ptr->mIndex);
				delete ptr;
			}
		}
	}

}
