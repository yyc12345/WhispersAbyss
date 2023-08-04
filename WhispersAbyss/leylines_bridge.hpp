#pragma once
#include "celestia_gnosis.hpp"
#include "abyss_client.hpp"

namespace WhispersAbyss {

	class LeyLinesBridge {
	public:
		static uint64_t smIndexDistributor;

		ModuleStatus mStatus;
		std::mutex mStatusMutex;
		ModuleStatus GetConnStatus();

		LeyLinesBridge(OutputHelper* output, const char* server, TcpInstance* celestia);
		~LeyLinesBridge();

		void ActiveStop();
		void ProcessBridge();

		void ReportStatus();
	private:
		uint64_t mIndex;
		OutputHelper* mOutput;
		TcpInstance* mCelestiaClient;
		AbyssClient* mAbyssClient;
		std::deque<Bmmo::Message*> mMessages;

		std::thread mTdStop, mTdRunningDetector;
		void StopWorker();
		void DetectRunning();

		std::atomic_uint64_t mCelestiaCounter, mAbyssCounter;
	};

}
