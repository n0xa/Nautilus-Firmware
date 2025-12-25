/**
 * Security+ 2.0 Protocol Implementation
 *
 * Based on the reverse-engineered protocol by argilo:
 * https://github.com/argilo/secplus
 *
 * And Flipper Zero implementation:
 * https://github.com/flipperdevices/flipperzero-firmware/blob/dev/lib/subghz/protocols/secplus_v2.c
 */

#include "protocol_secplus_v2.h"
#include <Arduino.h>

/**
 * Helper functions for Security+ 2.0 encoding/decoding
 * Ported from Flipper Zero Momentum firmware
 * Reference: https://github.com/Next-Flip/Momentum-Firmware
 */

// Bit read helper
static inline uint8_t bit_read(uint64_t value, uint8_t bit) {
    return (value >> bit) & 1;
}

/**
 * Apply selective bitwise inversion to the three 10-bit buffers
 * Based on 4-bit invert value extracted from packet
 */
static bool mix_invert(uint8_t invert, uint16_t p[3]) {
    switch(invert) {
    case 0x00: // (True, True, False)
        p[0] = ~p[0] & 0x03FF;
        p[1] = ~p[1] & 0x03FF;
        break;
    case 0x01: // (False, True, False)
        p[1] = ~p[1] & 0x03FF;
        break;
    case 0x02: // (False, False, True)
        p[2] = ~p[2] & 0x03FF;
        break;
    case 0x04: // (True, True, True)
        p[0] = ~p[0] & 0x03FF;
        p[1] = ~p[1] & 0x03FF;
        p[2] = ~p[2] & 0x03FF;
        break;
    case 0x05: // (True, False, True)
    case 0x0A: // (True, False, True)
        p[0] = ~p[0] & 0x03FF;
        p[2] = ~p[2] & 0x03FF;
        break;
    case 0x06: // (False, True, True)
        p[1] = ~p[1] & 0x03FF;
        p[2] = ~p[2] & 0x03FF;
        break;
    case 0x08: // (True, False, False)
        p[0] = ~p[0] & 0x03FF;
        break;
    case 0x09: // (False, False, False)
        break;
    default:
        return false;
    }
    return true;
}

/**
 * Reorder buffers for decoding (reverse permutation)
 */
static bool mix_order_decode(uint8_t order, uint16_t p[3]) {
    uint16_t a = p[0], b = p[1], c = p[2];

    switch(order) {
    case 0x06: // [2, 1, 0]
    case 0x09: // [2, 1, 0]
        p[2] = a;
        // p[1]: no change
        p[0] = c;
        break;
    case 0x08: // [1, 2, 0]
    case 0x04: // [1, 2, 0]
        p[1] = a;
        p[2] = b;
        p[0] = c;
        break;
    case 0x01: // [2, 0, 1]
        p[2] = a;
        p[0] = b;
        p[1] = c;
        break;
    case 0x00: // [0, 2, 1]
        // p[0]: no change
        p[2] = b;
        p[1] = c;
        break;
    case 0x05: // [1, 0, 2]
        p[1] = a;
        p[0] = b;
        // p[2]: no change
        break;
    case 0x02: // [0, 1, 2]
    case 0x0A: // [0, 1, 2]
        // no reordering
        break;
    default:
        return false;
    }
    return true;
}

/**
 * Reorder buffers for encoding (forward permutation)
 */
static bool mix_order_encode(uint8_t order, uint16_t p[3]) {
    uint16_t a, b, c;

    switch(order) {
    case 0x06: // [2, 1, 0]
    case 0x09: // [2, 1, 0]
        a = p[2];
        b = p[1];
        c = p[0];
        break;
    case 0x08: // [1, 2, 0]
    case 0x04: // [1, 2, 0]
        a = p[1];
        b = p[2];
        c = p[0];
        break;
    case 0x01: // [2, 0, 1]
        a = p[2];
        b = p[0];
        c = p[1];
        break;
    case 0x00: // [0, 2, 1]
        a = p[0];
        b = p[2];
        c = p[1];
        break;
    case 0x05: // [1, 0, 2]
        a = p[1];
        b = p[0];
        c = p[2];
        break;
    case 0x02: // [0, 1, 2]
    case 0x0A: // [0, 1, 2]
        a = p[0];
        b = p[1];
        c = p[2];
        break;
    default:
        return false;
    }

    p[0] = a;
    p[1] = b;
    p[2] = c;
    return true;
}

/**
 * Decode half-packet (38 bits data -> roll_array + fixed)
 * Ported from Flipper Zero implementation
 */
static bool decode_half(uint64_t data, uint8_t roll_array[9], uint32_t* fixed) {
    uint8_t order = (data >> 34) & 0x0F;
    uint8_t invert = (data >> 30) & 0x0F;
    uint16_t p[3] = {0};

    // De-interleave 30 bits into three 10-bit buffers
    for(int i = 29; i >= 0; i -= 3) {
        p[0] = (p[0] << 1) | bit_read(data, i);
        p[1] = (p[1] << 1) | bit_read(data, i - 1);
        p[2] = (p[2] << 1) | bit_read(data, i - 2);
    }

    // Apply inverse transformations
    if(!mix_invert(invert, p)) return false;
    if(!mix_order_decode(order, p)) return false;

    // Extract roll array from order and invert (4 ternary digits = 8 bits)
    uint64_t combined = (order << 4) | invert;
    int k = 0;
    for(int i = 6; i >= 0; i -= 2) {
        roll_array[k] = (combined >> i) & 0x03;
        if(roll_array[k] == 3) {
            return false;
        }
        k++;
    }

    // Extract remaining 5 ternary digits from p[2]
    for(int i = 8; i >= 0; i -= 2) {
        roll_array[k] = (p[2] >> i) & 0x03;
        if(roll_array[k] == 3) {
            return false;
        }
        k++;
    }

    // Combine p[0] and p[1] into 20-bit fixed code
    *fixed = (p[0] << 10) | p[1];
    return true;
}

