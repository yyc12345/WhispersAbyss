#include "tcp_factory.hpp"

namespace WhispersAbyss {

	TcpFactory::TcpFactory(OutputHelper* output, uint16_t port) :
		mOutput(output), mModuleStatus(StateMachine::Ready), mStatusReporter(mModuleStatus), mIndexDistributor(), mPort(port),
		mIoContext(), mTcpAcceptor(mIoContext, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), mPort)),
		mTdIoCtx(),
		mConnectionsMutex(), mConnections(),
		mDisposal()
	{
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
			this->mDisposal.Start([this](TcpInstance* instance) -> void {
				instance->Stop();
				instance->mStatusReporter.SpinUntil(StateMachine::Stopped);
				this->mIndexDistributor.Return(instance->mIndex);
				delete instance;
			});

			// end transition
			transition.SetTransitionError(false);
		}).detach();

		mOutput->Printf(OutputHelper::Component::TcpFactory, NO_INDEX, "Factory created.");
	}
	TcpFactory::~TcpFactory() {
		Stop();
		mStatusReporter.SpinUntil(StateMachine::Stopped);

		mOutput->Printf(OutputHelper::Component::TcpFactory, NO_INDEX, "Factory disposed.");
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
				std::lock_guard locker(mConnectionsMutex);
				mDisposal.Move(mConnections);
			}

			// wait disposal worker exit
			mDisposal.Stop();
		}).detach();
	}

	void TcpFactory::GetConnections(std::deque<TcpInstance*>& conn_list) {
		if (!mStatusReporter.IsInState(StateMachine::Running)) {
			mOutput->FatalError(OutputHelper::Component::TcpFactory, NO_INDEX, "Out of work time calling TcpFactory::GetConnections()!");
			return;
		}

		std::lock_guard<std::mutex> locker(mConnectionsMutex);
		CommonOpers::MoveDeque(mConnections, conn_list);
	}

	void TcpFactory::ReturnConnections(TcpInstance* conn) {
		if (!mStatusReporter.IsInState(StateMachine::Running)) {
			mOutput->FatalError(OutputHelper::Component::TcpFactory, NO_INDEX, "Out of work time calling TcpFactory::ReturnConnections()!");
			return;
		}

		mDisposal.Move(conn);
	}

	void TcpFactory::AcceptorWorker(std::error_code ec, asio::ip::tcp::socket socket) {
		// accept socket
		TcpInstance* new_connection = new TcpInstance(mOutput, mIndexDistributor.Get(), std::move(socket));
		{
			std::lock_guard<std::mutex> locker(mConnectionsMutex);
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

}
