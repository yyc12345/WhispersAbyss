#include "../include/cmmo_message.hpp"

namespace WhispersAbyss {
	namespace Cmmo {

		namespace Messages {

#pragma region IMessage

			Cmmo::Messages::IMessage* Cmmo::Messages::IMessage::CreateMessageFromStream(std::stringstream* data) {
				// peek opcode
				OpCode code{};
				data->read((char*)&code, sizeof(OpCode));
				data->seekg(-(int32_t)(sizeof(OpCode)), std::ios_base::cur);

				// check format
				switch (code) {
					case WhispersAbyss::Cmmo::OpCode::None:
						break;
					case WhispersAbyss::Cmmo::OpCode::CS_OrderChat:
						break;
					case WhispersAbyss::Cmmo::OpCode::SC_Chat:
						break;
					case WhispersAbyss::Cmmo::OpCode::CS_OrderGlobalCheat:
						break;
					case WhispersAbyss::Cmmo::OpCode::CS_OrderClientList:
						break;
					case WhispersAbyss::Cmmo::OpCode::SC_ClientList:
						break;
					case WhispersAbyss::Cmmo::OpCode::SC_BallState:
						break;
					case WhispersAbyss::Cmmo::OpCode::SC_ClientConnected:
						break;
					case WhispersAbyss::Cmmo::OpCode::SC_ClientDisconnected:
						break;
					case WhispersAbyss::Cmmo::OpCode::SC_CheatState:
						break;
					case WhispersAbyss::Cmmo::OpCode::SC_GlobalCheat:
						break;
					case WhispersAbyss::Cmmo::OpCode::SC_LevelFinish:
						break;
					default:
						throw std::logic_error("Invalid OpCode!");
				}

				auto msg = new Cmmo::Messages::IMessage();
				msg->Deserialize(data);
				return msg;
			}

			IMessage::IMessage() { mInternalType = (uint32_t)OpCode::None; }
			IMessage::~IMessage() {}
			bool IMessage::Serialize(std::stringstream* data) {
				SSTREAM_PRE_WR(data);
				SSTREAM_WR_FUNCTION(data, outOpCode(data));
				SSTREAM_END_WR(data);
			}
			bool IMessage::Deserialize(std::stringstream* data) {
				SSTREAM_PRE_RD(data);
				SSTREAM_RD_FUNCTION(data, inOpCode(data));
				SSTREAM_END_RD(data);
			}
			Cmmo::Messages::IMessage* IMessage::Clone() {
				return new Cmmo::Messages::IMessage();
			}

#pragma endregion

			//---------------------------------------
			// 
			//---------------------------------------

			OrderChat::OrderChat() { mInternalType = (uint32_t)OpCode::CS_OrderChat; }
			OrderChat::~OrderChat() {}
			bool OrderChat::Serialize(std::stringstream* data) {
				return false;
			}
			bool OrderChat::Deserialize(std::stringstream* data) {
				return false;
			}
			Cmmo::Messages::IMessage* OrderChat::Clone() {
				return nullptr;
			}
			void Cmmo::Messages::OrderChat::CopyTo(Cmmo::Messages::OrderChat* obj) {

			}

			//---------------------------------------
			// 
			//---------------------------------------

			Chat::Chat() { mInternalType = (uint32_t)OpCode::SC_Chat; }
			Chat::~Chat() {}
			bool Chat::Serialize(std::stringstream* data) {
				return false;
			}
			bool Chat::Deserialize(std::stringstream* data) {
				return false;
			}
			Cmmo::Messages::IMessage* Chat::Clone() {
				return nullptr;
			}

			//---------------------------------------
			// 
			//---------------------------------------

			OrderGlobalCheat::OrderGlobalCheat() { mInternalType = (uint32_t)OpCode::CS_OrderGlobalCheat; }
			OrderGlobalCheat::~OrderGlobalCheat() {}
			bool OrderGlobalCheat::Serialize(std::stringstream* data) {
				return false;
			}

			bool OrderGlobalCheat::Deserialize(std::stringstream* data) {
				return false;
			}

