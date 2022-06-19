#pragma once
#include "celestia_gnosis.hpp"
#include "abyss_client.hpp"

namespace WhispersAbyss {

	class LeyLinesBridge {
	public:
		ModuleStatus mStatus;
		std::mutex mStatusMutex;
		ModuleStatus GetConnStatus();

		LeyLinesBridge(OutputHelper* output, const char* server, CelestiaGnosis* celestia);
		~LeyLinesBridge();

		void ActiveStop();
		void ProcessBridge();

		void ReportStatus();
	private:
		OutputHelper* mOutput;
		CelestiaGnosis* mCelestiaClient;
		AbyssClient* mAbyssClient;
		std::deque<Bmmo::Message*> mMessages;

		std::thread mTdStop, mTdRunningDetector;
		void StopWorker();
		void DetectRunning();

		std::atomic_uint64_t mCelestiaCounter, mAbyssCounter;
	};

}
