//
// Created by Dr. Brandon Wiley on 7/31/25.
//

#include "caneta.h"
#include <string.h>

kbd_state_t kbd_state = {0};

char hid_to_ascii(uint8_t keycode, bool shift) {
    if (shift) {
        switch (keycode) {
            case 0x04: return 'A'; case 0x05: return 'B'; case 0x06: return 'C'; case 0x07: return 'D';
            case 0x08: return 'E'; case 0x09: return 'F'; case 0x0A: return 'G'; case 0x0B: return 'H';
            case 0x0C: return 'I'; case 0x0D: return 'J'; case 0x0E: return 'K'; case 0x0F: return 'L';
            case 0x10: return 'M'; case 0x11: return 'N'; case 0x12: return 'O'; case 0x13: return 'P';
            case 0x14: return 'Q'; case 0x15: return 'R'; case 0x16: return 'S'; case 0x17: return 'T';
            case 0x18: return 'U'; case 0x19: return 'V'; case 0x1A: return 'W'; case 0x1B: return 'X';
            case 0x1C: return 'Y'; case 0x1D: return 'Z';
            case 0x1E: return '!'; case 0x1F: return '@'; case 0x20: return '#'; case 0x21: return '$';
            case 0x22: return '%'; case 0x23: return '^'; case 0x24: return '&'; case 0x25: return '*';
            case 0x26: return '('; case 0x27: return ')';
            case 0x28: return '\r'; case 0x29: return '\x1B'; case 0x2A: return '\b'; case 0x2B: return '\t';
            case 0x2C: return ' ';
            case 0x2D: return '_'; case 0x2E: return '+'; case 0x2F: return '{'; case 0x30: return '}';
            case 0x31: return '|'; case 0x33: return ':'; case 0x34: return '"'; case 0x35: return '~';
            case 0x36: return '<'; case 0x37: return '>'; case 0x38: return '?';
            default: return 0;
        }
    } else {
        switch (keycode) {
            case 0x04: return 'a'; case 0x05: return 'b'; case 0x06: return 'c'; case 0x07: return 'd';
            case 0x08: return 'e'; case 0x09: return 'f'; case 0x0A: return 'g'; case 0x0B: return 'h';
            case 0x0C: return 'i'; case 0x0D: return 'j'; case 0x0E: return 'k'; case 0x0F: return 'l';
            case 0x10: return 'm'; case 0x11: return 'n'; case 0x12: return 'o'; case 0x13: return 'p';
            case 0x14: return 'q'; case 0x15: return 'r'; case 0x16: return 's'; case 0x17: return 't';
            case 0x18: return 'u'; case 0x19: return 'v'; case 0x1A: return 'w'; case 0x1B: return 'x';
            case 0x1C: return 'y'; case 0x1D: return 'z';
            case 0x1E: return '1'; case 0x1F: return '2'; case 0x20: return '3'; case 0x21: return '4';
            case 0x22: return '5'; case 0x23: return '6'; case 0x24: return '7'; case 0x25: return '8';
            case 0x26: return '9'; case 0x27: return '0';
            case 0x28: return '\r'; case 0x29: return '\x1B'; case 0x2A: return '\b'; case 0x2B: return '\t';
            case 0x2C: return ' ';
            case 0x2D: return '-'; case 0x2E: return '='; case 0x2F: return '['; case 0x30: return ']';
            case 0x31: return '\\'; case 0x33: return ';'; case 0x34: return '\''; case 0x35: return '`';
            case 0x36: return ','; case 0x37: return '.'; case 0x38: return '/';
            default: return 0;
        }
    }
}

const char* process_special_keys(uint8_t keycode) {
    switch(keycode) {
        case 0x4F: // Right Arrow
            return "[C";
        case 0x50: // Left Arrow
            return "[D";
        case 0x51: // Down Arrow
            return "[B";
        case 0x52: // Up Arrow
            return "[A";
        case 0x4A: // Home
            return "[H";
        case 0x4D: // End
            return "[F";
        case 0x4B: // Page Up
            return "[5~";
        case 0x4E: // Page Down
            return "[6~";
        case 0x49: // Insert
            return "[2~";
        case 0x4C: // Delete
            return "[3~";
        case 0x3A: // F1
            return "OP";
        case 0x3B: // F2
            return "OQ";
        case 0x3C: // F3
            return "OR";
        case 0x3D: // F4
            return "OS";
        case 0x3E: // F5
            return "[15~";
        case 0x3F: // F6
            return "[17~";
        case 0x40: // F7
            return "[18~";
        case 0x41: // F8
            return "[19~";
        case 0x42: // F9
            return "[20~";
        case 0x43: // F10
            return "[21~";
        case 0x44: // F11
            return "[23~";
        case 0x45: // F12
            return "[24~";
        default:
            // Unknown special key - return empty string
            return "";
    }
}