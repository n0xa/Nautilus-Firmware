/**
 * Security+ 1.0 Protocol Implementation
 *
 * Based on:
 * - Flipper Zero implementation
 * - argilo/secplus reverse engineering research
 */

#include "protocol_secplus_v1.h"
#include <Arduino.h>

/**
 * Reverse bit order of a 32-bit value
 */
static uint32_t reverse_bits_32(uint32_t value) {
    uint32_t result = 0;
    for (int i = 0; i < 32; i++) {
        result = (result << 1) | (value & 1);
        value >>= 1;
    }
    return result;
}

/**
 * Check if duration matches expected value within tolerance
 */
static inline bool duration_match(uint32_t duration, uint32_t expected, uint32_t tolerance) {
    return (duration >= (expected - tolerance)) && (duration <= (expected + tolerance));
}

/**
 * Encode 64-bit key into 40 ternary symbols using base-3 conversion
 * Based on Flipper Zero's subghz_protocol_secplus_v1_encode()
 */
bool SecPlusV1Protocol::encode_to_symbols(uint32_t fixed, uint32_t rolling) {
    // Validate fixed code range (3^20 = 3,486,784,401)
    if (fixed > 0xCFD41B90) {
        return false;
    }

    uint32_t original_fixed = fixed;
    uint32_t original_rolling = rolling;

    rolling += 2;
    encoded_rolling = rolling;

    if (rolling == 0xFFFFFFFF) {
        rolling = 0xE6000000;
        encoded_rolling = rolling;
    }

    uint32_t rolling_reversed = reverse_bits_32(rolling);

    uint8_t rolling_array[20] = {0};
    uint8_t fixed_array[20] = {0};

    uint32_t rolling_temp = rolling_reversed;
    uint32_t fixed_temp = fixed;

    for (int i = 19; i >= 0; i--) {
        rolling_array[i] = rolling_temp % 3;
        rolling_temp /= 3;
        fixed_array[i] = fixed_temp % 3;
        fixed_temp /= 3;
    }

    data_array[PACKET_1_INDEX_BASE] = PACKET_1_HEADER;
    data_array[PACKET_2_INDEX_BASE] = PACKET_2_HEADER;

    // Encode Packet 1: interleave rolling and fixed codes with accumulator
    uint32_t acc = 0;
    for (uint8_t i = 1; i < 11; i++) {
        acc += rolling_array[i - 1];
        data_array[i * 2 - 1] = rolling_array[i - 1];

        acc += fixed_array[i - 1];
        data_array[i * 2] = acc % 3;
    }

    // Encode Packet 2
    acc = 0;
    for (uint8_t i = 11; i < 21; i++) {
        acc += rolling_array[i - 1];
        data_array[i * 2] = rolling_array[i - 1];

        acc += fixed_array[i - 1];
        data_array[i * 2 + 1] = acc % 3;
    }

    uint8_t btn = original_fixed % 3;
    uint8_t id0 = (original_fixed / 3) % 3;
    uint8_t id1 = (original_fixed / 9) % 3;
    uint32_t serial = original_fixed / 27;

    const char* btn_name = (btn == 1) ? "left" : (btn == 0) ? "middle" : "right";

    return true;
}

/**
 * Decode 40 ternary symbols back to fixed and rolling codes
 * Based on Flipper Zero's subghz_protocol_secplus_v1_decode()
 */
bool SecPlusV1Protocol::decode_from_symbols(uint32_t& fixed, uint32_t& rolling) {
    uint8_t rolling_array[20] = {0};
    uint8_t fixed_array[20] = {0};

    uint32_t acc = 0;
    for (uint8_t i = 1; i < 11; i++) {
        rolling_array[i - 1] = data_array[i * 2 - 1];
        acc += rolling_array[i - 1];

        fixed_array[i - 1] = (data_array[i * 2] + 3 - (acc % 3)) % 3;
        acc += fixed_array[i - 1];
    }

    acc = 0;
    for (uint8_t i = 11; i < 21; i++) {
        rolling_array[i - 1] = data_array[i * 2];
        acc += rolling_array[i - 1];

        fixed_array[i - 1] = (data_array[i * 2 + 1] + 3 - (acc % 3)) % 3;
        acc += fixed_array[i - 1];
    }

    uint32_t rolling_temp = 0;
    uint32_t fixed_temp = 0;

    for (int i = 0; i < 20; i++) {
        rolling_temp = rolling_temp * 3 + rolling_array[i];
        fixed_temp = fixed_temp * 3 + fixed_array[i];
    }

    rolling = reverse_bits_32(rolling_temp);
    fixed = fixed_temp;

    return true;
}

