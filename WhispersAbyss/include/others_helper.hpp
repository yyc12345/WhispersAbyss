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

	class ModuleStateMachine {
		friend class StateMachineTransition;
	public:
		ModuleStateMachine(ModuleStates::ModuleStates_t init_state);
		~ModuleStateMachine();

		/// <summary>
		/// <para>Check whether state machine is in expected state.</para>
		/// <para>This function always return false if machine in transition.</para>
		/// </summary>
		/// <param name="expected_state">your expected state. use OR to combine more state.</param>
		/// <returns></returns>
		bool IsInState(ModuleStates::ModuleStates_t expected_state);
		/// <summary>
		/// Spin wait until current states fufill your requirement
		/// </summary>
		/// <param name="expected_state">your expected state. use OR to combine more state.</param>
		void SpinUntil(ModuleStates::ModuleStates_t expected_state);
	protected:
		/// <summary>
		/// Start the transition.
		/// </summary>
		/// <param name="state_condition">your expected state. use OR to combine more state.</param>
		/// <returns>
		/// <para>if current state fufill your expected. return true. otherwise return false and transition will not start.</para>
		/// <para>If a transition is running, return false.</para>
		/// <para>If returned value is true, a transition will start, otherwise nothing will happend.</para>
		/// </returns>
		bool BeginTransition(ModuleStates::ModuleStates_t state_condition);
		/// <summary>
		/// Stop the transition
		/// </summary>
		/// <param name="next_state">the next state to be switched.</param>
		void EndTransition(ModuleStates::ModuleStates_t next_state);
	private:
		ModuleStates::ModuleStates_t mState;
		bool mIsInTransition;
		std::mutex mStateMutex;

		/// <summary>
		/// test whether provided state is single state. not the combination of states.
		/// </summary>
		/// <param name="state">tested state</param>
		/// <returns></returns>
		bool IsSingleState(ModuleStates::ModuleStates_t state);
		/// <summary>
		/// <para>Test whether your provided is matched with current state.</para>
		/// <para>You must ensure lock mStateMutex before calling this function</para>
		/// </summary>
		/// <param name="state"></param>
		/// <returns></returns>
		bool TestState(ModuleStates::ModuleStates_t state);
	};

	class ModuleStateReporter {
	public:
		ModuleStateReporter(ModuleStateMachine& sm);
		~ModuleStateReporter();

		bool IsInState(ModuleStates::ModuleStates_t expected_state);
		void SpinUntil(ModuleStates::ModuleStates_t expected_state);
	private:
		ModuleStateMachine* mMachine;
	};

	class StateMachineTransition
	{
	public:
		StateMachineTransition(ModuleStateMachine& sm, ModuleStates::ModuleStates_t requirement);
		~StateMachineTransition();

		/// <summary>
		/// Test whether need do this transition, according to whether this state machine fullfill your requirement.
		/// </summary>
		/// <returns></returns>
		bool NeedTransition();
		/// <summary>
		/// Indicate which state will be next. If you do not call this function during the life time, the state will not changed.
		/// </summary>
		/// <param name="state"></param>
		void To(ModuleStates::ModuleStates_t state);
	private:
		ModuleStateMachine* mMachine;
		bool mNeedTransition, mNextStateSet;
		ModuleStates::ModuleStates_t mNextState;
	};


}
