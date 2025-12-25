/**
 * RF Utils - CC1101 Radio Utilities for Nautilus
 */

#include "rf_utils.h"
#include "peri_subghz.h"  // For RfCodes struct definition
#include "peripheral.h"   // For sd_is_valid() and other peripheral functions
#include "peri_config.h"  // For g_config
#include "utilities.h"
#include <RCSwitch.h>
#include <SD.h>
#include <SPI.h>
#include "subghz/subghz_protocols.h"  // Protocol framework
#include "subghz/protocols/protocol_came.h"  // For direct CAME transmission
#include "subghz/protocols/protocol_princeton.h"  // For direct Princeton transmission
#include "subghz/protocols/protocol_secplus_v1.h"  // For Security+ 1.0 special handling
#include "subghz/protocols/protocol_secplus_v2.h"  // For Security+ 2.0 special handling

// CRC-64-ECMA constants
const uint64_t CRC64_ECMA_POLY = 0x42F0E1EBA9EA3693;
const uint64_t CRC64_ECMA_INIT = 0xFFFFFFFFFFFFFFFF;

// Error tracking
RfTransmitError rf_last_error = RF_TX_SUCCESS;

const char* rf_get_error_string(RfTransmitError error) {
    switch (error) {
        case RF_TX_SUCCESS:
            return "Success";
        case RF_TX_USER_STOPPED:
            return "Transmission stopped";
        case RF_TX_UNSUPPORTED_PROTOCOL:
            return "Protocol unsupported";
        case RF_TX_FILE_ERROR:
            return "File error";
        case RF_TX_ENCODING_ERROR:
            return "Encoding error";
        default:
            return "Unknown error";
    }
}

// Frequency ranges (56 hardcoded frequencies total, indices 0-55)
const int range_limits[4][2] = {
    {0,  22}, // 300-348 MHz (23 frequencies)
    {23, 46}, // 387-464 MHz (24 frequencies)
    {47, 55}, // 779-928 MHz (9 frequencies)
    {0,  55}  // All ranges (56 frequencies)
};

const char *subghz_frequency_ranges[] = {"300-348 MHz", "387-464 MHz", "779-928 MHz", "All ranges"};

const float subghz_frequency_list[] = {
    /* 300 - 348 MHz */
    300.000f, 302.757f, 303.875f, 303.900f, 304.250f, 307.000f, 307.500f, 307.800f,
    309.000f, 310.000f, 312.000f, 312.100f, 312.200f, 313.000f, 313.850f, 314.000f,
    314.350f, 315.000f, 318.000f, 330.000f, 345.000f, 348.000f, 350.000f,

    /* 387 - 464 MHz */
    387.000f, 390.000f, 418.000f, 430.000f, 430.500f, 431.000f, 431.500f, 433.075f,
    433.220f, 433.420f, 433.657f, 433.889f, 433.920f, 434.075f, 434.177f, 434.190f,
    434.390f, 434.420f, 434.620f, 434.775f, 438.900f, 440.175f, 464.000f, 467.750f,

    /* 779 - 928 MHz */
    779.000f, 868.350f, 868.400f, 868.800f, 868.950f, 906.400f, 915.000f, 925.000f, 928.000f
};

