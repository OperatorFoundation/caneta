// key_logger.cpp
// Key press event logger implementation

#include "key_logger.h"
#include <sstream>
#include <cstring>

// Global keyboard state for caneta-c
kbd_state_t g_kbd_state = {0};

KeyLogger::KeyLogger() : window(nullptr), renderer(nullptr), running(false) {
    memset(&kbd_state, 0, sizeof(kbd_state));
}

KeyLogger::~KeyLogger() {
    if (renderer) {
        SDL_DestroyRenderer(renderer);
    }
    if (window) {
        SDL_DestroyWindow(window);
    }
    SDL_Quit();
}

bool KeyLogger::init() {
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) < 0) {
        std::cerr << "Failed to initialize SDL: " << SDL_GetError() << std::endl;
        return false;
    }

    // Create window
    window = SDL_CreateWindow(
        "Caneta Key Logger - Press ESC to exit",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        800, 600,
        SDL_WINDOW_SHOWN
    );

    if (!window) {
        std::cerr << "Failed to create window: " << SDL_GetError() << std::endl;
        return false;
    }

    // Create renderer
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        std::cerr << "Failed to create renderer: " << SDL_GetError() << std::endl;
        return false;
    }

    // Set up the keyboard handler
    keyboard.setReportCallback([this](const uint8_t* report, uint16_t len) {
        processHIDReport(report, len);
    });

    // Clear console and print header
    clearScreen();
    std::cout << "==================================" << std::endl;
    std::cout << "    Caneta macOS Key Logger" << std::endl;
    std::cout << "==================================" << std::endl;
    std::cout << "Press keys to see HID codes and VT100 sequences" << std::endl;
    std::cout << "Press ESC or close window to exit" << std::endl;
    std::cout << "----------------------------------" << std::endl << std::endl;

    return true;
}

void KeyLogger::run() {
    running = true;
    SDL_Event event;

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
                break;
            }

            // Check for ESC key to quit
            if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) {
                running = false;
                break;
            }

            // Process keyboard events through caneta-sdl
            keyboard.processEvent(event);
        }

        // Render status in window
        renderStatus();

        SDL_Delay(10);  // Small delay to prevent CPU spinning
    }

    std::cout << "\n----------------------------------" << std::endl;
    std::cout << "Exiting..." << std::endl;
}

void KeyLogger::stop() {
    running = false;
}

void KeyLogger::processHIDReport(const uint8_t* report, uint16_t len) {
    if (len < 8) return;

    // Print raw HID report
    printHIDReport(report, len);

    // Update modifier state
    uint8_t modifiers = report[0];
    kbd_state.shift_pressed = (modifiers & 0x22) != 0;  // Left or right shift
    kbd_state.ctrl_pressed = (modifiers & 0x11) != 0;   // Left or right ctrl
    kbd_state.alt_pressed = (modifiers & 0x44) != 0;    // Left or right alt

    // Check for newly pressed keys
    for (int i = 2; i < 8; i++) {
        uint8_t keycode = report[i];
        if (keycode == 0) continue;

        // Check if this is a new key press
        bool was_pressed = false;
        for (int j = 0; j < 6; j++) {
            if (kbd_state.last_keys[j] == keycode) {
                was_pressed = true;
                break;
            }
        }

        if (!was_pressed) {
            printKeyPress(keycode, kbd_state.shift_pressed,
                         kbd_state.ctrl_pressed, kbd_state.alt_pressed);

            // Process using caneta-c functions
            const char* special_seq = process_special_keys(keycode);
            if (*special_seq) {
                printSpecialKey(special_seq);
            } else {
                char ascii_char = hid_to_ascii(keycode, kbd_state.shift_pressed);
                if (ascii_char != 0) {
                    // Handle Ctrl combinations
                    if (kbd_state.ctrl_pressed && ascii_char >= 'a' && ascii_char <= 'z') {
                        ascii_char = ascii_char - 'a' + 1;
                    } else if (kbd_state.ctrl_pressed && ascii_char >= 'A' && ascii_char <= 'Z') {
                        ascii_char = ascii_char - 'A' + 1;
                    }
                    printASCIIChar(ascii_char);
                }
            }
        }
    }

    // Save current keys for next comparison
    memset(kbd_state.last_keys, 0, 6);
    for (int i = 2; i < 8; i++) {
        if (report[i] != 0) {
            kbd_state.last_keys[i-2] = report[i];
        }
    }
}

void KeyLogger::printHIDReport(const uint8_t* report, uint16_t len) {
    std::cout << "HID Report: ";
    for (int i = 0; i < len; i++) {
        std::cout << std::hex << std::setw(2) << std::setfill('0')
                  << (int)report[i] << " ";
    }
    std::cout << std::dec;

    // Print modifier interpretation
    if (report[0] != 0) {
        std::cout << " [" << getModifierString(report[0]) << "]";
    }

    std::cout << std::endl;
}

