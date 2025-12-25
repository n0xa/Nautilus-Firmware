#include "nfc.h"
#include "peripheral/peripheral.h"
#include <lvgl.h>

extern Adafruit_PN532 nfc;

// Parse hex byte string to byte array
int parseHexBytes(const String& hex_str, uint8_t* output, int max_len) {
    String trimmed = hex_str;
    trimmed.trim();

    int byte_count = 0;
    int start = 0;

    for (int i = 0; i <= trimmed.length() && byte_count < max_len; i++) {
        if (i == trimmed.length() || trimmed[i] == ' ' || trimmed[i] == ':') {
            if (i > start) {
                String byte_str = trimmed.substring(start, i);
                output[byte_count] = strtoul(byte_str.c_str(), NULL, 16);
                byte_count++;
            }
            start = i + 1;
        }
    }

    return byte_count;
}

// Convert byte array to hex string
String bytesToHexString(const uint8_t* bytes, int len) {
    String result;
    for (int i = 0; i < len; i++) {
        char buf[4];
        snprintf(buf, sizeof(buf), "%02X", bytes[i]);
        if (i > 0) result += " ";
        result += buf;
    }
    return result;
}

// Helper functions to reduce parsing duplication
inline bool parseLineString(const String& line, const char* prefix, String& output) {
    if (line.startsWith(prefix)) {
        output = line.substring(strlen(prefix));
        output.trim();
        return true;
    }
    return false;
}

inline bool parseLineHexByte(const String& line, const char* prefix, uint8_t& output) {
    if (line.startsWith(prefix)) {
        String str = line.substring(strlen(prefix));
        str.trim();
        output = strtoul(str.c_str(), NULL, 16);
        return true;
    }
    return false;
}

inline bool parseLineInt(const String& line, const char* prefix, int& output) {
    if (line.startsWith(prefix)) {
        output = atoi(line.substring(strlen(prefix)).c_str());
        return true;
    }
    return false;
}

inline bool parseLineInt(const String& line, const char* prefix, uint16_t& output) {
    if (line.startsWith(prefix)) {
        output = atoi(line.substring(strlen(prefix)).c_str());
        return true;
    }
    return false;
}

inline bool parseLineHexBytes(const String& line, const char* prefix, uint8_t* output, int max_len) {
    if (line.startsWith(prefix)) {
        String str = line.substring(strlen(prefix));
        parseHexBytes(str, output, max_len);
        return true;
    }
    return false;
}

inline bool parseLineHexBytesWithLen(const String& line, const char* prefix, uint8_t* output, int max_len, int& len) {
    if (line.startsWith(prefix)) {
        String str = line.substring(strlen(prefix));
        len = parseHexBytes(str, output, max_len);
        return true;
    }
    return false;
}

inline bool parseLineHexBytesWithLen(const String& line, const char* prefix, uint8_t* output, int max_len, uint8_t& len) {
    if (line.startsWith(prefix)) {
        String str = line.substring(strlen(prefix));
        len = parseHexBytes(str, output, max_len);
        return true;
    }
    return false;
}

