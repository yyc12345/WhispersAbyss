#pragma once
#include <deque>
#include "bmmo_message.hpp"

#define NUKE_CAPACITY 3072
#define WARNING_CAPACITY 2048
#define STEAM_MSG_CAPACITY 2048

namespace WhispersAbyss {

	enum class ModuleStatus : uint8_t {
		Ready,
		Initializing,
		Running,
		Stopping,
		Stopped
	};

}
