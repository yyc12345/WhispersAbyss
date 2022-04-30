#include "../include/abyss_client.hpp"

namespace WhispersAbyss {

	AbyssClient::AbyssClient(OutputHelper* output, const char* server, const char* username) :
		mIsRunning(false), mOutput(output)
	{

	}

	AbyssClient::~AbyssClient() {

	}

	void AbyssClient::Start() {
		mIsRunning.store(true);
	}

	void AbyssClient::Stop() {
		mIsRunning.store(false);
	}

	Bmmo::Messages::IMessage* AbyssClient::CreateMessageFromStream(std::stringstream* data) {
		// peek opcode
		Bmmo::OpCode code = Bmmo::Messages::IMessage::PeekOpCode(data);

		// check format
		Bmmo::Messages::IMessage* msg = NULL;
		switch (code) {
			case WhispersAbyss::Bmmo::OpCode::LoginRequest:
				msg = new Bmmo::Messages::LoginRequest();
				break;
			case WhispersAbyss::Bmmo::OpCode::LoginAccepted:
				msg = new Bmmo::Messages::LoginAccepted();
				break;
			case WhispersAbyss::Bmmo::OpCode::LoginDenied:
				msg = new Bmmo::Messages::LoginDenied();
				break;
			case WhispersAbyss::Bmmo::OpCode::PlayerDisconnected:
				msg = new Bmmo::Messages::PlayerDisconnected();
				break;
			case WhispersAbyss::Bmmo::OpCode::PlayerConnected:
				msg = new Bmmo::Messages::PlayerConnected();
				break;
			case WhispersAbyss::Bmmo::OpCode::BallState:
				msg = new Bmmo::Messages::BallState();
				break;
			case WhispersAbyss::Bmmo::OpCode::OwnedBallState:
				msg = new Bmmo::Messages::OwnedBallState();
				break;
			case WhispersAbyss::Bmmo::OpCode::Chat:
				msg = new Bmmo::Messages::Chat();
				break;
			case WhispersAbyss::Bmmo::OpCode::LevelFinish:
				msg = new Bmmo::Messages::LevelFinish();
				break;
			case WhispersAbyss::Bmmo::OpCode::LoginRequestV2:
				msg = new Bmmo::Messages::LoginRequestV2();
				break;
			case WhispersAbyss::Bmmo::OpCode::LoginAcceptedV2:
				msg = new Bmmo::Messages::LoginAcceptedV2();
				break;
			case WhispersAbyss::Bmmo::OpCode::PlayerConnectedV2:
				msg = new Bmmo::Messages::PlayerConnectedV2();
				break;
			case WhispersAbyss::Bmmo::OpCode::CheatState:
				msg = new Bmmo::Messages::CheatState();
				break;
			case WhispersAbyss::Bmmo::OpCode::OwnedCheatState:
				msg = new Bmmo::Messages::OwnedCheatState();
				break;
			case WhispersAbyss::Bmmo::OpCode::CheatToggle:
				msg = new Bmmo::Messages::CheatToggle();
				break;
			case WhispersAbyss::Bmmo::OpCode::OwnedCheatToggle:
				msg = new Bmmo::Messages::OwnedCheatToggle();
				break;
			case WhispersAbyss::Bmmo::OpCode::KeyboardInput:
			case WhispersAbyss::Bmmo::OpCode::Ping:
			case WhispersAbyss::Bmmo::OpCode::None:
				mOutput->Printf("[Abyss] Invalid OpCode %d.", code);
				break;
			default:
				mOutput->Printf("[Abyss] Unknow OpCode %d.", code);
				break;
		}

		if (msg != NULL) {
			if (!msg->Deserialize(data)) {
				mOutput->Printf("[Abyss] Fail to parse message with OpCode %d.", code);
				delete msg;
				msg = NULL;
			}
		}
		return msg;
	}

}
