#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/gpio.h"

// PIO-USB includes
#include "pio_usb.h"
#include "tusb.h"
#include "class/hid/hid_host.h"

// Manual function declarations for HID functions
extern bool tuh_hid_receive_report(uint8_t dev_addr, uint8_t instance);
extern bool tuh_hid_set_protocol(uint8_t dev_addr, uint8_t instance, uint8_t protocol);

// UART configuration
#define UART_ID uart1
#define UART_TX_PIN 20  // GPIO20 - TX only

// USB pins
#define USB_HOST_DP_PIN 4   // GPIO4 for D+

// HID scan codes to ASCII mapping (US keyboard layout)
static const char hid_to_ascii[256] = {
    0,   0,   0,   0,   'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l',  // 0x00-0x0F
    'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', '1', '2',  // 0x10-0x1F
    '3', '4', '5', '6', '7', '8', '9', '0', '\r', '\x1B', '\b', '\t', ' ', '-', '=', '[', // 0x20-0x2F
    ']', '\\', 0, ';', '\'', '`', ',', '.', '/', 0, 0, 0, 0, 0, 0, 0,                    // 0x30-0x3F
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,                                    // 0x40-0x4F
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,                                    // 0x50-0x5F
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,                                    // 0x60-0x6F
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,                                    // 0x70-0x7F
    [0x80 ... 0xFF] = 0  // Rest are 0
};

// Shifted characters for uppercase and symbols
static const char hid_to_ascii_shift[256] = {
    0,   0,   0,   0,   'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L',  // 0x00-0x0F
    'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '!', '@',  // 0x10-0x1F
    '#', '$', '%', '^', '&', '*', '(', ')', '\r', '\x1B', '\b', '\t', ' ', '_', '+', '{', // 0x20-0x2F
    '}', '|', 0, ':', '"', '~', '<', '>', '?', 0, 0, 0, 0, 0, 0, 0,                    // 0x30-0x3F
    [0x40 ... 0xFF] = 0  // Rest are 0
};

// Keyboard state
static struct {
    bool shift_pressed;
    bool ctrl_pressed;
    bool alt_pressed;
    uint8_t last_keys[6];
} kbd_state = {0};

void uart_setup() {
    uart_init(UART_ID, 115200);
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    uart_set_hw_flow(UART_ID, false, false);
    uart_set_format(UART_ID, 8, 1, UART_PARITY_NONE);
    uart_set_fifo_enabled(UART_ID, false);
}

void send_to_terminal(const char* str) {
    uart_puts(UART_ID, str);
}

void send_vt100_escape(const char* sequence) {
    uart_putc_raw(UART_ID, '\x1B');  // ESC
    uart_puts(UART_ID, sequence);
}

void process_special_keys(uint8_t keycode) {
    switch(keycode) {
        case 0x4F: // Right Arrow
            send_vt100_escape("[C");
            break;
        case 0x50: // Left Arrow
            send_vt100_escape("[D");
            break;
        case 0x51: // Down Arrow
            send_vt100_escape("[B");
            break;
        case 0x52: // Up Arrow
            send_vt100_escape("[A");
            break;
        case 0x4A: // Home
            send_vt100_escape("[H");
            break;
        case 0x4D: // End
            send_vt100_escape("[F");
            break;
        case 0x4B: // Page Up
            send_vt100_escape("[5~");
            break;
        case 0x4E: // Page Down
            send_vt100_escape("[6~");
            break;
        case 0x49: // Insert
            send_vt100_escape("[2~");
            break;
        case 0x4C: // Delete
            send_vt100_escape("[3~");
            break;
        case 0x3A: // F1
            send_vt100_escape("OP");
            break;
        case 0x3B: // F2
            send_vt100_escape("OQ");
            break;
        case 0x3C: // F3
            send_vt100_escape("OR");
            break;
        case 0x3D: // F4
            send_vt100_escape("OS");
            break;
        case 0x3E: // F5
            send_vt100_escape("[15~");
            break;
        case 0x3F: // F6
            send_vt100_escape("[17~");
            break;
        case 0x40: // F7
            send_vt100_escape("[18~");
            break;
        case 0x41: // F8
            send_vt100_escape("[19~");
            break;
        case 0x42: // F9
            send_vt100_escape("[20~");
            break;
        case 0x43: // F10
            send_vt100_escape("[21~");
            break;
        case 0x44: // F11
            send_vt100_escape("[23~");
            break;
        case 0x45: // F12
            send_vt100_escape("[24~");
            break;
        default:
            // Unknown special key - ignore
            break;
    }
}

