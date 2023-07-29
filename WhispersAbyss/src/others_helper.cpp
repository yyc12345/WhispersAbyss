#include "../include/others_helper.hpp"
#include <stdexcept>
#include <thread>
#include <type_traits>

namespace WhispersAbyss {

	ModuleStateMachine::ModuleStateMachine(ModuleStates init_state) :
		mState(init_state), mIsInTransition(false), mStateMutex() {
	}
	ModuleStateMachine::~ModuleStateMachine() {}

	bool ModuleStateMachine::IsInState(ModuleStates expected_state) {
		std::lock_guard<std::mutex> locker(mStateMutex);
		if (mIsInTransition) return false;
		return TestState(expected_state);
	}

	bool ModuleStateMachine::BeginTransition(ModuleStates state_condition) {
		//// test if not fufill requirement. exit instantly.
		//{
		//	std::lock_guard<std::mutex> locker(mStateMutex);
		//	if (!TestState(state_condition)) {
		//		return false;
		//	}
		//}

		//// current state fufill. waiting transition lock
		//{
		//	std::lock(mTransitionMutex, mStateMutex);
		//	std::lock_guard<std::mutex> locker(mStateMutex, std::adopt_lock);
		//	// test requirement again. because the state may changed
		//	if (!TestState(state_condition)) {
		//		mTransitionMutex.unlock();
		//		return false;
		//	}

		//	// leave a locked mTransitionMutex alone
		//	mIsInTransition = true;
		//}
		//return true;
		while (true) {
			mStateMutex.lock();

			if (!TestState(state_condition)) return false;

			if (mIsInTransition) {
				mStateMutex.unlock();
				std::this_thread::sleep_for(SPIN_INTERVAL);
				continue;
			}

			mIsInTransition = true;
			mStateMutex.unlock();
			return true;
		}
	}

	void ModuleStateMachine::EndTransition(ModuleStates next_state) {
		//// test whether in transition
		//std::lock_guard<std::mutex> locker(mStateMutex);
		//if (!mIsInTransition) return;

		//// start free locker
		//std::lock_guard<std::mutex> locker(mTransitionMutex, std::adopt_lock);
		//if (!IsSingleState(next_state)) throw std::invalid_argument("invalid state. not a single state.");
		//mState = next_state;
		//mIsInTransition = false;
		std::lock_guard<std::mutex> locker(mStateMutex);
		if (!mIsInTransition) return;

		if (!IsSingleState(next_state)) throw std::invalid_argument("invalid state. not a single state.");
		mState = next_state;
		mIsInTransition = false;
	}

	bool ModuleStateMachine::IsSingleState(ModuleStates state) {
		switch (state)
		{
			case WhispersAbyss::ModuleStates::Ready:
			case WhispersAbyss::ModuleStates::Running:
			case WhispersAbyss::ModuleStates::Stopped:
				return true;
			default:
				return false;
		}
	}

	bool ModuleStateMachine::TestState(ModuleStates state) {
		return static_cast<std::underlying_type_t<ModuleStates>>(mState) & static_cast<std::underlying_type_t<ModuleStates>>(state);
	}


	StateMachineTransition::StateMachineTransition(ModuleStateMachine& sm, ModuleStates requirement) :
		mMachine(&sm), mNeedTransition(false), mNextStateSet(false), mNextState(ModuleStates::None) {
		mNeedTransition = mMachine->BeginTransition(requirement);
	}
	StateMachineTransition::~StateMachineTransition() {
		if (!mNeedTransition) return;

		if (!mNextStateSet) mMachine->EndTransition(mMachine->mState);	// keep its original state
		else mMachine->EndTransition(mNextState);
	}
	bool StateMachineTransition::NeedTransition() {
		return mNeedTransition;
	}
	void StateMachineTransition::To(ModuleStates state) {
		if (!mMachine->IsSingleState(state)) throw std::logic_error("combined states is invalid for To()");
		mNextStateSet = true;
		mNextState = state;
	}

}
