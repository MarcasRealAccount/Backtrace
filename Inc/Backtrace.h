#pragma once

#include "Build.h"

#include <cstddef>

#include <string_view>
#include <filesystem>
#include <vector>

// TODO(MarcasRealAccount): Migrate to CommonBuild package
namespace Common
{
	template <class F, class... Args>
	using InvokeResult = decltype(std::declval<F>()(std::declval<Args>()...));
	template <class F, class... Args>
	concept Invocable = requires(F&& f, Args&&... args) { f(std::forward<Args>(args)...); };
	template <class F, class R, class... Args>
	concept InvocableReturn = Invocable<F, Args...> && std::convertible_to<InvokeResult<F, Args...>, R>;
} // namespace Common
// ENDTODO

namespace Backtrace
{
	static constexpr bool c_EnableAsserts    = Common::c_IsConfigDebug;
	static constexpr bool c_EnableBreakpoint = !Common::c_IsConfigDist;

	struct SourceLocation
	{
	public:
		SourceLocation() noexcept;
		SourceLocation(const SourceLocation& copy) noexcept;
		SourceLocation(SourceLocation&& move) noexcept;
		SourceLocation& operator=(const SourceLocation& copy) noexcept;
		SourceLocation& operator=(SourceLocation&& move) noexcept;

		void SetFile(std::filesystem::path file, std::size_t line, std::size_t column) noexcept;
		void SetFunction(std::string function) noexcept;

		auto& GetFile() const noexcept { return m_File; }
		auto& GetFunction() const noexcept { return m_Function; }
		auto  GetLine() const noexcept { return m_Line; }
		auto  GetColumn() const noexcept { return m_Column; }

		std::string ToString() const noexcept;

	private:
		std::filesystem::path m_File;
		std::string           m_Function;
		std::size_t           m_Line;
		std::size_t           m_Column;
	};

	struct StackFrame
	{
	public:
		StackFrame() noexcept;
		StackFrame(const StackFrame& copy) noexcept;
		StackFrame(StackFrame&& move) noexcept;
		StackFrame& operator=(const StackFrame& copy) noexcept;
		StackFrame& operator=(StackFrame&& move) noexcept;

		void SetAddress(void* address, std::size_t offset) noexcept;
		void SetSource(SourceLocation source) noexcept;

		auto  GetAddress() const noexcept { return m_Address; }
		auto  GetOffset() const noexcept { return m_Offset; }
		auto& GetSource() const noexcept { return m_Source; }
		bool  HasSource() const noexcept { return !m_Source.GetFile().empty(); }

		std::string ToString() const noexcept;

	private:
		void*          m_Address;
		std::size_t    m_Offset;
		SourceLocation m_Source;
	};

	struct Backtrace
	{
	public:
		Backtrace() noexcept;
		Backtrace(const Backtrace& copy) noexcept;
		Backtrace(Backtrace&& move) noexcept;
		Backtrace& operator=(const Backtrace& copy) noexcept;
		Backtrace& operator=(Backtrace&& move) noexcept;

		void  SetFrames(std::vector<StackFrame> frames) noexcept;
		auto& GetFrames() const noexcept { return m_Frames; }

		std::string ToString() const noexcept;

	private:
		std::vector<StackFrame> m_Frames;
	};

	Backtrace Capture(std::size_t skip = 0, std::size_t frames = 10) noexcept;

	struct Exception
	{
	public:
		Exception(std::string title, std::string message, Backtrace backtrace = Capture()) noexcept;
		Exception(const Exception& copy) noexcept;
		Exception(Exception&& move) noexcept;
		Exception& operator=(const Exception& copy) noexcept;
		Exception& operator=(Exception&& move) noexcept;

		auto& GetTitle() const noexcept { return m_Title; }
		auto& GetMessage() const noexcept { return m_Message; }
		auto& GetBacktrace() const noexcept { return m_Backtrace; }

		std::string ToString() const noexcept;

	private:
		std::string m_Title;
		std::string m_Message;
		Backtrace   m_Backtrace;
	};

