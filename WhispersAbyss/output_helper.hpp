#pragma once

#include <cstdint>
#include <cstdarg>

namespace WhispersAbyss {

	class OutputHelper {
	public:
		OutputHelper();
		~OutputHelper();

		/// <summary>
		/// Print fatal error and try to nuke process.
		/// </summary>
		void FatalError(const char* fmt, ...);
		/// <summary>
		/// Print normal log
		/// </summary>
		void Printf(const char* fmt, ...);
		/// <summary>
		/// Print log without timestamp.
		/// </summary>
		void RawPrintf(const char* fmt, ...);
	private:
		int64_t g_logTimeZero;

		void PrintTimestamp();
		void PrintMessage(const char* fmt, va_list ap);
		void NukeProcess(int rc);
		int64_t GetSysTimeMicros();
	};



}


