#include "CanetaBluetooth.h"
#include <cstring>
#include <Arduino.h>

// Static instance pointer for callback routing
CanetaBluetooth* CanetaBluetooth::instance_ = nullptr;

CanetaBluetooth::CanetaBluetooth()
    : client_(nullptr)
    , scan_(nullptr)
    , hid_service_(nullptr)
    , hid_report_char_(nullptr)
    , initialized_(false)
    , connected_(false)
    , scanning_(false)
    , key_event_callback_(nullptr)
    , escape_sequence_callback_(nullptr)
    , connection_callback_(nullptr)
{
    memset(device_name_, 0, sizeof(device_name_));
    memset(connected_device_name_, 0, sizeof(connected_device_name_));
    memset(&kbd_state, 0, sizeof(kbd_state));

    // Set static instance pointer for callback routing
    instance_ = this;
}

CanetaBluetooth::~CanetaBluetooth() {
    end();
    if (instance_ == this) {
        instance_ = nullptr;
    }
}

bool CanetaBluetooth::begin(const char* device_name) {
    if (initialized_) {
        return true;
    }

    Serial.println("Initializing BLE HID Host...");

    // Store device name
    strncpy(device_name_, device_name, sizeof(device_name_) - 1);
    device_name_[sizeof(device_name_) - 1] = '\0';

    // Initialize BLE
    BLEDevice::init(device_name);

    // Create BLE client
    client_ = BLEDevice::createClient();
    client_->setClientCallbacks(this);

    // Create BLE scanner
    scan_ = BLEDevice::getScan();
    scan_->setAdvertisedDeviceCallbacks(this);
    scan_->setActiveScan(true);
    scan_->setInterval(100);
    scan_->setWindow(99);

    initialized_ = true;
    Serial.println("BLE HID Host initialized successfully");
    return true;
}

void CanetaBluetooth::end() {
    if (!initialized_) {
        return;
    }

    stopScan();

    if (connected_ && client_) {
        client_->disconnect();
    }

    if (client_) {
        delete client_;
        client_ = nullptr;
    }

    BLEDevice::deinit(false);

    initialized_ = false;
    connected_ = false;
    scanning_ = false;
    hid_service_ = nullptr;
    hid_report_char_ = nullptr;

    memset(connected_device_name_, 0, sizeof(connected_device_name_));
    memset(&kbd_state, 0, sizeof(kbd_state));

    Serial.println("BLE HID Host deinitialized");
}

void CanetaBluetooth::setKeyEventCallback(KeyEventCallback callback) {
    key_event_callback_ = callback;
}

void CanetaBluetooth::setEscapeSequenceCallback(EscapeSequenceCallback callback) {
    escape_sequence_callback_ = callback;
}

void CanetaBluetooth::setConnectionCallback(ConnectionCallback callback) {
    connection_callback_ = callback;
}

bool CanetaBluetooth::startScan(uint32_t duration_seconds) {
    if (!initialized_ || scanning_) {
        return false;
    }

    Serial.printf("Scanning for BLE HID keyboards for %d seconds...\n", duration_seconds);
    Serial.println("Put your keyboard in pairing mode.");

    scanning_ = true;
    scan_->start(duration_seconds, false);
    return true;
}

void CanetaBluetooth::stopScan() {
    if (scanning_) {
        scan_->stop();
        scanning_ = false;
        Serial.println("Scan stopped");
    }
}

void CanetaBluetooth::update() {
    // BLE handles most updates automatically through callbacks
    // This method can be used for periodic maintenance if needed
}

// BLE Client Callbacks
void CanetaBluetooth::onConnect(BLEClient* client) {
    Serial.println("BLE Client connected");
    connected_ = true;

    // Get HID service
    hid_service_ = client->getService(HID_SERVICE_UUID);
    if (hid_service_ == nullptr) {
        Serial.println("Failed to find HID service");
        client->disconnect();
        return;
    }

    // Get HID report characteristic
    hid_report_char_ = hid_service_->getCharacteristic(HID_REPORT_CHAR_UUID);
    if (hid_report_char_ == nullptr) {
        Serial.println("Failed to find HID report characteristic");
        client->disconnect();
        return;
    }

    // Register for notifications
    if (hid_report_char_->canNotify()) {
        hid_report_char_->registerForNotify(hid_report_callback);
        Serial.println("Registered for HID notifications");
    }

    if (connection_callback_) {
        connection_callback_(true, connected_device_name_);
    }
}

