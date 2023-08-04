#pragma once
#include <cinttypes>
#include <mutex>
#include <chrono>
#include <deque>

namespace WhispersAbyss {

	constexpr const size_t NUKE_CAPACITY = 3072u;
	constexpr const size_t WARNING_CAPACITY = 2048u;
	constexpr const size_t STEAM_MSG_CAPACITY = 2048u;
	constexpr const std::chrono::milliseconds SPIN_INTERVAL(10);

	namespace DequeOperations {

		template<class T>
		void MoveDeque(std::deque<T>& _from, std::deque<T>& _to) {
			// https://stackoverflow.com/questions/49928501/better-way-to-move-objects-from-one-stddeque-to-another
			_to.insert(_to.end(),
				std::make_move_iterator(_from.begin()),
				std::make_move_iterator(_from.end())
			);

			_from.erase(_from.begin(), _from.end());
		}

		template<class T>
		void FreeDeque(std::deque<T>& _data) {
			//for (auto& ptr : _data) {
			//	delete ptr;
			//}
			_data.erase(_data.begin(), _data.end());
		}

	}

	class IndexDistributor {
	public:
		using Index_t = uint64_t;
		IndexDistributor() :
			mLock(), mCurrentIndex(0), mReturnedIndex() {}
		~IndexDistributor() {}

		Index_t Get() {
			std::lock_guard<std::mutex> locker(mLock);
			if (mReturnedIndex.empty()) return mCurrentIndex++;
			else {
				IndexDistributor::Index_t v = mReturnedIndex.front();
				mReturnedIndex.pop_front();
				return v;
			}
		}
		void Return(Index_t v) {
			std::lock_guard<std::mutex> locker(mLock);
			mReturnedIndex.push_back(v);
		}
	private:
		std::mutex mLock;
		Index_t mCurrentIndex;
		std::deque<Index_t> mReturnedIndex;
	};

}
