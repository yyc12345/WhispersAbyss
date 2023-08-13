#include "tcp_instance.hpp"

namespace WhispersAbyss {

	constexpr const uint32_t MAX_MSG_BODY = 2048u;

	TcpInstance::TcpInstance(OutputHelper* output, IndexDistributor::Index_t index, asio::ip::tcp::socket socket) :
		mOutput(output), mModuleStatus(StateMachine::Ready), mStatusReporter(mModuleStatus), mIndex(index),
		mSocket(std::move(socket)),
		mRecvMsgMutex(), mSendMsgMutex(), mOrderedUrlMutex(),
		mRecvMsg(), mSendMsg(), mOrderedUrl(),
		mTdSend(), mTdRecv()
	{
		std::thread([this]() -> void {
			// start transition
			StateMachine::TransitionInitializing transition(mModuleStatus);
			if (!transition.CanTransition()) return;

			// active sender, recver
			this->mTdRecv = std::jthread(std::bind(&TcpInstance::RecvWorker, this, std::placeholders::_1));
			this->mTdSend = std::jthread(std::bind(&TcpInstance::SendWorker, this, std::placeholders::_1));

			mOutput->Printf(OutputHelper::Component::TcpInstance, mIndex, "Started.");

			// end transition
			transition.SetTransitionError(false);
		}).detach();

		mOutput->Printf(OutputHelper::Component::TcpInstance, mIndex, "Instance created.");
	}

	TcpInstance::~TcpInstance() {
		// make sure stopped
		if (!mStatusReporter.IsInState(StateMachine::Stopped)) Stop();
		mStatusReporter.SpinUntil(StateMachine::Stopped);

		mOutput->Printf(OutputHelper::Component::TcpInstance, mIndex, "Instance disposed.");
	}

