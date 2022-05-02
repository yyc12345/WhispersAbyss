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
		mTdSend(&CelestiaGnosis::SendWorker, this) {
		mOutput->Printf("[Gnosis-#%ld] Connection instance created.", mIndex);
	}

	CelestiaGnosis::~CelestiaGnosis() {
		if (mTdRecv.joinable())
			mTdRecv.join();
		if (mTdSend.joinable())
			mTdSend.join();

		WhispersAbyss::Cmmo::Messages::IMessage* ptr;
		while (mRecvMsg.begin() != mRecvMsg.end()) {
			ptr = *mRecvMsg.begin();
			mRecvMsg.pop_front();
			delete ptr;
		}
		while (mSendMsg.begin() != mSendMsg.end()) {
			ptr = *mSendMsg.begin();
			mSendMsg.pop_front();
			delete ptr;
		}

		mOutput->Printf("[Gnosis-#%ld] Connection instance disposed.", mIndex);
	}

	void CelestiaGnosis::Send(std::deque<Cmmo::Messages::IMessage*>* manager_list) {
		size_t msg_size;

		// push data
		mSendMsgMutex.lock();
		for (auto it = manager_list->begin(); it != manager_list->end(); ++it) {
			// push its clone.
			mSendMsg.push_back((*it)->Clone());
			// do not clean manager_list
		}
		msg_size = mSendMsg.size();
		mSendMsgMutex.unlock();

		if (msg_size >= WARNING_CAPACITY)
			mOutput->Printf("[Gnosis-#%ld] Message list reach warning level: %d", mIndex, msg_size);
		if (msg_size >= NUKE_CAPACITY) {
			mOutput->Printf("[Gnosis-#%ld] Message list reach nuke level: %d. This gnosis will be nuked!!!", mIndex, msg_size);
			mSocket.close();
		}
	}
	void CelestiaGnosis::Recv(std::deque<Cmmo::Messages::IMessage*>* manager_list) {
		mRecvMsgMutex.lock();
		while (mRecvMsg.begin() != mRecvMsg.end()) {
			manager_list->push_back(*mRecvMsg.begin());
			mRecvMsg.pop_front();
		}
		mRecvMsgMutex.unlock();
	}

	bool CelestiaGnosis::IsConnected() {
		return mSocket.is_open();
	}

	Cmmo::Messages::IMessage* CelestiaGnosis::CreateMessageFromStream(std::stringstream* data) {
		// peek opcode
		Cmmo::OpCode code = Cmmo::Messages::IMessage::PeekOpCode(data);

		// check format
		Cmmo::Messages::IMessage* msg = NULL;
		switch (code) {
			case WhispersAbyss::Cmmo::OpCode::None:
				mOutput->Printf("[Gnosis-#%ld] Invalid OpCode %d.", mIndex, code);
				break;
			case WhispersAbyss::Cmmo::OpCode::CS_OrderChat:
				msg = new Cmmo::Messages::OrderChat();
				break;
			case WhispersAbyss::Cmmo::OpCode::SC_Chat:
				msg = new Cmmo::Messages::Chat();
				break;
			case WhispersAbyss::Cmmo::OpCode::CS_OrderGlobalCheat:
				msg = new Cmmo::Messages::OrderGlobalCheat();
				break;
			case WhispersAbyss::Cmmo::OpCode::CS_OrderClientList:
				msg = new Cmmo::Messages::OrderClientList();
				break;
			case WhispersAbyss::Cmmo::OpCode::SC_ClientList:
				msg = new Cmmo::Messages::ClientList();
				break;
			case WhispersAbyss::Cmmo::OpCode::SC_BallState:
				msg = new Cmmo::Messages::BallState();
				break;
			case WhispersAbyss::Cmmo::OpCode::SC_ClientConnected:
				msg = new Cmmo::Messages::ClientConnected();
				break;
			case WhispersAbyss::Cmmo::OpCode::SC_ClientDisconnected:
				msg = new Cmmo::Messages::ClientDisconnected();
				break;
			case WhispersAbyss::Cmmo::OpCode::SC_CheatState:
				msg = new Cmmo::Messages::CheatState();
				break;
			case WhispersAbyss::Cmmo::OpCode::SC_GlobalCheat:
				msg = new Cmmo::Messages::GlobalCheat();
				break;
			case WhispersAbyss::Cmmo::OpCode::SC_LevelFinish:
				msg = new Cmmo::Messages::LevelFinish();
				break;
			default:
				mOutput->Printf("[Gnosis-#%ld] Unknow OpCode %d.", mIndex, code);
				break;
		}

		if (msg != NULL) {
			if (!msg->Deserialize(data)) {
				mOutput->Printf("[Gnosis-#%ld] Fail to parse message with OpCode %d.", mIndex, code);
				delete msg;
				msg = NULL;
			}
		}
		return msg;
	}

	void CelestiaGnosis::SendWorker() {
		Cmmo::Messages::IMessage* msg;
		std::stringstream buffer;
		asio::error_code ec;

		while (IsConnected()) {
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

			// get serialized msg and delete msg
			buffer.str("");
			buffer.clear();
			msg->Serialize(&buffer);
			uint32_t length = buffer.str().size();
			delete msg;
			// write data
			asio::write(mSocket, asio::buffer(&length, sizeof(uint32_t)), ec);
			if (ec) {
				mOutput->Printf("[Gnosis-#%ld] Fail to write header: %s", mIndex, ec.message().c_str());
				mSocket.close();
				return;
			}
			asio::write(mSocket, asio::buffer(buffer.str().data(), length), ec);
			if (ec) {
				mOutput->Printf("[Gnosis-#%ld] Fail to write body: %s", mIndex, ec.message().c_str());
				mSocket.close();
				return;
			}

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
				if (mMsgHeader >= MAX_MSG_SIZE) {
					mOutput->Printf("[Gnosis-#%ld] Header exceed MAX_MSG_SIZE.", mIndex);
					mSocket.close();
					return;
				} else {
					// preparing reading body
					;
				}
			} else {
				mOutput->Printf("[Gnosis-#%ld] Fail to read header: %s", mIndex, ec.message().c_str());
				mSocket.close();
				return;
			}

			// recv body
			mMsgBuffer.resize(mMsgHeader);
			asio::read(mSocket, asio::buffer(mMsgBuffer.data(), mMsgHeader), ec);
			if (!ec) {

				// set for stream
				mMsgStream.str("");
				mMsgStream.clear();
				mMsgStream.write(mMsgBuffer.c_str(), mMsgHeader);

				// get msg and push it
				auto msg = CreateMessageFromStream(&mMsgStream);
				if (msg != NULL) {
					mRecvMsgMutex.lock();
					mRecvMsg.push_back(msg);
					mRecvMsgMutex.unlock();
				}

				// for next reading
			} else {
				mOutput->Printf("[Gnosis-#%ld] Fail to read body: %s", mIndex, ec.message().c_str());
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

