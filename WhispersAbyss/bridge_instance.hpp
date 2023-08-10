#pragma once

#include "others_helper.hpp"
#include "state_machine.hpp"
#include "tcp_factory.hpp"
#include "gns_factory.hpp"
#include <atomic>
#include <thread>

namespace WhispersAbyss {

	struct InstanceStatus {
		bool mIsExisted;
		IndexDistributor::Index_t mIndex;
		StateMachine::State_t mState;
		bool mIsInTransition;
	};
	struct BridgeInstanceProfile {
		uint64_t mRecvTcp, mSendTcp, mRecvGns, mSendGns;
		InstanceStatus mSelfStatus, mTcpStatus, mGnsStatus;
	};

	class BridgeInstance {
	private:
		OutputHelper* mOutput;
		StateMachine::StateMachineCore mModuleStatus;
		IndexDistributor mIndexDistributor;

		TcpFactory* mTcpFactory;
		GnsFactory* mGnsFactory;

		TcpInstance* mTcpInstance;
		GnsInstance* mGnsInstance;

		std::atomic_uint64_t mRecvTcp, mSendTcp, mRecvGns, mSendGns;

		std::jthread mTdCtx;
	public:
		StateMachine::StateMachineReporter mStatusReporter;
		IndexDistributor::Index_t mIndex;

	public:
		BridgeInstance(OutputHelper* output, TcpFactory* tcp_factory, GnsFactory* gns_factory, TcpInstance* tcp_instance, IndexDistributor::Index_t index);
		BridgeInstance(const BridgeInstance& rhs) = delete;
		BridgeInstance(BridgeInstance&& rhs) = delete;
		~BridgeInstance();

		void Stop();
		BridgeInstanceProfile ReportStatus();
	private:
		void InternalStop();
		void CtxWorker(std::stop_token st);
	};

}