	enum class EExceptionResult
	{
		Ignore,
		Retry,
		Abort,
		Critical
	};

	using ExceptionCallback = EExceptionResult (*)(void* userdata, const Exception& exception);

	inline void             DebugBreak() noexcept;
	EExceptionResult        OpenErrorModal(const Exception& exception) noexcept;
	inline EExceptionResult DefaultExceptionCallback([[maybe_unused]] void* userdata, const Exception& exception) noexcept
	{
		return OpenErrorModal(exception);
	}

	Backtrace&       LastBacktrace() noexcept;
	void             HookThrow() noexcept;
	void             UnhookThrow() noexcept;
	void             PushExceptionCallback(ExceptionCallback callback, void* userdata) noexcept;
	void             PopExceptionCallback() noexcept;
	EExceptionResult InvokeExceptionCallback(const Exception& exception) noexcept;

	template <class... Args>
	std::uint64_t SafeExecute(Common::Invocable<Args...> auto&& executor, Args&&... args);

	constexpr void Assert(bool statement, std::string message);
	constexpr void Assert(bool statement, Common::InvocableReturn<std::string> auto&& messageProvider);
} // namespace Backtrace

#if BUILD_IS_SYSTEM_WINDOWS
	#include "Backtrace/Platforms/Windows/WindowsBacktrace.h"
#endif

#include <utility>

namespace Backtrace
{
	template <class... Args>
	std::uint64_t SafeExecute(Common::Invocable<Args...> auto&& executor, Args&&... args)
	{
		std::uint64_t    code   = 0;
		EExceptionResult result = EExceptionResult::Retry;
		// PushExceptionCallback();
		while (result == EExceptionResult::Retry)
		{
			try
			{
				if constexpr (std::convertible_to<Common::InvokeResult<decltype(executor), Args...>, std::uint64_t>)
				{
					return static_cast<std::uint64_t>(executor(std::forward<Args>(args)...));
				}
				else
				{
					executor(std::forward<Args>(args)...);
					code   = 0;
					result = EExceptionResult::Critical;
				}
			}
			catch (const Exception& exception)
			{
				result = InvokeExceptionCallback(exception);
				switch (result)
				{
				case EExceptionResult::Ignore: code = 0x8080ULL; break;
				case EExceptionResult::Abort: code = 0xC080ULL; break;
				case EExceptionResult::Critical: code = 0xF080ULL; break;
				default: break;
				}
			}
			catch (const std::exception& exception)
			{
				result = InvokeExceptionCallback(Exception { "std::exception", exception.what(), LastBacktrace() });
				switch (result)
				{
				case EExceptionResult::Ignore: code = 0x8001ULL; break;
				case EExceptionResult::Abort: code = 0xC001ULL; break;
				case EExceptionResult::Critical: code = 0xF001ULL; break;
				default: break;
				}
			}
			catch (...)
			{
				result = InvokeExceptionCallback(Exception { "Unknown exception", "Unknown exception occurred", LastBacktrace() });
				switch (result)
				{
				case EExceptionResult::Ignore: code = 0x8000ULL; break;
				case EExceptionResult::Abort: code = 0xC000ULL; break;
				case EExceptionResult::Critical: code = 0xF000ULL; break;
				default: break;
				}
			}
		}
		// PopExceptionCallback();
		return code;
	}

	constexpr void Assert(bool statement, std::string message)
	{
		if constexpr (c_EnableAsserts)
		{
			if (statement)
				return;

			if constexpr (c_EnableBreakpoint)
				DebugBreak();

			throw Exception("Assert", std::move(message), Capture(1));
		}
	}

	constexpr void Assert(bool statement, Common::InvocableReturn<std::string> auto&& messageProvider)
	{
		if constexpr (c_EnableAsserts)
		{
			if (statement)
				return;

			if constexpr (c_EnableBreakpoint)
				DebugBreak();

			throw Exception("Assert", static_cast<std::string>(messageProvider()), Capture(1));
		}
	}
} // namespace Backtrace