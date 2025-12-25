/**
 * TE-Nautilus SubGHz Radio Module Implementation
 */

#include "peri_subghz.h"
#include "peripheral.h"
#include "rf_utils.h"  // CC1101 radio utilities
#include <Arduino.h>
#include <RCSwitch.h>  // For protocol decoding
#include "lvgl.h"  // For lv_timer_handler() to keep UI responsive during scanning
#include "subghz/subghz_protocols.h"  // Protocol framework

// Global state
bool subghz_initialized = false;
RawRecordingStatus subghz_status;

// Internal state
static RawRecording *current_recording = nullptr;
static RingbufHandle_t rmt_ringbuf = nullptr;
static bool rmt_rx_initialized = false;
static bool rmt_tx_initialized = false;
static volatile bool capturing = false;

// Forward declaration for scan/record RMT capture (defined later with scan/record state)
static RawRecording *scan_rmt_recording = nullptr;

// Track recently saved RAW files for cleanup when protocol is detected
static std::vector<String> recent_raw_files;
static unsigned long last_raw_save_time = 0;
static const unsigned long RAW_CLEANUP_TIMEOUT_MS = 5000;  // 5 seconds

/**
 * Convert preset number to Flipper Zero preset name
 */
const char* subghz_preset_to_string(uint8_t preset) {
    switch(preset) {
        case SUBGHZ_PRESET_OOK270:
            return "FuriHalSubGhzPresetOok270Async";
        case SUBGHZ_PRESET_OOK650:
            return "FuriHalSubGhzPresetOok650Async";
        case SUBGHZ_PRESET_2FSK_DEV238:
            return "FuriHalSubGhzPreset2FSKDev238Async";
        case SUBGHZ_PRESET_2FSK_DEV476:
            return "FuriHalSubGhzPreset2FSKDev476Async";
        case SUBGHZ_PRESET_RAW:
        default:
            // Use OOK270 as default for RAW captures (256kHz ~= 270kHz)
            return "FuriHalSubGhzPresetOok270Async";
    }
}

/**
 * Initialize SubGHz module
 */
void subghz_init(void) {
    if (subghz_initialized) {
        return;
    }


    // Initialize protocol system
    subghz_protocols_init();

    // Create directory structure on SD card
    if (sd_is_valid()) {
        subghz_create_dir_structure();
    }

    // Initialize RMT for receive
    if (subghz_rmt_rx_init()) {
    } else {
        return;
    }

    subghz_initialized = true;
}

/**
 * Deinitialize SubGHz module
 */
void subghz_deinit(void) {
    if (!subghz_initialized) {
        return;
    }

    subghz_stop_capture();
    subghz_rmt_deinit();

    if (current_recording != nullptr) {
        current_recording->clear();
        delete current_recording;
        current_recording = nullptr;
    }

    if (scan_rmt_recording != nullptr) {
        scan_rmt_recording->clear();
        delete scan_rmt_recording;
        scan_rmt_recording = nullptr;
    }

    // Deinitialize protocol system
    subghz_protocols_deinit();

    subghz_initialized = false;
}

/**
 * Check if SubGHz module is initialized
 */
bool subghz_is_init(void) {
    return subghz_initialized;
}

/**
 * Initialize RMT for receive (capture)
 */
bool subghz_rmt_rx_init(void) {
    if (rmt_rx_initialized) {
        return true;
    }

    rmt_driver_uninstall(RMT_RX_CHANNEL);

    const gpio_num_t rx_gpio = (gpio_num_t)BOARD_SGHZ_IO2;

    gpio_reset_pin(rx_gpio);
    gpio_set_direction(rx_gpio, GPIO_MODE_INPUT);
    rmt_config_t rmt_rx_config;
    rmt_rx_config.rmt_mode = RMT_MODE_RX;
    rmt_rx_config.channel = RMT_RX_CHANNEL;
    rmt_rx_config.gpio_num = rx_gpio;
    rmt_rx_config.clk_div = RMT_CLK_DIV;
    rmt_rx_config.mem_block_num = 4;
    rmt_rx_config.flags = 0;
    rmt_rx_config.rx_config.idle_threshold = 12 * RMT_1MS_TICKS;
    rmt_rx_config.rx_config.filter_ticks_thresh = 100 * RMT_1US_TICKS;
    rmt_rx_config.rx_config.filter_en = true;

    esp_err_t err = rmt_config(&rmt_rx_config);
    if (err != ESP_OK) {
        return false;
    }

    err = rmt_driver_install(RMT_RX_CHANNEL, 32768, 0);
    if (err != ESP_OK) {
        return false;
    }

    err = rmt_get_ringbuf_handle(RMT_RX_CHANNEL, &rmt_ringbuf);
    if (err != ESP_OK || rmt_ringbuf == nullptr) {
        rmt_driver_uninstall(RMT_RX_CHANNEL);
        return false;
    }

    rmt_rx_initialized = true;
    return true;
}

/**
 * Initialize RMT for transmit (playback)
 */
bool subghz_rmt_tx_init(void) {
    if (rmt_tx_initialized) {
        return true;
    }

    rmt_driver_uninstall(RMT_TX_CHANNEL);

    rmt_config_t rmt_tx_config = RMT_DEFAULT_CONFIG_TX(
        (gpio_num_t)BOARD_SGHZ_IO0,
        RMT_TX_CHANNEL
    );

    rmt_tx_config.clk_div = RMT_CLK_DIV;
    rmt_tx_config.mem_block_num = RMT_MEM_BLOCKS;

    esp_err_t err = rmt_config(&rmt_tx_config);
    if (err != ESP_OK) {
        return false;
    }

    err = rmt_driver_install(RMT_TX_CHANNEL, 0, 0);
    if (err != ESP_OK) {
        return false;
    }

    rmt_tx_initialized = true;
    return true;
}

/**
 * Deinitialize RMT peripheral
 */
void subghz_rmt_deinit(void) {
    if (rmt_rx_initialized) {
        rmt_rx_stop(RMT_RX_CHANNEL);
        rmt_driver_uninstall(RMT_RX_CHANNEL);
        rmt_rx_initialized = false;
        rmt_ringbuf = nullptr;
    }

    if (rmt_tx_initialized) {
        rmt_driver_uninstall(RMT_TX_CHANNEL);
        rmt_tx_initialized = false;
    }
}

/**
 * Configure CC1101 radio for SubGHz operations
 */
