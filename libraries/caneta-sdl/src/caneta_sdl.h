// caneta-sdl.h
// SDL2 to USB HID keycode bridge for use with caneta-c library

#ifndef CANETA_SDL_H
#define CANETA_SDL_H

#include <SDL2/SDL.h>
#include <stdint.h>
#include <stdbool.h>
#include <functional>

#ifdef __cplusplus
extern "C" {
#endif
#include <caneta.h>
#ifdef __cplusplus
}
#endif

namespace caneta {

  class SDLToHID {
    public:
      // Callback type - mimics the HID report format
      // report[0] = modifiers, report[2-7] = up to 6 pressed keys
      using HIDReportCallback = std::function<void(const uint8_t* report, uint16_t len)>;

      SDLToHID();
      ~SDLToHID();

      // Set the callback for HID reports
      void setReportCallback(HIDReportCallback callback);

      // Process SDL keyboard events
      void processEvent(const SDL_Event& event);

      // Convert SDL keycode to USB HID keycode
      static uint8_t sdlToHIDKeycode(SDL_Keycode sdlKey);

      // Get current modifier state as HID modifier byte
      static uint8_t getHIDModifiers(uint16_t sdlMod);

    private:
      HIDReportCallback reportCallback;
      uint8_t currentReport[8];  // Standard HID keyboard report

      void handleKeyDown(const SDL_KeyboardEvent& key);
      void handleKeyUp(const SDL_KeyboardEvent& key);
      void sendReport();
      void updateReport();

      // Track pressed keys (up to 6 simultaneously)
      uint8_t pressedKeys[6];
      uint8_t keyCount;

      // Add/remove key from pressed keys array
      void addKey(uint8_t hidCode);
      void removeKey(uint8_t hidCode);
  };

} // namespace caneta

#endif // CANETA_SDL_H