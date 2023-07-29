#pragma once
#include <cinttypes>
#include <mutex>
#include <chrono>

namespace WhispersAbyss {

	constexpr const size_t NUKE_CAPACITY = 3072u;
	constexpr const size_t WARNING_CAPACITY = 2048u;
	constexpr const size_t STEAM_MSG_CAPACITY = 2048u;
	constexpr const std::chrono::milliseconds SPIN_INTERVAL(10);

	enum class ModuleStates : uint8_t {
		None = 0,
		/// <summary>
		/// Module has been created successfully.
		/// </summary>
		Ready = 0b1,
		/// <summary>
		/// Module has been started and is running now.
		/// </summary>
		Running = 0b10,
		/// <summary>
		/// Module has stopped and can be disposed safely.
		/// </summary>
		Stopped = 0b100
	};

	class ModuleStateMachine {
		friend class StateMachineTransition;
	public:
		ModuleStateMachine(ModuleStates init_state);
		~ModuleStateMachine();

		/// <summary>
		/// <para>Check whether state machine is in expected state.</para>
		/// <para>This function always return false if machine in transition.</para>
		/// </summary>
		/// <param name="expected_state">your expected state. use OR to combine more state.</param>
		/// <returns></returns>
		bool IsInState(ModuleStates expected_state);
	protected:
		/// <summary>
		/// Start the transition
		/// </summary>
		/// <param name="state_condition">your expected state. use OR to combine more state.</param>
		/// <returns>if current state fufill your expected. return true. otherwise return false and transition will not start.</returns>
		bool BeginTransition(ModuleStates state_condition);
		/// <summary>
		/// Stop the transition
		/// </summary>
		/// <param name="next_state">the next state to be switched.</param>
		void EndTransition(ModuleStates next_state);
	private:
		ModuleStates mState;
		bool mIsInTransition;
		std::mutex mStateMutex;

		/// <summary>
		/// test whether provided state is single state. not the combination of states.
		/// </summary>
		/// <param name="state">tested state</param>
		/// <returns></returns>
		bool IsSingleState(ModuleStates state);
		/// <summary>
		/// <para>Test whether your provided is matched with current state.</para>
		/// <para>You must ensure lock mStateMutex before calling this function</para>
		/// </summary>
		/// <param name="state"></param>
		/// <returns></returns>
		bool TestState(ModuleStates state);
	};

	class StateMachineTransition
	{
	public:
		StateMachineTransition(ModuleStateMachine& sm, ModuleStates requirement);
		~StateMachineTransition();

		/// <summary>
		/// Test whether need do this transition, according to whether this state machine fullfill your requirement.
		/// </summary>
		/// <returns></returns>
		bool NeedTransition();
		/// <summary>
		/// Indicate which state will be next. This method must be called at least once during the life time.
		/// </summary>
		/// <param name="state"></param>
		void To(ModuleStates state);
	private:
		ModuleStateMachine* mMachine;
		bool mNeedTransition, mNextStateSet;
		ModuleStates mNextState;
	};


}
