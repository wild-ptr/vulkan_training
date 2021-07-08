#pragma once
#include <iostream>
#include <map>
#include <mutex>

#define dbgI \
    entitysystem::Logger { __PRETTY_FUNCTION__, entitysystem::Logger::ESeverity::INFO }
#define dbgE \
    entitysystem::Logger { __PRETTY_FUNCTION__, entitysystem::Logger::ESeverity::ERROR }
#define dbgC \
    entitysystem::Logger { __PRETTY_FUNCTION__, entitysystem::Logger::ESeverity::CRITICAL }
#define dbgValidLayer \
    entitysystem::Logger { __FUNCTION__, entitysystem::Logger::ESeverity::INFO }

#define NEWL "\n"

// blocking (for now) thread-safe logger.
namespace entitysystem {
class Logger {
public:
    enum class ESeverity {
        INFO = 0,
        ERROR = 1,
        CRITICAL = 2,
    };

    explicit Logger(const char* func, Logger::ESeverity severity);

    template <class T>
    Logger& operator<<(T&& msg)
    {
#ifdef NDEBUG
        static_cast<void>(msg);
        return *this;
#else
        {
            std::lock_guard<std::mutex> lock(io_mutex);
            if (not function_info_already_displayed) {
                std::cout << "[" << func << "]"
                          << " " << translateSeverity() << " ";
                function_info_already_displayed = true;
            }

            std::cout << msg << " ";

            return *this;
        }
#endif
    }

private:
    std::string translateSeverity();

    const char* func;
    ESeverity severity;
    static std::mutex io_mutex;
    bool function_info_already_displayed { false };
};

} // namespace entitysystem