const float subghz_mhz_list[] = {
    300.000f, 301.000f, 302.000f, 303.000f, 304.000f, 305.000f, 306.000f,
    307.000f, 308.000f, 309.000f, 310.000f, 311.000f, 312.000f, 313.000f,
    314.000f, 315.000f, 316.000f, 317.000f, 318.000f, 319.000f, 320.000f,
    321.000f, 322.000f, 323.000f, 324.000f, 325.000f, 326.000f, 327.000f,
    328.000f, 329.000f, 330.000f, 331.000f, 332.000f, 333.000f, 334.000f,
    335.000f, 336.000f, 337.000f, 338.000f, 339.000f, 340.000f, 341.000f,
    342.000f, 343.000f, 344.000f, 345.000f, 346.000f, 347.000f, 348.000f,
    387.000f, 388.000f, 389.000f, 390.000f, 391.000f, 392.000f, 393.000f,
    394.000f, 395.000f, 396.000f, 397.000f, 398.000f, 399.000f, 400.000f,
    401.000f, 402.000f, 403.000f, 404.000f, 405.000f, 406.000f, 407.000f,
    408.000f, 409.000f, 410.000f, 411.000f, 412.000f, 413.000f, 414.000f,
    415.000f, 416.000f, 417.000f, 418.000f, 419.000f, 420.000f, 421.000f,
    422.000f, 423.000f, 424.000f, 425.000f, 426.000f, 427.000f, 428.000f,
    429.000f, 430.000f, 431.000f, 432.000f, 433.000f, 434.000f, 435.000f,
    436.000f, 437.000f, 438.000f, 439.000f, 440.000f, 441.000f, 442.000f,
    443.000f, 444.000f, 445.000f, 446.000f, 447.000f, 448.000f, 449.000f,
    450.000f, 451.000f, 452.000f, 453.000f, 454.000f, 455.000f, 456.000f,
    457.000f, 458.000f, 459.000f, 460.000f, 461.000f, 462.000f, 463.000f,
    464.000f, 779.000f, 780.000f, 781.000f, 782.000f, 783.000f, 784.000f,
    785.000f, 786.000f, 787.000f, 788.000f, 789.000f, 790.000f, 791.000f,
    792.000f, 793.000f, 794.000f, 795.000f, 796.000f, 797.000f, 798.000f,
    799.000f, 800.000f, 801.000f, 802.000f, 803.000f, 804.000f, 805.000f,
    806.000f, 807.000f, 808.000f, 809.000f, 810.000f, 811.000f, 812.000f,
    813.000f, 814.000f, 815.000f, 816.000f, 817.000f, 818.000f, 819.000f,
    820.000f, 821.000f, 822.000f, 823.000f, 824.000f, 825.000f, 826.000f,
    827.000f, 828.000f, 829.000f, 830.000f, 831.000f, 832.000f, 833.000f,
    834.000f, 835.000f, 836.000f, 837.000f, 838.000f, 839.000f, 840.000f,
    841.000f, 842.000f, 843.000f, 844.000f, 845.000f, 846.000f, 847.000f,
    848.000f, 849.000f, 850.000f, 851.000f, 852.000f, 853.000f, 854.000f,
    855.000f, 856.000f, 857.000f, 858.000f, 859.000f, 860.000f, 861.000f,
    862.000f, 863.000f, 864.000f, 865.000f, 866.000f, 867.000f, 868.000f,
    869.000f, 870.000f, 871.000f, 872.000f, 873.000f, 874.000f, 875.000f,
    876.000f, 877.000f, 878.000f, 879.000f, 880.000f, 881.000f, 882.000f,
    883.000f, 884.000f, 885.000f, 886.000f, 887.000f, 888.000f, 889.000f,
    890.000f, 891.000f, 892.000f, 893.000f, 894.000f, 895.000f, 896.000f,
    897.000f, 898.000f, 899.000f, 900.000f, 901.000f, 902.000f, 903.000f,
    904.000f, 905.000f, 906.000f, 907.000f, 908.000f, 909.000f, 910.000f,
    911.000f, 912.000f, 913.000f, 914.000f, 915.000f, 916.000f, 917.000f,
    918.000f, 919.000f, 920.000f, 921.000f, 922.000f, 923.000f, 924.000f,
    925.000f, 926.000f, 927.000f, 928.000f
};

static bool cc1101_spi_ready = false;

// Cache for combined frequency list
static std::vector<float> cached_combined_list;
static size_t cached_custom_count = 0;
static bool cache_valid = false;

/**
 * Invalidate the combined frequency list cache
 * Call this whenever custom frequencies are added/removed
 */
void rf_invalidate_frequency_cache() {
    cache_valid = false;
}

/**
 * Get combined frequency list (hardcoded + custom from config)
 * Returns a vector containing all 57 default frequencies plus any custom ones from nautilus.json
 * Result is cached for efficiency - call rf_invalidate_frequency_cache() when custom frequencies change
 */
std::vector<float> rf_get_combined_frequency_list() {
    // Check if cache is still valid (same number of custom frequencies)
    if (cache_valid && cached_custom_count == g_config.subghz.custom_frequencies.size()) {
        return cached_combined_list;
    }

    // Rebuild the list
    cached_combined_list.clear();

    // Add all hardcoded frequencies (56 total)
    for (int i = 0; i < 56; i++) {
        cached_combined_list.push_back(subghz_frequency_list[i]);
    }

    // Add custom frequencies from config (avoiding duplicates)
    for (size_t i = 0; i < g_config.subghz.custom_frequencies.size(); i++) {
        float custom_freq = g_config.subghz.custom_frequencies[i];

        // Check if this frequency already exists in the list
        bool is_duplicate = false;
        for (size_t j = 0; j < cached_combined_list.size(); j++) {
            if (fabs(cached_combined_list[j] - custom_freq) < 0.01f) {
                is_duplicate = true;
                break;
            }
        }

        if (!is_duplicate) {
            cached_combined_list.push_back(custom_freq);
        }
    }

    cached_custom_count = g_config.subghz.custom_frequencies.size();
    cache_valid = true;

    return cached_combined_list;
}

/**
 * Get the count of combined frequencies
 */
