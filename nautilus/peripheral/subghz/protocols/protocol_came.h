/**
 * CAME Protocol Implementation
 *
 * Fixed-code protocol used in gate openers, garage doors, and doorbells.
 * Supports 12-bit and 24-bit variants.
 *
 * Timing Specifications:
 * - Base TE: 320-350 us (typical: 350us based on real captures)
 * - Bit 0: SHORT pulse + LONG gap (350us + 700us)
 * - Bit 1: LONG pulse + SHORT gap (700us + 350us)
 * - Preamble: SHORT pulse (350us)
 * - Inter-frame gap: ~12.2ms
 * - Encoding: MSB first (most significant bit transmitted first)
 *
 * Reference: Real doorbell capture analysis
 */

#ifndef __PROTOCOL_CAME_H__
#define __PROTOCOL_CAME_H__

#include "../protocol_base.h"
#include <vector>

class CAMEProtocol : public SubGhzProtocol {
private:
    // Timing constants (adjusted for actual captured signals)
    // Real signals show TE ~= 320-420us (not the theoretical 350us)
    static const uint16_t CAME_TE_SHORT = 350;        // Short pulse/gap (us) - target value
    static const uint16_t CAME_TE_LONG = 700;         // Long pulse/gap (2 x SHORT)
    static const uint16_t CAME_PREAMBLE = 350;        // Preamble pulse (us)
    static const uint16_t CAME_REPEAT_GAP = 15000;    // Gap between repeats (us) - Flipper timing
    static const uint8_t CAME_DEFAULT_REPEATS = 5;    // Default repeats for short press

    // Timing tolerance for decoder (+/-50% to handle variation)
    static const uint8_t CAME_TOLERANCE_PERCENT = 50;

    // Encoded signal parameters (for direct GPIO transmission)
    uint64_t encoded_key = 0;
    uint8_t encoded_bits = 0;
    int encoded_repeats = 0;

    // State machine state (for edge-based decoding)
    enum DecoderState {
        STATE_RESET = 0,           // Idle, waiting for header or data
        STATE_FOUND_START_BIT,     // Found header, waiting for start bit
        STATE_SAVE_DURATION,       // Save first half of bit (gap)
        STATE_CHECK_DURATION,      // Check second half (pulse) and decode bit
        STATE_FOUND_FIRST_BIT      // Found valid bit pattern without header (opportunistic)
    };

    DecoderState state;
    uint64_t decode_data;          // Accumulated decoded data
    uint32_t decode_count_bit;     // Number of bits decoded
    uint32_t te_last;              // Last edge duration (for multi-edge bit decoding)
    bool has_result;               // True when decode is complete
    ProtocolDecodeResult current_result;

public:
    CAMEProtocol() : state(STATE_RESET), decode_data(0), decode_count_bit(0),
                     te_last(0), has_result(false) {
        current_result.valid = false;
    }

    /**
     * Transmit CAME signal directly via GPIO
     * This bypasses RMT items and uses proven working code
     *
     * @param tx_pin GPIO pin for transmission
     * @param continuous If true, transmit continuously until button released
     */
    bool transmitDirect(int tx_pin, bool continuous = false);

    const char* getName() const override {
        return "CAME";
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
        return 13;  // Preamble + 12 bits minimum
    }

    const uint32_t* getCommonFrequencies() const override {
        static const uint32_t freqs[] = {
            315000000,   // 315 MHz (North America)
            433920000,   // 433.92 MHz (Europe/Asia)
            868000000,   // 868 MHz (Europe)
            0
        };
        return freqs;
    }
};

#endif // __PROTOCOL_CAME_H__
