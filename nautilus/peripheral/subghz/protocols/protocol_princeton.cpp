/**
 * Princeton Protocol Implementation
 */

#include "protocol_princeton.h"
#include "../../peripheral.h"

/**
 * Decode Princeton signal from RMT items
 *
 * Princeton encoding:
 * - Sync: LOW for 36xTE (~14,400us)
 * - Bit 0: HIGH for 1xTE + LOW for 3xTE (400us + 1200us)
 * - Bit 1: HIGH for 3xTE + LOW for 1xTE (1200us + 400us)
 * - Stop bit: HIGH for 1xTE
 * - Guard: LOW for 30xTE
 * - MSB transmitted first
 */
bool PrincetonProtocol::decode(const rmt_item32_t* data, size_t len, ProtocolDecodeResult& result) {
    if (data == nullptr || len < getMinimumLength()) {
        return false;
    }

    initDecodeResult(result, PRINCETON_TE_DEFAULT);
    size_t idx = 0;

    while (idx < len && data[idx].level0 == 1) {
        idx++;
    }

    if (idx >= len) {
        return false;
    }

    // Check for sync pattern (36 x TE)
    uint32_t sync_us = rmt_item_to_us(data[idx], true);
    uint32_t expected_sync = PRINCETON_TE_SHORT * 36;

    if (!pulse_matches(sync_us, expected_sync, PRINCETON_TOLERANCE_PERCENT)) {
        return false;
    }

    uint32_t detected_te = sync_us / 36;
    idx++;

    uint64_t decoded_key = 0;
    uint8_t bit_count = 0;

    while (idx < len && bit_count < PRINCETON_BIT_COUNT) {
        uint32_t high_us = rmt_item_to_us(data[idx], true);
        uint32_t low_us = rmt_item_to_us(data[idx], false);

        uint32_t te_short = detected_te;
        uint32_t te_long = detected_te * 3;

        bool is_short_high = pulse_matches(high_us, te_short, PRINCETON_TOLERANCE_PERCENT);
        bool is_long_high = pulse_matches(high_us, te_long, PRINCETON_TOLERANCE_PERCENT);
        bool is_short_low = pulse_matches(low_us, te_short, PRINCETON_TOLERANCE_PERCENT);
        bool is_long_low = pulse_matches(low_us, te_long, PRINCETON_TOLERANCE_PERCENT);

        if (is_short_high && is_long_low) {
            // short high + long low = 0
            decoded_key = (decoded_key << 1) | 0;
            bit_count++;
        } else if (is_long_high && is_short_low) {
            // long high + short low = 1
            decoded_key = (decoded_key << 1) | 1;
            bit_count++;
        } else {
            if (bit_count == PRINCETON_BIT_COUNT && is_short_high) {
                break;
            }
            return false;
        }

        idx++;
    }

    if (bit_count != PRINCETON_BIT_COUNT) {
        return false;
    }

    result.valid = true;
    result.key = decoded_key;
    result.bit_count = PRINCETON_BIT_COUNT;
    result.te = detected_te;

    Serial.printf("[Princeton] Decoded successfully! Key=0x%06llX, TE=%dus\n",
                  decoded_key, detected_te);
    return true;
}

/**
 * Encode Princeton signal into RMT items
 */
bool PrincetonProtocol::encode(const ProtocolEncodeParams& params) {
    if (params.bit_count != PRINCETON_BIT_COUNT) {
        return false;
    }

    encoded_key = params.key;
    encoded_te = (params.te > 0) ? params.te : PRINCETON_TE_DEFAULT;
    encoded_repeats = (params.repeat_count > 0) ? params.repeat_count : PRINCETON_DEFAULT_REPEATS;

    uint32_t te_short = encoded_te;
    uint32_t te_long = encoded_te * 3;
    uint32_t sync_duration = encoded_te * 36;
    uint32_t guard_duration = encoded_te * encoded_guard_time;

    rmt_items.clear();

    rmt_items.push_back(make_item(sync_duration, 0, 0, 0));

    for (int i = PRINCETON_BIT_COUNT - 1; i >= 0; i--) {
        bool bit = (params.key >> i) & 1;

        if (bit) {
            // long high + short low = 1
            rmt_items.push_back(make_item(te_long, 1, te_short, 0));
        } else {
            // short high + long low = 0
            rmt_items.push_back(make_item(te_short, 1, te_long, 0));
        }
    }

    rmt_items.push_back(make_item(te_short, 1, 0, 0));
    rmt_items.push_back(make_item(guard_duration, 0, 0, 0));
    return true;
}