/**
 * Encode half-packet (roll_array + fixed -> 38 bits data)
 * Ported from Flipper Zero implementation
 */
static uint64_t encode_half(uint8_t roll_array[9], uint32_t fixed) {
    uint64_t data = 0;
    uint16_t p[3] = {(fixed >> 10) & 0x3FF, fixed & 0x3FF, 0};

    // First 4 ternary digits encode order and invert
    uint8_t order = (roll_array[0] << 2) | roll_array[1];
    uint8_t invert = (roll_array[2] << 2) | roll_array[3];

    // Last 5 ternary digits encode into p[2]
    p[2] = ((uint16_t)roll_array[4] << 8) | (roll_array[5] << 6) |
           (roll_array[6] << 4) | (roll_array[7] << 2) | roll_array[8];

    // Apply transformations
    if(!mix_order_encode(order, p)) return 0;
    if(!mix_invert(invert, p)) return 0;

    // Interleave three 10-bit buffers into 30 bits
    for(int i = 0; i < 10; i++) {
        data <<= 3;
        data |= (bit_read(p[0], 9 - i) << 2) |
                (bit_read(p[1], 9 - i) << 1) |
                 bit_read(p[2], 9 - i);
    }

    // Prepend order and invert (8 bits total)
    data |= ((uint64_t)order << 34) | ((uint64_t)invert << 30);

    return data;
}

// Note: Base-3 and bit-reverse functions removed - Flipper implementation
// uses direct ternary digit encoding in roll_array instead

/**
 * Wrapper for encode_half - kept for API compatibility
 */
uint64_t SecPlusV2Protocol::scramble_half_packet(uint32_t fixed, const uint8_t roll[9], uint16_t data) {
    uint8_t roll_copy[9];
    memcpy(roll_copy, roll, 9);
    return encode_half(roll_copy, fixed);
}

/**
 * Wrapper for decode_half - kept for API compatibility
 */
bool SecPlusV2Protocol::unscramble_half_packet(uint64_t packet, uint32_t& fixed, uint8_t roll[9], uint16_t& data) {
    data = 0;  // v2.0 doesn't use data field
    return decode_half(packet, roll, &fixed);
}

/**
 * Manchester decode from RMT items
 */
size_t SecPlusV2Protocol::manchester_decode(const rmt_item32_t* data, size_t len, uint8_t* bits, size_t max_bits) {
    size_t bit_count = 0;

    for (size_t i = 0; i < len && bit_count < max_bits; i++) {
        uint32_t dur0 = rmt_item_to_us(data[i], true);
        uint32_t dur1 = rmt_item_to_us(data[i], false);

        bool is_short0 = pulse_matches(dur0, SECPLUS_V2_TE_SHORT, SECPLUS_V2_TOLERANCE);
        bool is_short1 = pulse_matches(dur1, SECPLUS_V2_TE_SHORT, SECPLUS_V2_TOLERANCE);

        if (is_short0 && is_short1) {
            // Inverted Manchester: level0=HIGH => bit=1, level0=LOW => bit=0
            uint8_t bit = data[i].level0 ? 1 : 0;
            bits[bit_count / 8] |= (bit << (7 - (bit_count % 8)));
            bit_count++;
        }
    }

    return bit_count;
}

/**
 * Manchester encode to RMT items
 */
size_t SecPlusV2Protocol::manchester_encode(const uint8_t* bits, size_t bit_count, std::vector<rmt_item32_t>& items) {
    items.clear();
    items.reserve(bit_count);

    for (size_t i = 0; i < bit_count; i++) {
        uint8_t bit = (bits[i / 8] >> (7 - (i % 8))) & 1;

        rmt_item32_t item;
        if (bit) {
            item.level0 = 0;
            item.duration0 = US_TO_RMT_TICKS(SECPLUS_V2_TE_SHORT);
            item.level1 = 1;
            item.duration1 = US_TO_RMT_TICKS(SECPLUS_V2_TE_SHORT);
        } else {
            item.level0 = 1;
            item.duration0 = US_TO_RMT_TICKS(SECPLUS_V2_TE_SHORT);
            item.level1 = 0;
            item.duration1 = US_TO_RMT_TICKS(SECPLUS_V2_TE_SHORT);
        }

        items.push_back(item);
    }

    return items.size();
}

/**
 * Decode Security+ 2.0 signal
 */