// Parse Flipper Zero .nfc file
bool parseFlipperNFCFile(const char* filepath, NFCTag& tag) {
    File file = SD.open(filepath, FILE_READ);
    if (!file) {
        return false;
    }

    // Reset tag
    tag = NFCTag();
    tag.filepath = filepath;

    String line;
    bool valid_header = false;


    while (file.available()) {
        line = file.readStringUntil('\n');
        line.trim();

        // Skip empty lines and comments
        if (line.length() == 0 || line.startsWith("#")) {
            continue;
        }

        // Parse header
        if (line.startsWith("Filetype:")) {
            if (line.indexOf("Flipper NFC device") != -1) {
                valid_header = true;
            }
            continue;
        }

        if (line.startsWith("Version:")) {
            continue;
        }

        // Parse device type
        if (parseLineString(line, "Device type:", tag.device_type)) {
            // Parsed successfully
        }
        // Parse UID
        else if (parseLineHexBytesWithLen(line, "UID:", tag.uid, sizeof(tag.uid), tag.uid_len)) {
            // Parsed successfully
        }
        // Parse ATQA
        else if (line.startsWith("ATQA:")) {
            String atqa_str = line.substring(5);
            uint8_t atqa_bytes[2];
            if (parseHexBytes(atqa_str, atqa_bytes, 2) == 2) {
                tag.atqa = (atqa_bytes[1] << 8) | atqa_bytes[0];
            }
        }
        // Parse SAK
        else if (parseLineHexByte(line, "SAK:", tag.sak)) {
            // Parsed successfully
        }
        // Parse T0
        else if (parseLineHexByte(line, "T0:", tag.t0)) {
            // Parsed successfully
        }
        // Parse TA(1)
        else if (parseLineHexByte(line, "TA(1):", tag.ta1)) {
            // Parsed successfully
        }
        // Parse TB(1)
        else if (parseLineHexByte(line, "TB(1):", tag.tb1)) {
            // Parsed successfully
        }
        // Parse TC(1)
        else if (parseLineHexByte(line, "TC(1):", tag.tc1)) {
            // Parsed successfully
        }
        // Parse Signature
        else if (parseLineHexBytes(line, "Signature:", tag.signature, sizeof(tag.signature))) {
            // Parsed successfully
        }
        // Parse Mifare version
        else if (parseLineHexBytes(line, "Mifare version:", tag.mifare_version, sizeof(tag.mifare_version))) {
            // Parsed successfully
        }
        // Parse Pages total
        else if (parseLineInt(line, "Pages total:", tag.pages_total)) {
            // Parsed successfully
        }
        // Parse Pages read
        else if (parseLineInt(line, "Pages read:", tag.pages_read)) {
            // Parsed successfully
        }
        // Parse Counter
        else if (line.startsWith("Counter ")) {
            int idx = atoi(line.substring(8).c_str());
            if (idx >= 0 && idx < 3) {
                int colon_pos = line.indexOf(':');
                if (colon_pos > 0) {
                    tag.counters[idx] = atoi(line.substring(colon_pos + 1).c_str());
                }
            }
        }
        // Parse Tearing
        else if (line.startsWith("Tearing ")) {
            int idx = atoi(line.substring(8).c_str());
            if (idx >= 0 && idx < 3) {
                int colon_pos = line.indexOf(':');
                if (colon_pos > 0) {
                    String tear_str = line.substring(colon_pos + 1);
                    tear_str.trim();
                    tag.tearing[idx] = strtoul(tear_str.c_str(), NULL, 16);
                }
            }
        }
        // Parse Page data
        else if (line.startsWith("Page ")) {
            int page_num = atoi(line.substring(5).c_str());
            int colon_pos = line.indexOf(':');
            if (colon_pos > 0 && page_num < MAX_NFC_PAGES) {
                String page_str = line.substring(colon_pos + 1);
                parseHexBytes(page_str, tag.page_data[page_num], NFC_PAGE_SIZE);
            }
        }
    }

    file.close();

    if (!valid_header) {
        return false;
    }

    return true;
}

