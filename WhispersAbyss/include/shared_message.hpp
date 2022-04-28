#pragma once

#include <sstream>

namespace WhispersAbyss {
	namespace SharedMMO {

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

        class IMessage {
        public:
            IMessage();
            virtual ~IMessage();

            virtual bool Serialize(std::stringstream* data)=0;
            virtual bool Deserialize(std::stringstream* data)=0;
            //virtual SharedMMO::IMessage* Clone();
            //virtual uint32_t ExpectedSize();
        protected:
            uint32_t mInternalType;
            bool inOpCode(std::stringstream* data);
            bool outOpCode(std::stringstream* data);
        };

	}
}
