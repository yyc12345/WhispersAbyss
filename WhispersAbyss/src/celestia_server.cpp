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
		mTdBroadcast = std::thread(&CelestiaServer::BroadcastWorker, this);

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

	void CelestiaServer::Send(std::deque<Cmmo::Messages::IMessage*>* manager_list) {
		size_t msg_size;

		// push data
		mSendMsgMutex.lock();
		for (auto it = manager_list->begin(); it != manager_list->end(); ++it) {
			mSendMsg.push_back(*it);
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

	void CelestiaServer::Recv(std::deque<Cmmo::Messages::IMessage*>* manager_list) {
		mRecvMsgMutex.lock();
		while (mRecvMsg.begin() != mRecvMsg.end()) {
			manager_list->push_back(*mRecvMsg.begin());
			mRecvMsg.pop_front();
		}
		mRecvMsgMutex.unlock();
	}

	void CelestiaServer::BroadcastWorker() {
		std::deque<CelestiaGnosis*> pending_connection;
		std::deque<Cmmo::Messages::IMessage*> pending_message;

		std::deque<Cmmo::Messages::IMessage*> gotten_msg;
		std::deque<CelestiaGnosis*> success_connection;

		while (!mStopBroadcast.load()) {
			std::this_thread::sleep_for(std::chrono::milliseconds(10));

			// destroy all pending msg and clean it
			while (pending_message.begin() != pending_message.end()) {
				Cmmo::Messages::IMessage* msg = *pending_message.begin();
				pending_message.pop_front();
				delete msg;
			}
			// copy send message list
			mSendMsgMutex.lock();
			while (mSendMsg.begin() != mSendMsg.end()) {
				pending_message.push_back(*mSendMsg.begin());
				mSendMsg.pop_front();
			}
			mSendMsgMutex.unlock();
			
			// pick all connections, process it and push it into tail
			pending_connection.clear();
			mConnectionsMutex.lock();
			while (mConnections.begin() != mConnections.end()) {
				pending_connection.push_back(*mConnections.begin());
				mConnections.pop_front();
			}
			mConnectionsMutex.unlock();

			// if no connection, sleep and back to next run
			if (pending_connection.size() == 0) {
				//std::this_thread::sleep_for(std::chrono::milliseconds(10));
				continue;
			}

			// process connection
			gotten_msg.clear();
			success_connection.clear();
			while (pending_connection.begin() != pending_connection.end()) {
				// get current connection and pop it from pending connections
				CelestiaGnosis* connection = *pending_connection.begin();
				pending_connection.pop_front();

				// detect status
				// if lost connection, report
				// and dispose it, never push it back into connections deque
				if (!connection->IsConnected()) {
					delete connection;
					continue;
				}

				// push all send message
				connection->Send(&pending_message);
				// get all recv message and push into recv list
				connection->Recv(&gotten_msg);
				// push current connection back to connections
				success_connection.push_back(connection);
			}

			// push recv data and success conn
			mRecvMsgMutex.lock();
			while (gotten_msg.begin() != gotten_msg.end()) {
				mRecvMsg.push_back(*gotten_msg.begin());
				gotten_msg.pop_front();
			}
			mRecvMsgMutex.unlock();

			mConnectionsMutex.lock();
			while (success_connection.begin() != success_connection.end()) {
				mConnections.push_back(*success_connection.begin());
				success_connection.pop_front();
			}
			mConnectionsMutex.unlock();

			// destroy all pending msg
			while (pending_message.begin() != pending_message.end()) {
				Cmmo::Messages::IMessage* msg = *pending_message.begin();
				pending_message.pop_front();
				delete msg;
			}
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