/**
 * Generate RMT items from encoded symbol array
 */
bool SecPlusV1Protocol::generate_rmt_items() {
    rmt_packet1_with_preamble.clear();
    rmt_packet1_no_preamble.clear();
    rmt_packet2_items.clear();

    rmt_packet1_with_preamble.reserve(24);
    rmt_packet1_no_preamble.reserve(23);
    rmt_packet2_items.reserve(23);

    rmt_item32_t preamble;
    preamble.duration0 = US_TO_RMT_TICKS(SECPLUS_V1_PREAMBLE_HIGH);
    preamble.level0 = 1;
    preamble.duration1 = US_TO_RMT_TICKS(SECPLUS_V1_PREAMBLE_LOW);
    preamble.level1 = 0;

    // Packet 1 header: 59,500us low (split: 32767 + 26733), 500us high
    rmt_item32_t p1_header_1;
    p1_header_1.duration0 = 32767;
    p1_header_1.level0 = 0;
    p1_header_1.duration1 = 26733;
    p1_header_1.level1 = 0;

    rmt_item32_t p1_header_2;
    p1_header_2.duration0 = US_TO_RMT_TICKS(SECPLUS_V1_TE_SHORT);
    p1_header_2.level0 = 1;
    p1_header_2.duration1 = 0;
    p1_header_2.level1 = 0;

    // Packet 2 header: 58,000us low (split: 32767 + 25233), 1500us high
    rmt_item32_t p2_header_1;
    p2_header_1.duration0 = 32767;
    p2_header_1.level0 = 0;
    p2_header_1.duration1 = 25233;
    p2_header_1.level1 = 0;

    rmt_item32_t p2_header_2;
    p2_header_2.duration0 = US_TO_RMT_TICKS(SECPLUS_V1_TE_SHORT * 3);
    p2_header_2.level0 = 1;
    p2_header_2.duration1 = 0;
    p2_header_2.level1 = 0;

    rmt_packet1_with_preamble.push_back(preamble);
    rmt_packet1_with_preamble.push_back(p1_header_1);
    rmt_packet1_with_preamble.push_back(p1_header_2);

    rmt_packet1_no_preamble.push_back(p1_header_1);
    rmt_packet1_no_preamble.push_back(p1_header_2);

    for (uint8_t i = PACKET_1_INDEX_BASE + 1; i < PACKET_1_INDEX_BASE + 21; i++) {
        rmt_item32_t item;

        switch (data_array[i]) {
            case SYMBOL_0:  // 3L + 1H
                item.duration0 = US_TO_RMT_TICKS(SECPLUS_V1_TE_SHORT * 3);
                item.level0 = 0;
                item.duration1 = US_TO_RMT_TICKS(SECPLUS_V1_TE_SHORT);
                item.level1 = 1;
                break;

            case SYMBOL_1:  // 2L + 2H
                item.duration0 = US_TO_RMT_TICKS(SECPLUS_V1_TE_SHORT * 2);
                item.level0 = 0;
                item.duration1 = US_TO_RMT_TICKS(SECPLUS_V1_TE_SHORT * 2);
                item.level1 = 1;
                break;

            case SYMBOL_2:  // 1L + 3H
                item.duration0 = US_TO_RMT_TICKS(SECPLUS_V1_TE_SHORT);
                item.level0 = 0;
                item.duration1 = US_TO_RMT_TICKS(SECPLUS_V1_TE_SHORT * 3);
                item.level1 = 1;
                break;

            default:
                return false;
        }

        rmt_packet1_with_preamble.push_back(item);
        rmt_packet1_no_preamble.push_back(item);
    }

    rmt_packet2_items.push_back(p2_header_1);
    rmt_packet2_items.push_back(p2_header_2);

    for (uint8_t i = PACKET_2_INDEX_BASE + 1; i < PACKET_2_INDEX_BASE + 21; i++) {
        rmt_item32_t item;

        switch (data_array[i]) {
            case SYMBOL_0:  // 3L + 1H
                item.duration0 = US_TO_RMT_TICKS(SECPLUS_V1_TE_SHORT * 3);
                item.level0 = 0;
                item.duration1 = US_TO_RMT_TICKS(SECPLUS_V1_TE_SHORT);
                item.level1 = 1;
                break;

            case SYMBOL_1:  // 2L + 2H
                item.duration0 = US_TO_RMT_TICKS(SECPLUS_V1_TE_SHORT * 2);
                item.level0 = 0;
                item.duration1 = US_TO_RMT_TICKS(SECPLUS_V1_TE_SHORT * 2);
                item.level1 = 1;
                break;

            case SYMBOL_2:  // 1L + 3H
                item.duration0 = US_TO_RMT_TICKS(SECPLUS_V1_TE_SHORT);
                item.level0 = 0;
                item.duration1 = US_TO_RMT_TICKS(SECPLUS_V1_TE_SHORT * 3);
                item.level1 = 1;
                break;

            default:
                return false;
        }

        rmt_packet2_items.push_back(item);
    }

    rmt_item32_t end_marker;
    end_marker.level0 = 0;
    end_marker.duration0 = 0;
    end_marker.level1 = 0;
    end_marker.duration1 = 0;

    rmt_packet1_with_preamble.push_back(end_marker);
    rmt_packet1_no_preamble.push_back(end_marker);
    rmt_packet2_items.push_back(end_marker);

    return true;
}