int rf_get_combined_frequency_count() {
    return 56 + g_config.subghz.custom_frequencies.size();
}

/**
 * Initialize CC1101 once with proper SPI instance
 */
void rf_initCC1101(SPIClass *SSPI) {
    // Set SPI instance - Nautilus uses standard SPI object
    if (SSPI != NULL) {
        ELECHOUSE_cc1101.setSPIinstance(SSPI);
    } else {
        ELECHOUSE_cc1101.setSPIinstance(&SPI);
    }

    // Configure SPI pins for Nautilus hardware
    ELECHOUSE_cc1101.setSpiPin(
        BOARD_SGHZ_SCK,   // SCK = 11
        BOARD_SGHZ_MISO,  // MISO = 10
        BOARD_SGHZ_MOSI,  // MOSI = 9
        BOARD_SGHZ_CS     // CS = 12
    );

    // Set GDO0 pin (GPIO 3) for data I/O
    ELECHOUSE_cc1101.setGDO0(BOARD_SGHZ_IO0);
}

/**
 * Set antenna switch based on frequency
 * Nautilus has SW1=GPIO47, SW0=GPIO48
 */
void rf_setFrequency(float frequency) {
    if (frequency > 928 || frequency < 280) {
        frequency = 433.92;
        Serial.println("[RF] Frequency out of band, using 433.92");
    }

    static uint8_t antenna = 200; // First run flag

    // SW1:1  SW0:0 --- 315MHz
    // SW1:1  SW0:1 --- 433MHz
    // SW1:0  SW0:1 --- 868/915MHz
    if (frequency <= 350 && antenna != 0) {
        digitalWrite(BOARD_SGHZ_SW1, HIGH);
        digitalWrite(BOARD_SGHZ_SW0, LOW);
        antenna = 0;
        vTaskDelay(10 / portTICK_PERIOD_MS);
    } else if (frequency > 350 && frequency < 468 && antenna != 1) {
        digitalWrite(BOARD_SGHZ_SW1, HIGH);
        digitalWrite(BOARD_SGHZ_SW0, HIGH);
        antenna = 1;
        vTaskDelay(10 / portTICK_PERIOD_MS);
    } else if (frequency > 778 && antenna != 2) {
        digitalWrite(BOARD_SGHZ_SW1, LOW);
        digitalWrite(BOARD_SGHZ_SW0, HIGH);
        antenna = 2;
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }

    ELECHOUSE_cc1101.setMHZ(frequency);
}

/**
 * Initialize RF module
 *
 * @param mode "tx", "rx", or "" for config only
 * @param frequency Frequency in MHz (0 = default 433.92)
 */
bool rf_initModule(String mode, float frequency) {
    // Use default frequency if none provided
    if (!frequency || frequency == 0) {
        frequency = 433.92;
    }

    // Initialize antenna switch pins
    pinMode(BOARD_SGHZ_SW1, OUTPUT);
    pinMode(BOARD_SGHZ_SW0, OUTPUT);

    // Initialize CC1101 with SPI instance
    // Nautilus shares SPI bus with TFT and SD card
    rf_initCC1101(&SPI);

    // Initialize CC1101
    ELECHOUSE_cc1101.Init();

    // Verify SPI communication
    if (!ELECHOUSE_cc1101.getCC1101()) {
        Serial.println("[RF] CC1101 connection FAILED!");
        return false;
    }

    // Validate frequency
    if (!((frequency >= 280 && frequency <= 350) ||
          (frequency >= 387 && frequency <= 468) ||
          (frequency >= 779 && frequency <= 928))) {
        Serial.printf("[RF] Invalid frequency %.2f, using 433.92\n", frequency);
        frequency = 433.92;
    }

    // Configure CC1101 parameters
    ELECHOUSE_cc1101.setRxBW(256);      // Narrow band for better accuracy
    ELECHOUSE_cc1101.setClb(1, 13, 15); // Calibration Offset
    ELECHOUSE_cc1101.setClb(2, 16, 19); // Calibration Offset
    ELECHOUSE_cc1101.setModulation(2);  // ASK/OOK modulation
    ELECHOUSE_cc1101.setDRate(50);      // 50 kBaud data rate
    ELECHOUSE_cc1101.setPktFormat(3);   // Asynchronous serial mode (CRITICAL for RMT!)

    rf_setFrequency(frequency);

    // Set mode
    if (mode == "tx") {
        pinMode(BOARD_SGHZ_IO0, OUTPUT);
        ELECHOUSE_cc1101.setPA(12);  // Max power
        ELECHOUSE_cc1101.SetTx();
    } else if (mode == "rx") {
        pinMode(BOARD_SGHZ_IO2, INPUT);  // GDO2 for RX
        ELECHOUSE_cc1101.SetRx();
    } 

    cc1101_spi_ready = true;
    return true;
}

