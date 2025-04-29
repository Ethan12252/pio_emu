#pragma once
#include <string>
#include <fstream>
#include <fmt/core.h>

class Logger
{
public:
    // default level: info
    enum class LogLevel
    {
        LEVEL_DEBUG,
        LEVEL_INFO,
        LEVEL_ERROR,
        LEVEL_WARNING,
        LEVEL_FATAL
    };

    Logger();
    ~Logger();

    void setLevel(LogLevel level);
    void enableConsoleOutput(bool enable);
    void setLogFile(const std::string& filename);

    void log(LogLevel level, const std::string& message);
    void debug(const std::string& message);
    void info(const std::string& message);
    void warning(const std::string& message);
    void error(const std::string& message);
    void fatal(const std::string& message);

private:
    LogLevel currentLevel_;
    bool consoleOutput_;
    bool fileOutput_;
    std::ofstream logFile_;
};

inline Logger logger;

// Macros for logging
#define LOG_DEBUG(message) logger.debug(message)
#define LOG_INFO(message) logger.info(message)
#define LOG_WARNING(message) logger.warning(message)
#define LOG_ERROR(message) logger.error(message)
#define LOG_FATAL(message) logger.fatal(message)

// With formatting using fmt
#define LOG_DEBUG_FMT(fmt_str, ...) logger.debug(::fmt::format(fmt_str, __VA_ARGS__))
#define LOG_INFO_FMT(fmt_str, ...) logger.info(::fmt::format(fmt_str, __VA_ARGS__))
#define LOG_WARNING_FMT(fmt_str, ...) logger.warning(::fmt::format(fmt_str, __VA_ARGS__))
#define LOG_ERROR_FMT(fmt_str, ...) logger.error(::fmt::format(fmt_str, __VA_ARGS__))
#define LOG_FATAL_FMT(fmt_str, ...) logger.fatal(::fmt::format(fmt_str, __VA_ARGS__))
