#pragma once

#include <sdkddkver.h>	// need by asio
#include "asio.hpp"
#include "output_helper.hpp"
#include <atomic>
#include <deque>
#include "cmmo_message.hpp"
#include "celestia_gnosis.hpp"

namespace WhispersAbyss {

	class CelestiaServer {
	public:
		std::atomic_bool mIsRunning;

		CelestiaServer(OutputHelper* output, const char* port);
		~CelestiaServer();

		void Start();
		void Stop();

		void Send(std::deque<Cmmo::Messages::IMessage*>* manager_list);
		void Recv(std::deque<Cmmo::Messages::IMessage*>* manager_list);
	private:
		uint64_t mIndexDistributor;
		OutputHelper* mOutput;
		uint16_t mPort;
		std::atomic_bool mStopBroadcast;
		
		asio::io_context mIoContext;	// this 2 decleartion should keep this order. due to init list order.
		asio::ip::tcp::acceptor mTcpAcceptor;
		
		std::thread mTdCtx, mTdBroadcast;

		std::mutex mRecvMsgMutex, mSendMsgMutex, mConnectionsMutex;
		std::deque<Cmmo::Messages::IMessage*> mRecvMsg, mSendMsg;
		std::deque<CelestiaGnosis*> mConnections;

		void BroadcastWorker();
		void CtxWorker();
		void AcceptorWorker(std::error_code ec, asio::ip::tcp::socket socket);
		void RegisterAsyncWork();
	};

}




