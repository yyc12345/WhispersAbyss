#pragma once

#include <cinttypes>
#include <stdexcept>
#include <sstream>
#include <vector>
#include <unordered_map>
#include "../include/shared_message.hpp"
#include <steam/steamnetworkingtypes.h>

#define BMMO_PLAYER_UUID HSteamNetConnection

namespace WhispersAbyss {
	namespace Bmmo {

        class PlayerStatusWithCheat {
        public:
            std::string name;
            uint8_t cheated;
        };

        enum PluginStage : uint8_t {
            Alpha,
            Beta,
            RC,
            Release
        };
        class PluginVersion {
        public:
            uint8_t mMajor;
            uint8_t mMinor;
            uint8_t mSubminor;
            PluginStage mStage;
            uint8_t mBuild;
        };

        enum OpCode : uint32_t {
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
        class IMessage : public SharedMMO::IMessage {
        public:
            static Bmmo::IMessage* CreateMessageFromStream(std::stringstream* data);

            IMessage();
            virtual ~IMessage();

            virtual bool Serialize(std::stringstream* data) override;
            virtual bool Deserialize(std::stringstream* data) override;
            virtual Bmmo::IMessage* Clone();
        };

        class BallState : public IMessage {
        public:
            BallState();
            virtual ~BallState();

            uint32_t mType;
            SharedMMO::VxVector mPosition;
            SharedMMO::VxQuaternion mRotation;
        };

        class Chat : public IMessage {
        public:
            Chat();
            virtual ~Chat();

            std::string mChatContent;
        };

        class CheatState : public IMessage {
        public:
            CheatState();
            virtual ~CheatState();

            uint8_t mCheated;
            uint8_t mNotify;
        };

        class CheatToggle : public IMessage {
        public:
            CheatToggle();
            virtual ~CheatToggle();

            uint8_t mCheated;
            uint8_t mNotify;
        };

        class LevelFinish : public IMessage {
        public:
            LevelFinish();
            virtual ~LevelFinish();

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

            std::unordered_map<BMMO_PLAYER_UUID, std::string*> mOnlinePlayers;
        };

        class LoginAcceptedV2 : public IMessage {
        public:
            LoginAcceptedV2();
            virtual ~LoginAcceptedV2();

            std::unordered_map<BMMO_PLAYER_UUID, PlayerStatusWithCheat*> mOnlinePlayers;
        };

        class LoginDenied : public IMessage {
        public:
            LoginDenied();
            virtual ~LoginDenied();
        };

        class LoginRequest : public IMessage {
        public:
            LoginRequest();
            virtual ~LoginRequest();

            std::string mNickname;
        };

        class LoginRequestV2 : public IMessage {
        public:
            LoginRequestV2();
            virtual ~LoginRequestV2();

            std::string mNickname;
            PluginVersion mVersion;
            uint8_t mCheated;
        };

        class OwnedBallState : public BallState {
        public:
            OwnedBallState();
            virtual ~OwnedBallState();

            BMMO_PLAYER_UUID mPlayerId;
        };

        class OwnedCheatState : public CheatState {
        public:
            OwnedCheatState();
            virtual ~OwnedCheatState();

            BMMO_PLAYER_UUID mPlayerId;
        };

        class OwnedCheatToggle : public CheatToggle {
        public:
            OwnedCheatToggle();
            virtual ~OwnedCheatToggle();

            BMMO_PLAYER_UUID mPlayerId;
        };

        class PlayerConnected : public IMessage {
        public:
            PlayerConnected();
            virtual ~PlayerConnected();

            BMMO_PLAYER_UUID mPlayerId;
            std::string mName;
        };

        class PlayerConnectedV2 : public IMessage {
        public:
            PlayerConnectedV2();
            virtual ~PlayerConnectedV2();

            BMMO_PLAYER_UUID mPlayerId;
            std::string mName;
            uint8_t mCheated;
        };

        class PlayerDisconnected : public IMessage {
        public:
            PlayerDisconnected();
            virtual ~PlayerDisconnected();

            BMMO_PLAYER_UUID mPlayerId;
        };

	}
}
