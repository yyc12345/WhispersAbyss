#pragma once

#include "bmmo_message.hpp"
#include "cmmo_message.hpp"

namespace WhispersAbyss {
	namespace SharedMMO {

		Bmmo::Messages::IMessage* Cmmo2Bmmo(Cmmo::Messages::IMessage* msg);
		Cmmo::Messages::IMessage* Bmmo2Cmmo(Bmmo::Messages::IMessage* msg);

	}
}