bool subghz_configure_radio(float frequency, uint8_t preset) {
    String mode;
    if (!rf_initModule(mode, frequency)) {
        return false;
    }

    switch(preset) {
        case SUBGHZ_PRESET_OOK650:
            ELECHOUSE_cc1101.setModulation(2);
            ELECHOUSE_cc1101.setRxBW(650);
            break;

        case SUBGHZ_PRESET_2FSK_DEV238:
            ELECHOUSE_cc1101.setModulation(0);
            ELECHOUSE_cc1101.setDeviation(2.38);
            ELECHOUSE_cc1101.setRxBW(270);
            break;

        case SUBGHZ_PRESET_2FSK_DEV476:
            ELECHOUSE_cc1101.setModulation(0);
            ELECHOUSE_cc1101.setDeviation(47.6);
            ELECHOUSE_cc1101.setRxBW(270);
            break;

        case SUBGHZ_PRESET_OOK270:
        case SUBGHZ_PRESET_RAW:
        default:
            ELECHOUSE_cc1101.setModulation(2);
            ELECHOUSE_cc1101.setRxBW(270);
            break;
    }

    return true;
}

void subghz_set_antenna_switch(float frequency) {
    if (frequency >= 300.0 && frequency < 348.0) {
        digitalWrite(BOARD_SGHZ_SW1, HIGH);
        digitalWrite(BOARD_SGHZ_SW0, LOW);
    } else if (frequency >= 387.0 && frequency < 464.0) {
        digitalWrite(BOARD_SGHZ_SW1, HIGH);
        digitalWrite(BOARD_SGHZ_SW0, HIGH);
    } else if (frequency >= 779.0 && frequency <= 928.0) {
        digitalWrite(BOARD_SGHZ_SW1, LOW);
        digitalWrite(BOARD_SGHZ_SW0, HIGH);
    }
}

/**
 * Set frequency for SubGHz operations
 */
void subghz_set_frequency(float freq_mhz) {
    extern uint8_t subghz_current_preset;
    subghz_status.frequency = freq_mhz;
    subghz_configure_radio(freq_mhz, subghz_current_preset);
}

/**
 * Get current RSSI
 */
int subghz_get_rssi(void) {
    return ELECHOUSE_cc1101.getRssi();
}

/**
 * Start RF signal capture
 */
bool subghz_start_capture(float frequency, uint32_t timeout_ms) {
    if (!subghz_initialized || !rmt_rx_initialized) {
        return false;
    }

    if (capturing) {
        return false;
    }

    if (xSemaphoreTake(radioLock, pdMS_TO_TICKS(1000)) != pdTRUE) {
        return false;
    }

    if (!rf_initModule("rx", frequency)) {
        xSemaphoreGive(radioLock);
        return false;
    }

    subghz_status = RawRecordingStatus();
    subghz_status.frequency = frequency;
    subghz_status.recordingStarted = true;
    subghz_status.firstSignalTime = millis();

    if (current_recording != nullptr) {
        current_recording->clear();
        delete current_recording;
    }
    current_recording = new RawRecording();
    current_recording->frequency = frequency;

    rmt_rx_start(RMT_RX_CHANNEL, true);

    capturing = true;

    xSemaphoreGive(radioLock);
    return true;
}

/**
 * Stop RF signal capture
 */
void subghz_stop_capture(void) {
    if (!capturing) {
        return;
    }

    rmt_rx_stop(RMT_RX_CHANNEL);
    rf_deinitModule();

    capturing = false;
    subghz_status.recordingFinished = true;
    subghz_status.lastSignalTime = millis();

}

/**
 * Check if currently capturing
 */
bool subghz_is_capturing(void) {
    return capturing;
}

RawRecording* subghz_get_capture(void) {
    if (!capturing || rmt_ringbuf == nullptr || current_recording == nullptr) {
        return current_recording;
    }

    size_t rx_size = 0;
    rmt_item32_t *items = (rmt_item32_t*)xRingbufferReceive(
        rmt_ringbuf,
        &rx_size,
        pdMS_TO_TICKS(100)
    );

    if (items != nullptr && rx_size > 0) {
        size_t num_items = rx_size / sizeof(rmt_item32_t);

        rmt_item32_t *code = (rmt_item32_t*)malloc(rx_size);
        if (code != nullptr) {
            memcpy(code, items, rx_size);
            current_recording->codes.push_back(code);
            current_recording->codeLengths.push_back(num_items);
            current_recording->gaps.push_back(0);

            subghz_status.pulseCount += num_items;
        }

        vRingbufferReturnItem(rmt_ringbuf, (void*)items);

        if (millis() - subghz_status.lastRssiUpdate > 100) {
            subghz_status.latestRssi = subghz_get_rssi();
            if (subghz_status.latestRssi > subghz_status.peakRssi) {
                subghz_status.peakRssi = subghz_status.latestRssi;
            }
            subghz_status.rssiCount++;
            subghz_status.lastRssiUpdate = millis();
        }
    }

    return current_recording;
}

/**
 * Get capture status
 */
RawRecordingStatus* subghz_get_status(void) {
    return &subghz_status;
}

String subghz_try_decode_recording(RawRecording &recording, ProtocolDecodeResult &result) {
    if (recording.codes.empty()) {
        return "";
    }

    for (size_t seq_idx = 0; seq_idx < recording.codes.size(); seq_idx++) {
        rmt_item32_t *sequence = recording.codes[seq_idx];
        size_t length = recording.codeLengths[seq_idx];

        if (sequence == nullptr || length < 5) {
            continue;
        }

        std::vector<std::pair<bool, uint32_t>> edges;
        for (size_t i = 0; i < length; i++) {
            if (sequence[i].duration0 > 0) {
                bool level = (sequence[i].level0 != 0);
                uint32_t duration = RMT_TICKS_TO_US(sequence[i].duration0);

                if (!edges.empty() && edges.back().first == level) {
                    edges.back().second += duration;
                } else {
                    edges.push_back(std::make_pair(level, duration));
                }
            }

            if (sequence[i].duration1 > 0) {
                bool level = (sequence[i].level1 != 0);
                uint32_t duration = RMT_TICKS_TO_US(sequence[i].duration1);

                if (!edges.empty() && edges.back().first == level) {
                    edges.back().second += duration;
                } else {
                    edges.push_back(std::make_pair(level, duration));
                }
            }
        }

        SubGhzProtocol* detected = autoDetectProtocolEdges(edges.data(), edges.size(), result);

        if (detected != nullptr) {
            if (!result.extra_data.isEmpty()) {
            }
            return String(detected->getName());
        }
    }

    if (recording.codes.size() >= 2) {
        std::vector<std::pair<bool, uint32_t>> combined_edges;
        size_t valid_bursts = 0;
        size_t last_valid_burst = 0;
        int last_level = -1;

        for (size_t seq_idx = 0; seq_idx < recording.codes.size(); seq_idx++) {
            rmt_item32_t *sequence = recording.codes[seq_idx];
            size_t length = recording.codeLengths[seq_idx];

            if (sequence == nullptr || length == 0) {
                continue;
            }

            bool is_substantial = (length >= 10);

            if (!is_substantial) {
                continue;
            }

            if (valid_bursts > 0) {
                uint32_t gap_duration = 10000;

                if (!combined_edges.empty() && !combined_edges.back().first) {
                    combined_edges.back().second += gap_duration;
                } else {
                    combined_edges.push_back(std::make_pair(false, gap_duration));
                }
            }

            for (size_t i = 0; i < length; i++) {
                if (sequence[i].duration0 > 0) {
                    bool level = (sequence[i].level0 != 0);
                    uint32_t duration = RMT_TICKS_TO_US(sequence[i].duration0);

                    if (!combined_edges.empty() && combined_edges.back().first == level) {
                        combined_edges.back().second += duration;
                    } else {
                        combined_edges.push_back(std::make_pair(level, duration));
                    }
                    last_level = level ? 1 : 0;
                }

                if (sequence[i].duration1 > 0) {
                    bool level = (sequence[i].level1 != 0);
                    uint32_t duration = RMT_TICKS_TO_US(sequence[i].duration1);

                    if (!combined_edges.empty() && combined_edges.back().first == level) {
                        combined_edges.back().second += duration;
                    } else {
                        combined_edges.push_back(std::make_pair(level, duration));
                    }
                    last_level = level ? 1 : 0;
                }
            }

            valid_bursts++;
            last_valid_burst = seq_idx;
        }

        if (valid_bursts >= 1 && combined_edges.size() >= 10) {
            SubGhzProtocol* detected = autoDetectProtocolEdges(combined_edges.data(), combined_edges.size(), result);

            if (detected != nullptr) {
                if (!result.extra_data.isEmpty()) {
                }
                return String(detected->getName());
            }
        }
    }

    return "";
}