// Write Flipper Zero .nfc file
bool writeFlipperNFCFile(const char* filepath, const NFCTag& tag) {
    File file = SD.open(filepath, FILE_WRITE);
    if (!file) {
        return false;
    }


    // Write header
    file.println("Filetype: Flipper NFC device");
    file.println("Version: 4");
    file.println("# Device type can be ISO14443-3A, ISO14443-3B, ISO14443-4A, ISO14443-4B, ISO15693-3, FeliCa, NTAG/Ultralight, Mifare Classic, Mifare Plus, Mifare DESFire, SLIX, ST25TB, NTAG4xx, Type 4 Tag, EMV");

    // Write device type
    file.print("Device type: ");
    file.println(tag.device_type);

    // Write UID
    file.println("# UID is common for all formats");
    file.print("UID: ");
    file.println(bytesToHexString(tag.uid, tag.uid_len));

    // Write ISO14443-3A data if applicable
    if (tag.getTagType() == NFC_TAG_ISO14443_3A ||
        tag.getTagType() == NFC_TAG_ISO14443_4A ||
        tag.getTagType() == NFC_TAG_NTAG_ULTRALIGHT) {
        file.println("# ISO14443-3A specific data");
        file.print("ATQA: ");
        file.printf("%02X %02X\n", tag.atqa & 0xFF, (tag.atqa >> 8) & 0xFF);
        file.print("SAK: ");
        file.printf("%02X\n", tag.sak);
    }

    // Write ISO14443-4A data if applicable
    if (tag.getTagType() == NFC_TAG_ISO14443_4A) {
        file.println("# ISO14443-4A specific data");
        file.printf("T0: %02X\n", tag.t0);
        file.printf("TA(1): %02X\n", tag.ta1);
        file.printf("TB(1): %02X\n", tag.tb1);
        file.printf("TC(1): %02X\n", tag.tc1);
    }

    // Write NTAG/Ultralight specific data
    if (tag.getTagType() == NFC_TAG_NTAG_ULTRALIGHT) {
        file.println("# NTAG/Ultralight specific data");
        file.println("Data format version: 2");

        // Write specific NTAG type
        if (tag.device_type.indexOf("NTAG216") >= 0) {
            file.println("NTAG/Ultralight type: NTAG216");
        } else if (tag.device_type.indexOf("NTAG215") >= 0) {
            file.println("NTAG/Ultralight type: NTAG215");
        } else if (tag.device_type.indexOf("NTAG213") >= 0) {
            file.println("NTAG/Ultralight type: NTAG213");
        }

        // Write signature if present
        bool has_signature = false;
        for (int i = 0; i < 32; i++) {
            if (tag.signature[i] != 0) {
                has_signature = true;
                break;
            }
        }
        if (has_signature) {
            file.print("Signature: ");
            file.println(bytesToHexString(tag.signature, 32));
        }

        // Write Mifare version if present
        bool has_version = false;
        for (int i = 0; i < 8; i++) {
            if (tag.mifare_version[i] != 0) {
                has_version = true;
                break;
            }
        }
        if (has_version) {
            file.print("Mifare version: ");
            file.println(bytesToHexString(tag.mifare_version, 8));
        }

        // Write counters and tearing flags
        for (int i = 0; i < 3; i++) {
            file.printf("Counter %d: %d\n", i, tag.counters[i]);
            file.printf("Tearing %d: %02X\n", i, tag.tearing[i]);
        }

        // Write page data
        if (tag.pages_total > 0) {
            file.printf("Pages total: %d\n", tag.pages_total);
            if (tag.pages_read > 0) {
                file.printf("Pages read: %d\n", tag.pages_read);
            }

            int pages_to_write = (tag.pages_read > 0) ? tag.pages_read : tag.pages_total;
            for (int i = 0; i < pages_to_write && i < MAX_NFC_PAGES; i++) {
                file.printf("Page %d: ", i);
                file.println(bytesToHexString(tag.page_data[i], NFC_PAGE_SIZE));
            }
        }
    }

    file.close();
    return true;
}

// Read NFC tag from PN532 hardware
bool readNFCTag(NFCTag& tag, uint16_t timeout_ms) {
    uint8_t uid_buffer[7];
    uint8_t uid_length;

    // Read passive target (ISO14443A card)
    bool success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid_buffer, &uid_length, timeout_ms);

    if (!success || uid_length == 0) {
        return false;
    }

    // Reset tag structure
    tag = NFCTag();

    // Copy UID
    tag.uid_len = uid_length;
    memcpy(tag.uid, uid_buffer, uid_length);


    uint8_t page_buffer[4];

    if (nfc.mifareultralight_ReadPage(0, page_buffer)) {

        uint8_t version_data[8];
        tag.device_type = "NTAG216";
        tag.sak = 0x00;
        tag.atqa = 0x0044;

        tag.pages_total = 231;
        tag.pages_read = 0;

        for (int page = 0; page < 231 && page < MAX_NFC_PAGES; page++) {
            if (nfc.mifareultralight_ReadPage(page, page_buffer)) {
                memcpy(tag.page_data[page], page_buffer, 4);
                tag.pages_read++;
            } else {
                break;
            }
        }

    } else {
        tag.device_type = "ISO14443-4A";
        tag.sak = 0x20;
        tag.atqa = 0x0001;
    }

    return true;
}

bool emulateNFCTagUID(const NFCTag& tag, uint16_t timeout_ms) {

    bool reader_detected = false;
    unsigned long start_time = millis();

    while (timeout_ms == 0 || (millis() - start_time) < timeout_ms) {
        uint8_t result = nfc.AsTarget();

        if (result) {
            reader_detected = true;

            unsigned long session_start = millis();
            while (millis() - session_start < 3000) {
                lv_task_handler();
                delay(10);
            }

            nfc.begin();
            delay(100);
            nfc.SAMConfig();
        } else {
            lv_task_handler();
            delay(10);
        }
    }

    if (!reader_detected && timeout_ms > 0) {
    }


    // Final reset to clean state
    nfc.begin();
    delay(100);
    nfc.SAMConfig();

    return reader_detected;
}

