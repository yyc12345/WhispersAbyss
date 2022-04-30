#pragma once

#include "output_helper.hpp"
#include <atomic>
#include "bmmo_message.hpp"

namespace WhispersAbyss {

	class AbyssClient {
	public:
		std::atomic_bool mIsRunning;

		AbyssClient(OutputHelper* output, const char* server, const char* username);
		~AbyssClient();

		void Start();
		void Stop();
	private:
		OutputHelper* mOutput;

		Bmmo::Messages::IMessage* CreateMessageFromStream(std::stringstream* data);

	};

}


