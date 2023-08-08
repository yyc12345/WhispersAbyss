#pragma once

#include "others_helper.hpp"
#include <cstdint>
#include <cstdarg>

namespace WhispersAbyss {

	class OutputHelper {
	public:
		enum class Component {
			TcpInstance, TcpFactory,
			GnsInstance, GnsFactory,
			BridgeInstance, BridgeFactory,
			Core
		};

		OutputHelper();
		~OutputHelper();

		/// <summary>
		/// Print fatal error and try to nuke process.
		/// </summary>
		void FatalError(const char* fmt, ...);
		void FatalError(Component comp, IndexDistributor::Index_t index, const char* fmt, ...);
		/// <summary>
		/// Print normal log
		/// </summary>
		void Printf(const char* fmt, ...);
		void Printf(Component comp, IndexDistributor::Index_t index, const char* fmt, ...);
		/// <summary>
		/// Print log without timestamp.
		/// </summary>
		void RawPrintf(const char* fmt, ...);
	private:
		int64_t g_logTimeZero;

		void PrintTimestamp();
		void PrintComponent(Component comp, IndexDistributor::Index_t index);
		void PrintMessage(const char* fmt, va_list ap);
		void NukeProcess(int rc);
		int64_t GetSysTimeMicros();
	};



}


