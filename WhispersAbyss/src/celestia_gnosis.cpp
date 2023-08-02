#include "../include/celestia_gnosis.hpp"

namespace WhispersAbyss {

	constexpr const uint32_t MAX_MSG_BODY = 2048u;

	CelestiaGnosis::CelestiaGnosis(OutputHelper* output, uint64_t index, asio::ip::tcp::socket socket) :
		mIndex(index),
		mOutput(output), mSocket(std::move(socket)),
		mRecvMsgMutex(), mSendMsgMutex(),
		mRecvMsg(), mSendMsg()
		/*mTdRecv(&CelestiaGnosis::RecvWorker, this),*/
		/*mTdSend(&CelestiaGnosis::SendWorker, this)*/ 
	{
		mOutput->Printf("[Gnosis-#%" PRIu64 "] Connection instance created.", mIndex);
	}

	CelestiaGnosis::~CelestiaGnosis() {
		// make sure stopped
		Stop();
		mStatusReporter.SpinUntil(StateMachine::State::Stopped);

		mOutput->Printf("[Gnosis-#%" PRIu64 "] Connection instance disposed.", mIndex);
	}

	void CelestiaGnosis::Start() {
		std::thread([this]() -> void {
			// start transition
			StateMachine::TransitionInitializing transition(mModuleStatus);
			if (!transition.CanTransition()) return;

			// active sender, recver
			this->mTdRecv = std::jthread(std::bind(&CelestiaGnosis::RecvWorker, this, std::placeholders::_1));
			this->mTdSend = std::jthread(std::bind(&CelestiaGnosis::SendWorker, this, std::placeholders::_1));

			mOutput->Printf("[Gnosis-#%" PRIu64 "] Connection instance run.", mIndex);

			// end transition
			transition.SetTransitionError(false);
		}).detach();
	}

	void CelestiaGnosis::Stop() {
		std::thread([this]() -> void {
			// start transition
			StateMachine::TransitionStopping transition(mModuleStatus);
			if (!transition.CanTransition()) return;

			// close socket if it still is opened
			if (mSocket.is_open()) {
				mSocket.close();
			}

			// stop recver and sender
			if (mTdRecv.joinable()) {
				mTdRecv.request_stop();
				mTdRecv.join();
			}
			if (mTdSend.joinable()) {
				mTdSend.request_stop();
				mTdSend.join();
			}

			// delete data
			{
				std::lock_guard locker(mRecvMsgMutex);
				DequeOperations::FreeDeque(mRecvMsg);
			}
			{
				std::lock_guard locker(mSendMsgMutex);
				DequeOperations::FreeDeque(mSendMsg);
			}

			mOutput->Printf("[Gnosis-#%" PRIu64 "] Connection instance stopped.", mIndex);

		}).detach();
	}

	void CelestiaGnosis::Send(std::deque<Bmmo::Message*>& manager_list) {
		StateMachine::WorkBasedOnRunning work(mModuleStatus);
		if (!work.CanWork()) return;

		size_t msg_size;

		// push data
		{
			std::lock_guard locker(mSendMsgMutex);
			DequeOperations::MoveDeque(manager_list, mSendMsg);
			msg_size = mSendMsg.size();		// get size now for following check
		}

		// check data size
		CheckSize(msg_size);
	}

	void CelestiaGnosis::Recv(std::deque<Bmmo::Message*>& manager_list) {
		StateMachine::WorkBasedOnRunning work(mModuleStatus);
		if (!work.CanWork()) return;

		std::lock_guard locker(mRecvMsgMutex);
		DequeOperations::MoveDeque(mRecvMsg, manager_list);
	}

	void CelestiaGnosis::CheckSize(size_t msg_size) {
		if (msg_size >= WARNING_CAPACITY)
			mOutput->Printf("[Gnosis-#%" PRIu64 "] Message list reach warning level: %d", mIndex, msg_size);
		if (msg_size >= NUKE_CAPACITY) {
			mOutput->Printf("[Gnosis-#%" PRIu64 "] Message list reach nuke level: %d. This gnosis will be nuked!!!", mIndex, msg_size);
			Stop();
		}
	}

	void CelestiaGnosis::SendWorker(std::stop_token st) {
		Bmmo::Message* msg;
		asio::error_code ec;

		while (!st.stop_requested()) {
			if (mSocket.is_open())
			std::this_thread::sleep_for(std::chrono::milliseconds(10));

			// get msg
			mSendMsgMutex.lock();
			if (mSendMsg.begin() != mSendMsg.end()) {
				msg = *mSendMsg.begin();
				mSendMsg.pop_front();
			} else {
				msg = NULL;
			}
			mSendMsgMutex.unlock();

			// if no msg, sleep
			if (msg == NULL) {
				//std::this_thread::sleep_for(std::chrono::milliseconds(10));
				continue;
			}

			// get serialized msg
			uint32_t length = msg->GetDataLen();
			uint32_t is_reliable = msg->GetIsReliable() ? 1u : 0u;
			// write data
			asio::write(mSocket, asio::buffer(&length, sizeof(uint32_t)), ec);
			if (ec) {
				mOutput->Printf("[Gnosis-#%" PRIu64 "] Fail to write header: %s", mIndex, ec.message().c_str());
				mSocket.close();
				return;
			}
			asio::write(mSocket, asio::buffer(&is_reliable, sizeof(uint32_t)), ec);
			if (ec) {
				mOutput->Printf("[Gnosis-#%" PRIu64 "] Fail to write is_reliable: %s", mIndex, ec.message().c_str());
				mSocket.close();
				return;
			}
			asio::write(mSocket, asio::buffer(msg->GetData(), length), ec);
			if (ec) {
				mOutput->Printf("[Gnosis-#%" PRIu64 "] Fail to write body: %s", mIndex, ec.message().c_str());
				mSocket.close();
				return;
			}
			// delete message
			delete msg;

		}
	}

