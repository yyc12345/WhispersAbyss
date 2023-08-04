#pragma once

#include <cinttypes>
#include <stdexcept>
#include <string>

namespace WhispersAbyss {

	class CommonMessage {
	public:
		CommonMessage() :
			mBuf(nullptr), mBufLen(0u), mIsReliable(true) {}
		CommonMessage(const CommonMessage& rhs) :
			mBuf(nullptr), mBufLen(rhs.mBufLen), mIsReliable(rhs.mIsReliable) {
			if (rhs.mBuf != nullptr) {
				mBuf = new char[mBufLen];
				memcpy(mBuf, rhs.mBuf, mBufLen);
			}
		}
		CommonMessage(CommonMessage&& rhs) noexcept :
			mBuf(rhs.mBuf), mBufLen(rhs.mBufLen), mIsReliable(rhs.mIsReliable) {
			if (rhs.mBuf != nullptr) {
				rhs.mBuf = nullptr;
				rhs.mBufLen = 0u;
			}
		}
		CommonMessage& operator=(const CommonMessage& rhs) {
			this->Clear();

			this->mIsReliable = rhs.mIsReliable;
			this->mBufLen = rhs.mBufLen;
			if (rhs.mBuf != nullptr) {
				mBuf = new char[mBufLen];
				memcpy(mBuf, rhs.mBuf, mBufLen);
			}

			return *this;
		}
		CommonMessage& operator=(CommonMessage&& rhs) noexcept {
			this->Clear();

			this->mIsReliable = rhs.mIsReliable;
			this->mBufLen = rhs.mBufLen;
			this->mBuf = rhs.mBuf;
			if (rhs.mBuf != nullptr) {
				rhs.mBuf = nullptr;
				rhs.mBufLen = 0u;
			}

			return *this;
		}
		~CommonMessage() {
			this->Clear();
		}

		const void* GetCommonData() const { return mBuf; }
		uint32_t GetCommonDataLen() const { return mBufLen; }
		bool GetTcpIsReliable() const { return mIsReliable; }

		void SetGnsData(const void* ss, int send_flag, int len);
		void SetTcpData(const void* ss, bool is_reliable, uint32_t len);
		int GetGnsSendFlag() const;
	private:
		void Clear() {
			if (mBuf != nullptr) {
				delete[] mBuf;
				mBuf = nullptr;
				mBufLen = 0u;
			}
		}

	private:
		char* mBuf;
		uint32_t mBufLen;
		bool mIsReliable;
	};

}
