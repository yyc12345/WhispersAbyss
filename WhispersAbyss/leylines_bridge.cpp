#include "leylines_bridge.hpp"

namespace WhispersAbyss {

	uint64_t LeyLinesBridge::smIndexDistributor = 0;

	LeyLinesBridge::LeyLinesBridge(OutputHelper* output, const char* server, TcpInstance* celestia) :
		mIndex(smIndexDistributor++),
		mOutput(output),
		mCelestiaClient(celestia),
		mAbyssClient(NULL),
		mTdStop(), mTdRunningDetector(),
		mStatus(ModuleStatus::Ready), mStatusMutex(),
		mMessages(),
		mCelestiaCounter(0), mAbyssCounter(0) {
		mAbyssClient = new AbyssClient(output, server);
		mStatus = ModuleStatus::Initializing;
		mTdRunningDetector = std::thread(&LeyLinesBridge::DetectRunning, this);
	}

	LeyLinesBridge::~LeyLinesBridge() {
		delete mCelestiaClient;
		delete mAbyssClient;

		if (mTdRunningDetector.joinable())
			mTdRunningDetector.join();
		if (mTdStop.joinable())
			mTdStop.join();
	}

	ModuleStatus LeyLinesBridge::GetConnStatus() {
		std::lock_guard<std::mutex> lockGuard(mStatusMutex);
		return mStatus;
	}

	void LeyLinesBridge::ActiveStop() {
		{
			std::lock_guard<std::mutex> lockGuard(mStatusMutex);
			switch (mStatus) {
				case WhispersAbyss::ModuleStatus::Ready:
				case WhispersAbyss::ModuleStatus::Initializing:
				case WhispersAbyss::ModuleStatus::Running:
					break;	// allow run stop
				case WhispersAbyss::ModuleStatus::Stopping:
				case WhispersAbyss::ModuleStatus::Stopped:
					return;	// not allowed to run stop
			}
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
				if (mStatus != ModuleStatus::Initializing) return;
			}
			if (mCelestiaClient->IsConnected()) break;
		}
		while (true) {
			std::this_thread::sleep_for(std::chrono::milliseconds(10));

			{	// leave when any client stopped before successfully running
				std::lock_guard<std::mutex> lockGuard(mStatusMutex);
				if (mStatus != ModuleStatus::Initializing) return;
			}
			if (mAbyssClient->GetConnStatus() == ModuleStatus::Running) break;
		}

		{
			std::lock_guard<std::mutex> lockGuard(mStatusMutex);
			mStatus = ModuleStatus::Running;
		}
	}

	void LeyLinesBridge::ReportStatus() {
		// status reporter
		mOutput->Printf("[Bridge#%" PRIu64 "] Client#%" PRIu64 ": %" PRIu64 " packages. Server#%" PRIu64 ": %" PRIu64 " packages.",
			mIndex,
			mCelestiaClient->mIndex,
			mCelestiaCounter.load(),
			mAbyssClient->mIndex,
			mAbyssCounter.load()
		);
	}

}
