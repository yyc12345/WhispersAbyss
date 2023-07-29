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

		template<class T, std::enable_if_t<std::is_pointer_v<T>, int> = 0>
		void MoveDeque(std::deque<T>& _from, std::deque<T>& _to) {
			// https://stackoverflow.com/questions/49928501/better-way-to-move-objects-from-one-stddeque-to-another
			_to.insert(_to.begin(),
				std::make_move_iterator(_from.begin()),
				std::make_move_iterator(_from.end())
			);

			_from.erase(_from.begin(), _from.end());
		}

		template<class T, std::enable_if_t<std::is_pointer_v<T>, int> = 0>
		void FreeDeque(std::deque<T>& _data) {
			for (auto& ptr : _data) {
				delete ptr;
			}

			_data.erase(_data.begin(), _data.end());
		}

	}

	class IndexDistributor {
	public:
		using Index_t = uint64_t;
		IndexDistributor();
		~IndexDistributor();

		Index_t Get();
		void Return(Index_t v);
	private:
		std::mutex mLock;
		Index_t mCurrentIndex;
		std::deque<Index_t> mReturnedIndex;
	};

	namespace ModuleStates {
		using ModuleStates_t = uint8_t;
		constexpr const ModuleStates_t None = 0;
		constexpr const ModuleStates_t Ready = 0b1;
		constexpr const ModuleStates_t Running = 0b10;
		constexpr const ModuleStates_t Stopped = 0b100;
	}

	/// <summary>
	/// <para>模块核心状态机</para>
	/// <para>可以有多个基于状态的操作同时运行。状态转换时或有状态转换排队时，不允许基于状态的操作</para>
	/// <para>同一时刻最多只有一个状态转换操作运行。多个状态转换操作将排队运行。</para>
	/// </summary>
	class ModuleStateMachine {
		friend class StateMachineTransition;
		friend class StateMachineStateBasedWork;
	public:
		ModuleStateMachine(ModuleStates::ModuleStates_t init_state);
		~ModuleStateMachine();

		/// <summary>
		/// 自旋等待直至状态机满足指定状态
		/// </summary>
		/// <param name="expected_state">指定的状态。使用OR可结合多个状态。</param>
		void SpinUntil(ModuleStates::ModuleStates_t expected_state);
	private:
		/// <summary>
		/// 开始基于状态的操作。
		/// </summary>
		/// <param name="state">指定的状态。使用OR可结合多个状态。</param>
		/// <returns></returns>
		bool BeginStateBasedWork(ModuleStates::ModuleStates_t state);
		/// <summary>
		/// 结束基于状态的操作
		/// </summary>
		void EndStateBasedWork();
		/// <summary>
		/// 开始状态转换
		/// </summary>
		/// <param name="state_condition">指定的状态。使用OR可结合多个状态。</param>
		/// <returns>
		/// <para>如果当前状态满足指定状态，返回true。否则返回false。</para>
		/// <para>如果任何状态转换正在运行，或存在基于状态的工作，自旋直到其结束。</para>
		/// <para>如果返回值为true，那么准许开始状态转换。否则不允许。</para>
		/// </returns>
		bool BeginTransition(ModuleStates::ModuleStates_t state_condition);
		/// <summary>
		/// 结束状态转换
		/// </summary>
		/// <param name="next_state">指示状态机要转换到的状态。</param>
		void EndTransition(ModuleStates::ModuleStates_t next_state);

		/// <summary>
		/// 评估传入状态是否是单个状态
		/// </summary>
		/// <param name="state">待评估的状态。</param>
		/// <returns></returns>
		bool IsSingleState(ModuleStates::ModuleStates_t state);
		/// <summary>
		/// <para>判断提供的状态是否符合当前状态。</para>
		/// <para>你必须在调用此函数前前使用mStateMutex互斥锁锁定。</para>
		/// </summary>
		/// <param name="state">待测试的状态</param>
		/// <returns></returns>
		bool TestState(ModuleStates::ModuleStates_t state);
	private:
		/// <summary>
		/// 当前状态
		/// </summary>
		ModuleStates::ModuleStates_t mState;
		/// <summary>
		/// <para>是否在状态转换</para>
		/// <para>在状态转换时，拒绝基于状态的操作。</para>
		/// </summary>
		bool mIsInTransition;
		/// <summary>
		/// <para>状态转换个数</para>
		/// <para>状态转换个数为0时表示没有状态转换正在运行。状态转换为N时，则其中1个状态转换正在运行，其余N-1个转换正在等待允许运行。</para>
		/// <para>状态转换个数不为0时，拒绝基于状态的操作。</para>
		/// </summary>
		size_t mTransitionCount;
		/// <summary>
		/// <para>基于状态的操作的数量。</para>
		/// <para>基于状态的操作不为0时，拒绝状态操作请求，但可以允许多个基于状态的操作运行。</para>
		/// </summary>
		size_t mSWCount;
		/// <summary>
		/// 核心互斥锁
		/// </summary>
		std::mutex mStateMutex;

	};
	/// <summary>
	/// 模块核心状态机对外暴露用汇报器。
	/// </summary>
	class ModuleStateReporter {
		friend class StateMachineStateBasedWork;
	public:
		ModuleStateReporter(ModuleStateMachine& sm);
		~ModuleStateReporter();

		void SpinUntil(ModuleStates::ModuleStates_t expected_state);
	private:
		ModuleStateMachine* mMachine;
	};
	/// <summary>
	/// <para>基于RAII的状态机 状态转换 辅助类。</para>
	/// <para>资源申请时，尝试开始转换。资源释放时，终止转换</para>
	/// </summary>
	class StateMachineTransition
	{
	public:
		StateMachineTransition(ModuleStateMachine& sm, ModuleStates::ModuleStates_t state);
		~StateMachineTransition();

		/// <summary>
		/// 检查是否可以进行状态转换。
		/// </summary>
		/// <returns></returns>
		bool CanTransition();
		/// <summary>
		/// <para>指示下一个将要转换的状态。</para>
		/// <para>如果生命周期内没有一次调用，则状态不改变。</para>
		/// <para>如果生命周期内调用多次，以最后一次指定的状态为准</para>
		/// </summary>
		/// <param name="state">下一个状态</param>
		void To(ModuleStates::ModuleStates_t state);
	private:
		ModuleStateMachine* mMachine;
		bool mCanTransition, mNextStateSet;
		ModuleStates::ModuleStates_t mNextState;
	};
	/// <summary>
	/// <para>基于RAII的状态机 基于状态操作 辅助类。</para>
	/// <para>资源申请时，尝试开始运行。资源释放时，终止运行</para>
	/// </summary>
	class StateMachineStateBasedWork
	{
	public:
		StateMachineStateBasedWork(ModuleStateMachine& sm, ModuleStates::ModuleStates_t state);
		StateMachineStateBasedWork(ModuleStateReporter& reporter, ModuleStates::ModuleStates_t state);
		~StateMachineStateBasedWork();

		/// <summary>
		/// 检查是否可以进行基于状态操作。
		/// </summary>
		/// <returns></returns>
		bool CanWork();
	private:
		ModuleStateMachine* mMachine;
		bool mCanWork;
	};

}
