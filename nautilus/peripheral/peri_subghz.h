/**
 * TE-Nautilus SubGHz Radio Module
 *
 * Provides RF signal capture and playback functionality using CC1101 transceiver
 * and ESP32 RMT peripheral for microsecond-precision timing.
 *
 * Compatible with Flipper Zero .sub file format
 */

#pragma once

#include "../utilities.h"
#include <RadioLib.h>
#include <vector>
#include "FS.h"
#include "SD.h"
#include "driver/rmt.h"
#include "subghz/protocol_base.h"  // For ProtocolDecodeResult

// RMT Configuration
// Use different channels - RX on channel 4, TX on channel 5
#define RMT_RX_CHANNEL     RMT_CHANNEL_4
#define RMT_TX_CHANNEL     RMT_CHANNEL_5
#define RMT_CLK_DIV        80          // 80MHz / 80 = 1MHz = 1µs resolution
#define RMT_MEM_BLOCKS     2           // Memory blocks for capture buffer
#define RMT_RX_BUF_SIZE    2048        // Ring buffer size

// RMT Timing Constants
#define RMT_1US_TICKS      (80000000 / RMT_CLK_DIV / 1000000)
#define RMT_1MS_TICKS      (RMT_1US_TICKS * 1000)

// SubGHz Configuration
#define SUBGHZ_DEFAULT_FREQ     433.92f    // Default frequency (MHz)
#define SUBGHZ_RSSI_THRESHOLD   -70        // RSSI threshold for signal detection (dBm)
#define SUBGHZ_MAX_RAW_PULSES   10000      // Maximum pulses per RAW recording
#define SUBGHZ_CAPTURE_TIMEOUT  30000      // Capture timeout (milliseconds)
#define SUBGHZ_FILE_DIR         "/rf"      // SD card directory for captures

// SubGHz File Presets
// Legacy (kept for compatibility)
#define SUBGHZ_PRESET_RAW       0  // Treated as OOK270

// AM/OOK Presets (Amplitude Modulation / On-Off Keying)
#define SUBGHZ_PRESET_OOK270    1  // AM270: OOK, 270 kHz bandwidth
#define SUBGHZ_PRESET_OOK650    2  // AM650: OOK, 650 kHz bandwidth

// FM/FSK Presets (Frequency Modulation / Frequency Shift Keying)
#define SUBGHZ_PRESET_2FSK_DEV238   3  // FM238: 2-FSK, 2.38 kHz deviation, 270 kHz BW
#define SUBGHZ_PRESET_2FSK_DEV476   4  // FM476: 2-FSK, 47.6 kHz deviation, 270 kHz BW

// Preset name conversion for Flipper Zero compatibility
const char* subghz_preset_to_string(uint8_t preset);

/**
 * RAW Recording Structure
 * Stores captured RF signal timing data from RMT peripheral
 */
struct RawRecording {
    float frequency;                           // Frequency in MHz
    std::vector<rmt_item32_t*> codes;         // Captured pulse/gap sequences
    std::vector<uint16_t> codeLengths;        // Number of items per sequence
    std::vector<uint16_t> gaps;               // Gap durations between sequences (ms)

    RawRecording() : frequency(SUBGHZ_DEFAULT_FREQ) {}

    // Free allocated memory
    void clear() {
        for (auto code : codes) {
            if (code != nullptr) {
                free(code);
            }
        }
        codes.clear();
        codeLengths.clear();
        gaps.clear();
        codes.shrink_to_fit();
        codeLengths.shrink_to_fit();
        gaps.shrink_to_fit();
    }

    ~RawRecording() {
        clear();
    }
};

/**
 * RAW Recording Status
 * Tracks real-time capture status and statistics
 */
struct RawRecordingStatus {
    float frequency;                    // Current frequency
    int rssiCount;                      // Number of RSSI readings
    int latestRssi;                     // Latest RSSI value (dBm)
    int peakRssi;                       // Peak RSSI during capture
    bool recordingStarted;              // Capture started flag
    bool recordingFinished;             // Capture finished flag
    unsigned long firstSignalTime;      // Timestamp of first signal
    unsigned long lastSignalTime;       // Timestamp of last signal
    unsigned long lastRssiUpdate;       // Last RSSI update time
    uint16_t pulseCount;                // Total pulses captured

    RawRecordingStatus() :
        frequency(SUBGHZ_DEFAULT_FREQ),
        rssiCount(0),
        latestRssi(-100),
        peakRssi(-100),
        recordingStarted(false),
        recordingFinished(false),
        firstSignalTime(0),
        lastSignalTime(0),
        lastRssiUpdate(0),
        pulseCount(0) {}
};

/**
 * RF Signal Metadata
 * Stores decoded or parsed signal information for playback
 */