void CanetaBluetooth::onDisconnect(BLEClient* client) {
    Serial.println("BLE Client disconnected");

    if (connection_callback_) {
        connection_callback_(false, connected_device_name_);
    }

    connected_ = false;
    hid_service_ = nullptr;
    hid_report_char_ = nullptr;
    memset(connected_device_name_, 0, sizeof(connected_device_name_));
    memset(&kbd_state, 0, sizeof(kbd_state));

    // Restart scanning after disconnect
    if (initialized_) {
        delay(1000); // Brief delay before restarting scan
        startScan(30);
    }
}

// BLE Scan Callbacks
void CanetaBluetooth::onResult(BLEAdvertisedDevice advertisedDevice) {
    Serial.printf("Found device: %s\n", advertisedDevice.toString().c_str());

    // Check if device advertises HID service
    if (advertisedDevice.haveServiceUUID() &&
        advertisedDevice.isAdvertisingService(BLEUUID(HID_SERVICE_UUID))) {

        Serial.println("Found HID keyboard! Attempting connection...");

        // Stop scanning
        scan_->stop();
        scanning_ = false;

        // Store device name
        String name = advertisedDevice.getName();
        if (name.length() == 0) {
            name = "Unknown Keyboard";
        }
        strncpy(connected_device_name_, name.c_str(), sizeof(connected_device_name_) - 1);
        connected_device_name_[sizeof(connected_device_name_) - 1] = '\0';

        // Connect to the keyboard
        if (connect_to_keyboard(advertisedDevice)) {
            Serial.printf("Successfully connected to: %s\n", connected_device_name_);
        } else {
            Serial.println("Failed to connect to keyboard");
            // Restart scanning
            startScan(30);
        }
    }
}

bool CanetaBluetooth::connect_to_keyboard(BLEAdvertisedDevice device) {
    if (!client_ || !initialized_) {
        return false;
    }

    try {
        // Connect to the device
        if (client_->connect(&device)) {
            Serial.println("BLE connection established");
            return true;
        } else {
            Serial.println("BLE connection failed");
            return false;
        }
    } catch (std::exception& e) {
        Serial.printf("Exception during connection: %s\n", e.what());
        return false;
    }
}

// Static callback for HID reports
void CanetaBluetooth::hid_report_callback(BLERemoteCharacteristic* characteristic,
                                         uint8_t* data, size_t length, bool isNotify) {
    if (instance_) {
        instance_->process_keyboard_report(data, length);
    }
}

void CanetaBluetooth::process_keyboard_report(const uint8_t* report, size_t len) {
    if (len < 8) return; // Standard keyboard report is 8 bytes

    // Byte 0: Modifier keys
    uint8_t modifiers = report[0];
    bool shift = (modifiers & 0x22) != 0;  // Left or right shift
    bool ctrl = (modifiers & 0x11) != 0;   // Left or right ctrl
    bool alt = (modifiers & 0x44) != 0;    // Left or right alt

    kbd_state.shift_pressed = shift;
    kbd_state.ctrl_pressed = ctrl;
    kbd_state.alt_pressed = alt;

    // Bytes 2-7: Up to 6 pressed keys
    for (int i = 2; i < 8 && i < len; i++) {
        uint8_t keycode = report[i];
        if (keycode == 0) continue; // No key

        // Check if this is a new key press
        bool was_pressed = false;
        for (int j = 0; j < 6; j++) {
            if (kbd_state.last_keys[j] == keycode) {
                was_pressed = true;
                break;
            }
        }

        if (!was_pressed) {
            // New key press - process it
            process_keycode(keycode, shift, ctrl, alt);
        }
    }

    // Save current keys for next comparison
    memset(kbd_state.last_keys, 0, 6);
    for (int i = 2; i < 8 && i < len; i++) {
        if (report[i] != 0) {
            kbd_state.last_keys[i-2] = report[i];
        }
    }
}

void CanetaBluetooth::process_keycode(uint8_t keycode, bool shift, bool ctrl, bool alt) {
    // Handle special keys first using caneta library
    const char* special_seq = process_special_keys(keycode);
    if (strlen(special_seq) > 0) {
        if (escape_sequence_callback_) {
            escape_sequence_callback_(special_seq);
        }
        return;
    }

    // Handle regular keys using caneta library
    char ascii_char = hid_to_ascii(keycode, shift);

    if (ascii_char != 0) {
        // Handle Ctrl combinations
        if (ctrl && ascii_char >= 'a' && ascii_char <= 'z') {
            ascii_char = ascii_char - 'a' + 1; // Ctrl+A = 0x01, etc.
        } else if (ctrl && ascii_char >= 'A' && ascii_char <= 'Z') {
            ascii_char = ascii_char - 'A' + 1;
        }

        // Send via callback
        if (key_event_callback_) {
            key_event_callback_(ascii_char);
        }
    }
}