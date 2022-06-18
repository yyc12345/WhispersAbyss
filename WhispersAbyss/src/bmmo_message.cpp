#include "../include/bmmo_message.hpp"
#include <deque>
#include <steam/steamnetworkingtypes.h>

namespace WhispersAbyss {
	namespace Bmmo {

#pragma region message class

		Message::Message() :
			mInternalBuffer(), mBufferLength(0), mIsReliable(true)
		{
			;
		}

		Message::~Message() {
		}

		void Message::ReadGnsData(void* ss, int send_flag, int len) {
			mInternalBuffer.resize(len);
			memcpy(mInternalBuffer.data(), ss, len);
			mIsReliable = (send_flag & k_nSteamNetworkingSend_Reliable) != 0;
			mBufferLength = len;
		}

		void Message::ReadTcpData(std::string* ss, bool is_reliable, uint32_t len) {
			mInternalBuffer.resize(len);
			memcpy(mInternalBuffer.data(), ss->data(), len);
			mIsReliable = is_reliable;
			mBufferLength = len;
		}

		void* Message::GetData() {
			return mInternalBuffer.data();
		}

		uint32_t Message::GetDataLen() {
			return mBufferLength;
		}

		int Message::GetGnsSendFlag() {
			if (mIsReliable) return k_nSteamNetworkingSend_Reliable;
			else return k_nSteamNetworkingSend_UnreliableNoNagle;
		}

		bool Message::GetIsReliable() {
			return mIsReliable;
		}

#pragma endregion

		void DeleteCachedMessage(std::deque<Bmmo::Message*>* lst) {
			WhispersAbyss::Bmmo::Message* ptr;
			while (lst->begin() != lst->end()) {
				ptr = *lst->begin();
				lst->pop_front();
				delete ptr;
			}
		}

		void MoveCachedMessage(std::deque<Bmmo::Message*>* src, std::deque<Bmmo::Message*>* dst) {
			while (src->begin() != src->end()) {
				dst->push_back(*src->begin());
				src->pop_front();
			}
		}

	}
}
