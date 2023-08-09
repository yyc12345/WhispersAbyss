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
	constexpr const std::chrono::milliseconds DISPOSAL_INTERVAL(500);

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
		IndexDistributor(const IndexDistributor& rhs) = delete;
		IndexDistributor(IndexDistributor&& rhs) = delete;
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
	constexpr const IndexDistributor::Index_t NO_INDEX = 0u;

	class OutputHelper {
	public:
		enum class Component {
			TcpInstance, TcpFactory,
			GnsInstance, GnsFactory,
			BridgeInstance, BridgeFactory,
			Core
		};

		OutputHelper();
		~OutputHelper();

		/// <summary>
		/// Print fatal error and try to nuke process.
		/// </summary>
		void FatalError(const char* fmt, ...);
		void FatalError(Component comp, IndexDistributor::Index_t index, const char* fmt, ...);
		/// <summary>
		/// Print normal log
		/// </summary>
		void Printf(const char* fmt, ...);
		void Printf(Component comp, IndexDistributor::Index_t index, const char* fmt, ...);
		/// <summary>
		/// Print log without timestamp.
		/// </summary>
		void RawPrintf(const char* fmt, ...);
	private:
		int64_t g_logTimeZero;

		void PrintTimestamp();
		void PrintComponent(Component comp, IndexDistributor::Index_t index);
		void PrintMessage(const char* fmt, va_list ap);
		void NukeProcess(int rc);
		int64_t GetSysTimeMicros();
	};

}
