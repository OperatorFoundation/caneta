//
// Created by Dr. Brandon Wiley on 7/31/25.
//

#ifndef CANETA_H
#define CANETA_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include <stdbool.h>

// Convert HID scan code to ASCII character (US keyboard layout)
char hid_to_ascii(uint8_t keycode, bool shift);

// Process special keys and return escape sequences
// Returns empty string ("") if keycode is not a special key
const char* process_special_keys(uint8_t keycode);

// Keyboard state
typedef struct {
  bool shift_pressed;
  bool ctrl_pressed;
  bool alt_pressed;
  uint8_t last_keys[6];
} kbd_state_t;

// Global keyboard state (you'll need to define this in your main code)
extern kbd_state_t kbd_state;

#ifdef __cplusplus
}
#endif

#endif //CANETA_H