/**
 * Deinitialize RF module
 */
void rf_deinitModule() {
    if (cc1101_spi_ready) {
        ELECHOUSE_cc1101.setSidle();
        cc1101_spi_ready = false;
        digitalWrite(BOARD_SGHZ_IO0, LOW);
        digitalWrite(BOARD_SGHZ_CS, HIGH);
    }
}

/**
 * Send RAW timing data
 * Returns true if completed, false if interrupted
 */
bool rf_sendRaw(int *ptrtransmittimings) {
    if (!ptrtransmittimings) return false;

    int nTransmitterPin = BOARD_SGHZ_IO0;
    bool currentlogiclevel = true;
    int nRepeatTransmit = 1;

    for (int nRepeat = 0; nRepeat < nRepeatTransmit; nRepeat++) {
        unsigned int currenttiming = 0;
        unsigned long accumulatedDelay = 0;
        const unsigned long checkIntervalUs = 10000;  // Check button every 10ms

        while (ptrtransmittimings[currenttiming]) {
            // Check if user released button to stop continuous transmission
            // Note: Only abort if button is HIGH (released), not LOW (pressed)
            if (accumulatedDelay >= checkIntervalUs) {
                if (digitalRead(ENCODER_KEY) == HIGH) {
                    digitalWrite(nTransmitterPin, LOW);
                    return false;  // Interrupted
                }
                accumulatedDelay = 0;
            }

            if (ptrtransmittimings[currenttiming] >= 0) {
                currentlogiclevel = true;
            } else {
                currentlogiclevel = false;
                ptrtransmittimings[currenttiming] = (-1) * ptrtransmittimings[currenttiming];
            }

            digitalWrite(nTransmitterPin, currentlogiclevel ? HIGH : LOW);
            delayMicroseconds(ptrtransmittimings[currenttiming]);
            accumulatedDelay += ptrtransmittimings[currenttiming];
            currenttiming++;
        }
        digitalWrite(nTransmitterPin, LOW);
    }
    return true;  // Completed successfully
}

bool rf_sendRMT(const rmt_item32_t* items, size_t len, bool skip_reset) {
    if (!items || len == 0) {
        return false;
    }

    int nTransmitterPin = BOARD_SGHZ_IO0;
    pinMode(nTransmitterPin, OUTPUT);

    if (!skip_reset) {
        digitalWrite(nTransmitterPin, LOW);
    }

    for (size_t i = 0; i < len; i++) {
        const rmt_item32_t& item = items[i];

        if (item.duration0 > 0) {
            digitalWrite(nTransmitterPin, item.level0 ? HIGH : LOW);
            delayMicroseconds(item.duration0);
        }

        if (item.duration1 > 0) {
            digitalWrite(nTransmitterPin, item.level1 ? HIGH : LOW);
            delayMicroseconds(item.duration1);
        }
    }

    digitalWrite(nTransmitterPin, LOW);
    return true;
}

/**
 * Send using RCSwitch protocol
 */
void rf_sendRCSwitch(uint64_t data, unsigned int bits, int pulse, int protocol, int repeat) {
    RCSwitch mySwitch = RCSwitch();
    mySwitch.enableTransmit(BOARD_SGHZ_IO0);
    mySwitch.setProtocol(protocol);
    if (pulse) {
        mySwitch.setPulseLength(pulse);
    }
    mySwitch.setRepeatTransmit(repeat);
    mySwitch.send(data, bits);
    mySwitch.disableTransmit();
}

/**
 * Send RF command from RfCodes structure
 */
