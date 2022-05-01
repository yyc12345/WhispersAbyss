#include "../include/cmmo_message.hpp"

namespace WhispersAbyss {
	namespace Cmmo {

#pragma region struct define

			PlayerEntity* WhispersAbyss::Cmmo::PlayerEntity::Clone() {
				auto obj = new PlayerEntity();
				CopyTo(obj);
				return obj;
			}
			void WhispersAbyss::Cmmo::PlayerEntity::CopyTo(Cmmo::PlayerEntity* obj) {
				obj->mPlayerId = this->mPlayerId;
				obj->mNickname = this->mNickname.c_str();
				obj->mCheated = this->mCheated;
			}
			void WhispersAbyss::Cmmo::PlayerEntity::InitFrom(Bmmo::PlayerRegisterV2* obj) {
				this->mPlayerId = obj->mPlayerId;
				this->mNickname = obj->mNickname.c_str();
				this->mCheated = obj->mCheated;
			}

#pragma endregion

		namespace Messages {

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
			OpCode WhispersAbyss::Cmmo::Messages::IMessage::GetOpCode() {
				return (OpCode)mInternalType;
			}
			OpCode IMessage::PeekOpCode(std::stringstream* data) {
				return (OpCode)peekInternalType(data);
			}

			//---------------------------------------
			// 
			//---------------------------------------

			OrderChat::OrderChat() { mInternalType = (uint32_t)OpCode::CS_OrderChat; }
			OrderChat::~OrderChat() {}
			bool OrderChat::Serialize(std::stringstream* data) {
				SSTREAM_PRE_WR(data);
				SSTREAM_WR_FUNCTION(data, Cmmo::Messages::IMessage::Serialize(data));

				SSTREAM_WR_STRING(data, mChatContent);

				SSTREAM_END_WR(data);
			}
			bool OrderChat::Deserialize(std::stringstream* data) {
				SSTREAM_PRE_RD(data);
				SSTREAM_RD_FUNCTION(data, Cmmo::Messages::IMessage::Deserialize(data));

				SSTREAM_RD_STRING(data, mChatContent);

				SSTREAM_END_RD(data);
			}
			Cmmo::Messages::IMessage* OrderChat::Clone() {
				auto obj = new Cmmo::Messages::OrderChat();
				CopyTo(obj);
				return obj;
			}
			void Cmmo::Messages::OrderChat::CopyTo(Cmmo::Messages::OrderChat* obj) {
				obj->mChatContent = this->mChatContent.c_str();
			}

			//---------------------------------------
			// 
			//---------------------------------------

			Chat::Chat() { mInternalType = (uint32_t)OpCode::SC_Chat; }
			Chat::~Chat() {}
			bool Chat::Serialize(std::stringstream* data) {
				SSTREAM_PRE_WR(data);
				SSTREAM_WR_FUNCTION(data, Cmmo::Messages::OrderChat::Serialize(data));

				SSTREAM_WR_STRUCT(data, BMMO_PLAYER_UUID, mPlayerId);

				SSTREAM_END_WR(data);
			}
			bool Chat::Deserialize(std::stringstream* data) {
				SSTREAM_PRE_RD(data);
				SSTREAM_RD_FUNCTION(data, Cmmo::Messages::OrderChat::Deserialize(data));

				SSTREAM_RD_STRUCT(data, BMMO_PLAYER_UUID, mPlayerId);

				SSTREAM_END_RD(data);
			}
			Cmmo::Messages::IMessage* Chat::Clone() {
				auto obj = new Cmmo::Messages::Chat();
				CopyTo(obj);
				obj->mPlayerId = this->mPlayerId;
				return obj;
			}

			//---------------------------------------
			// 
			//---------------------------------------

			OrderGlobalCheat::OrderGlobalCheat() { mInternalType = (uint32_t)OpCode::CS_OrderGlobalCheat; }
			OrderGlobalCheat::~OrderGlobalCheat() {}
			bool OrderGlobalCheat::Serialize(std::stringstream* data) {
				SSTREAM_PRE_WR(data);
				SSTREAM_WR_FUNCTION(data, Cmmo::Messages::IMessage::Serialize(data));

				SSTREAM_WR_STRUCT(data, uint8_t, mCheated);

				SSTREAM_END_WR(data);
			}

			bool OrderGlobalCheat::Deserialize(std::stringstream* data) {
				SSTREAM_PRE_RD(data);
				SSTREAM_RD_FUNCTION(data, Cmmo::Messages::IMessage::Deserialize(data));

				SSTREAM_RD_STRUCT(data, uint8_t, mCheated);

				SSTREAM_END_RD(data);
			}

