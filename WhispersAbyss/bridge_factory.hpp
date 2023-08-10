#pragma once

#include "others_helper.hpp"
#include "state_machine.hpp"
#include "tcp_factory.hpp"
#include "gns_factory.hpp"
#include "bridge_instance.hpp"
#include <thread>
#include <deque>
#include <mutex>

namespace WhispersAbyss {

	class BridgeFactory {
	private:
		OutputHelper* mOutput;
		StateMachine::StateMachineCore mModuleStatus;
		IndexDistributor mIndexDistributor;

		TcpFactory mTcpFactory;
		GnsFactory mGnsFactory;

		std::mutex mInstancesMutex, mDisposalsMutex;
		std::deque<BridgeInstance*> mInstances, mDisposals;

		std::jthread mTdCtx, mTdDisposal;
	public:
		StateMachine::StateMachineReporter mStatusReporter;

	public:
		BridgeFactory(OutputHelper* output, uint16_t tcp_port);
		BridgeFactory(const BridgeFactory& rhs) = delete;
		BridgeFactory(BridgeFactory&& rhs) = delete;
		~BridgeFactory();

		void Stop();
		void ReportStatus();
	private:
		void InternalStop();
		void CtxWorker(std::stop_token st);
		void DisposalWorker(std::stop_token st);
	};

}
