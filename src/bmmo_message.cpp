#include "../include/bmmo_message.hpp"

namespace WhispersAbyss {
	namespace Bmmo {

		Bmmo::IMessage* IMessage::CreateMessageFromStream(std::stringstream* data) {
			// peek opcode
			OpCode code;
			data->read((char*)&code, sizeof(OpCode));
			data->seekg(-(int32_t)(sizeof(OpCode)), std::ios_base::cur);

			// check format
			switch (code) {
				case WhispersAbyss::Bmmo::None:
					break;
				case WhispersAbyss::Bmmo::LoginRequest:
					break;
				case WhispersAbyss::Bmmo::LoginAccepted:
					break;
				case WhispersAbyss::Bmmo::LoginDenied:
					break;
				case WhispersAbyss::Bmmo::PlayerDisconnected:
					break;
				case WhispersAbyss::Bmmo::PlayerConnected:
					break;
				case WhispersAbyss::Bmmo::Ping:
					break;
				case WhispersAbyss::Bmmo::BallState:
					break;
				case WhispersAbyss::Bmmo::OwnedBallState:
					break;
				case WhispersAbyss::Bmmo::KeyboardInput:
					break;
				case WhispersAbyss::Bmmo::Chat:
					break;
				case WhispersAbyss::Bmmo::LevelFinish:
					break;
				default:
					throw std::logic_error("Invalid OpCode!");
			}

			auto msg = new Bmmo::IMessage();
			msg->Deserialize(data);
			return msg;
		}

		IMessage::IMessage() :
			mInternalType(OpCode::None)
		{
		}
		IMessage::~IMessage() {
		}
		void IMessage::Serialize(std::stringstream* data) {
			data->write((const char*)mInternalType, sizeof(OpCode));
		}
		void IMessage::Deserialize(std::stringstream* data) {
			OpCode c;
			data->read((char*)&c, sizeof(OpCode));
			if (c != mInternalType) {
				throw std::logic_error("OpCode is not matched between prototype and raw data.");
			}
		}
		Bmmo::IMessage* IMessage::Clone() {
			return new Bmmo::IMessage();
		}
		//uint32_t IMessage::ExpectedSize() {
		//	return sizeof(OpCode);
		//}

	}
}