			Cmmo::Messages::IMessage* OrderGlobalCheat::Clone() {
				auto obj = new Cmmo::Messages::OrderGlobalCheat();
				CopyTo(obj);
				return obj;
			}
			void Cmmo::Messages::OrderGlobalCheat::CopyTo(Cmmo::Messages::OrderGlobalCheat* obj) {
				obj->mCheated = this->mCheated;
			}

			//---------------------------------------
			// 
			//---------------------------------------

			OrderClientList::OrderClientList() { mInternalType = (uint32_t)OpCode::CS_OrderClientList; }
			OrderClientList::~OrderClientList() {}
			bool OrderClientList::Serialize(std::stringstream* data) {
				SSTREAM_PRE_WR(data);
				SSTREAM_WR_FUNCTION(data, Cmmo::Messages::IMessage::Serialize(data));
				SSTREAM_END_WR(data);
			}

			bool OrderClientList::Deserialize(std::stringstream* data) {
				SSTREAM_PRE_RD(data);
				SSTREAM_RD_FUNCTION(data, Cmmo::Messages::IMessage::Deserialize(data));
				SSTREAM_END_RD(data);
			}

			Cmmo::Messages::IMessage* OrderClientList::Clone() {
				auto obj = new Cmmo::Messages::OrderClientList();
				CopyTo(obj);
				return obj;
			}
			void Cmmo::Messages::OrderClientList::CopyTo(Cmmo::Messages::OrderClientList* obj) {
				;	// pass
			}

			//---------------------------------------
			// 
			//---------------------------------------

			ClientList::ClientList() { mInternalType = (uint32_t)OpCode::SC_ClientList; }
			ClientList::~ClientList() {}
			bool ClientList::Serialize(std::stringstream* data) {
				SSTREAM_PRE_WR(data);
				SSTREAM_WR_FUNCTION(data, Cmmo::Messages::OrderClientList::Serialize(data));

				uint32_t veclen = mOnlinePlayers.size();
				PlayerEntity* ptr = NULL;
				SSTREAM_WR_STRUCT(data, uint32_t, veclen);
				for (auto it = mOnlinePlayers.begin(); it != mOnlinePlayers.end(); ++it) {
					ptr = (*it);
					SSTREAM_WR_STRUCT(data, CMMO_PLAYER_UUID, ptr->mPlayerId);
					SSTREAM_WR_STRING(data, ptr->mNickname);
					SSTREAM_WR_STRUCT(data, uint8_t, ptr->mCheated);
				}

				SSTREAM_END_WR(data);
			}

			bool ClientList::Deserialize(std::stringstream* data) {
				SSTREAM_PRE_RD(data);
				SSTREAM_RD_FUNCTION(data, Cmmo::Messages::OrderClientList::Deserialize(data));

				uint32_t veclen = 0;
				PlayerEntity* ptr = NULL;
				SSTREAM_RD_STRUCT(data, uint32_t, veclen);
				mOnlinePlayers.reserve(veclen);
				for (uint32_t i = 0; i < veclen; ++i) {
					ptr = new PlayerEntity();
					SSTREAM_RD_STRUCT(data, CMMO_PLAYER_UUID, ptr->mPlayerId);
					SSTREAM_RD_STRING(data, ptr->mNickname);
					SSTREAM_RD_STRUCT(data, uint8_t, ptr->mCheated);
					mOnlinePlayers.push_back(ptr);
				}

				SSTREAM_END_RD(data);
			}

			Cmmo::Messages::IMessage* ClientList::Clone() {
				auto obj = new Cmmo::Messages::ClientList();
				CopyTo(obj);
				for (auto it = mOnlinePlayers.begin(); it != mOnlinePlayers.end(); ++it) {
					obj->mOnlinePlayers.push_back((*it)->Clone());
				}
				return obj;
			}

			//---------------------------------------
			// 
			//---------------------------------------

