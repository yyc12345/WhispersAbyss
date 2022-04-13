#pragma once

#include <cinttypes>
#include <stdexcept>
#include <sstream>
#include <vector>

namespace WhispersAbyss {
	namespace Bmmo {

        struct VxVector {
            float x = 0.0;
            float y = 0.0;
            float z = 0.0;
        };

        struct VxQuaternion {
            float x = 0.0;
            float y = 0.0;
            float z = 0.0;
            float w = 0.0;
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

            LevelFinish
        };

		class IMessage {
		public:
            static IMessage* CreateMessageFromStream(std::stringstream* data);

			IMessage();
			~IMessage();

            virtual void Serialize(std::stringstream* data);
            virtual void Deserialize(std::stringstream* data);
            //virtual uint32_t ExpectedSize();
		protected:
            OpCode mInternalType;
		};


	}
}
