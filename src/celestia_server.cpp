#include "../include/celestia_server.hpp"

namespace WhispersAbyss {

	CelestiaServer::CelestiaServer(OutputHelper* output, uint16_t port) :
		mPort(port),
		mIoContext(),
		mTcpAcceptor(mIoContext, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port)),
		mTdBroadcast(), mTdCtx(),
		mIsRunning(false), mStopBroadcast(false)
	{

	}

	CelestiaServer::~CelestiaServer() {
		;
	}

	void CelestiaServer::Start() {
		// register worker
		RegisterAsyncWork();

		// preparing broadcast worker and ctx worker
		mStopBroadcast.store(false);
		mTdCtx = std::thread(&CelestiaServer::CtxWorker, this);

		// notify main worker
		mIsRunning.store(true);
	}

	void CelestiaServer::Stop() {
		// try stop worker
		mIoContext.stop();
		mStopBroadcast.store(true);

		// waiting for thread over
		if (mTdCtx.joinable())
			mTdCtx.join();

		if (mTdBroadcast.joinable())
			mTdBroadcast.join();

		// notify main worker
		mIsRunning.store(false);
	}

	void CelestiaServer::BroadcastWorker() {
		while (!mStopBroadcast.load()) {
			// todo: add broadcast code
		}
	}

	void CelestiaServer::CtxWorker() {
		mIoContext.run();
	}

	void CelestiaServer::AcceptorWorker(std::error_code ec, asio::ip::tcp::socket socket) {
		//todo: accept socket

		// accept new connection
		RegisterAsyncWork();
	}

	void CelestiaServer::RegisterAsyncWork() {
		mTcpAcceptor.async_accept(std::bind(
			&CelestiaServer::AcceptorWorker, this, std::placeholders::_1, std::placeholders::_2
		));
	}


}
