#include "tcp_factory.hpp"

namespace WhispersAbyss {

	TcpFactory::TcpFactory(OutputHelper* output, uint16_t port) :
		mOutput(output), mModuleStatus(StateMachine::Ready), mModuleStatusReporter(mModuleStatus), mIndexDistributor(), mPort(port),
		mIoContext(), mTcpAcceptor(mIoContext, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), mPort)),
		mTdIoCtx(), mTdDisposal(),
		mConnectionsMutex(), mDisposalConnsMutex(), mConnections(), mDisposalConns()
	{}
	TcpFactory::~TcpFactory() {}

	void TcpFactory::Start() {
		std::thread([this]() -> void {
			// start transition
			StateMachine::TransitionInitializing transition(mModuleStatus);
			if (!transition.CanTransition()) return;

			// register worker
			this->RegisterAsyncWork();

			// preparing ctx worker
			this->mTdIoCtx = std::thread([this]() -> void {
				this->mIoContext.run();
			});

			// preparing disposal
			this->mTdDisposal = std::jthread(std::bind(&TcpFactory::DisposalWorker, this, std::placeholders::_1));

			// end transition
			transition.SetTransitionError(false);
		}).detach();
	}

	void TcpFactory::Stop() {
		std::thread([this]() -> void {
			// start transition
			StateMachine::TransitionStopping transition(mModuleStatus);
			if (!transition.CanTransition()) return;

			// try stop worker
			this->mIoContext.stop();

			// waiting for thread over
			if (this->mTdIoCtx.joinable()) {
				this->mTdIoCtx.join();
			}

			// move all pending connections into disposal list
			{
				std::scoped_lock locker(mConnectionsMutex, mDisposalConnsMutex);
				DequeOperations::MoveDeque(mConnections, mDisposalConns);
			}

			// wait disposal worker exit
			if (this->mTdDisposal.joinable()) {
				this->mTdDisposal.request_stop();
				this->mTdDisposal.join();
			}
		}).detach();
	}

	void TcpFactory::GetConnections(std::deque<TcpInstance*>& conn_list) {
		StateMachine::WorkBasedOnRunning work(mModuleStatus);
		if (!work.CanWork()) return;

		std::lock_guard<std::mutex> locker(mConnectionsMutex);
		DequeOperations::MoveDeque(mConnections, conn_list);
	}

	void TcpFactory::ReturnConnections(std::deque<TcpInstance*>& conn_list) {
		StateMachine::WorkBasedOnRunning work(mModuleStatus);
		if (!work.CanWork()) return;

		std::lock_guard<std::mutex> locker(mDisposalConnsMutex);
		DequeOperations::MoveDeque(conn_list, mDisposalConns);
	}

	void TcpFactory::AcceptorWorker(std::error_code ec, asio::ip::tcp::socket socket) {
		// accept socket
		TcpInstance* new_connection = new TcpInstance(mOutput, mIndexDistributor.Get(), std::move(socket));
		{
			std::lock_guard<std::mutex> locker(mConnectionsMutex);
			new_connection->Start();
			mConnections.push_back(new_connection);
		}

		// accept new connection
		RegisterAsyncWork();
	}

	void TcpFactory::RegisterAsyncWork() {
		mTcpAcceptor.async_accept(std::bind(
			&TcpFactory::AcceptorWorker, this, std::placeholders::_1, std::placeholders::_2
		));
	}

	void TcpFactory::DisposalWorker(std::stop_token st) {
		std::deque<TcpInstance*> cache;
		while (!st.stop_requested()) {
			// copy first
			{
				std::lock_guard<std::mutex> locker(mDisposalConnsMutex);
				DequeOperations::MoveDeque(mDisposalConns, cache);
			}

			// stop them one by one until all of them stopped.
			// then return index and free them.
			for (auto& ptr : mConnections) {
				ptr->Stop();
				ptr->mStatusReporter.SpinUntil(StateMachine::Stopped);
				mIndexDistributor.Return(ptr->mIndex);
				delete ptr;
			}
		}
	}


}