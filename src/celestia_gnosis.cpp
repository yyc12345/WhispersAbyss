#include "../include/celestia_gnosis.hpp"
#include "../include/global_define.hpp"

#define MAX_MSG_SIZE 2048

namespace WhispersAbyss {

	CelestiaGnosis::CelestiaGnosis(OutputHelper* output, uint64_t index, asio::ip::tcp::socket socket) :
		mIndex(index),
		mOutput(output), mSocket(std::move(socket)),
		mRecvMsgMutex(), mSendMsgMutex(),
		mRecvMsg(), mSendMsg(),
		mTdRecv(&CelestiaGnosis::RecvWorker, this),
		mTdSend(&CelestiaGnosis::SendWorker, this)
	{
		mOutput->Printf("[Gnosis - #%ld] Connection instance created.", mIndex);
	}

	CelestiaGnosis::~CelestiaGnosis() {
		if (mTdRecv.joinable())
			mTdRecv.join();
		if (mTdSend.joinable())
			mTdSend.join();
		
		WhispersAbyss::Bmmo::IMessage* ptr;
		while(mRecvMsg.begin() != mRecvMsg.end()) {
			ptr = *mRecvMsg.begin();
			mRecvMsg.pop_front();
			delete ptr;
		}
		while (mSendMsg.begin() != mSendMsg.end()) {
			ptr = *mSendMsg.begin();
			mSendMsg.pop_front();
			delete ptr;
		}

		mOutput->Printf("[Gnosis - #%ld] Connection instance disposed.", mIndex);
	}

	void CelestiaGnosis::Send(std::deque<Bmmo::IMessage*>* manager_list) {
		size_t msg_size;

		// push data
		mSendMsgMutex.lock();
		for(auto it = manager_list->begin(); it != manager_list->end(); ++it) {
			manager_list->push_back(*it);
			// do not clean manager_list
		}
		msg_size = mSendMsg.size();
		mSendMsgMutex.unlock();

		if (msg_size >= WARNING_CAPACITY)
			mOutput->Printf("[Gnosis - #%ld] Message list reach warning level: %d", mIndex, msg_size);
		if (msg_size >= NUKE_CAPACITY) {
			mOutput->Printf("[Gnosis - #%ld] Message list reach nuke level: %d. This gnosis will be nuked!!!", mIndex, msg_size);
			mSocket.close();
		}
	}
	void CelestiaGnosis::Recv(std::deque<Bmmo::IMessage*>* manager_list) {
		mRecvMsgMutex.lock();
		while (mSendMsg.begin() != mSendMsg.end()) {
			manager_list->push_back(*mSendMsg.begin());
			mSendMsg.pop_front();
		}
		mRecvMsgMutex.unlock();
	}

	bool CelestiaGnosis::IsConnected() {
		return mSocket.is_open();
	}

	void CelestiaGnosis::SendWorker() {
		Bmmo::IMessage* msg;

		while (IsConnected()) {

			mSendMsgMutex.lock();
			if (mSendMsg.begin() != mSendMsg.end()) {
				msg = *mSendMsg.begin();
				mSendMsg.pop_front();
			}
			mSendMsgMutex.unlock();


		}
	}

	void CelestiaGnosis::RecvWorker() {
		uint32_t mMsgHeader;
		std::string mMsgBuffer;
		std::stringstream mMsgStream;
		asio::error_code ec;

		while (IsConnected()) {

			// recv header
			asio::read(mSocket, asio::buffer(&mMsgHeader, sizeof(uint32_t)), ec);
			if (!ec) {
				if (mMsgHeader >= MAX_MSG_SIZE || mMsgHeader <= 0) {
					mOutput->Printf("[Gnosis - #%ld] Header exceed MAX_MSG_SIZE or lower than 0.", mIndex);
					mSocket.close();
					return;
				} else {
					// preparing reading body
					;
				}
			} else {
				mOutput->Printf("[Gnosis - #%ld] Fail to read header: %s", mIndex, ec.message().c_str());
				mSocket.close();
				return;
			}

			// recv body
			mMsgBuffer.resize(mMsgHeader);
			asio::read(mSocket, asio::buffer(mMsgBuffer.data(), mMsgHeader), ec);
			if (!ec) {
					
				// set for stream
				mMsgStream.clear();
				mMsgStream << mMsgBuffer.c_str();

				// get msg and push it
				Bmmo::IMessage* msg = Bmmo::IMessage::CreateMessageFromStream(&mMsgStream);
				mRecvMsgMutex.lock();
				mRecvMsg.push_back(msg);
				mRecvMsgMutex.unlock();

				// for next reading
			} else {
				mOutput->Printf("[Gnosis - #%ld] Fail to read body: %s", mIndex, ec.message().c_str());
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
