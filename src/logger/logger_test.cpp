#include "Logger.h"
#include <fmt/core.h>
#include <thread>
#include <chrono>

int main() {
    Logger logger;

    // Set log level to DEBUG to see all messages
    logger.setLevel(LogLevel::DEBUG);

    // Enable console output
    logger.enableConsoleOutput(true);

    // Set up file logging (optional)
    logger.setLogFile("application.log");

    // Test different log levels
    logger.debug("This is a debug message");
    logger.info("Application started successfully");
    logger.warning("This is a warning message");
    logger.error("An error occurred while processing data");
    logger.fatal("Critical system failure");

    // Test with formatted messages
    int errorCode = 404;
    logger.error(fmt::format("Error code: {}", errorCode));

    // Test with more complex formatting
    std::string username = "user123";
    int itemCount = 42;
    logger.info(fmt::format("User {} has {} items in cart", username, itemCount));

    // Simulate processing with different log levels
    logger.info("Starting processing task...");

    for (int i = 0; i < 3; i++) {
        logger.debug(fmt::format("Processing step {}/3", i+1));
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        if (i == 1) {
            logger.warning("Processing taking longer than expected");
        }
    }

    logger.info("Processing completed");

    return 0;
}