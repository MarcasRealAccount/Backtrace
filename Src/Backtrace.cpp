#include "Backtrace.h"

#include <UTF/UTF.h>

#include <iomanip>
#include <sstream>

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

	EExceptionResult InvokeExceptionCallback(const Exception& exception)
	{
		if (g_Callback)
		{
			try
			{
				return g_Callback(exception);
			}
			catch (...)
			{
				return EExceptionResult::Critical;
			}
		}
		return EExceptionResult::Abort;
	}

	std::string SourceLocation::toString() const
	{
		std::ostringstream str;
		str << UTF::Convert<char, std::filesystem::path::value_type>(m_File.native())
			<< '(' << m_Line << ':' << m_Column << "): "
			<< m_Function << "()";
		return str.str();
	}

	std::string StackFrame::toString() const
	{
		std::ostringstream str;
		str << "At ";
		if (hasSource())
		{
			str << m_Source.toString();
		}
		else
		{
			str << std::setw(sizeof(m_Address) * 2) << std::setfill('0')
				<< std::uppercase << std::hex
				<< m_Address << " + " << m_Offset
				<< std::dec << std::nouppercase;
		}
		return str.str();
	}

	std::string Backtrace::toString() const
	{
		std::ostringstream str;

		bool newline = false;
		for (auto& frame : m_Frames)
		{
			if (newline)
				str << "\n";
			newline = true;
			str << frame.toString();
		}
		return str.str();
	}

	std::string Exception::toString() const
	{
		std::ostringstream str;
		str << m_Title << ": "
			<< '"' << m_Message << '"';

		if (!m_Backtrace.frames().empty())
			str << '\n'
				<< m_Backtrace.toString();
		return str.str();
	}
} // namespace Backtrace