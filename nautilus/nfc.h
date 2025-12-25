#ifndef NFC_H
#define NFC_H

#include <Arduino.h>
#include <vector>
#include <SD.h>

// Maximum page data for NFC tags
#define MAX_NFC_PAGES 256
#define NFC_PAGE_SIZE 4

/**
 * NFC Tag Types (subset of Flipper Zero supported types)
 */
enum NFCTagType {
    NFC_TAG_ISO14443_3A,
    NFC_TAG_ISO14443_3B,
    NFC_TAG_ISO14443_4A,
    NFC_TAG_ISO14443_4B,
    NFC_TAG_NTAG_ULTRALIGHT,
    NFC_TAG_MIFARE_CLASSIC,
    NFC_TAG_UNKNOWN
};

/**
 * Single NFC tag data (from .nfc file)
 */
struct NFCTag {
    String filepath;
    String device_type;      // e.g., "ISO14443-4A", "NTAG216", "Mifare Classic"
    uint8_t uid[10];         // UID bytes (up to 10 bytes for some tags)
    uint8_t uid_len;         // Actual UID length (4, 7, or 10 bytes)

    // ISO14443-3A/4A specific
    uint16_t atqa;           // ATQA (Answer To reQuest A)
    uint8_t sak;             // SAK (Select AcKnowledge)

    // ISO14443-4A specific
    uint8_t t0;
    uint8_t ta1;
    uint8_t tb1;
    uint8_t tc1;

    // NTAG/Ultralight specific
    uint8_t signature[32];   // ECC signature
    uint8_t mifare_version[8];
    uint16_t pages_total;
    uint16_t pages_read;
    uint8_t page_data[MAX_NFC_PAGES][NFC_PAGE_SIZE];  // Page memory
    uint8_t counters[3];     // Counter values
    uint8_t tearing[3];      // Tearing flags

    // Constructor
    NFCTag() : filepath(""), device_type(""), uid_len(0), atqa(0), sak(0),
               t0(0), ta1(0), tb1(0), tc1(0), pages_total(0), pages_read(0) {
        memset(uid, 0, sizeof(uid));
        memset(signature, 0, sizeof(signature));
        memset(mifare_version, 0, sizeof(mifare_version));
        memset(page_data, 0, sizeof(page_data));
        memset(counters, 0, sizeof(counters));
        memset(tearing, 0, sizeof(tearing));
    }

    // Helper: Get tag type enum from device_type string
    NFCTagType getTagType() const {
        if (device_type.indexOf("ISO14443-3A") >= 0) return NFC_TAG_ISO14443_3A;
        if (device_type.indexOf("ISO14443-4A") >= 0) return NFC_TAG_ISO14443_4A;
        if (device_type.indexOf("NTAG") >= 0 || device_type.indexOf("Ultralight") >= 0) return NFC_TAG_NTAG_ULTRALIGHT;
        if (device_type.indexOf("Mifare Classic") >= 0) return NFC_TAG_MIFARE_CLASSIC;
        return NFC_TAG_UNKNOWN;
    }

    // Helper: Get UID as hex string
    String getUIDString() const {
        char buf[32];
        char* ptr = buf;
        for (int i = 0; i < uid_len; i++) {
            if (i > 0) *ptr++ = ':';
            sprintf(ptr, "%02X", uid[i]);
            ptr += 2;
        }
        *ptr = '\0';
        return String(buf);
    }
};

// Function declarations

/**
 * Parse a Flipper Zero .nfc file
 * @param filepath Path to .nfc file
 * @param tag NFCTag structure to populate
 * @return true if successful, false on error
 */
bool parseFlipperNFCFile(const char* filepath, NFCTag& tag);

/**
 * Write a Flipper Zero .nfc file
 * @param filepath Path to .nfc file
 * @param tag NFCTag structure to write
 * @return true if successful, false on error
 */
bool writeFlipperNFCFile(const char* filepath, const NFCTag& tag);

/**
 * Parse hex byte string to byte array
 * Format: "XX XX XX XX" or "XX:XX:XX:XX"
 * @param hex_str Hex string
 * @param output Output byte array
 * @param max_len Maximum bytes to parse
 * @return Number of bytes parsed
 */
int parseHexBytes(const String& hex_str, uint8_t* output, int max_len);

/**
 * Convert byte array to hex string
 * Format: "XX XX XX XX"
 * @param bytes Byte array
 * @param len Number of bytes
 * @return Hex string
 */
String bytesToHexString(const uint8_t* bytes, int len);

/**
 * Read NFC tag from PN532 hardware
 * @param tag NFCTag structure to populate
 * @param timeout_ms Timeout in milliseconds (0 = no timeout, blocks forever)
 * @return true if tag read successfully
 */
bool readNFCTag(NFCTag& tag, uint16_t timeout_ms = 0);

/**
 * Emulate NFC tag UID only using PN532 hardware
 * Only responds to anti-collision protocol with UID/ATQA/SAK
 * Reader can detect the card but cannot read data
 * @param tag NFCTag structure to emulate (uses UID, ATQA, SAK)
 * @param timeout_ms Timeout in milliseconds (0 = no timeout)
 * @return true if a reader detected the tag, false on timeout
 */
bool emulateNFCTagUID(const NFCTag& tag, uint16_t timeout_ms = 0);

/**
 * Emulate full NFC tag using PN532 hardware
 * Responds to all commands including page reads (NTAG/Ultralight only)
 * Reader can fully interact with the emulated tag data
 * @param tag NFCTag structure to emulate (full page data)
 * @param timeout_ms Timeout in milliseconds (0 = no timeout)
 * @return true if emulation session completed, false on timeout/error
 */
bool emulateNFCTagFull(const NFCTag& tag, uint16_t timeout_ms = 0);

/**
 * Generate sequential NFC filename (NFC_0.nfc, NFC_1.nfc, etc.)
 * @return String with available filename (not full path, just filename)
 */
String generateNFCName();

/**
 * NDEF Record Types
 */
enum NDEFRecordType {
    NDEF_TYPE_UNKNOWN,
    NDEF_TYPE_URL,
    NDEF_TYPE_TEXT,
    NDEF_TYPE_WIFI,
    NDEF_TYPE_VCARD,
    NDEF_TYPE_GEO,
    NDEF_TYPE_EMAIL,
    NDEF_TYPE_PHONE
};

/**
 * NDEF Record structure
 */
struct NDEFRecord {
    NDEFRecordType type;
    String label;       // Human-readable label (e.g., "URL", "Phone")
    String value;       // The actual data (URL, phone number, etc.)
    String details;     // Additional details if needed

    NDEFRecord() : type(NDEF_TYPE_UNKNOWN), label(""), value(""), details("") {}
    NDEFRecord(NDEFRecordType t, const String& l, const String& v, const String& d = "")
        : type(t), label(l), value(v), details(d) {}
};

/**
 * Parse NDEF records from NFC tag page data
 * @param tag NFCTag containing page data
 * @param records Vector to populate with parsed records
 * @return Number of records parsed
 */
int parseNDEFRecords(const NFCTag& tag, std::vector<NDEFRecord>& records);

#endif // NFC_H
