/**
 * Security+ 2.0 Protocol Implementation
 *
 * Rolling code protocol used in LiftMaster, Chamberlain, and Craftsman garage door openers.
 * Introduced in 2011, uses sophisticated encoding with base-3 conversion and scrambling.
 *
 * Timing Specifications:
 * - Frequency: 310-315 MHz (typically 315 MHz)
 * - Short pulse (TE): 250 us
 * - Long pulse (2xTE): 500 us
 * - Tolerance: +/-110 us
 * - Encoding: Manchester (differential)
 * - Packet format: Two 40-bit packets (v2.0) or two 64-bit packets (v2.5 with PIN)
 * - Total bits: 62 (v2.0: 2 frame bits + 2x40 data bits) or 82 (v2.5)
 *
 * Protocol Structure:
 * - Rolling code: 28 bits (increments with each button press)
 * - Fixed code: 40 bits (unique remote ID)
 * - Data: 32 bits optional (v2.5 only, for PIN codes)
 *
 * Encoding Process:
 * 1. Rolling code is bit-reversed and converted to base-3 (18 ternary digits)
 * 2. Fixed code and rolling code are interleaved
 * 3. Bits are permuted and inverted based on rolling code digits
 * 4. Manchester encoding applied for transmission
 * 5. Transmitted as two packets (frame 0 and frame 1)
 *
 * Reference: https://github.com/argilo/secplus
 */

#ifndef __PROTOCOL_SECPLUS_V2_H__
#define __PROTOCOL_SECPLUS_V2_H__

#include "../protocol_base.h"
#include <vector>

class SecPlusV2Protocol : public SubGhzProtocol {
private:
    // Timing constants (based on Flipper Zero implementation)
    static const uint16_t SECPLUS_V2_TE_SHORT = 250;      // Short pulse (us)
    static const uint16_t SECPLUS_V2_TE_LONG = 500;       // Long pulse (2 x SHORT)
    static const uint16_t SECPLUS_V2_TOLERANCE = 110;     // Timing tolerance (us)
    static const uint8_t SECPLUS_V2_BIT_COUNT = 62;       // Total bits (v2.0)
    static const uint8_t SECPLUS_V2_REPEATS = 2;          // Default repeat count

    // Packet structure constants
    static const uint8_t SECPLUS_V2_HALF_BITS = 40;       // Bits per packet (v2.0)
    static const uint8_t SECPLUS_V2_ROLLING_BITS = 28;    // Rolling code bits
    static const uint8_t SECPLUS_V2_FIXED_BITS = 40;      // Fixed code bits

    // Encoded signal storage
    uint64_t encoded_packet1 = 0;   // First 40-bit packet
    uint64_t encoded_packet2 = 0;   // Second 40-bit packet
    uint64_t encoded_fixed = 0;     // Fixed code (40 bits: button + serial)
    uint32_t encoded_rolling = 0;   // Rolling code (28 bits)
    uint8_t encoded_button = 0;     // Button code (4 bits, extracted from fixed)
    uint64_t encoded_serial = 0;    // Serial number (36 bits, extracted from fixed)
    int encoded_repeats = 0;

    // Packet assembly state (for handling 68ms gap between packets)
    struct PacketState {
        uint64_t packet = 0;        // Stored packet (40 bits)
        uint8_t frame = 0xFF;       // Frame bits (00 or 01), 0xFF = empty
        unsigned long timestamp = 0; // millis() when captured
        bool valid = false;
    } pending_packet;

    static const unsigned long PACKET_PAIR_TIMEOUT_MS = 200;  // Max time between packet1 and packet2

    // State machine state
    enum DecoderState {
        STATE_RESET = 0,
        STATE_DECODE_DATA
    };

    // Manchester decoder state
    enum ManchesterState {
        MANCHESTER_START1 = 0,     // Starting, expecting high
        MANCHESTER_MID1 = 1,       // Mid-bit, high level (decoded bit = 1)
        MANCHESTER_MID0 = 2,       // Mid-bit, low level (decoded bit = 0)
        MANCHESTER_START0 = 3      // Starting, expecting low
    };

    DecoderState state;
    ManchesterState manchester_state;
    uint64_t decode_data;          // Accumulated decoded data (62 bits)
    uint32_t decode_count_bit;     // Number of bits decoded
    bool has_result;               // True when decode is complete
    ProtocolDecodeResult current_result;
    uint64_t secplus_packet_1;     // Stored packet 1 (waiting for packet 2)
    uint64_t secplus_packet_2;     // Stored packet 2

    /**
     * Scramble half-packet (wrapper for Flipper encode_half)
     * @param fixed 20-bit fixed code half
     * @param roll 9 ternary digits
     * @param data Unused (kept for compatibility)
     * @return 40-bit scrambled packet
     */
    uint64_t scramble_half_packet(uint32_t fixed, const uint8_t roll[9], uint16_t data);

    /**
     * Unscramble half-packet (wrapper for Flipper decode_half)
     * @param packet 40-bit scrambled packet
     * @param fixed Output: 20-bit fixed code half
     * @param roll Output: 9 ternary digits
     * @param data Output: Unused (kept for compatibility)
     * @return true if unscrambling succeeded
     */
    bool unscramble_half_packet(uint64_t packet, uint32_t& fixed, uint8_t roll[9], uint16_t& data);

