#pragma once
#include <cinttypes>
#include <mutex>
#include <chrono>
#include <deque>
#include <functional>
#include <stdexcept>

namespace WhispersAbyss {

	constexpr const size_t NUKE_CAPACITY = 3072u;
	constexpr const size_t WARNING_CAPACITY = 2048u;
	constexpr const size_t STEAM_MSG_CAPACITY = 2048u;

	/// <summary>
	/// The interval for spin wait.
	/// </summary>
	constexpr const std::chrono::milliseconds SPIN_INTERVAL(10);
	/// <summary>
	/// Bridge factory used context worker interval
	/// </summary>
	constexpr const std::chrono::milliseconds BRIDGE_INTERVAL(50);
	/// <summary>
	/// The interval for disposal worker
	/// </summary>
	constexpr const std::chrono::milliseconds DISPOSAL_INTERVAL(500);
	/// <summary>
	/// The interval for waiting module starting to running.
	/// </summary>
	constexpr const double MODULE_WAITING_INTERVAL = 10000;	// 10 secs

	namespace StateMachine {
		using State_t = uint32_t;
		constexpr const State_t None = 0;
		constexpr const State_t Ready = 0b1;
		constexpr const State_t Running = 0b10;
		constexpr const State_t Stopped = 0b100;
	}

	namespace CommonOpers {

		template<class T>
		void MoveDeque(std::deque<T>& _from, std::deque<T>& _to) {
			// https://stackoverflow.com/questions/49928501/better-way-to-move-objects-from-one-stddeque-to-another
			_to.insert(_to.end(),
				std::make_move_iterator(_from.begin()),
				std::make_move_iterator(_from.end())
			);

			_from.erase(_from.begin(), _from.end());
		}

		const char* State2String(StateMachine::State_t state);
		void AppendStrF(std::string& strl, const char* fmt, ...);

	}

	class CountDownTimer {
	private:
		std::chrono::steady_clock::time_point mTimeStart;
		double mDuration;

	public:
		CountDownTimer(double duration_ms) : mTimeStart(std::chrono::steady_clock::now()), mDuration(duration_ms) {}
		CountDownTimer(const CountDownTimer& rhs) = delete;
		CountDownTimer(CountDownTimer&& rhs) = delete;
		~CountDownTimer() {}

		/// <summary>
		/// Return true if run out of time
		/// </summary>
		bool HasRunOutOfTime() {
			std::chrono::steady_clock::time_point timeend(std::chrono::steady_clock::now());
			return (std::chrono::duration<double, std::milli>(timeend - mTimeStart).count() > mDuration);
		}
	};

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

	template<class _Ty, std::enable_if_t<std::is_pointer_v<_Ty>, int> = 0>
	class DisposalHelper {
	public:
		/// <summary>
		/// The function pointet which can destroy provided data. including destroy its pointer.
		/// </summary>
		using DestroyFunc_t = std::function<void(_Ty)>;
	private:
		std::mutex mMutex;
		std::jthread mTdDisposal;
		std::deque<_Ty> mDequeDisposal;
		DestroyFunc_t mDestroyFunc;

	public:
		DisposalHelper() :
			mMutex(),
			mTdDisposal(), mDequeDisposal(),
			mDestroyFunc(nullptr)
		{}
		DisposalHelper(const DisposalHelper& rhs) = delete;
		DisposalHelper(DisposalHelper&& rhs) = delete;
		~DisposalHelper() {}

		/*
		We only make sure Move(...) is atomic.
		Because this is a helper class. Caller must make sure Start(), Stop(), Move() can not be called at the same time.
		*/

		void Start(DestroyFunc_t pfDestroy) {
			if (pfDestroy == nullptr) throw std::logic_error("DestroyFunc_t should not be nullptr!");
			mDestroyFunc = pfDestroy;
			mTdDisposal = std::jthread(std::bind(&DisposalHelper::DisposalWorker, this, std::placeholders::_1));
		}
		void Stop() {
			if (this->mTdDisposal.joinable()) {
				this->mTdDisposal.request_stop();
				this->mTdDisposal.join();
			}
		}
		void Move(_Ty v) {
			std::lock_guard locker(mMutex);
			mDequeDisposal.emplace_back(v);
		}
		void Move(std::deque<_Ty>& v) {
			std::lock_guard locker(mMutex);
			CommonOpers::MoveDeque(v, mDequeDisposal);
		}
	private:
		void DisposalWorker(std::stop_token st) {
			std::deque<_Ty> cache;
			while (true) {
				// copy first
				{
					std::lock_guard<std::mutex> locker(mMutex);
					CommonOpers::MoveDeque(mDequeDisposal, cache);
				}

				// no item
				if (cache.empty()) {
					// quit if ordered.
					if (st.stop_requested()) return;
					// otherwise sleep
					std::this_thread::sleep_for(DISPOSAL_INTERVAL);
					continue;
				}

				// stop them one by one until all of them stopped.
				// then return index and free them.
				for (auto& ptr : cache) {
					mDestroyFunc(ptr);
				}
				cache.clear();
			}
		}

	};

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
		std::mutex mMutex;

		void PrintTimestamp();
		void PrintComponent(Component comp, IndexDistributor::Index_t index);
		void PrintMessage(const char* fmt, va_list ap);
		void NukeProcess(int rc);
		int64_t GetSysTimeMicros();
	};

}
