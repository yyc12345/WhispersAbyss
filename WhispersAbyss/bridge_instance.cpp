#include "bridge_instance.hpp"
#include "gns_instance.hpp"
#include "tcp_instance.hpp"

namespace WhispersAbyss {

	BridgeInstance::BridgeInstance(OutputHelper* output, TcpFactory* tcp_factory, GnsFactory* gns_factory, TcpInstance* tcp_instance, IndexDistributor::Index_t index) :
		mOutput(output), mModuleStatus(StateMachine::Ready), mStatusReporter(mModuleStatus), mIndexDistributor(), mIndex(index),
		mTcpFactory(tcp_factory), mGnsFactory(gns_factory), mTcpInstance(tcp_instance), mGnsInstance(nullptr),
		mRecvTcp(0u), mSendTcp(0u), mRecvGns(0u), mSendGns(0u),
		mTdCtx()
	{
		std::thread([this]() -> void {
			// start transition
			StateMachine::TransitionInitializing transition(mModuleStatus);
			if (!transition.CanTransition()) return;

			// try get status or order from tcp connection
			std::string url;
			while (true) {
				if (mTcpInstance->mStatusReporter.IsInState(StateMachine::Stopped)) {
					// crash before getting it.
					mOutput->Printf(OutputHelper::Component::BridgeInstance, mIndex, "Unexpected Tcp instance crash before ordering URL.");
					transition.SetTransitionError(true);
					this->InternalStop();
					return;
				} else {
					// try get ordered url
					url = mTcpInstance->GetOrderedUrl();
					if (url.empty()) {
						std::this_thread::sleep_for(SPIN_INTERVAL);
					}
				}
			}

			// ok, we got it.
			// create gns instance
			mGnsInstance = mGnsFactory->GetConnections(url);

			// start context workder
			this->mTdCtx = std::jthread(std::bind(&BridgeInstance::CtxWorker, this, std::placeholders::_1));

			// end transition
			transition.SetTransitionError(false);
		}).detach();

		mOutput->Printf(OutputHelper::Component::BridgeInstance, mIndex, "Instance created.");
	}

	BridgeInstance::~BridgeInstance() {
		Stop();
		mStatusReporter.SpinUntil(StateMachine::Stopped);

		mOutput->Printf(OutputHelper::Component::BridgeInstance, mIndex, "Instance disposed.");
	}

	void BridgeInstance::InternalStop() {

		// stop context
		if (mTdCtx.joinable()) {
			mTdCtx.request_stop();
			mTdCtx.join();
		}

		// kill 2 instance
		if (mTcpInstance != nullptr) {
			mTcpInstance->Stop();
			mTcpInstance->mStatusReporter.SpinUntil(StateMachine::Stopped);
			mTcpFactory->ReturnConnections(mTcpInstance);
			mTcpInstance = nullptr;
		}
		if (mGnsInstance != nullptr) {
			mGnsInstance->Stop();
			mGnsInstance->mStatusReporter.SpinUntil(StateMachine::Stopped);
			mGnsFactory->ReturnConnections(mGnsInstance);
			mGnsInstance = nullptr;
		}

	}
	void BridgeInstance::Stop() {
		std::thread([this]() -> void {
			// start transition
			StateMachine::TransitionStopping transition(mModuleStatus);
			if (!transition.CanTransition()) return;

			// call internal stop
			this->InternalStop();
		}).detach();
	}

	BridgeInstanceProfile BridgeInstance::ReportStatus() {
		return BridgeInstanceProfile{
			mRecvTcp.load(), mSendTcp.load(), mRecvGns.load(), mSendGns.load(),
			mIndex, (mTcpInstance ? mTcpInstance->mIndex : 0u), (mGnsInstance ? mGnsInstance->mIndex : 0u)
		};
	}

	void BridgeInstance::CtxWorker(std::stop_token st) {
		std::deque<CommonMessage> msgtcp2gns, msggns2tcp;
		uint64_t count, allcount;
		bool can_work;

		while (!st.stop_requested()) {
			// check work status
			can_work = false;
			allcount = 0u;
			{
				StateMachine::WorkBasedOnRunning work(mModuleStatus);
				can_work = work.CanWork();
				if (work.CanWork()) {

					// check 2 instance status
					if (mGnsInstance->mStatusReporter.IsInState(StateMachine::Stopped) ||
						mTcpInstance->mStatusReporter.IsInState(StateMachine::Stopped)) {
						this->Stop();
						return;
					}

					// move msg data
					// and collect data count
					// ==================== Tcp 2 Gns ====================
					count = msgtcp2gns.size();
					mTcpInstance->Recv(msgtcp2gns);
					mRecvTcp.fetch_add(msgtcp2gns.size() - count);
					allcount += msgtcp2gns.size() - count;

					count = msgtcp2gns.size();
					mGnsInstance->Send(msgtcp2gns);
					mSendGns.fetch_add(count - msgtcp2gns.size());
					allcount += count - msgtcp2gns.size();

					// ==================== Gns 2 Tco ====================
					count = msggns2tcp.size();
					mGnsInstance->Recv(msggns2tcp);
					mRecvGns.fetch_add(msggns2tcp.size() - count);
					allcount += msggns2tcp.size() - count;

					count = msggns2tcp.size();
					mTcpInstance->Send(msggns2tcp);
					mSendTcp.fetch_add(count - msggns2tcp.size());
					allcount += count - msggns2tcp.size();

				}
			}

			// if no data or no work, sleep a while
			if (!can_work || allcount == 0u) {
				std::this_thread::sleep_for(SPIN_INTERVAL);
			}

		}
	}

}
