/**
 * Security+ 1.0 Protocol Implementation
 *
 * Rolling code protocol used in pre-2011 garage door openers from
 * LiftMaster, Chamberlain, and Craftsman (before Security+ 2.0).
 *
 * Timing Specifications:
 * - Frequency: 315 MHz / 390 MHz
 * - Short pulse (TE): 500 us
 * - Tolerance: +/-100 us
 * - Encoding: Tri-bit (3 symbols: 0, 1, 2) using OOK (On-Off Keying)
 * - Packet format: Two packets x 21 symbols each (1 header + 20 data)
 * - Total symbols: 40 data symbols (encodes 64 bits via base-3)
 *
 * Symbol Encoding (duration pairs LOW + HIGH):
 * - Symbol 0: 1500us LOW + 500us HIGH  (3L + 1H)
 * - Symbol 1: 1000us LOW + 1000us HIGH (2L + 2H)
 * - Symbol 2: 500us LOW + 1500us HIGH  (1L + 3H)
 *
 * Packet Headers:
 * - Packet 1: ~59.5ms LOW (119 TE units) + 500us HIGH -> carries symbols 1,3,5,...,39
 * - Packet 2: ~58ms LOW (116 TE units) + 1500us HIGH -> carries symbols 2,4,6,...,40
 *
 * Protocol Structure:
 * - Fixed code: 32 bits (device ID)
 * - Rolling code: 32 bits (increments by ~3 per button press)
 * - Total: 64 bits encoded as 40 base-3 symbols
 *
 * Reference:
 * - https://github.com/argilo/secplus
 * - Flipper Zero: lib/subghz/protocols/secplus_v1.c
 */

#ifndef __PROTOCOL_SECPLUS_V1_H__
#define __PROTOCOL_SECPLUS_V1_H__

#include "../protocol_base.h"
#include <vector>

class SecPlusV1Protocol : public SubGhzProtocol {
private:
    // Timing constants (from Flipper Zero implementation)
    static const uint16_t SECPLUS_V1_TE_SHORT = 500;       // 500 us base time unit
    static const uint16_t SECPLUS_V1_TE_LONG = 1500;       // 1500 us (3x short)
    static const uint16_t SECPLUS_V1_TOLERANCE = 100;      // +/-100 us
    static const uint8_t SECPLUS_V1_SYMBOL_COUNT = 40;     // 40 data symbols total
    static const uint8_t SECPLUS_V1_SYMBOLS_PER_PACKET = 20; // 20 symbols per packet

    // Preamble timing (wake-up pulse, sent once before first packet)
    static const uint16_t SECPLUS_V1_PREAMBLE_HIGH = 890;  // 890 us high
    static const uint16_t SECPLUS_V1_PREAMBLE_LOW = 135;   // 135 us low

    // Packet headers (from Flipper Zero secplus_v1.c)
    static const uint8_t SECPLUS_V1_HEADER_TE_PKT1 = 119;  // Packet 1: 119x500us low
    static const uint8_t SECPLUS_V1_HEADER_TE_PKT2 = 116;  // Packet 2: 116x500us low

    // Symbol values (tri-bit encoding)
    static const uint8_t SYMBOL_0 = 0;  // 3L + 1H pattern
    static const uint8_t SYMBOL_1 = 1;  // 2L + 2H pattern
    static const uint8_t SYMBOL_2 = 2;  // 1L + 3H pattern
    static const uint8_t SYMBOL_INVALID = 0xFF;

    // Packet indices (from Flipper)
    static const uint8_t PACKET_1_HEADER = 0x00;
    static const uint8_t PACKET_2_HEADER = 0x02;
    static const uint8_t PACKET_1_INDEX_BASE = 0;
    static const uint8_t PACKET_2_INDEX_BASE = 21;

    // Encoded data storage (for TX)
    uint64_t encoded_fixed = 0;       // Fixed code (32 bits)
    uint32_t encoded_rolling = 0;     // Rolling code (32 bits)
    uint8_t data_array[44];           // Symbol storage (2 packets x 21 symbols + headers)

    // Decoder state machine
    enum DecoderStep {
        STEP_RESET = 0,
        STEP_SEARCH_START_BIT,
        STEP_SAVE_DURATION,
        STEP_DECODE_DATA
    };

    DecoderStep decoder_step;
    uint32_t te_last;                 // Last pulse duration
    uint8_t decode_count_bit;         // Bit counter for current packet
    uint8_t base_packet_index;        // Current packet being decoded (0 or 21)
    uint8_t packet_accepted;          // Bitmap: bit 0 = pkt1, bit 1 = pkt2

    // Packet acceptance flags
    static const uint8_t PACKET_1_ACCEPTED = 0x01;
    static const uint8_t PACKET_2_ACCEPTED = 0x02;

    // RMT item storage for transmission
    std::vector<rmt_item32_t> rmt_packet1_with_preamble;  // Packet 1 with preamble + header
    std::vector<rmt_item32_t> rmt_packet1_no_preamble;     // Packet 1 with header only
    std::vector<rmt_item32_t> rmt_packet2_items;           // Packet 2 with header

    /**
     * Encode 64-bit key into 40 ternary symbols (TX)
     * Converts fixed and rolling codes to base-3 representation
     * and stores in data_array with proper interleaving
     *
     * @param fixed 32-bit fixed code (device ID)
     * @param rolling 32-bit rolling code (counter)
     * @return true if encoding successful
     */
    bool encode_to_symbols(uint32_t fixed, uint32_t rolling);