bool SecPlusV2Protocol::decode(const rmt_item32_t* data, size_t len, ProtocolDecodeResult& result) {

    if (data == nullptr || len < getMinimumLength()) {
        return false;
    }

    result.valid = false;

    uint8_t bit_buffer[16] = {0};
    size_t bit_count = manchester_decode(data, len, bit_buffer, 128);

    bool is_single_packet = (bit_count >= 42 && bit_count < 82);
    bool is_dual_packet = (bit_count >= 82);

    if (!is_single_packet && !is_dual_packet) {
        return false;
    }

    uint8_t frame0 = (bit_buffer[0] >> 7) & 1;
    uint8_t frame1 = (bit_buffer[0] >> 6) & 1;
    uint8_t frame = (frame0 << 1) | frame1;

    uint64_t current_packet = 0;
    for (int i = 0; i < 40; i++) {
        int bit_idx = 2 + i;
        uint8_t bit = (bit_buffer[bit_idx / 8] >> (7 - (bit_idx % 8))) & 1;
        current_packet = (current_packet << 1) | bit;
    }

    uint32_t fixed_tmp;
    uint8_t roll_tmp[9];
    uint16_t data_tmp;
    if (!unscramble_half_packet(current_packet, fixed_tmp, roll_tmp, data_tmp)) {
        return false;
    }

    if (is_dual_packet) {
        uint64_t packet1, packet2;

        if (frame == 0x00) {
            packet1 = current_packet;
            packet2 = 0;
            for (int i = 0; i < 40; i++) {
                int bit_idx = 44 + i;
                uint8_t bit = (bit_buffer[bit_idx / 8] >> (7 - (bit_idx % 8))) & 1;
                packet2 = (packet2 << 1) | bit;
            }
        } else if (frame == 0x01) {
            packet2 = current_packet;
            packet1 = 0;
            for (int i = 0; i < 40; i++) {
                int bit_idx = 44 + i;
                uint8_t bit = (bit_buffer[bit_idx / 8] >> (7 - (bit_idx % 8))) & 1;
                packet1 = (packet1 << 1) | bit;
            }
        } else {
            packet1 = current_packet;
            packet2 = 0;
        }

        return decode_packet_pair(packet1, packet2, result);
    }

    unsigned long now = millis();

    if (pending_packet.valid && (now - pending_packet.timestamp) > PACKET_PAIR_TIMEOUT_MS) {
        pending_packet.valid = false;
    }

    if (pending_packet.valid && pending_packet.frame != frame) {

        uint64_t packet1, packet2;
        if (pending_packet.frame == 0x00 && frame == 0x01) {
            packet1 = pending_packet.packet;
            packet2 = current_packet;
        } else if (pending_packet.frame == 0x01 && frame == 0x00) {
            packet1 = current_packet;
            packet2 = pending_packet.packet;
        } else {
            pending_packet.valid = false;
            goto store_pending;
        }

        pending_packet.valid = false;
        return decode_packet_pair(packet1, packet2, result);
    }

store_pending:
    pending_packet.packet = current_packet;
    pending_packet.frame = frame;
    pending_packet.timestamp = now;
    pending_packet.valid = true;

    return false;
}

/**
 * Helper: Decode a complete packet pair (frame 00 + frame 01)
 */
bool SecPlusV2Protocol::decode_packet_pair(uint64_t packet1, uint64_t packet2,
                                            ProtocolDecodeResult& result) {
    // Unscramble both packets
    uint32_t fixed1, fixed2;
    uint8_t roll1[9], roll2[9];
    uint16_t data1, data2;

    bool valid1 = unscramble_half_packet(packet1, fixed1, roll1, data1);
    bool valid2 = unscramble_half_packet(packet2, fixed2, roll2, data2);

    if (!valid1 || !valid2) {
        return false;
    }

    // Reconstruct rolling code using Flipper's exact method (if we have both packets)
    uint32_t rolling = 0;
    if(valid2) {
        // 1. Arrange 18 ternary digits in specific order
        uint8_t rolling_digits[18];
        rolling_digits[0] = roll2[8];
        rolling_digits[1] = roll1[8];
        rolling_digits[2] = roll2[4];
        rolling_digits[3] = roll2[5];
        rolling_digits[4] = roll2[6];
        rolling_digits[5] = roll2[7];
        rolling_digits[6] = roll1[4];
        rolling_digits[7] = roll1[5];
        rolling_digits[8] = roll1[6];
        rolling_digits[9] = roll1[7];
        rolling_digits[10] = roll2[0];
        rolling_digits[11] = roll2[1];
        rolling_digits[12] = roll2[2];
        rolling_digits[13] = roll2[3];
        rolling_digits[14] = roll1[0];
        rolling_digits[15] = roll1[1];
        rolling_digits[16] = roll1[2];
        rolling_digits[17] = roll1[3];

        // 2. Convert from base-3 to 28-bit integer
        uint32_t rolling_temp = 0;
        for(int i = 0; i < 18; i++) {
            rolling_temp = (rolling_temp * 3) + rolling_digits[i];
        }

        // 3. Bit-reverse to get final counter
        for(int i = 0; i < 28; i++) {
            rolling = (rolling << 1) | (rolling_temp & 1);
            rolling_temp >>= 1;
        }
    }

    // Reconstruct fixed code (40 bits from two 20-bit halves)
    uint64_t fixed = ((uint64_t)fixed1 << 20) | fixed2;

    // Extract button code from upper 4 bits of fixed code (bits 36-39)
    uint8_t button = (fixed >> 36) & 0xF;

    // Extract serial number from lower 36 bits of fixed code
    uint64_t serial = fixed & 0xFFFFFFFFFULL;  // 36-bit mask

    // Store results
    result.valid = true;
    result.key = fixed;  // Store full fixed code (button + serial) in key field
    result.bit_count = SECPLUS_V2_BIT_COUNT;
    result.te = SECPLUS_V2_TE_SHORT;

    // Store rolling code and button in extra_data for display
    char extra_str[64];
    snprintf(extra_str, sizeof(extra_str), "Button: 0x%X  Counter: 0x%07X", button, rolling);
    result.extra_data = String(extra_str);

    // Save for later retrieval
    encoded_packet1 = packet1;
    encoded_packet2 = packet2;
    encoded_fixed = fixed;
    encoded_rolling = rolling;
    encoded_button = button;
    encoded_serial = serial;

    return true;
}

