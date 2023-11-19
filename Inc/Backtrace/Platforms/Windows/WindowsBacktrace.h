#pragma once

#include <Build.h>

#if BUILD_IS_SYSTEM_WINDOWS

namespace Backtrace
{
	inline void DebugBreak() noexcept
	{
		__debugbreak();
	}
} // namespace Backtrace

#endif