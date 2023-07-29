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

			// ����״̬ת��ʱ��û���Ŷӵ�״̬ת�����ҷ���״̬���ŷ���
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

		// ����״̬ת����û���Ŷӵ�״̬ת����״̬���ϣ�����ʼ
		if (!mIsInTransition && mTransitionCount == 0u && TestState(state)) {
			++mSWCount;
			return true;
		} else {
			return false;
		}
	}

	void ModuleStateMachine::EndStateBasedWork() {
		std::lock_guard locker(mStateMutex);
		// �Լ�����Ҫ����0���¡�
		if (mSWCount != 0u) {
			--mSWCount;
		}
	}

	bool ModuleStateMachine::BeginTransition(ModuleStates::ModuleStates_t state) {
		// Ԥ�ȼ��
		{
			std::lock_guard locker(mStateMutex);
			// ״̬��ƥ�䣬����
			if (!TestState(state)) return false;

			if (!mIsInTransition && mSWCount == 0) {
				// ����״̬ת����״̬���ϣ���û�л���״̬�Ĳ���������ʼ
				mIsInTransition = true;
				return true;
			} else {
				// �Ŷӵȴ�
				++mTransitionCount;
			}
		}

		// �Ŷӵȴ�
		while (true) {
			std::this_thread::sleep_for(SPIN_INTERVAL);

			{
				std::lock_guard locker(mStateMutex);

				// ״̬��ƥ�䣬����
				if (!TestState(state)) return false;

				// ����״̬ת����״̬���ϣ���û�л���״̬�Ĳ���������ʼ
				if (!mIsInTransition && mSWCount == 0) {
					// �Լ��Ŷӣ���Ҫ����0���¡�
					if (mTransitionCount != 0u) {
						--mTransitionCount;
					}
					// ��ʼ����
					mIsInTransition = true;
					return true;
				}
				// �����������
			}
		}
	}

	void ModuleStateMachine::EndTransition(ModuleStates::ModuleStates_t next_state) {
		std::lock_guard<std::mutex> locker(mStateMutex);

		// ����Ƿ���״̬ת���У��Լ��¸�״̬�Ƿ��ǵ�״̬
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