// Emulate full NFC tag using PN532 hardware (NTAG/Ultralight)
bool emulateNFCTagFull(const NFCTag& tag, uint16_t timeout_ms) {

    // Only NTAG/Ultralight tags are supported for full emulation
    if (tag.getTagType() != NFC_TAG_NTAG_ULTRALIGHT) {
        return false;
    }


    bool reader_detected = false;
    unsigned long start_time = millis();

    while (timeout_ms == 0 || (millis() - start_time) < timeout_ms) {
        uint8_t result = nfc.AsTarget();

        if (result) {
            reader_detected = true;

            bool session_active = true;
            uint8_t rx_buffer[64];
            uint8_t tx_buffer[64];
            uint8_t rx_len;
            uint8_t status;

            while (session_active) {
                status = nfc.getDataTarget(rx_buffer, &rx_len);

                if (status == 0) {
                    break;
                }

                if (rx_len == 0) {
                    lv_task_handler();
                    delay(5);
                    continue;
                }

                uint8_t cmd = rx_buffer[0];
                uint8_t response_len = 0;

                switch (cmd) {
                    case 0x30:
                        {
                            if (rx_len < 2) {
                                session_active = false;
                                break;
                            }

                            uint8_t start_page = rx_buffer[1];

                            for (int i = 0; i < 4; i++) {
                                uint8_t page = start_page + i;
                                if (page < MAX_NFC_PAGES && page < tag.pages_read) {
                                    memcpy(tx_buffer + i * 4, tag.page_data[page], 4);
                                } else {
                                    memset(tx_buffer + i * 4, 0, 4);
                                }
                            }
                            response_len = 16;
                        }
                        break;

                    case 0xA2:
                        {
                            if (rx_len < 6) {
                                session_active = false;
                                break;
                            }

                            uint8_t page = rx_buffer[1];

                            tx_buffer[0] = 0x0A;
                            response_len = 1;
                        }
                        break;

                    case 0x60:
                        {
                            if (tag.mifare_version[0] != 0) {
                                memcpy(tx_buffer, tag.mifare_version, 8);
                                response_len = 8;
                            } else {
                                uint8_t default_version[] = {0x00, 0x04, 0x04, 0x02, 0x01, 0x00, 0x13, 0x03};
                                memcpy(tx_buffer, default_version, 8);
                                response_len = 8;
                            }
                        }
                        break;

                    case 0x3C:
                        {
                            if (tag.signature[0] != 0) {
                                memcpy(tx_buffer, tag.signature, 32);
                                response_len = 32;
                            } else {
                                // Return zeros
                                memset(tx_buffer, 0, 32);
                                response_len = 32;
                            }
                        }
                        break;

                    case 0x39:  // READ_CNT (Read Counter)
                        {

                            // Return counter 0 value (3 bytes)
                            tx_buffer[0] = tag.counters[0] & 0xFF;
                            tx_buffer[1] = (tag.counters[0] >> 8) & 0xFF;
                            tx_buffer[2] = (tag.counters[0] >> 16) & 0xFF;
                            response_len = 3;
                        }
                        break;

                    default:
                        // Send NACK
                        tx_buffer[0] = 0x00;
                        response_len = 1;
                        break;
                }

                // Send response
                if (response_len > 0) {
                    status = nfc.setDataTarget(tx_buffer, response_len);
                    if (status == 0) {
                        session_active = false;
                    }
                }
            }


            // Reset for next reader - full hardware reset
            nfc.begin();
            delay(100);  // Give PN532 time to reset
            nfc.SAMConfig();
        } else {
            // No reader yet, keep UI responsive while waiting
            lv_task_handler();
            delay(10);
        }
    }

    if (!reader_detected && timeout_ms > 0) {
    }


    // Final reset to clean state
    nfc.begin();
    delay(100);
    nfc.SAMConfig();

    return reader_detected;
}

// Generate sequential NFC filename
String generateNFCName() {
    int index = 0;
    String filename;
    String fullpath;

    do {
        filename = "NFC_" + String(index);
        fullpath = "/nfc/" + filename + ".nfc";
        index++;
    } while (SD.exists(fullpath));

    return filename;
}

