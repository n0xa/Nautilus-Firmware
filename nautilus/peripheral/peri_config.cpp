#include "peri_config.h"
#include "rf_utils.h"
#include <cmath>

NautilusConfig g_config;

bool config_sd_available() {
    return SD.begin(BOARD_SD_CS);
}

bool config_load() {
    if (!config_sd_available()) {
        // Silently return if SD card is not available
        config_set_defaults();
        return false;
    }

    File file = SD.open(CONFIG_FILE_PATH, FILE_READ);
    if (!file) {
        // Config file doesn't exist - create example config on first boot
        Serial.println("[Config] nautilus.json not found, creating example config...");
        config_set_defaults();
        if (config_create_example()) {
            Serial.println("[Config] Created example nautilus.json successfully");
            // Now load the newly created config
            file = SD.open(CONFIG_FILE_PATH, FILE_READ);
            if (!file) {
                Serial.println("[Config] Failed to open newly created config");
                return false;
            }
        } else {
            Serial.println("[Config] Failed to create example config, using defaults");
            return false;
        }
    }

    // Allocate JSON document (adjust size as needed)
    JsonDocument doc;

    // Deserialize
    DeserializationError error = deserializeJson(doc, file);
    file.close();

    if (error) {
        config_set_defaults();
        return false;
    }

    // Load WS2812 settings
    // Support both hex string ("0xFF0000", "#FF0000") and decimal number formats
    if (doc["ws2812"]["color"].is<const char*>()) {
        const char* color_str = doc["ws2812"]["color"];
        // Skip leading '0x' or '#' if present
        if (color_str[0] == '0' && (color_str[1] == 'x' || color_str[1] == 'X')) {
            color_str += 2;
        } else if (color_str[0] == '#') {
            color_str += 1;
        }
        g_config.ws2812.color = strtoul(color_str, NULL, 16);
    } else {
        g_config.ws2812.color = doc["ws2812"]["color"] | 0xFF0000;
    }
    g_config.ws2812.brightness = doc["ws2812"]["brightness"] | 128;
    g_config.ws2812.mode = doc["ws2812"]["mode"] | 0;

    // Load WiFi settings
    strlcpy(g_config.wifi.ssid, doc["wifi"]["ssid"] | "", sizeof(g_config.wifi.ssid));
    strlcpy(g_config.wifi.password, doc["wifi"]["password"] | "", sizeof(g_config.wifi.password));
    g_config.wifi.deauth.rssi_scale_dbm = doc["wifi"]["deauth"]["rssi_scale"] | -20;
    g_config.wifi.deauth.packet_threshold = doc["wifi"]["deauth"]["threshold"] | 13;
    g_config.wifi.pineap.rssi_scale_dbm = doc["wifi"]["pineap"]["rssi_scale"] | -20;
    strlcpy(g_config.wifi.portal.ssid, doc["wifi"]["portal"]["ssid"] | "Nautilus WiFi", sizeof(g_config.wifi.portal.ssid));

    // Load SubGHz settings
    g_config.subghz.last_frequency = doc["subghz"]["last_frequency"] | 433.92f;
    JsonArray custom_freqs = doc["subghz"]["custom_frequencies"];
    g_config.subghz.custom_frequencies.clear();

    // Load custom frequencies with deduplication
    // (removes duplicates of hardcoded frequencies and within custom list)
    size_t original_count = custom_freqs.size();

    for (JsonVariant v : custom_freqs) {
        float freq = v.as<float>();
        bool is_duplicate = false;

        // Check against hardcoded frequency list (56 frequencies)
        for (int i = 0; i < 56; i++) {
            if (fabs(subghz_frequency_list[i] - freq) < 0.01f) {
                is_duplicate = true;
                break;
            }
        }

        // Check against already-loaded custom frequencies
        if (!is_duplicate) {
            for (float existing : g_config.subghz.custom_frequencies) {
                if (fabs(existing - freq) < 0.01f) {
                    is_duplicate = true;
                    break;
                }
            }
        }

        // Only add if not a duplicate
        if (!is_duplicate) {
            g_config.subghz.custom_frequencies.push_back(freq);
        }
    }
    bool needs_cleanup = (g_config.subghz.custom_frequencies.size() != original_count);
    if (needs_cleanup) {
        Serial.printf("[Config] Loaded %d custom frequencies (%d duplicates removed)\n",
                      g_config.subghz.custom_frequencies.size(),
                      original_count - g_config.subghz.custom_frequencies.size());
    } else if (original_count > 0) {
        Serial.printf("[Config] Loaded %d custom frequencies\n", g_config.subghz.custom_frequencies.size());
    }

    // Invalidate frequency cache since custom frequencies may have changed
    rf_invalidate_frequency_cache();

    // Load modulation preset
    strlcpy(g_config.subghz.modulation, doc["subghz"]["mod"] | "am650", sizeof(g_config.subghz.modulation));

    // Load Raw Record settings
    g_config.subghz.raw.last_frequency = doc["subghz"]["raw"]["last_frequency"] | 315.00f;

    // Load Scan/Record settings
    g_config.subghz.scan.threshold = doc["subghz"]["scan"]["thresh"] | -65;
    strlcpy(g_config.subghz.scan.type, doc["subghz"]["scan"]["type"] | "band", sizeof(g_config.subghz.scan.type));
    strlcpy(g_config.subghz.scan.range, doc["subghz"]["scan"]["range"] | "full", sizeof(g_config.subghz.scan.range));

    // Load Display settings
    g_config.display.rotation = doc["display"]["rotation"] | 1;
    g_config.display.theme = doc["display"]["theme"] | 0;

    // Load Audio settings
    g_config.audio.volume = doc["audio"]["volume"] | 15;

    config_print();

    // Auto-save if we cleaned up duplicate frequencies
    if (needs_cleanup) {
        Serial.println("Removed duplicate custom frequencies - saving cleaned config...");
        config_save();
    }

    return true;
}

