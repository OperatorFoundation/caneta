#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/gpio.h"

// PIO-USB includes
#include "pio_usb.h"
#include "tusb.h"
#include "class/hid/hid_host.h"

// Manual function declarations for HID functions that might not be properly declared
extern bool tuh_hid_receive_report(uint8_t dev_addr, uint8_t instance);
extern bool tuh_hid_set_protocol(uint8_t dev_addr, uint8_t instance, uint8_t protocol);

// UART configuration
#define UART_ID uart1
#define UART_TX_PIN 20  // GPIO20 - TX only

// USB pins - YOUR ACTUAL WIRING: D+ on GPIO4, D- on GPIO3
#define USB_HOST_DP_PIN 4   // GPIO4 for D+

void uart_setup() {
    uart_init(UART_ID, 115200);
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    uart_set_hw_flow(UART_ID, false, false);
    uart_set_format(UART_ID, 8, 1, UART_PARITY_NONE);
    uart_set_fifo_enabled(UART_ID, false);
}

void print_to_uart(const char* str) {
    uart_puts(UART_ID, str);
}

// Add debugging for any USB activity
void tuh_mount_cb(uint8_t dev_addr) {
    char msg[64];
    sprintf(msg, "*** DEVICE CONNECTED: addr=%d ***\r\n", dev_addr);
    print_to_uart(msg);
}

void tuh_umount_cb(uint8_t dev_addr) {
    char msg[64];
    sprintf(msg, "*** DEVICE DISCONNECTED: addr=%d ***\r\n", dev_addr);
    print_to_uart(msg);
}

// Add HID support
void tuh_hid_mount_cb(uint8_t dev_addr, uint8_t instance, uint8_t const* desc_report, uint16_t desc_len) {
    char msg[128];
    uint16_t vid, pid;
    tuh_vid_pid_get(dev_addr, &vid, &pid);

    sprintf(msg, "!!! HID DEVICE MOUNTED: addr=%d, instance=%d, VID=%04X, PID=%04X !!!\r\n",
            dev_addr, instance, vid, pid);
    print_to_uart(msg);

    sprintf(msg, "HID Report Descriptor Length: %d bytes\r\n", desc_len);
    print_to_uart(msg);

    // Print some of the report descriptor to see what we're dealing with
    print_to_uart("Report descriptor (first 16 bytes): ");
    for(int i = 0; i < 16 && i < desc_len; i++) {
        sprintf(msg, "%02X ", desc_report[i]);
        print_to_uart(msg);
    }
    print_to_uart("\r\n");

    print_to_uart("HID device ready - requesting reports...\r\n");

    // Now try to request HID reports
    if (tuh_hid_receive_report(dev_addr, instance)) {
        print_to_uart("SUCCESS: HID report request sent\r\n");
    } else {
        print_to_uart("FAILED: Could not request HID reports\r\n");
    }

    // Also try setting boot protocol for keyboards
    if (tuh_hid_set_protocol(dev_addr, instance, 0)) {  // 0 = boot protocol
        print_to_uart("Boot protocol set successfully\r\n");
        // Try requesting report again after setting boot protocol
        if (tuh_hid_receive_report(dev_addr, instance)) {
            print_to_uart("SUCCESS: HID report request sent after boot protocol\r\n");
        }
    }
}

void tuh_hid_umount_cb(uint8_t dev_addr, uint8_t instance) {
    char msg[64];
    sprintf(msg, "!!! HID DEVICE UNMOUNTED: addr=%d, instance=%d !!!\r\n", dev_addr, instance);
    print_to_uart(msg);
}

void tuh_hid_report_received_cb(uint8_t dev_addr, uint8_t instance, uint8_t const* report, uint16_t len) {
    char msg[128];

    sprintf(msg, "!!! KEY PRESS: addr=%d, len=%d, data: ", dev_addr, len);
    print_to_uart(msg);

    for(uint16_t i = 0; i < len && i < 8; i++) {
        sprintf(msg, "%02X ", report[i]);
        print_to_uart(msg);
    }
    print_to_uart(" !!!\r\n");

    // Request the next report to keep data flowing
    tuh_hid_receive_report(dev_addr, instance);
}

