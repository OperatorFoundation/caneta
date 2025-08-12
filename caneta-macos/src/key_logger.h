// key_logger.h
// Key press event logger using caneta-sdl

#ifndef KEY_LOGGER_H
#define KEY_LOGGER_H

#include <SDL2/SDL.h>
#include <cstdint>
#include <string>
#include <iostream>
#include <iomanip>
#include <vector>
#include "caneta_sdl.h"

extern "C" {
#include "caneta.h"
}

class KeyLogger {
  public:
    KeyLogger();
    ~KeyLogger();

    // Initialize SDL and create window
    bool init();

    // Main event loop
    void run();

    // Stop the event loop
    void stop();

  private:
    // SDL resources
    SDL_Window* window;
    SDL_Renderer* renderer;
    bool running;

    // Caneta SDL to HID converter
    caneta::SDLToHID keyboard;

    // Keyboard state for caneta-c
    kbd_state_t kbd_state;

    // Process HID report from caneta-sdl
    void processHIDReport(const uint8_t* report, uint16_t len);

    // Display functions
    void printHIDReport(const uint8_t* report, uint16_t len);
    void printKeyPress(uint8_t keycode, bool shift, bool ctrl, bool alt);
    void printSpecialKey(const char* sequence);
    void printASCIIChar(char c);

    // Helper functions
    std::string getKeyName(uint8_t hidCode);
    std::string getModifierString(uint8_t modifiers);
    void clearScreen();
    void renderStatus();
};

#endif