bool config_save() {
    if (!config_sd_available()) {
        return false;
    }

    JsonDocument doc;

    // Build hierarchical JSON structure
    doc["version"] = CONFIG_VERSION;

    // WS2812 settings
    // Save color as hex string for better readability
    char color_hex[10];
    snprintf(color_hex, sizeof(color_hex), "0x%06X", g_config.ws2812.color & 0xFFFFFF);
    doc["ws2812"]["color"] = color_hex;
    doc["ws2812"]["brightness"] = g_config.ws2812.brightness;
    doc["ws2812"]["mode"] = g_config.ws2812.mode;

    // WiFi settings
    doc["wifi"]["ssid"] = g_config.wifi.ssid;
    doc["wifi"]["password"] = g_config.wifi.password;
    doc["wifi"]["deauth"]["rssi_scale"] = g_config.wifi.deauth.rssi_scale_dbm;
    doc["wifi"]["deauth"]["threshold"] = g_config.wifi.deauth.packet_threshold;
    doc["wifi"]["pineap"]["rssi_scale"] = g_config.wifi.pineap.rssi_scale_dbm;
    doc["wifi"]["portal"]["ssid"] = g_config.wifi.portal.ssid;

    // SubGHz settings
    doc["subghz"]["custom_frequencies"] = JsonArray();
    JsonArray custom_freqs = doc["subghz"]["custom_frequencies"].to<JsonArray>();
    for (float freq : g_config.subghz.custom_frequencies) {
        custom_freqs.add(freq);
    }
    doc["subghz"]["mod"] = g_config.subghz.modulation;

    // Raw Record settings
    doc["subghz"]["raw"]["last_frequency"] = g_config.subghz.raw.last_frequency;

    // Scan/Record settings
    doc["subghz"]["scan"]["thresh"] = g_config.subghz.scan.threshold;
    doc["subghz"]["scan"]["type"] = g_config.subghz.scan.type;
    doc["subghz"]["scan"]["range"] = g_config.subghz.scan.range;

    // Display settings
    doc["display"]["rotation"] = g_config.display.rotation;
    doc["display"]["theme"] = g_config.display.theme;

    // Audio settings
    doc["audio"]["volume"] = g_config.audio.volume;

    // Write to file
    File file = SD.open(CONFIG_FILE_PATH, FILE_WRITE);
    if (!file) {
        return false;
    }

    if (serializeJsonPretty(doc, file) == 0) {
        file.close();
        return false;
    }

    file.close();
    return true;
}

