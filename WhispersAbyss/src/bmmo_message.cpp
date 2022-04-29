#include "../include/bmmo_message.hpp"

namespace WhispersAbyss {
	namespace Bmmo {

		PlayerRegister* PlayerRegister::Clone() {
			return nullptr;
		}
		PlayerRegisterV2* PlayerRegisterV2::Clone() {
			return nullptr;
		}

		namespace Messages {

#pragma region IMessage

			Bmmo::Messages::IMessage* Bmmo::Messages::IMessage::CreateMessageFromStream(std::stringstream* data) {
				// peek opcode
				OpCode code{};
				data->read((char*)&code, sizeof(OpCode));
				data->seekg(-(int32_t)(sizeof(OpCode)), std::ios_base::cur);

				// check format
				Bmmo::Messages::IMessage* msg = NULL;
				bool parse_success = false;
				switch (code) {
					case WhispersAbyss::Bmmo::OpCode::LoginRequest:
						break;
					case WhispersAbyss::Bmmo::OpCode::LoginAccepted:
						break;
					case WhispersAbyss::Bmmo::OpCode::LoginDenied:
						break;
					case WhispersAbyss::Bmmo::OpCode::PlayerDisconnected:
						break;
					case WhispersAbyss::Bmmo::OpCode::PlayerConnected:
						break;
					case WhispersAbyss::Bmmo::OpCode::Ping:
						break;
					case WhispersAbyss::Bmmo::OpCode::BallState:
						break;
					case WhispersAbyss::Bmmo::OpCode::OwnedBallState:
						break;
					case WhispersAbyss::Bmmo::OpCode::KeyboardInput:
						break;
					case WhispersAbyss::Bmmo::OpCode::Chat:
						break;
					case WhispersAbyss::Bmmo::OpCode::LevelFinish:
						break;
					case WhispersAbyss::Bmmo::OpCode::LoginRequestV2:
						break;
					case WhispersAbyss::Bmmo::OpCode::LoginAcceptedV2:
						break;
					case WhispersAbyss::Bmmo::OpCode::PlayerConnectedV2:
						break;
					case WhispersAbyss::Bmmo::OpCode::CheatState:
						break;
					case WhispersAbyss::Bmmo::OpCode::OwnedCheatState:
						break;
					case WhispersAbyss::Bmmo::OpCode::CheatToggle:
						break;
					case WhispersAbyss::Bmmo::OpCode::OwnedCheatToggle:
						break;
					case WhispersAbyss::Bmmo::OpCode::None:
					default:
						msg = new Bmmo::Messages::IMessage();
				}

				parse_success = msg->Deserialize(data);
				if (!parse_success) {
					delete msg;
					msg = NULL;
				}
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
			Bmmo::Messages::IMessage* IMessage::Clone() {
				return new Bmmo::Messages::IMessage();
			}

#pragma endregion

			//---------------------------------------
			// 
			//---------------------------------------

			BallState::BallState() { mInternalType = (uint32_t)OpCode::BallState; }
			BallState::~BallState() {}
			bool BallState::Serialize(std::stringstream* data) {
				SSTREAM_PRE_WR(data);
				SSTREAM_WR_FUNCTION(data, Bmmo::Messages::IMessage::Serialize(data));

				SSTREAM_WR_STRUCT(data, uint32_t, mType);
				SSTREAM_WR_STRUCT(data, SharedMMO::VxVector, mPosition);
				SSTREAM_WR_STRUCT(data, SharedMMO::VxQuaternion, mRotation);

				SSTREAM_END_WR(data);
			}
			bool BallState::Deserialize(std::stringstream* data) {
				SSTREAM_PRE_RD(data);
				SSTREAM_RD_FUNCTION(data, Bmmo::Messages::IMessage::Deserialize(data));

				SSTREAM_RD_STRUCT(data, uint32_t, mType);
				SSTREAM_RD_STRUCT(data, SharedMMO::VxVector, mPosition);
				SSTREAM_RD_STRUCT(data, SharedMMO::VxQuaternion, mRotation);

				SSTREAM_END_RD(data);
			}
			Bmmo::Messages::IMessage* BallState::Clone() {
				Bmmo::Messages::BallState* obj = new Bmmo::Messages::BallState();
				CopyTo(obj);
				return obj;
			}
			void Bmmo::Messages::BallState::CopyTo(Bmmo::Messages::BallState* obj) {
				obj->mType = this->mType;
				memcpy(&obj->mPosition, &this->mPosition, sizeof(SharedMMO::VxVector));
				memcpy(&obj->mRotation, &this->mRotation, sizeof(SharedMMO::VxQuaternion));
			}

			//---------------------------------------
			// 
			//---------------------------------------

			Chat::Chat() { mInternalType = (uint32_t)OpCode::Chat; }
			Chat::~Chat() {}
			bool Chat::Serialize(std::stringstream* data) {
				SSTREAM_PRE_WR(data);
				SSTREAM_WR_FUNCTION(data, Bmmo::Messages::IMessage::Serialize(data));

				SSTREAM_WR_STRING(data, mChatContent);

				SSTREAM_END_WR(data);
			}
			bool Chat::Deserialize(std::stringstream* data) {
				SSTREAM_PRE_RD(data);
				SSTREAM_RD_FUNCTION(data, Bmmo::Messages::IMessage::Deserialize(data));

				SSTREAM_RD_STRING(data, mChatContent);

				SSTREAM_END_RD(data);
			}
			Bmmo::Messages::IMessage* Chat::Clone() {
				Bmmo::Messages::Chat* obj = new Bmmo::Messages::Chat();
				obj->mChatContent = this->mChatContent.c_str();
				return obj;
			}

			//---------------------------------------
			// 
			//---------------------------------------

			CheatState::CheatState() { mInternalType = (uint32_t)OpCode::CheatState; }
			CheatState::~CheatState() {}
			bool CheatState::Serialize(std::stringstream* data) {
				SSTREAM_PRE_WR(data);
				SSTREAM_WR_FUNCTION(data, Bmmo::Messages::IMessage::Serialize(data));

				SSTREAM_WR_STRUCT(data, uint8_t, mCheated);
				SSTREAM_WR_STRUCT(data, uint8_t, mNotify);

				SSTREAM_END_WR(data);
			}
			bool CheatState::Deserialize(std::stringstream* data) {
				SSTREAM_PRE_RD(data);
				SSTREAM_RD_FUNCTION(data, Bmmo::Messages::IMessage::Deserialize(data));

				SSTREAM_RD_STRUCT(data, uint8_t, mCheated);
				SSTREAM_RD_STRUCT(data, uint8_t, mNotify);

				SSTREAM_END_RD(data);
			}
			Bmmo::Messages::IMessage* CheatState::Clone() {
				Bmmo::Messages::CheatState* obj = new Bmmo::Messages::CheatState();
				CopyTo(obj);
				return obj;
			}
			void Bmmo::Messages::CheatState::CopyTo(Bmmo::Messages::CheatState* obj) {
				obj->mCheated = this->mCheated;
				obj->mNotify = this->mNotify;
			}

			//---------------------------------------
			// 
			//---------------------------------------

			CheatToggle::CheatToggle() { mInternalType = (uint32_t)OpCode::CheatToggle; }
			CheatToggle::~CheatToggle() {}
			bool CheatToggle::Serialize(std::stringstream* data) {
				SSTREAM_PRE_WR(data);
				SSTREAM_WR_FUNCTION(data, Bmmo::Messages::IMessage::Serialize(data));

				SSTREAM_WR_STRUCT(data, uint8_t, mCheated);
				SSTREAM_WR_STRUCT(data, uint8_t, mNotify);

				SSTREAM_END_WR(data);
			}
			bool CheatToggle::Deserialize(std::stringstream* data) {
				SSTREAM_PRE_RD(data);
				SSTREAM_RD_FUNCTION(data, Bmmo::Messages::IMessage::Deserialize(data));

				SSTREAM_RD_STRUCT(data, uint8_t, mCheated);
				SSTREAM_RD_STRUCT(data, uint8_t, mNotify);

				SSTREAM_END_RD(data);
			}
			Bmmo::Messages::IMessage* CheatToggle::Clone() {
				Bmmo::Messages::CheatToggle* obj = new Bmmo::Messages::CheatToggle();
				CopyTo(obj);
				return obj;
			}
			void Bmmo::Messages::CheatToggle::CopyTo(Bmmo::Messages::CheatToggle* obj) {
				obj->mCheated = this->mCheated;
				obj->mNotify = this->mNotify;
			}

			//---------------------------------------
			// 
			//---------------------------------------

			LevelFinish::LevelFinish() { mInternalType = (uint32_t)OpCode::LevelFinish; }
			LevelFinish::~LevelFinish() {}
			bool LevelFinish::Serialize(std::stringstream* data) {
				SSTREAM_PRE_WR(data);
				SSTREAM_WR_FUNCTION(data, Bmmo::Messages::IMessage::Serialize(data));

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
				SSTREAM_RD_FUNCTION(data, Bmmo::Messages::IMessage::Deserialize(data));

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

			Bmmo::Messages::IMessage* LevelFinish::Clone() {
				Bmmo::Messages::LevelFinish* obj = new Bmmo::Messages::LevelFinish();

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

			//---------------------------------------
			// 
			//---------------------------------------

			LoginAccepted::LoginAccepted() { mInternalType = (uint32_t)OpCode::LoginAccepted; }
			LoginAccepted::~LoginAccepted() {
				Bmmo::PlayerRegister* ptr = NULL;
				for (auto it = mOnlinePlayers.begin(); it != mOnlinePlayers.end(); ++it) {
					ptr = (*it);
					delete ptr;
				}
				mOnlinePlayers.clear();
			}
			bool LoginAccepted::Serialize(std::stringstream* data) {
				SSTREAM_PRE_WR(data);
				SSTREAM_WR_FUNCTION(data, Bmmo::Messages::IMessage::Serialize(data));

				uint32_t veclen = mOnlinePlayers.size();
				Bmmo::PlayerRegister* ptr = NULL;
				SSTREAM_WR_STRUCT(data, uint32_t, veclen);
				for (auto it = mOnlinePlayers.begin(); it != mOnlinePlayers.end(); ++it) {
					ptr = (*it);
					SSTREAM_WR_STRING(data, ptr->mNickname);
					SSTREAM_WR_STRUCT(data, BMMO_PLAYER_UUID, ptr->mPlayerId);
				}

				SSTREAM_END_WR(data);
			}

			bool LoginAccepted::Deserialize(std::stringstream* data) {
				SSTREAM_PRE_RD(data);
				SSTREAM_RD_FUNCTION(data, Bmmo::Messages::IMessage::Deserialize(data));

				uint32_t veclen = 0;
				Bmmo::PlayerRegister* ptr = NULL;
				SSTREAM_RD_STRUCT(data, uint32_t, veclen);
				mOnlinePlayers.reserve(veclen);
				for (uint32_t i = 0; i < veclen; ++i) {
					ptr = new Bmmo::PlayerRegister();
					SSTREAM_RD_STRING(data, ptr->mNickname);
					SSTREAM_RD_STRUCT(data, BMMO_PLAYER_UUID, ptr->mPlayerId);
					mOnlinePlayers.push_back(ptr);
				}

				SSTREAM_END_RD(data);
			}

			Bmmo::Messages::IMessage* LoginAccepted::Clone() {
				Bmmo::Messages::LoginAccepted* obj = new	Bmmo::Messages::LoginAccepted();

				for (auto it = mOnlinePlayers.begin(); it != mOnlinePlayers.end(); ++it) {
					obj->mOnlinePlayers.push_back((*it)->Clone());
				}

				return obj;
			}

			//---------------------------------------
			// 
			//---------------------------------------

			LoginAcceptedV2::LoginAcceptedV2() { mInternalType = (uint32_t)OpCode::LoginAcceptedV2; }
			LoginAcceptedV2::~LoginAcceptedV2() {
				Bmmo::PlayerRegisterV2* ptr = NULL;
				for (auto it = mOnlinePlayers.begin(); it != mOnlinePlayers.end(); ++it) {
					ptr = (*it);
					delete ptr;
				}
				mOnlinePlayers.clear();
			}
			bool LoginAcceptedV2::Serialize(std::stringstream* data) {
				SSTREAM_PRE_WR(data);
				SSTREAM_WR_FUNCTION(data, Bmmo::Messages::IMessage::Serialize(data));

				uint32_t veclen = mOnlinePlayers.size();
				Bmmo::PlayerRegisterV2* ptr = NULL;
				SSTREAM_WR_STRUCT(data, uint32_t, veclen);
				for (auto it = mOnlinePlayers.begin(); it != mOnlinePlayers.end(); ++it) {
					ptr = (*it);
					SSTREAM_WR_STRING(data, ptr->mNickname);
					SSTREAM_WR_STRUCT(data, BMMO_PLAYER_UUID, ptr->mPlayerId);
					SSTREAM_WR_STRUCT(data, uint8_t, ptr->mCheated);
				}

				SSTREAM_END_WR(data);
			}

			bool LoginAcceptedV2::Deserialize(std::stringstream* data) {
				SSTREAM_PRE_RD(data);
				SSTREAM_RD_FUNCTION(data, Bmmo::Messages::IMessage::Deserialize(data));

				uint32_t veclen = 0;
				Bmmo::PlayerRegisterV2* ptr = NULL;
				SSTREAM_RD_STRUCT(data, uint32_t, veclen);
				mOnlinePlayers.reserve(veclen);
				for (uint32_t i = 0; i < veclen; ++i) {
					ptr = new Bmmo::PlayerRegisterV2();
					SSTREAM_RD_STRING(data, ptr->mNickname);
					SSTREAM_RD_STRUCT(data, BMMO_PLAYER_UUID, ptr->mPlayerId);
					SSTREAM_RD_STRUCT(data, uint8_t, ptr->mCheated);
					mOnlinePlayers.push_back(ptr);
				}

				SSTREAM_END_RD(data);
			}

			Bmmo::Messages::IMessage* LoginAcceptedV2::Clone() {
				Bmmo::Messages::LoginAcceptedV2* obj = new	Bmmo::Messages::LoginAcceptedV2();

				for (auto it = mOnlinePlayers.begin(); it != mOnlinePlayers.end(); ++it) {
					obj->mOnlinePlayers.push_back((*it)->Clone());
				}

				return obj;
			}

			//---------------------------------------
			// 
			//---------------------------------------

			LoginDenied::LoginDenied() { mInternalType = (uint32_t)OpCode::LoginDenied; }
			LoginDenied::~LoginDenied() {}
			bool LoginDenied::Serialize(std::stringstream* data) {
				SSTREAM_PRE_WR(data);
				SSTREAM_WR_FUNCTION(data, Bmmo::Messages::IMessage::Serialize(data));
				SSTREAM_END_WR(data);
			}

			bool LoginDenied::Deserialize(std::stringstream* data) {
				SSTREAM_PRE_RD(data);
				SSTREAM_RD_FUNCTION(data, Bmmo::Messages::IMessage::Deserialize(data));
				SSTREAM_END_RD(data);
			}

			Bmmo::Messages::IMessage* LoginDenied::Clone() {
				return new Bmmo::Messages::LoginDenied();
			}

			//---------------------------------------
			// 
			//---------------------------------------

			LoginRequest::LoginRequest() { mInternalType = (uint32_t)OpCode::LoginRequest; }
			LoginRequest::~LoginRequest() {}
			bool LoginRequest::Serialize(std::stringstream* data) {
				SSTREAM_PRE_WR(data);
				SSTREAM_WR_FUNCTION(data, Bmmo::Messages::IMessage::Serialize(data));

				SSTREAM_WR_STRING(data, mNickname);

				SSTREAM_END_WR(data);
			}

			bool LoginRequest::Deserialize(std::stringstream* data) {
				SSTREAM_PRE_RD(data);
				SSTREAM_RD_FUNCTION(data, Bmmo::Messages::IMessage::Deserialize(data));

				SSTREAM_RD_STRING(data, mNickname);

				SSTREAM_END_RD(data);
			}

			Bmmo::Messages::IMessage* LoginRequest::Clone() {
				Bmmo::Messages::LoginRequest* obj = new Bmmo::Messages::LoginRequest();
				obj->mNickname = this->mNickname.c_str();
				return obj;
			}

			//---------------------------------------
			// 
			//---------------------------------------

			LoginRequestV2::LoginRequestV2() { mInternalType = (uint32_t)OpCode::LoginRequestV2; }
			LoginRequestV2::~LoginRequestV2() {}
			bool LoginRequestV2::Serialize(std::stringstream* data) {
				SSTREAM_PRE_WR(data);
				SSTREAM_WR_FUNCTION(data, Bmmo::Messages::IMessage::Serialize(data));

				SSTREAM_WR_STRING(data, mNickname);
				SSTREAM_WR_STRUCT(data, uint8_t, mVersion.mMajor);
				SSTREAM_WR_STRUCT(data, uint8_t, mVersion.mMinor);
				SSTREAM_WR_STRUCT(data, uint8_t, mVersion.mSubminor);
				SSTREAM_WR_STRUCT(data, uint8_t, mVersion.mStage);
				SSTREAM_WR_STRUCT(data, uint8_t, mVersion.mBuild);
				SSTREAM_WR_STRUCT(data, uint8_t, mCheated);

				SSTREAM_END_WR(data);
			}

			bool LoginRequestV2::Deserialize(std::stringstream* data) {
				SSTREAM_PRE_RD(data);
				SSTREAM_RD_FUNCTION(data, Bmmo::Messages::IMessage::Deserialize(data));

				SSTREAM_RD_STRING(data, mNickname);
				SSTREAM_RD_STRUCT(data, uint8_t, mVersion.mMajor);
				SSTREAM_RD_STRUCT(data, uint8_t, mVersion.mMinor);
				SSTREAM_RD_STRUCT(data, uint8_t, mVersion.mSubminor);
				SSTREAM_RD_STRUCT(data, uint8_t, mVersion.mStage);
				SSTREAM_RD_STRUCT(data, uint8_t, mVersion.mBuild);
				SSTREAM_RD_STRUCT(data, uint8_t, mCheated);

				SSTREAM_END_RD(data);
			}

			Bmmo::Messages::IMessage* LoginRequestV2::Clone() {
				Bmmo::Messages::LoginRequestV2* obj = new Bmmo::Messages::LoginRequestV2();
				obj->mNickname = this->mNickname.c_str();
				this->mVersion.CopyTo(&obj->mVersion);
				obj->mCheated = this->mCheated;
				return obj;
			}

			//---------------------------------------
			// 
			//---------------------------------------

			OwnedBallState::OwnedBallState() { mInternalType = (uint32_t)OpCode::OwnedBallState; }
			OwnedBallState::~OwnedBallState() {}
			bool OwnedBallState::Serialize(std::stringstream* data) {
				SSTREAM_PRE_WR(data);
				SSTREAM_WR_FUNCTION(data, Bmmo::Messages::BallState::Serialize(data));
				SSTREAM_WR_STRUCT(data, BMMO_PLAYER_UUID, mPlayerId);
				SSTREAM_END_WR(data);
			}
			bool OwnedBallState::Deserialize(std::stringstream* data) {
				SSTREAM_PRE_RD(data);
				SSTREAM_RD_FUNCTION(data, Bmmo::Messages::BallState::Deserialize(data));
				SSTREAM_RD_STRUCT(data, BMMO_PLAYER_UUID, mPlayerId);
				SSTREAM_END_RD(data);
			}
			Bmmo::Messages::IMessage* OwnedBallState::Clone() {
				Bmmo::Messages::OwnedBallState* obj = new Bmmo::Messages::OwnedBallState();
				CopyTo(obj);
				obj->mPlayerId = this->mPlayerId;
				return obj;
			}

			//---------------------------------------
			// 
			//---------------------------------------

			OwnedCheatState::OwnedCheatState() { mInternalType = (uint32_t)OpCode::OwnedCheatState; }
			OwnedCheatState::~OwnedCheatState() {}
			bool OwnedCheatState::Serialize(std::stringstream* data) {
				SSTREAM_PRE_WR(data);
				SSTREAM_WR_FUNCTION(data, Bmmo::Messages::CheatState::Serialize(data));
				SSTREAM_WR_STRUCT(data, BMMO_PLAYER_UUID, mPlayerId);
				SSTREAM_END_WR(data);
			}
			bool OwnedCheatState::Deserialize(std::stringstream* data) {
				SSTREAM_PRE_RD(data);
				SSTREAM_RD_FUNCTION(data, Bmmo::Messages::CheatState::Deserialize(data));
				SSTREAM_RD_STRUCT(data, BMMO_PLAYER_UUID, mPlayerId);
				SSTREAM_END_RD(data);
			}
			Bmmo::Messages::IMessage* OwnedCheatState::Clone() {
				Bmmo::Messages::OwnedCheatState* obj = new Bmmo::Messages::OwnedCheatState();
				CopyTo(obj);
				obj->mPlayerId = this->mPlayerId;
				return obj;
			}

			//---------------------------------------
			// 
			//---------------------------------------

			OwnedCheatToggle::OwnedCheatToggle() { mInternalType = (uint32_t)OpCode::OwnedCheatToggle; }
			OwnedCheatToggle::~OwnedCheatToggle() {}
			bool OwnedCheatToggle::Serialize(std::stringstream* data) {
				SSTREAM_PRE_WR(data);
				SSTREAM_WR_FUNCTION(data, Bmmo::Messages::CheatToggle::Serialize(data));
				SSTREAM_WR_STRUCT(data, BMMO_PLAYER_UUID, mPlayerId);
				SSTREAM_END_WR(data);
			}

			bool OwnedCheatToggle::Deserialize(std::stringstream* data) {
				SSTREAM_PRE_RD(data);
				SSTREAM_RD_FUNCTION(data, Bmmo::Messages::CheatToggle::Deserialize(data));
				SSTREAM_RD_STRUCT(data, BMMO_PLAYER_UUID, mPlayerId);
				SSTREAM_END_RD(data);
			}

			Bmmo::Messages::IMessage* OwnedCheatToggle::Clone() {
				Bmmo::Messages::OwnedCheatToggle* obj = new Bmmo::Messages::OwnedCheatToggle();
				CopyTo(obj);
				obj->mPlayerId = this->mPlayerId;
				return obj;
			}

			//---------------------------------------
			// 
			//---------------------------------------

			PlayerConnected::PlayerConnected() { mInternalType = (uint32_t)OpCode::PlayerConnected; }
			PlayerConnected::~PlayerConnected() {}
			bool PlayerConnected::Serialize(std::stringstream* data) {
				SSTREAM_PRE_WR(data);
				SSTREAM_WR_FUNCTION(data, Bmmo::Messages::IMessage::Serialize(data));

				SSTREAM_WR_STRUCT(data, BMMO_PLAYER_UUID, mPlayerId);
				SSTREAM_WR_STRING(data, mName);

				SSTREAM_END_WR(data);
			}

			bool PlayerConnected::Deserialize(std::stringstream* data) {
				SSTREAM_PRE_RD(data);
				SSTREAM_RD_FUNCTION(data, Bmmo::Messages::IMessage::Deserialize(data));

				SSTREAM_RD_STRUCT(data, BMMO_PLAYER_UUID, mPlayerId);
				SSTREAM_RD_STRING(data, mName);

				SSTREAM_END_RD(data);
			}

			Bmmo::Messages::IMessage* PlayerConnected::Clone() {
				Bmmo::Messages::PlayerConnected* obj = new Bmmo::Messages::PlayerConnected();
				obj->mPlayerId = this->mPlayerId;
				obj->mName = this->mName.c_str();
				return obj;
			}

			//---------------------------------------
			// 
			//---------------------------------------

			PlayerConnectedV2::PlayerConnectedV2() { mInternalType = (uint32_t)OpCode::PlayerConnectedV2; }
			PlayerConnectedV2::~PlayerConnectedV2() {}
			bool PlayerConnectedV2::Serialize(std::stringstream* data) {
				SSTREAM_PRE_WR(data);
				SSTREAM_WR_FUNCTION(data, Bmmo::Messages::IMessage::Serialize(data));

				SSTREAM_WR_STRUCT(data, BMMO_PLAYER_UUID, mPlayerId);
				SSTREAM_WR_STRING(data, mName);
				SSTREAM_WR_STRUCT(data, uint8_t, mCheated);

				SSTREAM_END_WR(data);
			}

			bool PlayerConnectedV2::Deserialize(std::stringstream* data) {
				SSTREAM_PRE_RD(data);
				SSTREAM_RD_FUNCTION(data, Bmmo::Messages::IMessage::Deserialize(data));

				SSTREAM_RD_STRUCT(data, BMMO_PLAYER_UUID, mPlayerId);
				SSTREAM_RD_STRING(data, mName);
				SSTREAM_RD_STRUCT(data, uint8_t, mCheated);

				SSTREAM_END_RD(data);
			}

			Bmmo::Messages::IMessage* PlayerConnectedV2::Clone() {
				Bmmo::Messages::PlayerConnectedV2* obj = new Bmmo::Messages::PlayerConnectedV2();
				obj->mPlayerId = this->mPlayerId;
				obj->mName = this->mName.c_str();
				obj->mCheated = this->mCheated;
				return obj;
			}

			//---------------------------------------
			// 
			//---------------------------------------

			PlayerDisconnected::PlayerDisconnected() { mInternalType = (uint32_t)OpCode::PlayerDisconnected; }
			PlayerDisconnected::~PlayerDisconnected() {}
			bool PlayerDisconnected::Serialize(std::stringstream* data) {
				SSTREAM_PRE_WR(data);
				SSTREAM_WR_FUNCTION(data, Bmmo::Messages::IMessage::Serialize(data));

				SSTREAM_WR_STRUCT(data, BMMO_PLAYER_UUID, mPlayerId);

				SSTREAM_END_WR(data);
			}

			bool PlayerDisconnected::Deserialize(std::stringstream* data) {
				SSTREAM_PRE_RD(data);
				SSTREAM_RD_FUNCTION(data, Bmmo::Messages::IMessage::Deserialize(data));

				SSTREAM_RD_STRUCT(data, BMMO_PLAYER_UUID, mPlayerId);

				SSTREAM_END_RD(data);
			}

			Bmmo::Messages::IMessage* PlayerDisconnected::Clone() {
				Bmmo::Messages::PlayerDisconnected* obj = new Bmmo::Messages::PlayerDisconnected();
				obj->mPlayerId = this->mPlayerId;
				return obj;
			}

		}
	}
}
