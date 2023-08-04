#include "output_helper.hpp"

#include <cstdio>

// for got ms-based time
#ifdef _WIN32
#include <Windows.h>
#else
#include <time.h>
#endif // _WIND32

namespace WhispersAbyss {

	OutputHelper::OutputHelper() {
		g_logTimeZero = GetSysTimeMicros();
	}

	OutputHelper::~OutputHelper() {}


	void OutputHelper::FatalError(const char* fmt, ...) {
		PrintTimestamp();
		va_list ap;
		va_start(ap, fmt);
		PrintMessage(fmt, ap);
		va_end(ap);

		// nuke process
		NukeProcess(1);
	}

	void OutputHelper::Printf(const char* fmt, ...) {
		PrintTimestamp();
		va_list ap;
		va_start(ap, fmt);
		PrintMessage(fmt, ap);
		va_end(ap);
	}

	void OutputHelper::RawPrintf(const char* fmt, ...) {
		va_list ap;
		va_start(ap, fmt);
		PrintMessage(fmt, ap);
		va_end(ap);
	}


	void OutputHelper::PrintTimestamp() {
		float time = (float)(GetSysTimeMicros() - g_logTimeZero);
		fprintf(stdout, "%10.6f ", time * 1e-6);
		fflush(stdout);
	}

	void OutputHelper::PrintMessage(const char* fmt, va_list ap) {
		vfprintf(stdout, fmt, ap);
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

}
