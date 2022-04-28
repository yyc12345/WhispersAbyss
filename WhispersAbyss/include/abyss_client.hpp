#pragma once

#include "output_helper.hpp"
#include <atomic>

namespace WhispersAbyss {

	class AbyssClient {
	public:
		std::atomic_bool mIsRunning;

		AbyssClient(OutputHelper* output, const char* server, const char* username);
		~AbyssClient();

		void Start();
		void Stop();
	private:

	};

}


