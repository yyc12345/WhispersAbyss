#pragma once

#include <sdkddkver.h>	// need by asio
#include "asio.hpp"
#include "others_helper.hpp"
#include "state_machine.hpp"
#include "output_helper.hpp"
#include "tcp_instance.hpp"
#include <deque>
#include <atomic>

namespace WhispersAbyss {

	class TcpFactory {
	private:
		OutputHelper* mOutput;
		StateMachine::StateMachineCore mModuleStatus;
		IndexDistributor mIndexDistributor;
		uint16_t mPort;
		
		asio::io_context mIoContext;	// this 2 decleartion should keep this order. due to init list order.
		asio::ip::tcp::acceptor mTcpAcceptor;
		
		std::thread mTdIoCtx;
		std::jthread mTdDisposal;

		std::mutex mConnectionsMutex, mDisposalConnsMutex;
		std::deque<TcpInstance*> mConnections, mDisposalConns;
	public:
		StateMachine::StateMachineReporter mStatusReporter;

	private:
		void AcceptorWorker(std::error_code ec, asio::ip::tcp::socket socket);
		void RegisterAsyncWork();
		void DisposalWorker(std::stop_token st);
	public:
		TcpFactory(OutputHelper* output, uint16_t port);
		TcpFactory(const TcpFactory& rhs) = delete;
		TcpFactory(TcpFactory&& rhs) = delete;
		~TcpFactory();

		void Stop();

		void GetConnections(std::deque<TcpInstance*>& conn_list);
		void ReturnConnections(TcpInstance* conn);
	};

}




