#pragma once
#include "celestia_gnosis.hpp"
#include "abyss_client.hpp"

namespace WhispersAbyss {

	class LeyLinesBridge {
	public:
		std::atomic<ModuleStatus> mStatus;

		LeyLinesBridge(OutputHelper* output, const char* server, CelestiaGnosis* celestia);
		~LeyLinesBridge();

	private:
		OutputHelper* mOutput;
		CelestiaGnosis* mCelestiaClient;
		AbyssClient* mAbyssClient;
		std::deque<Bmmo::Message*> mMessages;

		void ActiveStop();
		void ProcessBridge();

		std::thread mTdStop, mTdRunningDetector;
		void StopWorker();
		void DetectRunning();
	};

}
