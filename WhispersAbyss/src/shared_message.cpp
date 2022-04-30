#include "../include/shared_message.hpp"

namespace WhispersAbyss {
	namespace SharedMMO {

		uint32_t WhispersAbyss::SharedMMO::IMessage::peekInternalType(std::stringstream* data) {
			uint32_t code = 0;
			data->read((char*)&code, sizeof(uint32_t));
			data->seekg(-(int32_t)(sizeof(uint32_t)), std::ios_base::cur);
			return code;
		}

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

	}
}
