#include "Backtrace.h"

#include <iomanip>
#include <sstream>
#include <stack>

namespace Backtrace
{
	static thread_local std::stack<std::pair<ExceptionCallback, void*>> t_ExceptionHandlerStack;

	SourceLocation::SourceLocation() noexcept
		: m_Line(0),
		  m_Column(0) {}

	SourceLocation::SourceLocation(const SourceLocation& copy) noexcept
		: m_File(copy.m_File),
		  m_Function(copy.m_Function),
		  m_Line(copy.m_Line),
		  m_Column(copy.m_Column) {}

	SourceLocation::SourceLocation(SourceLocation&& move) noexcept
		: m_File(std::move(move.m_File)),
		  m_Function(std::move(move.m_Function)),
		  m_Line(move.m_Line),
		  m_Column(move.m_Column)
	{
		move.m_Line   = 0;
		move.m_Column = 0;
	}

	SourceLocation& SourceLocation::operator=(const SourceLocation& copy) noexcept
	{
		m_File     = copy.m_File;
		m_Function = copy.m_Function;
		m_Line     = copy.m_Line;
		m_Column   = copy.m_Column;
		return *this;
	}

	SourceLocation& SourceLocation::operator=(SourceLocation&& move) noexcept
	{
		m_File        = std::move(move.m_File);
		m_Function    = std::move(move.m_Function);
		m_Line        = move.m_Line;
		m_Column      = move.m_Column;
		move.m_Line   = 0;
		move.m_Column = 0;
		return *this;
	}

	void SourceLocation::SetFile(std::filesystem::path file, std::size_t line, std::size_t column) noexcept
	{
		m_File   = std::move(file);
		m_Line   = line;
		m_Column = column;
	}

	void SourceLocation::SetFunction(std::string function) noexcept
	{
		m_Function = std::move(function);
	}

	std::string SourceLocation::ToString() const noexcept
	{
		std::ostringstream str;
		str << m_File << '(' << m_Line << ':' << m_Column << "): " << m_Function;
		return str.str();
	}

	StackFrame::StackFrame() noexcept
		: m_Address(nullptr),
		  m_Offset(0) {}

	StackFrame::StackFrame(const StackFrame& copy) noexcept
		: m_Address(copy.m_Address),
		  m_Offset(copy.m_Offset),
		  m_Source(copy.m_Source) {}

	StackFrame::StackFrame(StackFrame&& move) noexcept
		: m_Address(move.m_Address),
		  m_Offset(move.m_Offset),
		  m_Source(std::move(move.m_Source))
	{
		move.m_Address = nullptr;
		move.m_Offset  = 0;
	}

	StackFrame& StackFrame::operator=(const StackFrame& copy) noexcept
	{
		m_Address = copy.m_Address;
		m_Offset  = copy.m_Offset;
		m_Source  = copy.m_Source;
		return *this;
	}

	StackFrame& StackFrame::operator=(StackFrame&& move) noexcept
	{
		m_Address      = move.m_Address;
		m_Offset       = move.m_Offset;
		m_Source       = std::move(move.m_Source);
		move.m_Address = nullptr;
		move.m_Offset  = 0;
		return *this;
	}

	void StackFrame::SetAddress(void* address, std::size_t offset) noexcept
	{
		m_Address = address;
		m_Offset  = offset;
	}

	void StackFrame::SetSource(SourceLocation source) noexcept
	{
		m_Source = std::move(source);
	}

	std::string StackFrame::ToString() const noexcept
	{
		if (HasSource())
		{
			return "At " + m_Source.ToString();
		}
		else
		{
			std::ostringstream str;
			str << "At " << std::uppercase << std::hex << std::setw(16) << std::setfill('0') << m_Address << " + " << m_Offset;
			return str.str();
		}
	}

	Backtrace::Backtrace() noexcept {}

	Backtrace::Backtrace(const Backtrace& copy) noexcept
		: m_Frames(copy.m_Frames) {}

	Backtrace::Backtrace(Backtrace&& move) noexcept
		: m_Frames(std::move(move.m_Frames)) {}

	Backtrace& Backtrace::operator=(const Backtrace& copy) noexcept
	{
		m_Frames = copy.m_Frames;
		return *this;
	}

	Backtrace& Backtrace::operator=(Backtrace&& move) noexcept
	{
		m_Frames = std::move(move.m_Frames);
		return *this;
	}

	void Backtrace::SetFrames(std::vector<StackFrame> frames) noexcept
	{
		m_Frames = std::move(frames);
	}

	std::string Backtrace::ToString() const noexcept
	{
		std::ostringstream str;
		for (std::size_t i = 0; i < m_Frames.size(); ++i)
		{
			if (i > 0)
				str << '\n';
			str << i << " - " << m_Frames[i].ToString();
		}
		return str.str();
	}

	Exception::Exception(std::string title, std::string message, Backtrace backtrace) noexcept
		: m_Title(title),
		  m_Message(message),
		  m_Backtrace(std::move(backtrace)) {}

	Exception::Exception(const Exception& copy) noexcept
		: m_Title(copy.m_Title),
		  m_Message(copy.m_Message),
		  m_Backtrace(copy.m_Backtrace) {}

	Exception::Exception(Exception&& move) noexcept
		: m_Title(std::move(move.m_Title)),
		  m_Message(std::move(move.m_Message)),
		  m_Backtrace(std::move(move.m_Backtrace)) {}

	Exception& Exception::operator=(const Exception& copy) noexcept
	{
		m_Title     = copy.m_Title;
		m_Message   = copy.m_Message;
		m_Backtrace = copy.m_Backtrace;
		return *this;
	}

	Exception& Exception::operator=(Exception&& move) noexcept
	{
		m_Title     = std::move(move.m_Title);
		m_Message   = std::move(move.m_Message);
		m_Backtrace = std::move(move.m_Backtrace);
		return *this;
	}

	std::string Exception::ToString() const noexcept
	{
		std::ostringstream str;
		str << m_Title << ": " << m_Message << '\n'
			<< m_Backtrace.ToString();
		return str.str();
	}

	void PushExceptionCallback(ExceptionCallback callback, void* userdata) noexcept
	{
		t_ExceptionHandlerStack.push({ callback, userdata });
	}

	void PopExceptionCallback() noexcept
	{
		t_ExceptionHandlerStack.pop();
	}

	EExceptionResult InvokeExceptionCallback(const Exception& exception) noexcept
	{
		if (t_ExceptionHandlerStack.empty())
			return EExceptionResult::Critical;

		auto& handler = t_ExceptionHandlerStack.top();
		return handler.first(handler.second, exception);
	}
} // namespace Backtrace