#include "../include/celestia_server.hpp"

namespace WhispersAbyss {

	CelestiaServer::CelestiaServer(OutputHelper* output, uint16_t port) :
		mOutput(output), mModuleState(ModuleStates::Ready), mStateReporter(mModuleState), mIndexDistributor(), mPort(port),
		mIoContext(), mTcpAcceptor(mIoContext, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), mPort)),
		mTdIoCtx(), mTdDisposal(),
		mConnectionsMutex(), mDisposalConnsMutex(), mConnections(), mDisposalConns()
	{}
	CelestiaServer::~CelestiaServer() {}

	void CelestiaServer::Start() {
		std::thread([this]() -> void {
			// start transition
			StateMachineTransition transition(mModuleState, ModuleStates::Ready);
			if (!transition.NeedTransition()) return;

			// register worker
			this->RegisterAsyncWork();

			// preparing ctx worker
			this->mTdIoCtx = std::thread([this]() -> void {
				this->mIoContext.run();
			});

			// preparing disposal
			this->mTdDisposal = std::jthread(std::bind(&CelestiaServer::DisposalWorker, this, std::placeholders::_1));

			// end transition
			transition.To(ModuleStates::Running);
		}).detach();
	}

	void CelestiaServer::Stop() {
		std::thread([this]() -> void {
			// start transition
			StateMachineTransition transition(mModuleState, ModuleStates::Ready | ModuleStates::Running);
			if (!transition.NeedTransition()) return;

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

			// end transition
			transition.To(ModuleStates::Stopped);
		}).detach();
	}

	void CelestiaServer::GetConnections(std::deque<CelestiaGnosis*>& conn_list) {
		if (mModuleState.IsInState(ModuleStates::Running)) {
			std::lock_guard<std::mutex> locker(mConnectionsMutex);
			DequeOperations::MoveDeque(mConnections, conn_list);
		}
	}

	void CelestiaServer::ReturnConnections(std::deque<CelestiaGnosis*>& conn_list) {
		if (mModuleState.IsInState(ModuleStates::Running)) {
			std::lock_guard<std::mutex> locker(mDisposalConnsMutex);
			DequeOperations::MoveDeque(conn_list, mDisposalConns);
		}
	}

	void CelestiaServer::AcceptorWorker(std::error_code ec, asio::ip::tcp::socket socket) {
		// accept socket
		CelestiaGnosis* new_connection = new CelestiaGnosis(mOutput, mIndexDistributor.Get(), std::move(socket));
		{
			std::lock_guard<std::mutex> locker(mConnectionsMutex);
			new_connection->Start();
			mConnections.push_back(new_connection);
		}

		// accept new connection
		RegisterAsyncWork();
	}

	void CelestiaServer::RegisterAsyncWork() {
		mTcpAcceptor.async_accept(std::bind(
			&CelestiaServer::AcceptorWorker, this, std::placeholders::_1, std::placeholders::_2
		));
	}

	void CelestiaServer::DisposalWorker(std::stop_token st) {
		std::deque<CelestiaGnosis*> cache;
		while (!st.stop_requested()) {
			// copy first
			{
				std::lock_guard<std::mutex> locker(mDisposalConnsMutex);
				DequeOperations::MoveDeque(mDisposalConns, cache);
			}

			// stop them one by one until all of them stopped.
			for (auto& ptr : mConnections) {
				ptr->Stop();
				ptr->mStateReporter.SpinUntil(ModuleStates::Stopped);
			}
			// free every one
			DequeOperations::FreeDeque(mConnections);
		}
	}


}