	void TcpInstance::Stop() {
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

			mOutput->Printf(OutputHelper::Component::TcpInstance, mIndex, "Stopped.");

		}).detach();
	}

	void TcpInstance::Send(std::deque<CommonMessage>& msg_list) {
		if (!mStatusReporter.IsInState(StateMachine::Running)) return;

		// move msg
		// get size outside of bracket.
		size_t msg_list_size;
		{
			std::lock_guard locker(mSendMsgMutex);
			CommonOpers::MoveDeque(msg_list, mSendMsg);
			msg_list_size = mSendMsg.size();
		}
		CheckSize(msg_list_size, false);
	}

	void TcpInstance::Recv(std::deque<CommonMessage>& msg_list) {
		if (!mStatusReporter.IsInState(StateMachine::Running)) return;

		std::lock_guard locker(mRecvMsgMutex);
		CommonOpers::MoveDeque(mRecvMsg, msg_list);
	}

	std::string TcpInstance::GetOrderedUrl() {
		if (!mStatusReporter.IsInState(StateMachine::Running)) return std::string();

		return mOrderedUrl;
	}

	void TcpInstance::CheckSize(size_t msg_size, bool is_recv) {
		const char* side = is_recv ? "Recv" : "Send";

		if (msg_size >= NUKE_CAPACITY) {
			mOutput->Printf(OutputHelper::Component::TcpInstance, mIndex, "%s message list reach nuke level: %zu. Nuke instance!!!", side, msg_size);
			Stop();
		} else if (msg_size >= WARNING_CAPACITY) {
			mOutput->Printf(OutputHelper::Component::TcpInstance, mIndex, "%s message list reach warning level: %zu", side, msg_size);
		}
	}

	void TcpInstance::SendWorker(std::stop_token st) {
		asio::error_code ec;
		std::deque<CommonMessage> intermsg;

		while (!st.stop_requested()) {
			// wait if socket is not worked
			// wait if not in running
			if (!mSocket.is_open() || !mStatusReporter.IsInState(StateMachine::Running)) {
				std::this_thread::sleep_for(std::chrono::milliseconds(SPIN_INTERVAL));
				continue;
			}

			// copy message to internal buffer
			{
				std::lock_guard locker(mSendMsgMutex);
				CommonOpers::MoveDeque(mSendMsg, intermsg);
			}

			// if no message. sleep and continue
			if (intermsg.empty()) {
				std::this_thread::sleep_for(std::chrono::milliseconds(10));
				continue;
			}

			// write message one by one
			for (auto& msg : intermsg) {
				// mMsgSize, mFlagIsCommand, mIsReliable, mRaw
				uint32_t msg_size = msg.GetCommonDataLen() + sizeof(uint8_t) + sizeof(uint8_t);
				static uint8_t flag_data = 0u;
				uint8_t is_reliable = msg.GetTcpIsReliable();

				// write msg size + flag + is reliable
				asio::write(mSocket, asio::buffer(&msg_size, sizeof(uint32_t)), ec);
				if (ec) {
					mOutput->Printf(OutputHelper::Component::TcpInstance, mIndex, "Fail to write header - msg size: %s", ec.message().c_str());
					this->Stop();
					return;
				}
				asio::write(mSocket, asio::buffer(&flag_data, sizeof(uint8_t)), ec);
				if (ec) {
					mOutput->Printf(OutputHelper::Component::TcpInstance, mIndex, "Fail to write header - flag is command: %s", ec.message().c_str());
					this->Stop();
					return;
				}
				asio::write(mSocket, asio::buffer(&is_reliable, sizeof(uint8_t)), ec);
				if (ec) {
					mOutput->Printf(OutputHelper::Component::TcpInstance, mIndex, "Fail to write header - is reliable: %s", ec.message().c_str());
					this->Stop();
					return;
				}
				// write raw data
				asio::write(mSocket, asio::buffer(msg.GetCommonData(), msg.GetCommonDataLen()), ec);
				if (ec) {
					mOutput->Printf(OutputHelper::Component::TcpInstance, mIndex, "Fail to write body: %s", ec.message().c_str());
					this->Stop();
					return;
				}
			}

			// clear internal buffer
			intermsg.clear();

			// end of a loop of sender.
		}
	}

	void TcpInstance::RecvWorker(std::stop_token st) {
		uint32_t mMsgSize, mUrlSize;
		uint8_t mFlagIsCommand, mIsReliable;
		std::string mMsgBuffer;
		asio::error_code ec;
		std::deque<CommonMessage> intermsg;

		while (!st.stop_requested()) {
			// wait if socket is not worked
			if (!mSocket.is_open()) {
				std::this_thread::sleep_for(std::chrono::milliseconds(SPIN_INTERVAL));
				continue;
			}

			// recv msg size
			asio::read(mSocket, asio::buffer(&mMsgSize, sizeof(uint32_t)), ec);
			if (ec) {
				mOutput->Printf(OutputHelper::Component::TcpInstance, mIndex, "Fail to read header: %s", ec.message().c_str());
				this->Stop();
				return;
			}
			if (mMsgSize >= MAX_MSG_BODY) {
				mOutput->Printf(OutputHelper::Component::TcpInstance, mIndex, "Body size indicated by header higher than threshold: %" PRIu32, mMsgSize);
				this->Stop();
				return;
			}

			// recv msg body
			mMsgBuffer.resize(mMsgSize);
			asio::read(mSocket, asio::buffer(mMsgBuffer.data(), mMsgSize), ec);
			if (ec) {
				mOutput->Printf(OutputHelper::Component::TcpInstance, mIndex, "Fail to read body: %s", ec.message().c_str());
				this->Stop();
				return;
			}

			// analyse body
			if (mMsgBuffer.size() < sizeof(uint8_t)) {
				mOutput->Printf(OutputHelper::Component::TcpInstance, mIndex, "Too short msg body. No mFlagIsCommand.");
				this->Stop();
				return;
			}
			mFlagIsCommand = *reinterpret_cast<const uint8_t*>(mMsgBuffer.c_str());
			if (mFlagIsCommand) {
				// command message
				if (mMsgBuffer.size() < sizeof(uint8_t) + sizeof(uint32_t)) {
					mOutput->Printf(OutputHelper::Component::TcpInstance, mIndex, "Too short command msg. No mUrlSize.");
					this->Stop();
					return;
				}

				mUrlSize = *reinterpret_cast<const uint32_t*>(mMsgBuffer.c_str() + sizeof(uint8_t));
				{
					std::lock_guard locker(mOrderedUrlMutex);
					mOrderedUrl.resize(mUrlSize);
					memcpy(
						mOrderedUrl.data(),
						mMsgBuffer.c_str() + sizeof(uint8_t) + sizeof(uint32_t),
						mUrlSize
					);

					mOutput->Printf(OutputHelper::Component::TcpInstance, mIndex, "Request GNS connect to: %s", mOrderedUrl.c_str());
				}
			} else {
				// data message
				if (mMsgBuffer.size() < sizeof(uint8_t) + sizeof(uint8_t)) {
					mOutput->Printf(OutputHelper::Component::TcpInstance, mIndex, "Too short command msg. No mIsReliable.");
					this->Stop();
					return;
				}

				mIsReliable = *reinterpret_cast<const uint8_t*>(mMsgBuffer.c_str() + sizeof(uint8_t));

				CommonMessage msg;
				msg.SetTcpData(
					mMsgBuffer.c_str() + sizeof(uint8_t) + sizeof(uint8_t),
					mIsReliable,
					mMsgSize - sizeof(uint8_t) - sizeof(uint8_t)
				);
				intermsg.push_back(std::move(msg));
			}

			// try move all intermsg to recv msg.
			// if we cant, wait next time to move.
			size_t msg_list_size = 0u;
			if (mStatusReporter.IsInState(StateMachine::Running)) {
				std::lock_guard locker(mRecvMsgMutex);
				CommonOpers::MoveDeque(intermsg, mRecvMsg);
				msg_list_size = mRecvMsg.size();
			}
			// check size at the same time
			CheckSize(msg_list_size, true);

			// end of a loop of recver
		}
	}

}

