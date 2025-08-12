// main.cpp
// Entry point for caneta-macos key logger test

#include "key_logger.h"
#include <iostream>
#include <csignal>

// Global pointer for signal handler
KeyLogger* g_logger = nullptr;

// Signal handler for clean shutdown
void signalHandler(int signal) {
  if (signal == SIGINT || signal == SIGTERM) {
    std::cout << "\nReceived interrupt signal..." << std::endl;
    if (g_logger) {
      g_logger->stop();
    }
  }
}

int main(int argc, char* argv[]) {
  std::cout << "Starting Caneta macOS Key Logger..." << std::endl;

  // Set up signal handlers
  signal(SIGINT, signalHandler);
  signal(SIGTERM, signalHandler);

  // Create and initialize the key logger
  KeyLogger logger;
  g_logger = &logger;

  if (!logger.init()) {
    std::cerr << "Failed to initialize key logger" << std::endl;
    return 1;
  }

  // Run the main event loop
  logger.run();

  // Clean up
  g_logger = nullptr;

  return 0;
}