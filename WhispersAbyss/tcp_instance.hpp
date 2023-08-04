#pragma once

#include <sdkddkver.h>	// need by asio
#include "asio.hpp"
#include "output_helper.hpp"
#include "others_helper.hpp"
#include "state_machine.hpp"
#include "messages.hpp"
#include <thread>
#include <deque>
#include <mutex>
#include <string>

namespace WhispersAbyss {

	/*
	# Tcp Network Message Syntax
	There are 2 types of supported messages. Command message and data message.
	Command message will instruct this app which server should be connected.
	Data message is real communication between Tcp endpoint and Gns endpoint.

	## Message Syntax:

	uint32_t	mMsgSize;
	uint8_t		mFlagIsCommand;
	...			mPayload;

	mMsgSize describe the size of message, including mFlagIsCommand and mPayload.
	If mFlagIsCommand != 0, this mPayload belong to a command message, otherwise belong to a data message.

	## Payload Syntax

	### Data Message

	uint8_t	mIsReliable
	...		mRaw

	If mIsReliable != 0, this message should be sent in unreliabe mode, otherwise send it in reliable mode.
	mRaw is raw data. Just resend it directly.

	### Command Message

	uint32_t	mUrlSize;
	...			mUrl;

	Command message will provide a URL which the endpoint want to connect.
	It will write the length of this URL first.
	Then write the URL self without terminal null.
	The encoding of URL is undefined. I don't know how ASIO process it.

	*/

	class TcpInstance {
	private:
		OutputHelper* mOutput;
		StateMachine::StateMachineCore mModuleStatus;
		asio::ip::tcp::socket mSocket;

		std::mutex mRecvMsgMutex, mSendMsgMutex, mOrderedUrlMutex;
		std::deque<CommonMessage> mRecvMsg, mSendMsg;
		std::string mOrderedUrl;

		std::jthread mTdSend, mTdRecv;
	public:
		StateMachine::StateMachineReporter mStatusReporter;
		uint64_t mIndex;

	private:
		void SendWorker(std::stop_token st);
		void RecvWorker(std::stop_token st);

		void CheckSize(size_t msg_size);
	public:
		TcpInstance(OutputHelper* output, uint64_t index, asio::ip::tcp::socket socket);
		TcpInstance(const TcpInstance& rhs) = delete;
		TcpInstance(TcpInstance&& rhs) = delete;
		~TcpInstance();

		void Start();
		void Stop();

		void Send(std::deque<CommonMessage>& msg_list);
		void Recv(std::deque<CommonMessage>& msg_list);
		std::string GetEndpointOrder();
	};


}