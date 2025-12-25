/**
 * CAME Protocol Implementation
 */

#include "protocol_came.h"
#include "../../peripheral.h"  

/**
 * Decode CAME signal from RMT items
 *
 * CAME encoding (gap-then-mark):
 * - Starts with preamble pulse (~350us)
 * - Bit 0: SHORT gap + LONG pulse (350us + 700us)
 * - Bit 1: LONG gap + SHORT pulse (700us + 350us)
 * - MSB transmitted first
 */
bool CAMEProtocol::decode(const rmt_item32_t* data, size_t len, ProtocolDecodeResult& result) {
    if (data == nullptr || len < getMinimumLength()) {
        return false;
    }

    initDecodeResult(result, CAME_TE_SHORT);
    size_t idx = 0;

    while (idx < len && data[idx].level0 == 0) {
        idx++;
    }

    if (idx >= len) {
        return false;  
    }

    // Check preamble pulse 
    uint32_t preamble_us = rmt_item_to_us(data[idx], true);
    if (!pulse_matches(preamble_us, CAME_PREAMBLE, CAME_TOLERANCE_PERCENT)) {
        return false;
    }

    idx++;  

    uint64_t decoded_key = 0;
    uint8_t bit_count = 0;
    const uint8_t MAX_BITS = 24;  // works with 12- and 24-bit CAME

    while (idx < len && bit_count < MAX_BITS) {
        uint32_t pulse_us = rmt_item_to_us(data[idx], true);   // dur0 = carrier ON
        uint32_t gap_us = rmt_item_to_us(data[idx], false);    // dur1 = carrier OFF

        bool is_short_gap = pulse_matches(gap_us, CAME_TE_SHORT, CAME_TOLERANCE_PERCENT);
        bool is_long_gap = pulse_matches(gap_us, CAME_TE_LONG, CAME_TOLERANCE_PERCENT);
        bool is_short_pulse = pulse_matches(pulse_us, CAME_TE_SHORT, CAME_TOLERANCE_PERCENT);
        bool is_long_pulse = pulse_matches(pulse_us, CAME_TE_LONG, CAME_TOLERANCE_PERCENT);

        if (is_short_gap && is_long_pulse) {
            // short gap + long pulse = 0
            decoded_key = (decoded_key << 1) | 0;
            bit_count++;
        } else if (is_long_gap && is_short_pulse) {
            // long gap + short pulse = 1
            decoded_key = (decoded_key << 1) | 1;
            bit_count++;
        } else {
            break;
        }

        idx++;
    }

    // Validate bit count is 12 or 24 
    if (bit_count != 12 && bit_count != 24) {
        return false;
    }

    result.valid = true;
    result.key = decoded_key;
    result.bit_count = bit_count;
    result.te = CAME_TE_SHORT;

    Serial.printf("[CAME] Decoded successfully! Key=0x%llX, bits=%d\n", decoded_key, bit_count);
    return true;
}

/**
 * Encode CAME signal
 *
 * Stores parameters for direct GPIO transmission.
 * We bypass RMT items to use the proven working GPIO code.
 */
bool CAMEProtocol::encode(const ProtocolEncodeParams& params) {
    if (params.bit_count != 12 && params.bit_count != 24) {
        return false;
    }

    // Store parameters for transmitDirect()
    encoded_key = params.key;
    encoded_bits = params.bit_count;
    encoded_repeats = (params.repeat_count > 0) ? params.repeat_count : CAME_DEFAULT_REPEATS;
    return true;
}

/**
 * Get encoded RMT data
 *
 */
bool CAMEProtocol::getEncodedData(const rmt_item32_t** out_data, size_t* out_len) {
    // CAME TX is bit-banged via GPIO
    return false;
}

/**
 * Transmit CAME signal directly via GPIO
 * Uses interrupt protection for timing accuracy
 */