String subghz_save_capture(RawRecording &recording, const char *filename, String* out_protocol) {
    if (!sd_is_valid()) {
        return "";
    }

    ProtocolDecodeResult decode_result;
    String detected_protocol = subghz_try_decode_recording(recording, decode_result);
    SubGhzProtocol* proto = nullptr;

    if (!detected_protocol.isEmpty()) {
        proto = getProtocol(detected_protocol.c_str());
    } else {
    }

    extern RfCodes scan_received;
    if (proto == nullptr && !scan_received.protocol.isEmpty() &&
        scan_received.protocol != "RAW" && scan_received.protocol != "RcSwitch") {
        decode_result.valid = true;
        decode_result.key = scan_received.key;
        decode_result.bit_count = scan_received.Bit;
        decode_result.te = scan_received.te;

        detected_protocol = scan_received.protocol;
        proto = getProtocol(detected_protocol.c_str());
    }

    String filepath;
    if (filename == nullptr) {
        const char* proto_name = (proto != nullptr) ? detected_protocol.c_str() : "raw";
        filepath = subghz_generate_filename(proto_name);
    } else {
        filepath = String(SUBGHZ_FILE_DIR) + "/" + String(filename);
        if (!filepath.endsWith(".sub")) {
            filepath += ".sub";
        }
    }

    File file = SD.open(filepath, FILE_WRITE, true);
    if (!file) {
        return "";
    }

    extern uint8_t subghz_current_preset;
    if (proto != nullptr) {
        file.println("Filetype: Flipper SubGhz Key File");
        file.println("Version: 1");
        file.printf("Frequency: %d\n", (int)(recording.frequency * 1000000));
        file.printf("Preset: %s\n", subghz_preset_to_string(subghz_current_preset));
        file.println("Lat: nan");
        file.println("Lon: nan");

        if (!proto->serializeToFile(file, decode_result)) {
            file.close();
            SD.remove(filepath);

            file = SD.open(filepath, FILE_WRITE, true);
            if (!file) {
                return "";
            }
            proto = nullptr;
        } else {
            file.close();

            if (out_protocol != nullptr) {
                *out_protocol = detected_protocol;
            }

            if (!recent_raw_files.empty()) {
                for (const String& raw_file : recent_raw_files) {
                    if (SD.exists(raw_file)) {
                        SD.remove(raw_file);
                    }
                }
                recent_raw_files.clear();
            }

            return filepath;
        }
    }

    if (proto == nullptr) {
        file.println("Filetype: Flipper SubGhz RAW File");
        file.println("Version: 1");
        file.printf("Frequency: %d\n", (int)(recording.frequency * 1000000));
        file.printf("Preset: %s\n", subghz_preset_to_string(subghz_current_preset));
        file.println("Protocol: RAW");

        uint16_t values_written = 0;
        bool is_first_value = true;
        file.print("RAW_Data:");

    for (size_t i = 0; i < recording.codes.size(); i++) {
        size_t count = recording.codeLengths[i];

        for (size_t j = 0; j < count; j++) {
            rmt_item32_t &item = recording.codes[i][j];

            if (item.duration0 > 0) {
                if (values_written > 0 && values_written % 512 == 0) {
                    file.print("\nRAW_Data:");
                }
                file.print(" ");

                if (is_first_value && item.level0 == 0) {
                    is_first_value = false;
                    continue;
                }

                if (item.level0 == 0) file.print("-");
                file.print(item.duration0);
                values_written++;
                is_first_value = false;
            }

            if (item.duration1 > 0) {
                if (values_written > 0 && values_written % 512 == 0) {
                    file.print("\nRAW_Data:");
                }
                file.print(" ");
                if (item.level1 == 0) file.print("-");
                file.print(item.duration1);
                values_written++;
                is_first_value = false;
            }
        }

        if (i < recording.codes.size() - 1 && recording.gaps[i] > 0) {
            if (values_written > 0 && values_written % 512 == 0) {
                file.print("\nRAW_Data:");
            }
            file.print(" -");
            file.print((int)(recording.gaps[i] * 1000));
            values_written++;
        }

        if (i > 0 && i % 50 == 0) {
            delay(1);
        }
    }

        file.println();
        file.close();

        if (out_protocol != nullptr) {
            *out_protocol = "RAW";
        }

        recent_raw_files.push_back(filepath);
        last_raw_save_time = millis();

        if (recent_raw_files.size() > 1) {
            unsigned long now = millis();
            if (now - last_raw_save_time > RAW_CLEANUP_TIMEOUT_MS) {
                recent_raw_files.clear();
                recent_raw_files.push_back(filepath);
            }
        }

        return filepath;
    }

    file.close();
    return filepath;
}

bool subghz_create_dir_structure(void) {
    if (!sd_is_valid()) {
        return false;
    }

    if (!SD.exists(SUBGHZ_FILE_DIR)) {
        if (!SD.mkdir(SUBGHZ_FILE_DIR)) {
            return false;
        }
    }

    return true;
}

String subghz_generate_filename(const char* protocol_name) {
    int index = 0;
    String filepath;
    String prefix;

    if (protocol_name != nullptr && strlen(protocol_name) > 0) {
        String proto_str = String(protocol_name);

        if (proto_str == "Security+ 1.0") {
            prefix = "secplus_v1";
        } else if (proto_str == "Security+ 2.0") {
            prefix = "secplus_v2";
        } else {
            proto_str.toLowerCase();
            proto_str.replace(" ", "_");
            proto_str.replace("+", "plus");
            proto_str.replace(".", "_");
            prefix = proto_str;
        }
    } else {
        prefix = "capture";
    }

    do {
        filepath = String(SUBGHZ_FILE_DIR) + "/" + prefix + "_" + String(index) + ".sub";
        index++;
    } while (SD.exists(filepath));

    return filepath;
}

