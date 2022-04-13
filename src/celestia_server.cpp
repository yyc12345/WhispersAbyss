#include "../include/celestia_server.hpp"
#include "../include/global_define.hpp"

namespace WhispersAbyss {

	CelestiaServer::CelestiaServer(OutputHelper* output, const char* port) :
		mIndexDistributor(0),
		mOutput(output),
		mPort(atoi(port)),
		mIoContext(),
		mConnections(),
		mTcpAcceptor(mIoContext, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), mPort)),
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

	void CelestiaServer::Send(std::deque<Bmmo::IMessage*>* manager_list) {
		size_t msg_size;

		// push data
		mSendMsgMutex.lock();
		for (auto it = manager_list->begin(); it != manager_list->end(); ++it) {
			manager_list->push_back(*it);
			// do not clean manager_list
		}
		msg_size = mSendMsg.size();
		mSendMsgMutex.unlock();

		if (msg_size >= WARNING_CAPACITY)
			mOutput->Printf("[Celestia] Message list reach warning level: %d", msg_size);
		if (msg_size >= NUKE_CAPACITY) {
			mOutput->FatalError("[Celestia] Message list reach nuke level: %d. The program will be nuked!!!", msg_size);
		}
	}

	void CelestiaServer::Recv(std::deque<Bmmo::IMessage*>* manager_list) {
		mRecvMsgMutex.lock();
		while (mSendMsg.begin() != mSendMsg.end()) {
			manager_list->push_back(*mSendMsg.begin());
			mSendMsg.pop_front();
		}
		mRecvMsgMutex.unlock();
	}

	void CelestiaServer::BroadcastWorker() {
		CelestiaGnosis* connection;

		while (!mStopBroadcast.load()) {
			// todo: add broadcast code
			// pick first connection, process it and push it into tail
			mConnectionsMutex.lock();
			if (mConnections.begin() == mConnections.end())
				connection = NULL;
			else
				connection = *(mConnections.begin());
			mConnectionsMutex.unlock();

			// if no connection, sleep and back to next run
			if (connection == NULL) {
				std::this_thread::sleep_for(std::chrono::milliseconds(10));
				continue;
			}

			// push all existed message

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