bool CAMEProtocol::transmitDirect(int tx_pin, bool continuous) {
    pinMode(tx_pin, OUTPUT);

    // Ensure clean starting state (pin LOW before any transmission)
    digitalWrite(tx_pin, LOW);
    delayMicroseconds(100);

    int nRepeat = 0;
    while (true) {
        // Disable interrupts for precise timing
        noInterrupts();

        // Preamble 
        if (nRepeat == 0) {
            digitalWrite(tx_pin, HIGH);
            delayMicroseconds(1600);  // Long wake-up pulse
            digitalWrite(tx_pin, LOW);
            delayMicroseconds(130);   // Short gap before sync
        }

        // Send preamble/sync pulse (short HIGH pulse at start of burst)
        digitalWrite(tx_pin, HIGH);
        delayMicroseconds(CAME_PREAMBLE);

        for (int i = encoded_bits - 1; i >= 0; i--) {
            if (encoded_key & (1ULL << i)) {
                // Bit 1: long gap + short pulse
                digitalWrite(tx_pin, LOW);
                delayMicroseconds(CAME_TE_LONG);
                digitalWrite(tx_pin, HIGH);
                delayMicroseconds(CAME_TE_SHORT);
            } else {
                // Bit 0: short gap + long pulse
                digitalWrite(tx_pin, LOW);
                delayMicroseconds(CAME_TE_SHORT);
                digitalWrite(tx_pin, HIGH);
                delayMicroseconds(CAME_TE_LONG);
            }
        }

        digitalWrite(tx_pin, LOW);

        interrupts();

        nRepeat++;

        if (continuous) {
            if (digitalRead(ENCODER_KEY) == HIGH) {
                break;
            }
            delayMicroseconds(CAME_REPEAT_GAP - 1000);
            delay(1); 
        } else {
            if (nRepeat >= encoded_repeats) {
                break;
            }
            delayMicroseconds(CAME_REPEAT_GAP);
        }
    }

    digitalWrite(tx_pin, LOW);
    return true;
}

/**
 * Serialize to .sub file (Flipper Zero format)
 *
 * Example:
 * Protocol: CAME
 * Bit: 12
 * Key: 00 00 00 00 00 00 0E 84
 */
bool CAMEProtocol::serializeToFile(File& file, const ProtocolDecodeResult& result) {
    if (!file) {
        return false;
    }

    file.printf("Protocol: %s\n", getName());
    file.printf("Bit: %d\n", result.bit_count);

    file.printf("Key: ");
    for (int i = 7; i >= 0; i--) {
        uint8_t byte = (result.key >> (i * 8)) & 0xFF;
        file.printf("%02X", byte);
        if (i > 0) file.printf(" ");
    }
    file.printf("\n");

    return true;
}

/**
 * Deserialize from .sub file
 *
 * Reads:
 * - Bit: <count>
 * - Key: <hex bytes>
 */
bool CAMEProtocol::deserializeFromFile(File& file, ProtocolEncodeParams& params) {
    if (!file) {
        return false;
    }

    params.key = 0;
    params.bit_count = 0;
    params.te = 0;
    params.repeat_count = 0;

    String line;
    while (file.available()) {
        line = file.readStringUntil('\n');
        line.trim();

        if (line.startsWith("Bit:")) {
            params.bit_count = line.substring(4).toInt();
        } else if (line.startsWith("Key:")) {
            String key_str = line.substring(4);
            key_str.trim();
            key_str.replace(" ", ""); 

            params.key = 0;
            for (size_t i = 0; i < key_str.length() && i < 16; i++) {
                char c = key_str.charAt(i);
                uint8_t nibble = 0;
                if (c >= '0' && c <= '9') nibble = c - '0';
                else if (c >= 'A' && c <= 'F') nibble = c - 'A' + 10;
                else if (c >= 'a' && c <= 'f') nibble = c - 'a' + 10;
                params.key = (params.key << 4) | nibble;
            }
        } else if (line.startsWith("Protocol:") || line.startsWith("Frequency:") || line.startsWith("Preset:")) {
            continue;
        } else if (line.length() == 0) {
            break;
        }
    }

    // Validate
    if (params.bit_count != 12 && params.bit_count != 24) {
        Serial.printf("[CAME] Invalid bit count in file: %d\n", params.bit_count);
        return false;
    }

    Serial.printf("[CAME] Deserialized: key=0x%llX, bits=%d\n", params.key, params.bit_count);
    return true;
}

/**
 * Reset state machine
 */
void CAMEProtocol::reset() {
    state = STATE_RESET;
    decode_data = 0;
    decode_count_bit = 0;
    te_last = 0;
    has_result = false;
    current_result.valid = false;
}

/**
 * Check if decode is complete and get result
 */
