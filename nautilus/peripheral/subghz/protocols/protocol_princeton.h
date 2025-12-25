/**
 * Princeton Protocol Implementation
 *
 * Fixed-code protocol used in garage door openers, remote controls, and doorbells.
 * 24-bit data format with PWM encoding.
 *
 * Timing Specifications:
 * - Base TE: 400 us (typical, range 300-500 us)
 * - Bit 0: SHORT pulse + LONG gap (1xTE + 3xTE = 400us + 1200us)
 * - Bit 1: LONG pulse + SHORT gap (3xTE + 1xTE = 1200us + 400us)
 * - Sync/Preamble: LOW for 36xTE (~14,400us)
 * - Stop bit: HIGH for 1xTE (400us)
 * - Guard time: LOW for 30xTE (~12,000us) between transmissions
 * - Encoding: MSB first (most significant bit transmitted first)
 *
 * Data Format (24 bits):
 * - Bits [23:4]: Serial number (20 bits) OR Bits [23:8]: Serial number (16 bits)
 * - Bits [3:0]: Button code (4 bits) OR Bits [7:0]: Button code (8 bits)
 *
 * Reference: Flipper Zero/Momentum Firmware implementation
 */

#ifndef __PROTOCOL_PRINCETON_H__
#define __PROTOCOL_PRINCETON_H__

#include "../protocol_base.h"
#include <vector>

class PrincetonProtocol : public SubGhzProtocol {
private:
    // Timing constants (based on Flipper Zero/Momentum implementation)
    static const uint16_t PRINCETON_TE_DEFAULT = 400;      // Default time element (us)
    static const uint16_t PRINCETON_TE_SHORT = 400;        // Short pulse/gap (1xTE)
    static const uint16_t PRINCETON_TE_LONG = 1200;        // Long pulse/gap (3xTE)
    static const uint16_t PRINCETON_GUARD_TIME = 30;       // Guard time multiplier (30xTE)
    static const uint16_t PRINCETON_REPEAT_GAP = 12000;    // Gap between repeats (us) - 12ms
    static const uint8_t PRINCETON_BIT_COUNT = 24;         // Fixed 24-bit data
    static const uint8_t PRINCETON_DEFAULT_REPEATS = 10;   // Default transmission repeats

    // Timing tolerance for decoder (+/-20% to avoid false positives on CAME)
    // CAME has 17.9ms header which would match Princeton's 14.4ms +/-30%
    static const uint8_t PRINCETON_TOLERANCE_PERCENT = 20;

    // RMT encoded data for transmission
    std::vector<rmt_item32_t> rmt_items;

    // Encoded signal parameters
    uint64_t encoded_key = 0;
    uint32_t encoded_te = PRINCETON_TE_DEFAULT;
    uint32_t encoded_guard_time = PRINCETON_GUARD_TIME;
    int encoded_repeats = 0;

    // State machine state (for edge-based decoding)
    enum DecoderState {
        STATE_RESET = 0,           // Idle, waiting for sync/preamble
        STATE_SAVE_DURATION,       // Save first half of bit (HIGH pulse)
        STATE_CHECK_DURATION       // Check second half (LOW gap) and decode bit
    };

    DecoderState state;
    uint64_t decode_data;          // Accumulated decoded data
    uint32_t decode_count_bit;     // Number of bits decoded
    uint32_t te_last;              // Last HIGH pulse duration (for bit decoding)
    uint32_t detected_te;          // Detected TE from signal
    bool has_result;               // True when decode is complete
    ProtocolDecodeResult current_result;

    /**
     * Helper to create RMT item
     */
    inline rmt_item32_t make_item(uint32_t duration0_us, uint32_t level0,
                                   uint32_t duration1_us, uint32_t level1) {
        rmt_item32_t item;
        item.duration0 = US_TO_RMT_TICKS(duration0_us);
        item.level0 = level0;
        item.duration1 = US_TO_RMT_TICKS(duration1_us);
        item.level1 = level1;
        return item;
    }

public:
    PrincetonProtocol() : state(STATE_RESET), decode_data(0), decode_count_bit(0),
                          te_last(0), detected_te(PRINCETON_TE_DEFAULT),
                          has_result(false) {
        current_result.valid = false;
    }

    /**
     * Transmit Princeton signal directly via GPIO
     * This bypasses RMT items and uses direct GPIO for timing accuracy
     *
     * @param tx_pin GPIO pin for transmission
     * @param continuous If true, transmit continuously until button released
     */
    bool transmitDirect(int tx_pin, bool continuous = false);

    const char* getName() const override {
        return "Princeton";
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
        return 50;  // Sync (1) + 24 data bits (48) + stop bit (1)
    }

    const uint32_t* getCommonFrequencies() const override {
        static const uint32_t freqs[] = {
            315000000,   // 315 MHz (North America)
            433920000,   // 433.92 MHz (Europe/Asia - most common)
            868000000,   // 868 MHz (Europe)
            0
        };
        return freqs;
    }
};

#endif // __PROTOCOL_PRINCETON_H__
