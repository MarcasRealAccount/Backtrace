#pragma once

#include "Backtrace.h"

#include <format>
#include <string>
#include <utility>

namespace Backtrace
{
	template <class... Args>
	requires(sizeof...(Args) > 0)
	constexpr void Assert(bool statement, std::format_string<Args...> format, Args&&... args)
	{
		if constexpr (c_EnableAsserts)
		{
			if (statement)
				return;

			if constexpr (c_EnableBreakpoint)
				DebugBreak();

			std::string message = std::format(format, std::forward<Args>(args)...);
			throw Exception("Assert", std::move(message), Capture(1));
		}
	}
} // namespace Backtrace