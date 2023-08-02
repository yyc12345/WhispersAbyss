#pragma once
#include <cinttypes>
#include <mutex>
#include <chrono>
#include <deque>

namespace WhispersAbyss::StateMachine {

	enum class State {
		None = 0,
		Ready = 0b1,
		Running = 0b10,
		Stopped = 0b100
	};
	constexpr const std::chrono::milliseconds SPIN_INTERVAL(10);

	/*
	There are 3 possible state for StateMachineCore.
	Ready, Running, Stopped.

	There are 2 possible transition for StateMachineCore.
	Initializing: Ready to (Running or Stopped).
	Stopping: Running to Stopped.

	In any case, there are only 1 transition is running.
	Other transition will immediately rejected.
	State not matched transition also will be rejected.

	In Running state, user can raise multiple work, called Running based Work.
	Running state will block transition.
	The first comming transition will take place first (set mIsInTransition = true) but not run immediate. It will wait Running based Work until all work done.
	So that other transition requets during these time will see mIsInTransition == true and exit immediately.

	If the state machine entering Stopped state, we say it is dead.
	A dead state machine will not happend any transition again.
	This is convenient for detect the stop of module.
	*/

	/// <summary>
	/// <para>The core of state machine. Should not be visited directly. Should not be visited outside of class.</para>
	/// <para>This class has RAII feature.</para>
	/// <para>Use StateMachineReporter to check state and do wait work.</para>
	/// <para>Use TransitionInitializing and TransitionStopping to do transition.</para>
	/// <para>Use WorkBasedOnRunning to do Running based work.</para>
	/// </summary>
	class StateMachineCore {
		friend class StateMachineReporter;
		friend class TransitionInitializing;
		friend class TransitionStopping;
		friend class WorkBasedOnRunning;
	public:
		StateMachineCore(State init_state) :
			mState(init_state), mIsInTransition(false), mWorkCount(0), mStateMutex() {}
		~StateMachineCore() {}

	private:
		/// <summary>
		/// Current State
		/// </summary>
		State mState;
		/// <summary>
		/// <para>Is in transition</para>
		/// <para>If state machine is in transition, reject other transition request, reject Running based work.</para>
		/// </summary>
		bool mIsInTransition;
		/// <summary>
		/// <para>The count of Running based work.</para>
		/// <para>If this count != 0, any transition request will take place first and wait until all work done before executing its transition.</para>
		/// <para>Allow multiple Running based work. The count is recorded in there.</para>
		/// </summary>
		uint32_t mWorkCount;
		/// <summary>
		/// Core mutex
		/// </summary>
		std::mutex mStateMutex;
	};
	class StateMachineReporter {
	public:
		StateMachineReporter(StateMachineCore& sm) :
			mStateMachine(&sm) {}
		~StateMachineReporter() {}

		/// <summary>
		/// <para>Check whether this state machine is dead (is Stopped).</para>
		/// </summary>
		/// <returns></returns>
		bool IsStopped() {
			std::lock_guard locker(mStateMachine->mStateMutex);
			return mStateMachine->mState == State::Stopped;
		}
		/// <summary>
		/// Spin self until the state is Stopped.
		/// </summary>
		/// <returns></returns>
		bool SpinUntilStopped() {
			while (true) {
				// do not use lock guard 
				// because we want thread sleep will not occupy the mutex.
				mStateMachine->mStateMutex.lock();

				if (mStateMachine->mState == State::Stopped) {
					mStateMachine->mStateMutex.unlock();
					return;
				} else {
					mStateMachine->mStateMutex.unlock();
					std::this_thread::sleep_for(SPIN_INTERVAL);
				}
			}
		}
	private:
		StateMachineCore* mStateMachine;
	};
	class TransitionInitializing {
	public:
		TransitionInitializing(StateMachineCore& sm) :
			mStateMachine(&sm), mCanTransition(false), mHasProblem(false) {
			std::lock_guard locker(mStateMachine->mStateMutex);

			// if state matched, and no transition running, enter transition.
			if (mStateMachine->mState == State::Ready && !mStateMachine->mIsInTransition) {
				mCanTransition = true;
				mStateMachine->mIsInTransition = true;
			} else {
				mCanTransition = false;
			}
		}
		~TransitionInitializing() {
			if (mCanTransition) {
				std::lock_guard locker(mStateMachine->mStateMutex);
				mStateMachine->mState = mHasProblem ? State::Stopped : State::Running;
				mStateMachine->mIsInTransition = false;
			}
		}