	void CelestiaGnosis::RecvWorker(std::stop_token st) {
		uint32_t mMsgHeader, mMsgReliable;
		std::string mMsgBuffer;
		asio::error_code ec;

		while (IsConnected()) {

			// recv header
			asio::read(mSocket, asio::buffer(&mMsgHeader, sizeof(uint32_t)), ec);
			if (!ec) {
				if (mMsgHeader >= MAX_MSG_SIZE) {
					mOutput->Printf("[Gnosis-#%" PRIu64 "] Header exceed MAX_MSG_SIZE.", mIndex);
					mSocket.close();
					return;
				} else {
					// preparing reading body
					;
				}
			} else {
				mOutput->Printf("[Gnosis-#%" PRIu64 "] Fail to read header: %s", mIndex, ec.message().c_str());
				mSocket.close();
				return;
			}

			// recv is_reliable
			asio::read(mSocket, asio::buffer(&mMsgReliable, sizeof(uint32_t)), ec);
			if (!ec) {
				if (mMsgReliable >= MAX_MSG_SIZE) {
					mOutput->Printf("[Gnosis-#%" PRIu64 "] Header exceed MAX_MSG_SIZE.", mIndex);
					mSocket.close();
					return;
				} else {
					// preparing reading body
					;
				}
			} else {
				mOutput->Printf("[Gnosis-#%" PRIu64 "] Fail to read is_reliable: %s", mIndex, ec.message().c_str());
				mSocket.close();
				return;
			}

			// recv body
			mMsgBuffer.resize(mMsgHeader);
			asio::read(mSocket, asio::buffer(mMsgBuffer.data(), mMsgHeader), ec);
			if (!ec) {

				// get msg and push it
				auto msg = new Bmmo::Message();
				msg->ReadTcpData(&mMsgBuffer, mMsgReliable != 0, mMsgHeader);
				mRecvMsgMutex.lock();
				mRecvMsg.push_back(msg);
				mRecvMsgMutex.unlock();

				// for next reading
			} else {
				mOutput->Printf("[Gnosis-#%" PRIu64 "] Fail to read body: %s", mIndex, ec.message().c_str());
				mSocket.close();
				return;
			}

		}
	}



	//void CelestiaGnosis::RegisterHeaderRead() {
	//	asio::async_read(
	//		mSocket,
	//		asio::buffer(&mMsgHeader, sizeof(uint32_t)),
	//		std::bind(&CelestiaGnosis::ReadHeaderWorker, this, std::placeholders::_1, std::placeholders::_2)
	//	);
	//}

	//void CelestiaGnosis::ReadHeaderWorker(std::error_code ec, std::size_t length) {
	//	if (!ec) {
	//		if (mMsgHeader >= MAX_MSG_SIZE || mMsgHeader <= 0) {
	//			mOutput->Printf("[Gnosis] Header exceed MAX_MSG_SIZE or lower than 0.");
	//			mSocket.close();
	//		} else {
	//			// preparing reading body
	//			RegisterBodyRead(mMsgHeader);
	//		}
	//	} else {
	//		mOutput->Printf("[Gnosis] Fail to read header: %s", ec.message().c_str());
	//		mSocket.close();
	//	}
	//}

	//void CelestiaGnosis::RegisterBodyRead(uint32_t header_size) {
	//	mMsgBuffer.resize(header_size);

	//	asio::async_read(
	//		mSocket,
	//		asio::buffer(mMsgBuffer.data(), header_size),
	//		std::bind(&CelestiaGnosis::ReadBodyWorker, this, std::placeholders::_1, std::placeholders::_2)
	//	);
	//}

	//void CelestiaGnosis::ReadBodyWorker(std::error_code ec, std::size_t length) {
	//	if (!ec) {
	//		
	//		// set for stream
	//		mMsgStream.str("");
	//		mMsgStream.clear();
	//		mMsgStream << mMsgBuffer.c_str();

	//		// get msg and push it
	//		Bmmo::IMessage* msg = Bmmo::IMessage::CreateMessageFromStream(&mMsgStream);
	//		mRecvMsgMutex.lock();
	//		mRecvMsg.push_back(msg);
	//		mRecvMsgMutex.unlock();

	//		// for next reading
	//		RegisterHeaderRead();

	//	} else {
	//		mOutput->Printf("[Gnosis] Fail to read body: %s", ec.message().c_str());
	//		mSocket.close();
	//	}
	//}

}