void process_hid_report(uint8_t const* report, uint16_t len) {
    if (len < 8) return;  // Standard keyboard report is 8 bytes

    // Byte 0: Modifier keys
    uint8_t modifiers = report[0];
    bool shift = (modifiers & 0x22) != 0;  // Left or right shift
    bool ctrl = (modifiers & 0x11) != 0;   // Left or right ctrl
    bool alt = (modifiers & 0x44) != 0;    // Left or right alt

    kbd_state.shift_pressed = shift;
    kbd_state.ctrl_pressed = ctrl;
    kbd_state.alt_pressed = alt;

    // Byte 1: Reserved (always 0)
    // Bytes 2-7: Up to 6 pressed keys

    // Check for newly pressed keys
    for (int i = 2; i < 8; i++) {
        uint8_t keycode = report[i];
        if (keycode == 0) continue;  // No key

        // Check if this is a new key press (not in last report)
        bool was_pressed = false;
        for (int j = 0; j < 6; j++) {
            if (kbd_state.last_keys[j] == keycode) {
                was_pressed = true;
                break;
            }
        }

        if (!was_pressed) {
            // New key press - process it
            char ascii_char = 0;

            // Handle special keys first
            if (keycode >= 0x3A) {  // Function keys and arrows
                process_special_keys(keycode);
                continue;
            }

            // Handle regular keys
            if (shift) {
                ascii_char = hid_to_ascii_shift[keycode];
            } else {
                ascii_char = hid_to_ascii[keycode];
            }

            if (ascii_char != 0) {
                // Handle Ctrl combinations
                if (ctrl && ascii_char >= 'a' && ascii_char <= 'z') {
                    ascii_char = ascii_char - 'a' + 1;  // Ctrl+A = 0x01, etc.
                } else if (ctrl && ascii_char >= 'A' && ascii_char <= 'Z') {
                    ascii_char = ascii_char - 'A' + 1;
                }

                // Send the character
                uart_putc_raw(UART_ID, ascii_char);
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

// USB callbacks
void tuh_mount_cb(uint8_t dev_addr) {
    char msg[64];
    sprintf(msg, "USB device connected (addr=%d)\r\n", dev_addr);
    send_to_terminal(msg);
}

void tuh_umount_cb(uint8_t dev_addr) {
    char msg[64];
    sprintf(msg, "USB device disconnected (addr=%d)\r\n", dev_addr);
    send_to_terminal(msg);

    // Reset keyboard state
    memset(&kbd_state, 0, sizeof(kbd_state));
}

void tuh_hid_mount_cb(uint8_t dev_addr, uint8_t instance, uint8_t const* desc_report, uint16_t desc_len) {
    char msg[64];
    sprintf(msg, "Keyboard connected (addr=%d)\r\n", dev_addr);
    send_to_terminal(msg);

    // Set boot protocol and request reports
    tuh_hid_set_protocol(dev_addr, instance, 0);  // Boot protocol
    tuh_hid_receive_report(dev_addr, instance);

    (void)desc_report;
    (void)desc_len;
}

void tuh_hid_umount_cb(uint8_t dev_addr, uint8_t instance) {
    char msg[64];
    sprintf(msg, "Keyboard disconnected (addr=%d)\r\n", dev_addr);
    send_to_terminal(msg);
    (void)instance;
}

void tuh_hid_report_received_cb(uint8_t dev_addr, uint8_t instance, uint8_t const* report, uint16_t len) {
    // Process the HID report and translate to VT100
    process_hid_report(report, len);

    // Request next report
    tuh_hid_receive_report(dev_addr, instance);

    (void)dev_addr;
}

int main() {
    uart_setup();
    sleep_ms(100);

    send_to_terminal("\r\n=== USB HID to VT100 Terminal ===\r\n");
    send_to_terminal("Connect a USB keyboard to start typing\r\n");

    // Configure PIO-USB
    pio_usb_configuration_t pio_cfg = PIO_USB_DEFAULT_CONFIG;
    pio_cfg.pin_dp = USB_HOST_DP_PIN;
    pio_cfg.pinout = PIO_USB_PINOUT_DMDP;

    tuh_configure(1, TUH_CFGID_RPI_PIO_USB_CONFIGURATION, &pio_cfg);
    tuh_init(1);

    send_to_terminal("USB host initialized\r\n\r\n");

    while (1) {
        tuh_task();
        sleep_us(100);
    }

    return 0;
}