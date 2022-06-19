#include "../include/leylines_bridge.hpp"

namespace WhispersAbyss {

	LeyLinesBridge::LeyLinesBridge(OutputHelper* output, const char* server, CelestiaGnosis* celestia) :
		mOutput(output),
		mCelestiaClient(celestia),
		mAbyssClient(NULL),
		mTdStop(), mTdRunningDetector(),
		mStatus(ModuleStatus::Initializing), mStatusMutex(),
		mMessages(),
		mCelestiaCounter(0), mAbyssCounter(0) {
		mAbyssClient = new AbyssClient(output, server);
		mStatus = ModuleStatus::Ready;
		mTdRunningDetector = std::thread(&LeyLinesBridge::DetectRunning, this);
	}

	LeyLinesBridge::~LeyLinesBridge() {
		delete mCelestiaClient;
		delete mAbyssClient;
	}

	ModuleStatus LeyLinesBridge::GetConnStatus() {
		std::lock_guard<std::mutex> lockGuard(mStatusMutex);
		return mStatus;
	}

	void LeyLinesBridge::ActiveStop() {
		{
			std::lock_guard<std::mutex> lockGuard(mStatusMutex);
			if (mStatus != ModuleStatus::Running) return;
			mStatus = ModuleStatus::Stopping;
		}

		mTdStop = std::thread(&LeyLinesBridge::StopWorker, this);
	}

	void LeyLinesBridge::ProcessBridge() {
		// when any client stopped, stop the whole bridge actively
		if (!(mCelestiaClient->IsConnected() && mAbyssClient->GetConnStatus() != ModuleStatus::Stopped)) ActiveStop();

		// check msg delivr requirements
		{
			std::lock_guard<std::mutex> lockGuard(mStatusMutex);
			if (mStatus != ModuleStatus::Running) return;
		}
		// move msg data
		// and collect data count
		// i know adding counter can be rewritten by fetch_add() or +=
		// but i never use CAS before so i want to try it.
		uint64_t count;
		mCelestiaClient->Recv(&mMessages);
		count = mCelestiaCounter.load();
		while (!mCelestiaCounter.compare_exchange_strong(count, count + mMessages.size()));
		mAbyssClient->Send(&mMessages);

		mAbyssClient->Recv(&mMessages);
		count = mAbyssCounter.load();
		while (!mAbyssCounter.compare_exchange_strong(count, count + mMessages.size()));
		mCelestiaClient->Send(&mMessages);
	}

	void LeyLinesBridge::StopWorker() {
		// order stop
		mCelestiaClient->Stop();
		mAbyssClient->Stop();

		// then wait real stop
		while (mCelestiaClient->IsConnected()) {
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}
		while (mAbyssClient->GetConnStatus() != ModuleStatus::Stopped) {
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}

		{
			std::lock_guard<std::mutex> lockGuard(mStatusMutex);
			mStatus = ModuleStatus::Stopped;
		}
	}

	void LeyLinesBridge::DetectRunning() {
		while (true) {
			std::this_thread::sleep_for(std::chrono::milliseconds(10));

			{	// leave when any client stopped before successfully running
				std::lock_guard<std::mutex> lockGuard(mStatusMutex);
				if (mStatus != ModuleStatus::Ready) return;
			}
			if (mCelestiaClient->IsConnected()) break;
		}
		while (true) {
			std::this_thread::sleep_for(std::chrono::milliseconds(10));

			{	// leave when any client stopped before successfully running
				std::lock_guard<std::mutex> lockGuard(mStatusMutex);
				if (mStatus != ModuleStatus::Ready) return;
			}
			if (mAbyssClient->GetConnStatus() == ModuleStatus::Running) break;
		}

		{
			std::lock_guard<std::mutex> lockGuard(mStatusMutex);
			mStatus = ModuleStatus::Running;
		}
	}

	void LeyLinesBridge::ReportStatus() {
		// todo: finish status reporter
		mCelestiaCounter.load();
		mAbyssCounter.load();
	}

}
