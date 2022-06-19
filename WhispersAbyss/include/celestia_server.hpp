#pragma once

#include <sdkddkver.h>	// need by asio
#include "asio.hpp"
#include "output_helper.hpp"
#include <atomic>
#include <deque>
#include "celestia_gnosis.hpp"

namespace WhispersAbyss {

	class CelestiaServer {
	public:
		std::atomic_bool mIsRunning;

		CelestiaServer(OutputHelper* output, uint16_t port);
		~CelestiaServer();

		void Start();
		void Stop();

		void GetConnections(std::deque<CelestiaGnosis*>* conn_list);
	private:
		uint64_t mIndexDistributor;
		OutputHelper* mOutput;
		uint16_t mPort;
		
		asio::io_context mIoContext;	// this 2 decleartion should keep this order. due to init list order.
		asio::ip::tcp::acceptor mTcpAcceptor;
		
		std::thread mTdCtx;

		std::mutex mConnectionsMutex;
		std::deque<CelestiaGnosis*> mConnections;

		void CtxWorker();
		void AcceptorWorker(std::error_code ec, asio::ip::tcp::socket socket);
		void RegisterAsyncWork();
	};

}




