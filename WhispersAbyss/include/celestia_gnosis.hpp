#pragma once

#include <sdkddkver.h>	// need by asio
#include "asio.hpp"
#include "output_helper.hpp"
#include "cmmo_message.hpp"
#include <deque>
#include <mutex>
#include <string>

namespace WhispersAbyss {

	class CelestiaGnosis {
	public:
		CelestiaGnosis(OutputHelper* output, uint64_t index, asio::ip::tcp::socket socket);
		~CelestiaGnosis();

		void Send(std::deque<Cmmo::Messages::IMessage*>* manager_list);
		void Recv(std::deque<Cmmo::Messages::IMessage*>* manager_list);
		bool IsConnected();
	private:
		uint64_t mIndex;
		OutputHelper* mOutput;
		asio::ip::tcp::socket mSocket;

		std::mutex mRecvMsgMutex, mSendMsgMutex;
		std::deque<Cmmo::Messages::IMessage*> mRecvMsg, mSendMsg;

		//uint32_t mMsgHeader;
		//std::string mMsgBuffer;
		//std::stringstream mMsgStream;

		std::thread mTdSend, mTdRecv;

		Cmmo::Messages::IMessage* CreateMessageFromStream(std::stringstream* data);

		// Write routine:
		void SendWorker();
		void RecvWorker();

		//// Read routine:
		//// RegisterHeaderRead() -- async --> ReadHeaderWorker() --> 
		//// RegisterBodyRead() -- async --> ReadBodyWorker() -->
		//// RegisterHeaderRead() (LOOP)
		//void RegisterHeaderRead();
		//void ReadHeaderWorker(std::error_code ec, std::size_t length);
		//void RegisterBodyRead(uint32_t header_size);
		//void ReadBodyWorker(std::error_code ec, std::size_t length);
	};


}