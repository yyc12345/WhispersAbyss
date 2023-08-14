#pragma once
#include "others_helper.hpp"
#include <cinttypes>
#include <mutex>
#include <chrono>
#include <deque>

namespace WhispersAbyss::StateMachine {

	/*
	Vulnerable bug:
	Using StateMachine may easily cause use after free issue.
	Usually happend in transition allocation, although I have invent a ref counter for state machine.
	The suppress solution is do not calling like this:
	```
	Stop();
	mStatusReporter.SpinUntil(StateMachine::Stopped);
	```
	Use this calling style instread:
	```
	if (!mStatusReporter.IsInState(StateMachine::Stopped)) Stop();
	mStatusReporter.SpinUntil(StateMachine::Stopped);
	```

	This change can suppress this problem but can't solve it.
	But I really don't want to slove this issue and 
	actually I do not have time to solve this issue.

	NOTE:
	This style code used in every Stop() of Instance and Factory.
	Also used in Factory Disposal lambda.
	*/

	/*
	There are 3 possible state for StateMachineCore.
	Ready, Running, Stopped.

	There are 2 possible transition for StateMachineCore.
	Initializing: Ready -> (Running or Stopped).
	Stopping: (Ready or Running) -> Stopped.

	All transitions will wait previous transition (if existed) finished, then check its requirement.
	All transitions will only run once in the same state machine.

	In Running state, user can raise multiple work, called Running based Work.
	Running state will block transition.

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
		StateMachineCore(State_t init_state) :
			mState(init_state), mIsInTransition(false),
			mHasRunInitializing(false), mHasRunStopping(false),
			mRefCounter(0u), mStateMutex() {}
		~StateMachineCore() {
			// Self spin until all reference has been released.
			while (true) {
				mStateMutex.lock();
				if (mRefCounter != 0u) {
					mStateMutex.unlock();
					std::this_thread::sleep_for(SPIN_INTERVAL);
				} else {
					mStateMutex.unlock();
					break;
				}
			}
		}
		StateMachineCore(const StateMachineCore& rhs) = delete;
		StateMachineCore(StateMachineCore&& rhs) = delete;

	private:
		/// <summary>
		/// Current State
		/// </summary>
		State_t mState;
		/// <summary>
		/// <para>Is in transition</para>
		/// <para>If state machine is in transition, reject other transition request, reject Running based work.</para>
		/// </summary>
		bool mIsInTransition;

		/// <summary>
		/// True if a Initializing transition has been run.
		/// </summary>
		bool mHasRunInitializing;
		/// <summary>
		/// True if a Stopping transition has been run.
		/// </summary>
		bool mHasRunStopping;

		/// <summary>
		/// <para>The reference counter.</para>
		/// <para>Once a helper class created, increase this by 1. When free them, decrease this by 1.</para>
		/// </summary>
		uint32_t mRefCounter;

		/// <summary>
		/// Core mutex
		/// </summary>
		std::mutex mStateMutex;

		void IncRefCounter() { ++mRefCounter; }
		void DecRefCounter() { if (mRefCounter != 0u) --mRefCounter; }
	};
	class StateMachineReporter {
	public:
		StateMachineReporter(StateMachineCore& sm) :
			mStateMachine(&sm) {
			std::lock_guard locker(mStateMachine->mStateMutex);
			mStateMachine->IncRefCounter();
		}
		~StateMachineReporter() {
			std::lock_guard locker(mStateMachine->mStateMutex);
			mStateMachine->DecRefCounter();
		}
		StateMachineReporter(const StateMachineReporter& rhs) = delete;
		StateMachineReporter(StateMachineReporter&& rhs) = delete;

		/// <summary>
		/// <para>Check whether this state machine is matched.</para>
		/// </summary>
		/// <returns></returns>
		bool IsInState(State_t state) {
			std::lock_guard locker(mStateMachine->mStateMutex);
			return mStateMachine->mState == state && !mStateMachine->mIsInTransition;
		}
		/// <summary>
		/// Spin self until the state matched.
		/// </summary>
		/// <returns></returns>
		void SpinUntil(State_t state) {
			while (true) {
				// do not use lock guard 
				// because we want thread sleep will not occupy the mutex.
				mStateMachine->mStateMutex.lock();

				if (mStateMachine->mState == state && !mStateMachine->mIsInTransition) {
					mStateMachine->mStateMutex.unlock();
					return;
				} else {
					mStateMachine->mStateMutex.unlock();
					std::this_thread::sleep_for(SPIN_INTERVAL);
				}
			}
		}
		/// <summary>
		/// <para>Roughly get state machine status.</para>
		/// <para>This status should only be used for display. Should not be used as requirement checker.</para>
		/// </summary>
		/// <param name="state"></param>
		/// <param name="is_in_transition"></param>
		void GetStatus(State_t& state, bool& is_in_transition) {
			std::lock_guard locker(mStateMachine->mStateMutex);
			state = mStateMachine->mState;
			is_in_transition = mStateMachine->mIsInTransition;
		}
	private:
		StateMachineCore* mStateMachine;
	};
	class TransitionInitializing {
	public:
		TransitionInitializing(StateMachineCore& sm) :
			mStateMachine(&sm), mCanTransition(false), mHasProblem(false) {
			while (true) {
				std::lock_guard locker(mStateMachine->mStateMutex);
				mStateMachine->IncRefCounter();
				break;
			}

			// loop until no transition running
			while (true) {
				mStateMachine->mStateMutex.lock();

				if (mStateMachine->mIsInTransition) {
					mStateMachine->mStateMutex.unlock();
					std::this_thread::sleep_for(SPIN_INTERVAL);
					continue;
				}

				if (mStateMachine->mState == Ready && !mStateMachine->mHasRunInitializing) {
					mCanTransition = true;
					mStateMachine->mHasRunInitializing = true;
					mStateMachine->mIsInTransition = true;
				} else {
					mCanTransition = false;
				}
				mStateMachine->mStateMutex.unlock();
				break;
			}

		}
		~TransitionInitializing() {
			std::lock_guard locker(mStateMachine->mStateMutex);
			mStateMachine->DecRefCounter();

			if (mCanTransition) {
				mStateMachine->mState = mHasProblem ? Stopped : Running;
				mStateMachine->mIsInTransition = false;
			}
		}
		TransitionInitializing(const TransitionInitializing& rhs) = delete;
		TransitionInitializing(TransitionInitializing&& rhs) = delete;

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
			while (true) {
				std::lock_guard locker(mStateMachine->mStateMutex);
				mStateMachine->IncRefCounter();
				break;
			}

			while (true) {
				mStateMachine->mStateMutex.lock();

				if (mStateMachine->mIsInTransition) {
					mStateMachine->mStateMutex.unlock();
					std::this_thread::sleep_for(SPIN_INTERVAL);
					continue;
				}

				if ((mStateMachine->mState == Ready || mStateMachine->mState == Running) && !mStateMachine->mHasRunStopping) {
					mCanTransition = true;
					mStateMachine->mHasRunStopping = true;
					mStateMachine->mIsInTransition = true;
				} else {
					mCanTransition = false;
				}
				mStateMachine->mStateMutex.unlock();
				break;
			}
		}
		~TransitionStopping() {
			std::lock_guard locker(mStateMachine->mStateMutex);
			mStateMachine->DecRefCounter();

			if (mCanTransition) {
				mStateMachine->mState = Stopped;
				mStateMachine->mIsInTransition = false;
			}
		}
		TransitionStopping(const TransitionStopping& rhs) = delete;
		TransitionStopping(TransitionStopping&& rhs) = delete;

		/// <summary>
		/// Check whether caller can start transition work.
		/// </summary>
		/// <returns></returns>
		bool CanTransition() { return mCanTransition; }
	private:
		StateMachineCore* mStateMachine;
		bool mCanTransition;
	};

}