			BallState::BallState() { mInternalType = (uint32_t)OpCode::SC_BallState; }
			BallState::~BallState() {}
			bool BallState::Serialize(std::stringstream* data) {
				SSTREAM_PRE_WR(data);
				SSTREAM_WR_FUNCTION(data, Cmmo::Messages::IMessage::Serialize(data));

				SSTREAM_WR_STRUCT(data, CMMO_PLAYER_UUID, mPlayerId);
				SSTREAM_WR_STRUCT(data, uint32_t, mType);
				SSTREAM_WR_STRUCT(data, SharedMMO::VxVector, mPosition);
				SSTREAM_WR_STRUCT(data, SharedMMO::VxQuaternion, mRotation);

				SSTREAM_END_WR(data);
			}
			bool BallState::Deserialize(std::stringstream* data) {
				SSTREAM_PRE_RD(data);
				SSTREAM_RD_FUNCTION(data, Cmmo::Messages::IMessage::Deserialize(data));

				SSTREAM_RD_STRUCT(data, CMMO_PLAYER_UUID, mPlayerId);
				SSTREAM_RD_STRUCT(data, uint32_t, mType);
				SSTREAM_RD_STRUCT(data, SharedMMO::VxVector, mPosition);
				SSTREAM_RD_STRUCT(data, SharedMMO::VxQuaternion, mRotation);

				SSTREAM_END_RD(data);
			}
			Cmmo::Messages::IMessage* BallState::Clone() {
				auto obj = new Cmmo::Messages::BallState();
				obj->mPlayerId = this->mPlayerId;
				obj->mType = this->mType;
				memcpy(&obj->mPosition, &this->mPosition, sizeof(SharedMMO::VxVector));
				memcpy(&obj->mRotation, &this->mRotation, sizeof(SharedMMO::VxQuaternion));
				return obj;
			}

			//---------------------------------------
			// 
			//---------------------------------------

			ClientConnected::ClientConnected() { mInternalType = (uint32_t)OpCode::SC_ClientConnected; }
			ClientConnected::~ClientConnected() {}
			bool ClientConnected::Serialize(std::stringstream* data) {
				SSTREAM_PRE_WR(data);
				SSTREAM_WR_FUNCTION(data, Cmmo::Messages::IMessage::Serialize(data));

				SSTREAM_WR_STRING(data, mPlayer.mNickname);
				SSTREAM_WR_STRUCT(data, CMMO_PLAYER_UUID, mPlayer.mPlayerId);
				SSTREAM_WR_STRUCT(data, uint8_t, mPlayer.mCheated);

				SSTREAM_END_WR(data);
			}

			bool ClientConnected::Deserialize(std::stringstream* data) {
				SSTREAM_PRE_RD(data);
				SSTREAM_RD_FUNCTION(data, Cmmo::Messages::IMessage::Deserialize(data));

				SSTREAM_RD_STRING(data, mPlayer.mNickname);
				SSTREAM_RD_STRUCT(data, CMMO_PLAYER_UUID, mPlayer.mPlayerId);
				SSTREAM_RD_STRUCT(data, uint8_t, mPlayer.mCheated);

				SSTREAM_END_RD(data);
			}

			Cmmo::Messages::IMessage* ClientConnected::Clone() {
				auto obj = new Cmmo::Messages::ClientConnected();
				this->mPlayer.CopyTo(&obj->mPlayer);
				return obj;
			}

			//---------------------------------------
			// 
			//---------------------------------------

			ClientDisconnected::ClientDisconnected() { mInternalType = (uint32_t)OpCode::SC_ClientDisconnected; }
			ClientDisconnected::~ClientDisconnected() {}
			bool ClientDisconnected::Serialize(std::stringstream* data) {
				SSTREAM_PRE_WR(data);
				SSTREAM_WR_FUNCTION(data, Cmmo::Messages::IMessage::Serialize(data));
				
				SSTREAM_WR_STRUCT(data, CMMO_PLAYER_UUID, mPlayerId);

				SSTREAM_END_WR(data);
			}

			bool ClientDisconnected::Deserialize(std::stringstream* data) {
				SSTREAM_PRE_RD(data);
				SSTREAM_RD_FUNCTION(data, Cmmo::Messages::IMessage::Deserialize(data));
				
				SSTREAM_RD_STRUCT(data, CMMO_PLAYER_UUID, mPlayerId);

				SSTREAM_END_RD(data);
			}

			Cmmo::Messages::IMessage* ClientDisconnected::Clone() {
				auto obj = new Cmmo::Messages::ClientDisconnected();
				obj->mPlayerId = this->mPlayerId;
				return obj;
			}

			//---------------------------------------
			// 
			//---------------------------------------

			CheatState::CheatState() { mInternalType = (uint32_t)OpCode::SC_CheatState; }
			CheatState::~CheatState() {}
			bool CheatState::Serialize(std::stringstream* data) {
				SSTREAM_PRE_WR(data);
				SSTREAM_WR_FUNCTION(data, Cmmo::Messages::IMessage::Serialize(data));

				SSTREAM_WR_STRUCT(data, uint8_t, mCheated);
				SSTREAM_WR_STRUCT(data, CMMO_PLAYER_UUID, mPlayerId);

				SSTREAM_END_WR(data);
			}
			bool CheatState::Deserialize(std::stringstream* data) {
				SSTREAM_PRE_RD(data);
				SSTREAM_RD_FUNCTION(data, Cmmo::Messages::IMessage::Deserialize(data));

				SSTREAM_RD_STRUCT(data, uint8_t, mCheated);
				SSTREAM_RD_STRUCT(data, CMMO_PLAYER_UUID, mPlayerId);

				SSTREAM_END_RD(data);
			}
			Cmmo::Messages::IMessage* CheatState::Clone() {
				auto obj = new Cmmo::Messages::CheatState();
				obj->mPlayerId = this->mPlayerId;
				obj->mCheated = this->mCheated;
				return obj;
			}