// ===== Public Interface Implementation =====

bool SecPlusV1Protocol::decode(const rmt_item32_t* data, size_t len, ProtocolDecodeResult& result) {
    // Check if we have both packets decoded
    if (packet_accepted != (PACKET_1_ACCEPTED | PACKET_2_ACCEPTED)) {
        return false;
    }

    // Decode symbols to fixed and rolling codes
    uint32_t fixed, rolling;
    if (!decode_from_symbols(fixed, rolling)) {
        return false;
    }

    // Populate result
    result.valid = true;
    result.key = ((uint64_t)fixed << 32) | rolling;
    result.bit_count = 42;
    result.te = SECPLUS_V1_TE_SHORT;

    // Calculate button and serial (for display)
    uint8_t btn = fixed % 3;
    uint32_t serial = fixed / 27;

    const char* btn_name = (btn == 1) ? "left" : (btn == 0) ? "middle" : "right";

    // Store extra information
    char extra[128];
    snprintf(extra, sizeof(extra),
             "Fix:0x%08X Cnt:0x%08X Btn:%s Serial:%u",
             fixed, rolling, btn_name, serial);
    result.extra_data = String(extra);

    // Reset for next decode
    packet_accepted = 0;

    return true;
}

bool SecPlusV1Protocol::encode(const ProtocolEncodeParams& params) {
    // Extract fixed (upper 32 bits) and rolling (lower 32 bits) from 64-bit key
    uint32_t fixed = (params.key >> 32) & 0xFFFFFFFF;
    uint32_t rolling = params.key & 0xFFFFFFFF;

    // Store original fixed value
    encoded_fixed = fixed;
    // encoded_rolling will be set by encode_to_symbols (incremented value)

    // Convert to symbols (this will increment rolling and update encoded_rolling)
    if (!encode_to_symbols(fixed, rolling)) {
        return false;
    }

    // Generate RMT items
    if (!generate_rmt_items()) {
        return false;
    }

    return true;
}

bool SecPlusV1Protocol::encodeFromCodes(uint32_t fixed, uint32_t rolling) {
    ProtocolEncodeParams params;
    params.key = ((uint64_t)fixed << 32) | rolling;
    params.bit_count = 42;  // Flipper uses 42 bits for Security+ 1.0
    params.te = SECPLUS_V1_TE_SHORT;
    params.repeat_count = 0;  // Use default

    return encode(params);
}

bool SecPlusV1Protocol::getEncodedData(const rmt_item32_t** out_data, size_t* out_len) {
    if (rmt_packet1_with_preamble.empty()) {
        return false;
    }

    *out_data = rmt_packet1_with_preamble.data();
    *out_len = rmt_packet1_with_preamble.size();
    return true;
}

bool SecPlusV1Protocol::getPacket1NoPreamble(const rmt_item32_t** out_data, size_t* out_len) {
    if (rmt_packet1_no_preamble.empty()) {
        return false;
    }

    *out_data = rmt_packet1_no_preamble.data();
    *out_len = rmt_packet1_no_preamble.size();
    return true;
}

