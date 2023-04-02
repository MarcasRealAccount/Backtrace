#include <Build.h>
#include <UTF/UTF.h>

#if BUILD_IS_SYSTEM_WINDOWS

	#include "Backtrace.h"

	#include <mutex>

	#include <Windows.h>

	#include <DbgHelp.h>

namespace Backtrace
{
	static HANDLE g_Process;

	static struct NTSymInitializer
	{
	public:
		NTSymInitializer()
		{
			g_Process = GetCurrentProcess();

			SymSetOptions(SYMOPT_UNDNAME | SYMOPT_LOAD_LINES);
			SymInitializeW(g_Process, L"", true);
		}
	} s_SymInitializer;

	static void*      g_EET;
	static std::mutex g_EETMutex;

	static thread_local void* g_VectoredExceptionHandler;

	static LONG ExcludeExceptionHandler(EXCEPTION_POINTERS* ExceptionPointers);
	static LONG VectoredHandler(EXCEPTION_POINTERS* ExceptionPointers);

	Backtrace Capture(std::uint32_t skip, std::uint32_t maxFrames)
	{
		Backtrace backtrace;

		void** framePtrs  = new void*[maxFrames];
		WORD   frameCount = RtlCaptureStackBackTrace(1 + skip, maxFrames, framePtrs, nullptr);

		alignas(SYMBOL_INFOW) std::uint8_t symbolBuffer[sizeof(SYMBOL_INFOW) + MAX_SYM_NAME * sizeof(wchar_t)];

		SYMBOL_INFOW* symbol = reinterpret_cast<SYMBOL_INFOW*>(&symbolBuffer);
		symbol->SizeOfStruct = sizeof(SYMBOL_INFOW);
		symbol->MaxNameLen   = MAX_SYM_NAME;

		auto& frames = backtrace.frames();
		frames.resize(frameCount);
		for (std::size_t i = 0; i < frameCount; ++i)
		{
			auto&         frame   = frames[i];
			void*         address = framePtrs[i];
			std::uint64_t offset  = 0;

			if (!SymFromAddrW(g_Process, reinterpret_cast<std::uint64_t>(address), &offset, symbol))
			{
				frame.setAddress(address, 0);
				continue;
			}

			frame.setAddress(reinterpret_cast<void*>(symbol->Address), offset);

			SourceLocation source;
			source.setFunction(UTF::WideToChar(symbol->Name));

			DWORD            column = 0;
			IMAGEHLP_LINEW64 line {};
			line.SizeOfStruct = sizeof(line);
			if (SymGetLineFromAddrW64(g_Process, reinterpret_cast<std::uint64_t>(address), &column, &line))
				source.setFile(UTF::WideToChar(line.FileName), line.LineNumber, column);
			frame.setSource(std::move(source));
		}

		delete[] framePtrs;

		return backtrace;
	}

	void HookThrow()
	{
		g_EETMutex.lock();
		if (!g_EET)
		{
			void* handle = AddVectoredExceptionHandler(~0U, &ExcludeExceptionHandler);

			try
			{
				throw Exception("", "", {});
			}
			catch ([[maybe_unused]] const Exception& e)
			{
			}

			RemoveVectoredExceptionHandler(handle);
		}
		g_EETMutex.unlock();

		g_VectoredExceptionHandler = AddVectoredExceptionHandler(~0U, &VectoredHandler);
	}

	void UnhookThrow()
	{
		RemoveVectoredExceptionHandler(g_VectoredExceptionHandler);
	}

	EExceptionResult OpenErrorModal(std::string_view title, std::string_view description, const Backtrace& backtrace)
	{
		std::wstring msg = UTF::CharToWide(description);
		if (!backtrace.frames().empty())
			msg += L"\n\n" + UTF::CharToWide(backtrace.toString());
		std::wstring caption = title.empty()
								   ? L"Error"
								   : L"Error: " + UTF::CharToWide(title);
		int          result  = MessageBoxW(nullptr,
                                 msg.c_str(),
                                 caption.c_str(),
                                 MB_ABORTRETRYIGNORE | MB_ICONERROR | MB_SYSTEMMODAL);
		switch (result)
		{
		case IDABORT: return EExceptionResult::Abort;
		case IDIGNORE: return EExceptionResult::Ignore;
		case IDRETRY: return EExceptionResult::Retry;
		default: return EExceptionResult::Abort;
		}
	}

	void DebugBreak()
	{
		__debugbreak();
	}

	LONG ExcludeExceptionHandler(EXCEPTION_POINTERS* ExceptionPointers)
	{
		g_EET = reinterpret_cast<void*>(ExceptionPointers->ExceptionRecord->ExceptionInformation[2]);
		return 0;
	}

	LONG VectoredHandler(EXCEPTION_POINTERS* ExceptionPointers)
	{
		if (ExceptionPointers->ExceptionRecord->ExceptionInformation[0] == 429065504ULL)
		{
			if (reinterpret_cast<void*>(ExceptionPointers->ExceptionRecord->ExceptionInformation[2]) != g_EET)
				LastBacktrace() = Capture(6, 32);
			else
				LastBacktrace() = {};
		}
		else
		{
			LastBacktrace() = Capture(1, 32);
		}
		return 0;
	}
} // namespace Backtrace

#endif