/**
 * Encode Security+ 2.0 signal from ProtocolEncodeParams
 */
bool SecPlusV2Protocol::encode(const ProtocolEncodeParams& params) {
    // Extract fixed (40 bits) and rolling (28 bits) from extra_data
    // For basic encode, we expect key = fixed code
    // Rolling code should be passed separately via encodeFromCodes()
    return false;
}

/**
 * Encode from fixed + rolling codes
 */
bool SecPlusV2Protocol::encodeFromCodes(uint64_t fixed, uint32_t rolling) {
    if (fixed >= (1ULL << SECPLUS_V2_FIXED_BITS)) {
        return false;
    }

    if (rolling >= (1UL << SECPLUS_V2_ROLLING_BITS)) {
        return false;
    }

    // Encode rolling counter using Flipper's method (reverse of decode)
    // 1. Bit-reverse the counter to get the base-3 representation
    uint32_t rolling_reversed = 0;
    uint32_t temp = rolling;
    for(int i = 0; i < 28; i++) {
        rolling_reversed = (rolling_reversed << 1) | (temp & 1);
        temp >>= 1;
    }

    // 2. Convert to 18 base-3 digits
    uint8_t rolling_digits[18];
    for(int i = 17; i >= 0; i--) {
        rolling_digits[i] = rolling_reversed % 3;
        rolling_reversed /= 3;
    }

    // 3. Distribute digits to roll1 and roll2 arrays (reverse of decode order)
    uint8_t roll1[9], roll2[9];
    roll2[8] = rolling_digits[0];
    roll1[8] = rolling_digits[1];
    roll2[4] = rolling_digits[2];
    roll2[5] = rolling_digits[3];
    roll2[6] = rolling_digits[4];
    roll2[7] = rolling_digits[5];
    roll1[4] = rolling_digits[6];
    roll1[5] = rolling_digits[7];
    roll1[6] = rolling_digits[8];
    roll1[7] = rolling_digits[9];
    roll2[0] = rolling_digits[10];
    roll2[1] = rolling_digits[11];
    roll2[2] = rolling_digits[12];
    roll2[3] = rolling_digits[13];
    roll1[0] = rolling_digits[14];
    roll1[1] = rolling_digits[15];
    roll1[2] = rolling_digits[16];
    roll1[3] = rolling_digits[17];

    // Split fixed code into two 20-bit halves
    uint32_t fixed1 = (fixed >> 20) & 0xFFFFF;
    uint32_t fixed2 = fixed & 0xFFFFF;

    // Scramble both packets - these are the complete packet values
    encoded_packet1 = scramble_half_packet(fixed1, roll1, 0);
    encoded_packet2 = scramble_half_packet(fixed2, roll2, 0);

    // Security+ 2.0 requires TWO separate bursts with a gap between them
    // We'll store them as separate RMT arrays and transmit with delay between
    // Each burst: frame bits (2) + packet data (40 bits)
    // Gap between bursts: 136 x te_long = 136 x 500us = 68ms

    // Note: We only encode packet 1 here. Packet 2 will be transmitted
    // separately by the transmit function with a delay between them.

    // === BURST 1: Packet 1 ===
    uint8_t packet1_buffer[16] = {0};
    int bit_idx = 0;

    // Flipper format (decoded from cropped URH captures):
    // Clean packets show 123 Manchester bits = 61.5 data bits
    // Decoded structure: 1111111111111111 0000 XX DDDDDDDD...
    //   - 16 bits: all 1s (sync)
    //   - 4 bits: all 0s (sync continuation)
    //   - 2 bits: frame identifier (11 for pkt1, 10 for pkt2)
    //   - 40 bits: packet data
    // Total: 20 + 2 + 40 = 62 bits (but 123 Manchester = 61.5, so 1 bit truncated)

    // Add 20 sync bits: first 16 are 0s, last 4 are 1s (from header 0x3C = 111100)
    for (int i = 0; i < 16; i++) {
        packet1_buffer[bit_idx / 8] |= (0 << (7 - (bit_idx % 8)));
        bit_idx++;
    }
    for (int i = 0; i < 4; i++) {
        packet1_buffer[bit_idx / 8] |= (1 << (7 - (bit_idx % 8)));
        bit_idx++;
    }

    // Packet type bits (00 for packet1): part of the header structure
    packet1_buffer[bit_idx / 8] |= (0 << (7 - (bit_idx % 8))); bit_idx++;
    packet1_buffer[bit_idx / 8] |= (0 << (7 - (bit_idx % 8))); bit_idx++;

    // Data field upper 2 bits (always 00 for 38-bit encode result)
    packet1_buffer[bit_idx / 8] |= (0 << (7 - (bit_idx % 8))); bit_idx++;
    packet1_buffer[bit_idx / 8] |= (0 << (7 - (bit_idx % 8))); bit_idx++;

    // Packet 1 data: 38 bits from encode_half()
    for (int i = 37; i >= 0; i--) {
        uint8_t bit = (encoded_packet1 >> i) & 1;
        packet1_buffer[bit_idx / 8] |= (bit << (7 - (bit_idx % 8)));
        bit_idx++;
    }

    // Manchester encode packet 1
    std::vector<rmt_item32_t> packet1_data;
    size_t manchester_items = manchester_encode(packet1_buffer, bit_idx, packet1_data);

    // Build packet 1 with preamble at the start
    rmt_items.clear();

    // Add preamble: 1600us HIGH (continuous) + 100us LOW
    // This matches Flipper's exact structure
    rmt_item32_t preamble;
    preamble.level0 = 1;
    preamble.duration0 = US_TO_RMT_TICKS(1600);
    preamble.level1 = 0;
    preamble.duration1 = US_TO_RMT_TICKS(100);
    rmt_items.push_back(preamble);

    // Append Manchester data (sync + frame + packet data)
    rmt_items.insert(rmt_items.end(), packet1_data.begin(), packet1_data.end());

    // Add end marker for packet 1
    rmt_item32_t end_marker;
    end_marker.level0 = 0;
    end_marker.duration0 = 0;
    end_marker.level1 = 0;
    end_marker.duration1 = 0;
    rmt_items.push_back(end_marker);

    // Store packet 1 data WITHOUT preamble for continuous transmission
    // Must copy the Manchester-encoded data before packet1_data goes out of scope
    rmt_packet1_no_preamble.assign(packet1_data.begin(), packet1_data.end());
    rmt_packet1_no_preamble.push_back(end_marker);

    // === BURST 2: Packet 2 ===
    uint8_t packet2_buffer[16] = {0};
    bit_idx = 0;

    // Same structure as packet 1: 16+4 sync bits + 2 packet type bits + 40 data bits
    // Add 20 sync bits: first 16 are 0s, last 4 are 1s (from header)
    for (int i = 0; i < 16; i++) {
        packet2_buffer[bit_idx / 8] |= (0 << (7 - (bit_idx % 8)));
        bit_idx++;
    }
    for (int i = 0; i < 4; i++) {
        packet2_buffer[bit_idx / 8] |= (1 << (7 - (bit_idx % 8)));
        bit_idx++;
    }

    // Packet type bits (01 for packet2): PACKET_2 sets bit 40
    packet2_buffer[bit_idx / 8] |= (0 << (7 - (bit_idx % 8))); bit_idx++;
    packet2_buffer[bit_idx / 8] |= (1 << (7 - (bit_idx % 8))); bit_idx++;

    // Data field upper 2 bits (always 00 for 38-bit encode result)
    packet2_buffer[bit_idx / 8] |= (0 << (7 - (bit_idx % 8))); bit_idx++;
    packet2_buffer[bit_idx / 8] |= (0 << (7 - (bit_idx % 8))); bit_idx++;

    // Packet 2 data: 38 bits from encode_half()
    for (int i = 37; i >= 0; i--) {
        uint8_t bit = (encoded_packet2 >> i) & 1;
        packet2_buffer[bit_idx / 8] |= (bit << (7 - (bit_idx % 8)));
        bit_idx++;
    }

    // Total: 20 sync + 2 packet type + 2 padding + 38 data = 62 bits -> 62 Manchester items

    // Manchester encode packet 2 (no preamble - only packet 1 has preamble)
    manchester_encode(packet2_buffer, bit_idx, rmt_packet2_items);

    // Add end marker for packet 2
    rmt_packet2_items.push_back(end_marker);

    encoded_fixed = fixed;
    encoded_rolling = rolling;
    encoded_repeats = SECPLUS_V2_REPEATS;

    return true;
}

