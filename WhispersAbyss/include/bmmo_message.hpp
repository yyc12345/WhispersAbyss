#pragma once

#include <cinttypes>
#include <stdexcept>
#include <sstream>
#include <vector>
#include "../include/shared_message.hpp"
#include <steam/steamnetworkingtypes.h>

#define BMMO_PLAYER_UUID HSteamNetConnection

namespace WhispersAbyss {
	namespace Bmmo {

		class PlayerRegister {
		public:
			BMMO_PLAYER_UUID mPlayerId;
			std::string mNickname;

			//bool Serialize(std::stringstream* data);
			//bool Deserialize(std::stringstream* data);
			PlayerRegister* Clone();
			void CopyTo(PlayerRegister* obj);
		};
		class PlayerRegisterV2 {
		public:
			BMMO_PLAYER_UUID mPlayerId;
			std::string mNickname;
			uint8_t mCheated;

			//bool Serialize(std::stringstream* data);
			//bool Deserialize(std::stringstream* data);
			PlayerRegisterV2* Clone();
			void CopyTo(PlayerRegisterV2* obj);
		};

		enum class PluginStage : uint8_t {
			Alpha,
			Beta,
			RC,
			Release
		};
		class PluginVersion {
		public:
			//PluginVersion();

			uint8_t mMajor;
			uint8_t mMinor;
			uint8_t mSubminor;
			PluginStage mStage;
			uint8_t mBuild;

			//bool Serialize(std::stringstream* data);
			//bool Deserialize(std::stringstream* data);
			//PluginVersion* Clone();
			void CopyTo(PluginVersion* obj);
		};

		enum class OpCode : uint32_t {
			None,
			LoginRequest,
			LoginAccepted,
			LoginDenied,
			PlayerDisconnected,
			PlayerConnected,

			Ping,
			BallState,
			OwnedBallState,
			KeyboardInput,

			Chat,

			LevelFinish,

			LoginRequestV2,
			LoginAcceptedV2,
			PlayerConnectedV2,

			CheatState,
			OwnedCheatState,
			CheatToggle,
			OwnedCheatToggle
		};

		/*
		Data Flow:

		---
		Celestia -> Abyss


		---
		Abyss -> Celestia

		AbyssClient (new IMessage) -> MainWorker (deliver) ->
		CelestiaServer (IMessage.Clone, clone into each CelestiaGnosis) ->
		CelestiaGnosis (delete ptr)

		*/
		namespace Messages {

			class IMessage : public WhispersAbyss::SharedMMO::IMessage {
			public:
				IMessage();
				virtual ~IMessage();

				virtual bool Serialize(std::stringstream* data) override;
				virtual bool Deserialize(std::stringstream* data) override;
				virtual Bmmo::Messages::IMessage* Clone();
				OpCode GetOpCode();
				int GetMessageSendFlag();
				static OpCode PeekOpCode(std::stringstream* data);
			protected:
				int mMessageSendFlag;
			};

			class BallState : public IMessage {
			public:
				BallState();
				virtual ~BallState();
				virtual bool Serialize(std::stringstream* data) override;
				virtual bool Deserialize(std::stringstream* data) override;
				virtual Bmmo::Messages::IMessage* Clone();

				uint32_t mType;
				SharedMMO::VxVector mPosition;
				SharedMMO::VxQuaternion mRotation;

			protected:
				void CopyTo(Bmmo::Messages::BallState* obj);
			};

			class Chat : public IMessage {
			public:
				Chat();
				virtual ~Chat();
				virtual bool Serialize(std::stringstream* data) override;
				virtual bool Deserialize(std::stringstream* data) override;
				virtual Bmmo::Messages::IMessage* Clone();

				BMMO_PLAYER_UUID mPlayerId;
				std::string mChatContent;
			};

			class CheatState : public IMessage {
			public:
				CheatState();
				virtual ~CheatState();
				virtual bool Serialize(std::stringstream* data) override;
				virtual bool Deserialize(std::stringstream* data) override;
				virtual Bmmo::Messages::IMessage* Clone();

				uint8_t mCheated;
				uint8_t mNotify;

			protected:
				void CopyTo(Bmmo::Messages::CheatState* obj);
			};

			class CheatToggle : public IMessage {
			public:
				CheatToggle();
				virtual ~CheatToggle();
				virtual bool Serialize(std::stringstream* data) override;
				virtual bool Deserialize(std::stringstream* data) override;
				virtual Bmmo::Messages::IMessage* Clone();

				uint8_t mCheated;
				uint8_t mNotify;

			protected:
				void CopyTo(Bmmo::Messages::CheatToggle* obj);
			};