    /**
     * Decode 40 ternary symbols into fixed and rolling codes (RX)
     * Reverses the base-3 encoding and checksum algorithm
     *
     * @param fixed Output: 32-bit fixed code
     * @param rolling Output: 32-bit rolling code
     * @return true if decoding successful
     */
    bool decode_from_symbols(uint32_t& fixed, uint32_t& rolling);

    /**
     * Generate RMT items from symbol array
     * Creates complete transmission: Packet 1 + Packet 2
     *
     * @return true if RMT generation successful
     */
    bool generate_rmt_items();

    /**
     * Parse .sub file fields specific to Security+ 1.0
     * Extracts Fix and Cnt fields
     *
     * @param file File handle positioned after Protocol line
     * @param fixed Output: fixed code
     * @param rolling Output: rolling code
     * @return true if parsing successful
     */
    bool parse_sub_file(File& file, uint32_t& fixed, uint32_t& rolling);

public:
    SecPlusV1Protocol() :
        encoded_fixed(0),
        encoded_rolling(0),
        decoder_step(STEP_RESET),
        te_last(0),
        decode_count_bit(0),
        base_packet_index(0),
        packet_accepted(0) {
        memset(data_array, 0, sizeof(data_array));
    }

    ~SecPlusV1Protocol() override = default;

    const char* getName() const override {
        return "Security+ 1.0";
    }

    /**
     * Decode captured signal from RMT items
     * NOTE: Decode support not implemented yet (TX only)
     *
     * @param data RMT items array
     * @param len Number of RMT items
     * @param result Output: decode result
     * @return false (not implemented)
     */
    bool decode(const rmt_item32_t* data, size_t len, ProtocolDecodeResult& result) override;

    /**
     * Encode key/data into RMT items for transmission
     * Expects key to contain fixed (upper 32 bits) + rolling (lower 32 bits)
     *
     * @param params Encode parameters
     * @return true if encoding successful
     */
    bool encode(const ProtocolEncodeParams& params) override;

    /**
     * Get encoded RMT items for Packet 1 transmission WITH preamble
     * Must be called after successful encode()
     * Use this for the FIRST transmission only
     *
     * @param out_data Output: pointer to RMT items (preamble + header + packet 1 data)
     * @param out_len Output: number of RMT items
     * @return true if data available
     */
    bool getEncodedData(const rmt_item32_t** out_data, size_t* out_len) override;

    /**
     * Get encoded RMT items for Packet 1 WITHOUT preamble
     * Use this for repeated transmissions (continuous mode)
     *
     * @param out_data Output: pointer to RMT items (header + packet 1 data)
     * @param out_len Output: number of RMT items
     * @return true if data available
     */
    bool getPacket1NoPreamble(const rmt_item32_t** out_data, size_t* out_len);

    /**
     * Get Packet 2 data for transmission
     * Security+ 1.0 requires alternating packet 1 and packet 2
     *
     * @param out_data Output: pointer to RMT items (Packet 2 data only)
     * @param out_len Output: number of RMT items
     * @return true if data available
     */
    bool getPacket2Data(const rmt_item32_t** out_data, size_t* out_len);

    /**
     * Serialize to .sub file (not needed for TX-only implementation)
     *
     * @param file File handle
     * @param result Decode result
     * @return false (not implemented)
     */
    bool serializeToFile(File& file, const ProtocolDecodeResult& result) override;

    /**
     * Deserialize from .sub file for transmission
     * Reads Flipper Zero format and prepares for TX
     *
     * @param file File handle
     * @param params Output: encode parameters
     * @return true if deserialization successful
     */
    bool deserializeFromFile(File& file, ProtocolEncodeParams& params) override;

    // State machine interface
    void feed(bool level, uint32_t duration) override;

    void reset() override {
        decoder_step = STEP_RESET;
        te_last = 0;
        decode_count_bit = 0;
        base_packet_index = 0;
        packet_accepted = 0;
        memset(data_array, 0, sizeof(data_array));
    }

    bool decode_check(ProtocolDecodeResult& result) override {
        // Check if we have a complete decode ready
        if (packet_accepted == (PACKET_1_ACCEPTED | PACKET_2_ACCEPTED)) {
            return decode(nullptr, 0, result);
        }
        return false;
    }

    size_t getMinimumLength() const override {
        // Approximate: 40 symbols x 2 items per symbol = 80 items minimum
        return 80;
    }

    const uint32_t* getCommonFrequencies() const override {
        static const uint32_t freqs[] = {
            315000000,   // 315 MHz (most common)
            390000000,   // 390 MHz (some models)
            0
        };
        return freqs;
    }

    /**
     * Encode from fixed + rolling codes directly
     * Convenience method for creating transmissions
     *
     * @param fixed 32-bit fixed code
     * @param rolling 32-bit rolling code
     * @return true if encoding successful
     */
    bool encodeFromCodes(uint32_t fixed, uint32_t rolling);

    /**
     * Get encoded codes (for debugging)
     *
     * @param fixed Output: fixed code
     * @param rolling Output: rolling code
     * @return true if data available
     */
    bool getEncodedCodes(uint32_t& fixed, uint32_t& rolling) const {
        if (encoded_fixed == 0 && encoded_rolling == 0) {
            return false;
        }
        fixed = encoded_fixed;
        rolling = encoded_rolling;
        return true;
    }
};

#endif // __PROTOCOL_SECPLUS_V1_H__
