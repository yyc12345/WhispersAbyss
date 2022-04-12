#pragma once

#include <cstdint>

namespace WhispersAbyss {

	class OutputHelper {
	public:
		OutputHelper();
		~OutputHelper();

		void FatalError(const char* fmt, ...);
		void Printf(const char* fmt, ...);
	private:
		int64_t g_logTimeZero;

		void DebugOutput(const char* pszMsg);
		void NukeProcess(int rc);
		int64_t GetSysTimeMicros();
	};



}