			class LevelFinish : public IMessage {
			public:
				LevelFinish();
				virtual ~LevelFinish();
				virtual bool Serialize(std::stringstream* data) override;
				virtual bool Deserialize(std::stringstream* data) override;
				virtual Bmmo::Messages::IMessage* Clone();

				BMMO_PLAYER_UUID mPlayerId;
				int mPoints, mLifes, mLifeBouns, mLevelBouns;
				float mTimeElapsed;

				int mStartPoints, mCurrentLevel;
				bool mCheated;
			};

			class LoginAccepted : public IMessage {
			public:
				LoginAccepted();
				virtual ~LoginAccepted();
				virtual bool Serialize(std::stringstream* data) override;
				virtual bool Deserialize(std::stringstream* data) override;
				virtual Bmmo::Messages::IMessage* Clone();

				std::vector<Bmmo::PlayerRegister*> mOnlinePlayers;
			};

			class LoginAcceptedV2 : public IMessage {
			public:
				LoginAcceptedV2();
				virtual ~LoginAcceptedV2();
				virtual bool Serialize(std::stringstream* data) override;
				virtual bool Deserialize(std::stringstream* data) override;
				virtual Bmmo::Messages::IMessage* Clone();

				std::vector<Bmmo::PlayerRegisterV2*> mOnlinePlayers;
			};

			class LoginDenied : public IMessage {
			public:
				LoginDenied();
				virtual ~LoginDenied();
				virtual bool Serialize(std::stringstream* data) override;
				virtual bool Deserialize(std::stringstream* data) override;
				virtual Bmmo::Messages::IMessage* Clone();
			};

			class LoginRequest : public IMessage {
			public:
				LoginRequest();
				virtual ~LoginRequest();
				virtual bool Serialize(std::stringstream* data) override;
				virtual bool Deserialize(std::stringstream* data) override;
				virtual Bmmo::Messages::IMessage* Clone();

				std::string mNickname;
			};

			class LoginRequestV2 : public IMessage {
			public:
				LoginRequestV2();
				virtual ~LoginRequestV2();
				virtual bool Serialize(std::stringstream* data) override;
				virtual bool Deserialize(std::stringstream* data) override;
				virtual Bmmo::Messages::IMessage* Clone();

				std::string mNickname;
				Bmmo::PluginVersion mVersion;
				uint8_t mCheated;
			};

			class OwnedBallState : public BallState {
			public:
				OwnedBallState();
				virtual ~OwnedBallState();
				virtual bool Serialize(std::stringstream* data) override;
				virtual bool Deserialize(std::stringstream* data) override;
				virtual Bmmo::Messages::IMessage* Clone();

				BMMO_PLAYER_UUID mPlayerId;
			};

			class OwnedCheatState : public CheatState {
			public:
				OwnedCheatState();
				virtual ~OwnedCheatState();
				virtual bool Serialize(std::stringstream* data) override;
				virtual bool Deserialize(std::stringstream* data) override;
				virtual Bmmo::Messages::IMessage* Clone();

				BMMO_PLAYER_UUID mPlayerId;
			};

			class OwnedCheatToggle : public CheatToggle {
			public:
				OwnedCheatToggle();
				virtual ~OwnedCheatToggle();
				virtual bool Serialize(std::stringstream* data) override;
				virtual bool Deserialize(std::stringstream* data) override;
				virtual Bmmo::Messages::IMessage* Clone();

				BMMO_PLAYER_UUID mPlayerId;
			};

			class PlayerConnected : public IMessage {
			public:
				PlayerConnected();
				virtual ~PlayerConnected();
				virtual bool Serialize(std::stringstream* data) override;
				virtual bool Deserialize(std::stringstream* data) override;
				virtual Bmmo::Messages::IMessage* Clone();

				BMMO_PLAYER_UUID mPlayerId;
				std::string mName;
			};

			class PlayerConnectedV2 : public IMessage {
			public:
				PlayerConnectedV2();
				virtual ~PlayerConnectedV2();
				virtual bool Serialize(std::stringstream* data) override;
				virtual bool Deserialize(std::stringstream* data) override;
				virtual Bmmo::Messages::IMessage* Clone();

				BMMO_PLAYER_UUID mPlayerId;
				std::string mName;
				uint8_t mCheated;
			};

			class PlayerDisconnected : public IMessage {
			public:
				PlayerDisconnected();
				virtual ~PlayerDisconnected();
				virtual bool Serialize(std::stringstream* data) override;
				virtual bool Deserialize(std::stringstream* data) override;
				virtual Bmmo::Messages::IMessage* Clone();

				BMMO_PLAYER_UUID mPlayerId;
			};

		}
	}
}