		/// <summary>
		/// Check whether caller can start transition work.
		/// </summary>
		/// <returns></returns>
		bool CanTransition() { return mCanTransition; }
		/// <summary>
		/// Set transition result. The default value is no error.
		/// </summary>
		/// <param name="has_problem">Set true if error occurs.</param>
		void SetTransitionError(bool has_problem) { mHasProblem = has_problem; }
	private:
		StateMachineCore* mStateMachine;
		bool mCanTransition;
		bool mHasProblem;
	};
	class TransitionStopping
	{
	public:
		TransitionStopping(StateMachineCore& sm) :
			mStateMachine(&sm), mCanTransition(false) {

			// pre-requirement check
			while (true) {
				std::lock_guard locker(mStateMachine->mStateMutex);

				if (mStateMachine->mState == State::Running && !mStateMachine->mIsInTransition) {
					// we can do transition. set flag and take place in state machine first
					mCanTransition = true;
					mStateMachine->mIsInTransition = true;

					if (mStateMachine->mWorkCount == 0u) {
						// we can directly entering transition.
						return;
					} else {
						// there are still some Running based Work are running.
						// break the while and wait them in the next while block.
						break;
					}
				} else {
					// if state not matched or any other transition is running.
					// we definately can not run.
					mCanTransition = false;
					return;
				}
			}

			// Running based Work waiter
			while (true) {
				// sleep first
				std::this_thread::sleep_for(SPIN_INTERVAL);

				// check Running based Work again
				mStateMachine->mStateMutex.lock();
				if (mStateMachine->mWorkCount == 0u) {
					// start transition
					mStateMachine->mStateMutex.unlock();
					return;
				} else {
					// continue waiting
					mStateMachine->mStateMutex.unlock();
					continue;
				}
			}

		}
		~TransitionStopping() {
			if (mCanTransition) {
				std::lock_guard locker(mStateMachine->mStateMutex);
				mStateMachine->mState = State::Stopped;
				mStateMachine->mIsInTransition = false;
			}
		}

		/// <summary>
		/// Check whether caller can start transition work.
		/// </summary>
		/// <returns></returns>
		bool CanTransition() { return mCanTransition; }
	private:
		StateMachineCore* mStateMachine;
		bool mCanTransition;
	};
	class WorkBasedOnRunning {
	public:
		WorkBasedOnRunning(StateMachineCore& sm) :
			mStateMachine(&sm), mCanWork(false) {
			std::lock_guard locker(mStateMachine->mStateMutex);

			// if in running state, and no transition running, start work
			if (mStateMachine->mState == State::Running && !mStateMachine->mIsInTransition) {
				mCanWork = true;
				++mStateMachine->mWorkCount;
			} else {
				mCanWork = false;
			}
		}
		~WorkBasedOnRunning() {
			if (mCanWork) {
				std::lock_guard locker(mStateMachine->mStateMutex);

				// make sure no lower overflow.
				if (mStateMachine->mWorkCount != 0u) {
					--mStateMachine->mWorkCount;
				}
			}
		}

		/// <summary>
		/// Check whether caller can start Running based work.
		/// </summary>
		/// <returns></returns>
		bool CanWork() { return mCanWork; }
	private:
		StateMachineCore* mStateMachine;
		bool mCanWork;
	};

}