bool SecPlusV1Protocol::getPacket2Data(const rmt_item32_t** out_data, size_t* out_len) {
    if (rmt_packet2_items.empty()) {
        return false;
    }

    *out_data = rmt_packet2_items.data();
    *out_len = rmt_packet2_items.size();
    return true;
}

bool SecPlusV1Protocol::serializeToFile(File& file, const ProtocolDecodeResult& result) {
    if (!file) {
        return false;
    }

    // Write in Flipper Zero format
    file.printf("Protocol: %s\n", getName());
    file.printf("Bit: %d\n", result.bit_count);

    // Format key as space-separated hex bytes (big-endian)
    file.print("Key: ");
    for (int i = 7; i >= 0; i--) {
        file.printf("%02X", (uint8_t)((result.key >> (i * 8)) & 0xFF));
        if (i > 0) file.print(" ");
    }
    file.println();

    return true;
}

bool SecPlusV1Protocol::parse_sub_file(File& file, uint32_t& fixed, uint32_t& rolling) {
    // Parse Flipper .sub file format
    // Expected fields:
    // Bit: 42
    // Key: XX XX XX XX XX XX XX XX
    // Fix: 0xXXXXXXXX (optional)
    // Cnt: 0xXXXXXXXX (optional)

    char line[128];
    bool has_key = false;
    bool has_fix = false;
    bool has_cnt = false;
    uint64_t key = 0;

    while (file.available()) {
        size_t len = file.readBytesUntil('\n', line, sizeof(line) - 1);
        line[len] = '\0';

        // Trim trailing whitespace
        while (len > 0 && (line[len-1] == '\r' || line[len-1] == '\n' || line[len-1] == ' ')) {
            line[--len] = '\0';
        }

        if (len == 0) continue;

        // Parse Key field (hex bytes)
        if (strncmp(line, "Key:", 4) == 0) {
            // Parse hex bytes: "Key: XX XX XX XX XX XX XX XX"
            uint8_t key_bytes[8] = {0};
            int bytes_read = sscanf(line + 4, " %hhx %hhx %hhx %hhx %hhx %hhx %hhx %hhx",
                                   &key_bytes[0], &key_bytes[1], &key_bytes[2], &key_bytes[3],
                                   &key_bytes[4], &key_bytes[5], &key_bytes[6], &key_bytes[7]);

            if (bytes_read >= 5) {  // At least 5 bytes for valid Security+ 1.0
                // Convert bytes to 64-bit key (big-endian)
                key = 0;
                for (int i = 0; i < bytes_read; i++) {
                    key = (key << 8) | key_bytes[i];
                }
                has_key = true;
            }
        }
        // Parse Fix field (optional)
        else if (strncmp(line, "Fix:", 4) == 0) {
            if (sscanf(line + 4, " 0x%X", &fixed) == 1) {
                has_fix = true;
            }
        }
        // Parse Cnt field (optional)
        else if (strncmp(line, "Cnt:", 4) == 0) {
            if (sscanf(line + 4, " 0x%X", &rolling) == 1) {
                has_cnt = true;
            }
        }
    }

    // If we have explicit Fix and Cnt, use those
    if (has_fix && has_cnt) {
        return true;
    }

    // Otherwise extract from Key
    if (has_key) {
        fixed = (key >> 32) & 0xFFFFFFFF;
        rolling = key & 0xFFFFFFFF;
        return true;
    }

    return false;
}

bool SecPlusV1Protocol::deserializeFromFile(File& file, ProtocolEncodeParams& params) {
    uint32_t fixed = 0;
    uint32_t rolling = 0;

    if (!parse_sub_file(file, fixed, rolling)) {
        return false;
    }

    // Populate encode parameters
    params.key = ((uint64_t)fixed << 32) | rolling;
    params.bit_count = 42;  // Security+ 1.0 uses 42 bits in Flipper format
    params.te = SECPLUS_V1_TE_SHORT;
    params.repeat_count = 0;  // Use default


    return true;
}

/**
 * State machine feed function - processes each RF level transition
 * Called by SubGHz scanner with (level, duration) pairs
 */
