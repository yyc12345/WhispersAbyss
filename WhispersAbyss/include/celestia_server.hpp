#pragma once

#include <sdkddkver.h>	// need by asio
#include "asio.hpp"
#include "others_helper.hpp"
#include "output_helper.hpp"
#include "celestia_gnosis.hpp"
#include <deque>
#include <atomic>

namespace WhispersAbyss {

	class CelestiaServer {
	private:
		OutputHelper* mOutput;
		ModuleStateMachine mModuleState;
		IndexDistributor mIndexDistributor;
		uint16_t mPort;
		
		asio::io_context mIoContext;	// this 2 decleartion should keep this order. due to init list order.
		asio::ip::tcp::acceptor mTcpAcceptor;
		
		std::thread mTdIoCtx;
		std::jthread mTdDisposal;

		std::mutex mConnectionsMutex, mDisposalConnsMutex;
		std::deque<CelestiaGnosis*> mConnections, mDisposalConns;

		void AcceptorWorker(std::error_code ec, asio::ip::tcp::socket socket);
		void RegisterAsyncWork();
		void DisposalWorker(std::stop_token st);
	public:
		CelestiaServer(OutputHelper* output, uint16_t port);
		~CelestiaServer();

		void Start();
		void Stop();

		ModuleStateReporter mStateReporter;
		void GetConnections(std::deque<CelestiaGnosis*>& conn_list);
		void ReturnConnections(std::deque<CelestiaGnosis*>& conn_list);
	};

}