bool subghz_parse_raw_line(const String &line, std::vector<int32_t> &timings) {
    int start = 0;
    int end = line.indexOf(' ', start);

    while (end != -1) {
        String value_str = line.substring(start, end);
        value_str.trim();

        if (value_str.length() > 0) {
            int32_t value = value_str.toInt();
            timings.push_back(value);
        }

        start = end + 1;
        end = line.indexOf(' ', start);
    }

    String value_str = line.substring(start);
    value_str.trim();
    if (value_str.length() > 0) {
        int32_t value = value_str.toInt();
        timings.push_back(value);
    }

    return timings.size() > 0;
}

bool subghz_load_file(const char *filepath, RfCodes &codes) {
    if (!sd_is_valid()) {
        return false;
    }

    File file = SD.open(filepath);
    if (!file) {
        return false;
    }

    codes = RfCodes();  // Reset
    codes.filepath = String(filepath);

    while (file.available()) {
        String line = file.readStringUntil('\n');
        line.trim();

        if (line.startsWith("Frequency:")) {
            codes.frequency = line.substring(11).toInt();
        } else if (line.startsWith("Preset:")) {
            codes.preset = line.substring(8);
            codes.preset.trim();
        } else if (line.startsWith("Protocol:")) {
            codes.protocol = line.substring(10);
            codes.protocol.trim();
        } else if (line.startsWith("Bit:")) {
            codes.Bit = line.substring(5).toInt();
        } else if (line.startsWith("TE:")) {
            codes.te = line.substring(4).toInt();
        } else if (line.startsWith("Key:")) {
            String key_str = line.substring(5);
            key_str.trim();
            // Parse hex value (with or without spaces)
            key_str.replace(" ", "");
            if (key_str.startsWith("0x") || key_str.startsWith("0X")) {
                key_str = key_str.substring(2);
            }
            codes.key = strtoull(key_str.c_str(), nullptr, 16);
        } else if (line.startsWith("Secplus_packet_1:")) {
            // Parse Security+ 2.0 packet 1
            String pkt_str = line.substring(17);
            pkt_str.trim();
            pkt_str.replace(" ", "");
            codes.secplus_packet1 = strtoull(pkt_str.c_str(), nullptr, 16);
            codes.secplus_valid = true;
        } else if (line.startsWith("Secplus_packet_2:")) {
            // Parse Security+ 2.0 packet 2 (explicit field - rare)
            String pkt_str = line.substring(17);
            pkt_str.trim();
            pkt_str.replace(" ", "");
            codes.secplus_packet2 = strtoull(pkt_str.c_str(), nullptr, 16);
        } else if (line.startsWith("RAW_Data:")) {
            String data = line.substring(10);
            data.trim();
            if (codes.data.length() > 0) {
                codes.data += " ";
            }
            codes.data += data;
        }
    }

    file.close();

    // If this is a Security+ 2.0 file, decode the packets to extract metadata
    if (codes.protocol == "Security+ 2.0" && codes.secplus_valid && codes.secplus_packet1 != 0) {

        // In Flipper Zero format, if Secplus_packet_2 is not present,
        // the Key field contains packet2, not the fixed code!
        if (codes.secplus_packet2 == 0 && codes.key != 0) {
            codes.secplus_packet2 = codes.key;
        }


        // Get protocol instance from registry
        ProtocolRegistry* registry = ProtocolRegistry::getInstance();
        SubGhzProtocol* proto = registry->getProtocol("Security+ 2.0");

        if (proto != nullptr) {
            SecPlusV2Protocol* secplus = static_cast<SecPlusV2Protocol*>(proto);

            // Decode the packets to extract serial, button, and counter
            uint64_t serial;
            uint8_t button;
            uint32_t counter;

            if (secplus->decodePackets(codes.secplus_packet1, codes.secplus_packet2,
                                       serial, button, counter)) {
                // Store decoded values in RfCodes structure
                codes.secplus_fixed = serial;  // Store 32-bit serial number
                codes.secplus_rolling = counter;
                codes.secplus_button = button;

            } else {
                // Fallback: decoding failed
                codes.secplus_fixed = 0;
                codes.secplus_rolling = 0;
                codes.secplus_button = 0;
            }
        } else {
            // Fallback if protocol not found
            codes.secplus_fixed = 0;
            codes.secplus_rolling = 0;
            codes.secplus_button = 0;
        }
    }


    return true;
}

/**
 * Transmit RAW recording
 */
bool subghz_transmit_raw(RawRecording &recording) {
    if (!subghz_initialized) {
        return false;
    }

    if (recording.codes.size() == 0) {
        return false;
    }

    if (xSemaphoreTake(radioLock, pdMS_TO_TICKS(1000)) != pdTRUE) {
        return false;
    }

    extern uint8_t subghz_current_preset;  // Defined in ui.cpp
    subghz_configure_radio(recording.frequency, subghz_current_preset);

    pinMode(BOARD_SGHZ_IO0, OUTPUT);
    gpio_set_drive_capability((gpio_num_t)BOARD_SGHZ_IO0, GPIO_DRIVE_CAP_3);
    digitalWrite(BOARD_SGHZ_IO0, LOW);

    // Configure PATABLE for OOK modulation
    digitalWrite(BOARD_SGHZ_CS, LOW);
    delayMicroseconds(10);
    SPI.transfer(0x7E);
    SPI.transfer(0xC0);
    SPI.transfer(0xC0);
    SPI.transfer(0x00);
    SPI.transfer(0x00);
    SPI.transfer(0x00);
    SPI.transfer(0x00);
    SPI.transfer(0x00);
    SPI.transfer(0x00);
    delayMicroseconds(10);
    digitalWrite(BOARD_SGHZ_CS, HIGH);

    radio.SPIsetRegValue(0x15, 0x00);
    radio.SPIsetRegValue(0x12, 0x30);
    radio.SPIsetRegValue(0x22, 0x10);

    radio.SPIsendCommand(0x35);
    delayMicroseconds(500);

    unsigned long accumulatedDelay = 0;
    const unsigned long checkIntervalUs = 10000;  // Check button every 10ms
    bool interrupted = false;

    for (size_t i = 0; i < recording.codes.size() && !interrupted; i++) {
        for (uint16_t j = 0; j < recording.codeLengths[i]; j++) {
            // Check if user pressed select button to stop transmission
            if (accumulatedDelay >= checkIntervalUs) {
                if (digitalRead(ENCODER_KEY) == LOW) {
                    interrupted = true;
                    break;
                }
                accumulatedDelay = 0;
            }

            digitalWrite(BOARD_SGHZ_IO0, recording.codes[i][j].level0);
            delayMicroseconds(recording.codes[i][j].duration0);
            accumulatedDelay += recording.codes[i][j].duration0;

            digitalWrite(BOARD_SGHZ_IO0, recording.codes[i][j].level1);
            delayMicroseconds(recording.codes[i][j].duration1);
            accumulatedDelay += recording.codes[i][j].duration1;
        }

        if (!interrupted && i < recording.codes.size() - 1 && recording.gaps[i] > 0) {
            delay(recording.gaps[i]);
        }
    }

    digitalWrite(BOARD_SGHZ_IO0, LOW);
    radio.standby();
    xSemaphoreGive(radioLock);
    return !interrupted;
}