bool rf_sendCommand(struct RfCodes rfcode) {
    uint32_t frequency = rfcode.frequency;
    String protocol = rfcode.protocol;
    String preset = rfcode.preset;
    String data = rfcode.data;
    uint64_t key = rfcode.key;
    int bits = rfcode.Bit;
    byte modulation = 2;
    float rxBW = 270.83;

    int rcswitch_protocol_no = 1;

    // Parse preset
    if (preset == "FuriHalSubGhzPresetOok270Async") {
        rcswitch_protocol_no = 1;
        modulation = 2;
        rxBW = 270;
    } else if (preset == "FuriHalSubGhzPresetOok650Async") {
        rcswitch_protocol_no = 2;
        modulation = 2;
        rxBW = 650;
    } else {
        rcswitch_protocol_no = preset.toInt();
    }

    // Initialize transmitter
    if (!rf_initModule("", frequency / 1000000.0)) return false;

    ELECHOUSE_cc1101.setModulation(modulation);
    if (rxBW) ELECHOUSE_cc1101.setRxBW(rxBW);
    pinMode(BOARD_SGHZ_IO0, OUTPUT);
    ELECHOUSE_cc1101.setPA(12);
    ELECHOUSE_cc1101.SetTx();

    bool success = true;

    // Try protocol framework first
    SubGhzProtocol* proto = getProtocol(protocol.c_str());
    if (proto != nullptr) {
        // Use protocol framework
        Serial.printf("[RF] Sending %s via protocol framework: key=0x%llX bits=%d\n",
                     protocol.c_str(), key, bits);

        // Special handling for Security+ 1.0 (two-packet alternating transmission)
        if (protocol == "Security+ 1.0") {
            SecPlusV1Protocol* secplus = static_cast<SecPlusV1Protocol*>(proto);

            // Check if button held down for continuous transmission
            bool continuous = (digitalRead(ENCODER_KEY) == LOW);

            ProtocolEncodeParams params;
            params.key = key;
            params.bit_count = bits;
            params.te = rfcode.te;
            params.repeat_count = 0;

            if (proto->encode(params)) {
                // Get packet buffers
                const rmt_item32_t* packet1_with_preamble;
                const rmt_item32_t* packet1_no_preamble;
                const rmt_item32_t* packet2_items;
                size_t packet1_preamble_len, packet1_no_preamble_len, packet2_len;

                if (!proto->getEncodedData(&packet1_with_preamble, &packet1_preamble_len)) {
                    rf_last_error = RF_TX_ENCODING_ERROR;
                    success = false;
                }
                else if (!secplus->getPacket1NoPreamble(&packet1_no_preamble, &packet1_no_preamble_len)) {
                    rf_last_error = RF_TX_ENCODING_ERROR;
                    success = false;
                }
                else if (!secplus->getPacket2Data(&packet2_items, &packet2_len)) {
                    rf_last_error = RF_TX_ENCODING_ERROR;
                    success = false;
                }
                else {
                    if (continuous) {
                        // First transmission with preamble
                        // Pattern: [Preamble+Pkt1][Pkt2][Pkt1][Pkt2]...
                        // Note: No inter-packet delay needed - the packet headers provide the spacing
                        success = rf_sendRMT(packet1_with_preamble, packet1_preamble_len);

                        if (success) {
                            // Send packet 2 immediately (its header provides the LOW period)
                            rf_sendRMT(packet2_items, packet2_len);

                            // Continue alternating (without preamble) while button held
                            int loop_count = 0;
                            while (digitalRead(ENCODER_KEY) == LOW) {
                                // Send packet 1 (no preamble) - its header provides the LOW period
                                rf_sendRMT(packet1_no_preamble, packet1_no_preamble_len);

                                // Send packet 2 - its header provides the LOW period
                                rf_sendRMT(packet2_items, packet2_len);

                                // Feed watchdog every 20 iterations to prevent timeout
                                loop_count++;
                                if (loop_count % 20 == 0) {
                                    delay(1);  // Feed watchdog
                                }
                            }
                        }
                    } else {
                        // Single transmission: send both packets once with preamble
                        // No inter-packet delay - the packet headers provide the spacing
                        success = rf_sendRMT(packet1_with_preamble, packet1_preamble_len);

                        if (success) {
                            success = rf_sendRMT(packet2_items, packet2_len);
                        }
                    }
                }
            } else {
                Serial.println("[RF] Security+ 1.0 encoding failed");
                rf_last_error = RF_TX_ENCODING_ERROR;
                success = false;
            }
        }
        // Special handling for Security+ 2.0 (two-burst transmission)
        else if (protocol == "Security+ 2.0") {
            SecPlusV2Protocol* secplus = static_cast<SecPlusV2Protocol*>(proto);

            // Construct 40-bit fixed code from button + serial (like Flipper does)
            uint32_t fixed1 = (rfcode.secplus_button << 12) | (rfcode.secplus_fixed >> 20);
            uint32_t fixed2 = rfcode.secplus_fixed & 0xFFFFF;
            uint64_t fixed = ((uint64_t)fixed1 << 20) | fixed2;

            // Get rolling code from rfcode structure (decoded from file)
            uint32_t rolling = rfcode.secplus_rolling;

            // Check if button held down for continuous transmission
            bool continuous = (digitalRead(ENCODER_KEY) == LOW);

            if (secplus->encodeFromCodes(fixed, rolling)) {
                // Get packet data pointers
                const rmt_item32_t* packet1_items;
                const rmt_item32_t* packet2_items;
                size_t packet1_len, packet2_len;

                if (!secplus->getEncodedData(&packet1_items, &packet1_len)) {
                    rf_last_error = RF_TX_ENCODING_ERROR;
                    success = false;
                }
                else if (!secplus->getPacket2Data(&packet2_items, &packet2_len)) {
                    rf_last_error = RF_TX_ENCODING_ERROR;
                    success = false;
                }
                else {
                    // Send packet 1 with preamble (first transmission only)
                    success = rf_sendRMT(packet1_items, packet1_len);

                    if (success) {
                        if (continuous) {
                            // Get packet1 WITHOUT preamble for continuous mode
                            const rmt_item32_t* packet1_no_preamble;
                            size_t packet1_no_preamble_len;
                            if (secplus->getPacket1NoPreamble(&packet1_no_preamble, &packet1_no_preamble_len)) {
                                // First packet 2
                                delayMicroseconds(66730);
                                delay(1);  // Feed watchdog
                                rf_sendRMT(packet2_items, packet2_len);

                                // Continue sending packet1+packet2 while button held
                                while (digitalRead(ENCODER_KEY) == LOW) {
                                    // 68ms gap (split to feed watchdog)
                                    delayMicroseconds(66730);
                                    delay(1);  // Feed watchdog

                                    // Packet 1 (no preamble)
                                    rf_sendRMT(packet1_no_preamble, packet1_no_preamble_len);

                                    // 68ms gap (split to feed watchdog)
                                    delayMicroseconds(66730);
                                    delay(1);  // Feed watchdog

                                    // Packet 2
                                    rf_sendRMT(packet2_items, packet2_len);
                                }
                            } else {
                                // Fall back to single transmission
                                delayMicroseconds(66730);
                                delay(1);  // Feed watchdog
                                rf_sendRMT(packet2_items, packet2_len);
                            }
                        } else {
                            // Single transmission mode
                            delayMicroseconds(66730);
                            delay(1);  // Feed watchdog
                            success = rf_sendRMT(packet2_items, packet2_len);
                        }
                    }
                }
            } else {
                Serial.println("[RF] Security+ 2.0 encoding failed");
                rf_last_error = RF_TX_ENCODING_ERROR;
                success = false;
            }
        }
        // Special handling for BinRAW (requires deserializeFromFile)
        else if (protocol == "BinRAW") {
            // BinRAW needs to load data from file before encoding
            if (!sd_is_valid()) {
                Serial.println("[RF] SD card not available for BinRAW");
                rf_last_error = RF_TX_ENCODING_ERROR;
                success = false;
            } else {
                // Re-open the .sub file to load BinRAW data
                File file = SD.open(rfcode.filepath.c_str());
                if (!file) {
                    Serial.printf("[RF] Failed to reopen %s\n", rfcode.filepath.c_str());
                    rf_last_error = RF_TX_ENCODING_ERROR;
                    success = false;
                } else {
                    // Skip to protocol-specific section
                    String line;
                    while (file.available()) {
                        line = file.readStringUntil('\n');
                        line.trim();
                        if (line.startsWith("Protocol:")) {
                            break;
                        }
                    }

                    // Deserialize BinRAW data
                    ProtocolEncodeParams params;
                    if (proto->deserializeFromFile(file, params)) {
                        file.close();

                        // Now encode to RMT items
                        if (proto->encode(params)) {
                            const rmt_item32_t* items;
                            size_t len;
                            if (proto->getEncodedData(&items, &len)) {
                                // Check if button held down for continuous transmission
                                bool continuous = (digitalRead(ENCODER_KEY) == LOW);

                                if (continuous) {
                                    // Continuous mode: repeat while button held
                                    Serial.println("[RF] Continuous BinRAW transmission...");
                                    int transmit_count = 0;

                                    while (digitalRead(ENCODER_KEY) == LOW) {
                                        success = rf_sendRMT(items, len);
                                        if (!success) break;

                                        transmit_count++;

                                        // Feed watchdog every 10 transmissions
                                        if (transmit_count % 10 == 0) {
                                            delay(1);  // Feed watchdog
                                        }

                                        // Small gap between repeats (10ms)
                                        delayMicroseconds(9000);
                                        delay(1);  // Feed watchdog
                                    }
                                    Serial.printf("[RF] BinRAW transmitted %d times\n", transmit_count);
                                } else {
                                    // Single transmission
                                    success = rf_sendRMT(items, len);
                                }
                            } else {
                                Serial.println("[RF] Failed to get encoded BinRAW data");
                                rf_last_error = RF_TX_ENCODING_ERROR;
                                success = false;
                            }
                        } else {
                            Serial.println("[RF] BinRAW encoding failed");
                            rf_last_error = RF_TX_ENCODING_ERROR;
                            success = false;
                        }
                    } else {
                        file.close();
                        Serial.println("[RF] Failed to deserialize BinRAW file");
                        rf_last_error = RF_TX_ENCODING_ERROR;
                        success = false;
                    }
                }
            }
        }
        // Standard protocol encoding
        else {
            ProtocolEncodeParams params;
            params.key = key;
            params.bit_count = bits;
            params.te = rfcode.te;
            params.repeat_count = 0;  // Use protocol default

            if (proto->encode(params)) {
                // For CAME and Princeton protocols, use direct GPIO transmission
                if (protocol == "CAME") {
                    // Check if button held down for continuous transmission
                    bool continuous = (digitalRead(ENCODER_KEY) == LOW);

                    CAMEProtocol* came = static_cast<CAMEProtocol*>(proto);
                    success = came->transmitDirect(BOARD_SGHZ_IO0, continuous);

                } else if (protocol == "Princeton") {
                    // Check if button held down for continuous transmission
                    bool continuous = (digitalRead(ENCODER_KEY) == LOW);

                    PrincetonProtocol* princeton = static_cast<PrincetonProtocol*>(proto);
                    success = princeton->transmitDirect(BOARD_SGHZ_IO0, continuous);

                } else {
                    // Other protocols: try RMT items
                    const rmt_item32_t* items;
                    size_t len;
                    if (proto->getEncodedData(&items, &len)) {
                        success = rf_sendRMT(items, len);
                    } else {
                        Serial.println("[RF] Failed to get encoded data");
                        rf_last_error = RF_TX_ENCODING_ERROR;
                        success = false;
                    }
                }
            } else {
                Serial.println("[RF] Protocol encoding failed");
                rf_last_error = RF_TX_ENCODING_ERROR;
                success = false;
            }
        }
    }
    // Fallback to legacy protocol handling
    else if (protocol == "RAW") {
        // Parse RAW_Data string into int array
        int buff_size = 0;
        int index = 0;
        while (index >= 0) {
            index = data.indexOf(' ', index + 1);
            buff_size++;
        }

        int *transmittimings = (int *)calloc(sizeof(int), buff_size + 1);
        size_t transmittimings_idx = 0;

        int startIndex = 0;
        index = 0;
        for (transmittimings_idx = 0; transmittimings_idx < buff_size; transmittimings_idx++) {
            index = data.indexOf(' ', startIndex);
            if (index == -1) {
                transmittimings[transmittimings_idx] = data.substring(startIndex).toInt();
            } else {
                transmittimings[transmittimings_idx] = data.substring(startIndex, index).toInt();
            }
            startIndex = index + 1;
        }
        transmittimings[transmittimings_idx] = 0;

        Serial.println("[RF] Sending RAW signal...");
        bool raw_result = rf_sendRaw(transmittimings);
        free(transmittimings);

        // rf_sendRaw returns false when user releases button during continuous TX
        // This is not an error - treat as success
        if (!raw_result) {
            rf_last_error = RF_TX_USER_STOPPED;
        }
        success = true;  // RAW transmission always succeeds (user stop is not a failure)
    } else if (protocol == "RcSwitch") {
        Serial.printf("[RF] Sending RCSwitch: key=0x%llX bits=%d\n", rfcode.key, rfcode.Bit);
        rf_sendRCSwitch(rfcode.key, rfcode.Bit, rfcode.te, rcswitch_protocol_no, 10);
    } else {
        Serial.printf("[RF] Unsupported protocol: %s\n", protocol.c_str());
        rf_last_error = RF_TX_UNSUPPORTED_PROTOCOL;
        success = false;
    }

    rf_deinitModule();

    if (success) {
        rf_last_error = RF_TX_SUCCESS;
    }

    return success;
}

