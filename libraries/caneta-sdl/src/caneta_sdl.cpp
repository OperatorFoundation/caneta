// caneta-sdl.cpp
// SDL2 to USB HID keycode bridge implementation

#include "caneta_sdl.h"

namespace caneta
{
    // USB HID Usage Tables - Keyboard/Keypad Page (0x07)
    // These match the standard USB HID keycodes used by your RP2040 code

    SDLToHID::SDLToHID() : keyCount(0) {
        memset(currentReport, 0, sizeof(currentReport));
        memset(pressedKeys, 0, sizeof(pressedKeys));
    }

    SDLToHID::~SDLToHID() {
    }

    void SDLToHID::setReportCallback(HIDReportCallback callback) {
        reportCallback = callback;
    }

    void SDLToHID::processEvent(const SDL_Event& event) {
        switch (event.type) {
            case SDL_KEYDOWN:
                if (!event.key.repeat) {  // Ignore key repeat events
                    handleKeyDown(event.key);
                }
                break;
            case SDL_KEYUP:
                handleKeyUp(event.key);
                break;
        }
    }

    void SDLToHID::handleKeyDown(const SDL_KeyboardEvent& key) {
        uint8_t hidCode = sdlToHIDKeycode(key.keysym.sym);

        if (hidCode != 0) {
            addKey(hidCode);
            updateReport();
            sendReport();
        }
    }

    void SDLToHID::handleKeyUp(const SDL_KeyboardEvent& key) {
        uint8_t hidCode = sdlToHIDKeycode(key.keysym.sym);

        if (hidCode != 0) {
            removeKey(hidCode);
            updateReport();
            sendReport();
        }
    }

    void SDLToHID::addKey(uint8_t hidCode) {
        // Check if key is already pressed
        for (int i = 0; i < keyCount; i++) {
            if (pressedKeys[i] == hidCode) {
                return;  // Already pressed
            }
        }

        // Add key if there's room
        if (keyCount < 6) {
            pressedKeys[keyCount++] = hidCode;
        }
    }

    void SDLToHID::removeKey(uint8_t hidCode) {
        // Find and remove the key
        for (int i = 0; i < keyCount; i++) {
            if (pressedKeys[i] == hidCode) {
                // Shift remaining keys
                for (int j = i; j < keyCount - 1; j++) {
                    pressedKeys[j] = pressedKeys[j + 1];
                }
                keyCount--;
                pressedKeys[keyCount] = 0;
                break;
            }
        }
    }

    void SDLToHID::updateReport() {
        // Clear the report
        memset(currentReport, 0, sizeof(currentReport));

        // Byte 0: Modifiers
        currentReport[0] = getHIDModifiers(SDL_GetModState());

        // Byte 1: Reserved (always 0)

        // Bytes 2-7: Up to 6 pressed keys
        for (int i = 0; i < keyCount && i < 6; i++) {
            currentReport[i + 2] = pressedKeys[i];
        }
    }

    void SDLToHID::sendReport() {
        if (reportCallback) {
            reportCallback(currentReport, 8);
        }
    }

    uint8_t SDLToHID::getHIDModifiers(uint16_t sdlMod) {
        uint8_t hidMod = 0;

        if (sdlMod & KMOD_LCTRL)  hidMod |= 0x01;  // Left Control
        if (sdlMod & KMOD_LSHIFT) hidMod |= 0x02;  // Left Shift
        if (sdlMod & KMOD_LALT)   hidMod |= 0x04;  // Left Alt
        if (sdlMod & KMOD_LGUI)   hidMod |= 0x08;  // Left GUI (Command on Mac)
        if (sdlMod & KMOD_RCTRL)  hidMod |= 0x10;  // Right Control
        if (sdlMod & KMOD_RSHIFT) hidMod |= 0x20;  // Right Shift
        if (sdlMod & KMOD_RALT)   hidMod |= 0x40;  // Right Alt
        if (sdlMod & KMOD_RGUI)   hidMod |= 0x80;  // Right GUI (Command on Mac)

        return hidMod;
    }