bool subghz_transmit_file(const char *filepath) {
    if (xSemaphoreTake(radioLock, pdMS_TO_TICKS(1000)) != pdTRUE) {
        return false;
    }

    bool result = rf_transmitFile(String(filepath));
    xSemaphoreGive(radioLock);
    return result;
}

/**
 * ========================================================================
 * SCAN/RECORD FUNCTIONALITY
 * ========================================================================
 * Automatic frequency scanning with RCSwitch protocol decoding
 * Based on Squid's RF Scan implementation
 */

// Scan/Record state
static RCSwitch rcswitch_scanner;
static ScanRecordStatus scan_status;
RfCodes scan_received;  // Non-static so it can be accessed from subghz_save_capture()
static bool scan_record_active = false;
bool scan_rmt_rx_stopped = false;  // Non-static so it can be accessed from subghz_save_last_signal()
// Note: scan_rmt_recording is declared at top of file with other static variables

/**
 * Scan for active frequency in specified range
 * Returns the frequency with highest RSSI, or 0.0 if none found
 */
float subghz_scan_frequency(int range_index, int threshold) {
    if (!subghz_initialized) {
        return 0.0f;
    }


    scan_status.scanning = true;
    scan_status.frequency = 0.0f;
    scan_status.rssi = -100;
    scan_status.rssiThreshold = threshold;

    FreqFound best_freqs[FREQ_SCAN_MAX_TRIES];
    for (int i = 0; i < FREQ_SCAN_MAX_TRIES; i++) {
        best_freqs[i].freq = 433.92f;
        best_freqs[i].rssi = -75;
    }

    // Get combined frequency list (hardcoded + custom from config)
    std::vector<float> freq_list = rf_get_combined_frequency_list();
    int total_freqs = freq_list.size();

    // Adjust range limits for combined list if scanning "Full" range
    int start_idx = range_limits[range_index][0];
    int end_idx = (range_index == 3) ? (total_freqs - 1) : range_limits[range_index][1];

    int idx = start_idx;
    uint8_t attempt = 0;

    // Scan through frequency range
    while (attempt < FREQ_SCAN_MAX_TRIES && scan_status.scanning) {
        // Check if user requested stop
        if (!scan_status.scanning) {
            break;
        }

        // Wrap around if we reach the end
        if (idx < start_idx || idx > end_idx) {
            idx = start_idx;
        }

        float check_freq = freq_list[idx];

        // Update current frequency being scanned for UI display
        scan_status.frequency = check_freq;

        // Configure radio for this frequency
        if (rf_initModule("rx", check_freq)) {
            delay(5);  // Let radio settle

            int rssi = ELECHOUSE_cc1101.getRssi();
            scan_status.rssi = rssi;

            if (rssi > threshold) {
                best_freqs[attempt].freq = check_freq;
                best_freqs[attempt].rssi = rssi;
                attempt++;

            }
        }

        idx++;

        // Yield to UI task - allow button presses and display updates
        // Process UI events multiple times to ensure responsiveness
        for (int i = 0; i < 3; i++) {
            lv_timer_handler();
            delay(5);  // Give time for UI to update
        }
    }

    rf_deinitModule();

    // Find the best frequency from our samples
    if (attempt > 0) {
        int best_idx = 0;
        for (int i = 1; i < attempt; i++) {
            if (best_freqs[i].rssi > best_freqs[best_idx].rssi) {
                best_idx = i;
            }
        }

        scan_status.frequency = best_freqs[best_idx].freq;
        scan_status.rssi = best_freqs[best_idx].rssi;
    } else {
        scan_status.frequency = 0.0f;
    }

    scan_status.scanning = false;
    return scan_status.frequency;
}

/**
 * Start Scan/Record mode
 * Scans for frequency (if range_index >= 0) then listens for signals
 */
bool subghz_start_scan_record(int range_index) {
    if (!subghz_initialized) {
        return false;
    }

    if (scan_record_active) {
        return false;
    }

    // Reset status
    scan_status = ScanRecordStatus();
    scan_received = RfCodes();
    scan_status.signals = 0;

    // Scan for frequency if requested
    if (range_index >= 0 && range_index <= 3) {
        float freq = subghz_scan_frequency(range_index, -65);
        if (freq <= 0.0f) {
            return false;
        }
        scan_status.frequency = freq;
    } else {
        // Use default or previously set frequency
        scan_status.frequency = subghz_status.frequency;
        if (scan_status.frequency <= 0.0f) {
            scan_status.frequency = SUBGHZ_DEFAULT_FREQ;
        }
    }

    // Initialize radio for receive
    if (!rf_initModule("rx", scan_status.frequency)) {
        return false;
    }

    // Enable RCSwitch receiver on GDO2 (GPIO 38)
    rcswitch_scanner.enableReceive(BOARD_SGHZ_IO2);

    // Also start RMT capture for precise protocol detection
    if (scan_rmt_recording != nullptr) {
        scan_rmt_recording->clear();
        delete scan_rmt_recording;
    }
    scan_rmt_recording = new RawRecording();
    scan_rmt_recording->frequency = scan_status.frequency;

    // Start RMT receive (uses same GDO2 pin - will capture same signal)
    rmt_rx_start(RMT_RX_CHANNEL, true);

    scan_status.listening = true;
    scan_record_active = true;
    scan_rmt_rx_stopped = false;  // Reset flag for new scan

    return true;
}

/**
 * Stop Scan/Record mode
 */
void subghz_stop_scan_record(void) {
    if (!scan_record_active) {
        return;
    }

    rcswitch_scanner.disableReceive();
    rmt_rx_stop(RMT_RX_CHANNEL);
    rf_deinitModule();

    scan_status.listening = false;
    scan_status.scanning = false;
    scan_record_active = false;

}

/**
 * Check if Scan/Record is active
 */
bool subghz_is_scan_recording(void) {
    return scan_record_active;
}

/**
 * Get Scan/Record status
 * Also polls for new signals from RCSwitch
 */