// Parse NDEF records from NFC tag page data
int parseNDEFRecords(const NFCTag& tag, std::vector<NDEFRecord>& records) {
    records.clear();

    // Only parse NTAG/Ultralight tags
    if (tag.getTagType() != NFC_TAG_NTAG_ULTRALIGHT) {
        return 0;
    }

    if (tag.pages_read < 4) {
        return 0;
    }

    // NDEF message typically starts at page 4 for NTAG213/215/216
    // Page 0-2: UID, Page 3: Capability Container
    int ndef_start_page = 4;

    // Check if there's NDEF data (TLV format)
    // Look for NDEF TLV block (0x03)
    int page = ndef_start_page;
    bool found_ndef = false;
    int ndef_length = 0;
    int ndef_data_start = 0;

    for (; page < tag.pages_read; page++) {
        if (tag.page_data[page][0] == 0x03) {  // NDEF Message TLV
            found_ndef = true;
            ndef_length = tag.page_data[page][1];

            // If length is 0xFF, it's a 3-byte length format
            if (ndef_length == 0xFF) {
                ndef_length = (tag.page_data[page][2] << 8) | tag.page_data[page][3];
                ndef_data_start = page * 4 + 4;
            } else {
                ndef_data_start = page * 4 + 2;
            }
            break;
        }
    }

    if (!found_ndef || ndef_length == 0) {
        return 0;
    }


    int offset = ndef_data_start;
    int max_offset = ndef_data_start + ndef_length;

    uint8_t *data = new uint8_t[MAX_NFC_PAGES * NFC_PAGE_SIZE];
    for (int i = 0; i < tag.pages_read && i < MAX_NFC_PAGES; i++) {
        memcpy(data + i * NFC_PAGE_SIZE, tag.page_data[i], NFC_PAGE_SIZE);
    }

    while (offset < max_offset) {
        uint8_t header = data[offset];
        bool mb = (header & 0x80) != 0;
        bool me = (header & 0x40) != 0;
        bool cf = (header & 0x20) != 0;
        bool sr = (header & 0x10) != 0;
        bool il = (header & 0x08) != 0;
        uint8_t tnf = header & 0x07;

        offset++;

        uint8_t type_length = data[offset++];

        uint32_t payload_length = 0;
        if (sr) {
            payload_length = data[offset++];
        } else {
            payload_length = (data[offset] << 24) | (data[offset+1] << 16) |
                           (data[offset+2] << 8) | data[offset+3];
            offset += 4;
        }

        uint8_t id_length = 0;
        if (il) {
            id_length = data[offset++];
        }

        String type_str;
        for (int i = 0; i < type_length; i++) {
            type_str += (char)data[offset++];
        }

        offset += id_length;

        NDEFRecord record;

        if (tnf == 0x01) {
            if (type_str == "U") {
                uint8_t uri_code = data[offset];
                String uri_prefix[] = {"", "http://www.", "https://www.", "http://", "https://",
                                      "tel:", "mailto:", "ftp://anonymous:anonymous@", "ftp://ftp.",
                                      "ftps://", "sftp://", "smb://", "nfs://", "ftp://",
                                      "dav://", "news:", "telnet://", "imap:", "rtsp://",
                                      "urn:", "pop:", "sip:", "sips:", "tftp:", "btspp://",
                                      "btl2cap://", "btgoep://", "tcpobex://", "irdaobex://",
                                      "file://", "urn:epc:id:", "urn:epc:tag:", "urn:epc:pat:",
                                      "urn:epc:raw:", "urn:epc:", "urn:nfc:"};

                String url;
                if (uri_code < sizeof(uri_prefix)/sizeof(uri_prefix[0])) {
                    url = uri_prefix[uri_code];
                }

                for (uint32_t i = 1; i < payload_length; i++) {
                    url += (char)data[offset + i];
                }

                record = NDEFRecord(NDEF_TYPE_URL, "URL", url);
                records.push_back(record);
            }
            else if (type_str == "T") {  // Text
                uint8_t status = data[offset];
                uint8_t lang_len = status & 0x3F;

                String language;
                for (int i = 0; i < lang_len; i++) {
                    language += (char)data[offset + 1 + i];
                }

                String text;
                for (uint32_t i = 1 + lang_len; i < payload_length; i++) {
                    text += (char)data[offset + i];
                }

                record = NDEFRecord(NDEF_TYPE_TEXT, "Text", text, "Lang: " + language);
                records.push_back(record);
            }
        }
        else if (tnf == 0x02) {  // MIME media type
            // Parse based on MIME type
            if (type_str == "text/x-vCard" || type_str.startsWith("text/vcard")) {
                String vcard;
                for (uint32_t i = 0; i < payload_length; i++) {
                    vcard += (char)data[offset + i];
                }
                record = NDEFRecord(NDEF_TYPE_VCARD, "vCard", vcard);
                records.push_back(record);
            }
        }
        else if (tnf == 0x04) {  // NFC Forum external type
            if (type_str == "android.com:pkg") {
                String package = "";
                for (uint32_t i = 0; i < payload_length; i++) {
                    package += (char)data[offset + i];
                }
                record = NDEFRecord(NDEF_TYPE_TEXT, "Android App", package);
                records.push_back(record);
            }
        }

        offset += payload_length;

        if (me) break;  // Message End
    }

    delete[] data;

    return records.size();
}
