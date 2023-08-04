#include "messages.hpp"
#include <steam/steamnetworkingtypes.h>

namespace WhispersAbyss {

	void CommonMessage::SetGnsData(const void* ss, int send_flag, int len) {
		this->Clear();

		this->mBuf = new char[len];
		memcpy(this->mBuf, ss, len);
		this->mBufLen = len;
		this->mIsReliable = (send_flag & k_nSteamNetworkingSend_Reliable) != 0;
	}

	void CommonMessage::SetTcpData(const void* ss, bool is_reliable, uint32_t len) {
		this->Clear();

		this->mBuf = new char[len];
		memcpy(this->mBuf, ss, len);
		this->mBufLen = len;
		this->mIsReliable = is_reliable;
	}

	int CommonMessage::GetGnsSendFlag() const {
		if (mIsReliable) return k_nSteamNetworkingSend_Reliable;
		else return k_nSteamNetworkingSend_UnreliableNoNagle;
	}

}