void check_gpio_states() {
    // Read the actual GPIO pin states to see if there's any activity
    bool dp_state = gpio_get(4);  // D+
    bool dm_state = gpio_get(3);  // D-

    static bool last_dp = false, last_dm = false;
    static bool first_check = true;

    if (first_check || dp_state != last_dp || dm_state != last_dm) {
        char msg[64];
        sprintf(msg, "GPIO states: D+(GPIO4)=%d, D-(GPIO3)=%d\r\n", dp_state, dm_state);
        print_to_uart(msg);

        last_dp = dp_state;
        last_dm = dm_state;
        first_check = false;
    }
}

int main() {
    uart_setup();
    sleep_ms(100);

    print_to_uart("\r\n=== MINIMAL USB HOST TEST ===\r\n");
    print_to_uart("Trying GPIO4 (D+) and GPIO3 (D-) for USB\r\n");

    // Configure for your actual wiring: D+ on GPIO4, D- on GPIO3
    pio_usb_configuration_t pio_cfg = PIO_USB_DEFAULT_CONFIG;
    pio_cfg.pin_dp = USB_HOST_DP_PIN;      // GPIO4 for D+
    pio_cfg.pinout = PIO_USB_PINOUT_DMDP;  // D- = D+ - 1 = GPIO3

    print_to_uart("Configuring PIO-USB...\r\n");
    bool config_ok = tuh_configure(1, TUH_CFGID_RPI_PIO_USB_CONFIGURATION, &pio_cfg);
    if (config_ok) {
        print_to_uart("PIO-USB configuration: SUCCESS\r\n");
    } else {
        print_to_uart("PIO-USB configuration: FAILED\r\n");
    }

    print_to_uart("Initializing TinyUSB...\r\n");
    bool init_ok = tuh_init(1);
    if (init_ok) {
        print_to_uart("TinyUSB initialization: SUCCESS\r\n");
    } else {
        print_to_uart("TinyUSB initialization: FAILED\r\n");
    }

    // Try the opposite pinout as well
    if (!config_ok || !init_ok) {
        print_to_uart("Trying DPDM pinout (D- on GPIO5)...\r\n");
        pio_cfg.pinout = PIO_USB_PINOUT_DPDM;  // D- = D+ + 1 = GPIO5
        config_ok = tuh_configure(1, TUH_CFGID_RPI_PIO_USB_CONFIGURATION, &pio_cfg);
        if (config_ok) {
            print_to_uart("DPDM configuration: SUCCESS\r\n");
            init_ok = tuh_init(1);
            if (init_ok) {
                print_to_uart("TinyUSB with DPDM: SUCCESS\r\n");
            }
        }
    }

    print_to_uart("USB Host ready! Connect a device.\r\n");
    print_to_uart("Using D+ on GPIO4, D- on GPIO3\r\n");

    // Check if HID driver is enabled
    #ifdef CFG_TUH_HID
    char msg[64];
    sprintf(msg, "HID driver enabled, max devices: %d\r\n", CFG_TUH_HID);
    print_to_uart(msg);
    #else
    print_to_uart("WARNING: HID driver not enabled!\r\n");
    #endif

    print_to_uart("\r\n");

    uint32_t counter = 0;
    uint32_t last_status = 0;

    while (1) {
        tuh_task();
        counter++;

        // Status every 3 seconds
        uint32_t now = to_ms_since_boot(get_absolute_time());
        if (now - last_status > 3000) {
            char msg[64];
            sprintf(msg, "USB host running... (task cycles: %lu)\r\n", counter);
            print_to_uart(msg);

            // Check if any devices are mounted
            bool any_mounted = false;
            for (uint8_t addr = 1; addr < 5; addr++) {
                if (tuh_mounted(addr)) {
                    sprintf(msg, "Device mounted at address %d\r\n", addr);
                    print_to_uart(msg);
                    any_mounted = true;
                }
            }
            if (!any_mounted) {
                print_to_uart("No devices detected\r\n");
            }

            // Check GPIO pin states for any activity
            check_gpio_states();

            last_status = now;
        }

        sleep_us(100);
    }

    return 0;
}