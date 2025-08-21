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

// Update log signature to accept file name
void Logger::log(LogLevel level, const std::string& message, int lineNumber, const char* fileName)
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
        levelStr = "INFO ";
        textColor = fmt::color::light_green;
        break;
    case LogLevel::LEVEL_WARNING:
        levelStr = "WARN ";
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

    std::string plainMessage = fmt::format("[{}] {} (at file:{} line:{})\n", levelStr, message, fileName, lineNumber);
    
    if (consoleOutput_)
    {
        fmt::print(fg(textColor), "[{}] {} ", levelStr, message);
        fmt::print(fg(fmt::color::gray), "(at file:{} line:{})\n", fileName, lineNumber);
    }

    if (fileOutput_ && logFile_.is_open())
    {
        logFile_ << plainMessage;
        logFile_.flush();
    }
}

// Update helper methods to accept line/file
void Logger::debug(const std::string& message, int lineNumber, const char* fileName)
{
    log(LogLevel::LEVEL_DEBUG, message, lineNumber, fileName);
}

void Logger::info(const std::string& message, int lineNumber, const char* fileName)
{
    log(LogLevel::LEVEL_INFO, message, lineNumber, fileName);
}

void Logger::warning(const std::string& message, int lineNumber, const char* fileName)
{
    log(LogLevel::LEVEL_WARNING, message, lineNumber, fileName);
}

void Logger::error(const std::string& message, int lineNumber, const char* fileName)
{
    log(LogLevel::LEVEL_ERROR, message, lineNumber, fileName);
}

void Logger::fatal(const std::string& message, int lineNumber, const char* fileName)
{
    log(LogLevel::LEVEL_FATAL, message, lineNumber, fileName);
}