/**
 * Transmit .sub file from SD card
 */
bool rf_transmitFile(String filepath) {
    struct RfCodes selected_code;
    File databaseFile;
    String line;
    String txt;
    int sent = 0;

    databaseFile = SD.open(filepath, FILE_READ);

    if (!databaseFile) {
        Serial.printf("[RF] Failed to open file: %s\n", filepath.c_str());
        return false;
    }

    Serial.printf("[RF] Opened .sub file: %s\n", filepath.c_str());
    selected_code.filepath = filepath;  // Store full path for protocols like BinRAW that need to re-open the file

    std::vector<int> bitList;
    std::vector<uint64_t> keyList;
    std::vector<String> rawDataList;
    uint64_t secplus_packet1 = 0;

    // Parse .sub file
    while (databaseFile.available()) {
        line = databaseFile.readStringUntil('\n');
        txt = line.substring(line.indexOf(":") + 1);
        if (txt.endsWith("\r")) txt.remove(txt.length() - 1);
        txt.trim();

        if (line.startsWith("Protocol:")) selected_code.protocol = txt;
        if (line.startsWith("Preset:")) selected_code.preset = txt;
        if (line.startsWith("Frequency:")) selected_code.frequency = txt.toInt();
        if (line.startsWith("TE:")) selected_code.te = txt.toInt();
        if (line.startsWith("Bit:")) bitList.push_back(txt.toInt());
        if (line.startsWith("Key:")) {
            // Remove spaces from hex string (e.g., "00 00 0E 84" -> "0000E84")
            String hexStr = txt;
            hexStr.replace(" ", "");
            keyList.push_back(strtoull(hexStr.c_str(), nullptr, 16));
        }
        if (line.startsWith("Secplus_packet_1:")) {
            // Parse Security+ 2.0 packet 1
            String hexStr = txt;
            hexStr.replace(" ", "");
            secplus_packet1 = strtoull(hexStr.c_str(), nullptr, 16);
        }
        if (line.startsWith("RAW_Data:") || line.startsWith("Data_RAW:")) {
            rawDataList.push_back(txt);
        }
    }
    databaseFile.close();

    // Decode Security+ 2.0 packets if present
    if (selected_code.protocol == "Security+ 2.0" && secplus_packet1 != 0 && keyList.size() > 0) {
        uint64_t secplus_packet2 = keyList[0];  // Key field contains packet 2

        SubGhzProtocol* proto = getProtocol("Security+ 2.0");
        if (proto != nullptr) {
            SecPlusV2Protocol* secplus = static_cast<SecPlusV2Protocol*>(proto);
            uint64_t serial;
            uint8_t button;
            uint32_t counter;

            if (secplus->decodePackets(secplus_packet1, secplus_packet2, serial, button, counter)) {
                selected_code.secplus_fixed = serial;
                selected_code.secplus_rolling = counter;
                selected_code.secplus_button = button;
                selected_code.secplus_valid = true;
            }
        }
    }

    // Count actual signals (key/bit pairs count as 1, RAW data counts as 1)
    size_t signal_count = (keyList.size() > bitList.size()) ? keyList.size() : bitList.size();
    int total = signal_count + (rawDataList.size() > 0 ? 1 : 0);

    // Send all signals
    bool interrupted = false;
    rf_last_error = RF_TX_SUCCESS;

    if (selected_code.protocol != "" && selected_code.preset != "" && selected_code.frequency > 0) {
        // Send key/bit pairs (must match up corresponding values)
        size_t signal_count = (keyList.size() < bitList.size()) ? keyList.size() : bitList.size();
        for (size_t i = 0; i < signal_count && !interrupted; i++) {
            selected_code.key = keyList[i];
            selected_code.Bit = bitList[i];
            if (!rf_sendCommand(selected_code)) {
                interrupted = true;
                // rf_last_error already set by rf_sendCommand
                break;
            }
            sent++;
        }

        if (!interrupted) {
            for (String rawData : rawDataList) {
                selected_code.data = rawData;
                if (!rf_sendCommand(selected_code)) {
                    interrupted = true;
                    // rf_last_error already set by rf_sendCommand
                    break;
                }
            }
            if (rawDataList.size() > 0) sent++;
        }
    }

    if (interrupted) {
        // Error already set by rf_sendCommand
        if (rf_last_error == RF_TX_SUCCESS) {
            // Must have been user interrupt (button released during continuous TX)
            rf_last_error = RF_TX_USER_STOPPED;
        }
    } else {
        rf_last_error = RF_TX_SUCCESS;
    }

    // Cleanup
    bitList.clear();
    keyList.clear();
    rawDataList.clear();

    return !interrupted;  // Return false if interrupted, true if completed
}

/**
 * Utility: Find pulse index with tolerance
 */
int find_pulse_index(const std::vector<int> &indexed_durations, int duration) {
    int abs_duration = abs(duration);
    int closest_index = -1;
    int closest_diff = 999999;

    for (size_t i = 0; i < indexed_durations.size(); i++) {
        int diff = abs(indexed_durations[i] - abs_duration);
        if (diff <= 50) {  // +/-50us tolerance
            return i;
        }
        if (diff < closest_diff) {
            closest_diff = diff;
            closest_index = i;
        }
    }

    if (indexed_durations.size() < 4) {
        return -1;
    }

    return closest_index;
}

/**
 * Utility: CRC-64-ECMA
 */
uint64_t crc64_ecma(const std::vector<int> &data) {
    uint64_t crc = CRC64_ECMA_INIT;

    for (int value : data) {
        crc ^= (uint64_t)value << 56;
        for (int i = 0; i < 8; i++) {
            if (crc & 0x8000000000000000) {
                crc = (crc << 1) ^ CRC64_ECMA_POLY;
            } else {
                crc <<= 1;
            }
        }
    }

    return crc;
}
