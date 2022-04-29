#pragma once

#include <sstream>
#include "bmmo_message.hpp"
#include "cmmo_message.hpp"

#define SSTREAM_PRE_RD(ss) if (!(ss)->good()) \
return false;

#define SSTREAM_RD_STRUCT(ss, struct_type, variable) (ss)->read((char*)(&(variable)), sizeof(struct_type)); \
if (!(ss)->good() || (ss)->gcount() != sizeof(struct_type)) \
return false;

#define SSTREAM_RD_STRING(ss, strl) if (!inStdstring(ss, &(strl))) \
return false;

#define SSTREAM_RD_FUNCTION(ss, func) if (!(func)) \
return false;

#define SSTREAM_END_RD(ss) return (ss)->good();

#define SSTREAM_PRE_WR(ss) ;

#define SSTREAM_WR_STRUCT(ss, struct_type, variable) (ss)->write((char*)(&(variable)), sizeof(struct_type));

#define SSTREAM_WR_STRING(ss, strl) if (!outStdstring(ss, &(strl))) \
return false;

#define SSTREAM_WR_FUNCTION(ss, func) if (!(func)) \
return false;

#define SSTREAM_END_WR(ss) return (ss)->good();

namespace WhispersAbyss {
	namespace SharedMMO {

        Bmmo::Messages::IMessage* Cmmo2BmmoMsg(Cmmo::Messages::IMessage* msg);
        Cmmo::Messages::IMessage* Bmmo2CmmoMsg(Bmmo::Messages::IMessage* msg);

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

            bool inStdstring(std::stringstream* data, std::string* strl);
            bool outStdstring(std::stringstream* data, std::string* strl);
        };

	}
}
