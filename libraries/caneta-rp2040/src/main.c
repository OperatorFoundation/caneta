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

#include <caneta.h>

// Manual function declarations for HID functions
extern bool tuh_hid_receive_report(uint8_t dev_addr, uint8_t instance);
extern bool tuh_hid_set_protocol(uint8_t dev_addr, uint8_t instance, uint8_t protocol);

// Global keyboard state (required by caneta.h)
kbd_state_t kbd_state = {0};

// UART configuration
#define UART_ID uart1
#define UART_TX_PIN 20  // GPIO20 - TX only

// USB pins
#define USB_HOST_DP_PIN 4   // GPIO4 for D+

void uart_setup()
{
    uart_init(UART_ID, 115200);
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    uart_set_hw_flow(UART_ID, false, false);
    uart_set_format(UART_ID, 8, 1, UART_PARITY_NONE);
    uart_set_fifo_enabled(UART_ID, false);
}

void send_to_terminal(const char* str)
{
    uart_puts(UART_ID, str);
}

void send_vt100_escape(const char* sequence)
{
    uart_putc_raw(UART_ID, '\x1B');  // ESC
    uart_puts(UART_ID, sequence);
}

void process_hid_report(uint8_t const* report, uint16_t len)
{
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

            // Handle special keys first
            const char* special_seq = process_special_keys(keycode);
            if (*special_seq) {  // If not empty string
                send_vt100_escape(special_seq);
                continue;
            }

            // Handle regular keys using caneta function
            char ascii_char = hid_to_ascii(keycode, shift);

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
void tuh_mount_cb(uint8_t dev_addr)
{
    // Silent connection
    (void)dev_addr;
}

void tuh_umount_cb(uint8_t dev_addr)
{
    // Reset keyboard state on disconnect
    memset(&kbd_state, 0, sizeof(kbd_state));
    (void)dev_addr;
}

void tuh_hid_mount_cb(uint8_t dev_addr, uint8_t instance, uint8_t const* desc_report, uint16_t desc_len)
{
    // Set boot protocol and request reports
    tuh_hid_set_protocol(dev_addr, instance, 0);  // Boot protocol
    tuh_hid_receive_report(dev_addr, instance);

    (void)desc_report;
    (void)desc_len;
}

void tuh_hid_umount_cb(uint8_t dev_addr, uint8_t instance) {
    // Silent disconnection
    (void)dev_addr;
    (void)instance;
}

void tuh_hid_report_received_cb(uint8_t dev_addr, uint8_t instance, uint8_t const* report, uint16_t len)
{
    // Process the HID report and translate to VT100
    process_hid_report(report, len);

    // Request next report
    tuh_hid_receive_report(dev_addr, instance);

    (void)dev_addr;
}

int main()
{
    uart_setup();
    sleep_ms(100);

    // Configure PIO-USB
    pio_usb_configuration_t pio_cfg = PIO_USB_DEFAULT_CONFIG;
    pio_cfg.pin_dp = USB_HOST_DP_PIN;
    pio_cfg.pinout = PIO_USB_PINOUT_DMDP;

    tuh_configure(1, TUH_CFGID_RPI_PIO_USB_CONFIGURATION, &pio_cfg);
    tuh_init(1);

    while(1)
    {
        tuh_task();
        sleep_us(100);
    }

    return 0;
}