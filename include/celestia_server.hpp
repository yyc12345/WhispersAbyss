#pragma once

#include <sdkddkver.h>	// need by asio
#include "asio.hpp"
#include "output_helper.hpp"
#include <atomic>

namespace WhispersAbyss {

	class CelestiaServer {
	public:
		std::atomic_bool mIsRunning;

		CelestiaServer(OutputHelper* output, uint16_t port);
		~CelestiaServer();

		void Start();
		void Stop();

	private:
		uint16_t mPort;
		std::atomic_bool mStopBroadcast;
		asio::io_context mIoContext;	// this 2 decleartion should keep this order. due to init list order.
		asio::ip::tcp::acceptor mTcpAcceptor;
		std::thread mTdCtx, mTdBroadcast;

		void BroadcastWorker();
		void CtxWorker();
		void AcceptorWorker(std::error_code ec, asio::ip::tcp::socket socket);
		void RegisterAsyncWork();
	};

}




