#include "../include/shared_message.hpp"

namespace WhispersAbyss {
	namespace SharedMMO {

		bool IMessage::inOpCode(std::stringstream* data) {
			uint32_t c = 0;
			data->read((char*)&c, sizeof(uint32_t));
			return c == mInternalType;
		}

		bool IMessage::outOpCode(std::stringstream* data) {
			data->write((const char*)mInternalType, sizeof(uint32_t));
			return true;
		}

	}
}
