#include "../include/celestia_server.hpp"
#include "../include/global_define.hpp"

namespace WhispersAbyss {

	CelestiaServer::CelestiaServer(OutputHelper* output, uint16_t port) :
		mIndexDistributor(0),
		mOutput(output),
		mPort(port),
		mIoContext(),
		mConnections(), mConnectionsMutex(),
		mTcpAcceptor(mIoContext, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), mPort)),
		mTdCtx(),
		mIsRunning(false)
	{
		;
	}

	CelestiaServer::~CelestiaServer() {
		;
	}

	void CelestiaServer::Start() {
		// register worker
		RegisterAsyncWork();

		// preparing ctx worker
		mTdCtx = std::thread(&CelestiaServer::CtxWorker, this);

		// notify main worker
		mIsRunning.store(true);
	}

	void CelestiaServer::Stop() {
		// try stop worker
		mIoContext.stop();

		// waiting for thread over
		if (mTdCtx.joinable())
			mTdCtx.join();

		// try stop cached connection
		CelestiaGnosis* conn;
		mConnectionsMutex.lock();
		while (mConnections.begin() != mConnections.end()) {
			conn = *mConnections.begin();
			mConnections.pop_front();

			// active stop
			conn->Stop();
			// then delete it
			delete conn;
		}
		mConnectionsMutex.unlock();

		// notify main worker
		mIsRunning.store(false);
	}

	void CelestiaServer::GetConnections(std::deque<CelestiaGnosis*>* conn_list) {
		// move cached connections to outside worker
		while (mConnections.begin() != mConnections.end()) {
			conn_list->push_back(*mConnections.begin());
			mConnections.pop_front();
		}
	}

	void CelestiaServer::CtxWorker() {
		mIoContext.run();
	}

	void CelestiaServer::AcceptorWorker(std::error_code ec, asio::ip::tcp::socket socket) {
		// accept socket
		CelestiaGnosis* new_connection = new CelestiaGnosis(mOutput, mIndexDistributor++, std::move(socket));
		mConnectionsMutex.lock();
		mConnections.push_back(new_connection);
		mConnectionsMutex.unlock();

		// accept new connection
		RegisterAsyncWork();
	}

	void CelestiaServer::RegisterAsyncWork() {
		mTcpAcceptor.async_accept(std::bind(
			&CelestiaServer::AcceptorWorker, this, std::placeholders::_1, std::placeholders::_2
		));
	}


}
