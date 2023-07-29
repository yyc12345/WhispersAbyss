#include "../include/others_helper.hpp"
#include <stdexcept>
#include <thread>
#include <type_traits>

namespace WhispersAbyss {

#pragma region IndexDistributor

	IndexDistributor::IndexDistributor() :
		mLock(), mCurrentIndex(0), mReturnedIndex() {}
	IndexDistributor::~IndexDistributor() {}

	IndexDistributor::Index_t IndexDistributor::Get() {
		std::lock_guard<std::mutex> locker(mLock);
		if (mReturnedIndex.empty()) return mCurrentIndex++;
		else {
			IndexDistributor::Index_t v = mReturnedIndex.front();
			mReturnedIndex.pop_front();
			return v;
		}
	}

	void IndexDistributor::Return(IndexDistributor::Index_t v) {
		std::lock_guard<std::mutex> locker(mLock);
		mReturnedIndex.push_back(v);
	}

#pragma endregion

#pragma region ModuleStateMachine

	ModuleStateMachine::ModuleStateMachine(ModuleStates::ModuleStates_t init_state) :
		mState(init_state), mIsInTransition(false), mStateMutex() {
	}
	ModuleStateMachine::~ModuleStateMachine() {}

	bool ModuleStateMachine::IsInState(ModuleStates::ModuleStates_t expected_state) {
		std::lock_guard<std::mutex> locker(mStateMutex);
		if (mIsInTransition) return false;
		return TestState(expected_state);
	}

	void ModuleStateMachine::SpinUntil(ModuleStates::ModuleStates_t expected_state) {
		while (true) {
			mStateMutex.lock();

			if (mIsInTransition || !TestState(expected_state)) {
				mStateMutex.unlock();
				std::this_thread::sleep_for(SPIN_INTERVAL);
			} else {
				mStateMutex.unlock();
				return;
			}
		}
	}

	bool ModuleStateMachine::BeginTransition(ModuleStates::ModuleStates_t state_condition) {
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

		/*while (true) {
			mStateMutex.lock();

			if (!TestState(state_condition)) {
				mStateMutex.unlock();
				return false;
			}

			if (mIsInTransition) {
				mStateMutex.unlock();
				std::this_thread::sleep_for(SPIN_INTERVAL);
				continue;
			}

			mIsInTransition = true;
			mStateMutex.unlock();
			return true;
		}*/

		std::lock_guard locker(mStateMutex);
		if (mIsInTransition || !TestState(state_condition)) return false;

		mIsInTransition = true;
		return true;

	}

	void ModuleStateMachine::EndTransition(ModuleStates::ModuleStates_t next_state) {
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

	bool ModuleStateMachine::IsSingleState(ModuleStates::ModuleStates_t state) {
		switch (state) {
			case ModuleStates::Ready:
			case ModuleStates::Running:
			case ModuleStates::Stopped:
				return true;
			default:
				return false;
		}
	}

	bool ModuleStateMachine::TestState(ModuleStates::ModuleStates_t state) {
		return mState & state;
	}

#pragma endregion

#pragma region StateMachineTransition

	StateMachineTransition::StateMachineTransition(ModuleStateMachine& sm, ModuleStates::ModuleStates_t requirement) :
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
	void StateMachineTransition::To(ModuleStates::ModuleStates_t state) {
		if (!mMachine->IsSingleState(state)) throw std::logic_error("combined states is invalid for To()");
		mNextStateSet = true;
		mNextState = state;
	}

#pragma endregion

#pragma region ModuleStateReporter

	ModuleStateReporter::ModuleStateReporter(ModuleStateMachine& sm) : 
	mMachine(&sm) {}
	ModuleStateReporter::~ModuleStateReporter() {}

	bool ModuleStateReporter::IsInState(ModuleStates::ModuleStates_t expected_state) { return mMachine->IsInState(expected_state); }
	void ModuleStateReporter::SpinUntil(ModuleStates::ModuleStates_t expected_state) { mMachine->SpinUntil(expected_state); }

#pragma endregion

}