bool config_create_example() {
    if (!config_sd_available()) {
        return false;
    }

    JsonDocument doc;

    // Build fully-populated example JSON with comments via description fields
    doc["version"] = CONFIG_VERSION;
    doc["_comment"] = "TE-Nautilus Configuration File - Edit values below to customize your device";

    // WS2812 example
    doc["ws2812"]["color"] = "0x00FF00";  // Green
    doc["ws2812"]["brightness"] = 5;      // Low brightness
    doc["ws2812"]["mode"] = 0;            // 0=static, 1=breathing, 2=rainbow
    doc["ws2812"]["_comment"] = "color: 24-bit RGB hex (0xRRGGBB or #RRGGBB), brightness: 0-255, mode: 0-2";

    // WiFi example
    doc["wifi"]["ssid"] = "MyHomeNetwork";
    doc["wifi"]["password"] = "MySecurePassword123";
    doc["wifi"]["deauth"]["rssi_scale"] = -20;
    doc["wifi"]["deauth"]["threshold"] = 5;
    doc["wifi"]["pineap"]["rssi_scale"] = -20;
    doc["wifi"]["portal"]["ssid"] = "Nautilus WiFi";
    doc["wifi"]["_comment"] = "Credentials saved from WiFi SmartConfig, deauth/pineap settings for hunters, portal SSID for captive portal";

    // SubGHz example
    doc["subghz"]["last_frequency"] = 433.92f;
    doc["subghz"]["custom_frequencies"] = JsonArray();  // Empty - add your own frequencies here!
    doc["subghz"]["mod"] = "am650";
    doc["subghz"]["raw"]["last_frequency"] = 315.00f;
    doc["subghz"]["scan"]["thresh"] = -65;
    doc["subghz"]["scan"]["type"] = "band";
    doc["subghz"]["scan"]["range"] = "full";
    doc["subghz"]["_comment"] = "custom_frequencies: add frequencies NOT in the hardcoded list (57 frequencies already built-in), mod: am270/am650/fm238/fm476, scan.type: single/band/custom, scan.range: single=frequency(MHz), band=low/mid/high/full, custom=start-end(MHz)";

    // Display example
    doc["display"]["rotation"] = 1;   // 0=portrait, 1=landscape, 2=portrait180, 3=landscape180
    doc["display"]["theme"] = 0;      // 0=dark, 1=light
    doc["display"]["_comment"] = "rotation: 0-3, theme: 0=dark 1=light";

    // Audio example
    doc["audio"]["volume"] = 18;      // 0-21
    doc["audio"]["_comment"] = "volume: 0-21 (music player volume)";

    // Write to file
    File file = SD.open(CONFIG_FILE_PATH, FILE_WRITE);
    if (!file) {
        return false;
    }

    if (serializeJsonPretty(doc, file) == 0) {
        file.close();
        return false;
    }

    file.close();
    return true;
}

void config_set_defaults() {
    g_config = NautilusConfig();  // Reset to constructor defaults
}

void config_print() {


    for (size_t i = 0; i < g_config.subghz.custom_frequencies.size(); i++) {
    }


}

/**
 * Save raw record frequency to config
 * Auto-saves to nautilus.json
 */
void config_save_raw_frequency(float freq_mhz) {
    g_config.subghz.raw.last_frequency = freq_mhz;
    config_save();
}

/**
 * Save scan/record settings to config
 * Auto-saves to nautilus.json
 *
 * @param type "single", "band", or "custom"
 * @param range Depends on type:
 *              - single: frequency in MHz (e.g., "315.000")
 *              - band: "low" (300-348), "mid" (387-464), "high" (779-928), "full" (all bands)
 *              - custom: frequency range (e.g., "433-435")
 * @param threshold RSSI threshold in dBm
 */
void config_save_scan_settings(const char* type, const char* range, int threshold) {
    strlcpy(g_config.subghz.scan.type, type, sizeof(g_config.subghz.scan.type));
    strlcpy(g_config.subghz.scan.range, range, sizeof(g_config.subghz.scan.range));
    g_config.subghz.scan.threshold = threshold;
    config_save();
}

/**
 * Save modulation preset to config
 * Auto-saves to nautilus.json
 *
 * @param mod "am270", "am650", "fm238", or "fm476"
 */
void config_save_modulation(const char* mod) {
    strlcpy(g_config.subghz.modulation, mod, sizeof(g_config.subghz.modulation));
    config_save();
}