ScanRecordStatus* subghz_get_scan_status(void) {
    if (!scan_record_active) {
        return &scan_status;
    }

    // Poll RMT data and store in recording (for protocol detection later)
    // IMPORTANT: Drain ALL available items to prevent buffer overflow
    if (scan_rmt_recording != nullptr && rmt_ringbuf != nullptr) {
        int items_received = 0;
        int items_dropped = 0;

        // Keep polling until buffer is completely empty (prevent RMT RX BUFFER FULL errors)
        // No safety limit - we MUST drain everything to prevent overflow
        while (true) {
            size_t rx_size = 0;
            rmt_item32_t *items = (rmt_item32_t*)xRingbufferReceive(
                rmt_ringbuf,
                &rx_size,
                0  // Non-blocking
            );

            if (items == nullptr || rx_size == 0) {
                break;  // Buffer empty - this is our exit condition
            }

            size_t num_items = rx_size / sizeof(rmt_item32_t);

            // Limit accumulated captures to prevent memory bloat
            // Keep only recent captures for protocol detection
            if (scan_rmt_recording->codes.size() < 10) {
                // Allocate and store RMT data
                rmt_item32_t *code = (rmt_item32_t*)malloc(rx_size);
                if (code != nullptr) {
                    memcpy(code, items, rx_size);
                    scan_rmt_recording->codes.push_back(code);
                    scan_rmt_recording->codeLengths.push_back(num_items);
                    scan_rmt_recording->gaps.push_back(0);
                    items_received++;
                } else {
                    items_dropped++;
                }
            } else {
                // Drop oldest capture and add new one (circular buffer behavior)
                if (scan_rmt_recording->codes[0] != nullptr) {
                    free(scan_rmt_recording->codes[0]);
                }
                scan_rmt_recording->codes.erase(scan_rmt_recording->codes.begin());
                scan_rmt_recording->codeLengths.erase(scan_rmt_recording->codeLengths.begin());
                scan_rmt_recording->gaps.erase(scan_rmt_recording->gaps.begin());

                // Add new capture
                rmt_item32_t *code = (rmt_item32_t*)malloc(rx_size);
                if (code != nullptr) {
                    memcpy(code, items, rx_size);
                    scan_rmt_recording->codes.push_back(code);
                    scan_rmt_recording->codeLengths.push_back(num_items);
                    scan_rmt_recording->gaps.push_back(0);
                    items_received++;
                } else {
                    items_dropped++;
                }
            }

            vRingbufferReturnItem(rmt_ringbuf, (void*)items);
        }

        // Stop RMT RX after collecting enough data to prevent buffer overflow from continuous signals
        // We have 10 captures, which is enough for multi-burst protocol detection
        // This prevents "RMT RX BUFFER FULL" errors from continuous transmitters (Security+ 2.0, etc.)
        if (scan_rmt_recording->codes.size() >= 10 && !scan_rmt_rx_stopped) {
            rmt_rx_stop(RMT_RX_CHANNEL);
            scan_rmt_rx_stopped = true;
        }
    }

    // Check for decoded RCSwitch signal
    if (rcswitch_scanner.available()) {
        uint64_t value = rcswitch_scanner.getReceivedValue();

        if (value != 0 && value != scan_status.lastKey) {
            // Check RSSI before counting as valid signal
            int current_rssi = ELECHOUSE_cc1101.getRssi();
            scan_status.rssi = current_rssi;

            if (current_rssi > scan_status.rssiThreshold) {
                // New signal decoded with sufficient RSSI!
                scan_received.key = value;
                scan_received.te = rcswitch_scanner.getReceivedDelay();
                scan_received.Bit = rcswitch_scanner.getReceivedBitlength();
                scan_received.frequency = (uint32_t)(scan_status.frequency * 1000000);
                scan_received.filepath = "signal_" + String(scan_status.signals);

                // Default to RcSwitch (RCSwitch library decoded it)
                scan_received.protocol = "RcSwitch";
                scan_received.preset = String(rcswitch_scanner.getReceivedProtocol());

                // Try auto-detecting specific protocols using protocol framework
                // Note: This is a best-effort check based on RCSwitch-decoded data
                // True protocol detection happens on RAW captures below
                if (scan_received.Bit == 24 && scan_received.te >= 370 && scan_received.te <= 430) {
                    // Characteristics match Princeton (24-bit, TE ~400us)
                    scan_received.protocol = "Princeton";
                    scan_received.preset = "0";  // Will be overwritten by current preset
                } else if ((scan_received.Bit == 12 || scan_received.Bit == 24) &&
                           scan_received.te >= 280 && scan_received.te <= 370) {
                    // Characteristics match CAME (12/24-bit, TE ~320-350us)
                    scan_received.protocol = "CAME";
                    scan_received.preset = "0";  // Will be overwritten by current preset
                } else if (scan_received.Bit == 62 &&
                           scan_received.te >= 140 && scan_received.te <= 360) {
                    // Characteristics match Security+ 2.0 (Manchester encoded, ~250us TE)
                    // 62 bits total: 20 sync + 2 packet type + 2 padding + 38 data
                    scan_received.protocol = "Security+ 2.0";
                    scan_received.preset = "0";  // Will be overwritten by current preset
                }

                scan_status.signals++;
                scan_status.signalDetected = true;
                scan_status.lastKey = value;

            }
        }

        rcswitch_scanner.resetAvailable();
    }

    // Check for RAW signal
    if (rcswitch_scanner.RAWavailable()) {
        unsigned int *raw = rcswitch_scanner.getRAWReceivedRawdata();
        String raw_data = "";
        int transitions = 0;

        for (transitions = 0; transitions < RCSWITCH_RAW_MAX_CHANGES && raw[transitions] != 0; transitions++) {
            if (transitions > 0) raw_data += " ";
            signed int sign = (transitions % 2 == 0) ? 1 : -1;
            raw_data += String(sign * (int)raw[transitions]);
        }

        if (transitions > 10) {  // Only count significant signals
            // Check RSSI before counting as valid signal
            int current_rssi = ELECHOUSE_cc1101.getRssi();
            scan_status.rssi = current_rssi;

            // Calculate simple hash for deduplication
            uint64_t hash = 0;
            for (int i = 0; i < transitions && i < 20; i++) {
                hash = hash * 31 + raw[i];
            }

            if (hash != scan_status.lastKey && current_rssi > scan_status.rssiThreshold) {
                // New RAW signal with sufficient RSSI!
                scan_received.data = raw_data;
                scan_received.protocol = "RAW";
                scan_received.preset = "0";
                scan_received.Bit = transitions;
                scan_received.frequency = (uint32_t)(scan_status.frequency * 1000000);
                scan_received.filepath = "signal_" + String(scan_status.signals);

                scan_status.signals++;
                scan_status.signalDetected = true;
                scan_status.lastKey = hash;
            }
        }

        rcswitch_scanner.resetAvailable();
    }

    // Update RSSI periodically
    static unsigned long last_rssi_update = 0;
    if (millis() - last_rssi_update > 200) {
        scan_status.rssi = ELECHOUSE_cc1101.getRssi();
        last_rssi_update = millis();
    }

    return &scan_status;
}

/**
 * Get last captured signal
 */
RfCodes* subghz_get_last_signal(void) {
    // Return last signal if any were captured (even if scan/record is now stopped)
    if (scan_status.signals == 0) {
        return nullptr;
    }
    return &scan_received;
}