/**
 * Get encoded RMT data
 */
bool SecPlusV2Protocol::getEncodedData(const rmt_item32_t** out_data, size_t* out_len) {
    if (rmt_items.empty()) {
        return false;
    }

    *out_data = rmt_items.data();
    *out_len = rmt_items.size();
    return true;
}

/**
 * Serialize to .sub file (Flipper Zero format)
 */
bool SecPlusV2Protocol::serializeToFile(File& file, const ProtocolDecodeResult& result) {
    if (!file) {
        return false;
    }

    file.printf("Protocol: %s\n", getName());
    file.printf("Bit: %d\n", SECPLUS_V2_BIT_COUNT);

    // Format packet 2 as "Key" (Flipper Zero format - Key field contains packet2!)
    file.printf("Key: ");
    for (int i = 7; i >= 0; i--) {
        uint8_t byte = (encoded_packet2 >> (i * 8)) & 0xFF;
        file.printf("%02X", byte);
        if (i > 0) file.printf(" ");
    }
    file.printf("\n");

    // Format packet 1 (Secplus_packet_1)
    file.printf("Secplus_packet_1: ");
    for (int i = 7; i >= 0; i--) {
        uint8_t byte = (encoded_packet1 >> (i * 8)) & 0xFF;
        file.printf("%02X", byte);
        if (i > 0) file.printf(" ");
    }
    file.printf("\n");

    return true;
}

/**
 * Deserialize from .sub file
 */
