#include "../include/abyss_client.hpp"

namespace WhispersAbyss {

	AbyssClient::AbyssClient(OutputHelper* output, const char* server, const char* username) :
		mIsRunning(false)
	{

	}

	AbyssClient::~AbyssClient() {

	}

	void AbyssClient::Start() {
		mIsRunning.store(true);
	}

	void AbyssClient::Stop() {
		mIsRunning.store(false);
	}

}
