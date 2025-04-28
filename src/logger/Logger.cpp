#include "Logger.h"
#include <fmt/core.h>
#include <fmt/color.h>

Logger::Logger() :
    currentLevel_(LogLevel::INFO),
    consoleOutput_(true),
    fileOutput_(false)
{
}

Logger::~Logger()
{
    if (logFile_.is_open())
    {
        logFile_.close();
    }
}

void Logger::setLevel(LogLevel level)
{
    currentLevel_ = level;
}

void Logger::enableConsoleOutput(bool enable)
{
    consoleOutput_ = enable;
}

void Logger::setLogFile(const std::string& filename)
{
    if (!filename.empty())
    {
        if (logFile_.is_open())
        {
            logFile_.close();
        }
        logFile_.open(filename, std::ios::app);
        fileOutput_ = true;
    }
    else
    {
        if (logFile_.is_open())
        {
            logFile_.close();
        }
        fileOutput_ = false;
    }
}

void Logger::log(LogLevel level, const std::string& message)
{
    if (level < currentLevel_)
    {
        return;
    }

    std::string levelStr;
    fmt::color textColor = fmt::color::white;
    
    switch (level)
    {
    case LogLevel::DEBUG:
        levelStr = "DEBUG";
        textColor = fmt::color::cyan;
        break;
    case LogLevel::INFO:
        levelStr = "INFO";
        textColor = fmt::color::green;
        break;
    case LogLevel::WARNING:
        levelStr = "WARNING";
        textColor = fmt::color::yellow;
        break;
    case LogLevel::ERROR:
        levelStr = "ERROR";
        textColor = fmt::color::red;
        break;
    case LogLevel::FATAL:
        levelStr = "FATAL";
        textColor = fmt::color::purple;
        break;
    }

    std::string plainMessage = fmt::format("[{}] {}\n", levelStr, message);
    
    if (consoleOutput_)
    {
        // Print colored message to console
        fmt::print(fg(textColor), "[{}] {}\n", levelStr, message);
    }

    if (fileOutput_ && logFile_.is_open())
    {
        logFile_ << plainMessage;
        logFile_.flush();
    }
}

void Logger::debug(const std::string& message)
{
    log(LogLevel::DEBUG, message);
}

void Logger::info(const std::string& message)
{
    log(LogLevel::INFO, message);
}

void Logger::warning(const std::string& message)
{
    log(LogLevel::WARNING, message);
}

void Logger::error(const std::string& message)
{
    log(LogLevel::ERROR, message);
}

void Logger::fatal(const std::string& message)
{
    log(LogLevel::FATAL, message);
}