bool SecPlusV2Protocol::deserializeFromFile(File& file, ProtocolEncodeParams& params) {
    if (!file) {
        return false;
    }

    uint64_t fixed = 0;
    uint64_t packet1 = 0;
    uint64_t packet2 = 0;
    bool has_fixed = false;
    bool has_packet1 = false;
    bool has_packet2 = false;

    String line;
    while (file.available()) {
        line = file.readStringUntil('\n');
        line.trim();

        if (line.startsWith("Key:")) {
            // Parse fixed code
            String key_str = line.substring(4);
            key_str.trim();
            key_str.replace(" ", "");

            fixed = 0;
            for (size_t i = 0; i < key_str.length() && i < 16; i++) {
                char c = key_str.charAt(i);
                uint8_t nibble = 0;
                if (c >= '0' && c <= '9') nibble = c - '0';
                else if (c >= 'A' && c <= 'F') nibble = c - 'A' + 10;
                else if (c >= 'a' && c <= 'f') nibble = c - 'a' + 10;
                fixed = (fixed << 4) | nibble;
            }
            has_fixed = true;

        } else if (line.startsWith("Secplus_packet_1:")) {
            // Parse packet 1
            String pkt_str = line.substring(17);
            pkt_str.trim();
            pkt_str.replace(" ", "");

            packet1 = 0;
            for (size_t i = 0; i < pkt_str.length() && i < 16; i++) {
                char c = pkt_str.charAt(i);
                uint8_t nibble = 0;
                if (c >= '0' && c <= '9') nibble = c - '0';
                else if (c >= 'A' && c <= 'F') nibble = c - 'A' + 10;
                else if (c >= 'a' && c <= 'f') nibble = c - 'a' + 10;
                packet1 = (packet1 << 4) | nibble;
            }
            has_packet1 = true;

        } else if (line.startsWith("Secplus_packet_2:")) {
            // Parse packet 2
            String pkt_str = line.substring(17);
            pkt_str.trim();
            pkt_str.replace(" ", "");

            packet2 = 0;
            for (size_t i = 0; i < pkt_str.length() && i < 16; i++) {
                char c = pkt_str.charAt(i);
                uint8_t nibble = 0;
                if (c >= '0' && c <= '9') nibble = c - '0';
                else if (c >= 'A' && c <= 'F') nibble = c - 'A' + 10;
                else if (c >= 'a' && c <= 'f') nibble = c - 'a' + 10;
                packet2 = (packet2 << 4) | nibble;
            }
            has_packet2 = true;
        }

        if (line.length() == 0 || (!line.startsWith("Key:") && !line.startsWith("Secplus_packet_1:") &&
            !line.startsWith("Secplus_packet_2:") && !line.startsWith("Protocol:") &&
            !line.startsWith("Bit:") && !line.startsWith("Frequency:") && !line.startsWith("Preset:"))) {
            break;
        }
    }

    if (!has_fixed || !has_packet1) {
        return false;
    }

    // If we have both packets, decode them to extract rolling code
    uint32_t rolling = 0;
    if (has_packet2) {
        uint64_t serial;
        uint8_t button;
        if (!decodePackets(packet1, packet2, serial, button, rolling)) {
            rolling = 0;
        }
    } else {
        // Only have packet1, cannot fully decode rolling code
        rolling = 0;
    }

    // Encode for transmission with the decoded rolling code
    return encodeFromCodes(fixed, rolling);
}

/**
 * Get decoded codes
 */
bool SecPlusV2Protocol::getDecodedCodes(uint64_t& fixed, uint32_t& rolling) const {
    fixed = encoded_fixed;
    rolling = encoded_rolling;
    return (encoded_fixed != 0);
}

/**
 * Get all decoded Security+ 2.0 parameters
 */
bool SecPlusV2Protocol::getDecodedParams(uint64_t& serial, uint8_t& button, uint32_t& counter,
                                          uint64_t& packet1, uint64_t& packet2) const {
    if (encoded_fixed == 0) {
        return false;
    }

    serial = encoded_serial;
    button = encoded_button;
    counter = encoded_rolling;
    packet1 = encoded_packet1;
    packet2 = encoded_packet2;
    return true;
}

/**
 * Decode packets directly from hex values (for file loading)
 */
bool SecPlusV2Protocol::decodePackets(uint64_t packet1, uint64_t packet2,
                                       uint64_t& serial, uint8_t& button, uint32_t& counter) {
    // Unscramble both packets
    uint32_t fixed1, fixed2;
    uint8_t roll1[9], roll2[9];
    uint16_t data1, data2;

    bool valid1 = unscramble_half_packet(packet1, fixed1, roll1, data1);
    bool valid2 = unscramble_half_packet(packet2, fixed2, roll2, data2);

    if (!valid1 || !valid2) {
        return false;
    }

    // Reconstruct rolling code using Flipper's exact method
    // 1. Arrange 18 ternary digits in specific order
    uint8_t rolling_digits[18];
    rolling_digits[0] = roll2[8];
    rolling_digits[1] = roll1[8];
    rolling_digits[2] = roll2[4];
    rolling_digits[3] = roll2[5];
    rolling_digits[4] = roll2[6];
    rolling_digits[5] = roll2[7];
    rolling_digits[6] = roll1[4];
    rolling_digits[7] = roll1[5];
    rolling_digits[8] = roll1[6];
    rolling_digits[9] = roll1[7];
    rolling_digits[10] = roll2[0];
    rolling_digits[11] = roll2[1];
    rolling_digits[12] = roll2[2];
    rolling_digits[13] = roll2[3];
    rolling_digits[14] = roll1[0];
    rolling_digits[15] = roll1[1];
    rolling_digits[16] = roll1[2];
    rolling_digits[17] = roll1[3];

    // 2. Convert from base-3 to 28-bit integer
    uint32_t rolling = 0;
    for(int i = 0; i < 18; i++) {
        rolling = (rolling * 3) + rolling_digits[i];
    }

    // 3. Bit-reverse to get final counter
    counter = 0;
    for(int i = 0; i < 28; i++) {
        counter = (counter << 1) | (rolling & 1);
        rolling >>= 1;
    }

    // Extract button from upper 8 bits of fixed1 (Flipper: fixed_1[0] >> 12)
    button = fixed1 >> 12;

    // Reconstruct serial number: lower 12 bits of fixed1 + all 20 bits of fixed2
    // Flipper: fixed_1[0] << 20 | fixed_2[0]
    // Since fixed1 is 20 bits, the lower 12 bits are (fixed1 & 0xFFF)
    serial = ((uint64_t)(fixed1 & 0xFFF) << 20) | fixed2;  // 32-bit serial

    // Store decoded values for later use
    encoded_packet1 = packet1;
    encoded_packet2 = packet2;
    // Reconstruct full 40-bit fixed code for compatibility
    encoded_fixed = ((uint64_t)fixed1 << 20) | fixed2;
    encoded_rolling = counter;
    encoded_button = button;
    encoded_serial = serial;

    return true;
}

