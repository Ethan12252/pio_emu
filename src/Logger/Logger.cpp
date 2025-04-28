#include "Logger.h"
#include <fmt/core.h>
#include <fmt/color.h>

// TODO: Maybe rewrite in Singleton? and add locks?

Logger::Logger() :
    currentLevel_(LogLevel::LEVEL_INFO),
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
        return;

    std::string levelStr;
    fmt::color textColor = fmt::color::white;
    
    switch (level)
    {
    case LogLevel::LEVEL_DEBUG:
        levelStr = "DEBUG";
        textColor = fmt::color::light_blue;
        break;
    case LogLevel::LEVEL_INFO:
        levelStr = "INFO";
        textColor = fmt::color::light_green;
        break;
    case LogLevel::LEVEL_WARNING:
        levelStr = "WARNING";
        textColor = fmt::color::green_yellow;
        break;
    case LogLevel::LEVEL_ERROR:
        levelStr = "ERROR";
        textColor = fmt::color::red;
        break;
    case LogLevel::LEVEL_FATAL:
        levelStr = "FATAL";
        textColor = fmt::color::magenta;
        break;
    }

    std::string plainMessage = fmt::format("[{}] {}\n", levelStr, message);
    
    if (consoleOutput_)
    {
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
    log(LogLevel::LEVEL_DEBUG, message);
}

void Logger::info(const std::string& message)
{
    log(LogLevel::LEVEL_INFO, message);
}

void Logger::warning(const std::string& message)
{
    log(LogLevel::LEVEL_ERROR, message);
}

void Logger::error(const std::string& message)
{
    log(LogLevel::LEVEL_ERROR, message);
}

void Logger::fatal(const std::string& message)
{
    log(LogLevel::LEVEL_FATAL, message);
}