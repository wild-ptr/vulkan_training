#pragma once
#include <iostream>
#include <mutex>
#include <map>

#define dbgI entitysystem::Logger{__PRETTY_FUNCTION__, entitysystem::Logger::ESeverity::INFO}
#define dbgE entitysystem::Logger{__PRETTY_FUNCTION__, entitysystem::Logger::ESeverity::ERROR}
#define dbgC entitysystem::Logger{__PRETTY_FUNCTION__, entitysystem::Logger::ESeverity::CRITICAL}
#define dbgValidLayer entitysystem::Logger{__FUNCTION__, entitysystem::Logger::ESeverity::INFO}

// blocking (for now) thread-safe logger.
namespace entitysystem
{
class Logger
{
public:

enum class ESeverity
{
    INFO = 0,
    ERROR = 1,
    CRITICAL = 2,
};


explicit Logger(const char * func, Logger::ESeverity severity);

template <class T>
Logger& operator<<(T&& msg)
{
#ifdef NDEBUG
	static_cast<void>(msg);
	return *this;
#else
    {
        std::lock_guard<std::mutex> lock(io_mutex);
        std::cout << "[" << func << "]" << " " << translateSeverity() << msg << std::endl;
        return *this;
    }
#endif
}

private:
    std::string translateSeverity();

    const char * func;
    ESeverity severity;
    static std::mutex io_mutex;
};

} // namespace entitysystem