/**
 * Get packet 2 data for two-burst transmission
 */
bool SecPlusV2Protocol::getPacket2Data(const rmt_item32_t** out_data, size_t* out_len) {
    if (rmt_packet2_items.empty()) {
        return false;
    }

    *out_data = rmt_packet2_items.data();
    *out_len = rmt_packet2_items.size();
    return true;
}

/**
 * Get packet 1 data WITHOUT preamble (for continuous transmission)
 */
bool SecPlusV2Protocol::getPacket1NoPreamble(const rmt_item32_t** out_data, size_t* out_len) {
    if (rmt_packet1_no_preamble.empty()) {
        return false;
    }

    *out_data = rmt_packet1_no_preamble.data();
    *out_len = rmt_packet1_no_preamble.size();
    return true;
}

/**
 * Reset state machine
 */
void SecPlusV2Protocol::reset() {
    state = STATE_RESET;
    // Initialize to START0 to match Flipper's post-initialization state
    // (Flipper feeds LongHigh then ShortLow to MID1->MID0->START0)
    manchester_state = MANCHESTER_START0;
    decode_data = 0;
    decode_count_bit = 0;
    has_result = false;
    current_result.valid = false;
    secplus_packet_1 = 0;  // Reset packet storage on full reset
}

/**
 * Check if decode is complete and get result
 */
bool SecPlusV2Protocol::decode_check(ProtocolDecodeResult& result) {
    if (has_result) {
        result = current_result;
        has_result = false;  // Clear for next decode
        return true;
    }
    return false;
}

/**
 * Manchester state machine advance
 * Returns true if a bit was decoded
 */
bool SecPlusV2Protocol::manchester_advance(bool level, uint32_t duration, uint8_t& data) {
    // Flipper-compatible Manchester decoder using transition table
    // States: Start1=0, Mid1=1, Mid0=2, Start0=3
    // Events: ShortLow=0, ShortHigh=2, LongLow=4, LongHigh=6
    static const uint8_t transitions[] = {0b00000001, 0b10010001, 0b10011011, 0b11111011};

    bool is_short = (duration >= SECPLUS_V2_TE_SHORT - SECPLUS_V2_TOLERANCE &&
                     duration <= SECPLUS_V2_TE_SHORT + SECPLUS_V2_TOLERANCE);
    bool is_long = (duration >= SECPLUS_V2_TE_LONG - SECPLUS_V2_TOLERANCE &&
                    duration <= SECPLUS_V2_TE_LONG + SECPLUS_V2_TOLERANCE);

    if (!is_short && !is_long) {
        return false;
    }

    uint8_t event;
    if (is_short) {
        event = level ? 2 : 0;
    } else {
        event = level ? 6 : 4;
    }

    uint8_t current_state = (uint8_t)manchester_state;
    uint8_t next_state = (transitions[current_state] >> event) & 0x3;

    if (next_state == current_state) {
        manchester_state = MANCHESTER_START1;
        return false;
    }

    manchester_state = (ManchesterState)next_state;

    bool bit_decoded = false;
    if (next_state == 1) {
        data = 1;
        bit_decoded = true;
    } else if (next_state == 2) {
        data = 0;
        bit_decoded = true;
    } else {
	   // nothing
    }

    return bit_decoded;
}

/**
 * Feed edge event to state machine
 */