/**
 * Get encoded RMT data for transmission
 */
bool PrincetonProtocol::getEncodedData(const rmt_item32_t** out_data, size_t* out_len) {
    if (rmt_items.empty()) {
        return false;
    }

    *out_data = rmt_items.data();
    *out_len = rmt_items.size();
    return true;
}

/**
 * Transmit Princeton signal directly via GPIO
 */
bool PrincetonProtocol::transmitDirect(int tx_pin, bool continuous) {
    pinMode(tx_pin, OUTPUT);

    digitalWrite(tx_pin, LOW);
    delayMicroseconds(100);

    uint32_t te = (encoded_te > 0) ? encoded_te : PRINCETON_TE_DEFAULT;
    uint32_t te_short = te;
    uint32_t te_long = te * 3;
    uint32_t sync_duration = te * 36;
    int repeats = (encoded_repeats > 0) ? encoded_repeats : PRINCETON_DEFAULT_REPEATS;

    int nRepeat = 0;
    while (true) {
        noInterrupts();

        if (nRepeat == 0) {
            digitalWrite(tx_pin, LOW);
            delayMicroseconds(sync_duration);
        }

        for (int i = PRINCETON_BIT_COUNT - 1; i >= 0; i--) {
            bool bit = (encoded_key >> i) & 1;

            if (bit) {
                // long high + short low = 1
                digitalWrite(tx_pin, HIGH);
                delayMicroseconds(te_long);
                digitalWrite(tx_pin, LOW);
                delayMicroseconds(te_short);
            } else {
                // short high + long low = 0
                digitalWrite(tx_pin, HIGH);
                delayMicroseconds(te_short);
                digitalWrite(tx_pin, LOW);
                delayMicroseconds(te_long);
            }
        }

        digitalWrite(tx_pin, HIGH);
        delayMicroseconds(te_short);

        digitalWrite(tx_pin, LOW);

        interrupts();

        nRepeat++;

        if (continuous) {
            if (digitalRead(ENCODER_KEY) == HIGH) {
                break;
            }
            delayMicroseconds(PRINCETON_REPEAT_GAP - 1000);
            delay(1);
        } else {
            if (nRepeat >= repeats) {
                break;
            }
            delayMicroseconds(PRINCETON_REPEAT_GAP - 1000);
            delay(1);
        }
    }

    digitalWrite(tx_pin, LOW);
    return true;
}

/**
 * Serialize Princeton data to .sub file
 */
bool PrincetonProtocol::serializeToFile(File& file, const ProtocolDecodeResult& result) {
    if (!result.valid) {
        return false;
    }

    file.printf("Protocol: %s\n", getName());
    file.printf("Bit: %d\n", result.bit_count);

    file.print("Key: ");
    for (int i = 7; i >= 0; i--) {
        file.printf("%02X", (uint8_t)((result.key >> (i * 8)) & 0xFF));
        if (i > 0) file.print(" ");
    }
    file.println();

    file.printf("TE: %d\n", result.te);
    file.printf("Guard_time: %d\n", PRINCETON_GUARD_TIME);

    return true;
}

/**
 * Deserialize Princeton data from .sub file
 */
