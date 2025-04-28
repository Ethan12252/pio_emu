#include "Logger.h"
#include <fmt/core.h>
#include <thread>
#include <chrono>

int main() {
    // Set log level to DEBUG to see all messages
    logger.setLevel(Logger::LogLevel::LEVEL_DEBUG);

    // Enable console output (this is default)
    logger.enableConsoleOutput(true);

    // file logging (disable by default, this will enable it)
    // logger.setLogFile("application.log");

    // Different log level
    LOG_DEBUG("This is a debug message");
    LOG_INFO("Application started successfully");
    LOG_WARNING("This is a warning message");
    LOG_ERROR("An error occurred while processing data");
    LOG_FATAL("Critical system failure");

    // With formatted message
    int errorCode = 404;
    LOG_ERROR_FMT("Error code: {}", errorCode);
    
    std::string username = "user123";
    int itemCount = 42;
    LOG_INFO_FMT("User {} has {} items in cart", username, itemCount);

    return 0;
}