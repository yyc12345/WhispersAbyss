#include "../include/cmmo_message.hpp"

namespace WhispersAbyss {
	namespace Cmmo {

		Cmmo::IMessage* Cmmo::IMessage::CreateMessageFromStream(std::stringstream* data) {
			// peek opcode
			OpCode code{};
			data->read((char*)&code, sizeof(OpCode));
			data->seekg(-(int32_t)(sizeof(OpCode)), std::ios_base::cur);

			// check format
			switch (code) {
				case WhispersAbyss::Cmmo::None:
					break;
				case WhispersAbyss::Cmmo::CS_OrderChat:
					break;
				case WhispersAbyss::Cmmo::CS_OrderGlobalCheat:
					break;
				case WhispersAbyss::Cmmo::SC_Chat:
					break;
				case WhispersAbyss::Cmmo::CS_OrderClientList:
					break;
				case WhispersAbyss::Cmmo::SC_ClientList:
					break;
				case WhispersAbyss::Cmmo::SC_BallState:
					break;
				case WhispersAbyss::Cmmo::SC_ClientConnected:
					break;
				case WhispersAbyss::Cmmo::SC_ClientDisconnected:
					break;
				case WhispersAbyss::Cmmo::SC_CheatToggle:
					break;
				case WhispersAbyss::Cmmo::SC_GlobalCheatToggle:
					break;
				default:
					throw std::logic_error("Invalid OpCode!");
			}

			auto msg = new Cmmo::IMessage();
			msg->Deserialize(data);
			return msg;
		}

		IMessage::IMessage() {
			mInternalType = OpCode::None;
		}
		IMessage::~IMessage() {
		}
		bool IMessage::Serialize(std::stringstream* data) {
			return outOpCode(data);
		}
		bool IMessage::Deserialize(std::stringstream* data) {
			return inOpCode(data);
		}
		Cmmo::IMessage* IMessage::Clone() {
			return new Cmmo::IMessage();
		}
		//uint32_t IMessage::ExpectedSize() {
		//	return sizeof(OpCode);
		//}

	}
}