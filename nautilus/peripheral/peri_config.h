#pragma once
#include <ArduinoJson.h>
#include <SD.h>
#include <FS.h>
#include <vector>
#include "utilities.h"

#define CONFIG_FILE_PATH "/nautilus.json"
#define CONFIG_VERSION "1.0"

// Configuration structure with hierarchical organization
struct NautilusConfig {
    // WS2812 LED Settings
    struct {
        uint32_t color;
        uint8_t brightness;
        uint8_t mode;
    } ws2812;

    // WiFi Settings
    struct {
        char ssid[64];
        char password[64];

        // Deauth Hunter settings
        struct {
            int rssi_scale_dbm;        // RSSI scale (default -20)
            int packet_threshold;      // Packet count threshold (default 13)
        } deauth;

        // PineAP Hunter settings
        struct {
            int rssi_scale_dbm;        // RSSI scale (default -20)
        } pineap;

        // Portal settings
        struct {
            char ssid[64];             // Portal AP SSID (default "Nautilus WiFi")
        } portal;
    } wifi;

    // SubGHz Radio Settings
    struct {
        float last_frequency;
        std::vector<float> custom_frequencies;

        // Modulation preset (am270, am650, fm238, fm476)
        char modulation[8];

        // Raw Record settings
        struct {
            float last_frequency;
        } raw;

        // Scan/Record settings
        struct {
            int threshold;           // RSSI threshold in dBm
            char type[8];           // "single", "band", "custom"
            char range[16];         // Frequency or range (e.g., "315.000", "full", "433-435")
        } scan;
    } subghz;

    // Display Settings
    struct {
        uint8_t rotation;
        uint8_t theme;
    } display;

    // Audio Settings
    struct {
        uint8_t volume;
    } audio;

    // Constructor with defaults
    NautilusConfig() {
        // WS2812 defaults
        ws2812.color = 0x00FF00;      // Green
        ws2812.brightness = 5;        // Low brightness (~2%)
        ws2812.mode = 0;              // Static color

        // WiFi defaults
        wifi.ssid[0] = '\0';
        wifi.password[0] = '\0';
        wifi.deauth.rssi_scale_dbm = -20;
        wifi.deauth.packet_threshold = 13;
        wifi.pineap.rssi_scale_dbm = -20;
        strcpy(wifi.portal.ssid, "Nautilus WiFi");

        // SubGHz defaults
        subghz.last_frequency = 433.92f;  // 433.92 MHz
        subghz.custom_frequencies.clear();
        strcpy(subghz.modulation, "am650");
        subghz.raw.last_frequency = 315.00f;
        subghz.scan.threshold = -65;
        strcpy(subghz.scan.type, "band");
        strcpy(subghz.scan.range, "full");

        // Display defaults
        display.rotation = 1;         // Landscape
        display.theme = 0;            // Default theme

        // Audio defaults
        audio.volume = 15;            // Medium volume
    }
};

// Global config instance
extern NautilusConfig g_config;

// Functions
bool config_load();                    // Load from SD card
bool config_save();                    // Save to SD card
bool config_create_example();          // Create fully-populated example nautilus.json
void config_set_defaults();            // Set default values
void config_print();                   // Debug print
bool config_sd_available();            // Check if SD card is available

// Convenience functions for auto-saving SubGHz settings
void config_save_raw_frequency(float freq_mhz);
void config_save_scan_settings(const char* type, const char* range, int threshold);
void config_save_modulation(const char* mod);
