//
// Created by Dr. Brandon Wiley on 7/31/25.
//

#ifndef CANETABLUETOOTH_H
#define CANETABLUETOOTH_H

#include <caneta.h>
#include <cstdint>
#include <functional>
#include "BLEDevice.h"
#include "BLEClient.h"
#include "BLEScan.h"
#include "BLEAdvertisedDevice.h"
#include "BLERemoteService.h"
#include "BLERemoteCharacteristic.h"

// HID Service and Characteristic UUIDs
#define HID_SERVICE_UUID        "1812"
#define HID_REPORT_CHAR_UUID    "2A4D"
#define HID_REPORT_MAP_UUID     "2A4B"
#define HID_INFO_UUID           "2A4A"

class CanetaBluetooth : public BLEClientCallbacks, public BLEAdvertisedDeviceCallbacks {
public:
    // Callback function types
    using KeyEventCallback = std::function<void(char ascii_char)>;
    using EscapeSequenceCallback = std::function<void(const char* sequence)>;
    using ConnectionCallback = std::function<void(bool connected, const char* device_name)>;

    CanetaBluetooth();
    ~CanetaBluetooth();

    // Initialize BLE and start scanning
    bool begin(const char* device_name = "ESP32S3-KB-Host");

    // Stop BLE and cleanup
    void end();

    // Set callbacks for key events
    void setKeyEventCallback(KeyEventCallback callback);
    void setEscapeSequenceCallback(EscapeSequenceCallback callback);
    void setConnectionCallback(ConnectionCallback callback);

    // Start/stop scanning for keyboards
    bool startScan(uint32_t duration_seconds = 10);
    void stopScan();

    // Process updates (call in loop)
    void update();

    // Get current connection status
    bool isConnected() const { return connected_; }
    const char* getConnectedDeviceName() const { return device_name_; }

    // Get current keyboard state
    const kbd_state_t& getKeyboardState() const { return kbd_state; }

    // BLE Callbacks
    void onConnect(BLEClient* client) override;
    void onDisconnect(BLEClient* client) override;
    void onResult(BLEAdvertisedDevice advertisedDevice) override;

private:
    // Static callback for HID reports
    static void hid_report_callback(BLERemoteCharacteristic* characteristic,
                                   uint8_t* data, size_t length, bool isNotify);

    // Process HID keyboard report
    void process_keyboard_report(const uint8_t* report, size_t len);

    // Process individual keycode
    void process_keycode(uint8_t keycode, bool shift, bool ctrl, bool alt);

    // Connect to a discovered keyboard
    bool connect_to_keyboard(BLEAdvertisedDevice device);

    // Member variables
    BLEClient* client_;
    BLEScan* scan_;
    BLERemoteService* hid_service_;
    BLERemoteCharacteristic* hid_report_char_;

    bool initialized_;
    bool connected_;
    bool scanning_;
    char device_name_[64];
    char connected_device_name_[64];

    // Callbacks
    KeyEventCallback key_event_callback_;
    EscapeSequenceCallback escape_sequence_callback_;
    ConnectionCallback connection_callback_;

    // Static instance for callback routing
    static CanetaBluetooth* instance_;
};

#endif //CANETABLUETOOTH_H