    /**
     * Decode a complete Security+ 2.0 packet pair
     * Helper for both single-packet assembly and dual-packet decode
     * @param packet1 First packet (frame 00)
     * @param packet2 Second packet (frame 01)
     * @param result Output: decode result
     * @return true if successful
     */
    bool decode_packet_pair(uint64_t packet1, uint64_t packet2, ProtocolDecodeResult& result);

    /**
     * Manchester decode from RMT items
     *
     * @param data RMT items
     * @param len Number of items
     * @param bits Output: decoded bits
     * @param max_bits Maximum bits to decode
     * @return Number of bits decoded
     */
    size_t manchester_decode(const rmt_item32_t* data, size_t len, uint8_t* bits, size_t max_bits);

    /**
     * Manchester encode to RMT items
     *
     * @param bits Input bits
     * @param bit_count Number of bits
     * @param items Output RMT items
     * @return Number of RMT items generated
     */
    size_t manchester_encode(const uint8_t* bits, size_t bit_count, std::vector<rmt_item32_t>& items);

    // RMT item storage for transmission
    std::vector<rmt_item32_t> rmt_items;

    /**
     * Manchester state transition helper
     * @param level Signal level (true=HIGH, false=LOW)
     * @param duration Duration in microseconds
     * @param data Output: decoded bit (0 or 1) if return is true
     * @return true if a bit was decoded
     */
    bool manchester_advance(bool level, uint32_t duration, uint8_t& data);

public:
    SecPlusV2Protocol() : state(STATE_RESET), manchester_state(MANCHESTER_START0),
                         decode_data(0), decode_count_bit(0), has_result(false),
                         secplus_packet_1(0), secplus_packet_2(0) {
        current_result.valid = false;
    }

    const char* getName() const override {
        return "Security+ 2.0";
    }

    bool decode(const rmt_item32_t* data, size_t len, ProtocolDecodeResult& result) override;
    bool encode(const ProtocolEncodeParams& params) override;
    bool getEncodedData(const rmt_item32_t** out_data, size_t* out_len) override;
    bool serializeToFile(File& file, const ProtocolDecodeResult& result) override;
    bool deserializeFromFile(File& file, ProtocolEncodeParams& params) override;

    // State machine interface (Flipper Zero style)
    void feed(bool level, uint32_t duration) override;
    void reset() override;
    bool decode_check(ProtocolDecodeResult& result) override;

    size_t getMinimumLength() const override {
        // Manchester encoding: each bit requires ~2 RMT items (one for each half-bit)
        // Single packet: 42 bits x ~1.2 (Manchester overhead) ~= 50 RMT items
        // Accept single packets due to 68ms gap splitting transmission
        return 40;  // Minimum RMT items for single packet (conservative)
    }

    const uint32_t* getCommonFrequencies() const override {
        static const uint32_t freqs[] = {
            310000000,   // 310 MHz
            315000000,   // 315 MHz (most common)
            390000000,   // 390 MHz (some models)
            0
        };
        return freqs;
    }

    /**
     * Encode from fixed + rolling codes
     * Convenience method for creating transmissions
     */
    bool encodeFromCodes(uint64_t fixed, uint32_t rolling);

    /**
     * Get decoded fixed and rolling codes
     * Call after successful decode()
     */
    bool getDecodedCodes(uint64_t& fixed, uint32_t& rolling) const;

    /**
     * Get all decoded Security+ 2.0 parameters
     * Call after successful decode()
     *
     * @param serial Output: 36-bit serial number
     * @param button Output: 4-bit button code
     * @param counter Output: 28-bit rolling counter
     * @param packet1 Output: First scrambled packet
     * @param packet2 Output: Second scrambled packet
     * @return true if data is available
     */
    bool getDecodedParams(uint64_t& serial, uint8_t& button, uint32_t& counter,
                          uint64_t& packet1, uint64_t& packet2) const;

    /**
     * Decode packets directly from hex values (for file loading)
     * This extracts serial, button, and counter from the scrambled packets
     *
     * @param packet1 First 40-bit scrambled packet
     * @param packet2 Second 40-bit scrambled packet
     * @param serial Output: 36-bit serial number
     * @param button Output: 4-bit button code
     * @param counter Output: 28-bit rolling counter
     * @return true if decode successful
     */
    bool decodePackets(uint64_t packet1, uint64_t packet2,
                       uint64_t& serial, uint8_t& button, uint32_t& counter);

    /**
     * Get encoded packet 2 data (for two-burst transmission)
     * Security+ 2.0 requires packet 1, then 68ms delay, then packet 2
     *
     * @param out_data Output: pointer to RMT items for packet 2
     * @param out_len Output: number of RMT items
     * @return true if packet 2 data is available
     */
    bool getPacket2Data(const rmt_item32_t** out_data, size_t* out_len);

    /**
     * Get packet 1 data WITHOUT preamble (for continuous transmission loops)
     * Used when repeating packets - only the first transmission needs preamble
     *
     * @param out_data Output: pointer to RMT items for packet 1 (no preamble)
     * @param out_len Output: number of RMT items
     * @return true if packet 1 data is available
     */
    bool getPacket1NoPreamble(const rmt_item32_t** out_data, size_t* out_len);

private:
    // Storage for packet 2 RMT items
    std::vector<rmt_item32_t> rmt_packet2_items;
    // Storage for packet 1 without preamble (for continuous transmission)
    std::vector<rmt_item32_t> rmt_packet1_no_preamble;
};

#endif // __PROTOCOL_SECPLUS_V2_H__