void SecPlusV2Protocol::feed(bool level, uint32_t duration) {
    #define DURATION_DIFF(x, y) ((x) > (y) ? ((x) - (y)) : ((y) - (x)))

    switch(state) {
    case STATE_RESET:
        if (!level && duration >= 1000) {
            const char* state_names[] = {"START1", "MID1", "MID0", "START0"};

            manchester_state = MANCHESTER_MID1;

            uint8_t dummy_bit;
            manchester_advance(true, SECPLUS_V2_TE_LONG, dummy_bit);
            manchester_advance(false, SECPLUS_V2_TE_SHORT, dummy_bit);

            state = STATE_RESET;
            decode_count_bit = 0xFF;
        } else if (decode_count_bit == 0xFF) {
            bool is_v2_timing = (duration >= SECPLUS_V2_TE_SHORT - SECPLUS_V2_TOLERANCE &&
                                 duration <= SECPLUS_V2_TE_SHORT + SECPLUS_V2_TOLERANCE) ||
                                (duration >= SECPLUS_V2_TE_LONG - SECPLUS_V2_TOLERANCE &&
                                 duration <= SECPLUS_V2_TE_LONG + SECPLUS_V2_TOLERANCE);

            if (!is_v2_timing) {
                decode_count_bit = 0;
                state = STATE_RESET;
                break;
            }

            state = STATE_DECODE_DATA;
            decode_data = 0;
            decode_count_bit = 0;

            const char* state_names[] = {"START1", "MID1", "MID0", "START0"};

            uint8_t bit_data = 0;
            if (manchester_advance(level, duration, bit_data)) {
                decode_data = (decode_data << 1) | bit_data;
                decode_count_bit++;
            }
            break;
        } else {
            break;
        }
        break;

    case STATE_DECODE_DATA: {
        uint8_t bit_data;
        bool bit_decoded = false;

        if (decode_count_bit < 3) {
            const char* state_names[] = {"START1", "MID1", "MID0", "START0"};
        }

        if (!level && duration >= 1000) {
            if (decode_count_bit == 62) {
                // Extract frame bits (bits 21-22 after 20-bit preamble)
                uint8_t frame0 = (decode_data >> 41) & 1;
                uint8_t frame1 = (decode_data >> 40) & 1;
                uint8_t frame = (frame0 << 1) | frame1;

                // Extract 40-bit payload (lower 40 bits)
                uint64_t packet_40bit = decode_data & 0xFFFFFFFFFFULL;

                // Validate packet by attempting unscramble
                uint32_t fixed_tmp;
                uint8_t roll_tmp[9];
                uint16_t data_tmp;
                if (!unscramble_half_packet(packet_40bit, fixed_tmp, roll_tmp, data_tmp)) {
                    state = STATE_RESET;
                    break;
                }

                // Store or pair based on frame
                if (frame == 0x00) {
                    secplus_packet_1 = packet_40bit;  // Store 40-bit payload
                    state = STATE_RESET;  // Wait for packet 2
                } else if (frame == 0x01) {
                    if (secplus_packet_1 != 0) {
                        // Try to decode the pair
                        uint64_t serial;
                        uint8_t button;
                        uint32_t counter;

                        if (decodePackets(secplus_packet_1, packet_40bit, serial, button, counter)) {
                            current_result.valid = true;
                            current_result.key = packet_40bit;  // Store packet 2 as key
                            current_result.bit_count = 42;
                            current_result.te = SECPLUS_V2_TE_SHORT;
                            has_result = true;

                            Serial.printf("[SecPlus2] Decoded: Serial=0x%09llX Button=0x%X Counter=0x%07X\n",
                                         serial, button, counter);

                            secplus_packet_1 = 0;  // Clear for next pair
                            state = STATE_RESET;
                            break;
                        } else {
                            Serial.println("[SecPlus2] Packet pair decode failed");
                        }
                    } else {
                    }
                    state = STATE_RESET;
                } else {
                    state = STATE_RESET;
                }
            } else {
                state = STATE_RESET;
            }
        }
        // Feed edge to Manchester decoder
        else if (manchester_advance(level, duration, bit_data)) {
            decode_data = (decode_data << 1) | bit_data;
            decode_count_bit++;

            // Check if we've reached exactly 62 bits (full packet per Flipper)
            // 62 bits = 20-bit preamble + 2-bit frame + 40-bit payload
            // Process immediately without waiting for gap
            if (decode_count_bit == 62) {
                // Extract frame bits (bits 21-22, after 20-bit preamble)
                uint8_t frame0 = (decode_data >> 41) & 1;  // bit 41 (bit 21 of 62)
                uint8_t frame1 = (decode_data >> 40) & 1;  // bit 40 (bit 22 of 62)
                uint8_t frame = (frame0 << 1) | frame1;

                // Extract 40-bit payload (bits 23-62, i.e., lower 40 bits)
                uint64_t packet_40bit = decode_data & 0xFFFFFFFFFFULL;

                // Validate packet
                uint32_t fixed_tmp;
                uint8_t roll_tmp[9];
                uint16_t data_tmp;
                if (!unscramble_half_packet(packet_40bit, fixed_tmp, roll_tmp, data_tmp)) {
                    state = STATE_RESET;
                    break;
                }

                // Store or pair based on frame
                if (frame == 0x00) {
                    secplus_packet_1 = packet_40bit;
                    state = STATE_RESET;  // Wait for packet 2
                } else if (frame == 0x01) {
                    if (secplus_packet_1 != 0) {
                        uint64_t serial;
                        uint8_t button;
                        uint32_t counter;

                        if (decodePackets(secplus_packet_1, packet_40bit, serial, button, counter)) {
                            current_result.valid = true;
                            current_result.key = packet_40bit;
                            current_result.bit_count = 42;
                            current_result.te = SECPLUS_V2_TE_SHORT;
                            has_result = true;

                            Serial.printf("[SecPlus2] Decoded: Serial=0x%09llX Button=0x%X Counter=0x%07X\n",
                                         serial, button, counter);

                            secplus_packet_1 = 0;
                            state = STATE_RESET;
                            break;
                        } else {
                            Serial.println("[SecPlus2] Packet pair decode failed");
                        }
                    } 
                    state = STATE_RESET;
                } else {
                    state = STATE_RESET;
                }
            }
        }
        break;
    }
    }

    #undef DURATION_DIFF
}
