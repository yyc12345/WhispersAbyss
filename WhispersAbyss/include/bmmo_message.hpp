#pragma once

#include <cinttypes>
#include <stdexcept>
#include <vector>
#include <deque>
#include <string>

namespace WhispersAbyss {
	namespace Bmmo {

		class Message {
		public:
			Message();
			~Message();

			void ReadGnsData(void* ss, int send_flag, int len);
			void ReadTcpData(std::string* ss, bool is_reliable, uint32_t len);

			void* GetData();
			uint32_t GetDataLen();

			int GetGnsSendFlag();
			bool GetIsReliable();

		private:
			std::string mInternalBuffer;
			uint32_t mBufferLength;
			bool mIsReliable;
		};

		void DeleteCachedMessage(std::deque<Bmmo::Message*>* lst);
		void MoveCachedMessage(std::deque<Bmmo::Message*>* src, std::deque<Bmmo::Message*>* dst);
		
	}
}
