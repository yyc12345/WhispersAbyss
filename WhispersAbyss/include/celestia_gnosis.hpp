#pragma once

#include <sdkddkver.h>	// need by asio
#include "asio.hpp"
#include "output_helper.hpp"
#include "bmmo_message.hpp"
#include "others_helper.hpp"
#include <thread>
#include <deque>
#include <mutex>
#include <string>

namespace WhispersAbyss {

	class CelestiaGnosis {
	private:
		OutputHelper* mOutput;
		ModuleStateMachine mModuleState;
		asio::ip::tcp::socket mSocket;

		std::mutex mRecvMsgMutex, mSendMsgMutex;
		std::deque<Bmmo::Message*> mRecvMsg, mSendMsg;

		//uint32_t mMsgHeader;
		//std::string mMsgBuffer;
		//std::stringstream mMsgStream;

		std::jthread mTdSend, mTdRecv;

		// Write routine:
		void SendWorker(std::stop_token st);
		void RecvWorker(std::stop_token st);

		void CheckSize(size_t msg_size);
		//// Read routine:
		//// RegisterHeaderRead() -- async --> ReadHeaderWorker() --> 
		//// RegisterBodyRead() -- async --> ReadBodyWorker() -->
		//// RegisterHeaderRead() (LOOP)
		//void RegisterHeaderRead();
		//void ReadHeaderWorker(std::error_code ec, std::size_t length);
		//void RegisterBodyRead(uint32_t header_size);
		//void ReadBodyWorker(std::error_code ec, std::size_t length);
	public:

		CelestiaGnosis(OutputHelper* output, uint64_t index, asio::ip::tcp::socket socket);
		~CelestiaGnosis();

		void Start();
		void Stop();

		ModuleStateReporter mStateReporter;
		uint64_t mIndex;
		void Send(std::deque<Bmmo::Message*>& manager_list);
		void Recv(std::deque<Bmmo::Message*>& manager_list);
	};


}