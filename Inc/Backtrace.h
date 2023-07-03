#pragma once

#include <cstddef>

#include <filesystem>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <Build.h>

#ifdef BACKTRACE_FORMATTING
	#ifdef BACKTRACE_SPDLOG
		#include <spdlog/fmt/fmt.h>
		#define BACKTRACE_USE_FMT
	#elif defined(BACKTRACE_FMT)
		#include <fmt/format.h>
		#define BACKTRACE_USE_FMT
	#else
		#include <format>
		#define BACKTRACE_USE_STD
	#endif
#endif

namespace Backtrace
{
	struct SourceLocation
	{
	public:
		SourceLocation()
			: m_Line(0),
			  m_Column(0) {}

		SourceLocation(const SourceLocation& copy)
			: m_File(copy.m_File),
			  m_Function(copy.m_Function),
			  m_Line(copy.m_Line),
			  m_Column(copy.m_Column) {}

		SourceLocation(SourceLocation&& move) noexcept
			: m_File(std::move(move.m_File)),
			  m_Function(std::move(move.m_Function)),
			  m_Line(move.m_Line),
			  m_Column(move.m_Column) {}

		SourceLocation& operator=(const SourceLocation& copy)
		{
			m_File     = copy.m_File;
			m_Function = copy.m_Function;
			return *this;
		}

		SourceLocation& operator=(SourceLocation&& move) noexcept
		{
			m_File     = std::move(move.m_File);
			m_Function = std::move(move.m_Function);
			m_Line     = move.m_Line;
			m_Column   = move.m_Column;
			return *this;
		}

		void setFile(std::filesystem::path file, std::size_t line, std::size_t column)
		{
			m_File   = std::move(file);
			m_Line   = line;
			m_Column = column;
		}

		void setFunction(std::string function)
		{
			m_Function = std::move(function);
		}

		auto& file() const { return m_File; }

		auto& function() const { return m_Function; }

		auto line() const { return m_Line; }

		auto column() const { return m_Column; }

		std::string toString() const;

	private:
		std::filesystem::path m_File;
		std::string           m_Function;
		std::size_t           m_Line;
		std::size_t           m_Column;
	};

	struct StackFrame
	{
	public:
		StackFrame()
			: m_Address(nullptr),
			  m_Offset(0) {}

		StackFrame(void* address, std::size_t offset, SourceLocation source)
			: m_Address(address),
			  m_Offset(offset),
			  m_Source(std::move(source)) {}

		StackFrame(const StackFrame& copy)
			: m_Address(copy.m_Address),
			  m_Offset(copy.m_Offset),
			  m_Source(copy.m_Source)
		{
		}

		StackFrame(StackFrame&& move) noexcept
			: m_Address(move.m_Address),
			  m_Offset(move.m_Offset),
			  m_Source(std::move(move.m_Source))
		{
			move.m_Address = nullptr;
			move.m_Offset  = 0;
		}

		StackFrame& operator=(const StackFrame& copy)
		{
			m_Address = copy.m_Address;
			m_Offset  = copy.m_Offset;
			m_Source  = copy.m_Source;
			return *this;
		}

		StackFrame& operator=(StackFrame&& move) noexcept
		{
			m_Address = move.m_Address;
			m_Offset  = move.m_Offset;
			m_Source  = std::move(move.m_Source);

			move.m_Address = nullptr;
			move.m_Offset  = 0;
			return *this;
		}

		void setAddress(void* address, std::size_t offset)
		{
			m_Address = address;
			m_Offset  = offset;
		}

		void setSource(SourceLocation source)
		{
			m_Source = std::move(source);
		}

		auto address() const { return m_Address; }

		auto offset() const { return m_Offset; }

		auto& source() const { return m_Source; }

		bool hasSource() const { return !m_Source.file().empty(); }

		std::string toString() const;

	private:
		void*          m_Address;
		std::size_t    m_Offset;
		SourceLocation m_Source;
	};

