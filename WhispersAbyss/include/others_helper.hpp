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
	/// <para>ģ�����״̬��</para>
	/// <para>�����ж������״̬�Ĳ���ͬʱ���С�״̬ת��ʱ����״̬ת���Ŷ�ʱ�����������״̬�Ĳ���</para>
	/// <para>ͬһʱ�����ֻ��һ��״̬ת���������С����״̬ת���������Ŷ����С�</para>
	/// </summary>
	class ModuleStateMachine {
		friend class StateMachineTransition;
		friend class StateMachineStateBasedWork;
	public:
		ModuleStateMachine(ModuleStates::ModuleStates_t init_state);
		~ModuleStateMachine();

		/// <summary>
		/// �����ȴ�ֱ��״̬������ָ��״̬
		/// </summary>
		/// <param name="expected_state">ָ����״̬��ʹ��OR�ɽ�϶��״̬��</param>
		void SpinUntil(ModuleStates::ModuleStates_t expected_state);
	private:
		/// <summary>
		/// ��ʼ����״̬�Ĳ�����
		/// </summary>
		/// <param name="state">ָ����״̬��ʹ��OR�ɽ�϶��״̬��</param>
		/// <returns></returns>
		bool BeginStateBasedWork(ModuleStates::ModuleStates_t state);
		/// <summary>
		/// ��������״̬�Ĳ���
		/// </summary>
		void EndStateBasedWork();
		/// <summary>
		/// ��ʼ״̬ת��
		/// </summary>
		/// <param name="state_condition">ָ����״̬��ʹ��OR�ɽ�϶��״̬��</param>
		/// <returns>
		/// <para>�����ǰ״̬����ָ��״̬������true�����򷵻�false��</para>
		/// <para>����κ�״̬ת���������У�����ڻ���״̬�Ĺ���������ֱ���������</para>
		/// <para>�������ֵΪtrue����ô׼��ʼ״̬ת������������</para>
		/// </returns>
		bool BeginTransition(ModuleStates::ModuleStates_t state_condition);
		/// <summary>
		/// ����״̬ת��
		/// </summary>
		/// <param name="next_state">ָʾ״̬��Ҫת������״̬��</param>
		void EndTransition(ModuleStates::ModuleStates_t next_state);

		/// <summary>
		/// ��������״̬�Ƿ��ǵ���״̬
		/// </summary>
		/// <param name="state">��������״̬��</param>
		/// <returns></returns>
		bool IsSingleState(ModuleStates::ModuleStates_t state);
		/// <summary>
		/// <para>�ж��ṩ��״̬�Ƿ���ϵ�ǰ״̬��</para>
		/// <para>������ڵ��ô˺���ǰǰʹ��mStateMutex������������</para>
		/// </summary>
		/// <param name="state">�����Ե�״̬</param>
		/// <returns></returns>
		bool TestState(ModuleStates::ModuleStates_t state);
	private:
		/// <summary>
		/// ��ǰ״̬
		/// </summary>
		ModuleStates::ModuleStates_t mState;
		/// <summary>
		/// <para>�Ƿ���״̬ת��</para>
		/// <para>��״̬ת��ʱ���ܾ�����״̬�Ĳ�����</para>
		/// </summary>
		bool mIsInTransition;
		/// <summary>
		/// <para>״̬ת������</para>
		/// <para>״̬ת������Ϊ0ʱ��ʾû��״̬ת���������С�״̬ת��ΪNʱ��������1��״̬ת���������У�����N-1��ת�����ڵȴ��������С�</para>
		/// <para>״̬ת��������Ϊ0ʱ���ܾ�����״̬�Ĳ�����</para>
		/// </summary>
		size_t mTransitionCount;
		/// <summary>
		/// <para>����״̬�Ĳ�����������</para>
		/// <para>����״̬�Ĳ�����Ϊ0ʱ���ܾ�״̬�������󣬵���������������״̬�Ĳ������С�</para>
		/// </summary>
		size_t mSWCount;
		/// <summary>
		/// ���Ļ�����
		/// </summary>
		std::mutex mStateMutex;

	};
	/// <summary>
	/// ģ�����״̬�����Ⱪ¶�û㱨����
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
	/// <para>����RAII��״̬�� ״̬ת�� �����ࡣ</para>
	/// <para>��Դ����ʱ�����Կ�ʼת������Դ�ͷ�ʱ����ֹת��</para>
	/// </summary>
	class StateMachineTransition
	{
	public:
		StateMachineTransition(ModuleStateMachine& sm, ModuleStates::ModuleStates_t state);
		~StateMachineTransition();

		/// <summary>
		/// ����Ƿ���Խ���״̬ת����
		/// </summary>
		/// <returns></returns>
		bool CanTransition();
		/// <summary>
		/// <para>ָʾ��һ����Ҫת����״̬��</para>
		/// <para>�������������û��һ�ε��ã���״̬���ı䡣</para>
		/// <para>������������ڵ��ö�Σ������һ��ָ����״̬Ϊ׼</para>
		/// </summary>
		/// <param name="state">��һ��״̬</param>
		void To(ModuleStates::ModuleStates_t state);
	private:
		ModuleStateMachine* mMachine;
		bool mCanTransition, mNextStateSet;
		ModuleStates::ModuleStates_t mNextState;
	};
	/// <summary>
	/// <para>����RAII��״̬�� ����״̬���� �����ࡣ</para>
	/// <para>��Դ����ʱ�����Կ�ʼ���С���Դ�ͷ�ʱ����ֹ����</para>
	/// </summary>
	class StateMachineStateBasedWork
	{
	public:
		StateMachineStateBasedWork(ModuleStateMachine& sm, ModuleStates::ModuleStates_t state);
		StateMachineStateBasedWork(ModuleStateReporter& reporter, ModuleStates::ModuleStates_t state);
		~StateMachineStateBasedWork();

		/// <summary>
		/// ����Ƿ���Խ��л���״̬������
		/// </summary>
		/// <returns></returns>
		bool CanWork();
	private:
		ModuleStateMachine* mMachine;
		bool mCanWork;
	};

}
