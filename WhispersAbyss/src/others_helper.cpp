#include "../include/others_helper.hpp"
#include <stdexcept>
#include <thread>
#include <type_traits>
#include <algorithm>

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
		mState(init_state), mIsInTransition(false), mTransitionCount(0u), mSWCount(0u), mStateMutex() {
	}
	ModuleStateMachine::~ModuleStateMachine() {}

	void ModuleStateMachine::SpinUntil(ModuleStates::ModuleStates_t state) {
		while (true) {
			mStateMutex.lock();

			// 不在状态转换时，没有排队的状态转换，且符合状态，才返回
			if (!mIsInTransition && mTransitionCount == 0u && TestState(state)) {
				mStateMutex.unlock();
				return;
			} else {
				mStateMutex.unlock();
				std::this_thread::sleep_for(SPIN_INTERVAL);
			}
		}
	}

	bool ModuleStateMachine::BeginStateBasedWork(ModuleStates::ModuleStates_t state) {
		std::lock_guard locker(mStateMutex);

		// 不在状态转换，没有排队的状态转换，状态符合，允许开始
		if (!mIsInTransition && mTransitionCount == 0u && TestState(state)) {
			++mSWCount;
			return true;
		} else {
			return false;
		}
	}

	void ModuleStateMachine::EndStateBasedWork() {
		std::lock_guard locker(mStateMutex);
		// 自减，不要减到0以下。
		if (mSWCount != 0u) {
			--mSWCount;
		}
	}

	bool ModuleStateMachine::BeginTransition(ModuleStates::ModuleStates_t state) {
		// 预先检测
		{
			std::lock_guard locker(mStateMutex);
			// 状态不匹配，返回
			if (!TestState(state)) return false;

			if (!mIsInTransition && mSWCount == 0) {
				// 不在状态转换，状态符合，且没有基于状态的操作，允许开始
				mIsInTransition = true;
				return true;
			} else {
				// 排队等待
				++mTransitionCount;
			}
		}

		// 排队等待
		while (true) {
			std::this_thread::sleep_for(SPIN_INTERVAL);

			{
				std::lock_guard locker(mStateMutex);

				// 状态不匹配，返回
				if (!TestState(state)) return false;

				// 不在状态转换，状态符合，且没有基于状态的操作，允许开始
				if (!mIsInTransition && mSWCount == 0) {
					// 自减排队，不要减到0以下。
					if (mTransitionCount != 0u) {
						--mTransitionCount;
					}
					// 开始运行
					mIsInTransition = true;
					return true;
				}
				// 否则继续自旋
			}
		}
	}

	void ModuleStateMachine::EndTransition(ModuleStates::ModuleStates_t next_state) {
		std::lock_guard<std::mutex> locker(mStateMutex);

		// 检查是否在状态转换中，以及下个状态是否是单状态
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

	StateMachineTransition::StateMachineTransition(ModuleStateMachine& sm, ModuleStates::ModuleStates_t state) :
		mMachine(&sm), mCanTransition(false), mNextStateSet(false), mNextState(ModuleStates::None) {
		mCanTransition = mMachine->BeginTransition(state);
	}
	StateMachineTransition::~StateMachineTransition() {
		if (!mCanTransition) return;

		if (!mNextStateSet) mMachine->EndTransition(mMachine->mState);	// keep its original state
		else mMachine->EndTransition(mNextState);
	}
	bool StateMachineTransition::CanTransition() {
		return mCanTransition;
	}
	void StateMachineTransition::To(ModuleStates::ModuleStates_t state) {
		if (!mMachine->IsSingleState(state)) throw std::logic_error("combined states is invalid for To()");
		mNextStateSet = true;
		mNextState = state;
	}

#pragma endregion

#pragma region StateMachineStateBasedWork

	StateMachineStateBasedWork::StateMachineStateBasedWork(ModuleStateMachine& sm, ModuleStates::ModuleStates_t state) :
		mMachine(&sm), mCanWork(false) {
		mCanWork = mMachine->BeginStateBasedWork(state);
	}
	StateMachineStateBasedWork::StateMachineStateBasedWork(ModuleStateReporter& reporter, ModuleStates::ModuleStates_t state) :
		mMachine(reporter.mMachine), mCanWork(false) {
		mCanWork = mMachine->BeginStateBasedWork(state);
	}
	StateMachineStateBasedWork::~StateMachineStateBasedWork() {
		if (!mCanWork) return;
		mMachine->EndStateBasedWork();
	}
	bool StateMachineStateBasedWork::CanWork() {
		return mCanWork;
	}

#pragma endregion


#pragma region ModuleStateReporter

	ModuleStateReporter::ModuleStateReporter(ModuleStateMachine& sm) :
		mMachine(&sm) {}
	ModuleStateReporter::~ModuleStateReporter() {}

	void ModuleStateReporter::SpinUntil(ModuleStates::ModuleStates_t expected_state) { mMachine->SpinUntil(expected_state); }

#pragma endregion

}