	struct Backtrace
	{
	public:
		auto& frames() { return m_Frames; }

		auto& frames() const { return m_Frames; }

		std::string toString() const;

	private:
		std::vector<StackFrame> m_Frames;
	};

	Backtrace  Capture(std::uint32_t skip, std::uint32_t maxFrames);
	Backtrace& LastBacktrace();
	void       HookThrow();
	void       UnhookThrow();

	enum class EExceptionResult
	{
		Ignore,
		Retry,
		Abort,
		Critical
	};

	EExceptionResult OpenErrorModal(std::string_view title, std::string_view description, const Backtrace& backtrace);

	struct Exception
	{
	public:
		Exception(std::string title, std::string message, Backtrace backtrace = Capture(0, 10))
			: m_Title(std::move(title)),
			  m_Message(std::move(message)),
			  m_Backtrace(std::move(backtrace))
		{
		}

		Exception(const Exception& copy)
			: m_Title(copy.m_Title),
			  m_Message(copy.m_Message),
			  m_Backtrace(copy.m_Backtrace)
		{
		}

		Exception(Exception&& move) noexcept
			: m_Title(std::move(move.m_Title)),
			  m_Message(std::move(move.m_Message)),
			  m_Backtrace(std::move(move.m_Backtrace))
		{
		}

		Exception& operator=(const Exception& copy)
		{
			m_Title     = copy.m_Title;
			m_Message   = copy.m_Message;
			m_Backtrace = copy.m_Backtrace;
			return *this;
		}

		Exception& operator=(Exception&& move) noexcept
		{
			m_Title     = std::move(move.m_Title);
			m_Message   = std::move(move.m_Message);
			m_Backtrace = std::move(move.m_Backtrace);
			return *this;
		}

		auto& title() const { return m_Title; }

		auto& what() const { return m_Message; }

		auto& backtrace() const { return m_Backtrace; }

		std::string toString() const;

	private:
		std::string m_Title;
		std::string m_Message;
		Backtrace   m_Backtrace;
	};

	using ExceptionCallback = EExceptionResult (*)(const Exception& exception);

	ExceptionCallback SetExceptionCallback(ExceptionCallback callback);

	EExceptionResult InvokeExceptionCallback(const Exception& exception);

	void DebugBreak();

	template <class F, class R, class... Args>
	concept Functor =
		requires(F&& f, Args&&... args) {
			{
				f(std::forward<Args>(args)...)
			} -> std::same_as<R>;
		};

	template <class F, class... Args>
	std::uint32_t SafeExecute(F&& f, Args&&... args)
	{
		while (true)
		{
			try
			{
				if constexpr (Functor<F, std::uint32_t>)
				{
					return f(std::forward<Args>(args)...);
				}
				else
				{
					f(std::forward<Args>(args)...);
					return 0;
				}
			}
			catch (const Exception& exception)
			{
				auto result = InvokeExceptionCallback(exception);
				if (result == EExceptionResult::Critical)
					return 0x7FFF'FFFF;
				else if (result == EExceptionResult::Abort)
					return 0x007F'FFFF;
				else if (result == EExceptionResult::Ignore)
					return 0;
			}
			catch (const std::exception& exception)
			{
				auto result = InvokeExceptionCallback({ "std::exception", exception.what(), LastBacktrace() });
				if (result == EExceptionResult::Critical)
					return 0x7FFF'FFFF;
				else if (result == EExceptionResult::Abort)
					return 0x000'7FFF;
				else if (result == EExceptionResult::Ignore)
					return 0;
			}
			catch (...)
			{
				auto result = InvokeExceptionCallback({ "Uncaught exception", "Uncaught exception occurred", LastBacktrace() });
				if (result == EExceptionResult::Critical)
					return 0x7FFF'FFFF;
				else if (result == EExceptionResult::Abort)
					return 0x0000'007F;
				else if (result == EExceptionResult::Ignore)
					return 0;
			}
		}
	}

