#include "Backtrace.h"

#if BUILD_IS_SYSTEM_WINDOWS

	#include <algorithm>
	#include <mutex>

	#include <UTF/UTF.h>
	#include <mimalloc.h>

	#include <Windows.h>

	#include <DbgHelp.h>

	#undef GetMessage

namespace Backtrace
{
	static HINSTANCE              s_Instance;
	static HANDLE                 s_Process;
	static void*                  s_TypeToSkip = nullptr;
	static std::mutex             s_VEHMtx;
	static thread_local void*     s_VEHHandle;
	static thread_local Backtrace t_PreviousCapture;

	static struct WindowsInit
	{
		WindowsInit()
		{
			s_Instance = GetModuleHandleW(nullptr);
			s_Process  = GetCurrentProcess();

			SymSetOptions(SYMOPT_UNDNAME | SYMOPT_LOAD_LINES);
			SymInitializeW(s_Process, L"", true);
		}
	} s_WindowsInit;

	static LONG ExcludeVEH(EXCEPTION_POINTERS* ExceptionPointers);
	static LONG VEH(EXCEPTION_POINTERS* ExceptionPointers);

	Backtrace Capture(std::size_t skip, std::size_t frames) noexcept
	{
		DWORD dskip   = (DWORD) std::min<std::size_t>(skip, std::numeric_limits<DWORD>::max());
		DWORD dframes = (DWORD) std::min<std::size_t>(frames, std::numeric_limits<DWORD>::max());

		void** framePtrs  = (void**) mi_malloc(dframes * sizeof(void*));
		WORD   frameCount = RtlCaptureStackBackTrace(dskip, dframes, framePtrs, nullptr);

		std::uint8_t symbolBuffer[sizeof(SYMBOL_INFOW) + MAX_SYM_NAME * sizeof(wchar_t)];
		memset(symbolBuffer, 0, sizeof(symbolBuffer));
		auto symbol          = (SYMBOL_INFOW*) (&symbolBuffer);
		symbol->SizeOfStruct = sizeof(SYMBOL_INFOW);
		symbol->MaxNameLen   = MAX_SYM_NAME;

		std::vector<StackFrame> bframes(frameCount);
		for (std::size_t i = 0; i < frameCount; ++i)
		{
			auto&   bframe  = bframes[i];
			void*   address = framePtrs[i];
			DWORD64 offset  = 0;

			if (!SymFromAddrW(s_Process, (DWORD64) address, &offset, symbol))
			{
				bframe.SetAddress(address, 0);
				continue;
			}
			bframe.SetAddress((void*) symbol->Address, offset);

			SourceLocation source;
			source.SetFunction(UTF::Convert<char, wchar_t>(std::wstring_view(symbol->Name)));

			DWORD            column = 0;
			IMAGEHLP_LINEW64 line {};
			line.SizeOfStruct = sizeof(line);
			if (SymGetLineFromAddrW64(s_Process, (DWORD64) address, &column, &line))
				source.SetFile(std::wstring_view(line.FileName), line.LineNumber, column);
			bframe.SetSource(std::move(source));
		}

		mi_free(framePtrs);
		Backtrace backtrace;
		backtrace.SetFrames(std::move(bframes));
		return backtrace;
	}

	EExceptionResult OpenErrorModal(const Exception& exception) noexcept
	{
		auto title   = UTF::Convert<wchar_t, char>(exception.GetTitle());
		auto message = UTF::Convert<wchar_t, char>(exception.GetMessage() + '\n' + exception.GetBacktrace().ToString());
		switch (MessageBoxW(nullptr, message.c_str(), title.c_str(), MB_ABORTRETRYIGNORE | MB_ICONERROR | MB_SYSTEMMODAL))
		{
		case IDABORT: return EExceptionResult::Abort;
		case IDIGNORE: return EExceptionResult::Ignore;
		case IDRETRY: return EExceptionResult::Retry;
		default: return EExceptionResult::Abort;
		}
	}

	Backtrace& LastBacktrace() noexcept
	{
		return t_PreviousCapture;
	}

	void HookThrow() noexcept
	{
		PushExceptionCallback(&DefaultExceptionCallback, nullptr);

		s_VEHMtx.lock();
		if (!s_TypeToSkip)
		{
			auto vehHandle = AddVectoredExceptionHandler(~0U, &ExcludeVEH);
			if (vehHandle)
			{
				try
				{
					throw Exception("", "", {});
				}
				catch (...)
				{
				}
				RemoveVectoredExceptionHandler(vehHandle);
			}
			else
			{
				InvokeExceptionCallback({ "HookThrow", "Failed to add VEH" });
			}
		}
		s_VEHMtx.unlock();

		s_VEHHandle = AddVectoredExceptionHandler(~0U, &VEH);
	}

	void UnhookThrow() noexcept
	{
		RemoveVectoredExceptionHandler(s_VEHHandle);
		PopExceptionCallback();
	}

	LONG ExcludeVEH(EXCEPTION_POINTERS* ExceptionPointers)
	{
		s_TypeToSkip = (void*) ExceptionPointers->ExceptionRecord->ExceptionInformation[2];
		return 0;
	}

	LONG VEH(EXCEPTION_POINTERS* ExceptionPointers)
	{
		if (ExceptionPointers->ExceptionRecord->ExceptionInformation[0] == 429065504ULL)
		{
			if ((void*) ExceptionPointers->ExceptionRecord->ExceptionInformation[2] != s_TypeToSkip)
				t_PreviousCapture = Capture(6, 32);
			else
				t_PreviousCapture = {};
		}
		else
		{
			t_PreviousCapture = Capture(1, 32);
		}
		return 0;
	}
} // namespace Backtrace

#endif