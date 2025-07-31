/*
 * ESP32S3 Bluetooth Keyboard Host
 * Main Arduino sketch using CanetaBluetooth class
 * Outputs VT100 sequences via hardware UART
 */

#include "CanetaBluetooth.h"

// Hardware UART configuration
#define UART_TX_PIN 17
#define UART_RX_PIN 18
#define UART_BAUD 115200

CanetaBluetooth keyboard;

void setup() {
    Serial.begin(115200);
    Serial.println("ESP32S3 Bluetooth Keyboard Host Starting...");

    // Configure hardware UART for terminal output
    Serial1.begin(UART_BAUD, SERIAL_8N1, UART_RX_PIN, UART_TX_PIN);

    // Initialize Bluetooth keyboard handler
    if (!keyboard.begin("ESP32S3-KB-Host")) {
        Serial.println("Failed to initialize Bluetooth keyboard");
        return;
    }

    // Set up callbacks
    keyboard.setKeyEventCallback([](char ascii_char) {
        // Send regular characters to UART
        Serial1.write(ascii_char);
        Serial.printf("Key: 0x%02X ('%c')\n", ascii_char,
                     (ascii_char >= 32 && ascii_char <= 126) ? ascii_char : '?');
    });

    keyboard.setEscapeSequenceCallback([](const char* sequence) {
        // Send VT100 escape sequences to UART
        Serial1.write('\x1B'); // ESC
        Serial1.print(sequence);
        Serial.printf("Escape: ESC%s\n", sequence);
    });

    keyboard.setConnectionCallback([](bool connected, const char* device_name) {
        if (connected) {
            Serial.printf("✓ Connected to keyboard: %s\n", device_name);
        } else {
            Serial.printf("✗ Disconnected from keyboard: %s\n", device_name);
            Serial.println("Will automatically scan for new keyboards...");
        }
    });

    // Start scanning for BLE keyboards
    Serial.println("Starting scan for BLE HID keyboards...");
    Serial.println("Put your Bluetooth keyboard in pairing mode.");
    keyboard.startScan(30); // Scan for 30 seconds initially
}

void loop() {
    static unsigned long last_status = 0;

    // Update Bluetooth keyboard handler (important!)
    keyboard.update();

    // Print status every 10 seconds
    if (millis() - last_status > 10000) {
        last_status = millis();

        if (keyboard.isConnected()) {
            Serial.printf("Status: Connected to %s\n", keyboard.getConnectedDeviceName());

            // Print current modifier state
            const auto& state = keyboard.getKeyboardState();
            if (state.shift_pressed || state.ctrl_pressed || state.alt_pressed) {
                Serial.printf("Modifiers: %s%s%s\n",
                             state.shift_pressed ? "SHIFT " : "",
                             state.ctrl_pressed ? "CTRL " : "",
                             state.alt_pressed ? "ALT " : "");
            }
        } else {
            Serial.println("Status: No keyboard connected - waiting for pairing");
        }
    }

    // Small delay to prevent watchdog issues
    delay(10);
}