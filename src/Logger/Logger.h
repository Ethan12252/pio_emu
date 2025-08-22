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

    void log(LogLevel level, const std::string& message, int lineNumber, const char* fileName);
    void debug(const std::string& message, int lineNumber, const char* fileName);
    void info(const std::string& message, int lineNumber, const char* fileName);
    void warning(const std::string& message, int lineNumber, const char* fileName);
    void error(const std::string& message, int lineNumber, const char* fileName);
    void fatal(const std::string& message, int lineNumber, const char* fileName);

private:
    LogLevel currentLevel_;
    bool consoleOutput_;
    bool fileOutput_;
    std::ofstream logFile_;
};

static inline Logger logger;

// Macros for logging
#define LOG_DEBUG(message) logger.debug(message, __LINE__, __FILE__)
#define LOG_INFO(message) logger.info(message, __LINE__, __FILE__)
#define LOG_WARNING(message) logger.warning(message, __LINE__, __FILE__)
#define LOG_ERROR(message) logger.error(message, __LINE__, __FILE__)
#define LOG_FATAL(message) logger.fatal(message, __LINE__, __FILE__)

// With formatting using fmt
#define LOG_DEBUG_FMT(fmt_str, ...) logger.debug(::fmt::format(fmt_str, __VA_ARGS__), __LINE__, __FILE__)
#define LOG_INFO_FMT(fmt_str, ...) logger.info(::fmt::format(fmt_str, __VA_ARGS__), __LINE__, __FILE__)
#define LOG_WARNING_FMT(fmt_str, ...) logger.warning(::fmt::format(fmt_str, __VA_ARGS__), __LINE__, __FILE__)
#define LOG_ERROR_FMT(fmt_str, ...) logger.error(::fmt::format(fmt_str, __VA_ARGS__), __LINE__, __FILE__)
#define LOG_FATAL_FMT(fmt_str, ...) logger.fatal(::fmt::format(fmt_str, __VA_ARGS__), __LINE__, __FILE__)
