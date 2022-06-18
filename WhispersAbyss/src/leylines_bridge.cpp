#include "../include/leylines_bridge.hpp"

namespace WhispersAbyss {

	LeyLinesBridge::LeyLinesBridge(OutputHelper* output, const char* server, CelestiaGnosis* celestia) :
		mOutput(output),
		mCelestiaClient(celestia),
		mAbyssClient(NULL),
		mTdStop(), mTdRunningDetector(),
		mStatus(ModuleStatus::Initializing),
		mMessages()
	{
		mAbyssClient = new AbyssClient(output, server);
		mStatus.store(ModuleStatus::Ready);
		mTdRunningDetector = std::thread(&LeyLinesBridge::DetectRunning, this);
	}

	LeyLinesBridge::~LeyLinesBridge() {
		delete mCelestiaClient;
		delete mAbyssClient;
	}

	void LeyLinesBridge::ActiveStop() {
		if (mStatus.load() != ModuleStatus::Running) return;
		mStatus.store(ModuleStatus::Stopping);
		mTdStop = std::thread(&LeyLinesBridge::StopWorker, this);
	}

	void LeyLinesBridge::ProcessBridge() {
		// check requirements
		if (mStatus.load() != ModuleStatus::Running) return;
		// when any client stopped, stop the whole bridge actively
		if (!(mCelestiaClient->IsConnected() && mAbyssClient->mStatus.load() != ModuleStatus::Stopped)) ActiveStop();

		// otherwise, move msg data
		mCelestiaClient->Recv(&mMessages);
		mAbyssClient->Send(&mMessages);

		mAbyssClient->Recv(&mMessages);
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
		while (mAbyssClient->mIsDead.load()) {
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}

		mStatus.store(ModuleStatus::Stopped);
	}

	void LeyLinesBridge::DetectRunning() {
		while (!mCelestiaClient->IsConnected()) {
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}
		while (!mAbyssClient->mIsRunning.load()) {
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}

		mStatus.store(ModuleStatus::Running);
	}

}
