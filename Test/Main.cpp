#include <Backtrace.h>

#include <iomanip>
#include <iostream>

int main()
{
	Backtrace::HookThrow();
	auto ret = Backtrace::SafeExecute([] {
		throw Backtrace::Exception("Error", "Some unexplainable thing happened");
	});
	Backtrace::UnhookThrow();
	std::cout << std::hex << std::uppercase << std::setfill('0') << std::setw(16) << ret << '\n';
	return (int) ret;
}