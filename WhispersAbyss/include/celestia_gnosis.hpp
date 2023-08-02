#pragma once

#include <sdkddkver.h>	// need by asio
#include "asio.hpp"
#include "output_helper.hpp"
#include "bmmo_message.hpp"
#include "others_helper.hpp"
#include "state_machine.hpp"
#include <thread>
#include <deque>
#include <mutex>
#include <string>

namespace WhispersAbyss {

	class CelestiaGnosis {
	private:
		OutputHelper* mOutput;
		StateMachine::StateMachineCore mModuleStatus;
		asio::ip::tcp::socket mSocket;

		std::mutex mRecvMsgMutex, mSendMsgMutex;
		std::deque<Bmmo::Message*> mRecvMsg, mSendMsg;

		std::jthread mTdSend, mTdRecv;
	public:
		StateMachine::StateMachineReporter mStatusReporter;
		uint64_t mIndex;

	private:
		void SendWorker(std::stop_token st);
		void RecvWorker(std::stop_token st);

		void CheckSize(size_t msg_size);
	public:
		CelestiaGnosis(OutputHelper* output, uint64_t index, asio::ip::tcp::socket socket);
		~CelestiaGnosis();

		void Start();
		void Stop();

		void Send(std::deque<Bmmo::Message*>& manager_list);
		void Recv(std::deque<Bmmo::Message*>& manager_list);
	};


}