#pragma once

#include <cinttypes>
#include <stdexcept>
#include <sstream>
#include <vector>
#include "../include/shared_message.hpp"

namespace WhispersAbyss {
	namespace Cmmo {

		enum OpCode : uint32_t {
			None,
			CS_OrderChat,
			CS_OrderGlobalCheat,
			SC_Chat,

			CS_OrderClientList,
			SC_ClientList,

			SC_BallState,

			SC_ClientConnected,
			SC_ClientDisconnected,
			SC_CheatToggle,
			SC_GlobalCheatToggle
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

		}

	}
}