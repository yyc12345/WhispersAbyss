#include "../include/bmmo_message.hpp"

namespace WhispersAbyss {
	namespace Bmmo {

#pragma region IMessage

		Bmmo::IMessage* Bmmo::IMessage::CreateMessageFromStream(std::stringstream* data) {
			// peek opcode
			OpCode code{};
			data->read((char*)&code, sizeof(OpCode));
			data->seekg(-(int32_t)(sizeof(OpCode)), std::ios_base::cur);

			// check format
			Bmmo::IMessage* msg = NULL;
			bool parse_success = false;
			switch (code) {
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
				case WhispersAbyss::Bmmo::LoginRequestV2:
					break;
				case WhispersAbyss::Bmmo::LoginAcceptedV2:
					break;
				case WhispersAbyss::Bmmo::PlayerConnectedV2:
					break;
				case WhispersAbyss::Bmmo::CheatState:
					break;
				case WhispersAbyss::Bmmo::OwnedCheatState:
					break;
				case WhispersAbyss::Bmmo::CheatToggle:
					break;
				case WhispersAbyss::Bmmo::OwnedCheatToggle:
					break;
				case WhispersAbyss::Bmmo::None:
				default:
					msg = new Bmmo::IMessage();
			}

			parse_success = msg->Deserialize(data);
			if (!parse_success) {
				delete msg;
				msg = NULL;
			}
			return msg;
		}

		IMessage::IMessage() { mInternalType = OpCode::None; }
		IMessage::~IMessage() { }
		bool IMessage::Serialize(std::stringstream* data) {
			return outOpCode(data);
		}
		bool IMessage::Deserialize(std::stringstream* data) {
			return inOpCode(data);
		}
		Bmmo::IMessage* IMessage::Clone() {
			return new Bmmo::IMessage();
		}

#pragma endregion





	}
}