			//---------------------------------------
			// 
			//---------------------------------------

			GlobalCheat::GlobalCheat() { mInternalType = (uint32_t)OpCode::SC_GlobalCheat; }
			GlobalCheat::~GlobalCheat() {}
			bool GlobalCheat::Serialize(std::stringstream* data) {
				SSTREAM_PRE_WR(data);
				SSTREAM_WR_FUNCTION(data, Cmmo::Messages::OrderGlobalCheat::Serialize(data));

				SSTREAM_WR_STRUCT(data, CMMO_PLAYER_UUID, mPlayerId);

				SSTREAM_END_WR(data);
			}
			bool GlobalCheat::Deserialize(std::stringstream* data) {
				SSTREAM_PRE_RD(data);
				SSTREAM_RD_FUNCTION(data, Cmmo::Messages::OrderGlobalCheat::Deserialize(data));

				SSTREAM_RD_STRUCT(data, CMMO_PLAYER_UUID, mPlayerId);

				SSTREAM_END_RD(data);
			}
			Cmmo::Messages::IMessage* GlobalCheat::Clone() {
				auto obj = new Cmmo::Messages::GlobalCheat();
				CopyTo(obj);
				obj->mPlayerId = this->mPlayerId;
				return obj;
			}

			//---------------------------------------
			// 
			//---------------------------------------

			LevelFinish::LevelFinish() { mInternalType = (uint32_t)OpCode::SC_LevelFinish; }
			LevelFinish::~LevelFinish() {}
			bool LevelFinish::Serialize(std::stringstream* data) {
				SSTREAM_PRE_WR(data);
				SSTREAM_WR_FUNCTION(data, Cmmo::Messages::IMessage::Serialize(data));

				SSTREAM_WR_STRUCT(data, BMMO_PLAYER_UUID, mPlayerId);
				SSTREAM_WR_STRUCT(data, int, mPoints);
				SSTREAM_WR_STRUCT(data, int, mLifes);
				SSTREAM_WR_STRUCT(data, int, mLifeBouns);
				SSTREAM_WR_STRUCT(data, int, mLevelBouns);
				SSTREAM_WR_STRUCT(data, float, mTimeElapsed);
				SSTREAM_WR_STRUCT(data, int, mStartPoints);
				SSTREAM_WR_STRUCT(data, int, mCurrentLevel);
				SSTREAM_WR_STRUCT(data, bool, mCheated);

				SSTREAM_END_WR(data);
			}
			bool LevelFinish::Deserialize(std::stringstream* data) {
				SSTREAM_PRE_RD(data);
				SSTREAM_RD_FUNCTION(data, Cmmo::Messages::IMessage::Deserialize(data));

				SSTREAM_RD_STRUCT(data, BMMO_PLAYER_UUID, mPlayerId);
				SSTREAM_RD_STRUCT(data, int, mPoints);
				SSTREAM_RD_STRUCT(data, int, mLifes);
				SSTREAM_RD_STRUCT(data, int, mLifeBouns);
				SSTREAM_RD_STRUCT(data, int, mLevelBouns);
				SSTREAM_RD_STRUCT(data, float, mTimeElapsed);
				SSTREAM_RD_STRUCT(data, int, mStartPoints);
				SSTREAM_RD_STRUCT(data, int, mCurrentLevel);
				SSTREAM_RD_STRUCT(data, bool, mCheated);

				SSTREAM_END_RD(data);
			}
			Cmmo::Messages::IMessage* LevelFinish::Clone() {
				auto obj = new Cmmo::Messages::LevelFinish();

				obj->mPlayerId = this->mPlayerId;

				obj->mPoints = this->mPoints;
				obj->mLifes = this->mLifes;
				obj->mLifeBouns = this->mLifeBouns;
				obj->mLevelBouns = this->mLevelBouns;

				obj->mTimeElapsed = this->mTimeElapsed;

				obj->mStartPoints = this->mStartPoints;
				obj->mCurrentLevel = this->mCurrentLevel;
				obj->mCheated = this->mCheated;

				return obj;
			}
}

	}
}