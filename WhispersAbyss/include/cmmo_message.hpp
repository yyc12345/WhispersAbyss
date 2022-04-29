#pragma once

#include <cinttypes>
#include <stdexcept>
#include <sstream>
#include <vector>
#include "../include/shared_message.hpp"

#define CMMO_PLAYER_UUID uint32_t

namespace WhispersAbyss {
	namespace Cmmo {

		enum class OpCode : uint32_t {
			None,
			CS_OrderChat,
			SC_Chat,

			CS_OrderGlobalCheat,

			CS_OrderClientList,
			SC_ClientList,

			SC_BallState,

			SC_ClientConnected,
			SC_ClientDisconnected,
			SC_CheatState,
			SC_GlobalCheat,
			SC_LevelFinish
		};

		namespace Messages {

			class IMessage : public SharedMMO::IMessage {
			public:
				static Cmmo::Messages::IMessage* CreateMessageFromStream(std::stringstream* data);

				IMessage();
				virtual ~IMessage();

				virtual bool Serialize(std::stringstream* data) override;
				virtual bool Deserialize(std::stringstream* data) override;
				virtual Cmmo::Messages::IMessage* Clone();
			};

			class OrderChat : public IMessage {
			public:
				OrderChat();
				virtual ~OrderChat();
				virtual bool Serialize(std::stringstream* data) override;
				virtual bool Deserialize(std::stringstream* data) override;
				virtual Cmmo::Messages::IMessage* Clone();

				std::string mChatContent;
			protected:
				void CopyTo(Cmmo::Messages::OrderChat* obj);
			};

			class Chat : public OrderChat {
			public:
				Chat();
				virtual ~Chat();
				virtual bool Serialize(std::stringstream* data) override;
				virtual bool Deserialize(std::stringstream* data) override;
				virtual Cmmo::Messages::IMessage* Clone();

				CMMO_PLAYER_UUID mPlayerId;
			};

			class OrderGlobalCheat : public IMessage {
			public:
				OrderGlobalCheat();
				virtual ~OrderGlobalCheat();
				virtual bool Serialize(std::stringstream* data) override;
				virtual bool Deserialize(std::stringstream* data) override;
				virtual Cmmo::Messages::IMessage* Clone();

				uint8_t mCheated;
			protected:
				void CopyTo(Cmmo::Messages::OrderGlobalCheat* obj);
			};

			class OrderClientList : public IMessage {
			public:
				OrderClientList();
				virtual ~OrderClientList();
				virtual bool Serialize(std::stringstream* data) override;
				virtual bool Deserialize(std::stringstream* data) override;
				virtual Cmmo::Messages::IMessage* Clone();
			protected:
				void CopyTo(Cmmo::Messages::OrderClientList* obj);
			};

			class ClientList : public OrderClientList {
			public:
				ClientList();
				virtual ~ClientList();
				virtual bool Serialize(std::stringstream* data) override;
				virtual bool Deserialize(std::stringstream* data) override;
				virtual Cmmo::Messages::IMessage* Clone();

				std::vector<Bmmo::PlayerRegisterV2*> mOnlinePlayers;
			};

			class BallState : public IMessage {
			public:
				BallState();
				virtual ~BallState();
				virtual bool Serialize(std::stringstream* data) override;
				virtual bool Deserialize(std::stringstream* data) override;
				virtual Cmmo::Messages::IMessage* Clone();

				uint32_t mType;
				SharedMMO::VxVector mPosition;
				SharedMMO::VxQuaternion mRotation;
			};

			class ClientConnected : public IMessage {
			public:
				ClientConnected();
				virtual ~ClientConnected();
				virtual bool Serialize(std::stringstream* data) override;
				virtual bool Deserialize(std::stringstream* data) override;
				virtual Cmmo::Messages::IMessage* Clone();

				Bmmo::PlayerRegisterV2 mPlayer;
			};

			class ClientDisconnected : public IMessage {
			public:
				ClientDisconnected();
				virtual ~ClientDisconnected();
				virtual bool Serialize(std::stringstream* data) override;
				virtual bool Deserialize(std::stringstream* data) override;
				virtual Cmmo::Messages::IMessage* Clone();

				CMMO_PLAYER_UUID mPlayerId;
			};

			class CheatState : public IMessage {
			public:
				CheatState();
				virtual ~CheatState();
				virtual bool Serialize(std::stringstream* data) override;
				virtual bool Deserialize(std::stringstream* data) override;
				virtual Cmmo::Messages::IMessage* Clone();
			};

			class GlobalCheat : public OrderGlobalCheat {
			public:
				GlobalCheat();
				virtual ~GlobalCheat();
				virtual bool Serialize(std::stringstream* data) override;
				virtual bool Deserialize(std::stringstream* data) override;
				virtual Cmmo::Messages::IMessage* Clone();

				CMMO_PLAYER_UUID mPlayerId;
			};

			class LevelFinish : public IMessage {
			public:
				LevelFinish();
				virtual ~LevelFinish();
				virtual bool Serialize(std::stringstream* data) override;
				virtual bool Deserialize(std::stringstream* data) override;
				virtual Cmmo::Messages::IMessage* Clone();

				BMMO_PLAYER_UUID mPlayerId;
				int mPoints, mLifes, mLifeBouns, mLevelBouns;
				float mTimeElapsed;

				int mStartPoints, mCurrentLevel;
				bool mCheated;
			};

		}

	}
}