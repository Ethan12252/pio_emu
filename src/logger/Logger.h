#pragma once

#include <string>
#include <fstream>
#include <fmt/core.h>
#include <fmt/color.h>

enum class LogLevel
{
    DEBUG,
    INFO,
    WARNING,
    ERROR,
    FATAL
};

class Logger
{
public:
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
