#include "others_helper.hpp"
#include <cstdio>
#include <cstdarg>

// for got ms-based time
#ifdef _WIN32
#include <Windows.h>
#else
#include <time.h>
#endif // _WIND32

namespace WhispersAbyss {

	const char* CommonOpers::State2String(StateMachine::State_t state) {
		switch (state) {
			case StateMachine::Ready: return "Ready";
			case StateMachine::Running: return "Running";
			case StateMachine::Stopped: return "Stopped";
			default: return "Unknow";
		}
	}

	void CommonOpers::AppendStrF(std::string& strl, const char* fmt, ...) {
		va_list args1;
		va_start(args1, fmt);
		va_list args2;
		va_start(args2, fmt);

		size_t oldsize = strl.size();
		int count = std::vsnprintf(nullptr, 0, fmt, args1);
		if (count < 0) throw new std::length_error("Invalid length returned by vsnprintf.");
		va_end(args1);

		// count is required size
		// add 1 for count in calling to ensure \0 is written in illegal area.
		strl.resize(oldsize + count);
		int write_result = std::vsnprintf(strl.data() + oldsize, count + 1, fmt, args2);
		va_end(args2);

		if (write_result < 0 || write_result > count) throw new std::length_error("Invalid write result in vsnprintf.");
	}

#pragma region OutputHelper

	OutputHelper::OutputHelper() {
		g_logTimeZero = GetSysTimeMicros();
	}

	OutputHelper::~OutputHelper() {}

#define CALL_FMT va_list ap; \
va_start(ap, fmt); \
PrintMessage(fmt, ap); \
va_end(ap);

	void OutputHelper::FatalError(const char* fmt, ...) {
		{
			std::lock_guard locker(mMutex);
			PrintTimestamp();
			CALL_FMT;
		}
		NukeProcess(1);
	}
	void OutputHelper::FatalError(Component comp, IndexDistributor::Index_t index, const char* fmt, ...) {
		{
			std::lock_guard locker(mMutex);
			PrintTimestamp();
			PrintComponent(comp, index);
			CALL_FMT;
		}
		NukeProcess(1);
	}
	void OutputHelper::Printf(const char* fmt, ...) {
		{
			std::lock_guard locker(mMutex);
			PrintTimestamp();
			CALL_FMT;
		}
	}
	void OutputHelper::Printf(Component comp, IndexDistributor::Index_t index, const char* fmt, ...) {
		{
			std::lock_guard locker(mMutex);
			PrintTimestamp();
			PrintComponent(comp, index);
			CALL_FMT;
		}
	}
	void OutputHelper::RawPrintf(const char* fmt, ...) {
		{
			std::lock_guard locker(mMutex);
			CALL_FMT;
		}
	}

#undef CALL_FMT

	void OutputHelper::PrintTimestamp() {
		float time = (float)(GetSysTimeMicros() - g_logTimeZero);
		fprintf(stdout, "%10.6f ", time * 1e-6);
	}

	void OutputHelper::PrintComponent(Component comp, IndexDistributor::Index_t index) {
		switch (comp) {
			case Component::TcpInstance:
				fprintf(stdout, "[Tcp-#%" PRIu64 "] ", index);
				break;
			case Component::TcpFactory:
				fputs("[Tcp Factory] ", stdout);
				break;
			case Component::GnsInstance:
				fprintf(stdout, "[Gns-#%" PRIu64 "] ", index);
				break;
			case Component::GnsFactory:
				fputs("[Gns Factory] ", stdout);
				break;
			case Component::BridgeInstance:
				fprintf(stdout, "[Bridge-#%" PRIu64 "] ", index);
				break;
			case Component::BridgeFactory:
				fputs("[Bridge Factory] ", stdout);
				break;
			case Component::Core:
				fputs("[Core] ", stdout);
				break;
		}
	}

	void OutputHelper::PrintMessage(const char* fmt, va_list ap) {
		vfprintf(stdout, fmt, ap);
		fputc('\n', stdout);
		fflush(stdout);
	}

	void OutputHelper::NukeProcess(int rc) {
#ifdef WIN32
		ExitProcess(rc);
#else
		(void)rc; // Unused formal parameter
		kill(getpid(), SIGKILL);
#endif
	}

	int64_t OutputHelper::GetSysTimeMicros() {
#ifdef _WIN32
		// from 1/1/1601. to 1/1/1970 time(unit 100ns)
#define EPOCHFILETIME (116444736000000000UL)
		FILETIME ft;
		LARGE_INTEGER li;
		int64_t tt = 0;
		GetSystemTimeAsFileTime(&ft);
		li.LowPart = ft.dwLowDateTime;
		li.HighPart = ft.dwHighDateTime;
		// get utc ms-based time
		tt = (li.QuadPart - EPOCHFILETIME) / 10;
		return tt;
#else
		timeval tv;
		gettimeofday(&tv, 0);
		return (int64_t)tv.tv_sec * 1000000 + (int64_t)tv.tv_usec;
#endif // WIN32

		return 0;
	}

#pragma endregion

}