bool PrincetonProtocol::deserializeFromFile(File& file, ProtocolEncodeParams& params) {
    String line;
    bool found_bit = false;
    bool found_key = false;
    uint32_t te = PRINCETON_TE_DEFAULT;
    uint32_t guard_time = PRINCETON_GUARD_TIME;

    while (file.available()) {
        line = file.readStringUntil('\n');
        line.trim();

        if (line.startsWith("Bit:")) {
            params.bit_count = line.substring(5).toInt();
            found_bit = true;
        } else if (line.startsWith("Key:")) {
            String key_str = line.substring(5);
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
            found_key = true;
        } else if (line.startsWith("TE:")) {
            te = line.substring(4).toInt();
        } else if (line.startsWith("Guard_time:")) {
            guard_time = line.substring(12).toInt();
            if (guard_time < 15 || guard_time > 72) {
                guard_time = PRINCETON_GUARD_TIME;
            }
        }
    }

    if (!found_bit || !found_key) {
        return false;
    }

    if (params.bit_count != PRINCETON_BIT_COUNT) {
        return false;
    }

    params.te = te;
    params.repeat_count = 0;
    encoded_guard_time = guard_time;

    Serial.printf("[Princeton] Loaded from file: key=0x%06llX, TE=%dus, guard_time=%d\n",
                 params.key, te, guard_time);

    return true;
}

/**
 * Feed edge event to decoder state machine
 */
void PrincetonProtocol::feed(bool level, uint32_t duration) {
    switch (state) {
        case STATE_RESET:
            if (!level) {
                uint32_t expected_sync = PRINCETON_TE_SHORT * 36;
                if (pulse_matches(duration, expected_sync, PRINCETON_TOLERANCE_PERCENT)) {
                    detected_te = duration / 36;
                    decode_data = 0;
                    decode_count_bit = 0;
                    state = STATE_SAVE_DURATION;
                }
            }
            break;

        case STATE_SAVE_DURATION:
            if (level) {
                te_last = duration;
                state = STATE_CHECK_DURATION;
            } else {
                state = STATE_RESET;
            }
            break;

        case STATE_CHECK_DURATION:
            if (!level) {
                uint32_t te_short = detected_te;
                uint32_t te_long = detected_te * 3;

                bool is_short_high = pulse_matches(te_last, te_short, PRINCETON_TOLERANCE_PERCENT);
                bool is_long_high = pulse_matches(te_last, te_long, PRINCETON_TOLERANCE_PERCENT);
                bool is_short_low = pulse_matches(duration, te_short, PRINCETON_TOLERANCE_PERCENT);
                bool is_long_low = pulse_matches(duration, te_long, PRINCETON_TOLERANCE_PERCENT);

                if (is_short_high && is_long_low) {
                    decode_data = (decode_data << 1) | 0;
                    decode_count_bit++;
                    state = STATE_SAVE_DURATION;
                } else if (is_long_high && is_short_low) {
                    decode_data = (decode_data << 1) | 1;
                    decode_count_bit++;
                    state = STATE_SAVE_DURATION;
                } else if (decode_count_bit >= PRINCETON_BIT_COUNT) {
                    current_result.valid = true;
                    current_result.key = decode_data;
                    current_result.bit_count = PRINCETON_BIT_COUNT;
                    current_result.te = detected_te;
                    has_result = true;
                    state = STATE_RESET;
                } else {
                    state = STATE_RESET;
                }
            } else {
                state = STATE_RESET;
            }
            break;
    }
}

/**
 * Reset decoder state machine
 */
void PrincetonProtocol::reset() {
    state = STATE_RESET;
    decode_data = 0;
    decode_count_bit = 0;
    te_last = 0;
    detected_te = PRINCETON_TE_DEFAULT;
    has_result = false;
    current_result.valid = false;
}

/**
 * Check if decoder has successfully decoded a signal
 */
bool PrincetonProtocol::decode_check(ProtocolDecodeResult& result) {
    if (has_result && current_result.valid) {
        result = current_result;
        has_result = false;
        Serial.printf("[Princeton] State machine decoded: key=0x%06llX\n", result.key);
        return true;
    }
    return false;
}
