#ifndef IR_FLIPPER_H
#define IR_FLIPPER_H

#include <Arduino.h>
#include <vector>
#include <SD.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <IRrecv.h>
#include <IRutils.h>

// Maximum raw data size for IR signals (Flipper Zero uses ~512)
#define MAX_IR_RAW_DATA 512

/**
 * IR Signal Types
 */
enum IRSignalType {
    IR_SIGNAL_PARSED,  // Known protocol with address/command
    IR_SIGNAL_RAW      // Raw timing data
};

/**
 * Single IR signal/button
 */
struct IRSignal {
    String name;
    IRSignalType type;

    // For parsed signals (NEC, NECext, Samsung32, etc.)
    String protocol;
    uint32_t address;   // 4-byte address (little-endian stored as 4 hex bytes in file)
    uint32_t command;   // 4-byte command (little-endian stored as 4 hex bytes in file)

    // For raw signals
    uint32_t frequency;      // Carrier frequency in Hz (typically 38000)
    float duty_cycle;        // Duty cycle 0.0-1.0 (typically 0.33)
    std::vector<uint16_t> raw_data;  // Pulse timings in microseconds

    // Constructor
    IRSignal() : name(""), type(IR_SIGNAL_PARSED), protocol(""),
                 address(0), command(0), frequency(38000), duty_cycle(0.33f) {}
};

/**
 * IR Remote - collection of signals in a single .ir file
 */
struct IRRemote {
    String filepath;
    std::vector<IRSignal> signals;

    // Constructor
    IRRemote() : filepath("") {}

    // Helper methods
    void clear() {
        filepath = "";
        signals.clear();
    }

    int size() const {
        return signals.size();
    }

    void addSignal(const IRSignal& signal) {
        signals.push_back(signal);
    }
};

/**
 * Protocol information
 */
struct IRProtocolInfo {
    const char* name;
    uint32_t address_mask;
    uint32_t command_mask;
};

// Supported protocols for Phase 1
static const IRProtocolInfo IR_PROTOCOLS[] = {
    {"NEC",       0x000000FF, 0x000000FF},  // 8-bit address, 8-bit command
    {"NECext",    0x0000FFFF, 0x0000FFFF},  // 16-bit address, 16-bit command
    {"Samsung32", 0x000000FF, 0x000000FF},  // 8-bit address, 8-bit command
};
static const int IR_PROTOCOL_COUNT = sizeof(IR_PROTOCOLS) / sizeof(IR_PROTOCOLS[0]);

// Function declarations

/**
 * Parse a Flipper Zero .ir file
 * @param filepath Path to .ir file
 * @param remote IRRemote structure to populate
 * @return true if successful, false on error
 */
bool parseFlipperIRFile(const char* filepath, IRRemote& remote);

/**
 * Write a Flipper Zero .ir file
 * @param filepath Path to .ir file
 * @param remote IRRemote structure to write
 * @return true if successful, false on error
 */
bool writeFlipperIRFile(const char* filepath, const IRRemote& remote);

/**
 * Get protocol info by name
 * @param protocol_name Name of the protocol
 * @return Pointer to protocol info, or nullptr if not found
 */
const IRProtocolInfo* getProtocolInfo(const char* protocol_name);

/**
 * Validate address/command against protocol mask
 * @param value Value to validate
 * @param mask Protocol mask
 * @return true if valid, false if out of range
 */
bool validateProtocolValue(uint32_t value, uint32_t mask);

/**
 * Convert 4-byte little-endian hex string to uint32_t
 * Format: "XX XX XX XX" (e.g., "07 00 00 00" = 0x07)
 * @param hex_str Hex string
 * @return uint32_t value
 */
uint32_t hexStringToUint32(const String& hex_str);

/**
 * Convert uint32_t to 4-byte little-endian hex string
 * Format: "XX XX XX XX" (e.g., 0x07 = "07 00 00 00")
 * @param value Value to convert
 * @return Hex string
 */
String uint32ToHexString(uint32_t value);

/**
 * Create a default IR signal (for capture workflow)
 */
IRSignal createDefaultIRSignal();

/**
 * Transmit an IR signal using IRremoteESP8266
 * @param signal IRSignal to transmit
 * @param irsend IRsend object (from IRremoteESP8266)
 * @return true if transmitted successfully
 */
bool transmitIRSignal(const IRSignal& signal, IRsend& irsend);

/**
 * Convert IRremoteESP8266 decode_results to IRSignal
 * @param results decode_results from IRrecv
 * @param signal IRSignal to populate
 * @return true if conversion successful
 */
bool decodeResultsToIRSignal(const decode_results& results, IRSignal& signal);

/**
 * Generate sequential remote filename (Remote_0.ir, Remote_1.ir, etc.)
 * @return String with available filename (not full path, just filename)
 */
String generateRemoteName();

/**
 * Generate sequential button name (Btn_0, Btn_1, etc.) for a given remote
 * @param remote IRRemote to check existing button names
 * @return String with available button name
 */
String generateButtonName(const IRRemote& remote);

/**
 * Create an empty Flipper Zero .ir file
 * @param filepath Path to create file at
 * @return true if created successfully
 */
bool createEmptyRemote(const char* filepath);

/**
 * Append a signal to an existing .ir file
 * @param filepath Path to .ir file
 * @param signal IRSignal to append
 * @return true if successful
 */
bool appendSignalToRemote(const char* filepath, const IRSignal& signal);

/**
 * Replace a signal at a specific index in a .ir file
 * @param filepath Path to .ir file
 * @param index Index of signal to replace (0-based)
 * @param signal New IRSignal
 * @return true if successful
 */
bool replaceSignalInRemote(const char* filepath, int index, const IRSignal& signal);

/**
 * Delete a signal at a specific index from a .ir file
 * @param filepath Path to .ir file
 * @param index Index of signal to delete (0-based)
 * @return true if successful
 */
bool deleteSignalFromRemote(const char* filepath, int index);

#endif // IR_FLIPPER_H
