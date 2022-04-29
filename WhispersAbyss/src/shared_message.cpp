#include "../include/shared_message.hpp"

namespace WhispersAbyss {
	namespace SharedMMO {

		bool IMessage::inOpCode(std::stringstream* data) {
			uint32_t c = 0;
			SSTREAM_RD_STRUCT(data, uint32_t, c);
			return c == mInternalType;
		}

		bool IMessage::outOpCode(std::stringstream* data) {
			SSTREAM_WR_STRUCT(data, uint32_t, mInternalType);
			return true;
		}

		bool IMessage::inStdstring(std::stringstream* data, std::string* strl) {
			uint32_t length = 0;
			SSTREAM_RD_STRUCT(data, uint32_t, length);
			if (length > data->str().length()) return false;

			strl->resize(length);
			data->read(strl->data(), length);
			if (!data->good() || data->gcount() != length) \
				return false;

			return true;
		}

		bool IMessage::outStdstring(std::stringstream* data, std::string* strl) {
			uint32_t length = strl->size();
			SSTREAM_WR_STRUCT(data, uint32_t, length);
			data->write(strl->c_str(), length);

			return true;
		}

		Bmmo::Messages::IMessage* Cmmo2BmmoMsg(Cmmo::Messages::IMessage* msg) {
			return nullptr;
		}

		Cmmo::Messages::IMessage* Bmmo2CmmoMsg(Bmmo::Messages::IMessage* msg) {
			return nullptr;
		}

	}
}