// Forward declarations for scan/record state (defined later in file)
extern bool scan_rmt_rx_stopped;

/**
 * Save last captured signal to SD card
 * Returns filename if successful, empty string if failed
 */
String subghz_save_last_signal(const char *filename, String* out_protocol) {
    if (scan_status.signals == 0) {
        return "";
    }

    if (!sd_is_valid()) {
        return "";
    }

    // If we have RMT data, use proper protocol detection instead of RcSwitch heuristics
    // But only if the RMT data is substantial (not just noise or empty captures)
    bool has_useful_rmt = false;
    if (scan_rmt_recording != nullptr && !scan_rmt_recording->codes.empty()) {

        // Calculate total items across all sequences
        size_t total_items = 0;
        for (size_t i = 0; i < scan_rmt_recording->codes.size(); i++) {
            total_items += scan_rmt_recording->codeLengths[i];
        }

        // Check if total data is enough for protocol detection
        // Each RMT item = 2 edges, so 10 items = 20 edges minimum
        if (total_items >= 10) {
            has_useful_rmt = true;
        }
    }

    if (has_useful_rmt) {
        String result = subghz_save_capture(*scan_rmt_recording, filename, out_protocol);

        // DON'T clear RMT recording in scan mode - we want to keep capturing
        // The circular buffer behavior in subghz_update() handles memory management
        // Only clear if protocol detection succeeded to avoid re-detecting same signal
        if (!result.isEmpty() && out_protocol != nullptr && !out_protocol->isEmpty()) {
            scan_rmt_recording->clear();

            // Restart RMT receiver if it was stopped (after capturing 10 sequences)
            if (scan_rmt_rx_stopped) {
                rmt_rx_start(RMT_RX_CHANNEL, true);
                scan_rmt_rx_stopped = false;
            }
        }

        return result;
    } else {
        // DON'T clear useless data in scan mode - next signal might be better
        // if (scan_rmt_recording != nullptr && !scan_rmt_recording->codes.empty()) {
        //     scan_rmt_recording->clear();  // Clear useless data
        // }
    }

    // Fall back to RcSwitch-based save (heuristic detection only)

    // Create directory if needed
    if (!SD.exists(SUBGHZ_FILE_DIR)) {
        SD.mkdir(SUBGHZ_FILE_DIR);
    }

    // Generate filename if not provided
    String filepath;
    if (filename == nullptr || strlen(filename) == 0) {
        // Use protocol name from scan_received (RcSwitch detection)
        const char* proto_name = (!scan_received.protocol.isEmpty() &&
                                  scan_received.protocol != "RcSwitch")
                                 ? scan_received.protocol.c_str()
                                 : "scan";
        filepath = subghz_generate_filename(proto_name);
    } else {
        filepath = String(SUBGHZ_FILE_DIR) + "/" + String(filename);
        if (!filepath.endsWith(".sub")) {
            filepath += ".sub";
        }
    }

    File file = SD.open(filepath, FILE_WRITE, true);
    if (!file) {
        return "";
    }

    // Write file header - Flipper Zero format
    file.println("Filetype: Flipper SubGhz Key File");
    file.println("Version: 1");
    file.printf("Frequency: %u\n", scan_received.frequency);

    // Determine preset (CAME and Princeton protocols require OOK modulation)
    extern uint8_t subghz_current_preset;  // Defined in ui.cpp
    const char* preset_str;
    if (scan_received.protocol == "CAME" || scan_received.protocol == "Princeton") {
        // CAME and Princeton use OOK650 modulation (fixed)
        preset_str = "FuriHalSubGhzPresetOok650Async";
    } else {
        preset_str = subghz_preset_to_string(subghz_current_preset);
    }
    file.printf("Preset: %s\n", preset_str);

    file.printf("Protocol: %s\n", scan_received.protocol.c_str());

    if (scan_received.protocol == "CAME") {
        // Save CAME protocol in FZ format (space-separated hex bytes)
        file.printf("Bit: %d\n", scan_received.Bit);

        // Format key as 8-byte hex string with spaces (e.g., "00 00 00 00 00 00 0E 84")
        file.printf("Key: ");
        for (int i = 7; i >= 0; i--) {
            uint8_t byte = (scan_received.key >> (i * 8)) & 0xFF;
            file.printf("%02X", byte);
            if (i > 0) file.printf(" ");
        }
        file.printf("\n");
        // CAME doesn't use TE field (has fixed 320us timing)
    } else if (scan_received.protocol == "Princeton") {
        // Save Princeton protocol in FZ format (same as CAME but with TE and Guard_time)
        file.printf("Bit: %d\n", scan_received.Bit);

        // Format key as 8-byte hex string with spaces
        file.printf("Key: ");
        for (int i = 7; i >= 0; i--) {
            uint8_t byte = (scan_received.key >> (i * 8)) & 0xFF;
            file.printf("%02X", byte);
            if (i > 0) file.printf(" ");
        }
        file.printf("\n");
        file.printf("TE: %d\n", scan_received.te);
        file.printf("Guard_time: 30\n");  // Default Princeton guard time
    } else if (scan_received.protocol == "Security+ 2.0") {
        // Save Security+ 2.0 protocol in Flipper Zero format
        // Note: RcSwitch cannot properly decode Manchester-encoded SecPlus v2
        // This saves basic metadata; proper decoding requires RMT-based capture
        file.printf("Bit: %d\n", 62);

        // For now, save the raw key as packet2 (this is incomplete data)
        // Full implementation requires RMT-based protocol detection
        file.printf("Key: ");
        for (int i = 7; i >= 0; i--) {
            uint8_t byte = (scan_received.key >> (i * 8)) & 0xFF;
            file.printf("%02X", byte);
            if (i > 0) file.printf(" ");
        }
        file.printf("\n");

        // Placeholder for packet1 (would need proper Manchester decode)
        file.printf("Secplus_packet_1: 00 00 00 00 00 00 00 00\n");

    } else if (scan_received.protocol == "RcSwitch") {
        // Save RCSwitch decoded signal
        file.printf("Bit: %d\n", scan_received.Bit);
        file.printf("Key: %llX\n", scan_received.key);
        if (scan_received.te > 0) {
            file.printf("TE: %d\n", scan_received.te);
        }
    } else if (scan_received.protocol == "RAW") {
        // Save RAW timing data
        file.printf("RAW_Data: %s\n", scan_received.data.c_str());
    }

    file.close();

    return filepath;
}

/**
 * ========================================================================
 * CUSTOM RANGE SCANNING FUNCTIONALITY
 * ========================================================================
 * Enables scanning across arbitrary frequency ranges with configurable step sizes
 * Automatically skips invalid CC1101 frequency gaps
 */

/**
 * Check if a frequency is in a valid CC1101 band
 * Valid bands: 300-348 MHz, 387-464 MHz, 779-928 MHz
 */
