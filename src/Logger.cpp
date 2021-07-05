#include <Logger.hpp>

namespace entitysystem {
Logger::Logger(const char* func, Logger::ESeverity severity)
    : func(func)
    , severity(severity)
{
}

std::string Logger::translateSeverity()
{
    switch (severity) {
    case ESeverity::INFO:
        return "INFO: ";
    case ESeverity::ERROR:
        return "ERROR: ";
    case ESeverity::CRITICAL:
        return "CRITICAL: ";
    default:
        return "";
    }
    return "";
}

std::mutex Logger::io_mutex;
} // namespace entitysystem