bool CAMEProtocol::decode_check(ProtocolDecodeResult& result) {
    if (has_result) {
        result = current_result;
        has_result = false;  // Clear for next decode
        return true;
    }
    return false;
}

/**
 * Feed edge event to state machine (Flipper Zero style)
 * Based on Flipper Zero's came.c implementation
 */
void CAMEProtocol::feed(bool level, uint32_t duration) {
    // Timing constants (from Flipper Zero)
    const uint32_t TE_SHORT = 320;      // us
    const uint32_t TE_LONG = 640;       // us (2xTE_SHORT)
    const uint32_t TE_DELTA = 150;      // Tolerance
    const uint32_t HEADER_DURATION = 17920;  // 56xTE_SHORT
    const uint32_t HEADER_TOLERANCE = 9450;  // 63xTE_DELTA

    // Helper macro for duration comparison
    #define DURATION_DIFF(x, y) ((x) > (y) ? ((x) - (y)) : ((y) - (x)))

    switch(state) {
    case STATE_RESET:
        if (!level && DURATION_DIFF(duration, HEADER_DURATION) < HEADER_TOLERANCE) {
            state = STATE_FOUND_START_BIT;
        }
        // Fallback: Opportunistic detection 
        else if (!level && (DURATION_DIFF(duration, TE_SHORT) < TE_DELTA ||
                           DURATION_DIFF(duration, TE_LONG) < TE_DELTA)) {
            te_last = duration;
            state = STATE_FOUND_FIRST_BIT;
            decode_data = 0;
            decode_count_bit = 0;
        }
        break;

    case STATE_FOUND_FIRST_BIT:
        // Continuing opportunistic detection
        if (level) {
            if (DURATION_DIFF(te_last, TE_SHORT) < TE_DELTA &&
                DURATION_DIFF(duration, TE_LONG) < TE_DELTA) {
                decode_data = (decode_data << 1) | 0;
                decode_count_bit++;
                state = STATE_SAVE_DURATION;
            }
            else if (DURATION_DIFF(te_last, TE_LONG) < TE_DELTA &&
                     DURATION_DIFF(duration, TE_SHORT) < TE_DELTA) {
                decode_data = (decode_data << 1) | 1;
                decode_count_bit++;
                state = STATE_SAVE_DURATION;
            }
            else {
                state = STATE_RESET;
            }
        } else {
            state = STATE_RESET;
        }
        break;

    case STATE_FOUND_START_BIT:
        if (!level) {
            break;
        }
        if (level && DURATION_DIFF(duration, TE_SHORT) < TE_DELTA) {
            state = STATE_SAVE_DURATION;
            decode_data = 0;
            decode_count_bit = 0;
        } else {
            state = STATE_RESET;
        }
        break;

    case STATE_SAVE_DURATION:
        if (!level) {
            if (duration >= TE_SHORT * 4) {
                if (decode_count_bit == 12 || decode_count_bit == 24) {
                    current_result.valid = true;
                    current_result.key = decode_data;
                    current_result.bit_count = decode_count_bit;
                    current_result.te = TE_SHORT;
                    has_result = true;

                    Serial.printf("[CAME-SM] Decoded: key=0x%llX, bits=%d\n",
                                 decode_data, decode_count_bit);
                } 
                state = STATE_RESET;
                break;
            }

            te_last = duration;
            state = STATE_CHECK_DURATION;
        } else {
            state = STATE_RESET;
        }
        break;

    case STATE_CHECK_DURATION:
        if (level) {
            if (DURATION_DIFF(te_last, TE_SHORT) < TE_DELTA &&
                DURATION_DIFF(duration, TE_LONG) < TE_DELTA) {
                decode_data = (decode_data << 1) | 0;
                decode_count_bit++;
                state = STATE_SAVE_DURATION;
            }
            else if (DURATION_DIFF(te_last, TE_LONG) < TE_DELTA &&
                     DURATION_DIFF(duration, TE_SHORT) < TE_DELTA) {
                decode_data = (decode_data << 1) | 1;
                decode_count_bit++;
                state = STATE_SAVE_DURATION;
            }
            else {
                state = STATE_RESET;
            }
        } else {
            state = STATE_RESET;
        }
        break;
    }

    #undef DURATION_DIFF
}