			Cmmo::Messages::IMessage* OrderGlobalCheat::Clone() {
				return nullptr;
			}
			void Cmmo::Messages::OrderGlobalCheat::CopyTo(Cmmo::Messages::OrderGlobalCheat* obj) {
			
			}

			//---------------------------------------
			// 
			//---------------------------------------

			OrderClientList::OrderClientList() { mInternalType = (uint32_t)OpCode::CS_OrderClientList; }
			OrderClientList::~OrderClientList() {}
			bool OrderClientList::Serialize(std::stringstream* data) {
				return false;
			}

			bool OrderClientList::Deserialize(std::stringstream* data) {
				return false;
			}

			Cmmo::Messages::IMessage* OrderClientList::Clone() {
				return nullptr;
			}
			void Cmmo::Messages::OrderClientList::CopyTo(Cmmo::Messages::OrderClientList* obj) {
			
			}

			//---------------------------------------
			// 
			//---------------------------------------

			ClientList::ClientList() { mInternalType = (uint32_t)OpCode::SC_ClientList; }
			ClientList::~ClientList() {}
			bool ClientList::Serialize(std::stringstream* data) {
				return false;
			}

			bool ClientList::Deserialize(std::stringstream* data) {
				return false;
			}

			Cmmo::Messages::IMessage* ClientList::Clone() {
				return nullptr;
			}

			//---------------------------------------
			// 
			//---------------------------------------

			BallState::BallState() { mInternalType = (uint32_t)OpCode::SC_BallState; }
			BallState::~BallState() {}
			bool BallState::Serialize(std::stringstream* data) {
				return false;
			}
			bool BallState::Deserialize(std::stringstream* data) {
				return false;
			}
			Cmmo::Messages::IMessage* BallState::Clone() {
				return nullptr;
			}

			//---------------------------------------
			// 
			//---------------------------------------

			ClientConnected::ClientConnected() { mInternalType = (uint32_t)OpCode::SC_ClientConnected; }
			ClientConnected::~ClientConnected() {}
			bool ClientConnected::Serialize(std::stringstream* data) {
				return false;
			}

			bool ClientConnected::Deserialize(std::stringstream* data) {
				return false;
			}

			Cmmo::Messages::IMessage* ClientConnected::Clone() {
				return nullptr;
			}

			//---------------------------------------
			// 
			//---------------------------------------

			ClientDisconnected::ClientDisconnected() { mInternalType = (uint32_t)OpCode::SC_ClientDisconnected; }
			ClientDisconnected::~ClientDisconnected() {}
			bool ClientDisconnected::Serialize(std::stringstream* data) {
				return false;
			}

			bool ClientDisconnected::Deserialize(std::stringstream* data) {
				return false;
			}

			Cmmo::Messages::IMessage* ClientDisconnected::Clone() {
				return nullptr;
			}

			//---------------------------------------
			// 
			//---------------------------------------

			CheatState::CheatState() { mInternalType = (uint32_t)OpCode::SC_CheatState; }
			CheatState::~CheatState() {}
			bool CheatState::Serialize(std::stringstream* data) {
				return false;
			}
			bool CheatState::Deserialize(std::stringstream* data) {
				return false;
			}
			Cmmo::Messages::IMessage* CheatState::Clone() {
				return nullptr;
			}

			//---------------------------------------
			// 
			//---------------------------------------

			GlobalCheat::GlobalCheat() { mInternalType = (uint32_t)OpCode::SC_GlobalCheat; }
			GlobalCheat::~GlobalCheat() {}
			bool GlobalCheat::Serialize(std::stringstream* data) {
				return false;
			}
			bool GlobalCheat::Deserialize(std::stringstream* data) {
				return false;
			}
			Cmmo::Messages::IMessage* GlobalCheat::Clone() {
				return nullptr;
			}

			//---------------------------------------
			// 
			//---------------------------------------

			LevelFinish::LevelFinish() { mInternalType = (uint32_t)OpCode::SC_LevelFinish; }
			LevelFinish::~LevelFinish() {}
			bool LevelFinish::Serialize(std::stringstream* data) {
				return false;
			}
			bool LevelFinish::Deserialize(std::stringstream* data) {
				return false;
			}
			Cmmo::Messages::IMessage* LevelFinish::Clone() {
				return nullptr;
			}
}

	}
}