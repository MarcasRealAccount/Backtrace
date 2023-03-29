#pragma once

#include <cstddef>

#include <filesystem>
#include <stdexcept>
#include <vector>

#include <Build.h>

#ifdef BACKTRACE_FORMATTING
	#ifdef BACKTRACE_FMT
		#include <fmt/format.h>
	#else
		#include <format>
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

		SourceLocation(SourceLocation&& move) noexcept
			: m_File(std::move(move.m_File)),
			  m_Function(std::move(move.m_Function)),
			  m_Line(move.m_Line),
			  m_Column(move.m_Column) {}

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

		StackFrame(StackFrame&& move) noexcept
			: m_Address(move.m_Address),
			  m_Offset(move.m_Offset),
			  m_Source(std::move(move.m_Source))
		{
			move.m_Address = nullptr;
			move.m_Offset  = 0;
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

	private:
		std::vector<StackFrame> m_Frames;
	};

	Backtrace  Capture(std::uint32_t skip, std::uint32_t maxFrames);
	Backtrace& LastBacktrace();
	void       HookThrow();
	void       UnhookThrow();

	struct Exception
	{
	public:
		Exception(std::string title, std::string message, Backtrace backtrace = Capture(0, 10))
			: m_Title(std::move(title)),
			  m_Message(std::move(message)),
			  m_Backtrace(std::move(backtrace)) {}

#ifdef BACKTRACE_FORMATTING
	#ifdef BACKTRACE_FMT
		template <class... Args>
		Exception(std::string title, fmt::format_string<Args...> format, Args&&... args, Backtrace backtrace = Capture(0, 10))
			: m_Title(std::move(title)),
			  m_Message(fmt::format(format, std::forward<Args>(args)...)),
			  m_Backtrace(std::move(backtrace))
		{
		}
	#else
		template <class... Args>
		Exception(std::string title, std::format_string<Args...> format, Args&&... args, Backtrace backtrace = Capture(0, 10))
			: m_Title(std::move(title)),
			  m_Message(std::format(format, std::forward<Args>(args)...)),
			  m_Backtrace(std::move(backtrace))
		{
		}
	#endif
#endif

		Exception(Exception&& exception) noexcept
			: m_Title(std::move(exception.m_Title)),
			  m_Message(std::move(exception.m_Message)),
			  m_Backtrace(std::move(exception.m_Backtrace))
		{
		}

		Exception& operator=(Exception&& exception) noexcept
		{
			m_Title     = std::move(exception.m_Title);
			m_Message   = std::move(exception.m_Message);
			m_Backtrace = std::move(exception.m_Backtrace);
			return *this;
		}

		auto& title() const { return m_Title; }

		auto& what() const { return m_Message; }

		auto& backtrace() const { return m_Backtrace; }

	private:
		std::string m_Title;
		std::string m_Message;
		Backtrace   m_Backtrace;
	};

	using ExceptionCallback = void (*)(const Exception& exception);

	ExceptionCallback SetExceptionCallback(ExceptionCallback callback);

	void InvokeExceptionCallback(const Exception& exception);

	void DebugBreak();

	template <class F, class... Args>
	std::uint32_t SafeExecute(F&& f, Args&&... args)
	{
		try
		{
			f(std::forward<Args>(args)...);
			return 0;
		}
		catch (const Exception& exception)
		{
			InvokeExceptionCallback(exception);
			return 0x7FFF'FFFF;
		}
		catch (const std::exception& exception)
		{
			InvokeExceptionCallback({ "std::exception", exception.what(), LastBacktrace() });
			return 0x000'7FFF;
		}
		catch (...)
		{
			InvokeExceptionCallback({ "Uncaught exception", "Uncaught exception occurred", LastBacktrace() });
			return 0x007F'FFFF;
		}
	}

	void AssertImpl(bool condition, std::string_view message)
	{
		if constexpr (CommonBuild::c_IsConfigDebug)
		{
			if (!condition)
			{
				if constexpr (!CommonBuild::c_IsConfigDist)
					DebugBreak();

				throw Exception("Assertion", std::string { message }, Capture(1, 10));
			}
		}
	}

#ifdef BACKTRACE_FORMATTING
	#ifdef BACKTRACE_FMT
	template <class... Args>
	void Assert(bool condition, fmt::format_string<Args...> format, Args&&... args)
	{
		if constexpr (CommonBuild::c_IsConfigDebug)
		{
			if (!condition)
			{
				if constexpr (!CommonBuild::c_IsConfigDist)
					DebugBreak();

				throw Exception("Assertion", format, std::forward<Args>(args)..., Capture(1, 10));
			}
		}
	}
	#else
	template <class... Args>
	void Assert(bool condition, std::format_string<Args...> format, Args&&... args)
	{
		if constexpr (CommonBuild::c_IsConfigDebug)
		{
			if (!condition)
			{
				if constexpr (!CommonBuild::c_IsConfigDist)
					DebugBreak();

				throw Exception("Assertion", format, std::forward<Args>(args)..., Capture(1, 10));
			}
		}
	}
	#endif
#endif
} // namespace Backtrace