bool subghz_is_freq_valid(float freq_mhz) {
    return (freq_mhz >= 300.0f && freq_mhz <= 348.0f) ||
           (freq_mhz >= 387.0f && freq_mhz <= 464.0f) ||
           (freq_mhz >= 779.0f && freq_mhz <= 928.0f);
}

/**
 * Align frequency to step boundary to avoid floating-point drift
 * @param freq_mhz Frequency in MHz
 * @param step_khz Step size in kHz (supports fractional kHz like 6.25, 12.5)
 * @return Frequency aligned to nearest step boundary
 */
float subghz_align_freq_to_step(float freq_mhz, float step_khz) {
    // Work in 100 Hz increments to balance precision and avoid float overflow
    // 312.0 MHz = 3,120,000 units (100 Hz each)
    // 10 kHz = 100 units, 6.25 kHz = 62.5 units
    long freq_units = lroundf(freq_mhz * 10000.0f);
    float step_units = step_khz * 10.0f;

    // Calculate number of steps from zero and round
    long num_steps = lroundf((float)freq_units / step_units);

    // Calculate aligned frequency in 100 Hz units
    long aligned_units = lroundf(num_steps * step_units);

    // Convert back to MHz
    return aligned_units / 10000.0f;
}

/**
 * Get next valid frequency, skipping invalid gaps
 * @param current_freq_mhz Current frequency in MHz
 * @param step_khz Step size in kHz (supports fractional kHz like 6.25, 12.5)
 * @return Next valid frequency, or 0 if end of range
 */
float subghz_next_valid_freq(float current_freq_mhz, float step_khz) {
    // Use integer arithmetic in 100 Hz units to avoid floating-point accumulation errors
    // Example: 312.000 MHz + 10 kHz = 3,120,000 units + 100 units = 3,120,100 units = 312.010 MHz
    long current_units = lroundf(current_freq_mhz * 10000.0f);
    long step_units = lroundf(step_khz * 10.0f);

    long next_units = current_units + step_units;
    float next_freq = next_units / 10000.0f;

    // Skip gap between 348-387 MHz
    if (current_freq_mhz <= 348.0f && next_freq > 348.0f && next_freq < 387.0f) {
        next_freq = 387.0f;
    }

    // Skip gap between 464-779 MHz
    if (current_freq_mhz <= 464.0f && next_freq > 464.0f && next_freq < 779.0f) {
        next_freq = 779.0f;
    }

    // Check if we've exceeded the maximum frequency
    if (next_freq > 928.0f) {
        return 0.0f;  // End of range
    }

    return next_freq;
}

/**
 * Scan custom frequency range with specified step size
 * @param start_mhz Start frequency in MHz
 * @param end_mhz End frequency in MHz
 * @param step_khz Step size in kHz
 * @param threshold RSSI threshold for signal detection
 * @return Best frequency found, or 0.0 if none found
 */
float subghz_scan_custom_range(float start_mhz, float end_mhz, float step_khz, int threshold) {
    if (!subghz_initialized) {
        return 0.0f;
    }


    scan_status.scanning = true;
    scan_status.frequency = 0.0f;
    scan_status.rssi = -100;
    scan_status.rssiThreshold = threshold;

    FreqFound best_freqs[FREQ_SCAN_MAX_TRIES];
    for (int i = 0; i < FREQ_SCAN_MAX_TRIES; i++) {
        best_freqs[i].freq = start_mhz;
        best_freqs[i].rssi = -75;
    }

    uint8_t attempt = 0;
    float current_freq = start_mhz;

    // Ensure start frequency is valid
    if (!subghz_is_freq_valid(current_freq)) {
        current_freq = subghz_next_valid_freq(current_freq - (step_khz / 1000.0f), step_khz);
        if (current_freq == 0.0f) {
            scan_status.scanning = false;
            return 0.0f;
        }
    }

    // Scan through frequency range
    while (attempt < FREQ_SCAN_MAX_TRIES && scan_status.scanning && current_freq <= end_mhz && current_freq > 0.0f) {
        // Check if user requested stop
        if (!scan_status.scanning) {
            break;
        }

        // Only scan valid frequencies
        if (subghz_is_freq_valid(current_freq)) {
            // Update current frequency being scanned for UI display
            scan_status.frequency = current_freq;

            // Configure radio for this frequency
            if (rf_initModule("rx", current_freq)) {
                delay(5);  // Let radio settle

                int rssi = ELECHOUSE_cc1101.getRssi();
                scan_status.rssi = rssi;

                if (rssi > threshold) {
                    best_freqs[attempt].freq = current_freq;
                    best_freqs[attempt].rssi = rssi;
                    attempt++;

                }
            }
        }

        // Move to next frequency
        current_freq = subghz_next_valid_freq(current_freq, step_khz);

        // Yield to UI task - allow button presses and display updates
        for (int i = 0; i < 3; i++) {
            lv_timer_handler();
            delay(5);
        }
    }

    rf_deinitModule();

    // Find the best frequency from our samples
    if (attempt > 0) {
        int best_idx = 0;
        for (int i = 1; i < attempt; i++) {
            if (best_freqs[i].rssi > best_freqs[best_idx].rssi) {
                best_idx = i;
            }
        }

        scan_status.frequency = best_freqs[best_idx].freq;
        scan_status.rssi = best_freqs[best_idx].rssi;
    } else {
        scan_status.frequency = 0.0f;
    }

    scan_status.scanning = false;
    return scan_status.frequency;
}

/**
 * Start Scan/Record mode with custom frequency range
 * @param start_mhz Start frequency in MHz
 * @param end_mhz End frequency in MHz
 * @param step_khz Step size in kHz
 */
bool subghz_start_scan_record_custom(float start_mhz, float end_mhz, float step_khz) {
    if (!subghz_initialized) {
        return false;
    }

    if (scan_record_active) {
        return false;
    }

    // Reset status
    scan_status = ScanRecordStatus();
    scan_received = RfCodes();
    scan_status.signals = 0;

    // Scan for frequency in custom range
    float freq = subghz_scan_custom_range(start_mhz, end_mhz, step_khz, -65);
    if (freq <= 0.0f) {
        return false;
    }
    scan_status.frequency = freq;

    // Initialize radio for receive
    if (!rf_initModule("rx", scan_status.frequency)) {
        return false;
    }

    // Enable RCSwitch receiver on GDO2 (GPIO 38)
    rcswitch_scanner.enableReceive(BOARD_SGHZ_IO2);

    // Also start RMT capture for precise protocol detection
    if (scan_rmt_recording != nullptr) {
        scan_rmt_recording->clear();
        delete scan_rmt_recording;
    }
    scan_rmt_recording = new RawRecording();
    scan_rmt_recording->frequency = scan_status.frequency;

    // Start RMT receive
    rmt_rx_start(RMT_RX_CHANNEL, true);

    scan_status.listening = true;
    scan_record_active = true;

    return true;
}