	inline void AssertImpl(bool condition, std::string_view message)
	{
		if constexpr (Common::c_IsConfigDebug)
		{
			if (!condition)
			{
				if constexpr (!Common::c_IsConfigDist)
					DebugBreak();

				throw Exception("Assertion", std::string { message }, Capture(1, 10));
			}
		}
	}

#ifdef BACKTRACE_USE_FMT
	template <class... Args>
	void Assert(bool condition, fmt::format_string<Args...> format, Args&&... args)
	{
		if constexpr (Common::c_IsConfigDebug)
		{
			if (!condition)
			{
				if constexpr (!Common::c_IsConfigDist)
					DebugBreak();

				throw Exception("Assertion", fmt::format(format, std::forward<Args>(args)...), Capture(1, 10));
			}
		}
	}
#elif defined(BACKTRACE_USE_STD)
	template <class... Args>
	void Assert(bool condition, std::format_string<Args...> format, Args&&... args)
	{
		if constexpr (Common::c_IsConfigDebug)
		{
			if (!condition)
			{
				if constexpr (!Common::c_IsConfigDist)
					DebugBreak();

				throw Exception("Assertion", std::format(format, std::forward<Args>(args)...), Capture(1, 10));
			}
		}
	}
#endif
} // namespace Backtrace

#ifdef BACKTRACE_USE_FMT
template <>
struct fmt::formatter<Backtrace::SourceLocation>
{
public:
	constexpr auto parse(format_parse_context& ctx) -> decltype(ctx.begin())
	{
		return ctx.begin();
	}

	template <class FormatContext>
	auto format(const Backtrace::SourceLocation& location, FormatContext& ctx) -> decltype(ctx.out())
	{
		return fmt::format_to(ctx.out(), "{}", location.toString());
	}
};

template <>
struct fmt::formatter<Backtrace::Backtrace>
{
public:
	constexpr auto parse(format_parse_context& ctx) -> decltype(ctx.begin())
	{
		return ctx.begin();
	}

	template <class FormatContext>
	auto format(const Backtrace::Backtrace& backtrace, FormatContext& ctx) -> decltype(ctx.out())
	{
		return fmt::format_to(ctx.out(), "{}", backtrace.toString());
	}
};

template <>
struct fmt::formatter<Backtrace::Exception>
{
public:
	constexpr auto parse(format_parse_context& ctx) -> decltype(ctx.begin())
	{
		return ctx.begin();
	}

	template <class FormatContext>
	auto format(const Backtrace::Exception& exception, FormatContext& ctx) -> decltype(ctx.out())
	{
		return fmt::format_to(ctx.out(), "{}", exception.toString());
	}
};
#elif defined(BACKTRACE_USE_STD)
template <>
struct std::formatter<Backtrace::SourceLocation>
{
public:
	constexpr auto parse(format_parse_context& ctx) -> decltype(ctx.begin())
	{
		return ctx.begin();
	}

	template <class FormatContext>
	auto format(const Backtrace::SourceLocation& location, FormatContext& ctx) -> decltype(ctx.out())
	{
		return std::format_to(ctx.out(), "{}", location.toString());
	}
};

template <>
struct std::formatter<Backtrace::Backtrace>
{
public:
	constexpr auto parse(format_parse_context& ctx) -> decltype(ctx.begin())
	{
		return ctx.begin();
	}

	template <class FormatContext>
	auto format(const Backtrace::Backtrace& backtrace, FormatContext& ctx) -> decltype(ctx.out())
	{
		return std::format_to(ctx.out(), "{}", backtrace.toString());
	}
};

template <>
struct std::formatter<Backtrace::Exception>
{
public:
	constexpr auto parse(format_parse_context& ctx) -> decltype(ctx.begin())
	{
		return ctx.begin();
	}

	template <class FormatContext>
	auto format(const Backtrace::Exception& exception, FormatContext& ctx) -> decltype(ctx.out())
	{
		return std::format_to(ctx.out(), "{}", exception.toString());
	}
};
#endif