void KeyLogger::printKeyPress(uint8_t keycode, bool shift, bool ctrl, bool alt) {
    std::cout << "  Key Press: 0x" << std::hex << std::setw(2) << std::setfill('0')
              << (int)keycode << std::dec;

    std::string keyName = getKeyName(keycode);
    if (!keyName.empty()) {
        std::cout << " (" << keyName << ")";
    }

    if (shift || ctrl || alt) {
        std::cout << " [";
        if (ctrl) std::cout << "Ctrl ";
        if (shift) std::cout << "Shift ";
        if (alt) std::cout << "Alt ";
        std::cout << "]";
    }

    std::cout << std::endl;
}

void KeyLogger::printSpecialKey(const char* sequence) {
    std::cout << "    VT100: ESC" << sequence;

    // Show escape sequence in hex
    std::cout << " (hex: 1B";
    for (const char* p = sequence; *p; p++) {
        std::cout << " " << std::hex << std::setw(2) << std::setfill('0')
                  << (int)(unsigned char)*p;
    }
    std::cout << std::dec << ")" << std::endl;
}

void KeyLogger::printASCIIChar(char c) {
    std::cout << "    ASCII: ";

    if (c >= 32 && c < 127) {
        std::cout << "'" << c << "'";
    } else if (c == '\r') {
        std::cout << "CR";
    } else if (c == '\n') {
        std::cout << "LF";
    } else if (c == '\t') {
        std::cout << "TAB";
    } else if (c == 27) {
        std::cout << "ESC";
    } else if (c < 32) {
        std::cout << "Ctrl+" << (char)('A' + c - 1);
    }

    std::cout << " (0x" << std::hex << std::setw(2) << std::setfill('0')
              << (int)(unsigned char)c << std::dec << ")" << std::endl;
}

std::string KeyLogger::getKeyName(uint8_t hidCode) {
    // Map common HID codes to key names
    if (hidCode >= 0x04 && hidCode <= 0x1D) {
        return std::string(1, 'A' + (hidCode - 0x04));
    }
    if (hidCode >= 0x1E && hidCode <= 0x26) {
        return std::string(1, '1' + (hidCode - 0x1E));
    }
    if (hidCode == 0x27) return "0";

    switch (hidCode) {
        case 0x28: return "Enter";
        case 0x29: return "Escape";
        case 0x2A: return "Backspace";
        case 0x2B: return "Tab";
        case 0x2C: return "Space";
        case 0x2D: return "- _";
        case 0x2E: return "= +";
        case 0x2F: return "[ {";
        case 0x30: return "] }";
        case 0x31: return "\\ |";
        case 0x33: return "; :";
        case 0x34: return "' \"";
        case 0x35: return "` ~";
        case 0x36: return ", <";
        case 0x37: return ". >";
        case 0x38: return "/ ?";
        case 0x39: return "Caps Lock";
        case 0x3A: return "F1";
        case 0x3B: return "F2";
        case 0x3C: return "F3";
        case 0x3D: return "F4";
        case 0x3E: return "F5";
        case 0x3F: return "F6";
        case 0x40: return "F7";
        case 0x41: return "F8";
        case 0x42: return "F9";
        case 0x43: return "F10";
        case 0x44: return "F11";
        case 0x45: return "F12";
        case 0x49: return "Insert";
        case 0x4A: return "Home";
        case 0x4B: return "Page Up";
        case 0x4C: return "Delete";
        case 0x4D: return "End";
        case 0x4E: return "Page Down";
        case 0x4F: return "Right";
        case 0x50: return "Left";
        case 0x51: return "Down";
        case 0x52: return "Up";
        default: return "";
    }
}

std::string KeyLogger::getModifierString(uint8_t modifiers) {
    std::stringstream ss;
    bool first = true;

    if (modifiers & 0x01) { ss << "LCtrl"; first = false; }
    if (modifiers & 0x02) { if (!first) ss << "+"; ss << "LShift"; first = false; }
    if (modifiers & 0x04) { if (!first) ss << "+"; ss << "LAlt"; first = false; }
    if (modifiers & 0x08) { if (!first) ss << "+"; ss << "LCmd"; first = false; }
    if (modifiers & 0x10) { if (!first) ss << "+"; ss << "RCtrl"; first = false; }
    if (modifiers & 0x20) { if (!first) ss << "+"; ss << "RShift"; first = false; }
    if (modifiers & 0x40) { if (!first) ss << "+"; ss << "RAlt"; first = false; }
    if (modifiers & 0x80) { if (!first) ss << "+"; ss << "RCmd"; first = false; }

    return ss.str();
}

void KeyLogger::clearScreen() {
    // ANSI escape code to clear screen
    std::cout << "\033[2J\033[H";
}

void KeyLogger::renderStatus() {
    // Clear the window with dark gray
    SDL_SetRenderDrawColor(renderer, 32, 32, 32, 255);
    SDL_RenderClear(renderer);

    // Draw some status text (would need SDL_ttf for actual text)
    // For now, just show different colors based on modifier state
    int r = 64, g = 64, b = 64;
    if (kbd_state.shift_pressed) g += 64;
    if (kbd_state.ctrl_pressed) r += 64;
    if (kbd_state.alt_pressed) b += 64;

    SDL_Rect rect = {50, 50, 700, 500};
    SDL_SetRenderDrawColor(renderer, r, g, b, 255);
    SDL_RenderFillRect(renderer, &rect);

    SDL_RenderPresent(renderer);
}