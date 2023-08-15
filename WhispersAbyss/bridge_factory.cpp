#include "bridge_factory.hpp"

namespace WhispersAbyss {

	BridgeFactory::BridgeFactory(OutputHelper* output, uint16_t tcp_port) :
		mOutput(output), mModuleStatus(StateMachine::Ready), mStatusReporter(mModuleStatus), mIndexDistributor(),
		mTcpFactory(output, tcp_port), mGnsFactory(output),
		mInstances(), mInstancesMutex(),
		mTdCtx(),
		mDisposal()
	{
		std::thread([this]() -> void {
			// start transition
			StateMachine::TransitionInitializing transition(mModuleStatus);
			if (!transition.CanTransition()) return;

			// wait them
			CountDownTimer waiting(MODULE_WAITING_INTERVAL);
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

			// start disposal
			this->mDisposal.Start([this](BridgeInstance* instance) -> void {
				if (!instance->mStatusReporter.IsInState(StateMachine::Stopped)) instance->Stop();
				instance->mStatusReporter.SpinUntil(StateMachine::Stopped);
				this->mIndexDistributor.Return(instance->mIndex);
				delete instance;
			});

			// end transition
			transition.SetTransitionError(false);
		}).detach();

		mOutput->Printf(OutputHelper::Component::BridgeFactory, NO_INDEX, "Factory created.");
	}
	BridgeFactory::~BridgeFactory() {
		if (!mStatusReporter.IsInState(StateMachine::Stopped)) Stop();
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
			std::lock_guard locker(mInstancesMutex);
			this->mDisposal.Move(mInstances);
		}
		this->mDisposal.Stop();

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

		// show profiles
		// reserve string first
		// (profiles.size() * 5 + 1) is the total used lines. every profile will use 5 lines in average.
		// (3 * 20 + 1) is the character used by one line. every line have 3 column and each use 20 chars in average.
		// 128 is padding. just to make sure no extra allocation.
		std::string buf;
		buf.reserve((profiles.size() * 5 + 1) * (3 * 20 + 1) + 128);
		constexpr const char cInTrans[] = "(Trans)";
		constexpr const char cNotInTrans[] = "";
		for (auto& profile : profiles) {
			buf.append("+--------------+--------------+--------------+\n");
			CommonOpers::AppendStrF(buf, "|%12" PRIu64 "->|  Tcp to Gns  |->%-12" PRIu64 "|\n",
				profile.mRecvTcp, profile.mSendGns);
			
			if (profile.mTcpStatus.mIsExisted) {
				CommonOpers::AppendStrF(buf, "|Tcp#%-10" PRIu64, profile.mTcpStatus.mIndex);
			} else buf.append("|NO TCP       ");
			if (profile.mSelfStatus.mIsExisted) {
				CommonOpers::AppendStrF(buf, "|Bridge#%-7" PRIu64, profile.mSelfStatus.mIndex);
			} else buf.append("|NO BRIDGE    ");
			if (profile.mGnsStatus.mIsExisted) {
				CommonOpers::AppendStrF(buf, "|Gns#%-10" PRIu64, profile.mGnsStatus.mIndex);
			} else buf.append("|NO GNS       ");
			buf.append("|\n");

			if (profile.mTcpStatus.mIsExisted) {
				CommonOpers::AppendStrF(buf, "|%7s%7s",
					CommonOpers::State2String(profile.mTcpStatus.mState), (profile.mTcpStatus.mIsInTransition ? cInTrans : cNotInTrans));
			} else buf.append("|             ");
			if (profile.mSelfStatus.mIsExisted) {
				CommonOpers::AppendStrF(buf, "|%7s%7s",
					CommonOpers::State2String(profile.mSelfStatus.mState), (profile.mSelfStatus.mIsInTransition ? cInTrans : cNotInTrans));
			} else buf.append("|             ");
			if (profile.mGnsStatus.mIsExisted) {
				CommonOpers::AppendStrF(buf, "|%7s%7s",
					CommonOpers::State2String(profile.mGnsStatus.mState), (profile.mGnsStatus.mIsInTransition ? cInTrans : cNotInTrans));
			} else buf.append("|             ");
			buf.append("|\n");

			CommonOpers::AppendStrF(buf, "|%12" PRIu64 "<-|  Gns to Tcp  |<-%-12" PRIu64 "|\n",
				profile.mSendTcp, profile.mRecvGns);
		}
		if (!profiles.empty()) {
			buf.append("+--------------+--------------+--------------+\n");
		} else {
			buf = "No available profile.";
		}

		// print profile
		mOutput->RawPrintf(buf.c_str());
	}

	void BridgeFactory::CtxWorker(std::stop_token st) {
		std::deque<TcpInstance*> new_incoming;
		std::deque<BridgeInstance*> cache;

		while (!st.stop_requested()) {
			// if not in running. spin
			if (!mStatusReporter.IsInState(StateMachine::Running)) {
				std::this_thread::sleep_for(SPIN_INTERVAL);
				continue;
			}


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
			new_incoming.clear();

			{
				std::lock_guard locker(mInstancesMutex);
				// process old bridges
				for (auto& instance : mInstances) {
					if (instance->mStatusReporter.IsInState(StateMachine::Stopped)) {
						mDisposal.Move(instance);
					} else {
						cache.push_back(instance);
					}
				}
				// replace instances
				mInstances.clear();
				CommonOpers::MoveDeque(cache, mInstances);
			}

			// in any case, sleep. sleep like disposal long time.
			std::this_thread::sleep_for(BRIDGE_INTERVAL);
		}
	}

}