/**
 * SubGHz Protocol Base Class
 *
 * Abstract interface for all SubGHz protocol implementations.
 * Each protocol (CAME, Princeton, Security+ 2.0, etc.) implements this interface.
 *
 * Architecture follows Flipper Zero's protocol system design.
 */

#ifndef __PROTOCOL_BASE_H__
#define __PROTOCOL_BASE_H__

#include <Arduino.h>
#include <driver/rmt.h>
#include <FS.h>

/**
 * Protocol decode result
 */
struct ProtocolDecodeResult {
    bool valid;              // True if signal was successfully decoded
    uint64_t key;            // Decoded key/data (up to 64 bits)
    uint8_t bit_count;       // Number of bits in the key
    uint32_t te;             // Time element / pulse length (microseconds)
    String extra_data;       // Protocol-specific extra data (for complex protocols)
};

/**
 * Initialize decode result with default values
 * @param result Result structure to initialize
 * @param te_value Protocol-specific time element value
 */
inline void initDecodeResult(ProtocolDecodeResult& result, uint32_t te_value) {
    result.valid = false;
    result.key = 0;
    result.bit_count = 0;
    result.te = te_value;
}

/**
 * Protocol encode parameters
 */
struct ProtocolEncodeParams {
    uint64_t key;            // Key/data to encode
    uint8_t bit_count;       // Number of bits
    uint32_t te;             // Time element (0 = use protocol default)
    int repeat_count;        // Number of transmissions (0 = use protocol default)
};

/**
 * Abstract base class for SubGHz protocols
 */
class SubGhzProtocol {
public:
    virtual ~SubGhzProtocol() {}

    /**
     * Get protocol name (e.g., "CAME", "Princeton", "RAW")
     */
    virtual const char* getName() const = 0;

    /**
     * Decode captured signal from RMT items
     *
     * @param data Pointer to RMT items array
     * @param len Number of RMT items
     * @param result Output: decode result
     * @return true if signal matches this protocol and was decoded successfully
     */
    virtual bool decode(const rmt_item32_t* data, size_t len, ProtocolDecodeResult& result) = 0;

    /**
     * Encode key/data into RMT items for transmission
     *
     * @param params Encode parameters (key, bit count, etc.)
     * @return true if encoding was successful
     */
    virtual bool encode(const ProtocolEncodeParams& params) = 0;

    /**
     * Get encoded RMT items for transmission
     * Must be called after successful encode()
     *
     * @param out_data Output: pointer to RMT items array (owned by protocol instance)
     * @param out_len Output: number of RMT items
     * @return true if RMT data is available
     */
    virtual bool getEncodedData(const rmt_item32_t** out_data, size_t* out_len) = 0;

    /**
     * Serialize protocol-specific data to .sub file
     * Writes lines like "Bit: 12", "Key: 00 00 00 00 00 00 0E 84", etc.
     *
     * @param file File handle to write to
     * @param result Decode result to serialize
     * @return true if serialization was successful
     */
    virtual bool serializeToFile(File& file, const ProtocolDecodeResult& result) = 0;

    /**
     * Deserialize protocol-specific data from .sub file
     * Reads protocol-specific fields and populates encode parameters
     *
     * @param file File handle to read from
     * @param params Output: encode parameters for transmission
     * @return true if deserialization was successful
     */
    virtual bool deserializeFromFile(File& file, ProtocolEncodeParams& params) = 0;

    /**
     * Feed edge event to decoder state machine (Flipper Zero style)
     * @param level Signal level (true=HIGH, false=LOW)
     * @param duration Duration of this level in microseconds
     */
    virtual void feed(bool level, uint32_t duration) {
        // Default: no-op (protocols must override to support edge-based decoding)
        (void)level;
        (void)duration;
    }

    /**
     * Reset decoder state machine
     */
    virtual void reset() {
        // Default: no-op (protocols should override if they have state)
    }

    /**
     * Check if decoder has successfully decoded a signal
     * @param result Output: decode result (only valid if returns true)
     * @return true if a valid signal was decoded
     */
    virtual bool decode_check(ProtocolDecodeResult& result) {
        // Default: not supported
        (void)result;
        return false;
    }

    /**
     * Get minimum number of RMT items this protocol needs for detection
     * Used to optimize decoder attempts
     */
    virtual size_t getMinimumLength() const {
        return 10;  // Default: at least 10 pulses
    }

    /**
     * Get typical frequency range for this protocol (in Hz)
     * Returns array of common frequencies, terminated by 0
     * Used for frequency selection hints
     */
    virtual const uint32_t* getCommonFrequencies() const {
        // Default: common ISM bands
        static const uint32_t freqs[] = { 315000000, 433920000, 868000000, 0 };
        return freqs;
    }
};

/**
 * Helper macros for protocol timing
 */
#define US_TO_RMT_TICKS(us) ((us) * SUBGHZ_RMT_1US_TICKS)
#define RMT_TICKS_TO_US(ticks) ((ticks) / SUBGHZ_RMT_1US_TICKS)

// RMT tick definitions (from rf_utils.h)
#ifndef SUBGHZ_RMT_1US_TICKS
#define SUBGHZ_RMT_CLK_DIV 80
#define SUBGHZ_RMT_1US_TICKS (80000000 / SUBGHZ_RMT_CLK_DIV / 1000000)
#endif

/**
 * Utility functions for protocol implementations
 */

/**
 * Check if pulse duration matches expected value within tolerance
 *
 * @param duration_us Measured pulse duration in microseconds
 * @param expected_us Expected duration
 * @param tolerance_percent Tolerance percentage (e.g., 20 for +/-20%)
 * @return true if duration is within tolerance
 */
inline bool pulse_matches(uint32_t duration_us, uint32_t expected_us, uint8_t tolerance_percent = 30) {
    uint32_t tolerance = (expected_us * tolerance_percent) / 100;
    return (duration_us >= expected_us - tolerance) && (duration_us <= expected_us + tolerance);
}

/**
 * Convert RMT item to duration in microseconds
 */
inline uint32_t rmt_item_to_us(const rmt_item32_t& item, bool get_level0) {
    uint32_t ticks = get_level0 ? item.duration0 : item.duration1;
    return RMT_TICKS_TO_US(ticks);
}

#endif // __PROTOCOL_BASE_H__
