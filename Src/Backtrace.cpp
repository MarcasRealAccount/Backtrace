#include "Backtrace.h"

namespace Backtrace
{
	ExceptionCallback g_Callback;

	thread_local Backtrace g_Backtrace;

	Backtrace& LastBacktrace()
	{
		return g_Backtrace;
	}

	ExceptionCallback SetExceptionCallback(ExceptionCallback callback)
	{
		ExceptionCallback old = g_Callback;
		g_Callback            = callback;
		return old;
	}

	void InvokeExceptionCallback(const Exception& exception)
	{
		if (g_Callback)
			g_Callback(exception);
	}
} // namespace Backtrace