void SecPlusV1Protocol::feed(bool level, uint32_t duration) {
    static int edge_count = 0;
    static bool debug_enabled = false;

    // Enable debug after first edge
    if (edge_count == 0) {
        debug_enabled = true;
    }
    edge_count++;

    switch (decoder_step) {
        case STEP_RESET:
            // Looking for packet header (long LOW pulse)
            // Packet 1: ~59.5ms (119 x 500us), Packet 2: ~58ms (116 x 500us)
            // IMPORTANT: RMT fragmentation means we often see only 4-8ms of the header
            // Accept any LOW pulse >= 3ms as a potential header fragment
            if (!level && duration >= 3000) {
                // Found potential header - reset state for new packet
                decoder_step = STEP_SEARCH_START_BIT;
                decode_count_bit = 0;
                edge_count = 0;  // Reset for next packet
                debug_enabled = false;  // Disable until next attempt
            }
            break;

        case STEP_SEARCH_START_BIT:
            // Packet header HIGH pulse determines packet type
            if (level) {
                if (duration_match(duration, SECPLUS_V1_TE_SHORT, SECPLUS_V1_TOLERANCE)) {
                    // Short HIGH = Packet 1 (500us)
                    base_packet_index = PACKET_1_INDEX_BASE;
                    data_array[PACKET_1_INDEX_BASE] = PACKET_1_HEADER;
                    decoder_step = STEP_SAVE_DURATION;
                    decode_count_bit = 1;  // Header counts as first symbol
                }
                else if (duration_match(duration, SECPLUS_V1_TE_LONG, SECPLUS_V1_TOLERANCE)) {
                    // Long HIGH = Packet 2 (1500us)
                    base_packet_index = PACKET_2_INDEX_BASE;
                    data_array[PACKET_2_INDEX_BASE] = PACKET_2_HEADER;
                    decoder_step = STEP_SAVE_DURATION;
                    decode_count_bit = 1;  // Header counts as first symbol
                }
                else {
                    decoder_step = STEP_RESET;
                }
            }
            else {
                decoder_step = STEP_RESET;
            }
            break;

        case STEP_SAVE_DURATION:
            // Save LOW duration for next state
            if (!level) {
                te_last = duration;
                decoder_step = STEP_DECODE_DATA;
            }
            else {
                decoder_step = STEP_RESET;
            }
            break;

        case STEP_DECODE_DATA:
            // Decode tri-bit symbols from LOW+HIGH duration pairs
            if (level && decode_count_bit < 21) {
                uint8_t symbol = SYMBOL_INVALID;

                // Symbol 0: 3L + 1H (1500us LOW, 500us HIGH)
                if (duration_match(te_last, SECPLUS_V1_TE_SHORT * 3, SECPLUS_V1_TOLERANCE * 3) &&
                    duration_match(duration, SECPLUS_V1_TE_SHORT, SECPLUS_V1_TOLERANCE)) {
                    symbol = SYMBOL_0;
                }
                // Symbol 1: 2L + 2H (1000us LOW, 1000us HIGH)
                else if (duration_match(te_last, SECPLUS_V1_TE_SHORT * 2, SECPLUS_V1_TOLERANCE * 2) &&
                         duration_match(duration, SECPLUS_V1_TE_SHORT * 2, SECPLUS_V1_TOLERANCE * 2)) {
                    symbol = SYMBOL_1;
                }
                // Symbol 2: 1L + 3H (500us LOW, 1500us HIGH)
                else if (duration_match(te_last, SECPLUS_V1_TE_SHORT, SECPLUS_V1_TOLERANCE) &&
                         duration_match(duration, SECPLUS_V1_TE_SHORT * 3, SECPLUS_V1_TOLERANCE * 3)) {
                    symbol = SYMBOL_2;
                }

                if (symbol != SYMBOL_INVALID) {
                    data_array[base_packet_index + decode_count_bit] = symbol;
                    decode_count_bit++;
                    decoder_step = STEP_SAVE_DURATION;
                }
                else {
                    decoder_step = STEP_RESET;
                }

                // Check if packet complete (21 symbols: 1 header + 20 data)
                if (decode_count_bit == 21) {
                    if (base_packet_index == PACKET_1_INDEX_BASE) {
                        packet_accepted |= PACKET_1_ACCEPTED;
                    }
                    else if (base_packet_index == PACKET_2_INDEX_BASE) {
                        packet_accepted |= PACKET_2_ACCEPTED;
                    }

                    // If both packets received, decode is ready
                    // Don't reset here - let decode_check() handle the reset after returning result
                    if (packet_accepted == (PACKET_1_ACCEPTED | PACKET_2_ACCEPTED)) {
                    }

                    decoder_step = STEP_RESET;
                }
            }
            else {
                decoder_step = STEP_RESET;
            }
            break;
    }
}