struct RfCodes {
    uint32_t frequency;                 // Frequency in Hz
    uint64_t key;                       // Decoded value (for RCSwitch)
    String protocol;                    // Protocol name ("RAW", "RcSwitch", etc.)
    String preset;                      // Preset name or number
    String data;                        // RAW timing data (for parsing)
    int te;                             // Timing element (µs)
    int Bit;                            // Bit length
    String filepath;                    // Source file path

    // Security+ 2.0 specific fields
    uint64_t secplus_packet1;           // Security+ 2.0 packet 1 data
    uint64_t secplus_packet2;           // Security+ 2.0 packet 2 data
    uint64_t secplus_fixed;             // Security+ 2.0 serial number (40-bit fixed code)
    uint32_t secplus_rolling;           // Security+ 2.0 counter (28-bit rolling code)
    uint8_t secplus_button;             // Security+ 2.0 button code
    bool secplus_valid;                 // True if Security+ fields are populated

    RfCodes() :
        frequency(433920000),
        key(0),
        protocol("RAW"),
        preset("0"),
        te(0),
        Bit(0),
        secplus_packet1(0),
        secplus_packet2(0),
        secplus_fixed(0),
        secplus_rolling(0),
        secplus_button(0),
        secplus_valid(false) {}
};

// Global SubGHz state
extern bool subghz_initialized;
extern RawRecordingStatus subghz_status;

/**
 * SubGHz Module Initialization
 */
void subghz_init(void);
void subghz_deinit(void);
bool subghz_is_init(void);

/**
 * RMT Peripheral Configuration
 */
bool subghz_rmt_rx_init(void);
bool subghz_rmt_tx_init(void);
void subghz_rmt_deinit(void);

/**
 * RF Signal Capture
 */
bool subghz_start_capture(float frequency, uint32_t timeout_ms = SUBGHZ_CAPTURE_TIMEOUT);
void subghz_stop_capture(void);
bool subghz_is_capturing(void);
RawRecording* subghz_get_capture(void);
RawRecordingStatus* subghz_get_status(void);

/**
 * RF Signal Playback
 */
bool subghz_transmit_raw(RawRecording &recording);
bool subghz_transmit_file(const char *filepath);

/**
 * File I/O - .sub format (Flipper Zero compatible)
 * Returns the filename if successful, empty string if failed
 */
String subghz_try_decode_recording(RawRecording &recording, ProtocolDecodeResult &result);
String subghz_save_capture(RawRecording &recording, const char *filename = nullptr, String* out_protocol = nullptr);
bool subghz_load_file(const char *filepath, RfCodes &codes);
bool subghz_parse_raw_line(const String &line, std::vector<int32_t> &timings);

/**
 * Utility Functions
 */
void subghz_set_frequency(float freq_mhz);
int subghz_get_rssi(void);
bool subghz_create_dir_structure(void);
String subghz_generate_filename(const char* protocol_name = nullptr);

/**
 * CC1101 Configuration for SubGHz
 */
bool subghz_configure_radio(float frequency, uint8_t preset = SUBGHZ_PRESET_OOK270);
void subghz_set_antenna_switch(float frequency);

/**
 * Frequency Scanning Structures and Functions
 */
struct FreqFound {
    float freq;
    int rssi;
};

#define FREQ_SCAN_MAX_TRIES 5

/**
 * Scan/Record Status
 * Tracks scanning and signal detection status
 */
struct ScanRecordStatus {
    float frequency;                    // Current or detected frequency
    int signals;                        // Number of signals captured
    int rssi;                          // Current RSSI
    int rssiThreshold;                 // RSSI threshold for detection
    bool scanning;                     // Currently scanning for frequency
    bool listening;                    // Listening for signals
    bool signalDetected;               // Signal detected flag
    uint64_t lastKey;                  // Last decoded key (for dedup)

    ScanRecordStatus() :
        frequency(0.0f),
        signals(0),
        rssi(-100),
        rssiThreshold(-65),
        scanning(false),
        listening(false),
        signalDetected(false),
        lastKey(0) {}
};

/**
 * Scan/Record Functions
 */
float subghz_scan_frequency(int range_index = 3, int threshold = -65);
bool subghz_start_scan_record(int range_index = 3);
void subghz_stop_scan_record(void);
bool subghz_is_scan_recording(void);
ScanRecordStatus* subghz_get_scan_status(void);
RfCodes* subghz_get_last_signal(void);
String subghz_save_last_signal(const char *filename = nullptr, String* out_protocol = nullptr);

/**
 * Custom Range Scanning Functions
 */
bool subghz_is_freq_valid(float freq_mhz);
float subghz_align_freq_to_step(float freq_mhz, float step_khz);
float subghz_next_valid_freq(float current_freq_mhz, float step_khz);
float subghz_scan_custom_range(float start_mhz, float end_mhz, float step_khz, int threshold = -65);
bool subghz_start_scan_record_custom(float start_mhz, float end_mhz, float step_khz);
