#include "bridge_factory.hpp"

namespace WhispersAbyss {

	BridgeFactory::BridgeFactory(OutputHelper* output, uint16_t tcp_port) :
		mOutput(output), mModuleStatus(StateMachine::Ready), mStatusReporter(mModuleStatus), mIndexDistributor(),
		mTcpFactory(output, tcp_port), mGnsFactory(output),
		mInstances(), mInstancesMutex(), mDisposals(), mDisposalsMutex(),
		mTdCtx()
	{
		std::thread([this]() -> void {
			// start transition
			StateMachine::TransitionInitializing transition(mModuleStatus);
			if (!transition.CanTransition()) return;

			// wait them
			CountDownTimer waiting(WAITING_COUNTDOWN);
			while (true) {
				if (mTcpFactory.mStatusReporter.IsInState(StateMachine::Stopped) ||
					mGnsFactory.mStatusReporter.IsInState(StateMachine::Stopped)) {
					mOutput->Printf(OutputHelper::Component::BridgeFactory, NO_INDEX, "Unexpected crash of TcpFactory or GnsFactory.");
					this->InternalStop();
					transition.SetTransitionError(true);
					return;
				}

				if (mTcpFactory.mStatusReporter.IsInState(StateMachine::Running) &&
					mGnsFactory.mStatusReporter.IsInState(StateMachine::Running)) {
					break;	// all factory are running
				}

				// otherwise check time and sleep
				if (waiting.HasRunOutOfTime()) {
					mOutput->Printf(OutputHelper::Component::BridgeFactory, NO_INDEX, "Run out of time of waiting factory running.");
					this->InternalStop();
					transition.SetTransitionError(true);
					return;
				}
				std::this_thread::sleep_for(SPIN_INTERVAL);
			}

			// Start context
			this->mTdCtx = std::jthread(std::bind(&BridgeFactory::CtxWorker, this, std::placeholders::_1));

			// end transition
			transition.SetTransitionError(false);
		}).detach();

		mOutput->Printf(OutputHelper::Component::BridgeFactory, NO_INDEX, "Factory created.");
	}
	BridgeFactory::~BridgeFactory() {
		Stop();
		mStatusReporter.SpinUntil(StateMachine::Stopped);

		mOutput->Printf(OutputHelper::Component::BridgeFactory, NO_INDEX, "Factory disposed.");
	}

	void BridgeFactory::InternalStop() {
		// stop context first
		if (mTdCtx.joinable()) {
			mTdCtx.request_stop();
			mTdCtx.join();
		}

		// move all bridge instance to disposal and wait disposal exit
		{
			std::scoped_lock locker(mInstancesMutex, mDisposalsMutex);
			DequeOperations::MoveDeque(mInstances, mDisposals);
		}
		if (mTdDisposal.joinable()) {
			mTdDisposal.request_stop();
			mTdDisposal.join();
		}

		// stop 2 factory
		mTcpFactory.Stop();
		mTcpFactory.mStatusReporter.SpinUntil(StateMachine::Stopped);
		mGnsFactory.Stop();
		mGnsFactory.mStatusReporter.SpinUntil(StateMachine::Stopped);

	}
	void BridgeFactory::Stop() {
		std::thread([this]() -> void {
			// start transition
			StateMachine::TransitionStopping transition(mModuleStatus);
			if (!transition.CanTransition()) return;

			// run internal one
			this->InternalStop();
		}).detach();
	}

	void BridgeFactory::ReportStatus() {
		std::deque<BridgeInstanceProfile> profiles;
		{
			std::lock_guard locker(mInstancesMutex);
			for (auto& instance : mInstances) {
				profiles.emplace_back(instance->ReportStatus());
			}
		}

		// todo: show profiles
		//for (auto& profile : profiles) {
		//	mOutput->RawPrintf("Tcp R/W: %" PRIu64 "/%" PRIu64 "");
		//}
	}

	void BridgeFactory::CtxWorker(std::stop_token st) {
		std::deque<TcpInstance*> new_incoming;
		std::deque<BridgeInstance*> cache;

		while (!st.stop_requested()) {
			{
				StateMachine::WorkBasedOnRunning work(mModuleStatus);
				if (work.CanWork()) {

					// get new tcp incoming
					mTcpFactory.GetConnections(new_incoming);
					for (auto& ptr : new_incoming) {
						cache.push_back(new BridgeInstance(
							mOutput,
							&mTcpFactory,
							&mGnsFactory,
							ptr,
							mIndexDistributor.Get()
						));
					}

					{
						std::scoped_lock locker(mInstancesMutex, mDisposalsMutex);
						// process old bridges
						for (auto& instance : mInstances) {
							if (instance->mStatusReporter.IsInState(StateMachine::Stopped)) {
								mDisposals.push_back(instance);
							} else {
								cache.push_back(instance);
							}
						}
						// replace instances
						mInstances.clear();
						DequeOperations::MoveDeque(cache, mInstances);
					}

				}
			}

			// in any case, sleep. sleep like disposal long time.
			std::this_thread::sleep_for(DISPOSAL_INTERVAL);
		}
	}

	void BridgeFactory::DisposalWorker(std::stop_token st) {
		std::deque<BridgeInstance*> cache;
		while (true) {
			// copy first
			{
				std::lock_guard<std::mutex> locker(mDisposalsMutex);
				DequeOperations::MoveDeque(mDisposals, cache);
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