    uint8_t SDLToHID::sdlToHIDKeycode(SDL_Keycode sdlKey) {
        // USB HID Keyboard Usage Table conversions
        // Based on USB HID Usage Tables v1.12, Section 10 (Keyboard/Keypad)

        // Letters A-Z (HID: 0x04-0x1D)
        if (sdlKey >= SDLK_a && sdlKey <= SDLK_z) {
            return 0x04 + (sdlKey - SDLK_a);
        }

        // Numbers 1-9 (HID: 0x1E-0x26)
        if (sdlKey >= SDLK_1 && sdlKey <= SDLK_9) {
            return 0x1E + (sdlKey - SDLK_1);
        }

        // Special number keys
        if (sdlKey == SDLK_0) return 0x27;  // 0

        // Common keys
        switch (sdlKey)
        {
            case SDLK_RETURN:     return 0x28;  // Enter
            case SDLK_ESCAPE:     return 0x29;  // Escape
            case SDLK_BACKSPACE:  return 0x2A;  // Backspace
            case SDLK_TAB:        return 0x2B;  // Tab
            case SDLK_SPACE:      return 0x2C;  // Space
            case SDLK_MINUS:      return 0x2D;  // - _
            case SDLK_EQUALS:     return 0x2E;  // = +
            case SDLK_LEFTBRACKET: return 0x2F; // [ {
            case SDLK_RIGHTBRACKET: return 0x30; // ] }
            case SDLK_BACKSLASH:  return 0x31;  // \ |
            case SDLK_SEMICOLON:  return 0x33;  // ; :
            case SDLK_QUOTE:      return 0x34;  // ' "
            case SDLK_BACKQUOTE:  return 0x35;  // ` ~
            case SDLK_COMMA:      return 0x36;  // , <
            case SDLK_PERIOD:     return 0x37;  // . >
            case SDLK_SLASH:      return 0x38;  // / ?
            case SDLK_CAPSLOCK:   return 0x39;  // Caps Lock

                // Function keys F1-F12 (HID: 0x3A-0x45)
            case SDLK_F1:  return 0x3A;
            case SDLK_F2:  return 0x3B;
            case SDLK_F3:  return 0x3C;
            case SDLK_F4:  return 0x3D;
            case SDLK_F5:  return 0x3E;
            case SDLK_F6:  return 0x3F;
            case SDLK_F7:  return 0x40;
            case SDLK_F8:  return 0x41;
            case SDLK_F9:  return 0x42;
            case SDLK_F10: return 0x43;
            case SDLK_F11: return 0x44;
            case SDLK_F12: return 0x45;

                // Control keys
            case SDLK_PRINTSCREEN: return 0x46;  // Print Screen
            case SDLK_SCROLLLOCK:  return 0x47;  // Scroll Lock
            case SDLK_PAUSE:       return 0x48;  // Pause
            case SDLK_INSERT:      return 0x49;  // Insert
            case SDLK_HOME:        return 0x4A;  // Home
            case SDLK_PAGEUP:      return 0x4B;  // Page Up
            case SDLK_DELETE:      return 0x4C;  // Delete
            case SDLK_END:         return 0x4D;  // End
            case SDLK_PAGEDOWN:    return 0x4E;  // Page Down

                // Arrow keys
            case SDLK_RIGHT:       return 0x4F;  // Right Arrow
            case SDLK_LEFT:        return 0x50;  // Left Arrow
            case SDLK_DOWN:        return 0x51;  // Down Arrow
            case SDLK_UP:          return 0x52;  // Up Arrow

                // Keypad keys (when Num Lock is active)
            case SDLK_KP_DIVIDE:   return 0x54;  // Keypad /
            case SDLK_KP_MULTIPLY: return 0x55;  // Keypad *
            case SDLK_KP_MINUS:    return 0x56;  // Keypad -
            case SDLK_KP_PLUS:     return 0x57;  // Keypad +
            case SDLK_KP_ENTER:    return 0x58;  // Keypad Enter
            case SDLK_KP_1:        return 0x59;  // Keypad 1
            case SDLK_KP_2:        return 0x5A;  // Keypad 2
            case SDLK_KP_3:        return 0x5B;  // Keypad 3
            case SDLK_KP_4:        return 0x5C;  // Keypad 4
            case SDLK_KP_5:        return 0x5D;  // Keypad 5
            case SDLK_KP_6:        return 0x5E;  // Keypad 6
            case SDLK_KP_7:        return 0x5F;  // Keypad 7
            case SDLK_KP_8:        return 0x60;  // Keypad 8
            case SDLK_KP_9:        return 0x61;  // Keypad 9
            case SDLK_KP_0:        return 0x62;  // Keypad 0
            case SDLK_KP_PERIOD:   return 0x63;  // Keypad .

                // Additional keys
            case SDLK_APPLICATION: return 0x65;  // Application (Menu) key
            case SDLK_NUMLOCKCLEAR: return 0x53; // Num Lock

                // Modifier keys (these are handled separately in modifiers byte)
                // But some apps might want to track them as regular keys too
            case SDLK_LCTRL:       return 0xE0;  // Left Control
            case SDLK_LSHIFT:      return 0xE1;  // Left Shift
            case SDLK_LALT:        return 0xE2;  // Left Alt
            case SDLK_LGUI:        return 0xE3;  // Left GUI (Command)
            case SDLK_RCTRL:       return 0xE4;  // Right Control
            case SDLK_RSHIFT:      return 0xE5;  // Right Shift
            case SDLK_RALT:        return 0xE6;  // Right Alt
            case SDLK_RGUI:        return 0xE7;  // Right GUI (Command)

            default:
                return 0;  // Unknown key
        }
    }
}