#include "ir.h"

// Helper: Get protocol info by name
const IRProtocolInfo* getProtocolInfo(const char* protocol_name) {
    for (int i = 0; i < IR_PROTOCOL_COUNT; i++) {
        if (strcasecmp(protocol_name, IR_PROTOCOLS[i].name) == 0) {
            return &IR_PROTOCOLS[i];
        }
    }
    return nullptr;
}

// Helper: Validate value against protocol mask
bool validateProtocolValue(uint32_t value, uint32_t mask) {
    return (value & ~mask) == 0;
}

// Helper: Convert 4-byte hex string to uint32_t
// Format: "XX XX XX XX" (little-endian)
// Example: "07 00 00 00" = 0x00000007
uint32_t hexStringToUint32(const String& hex_str) {
    uint32_t result = 0;
    String trimmed = hex_str;
    trimmed.trim();

    // Parse space-separated hex bytes
    int byte_index = 0;
    int start = 0;
    for (int i = 0; i <= trimmed.length() && byte_index < 4; i++) {
        if (i == trimmed.length() || trimmed[i] == ' ') {
            if (i > start) {
                String byte_str = trimmed.substring(start, i);
                uint8_t byte_val = strtoul(byte_str.c_str(), NULL, 16);
                result |= ((uint32_t)byte_val << (byte_index * 8));
                byte_index++;
            }
            start = i + 1;
        }
    }

    return result;
}

// Helper: Convert uint32_t to 4-byte hex string
// Format: "XX XX XX XX" (little-endian)
// Example: 0x00000007 = "07 00 00 00"
String uint32ToHexString(uint32_t value) {
    char buffer[12];
    snprintf(buffer, sizeof(buffer), "%02X %02X %02X %02X",
             (uint8_t)(value & 0xFF),
             (uint8_t)((value >> 8) & 0xFF),
             (uint8_t)((value >> 16) & 0xFF),
             (uint8_t)((value >> 24) & 0xFF));
    return String(buffer);
}

// Create default IR signal
IRSignal createDefaultIRSignal() {
    IRSignal signal;
    signal.name = "Button";
    signal.type = IR_SIGNAL_PARSED;
    signal.protocol = "NEC";
    signal.address = 0;
    signal.command = 0;
    signal.frequency = 38000;
    signal.duty_cycle = 0.33f;
    return signal;
}

// Parse Flipper Zero .ir file
bool parseFlipperIRFile(const char* filepath, IRRemote& remote) {
    File file = SD.open(filepath, FILE_READ);
    if (!file) {
        return false;
    }

    remote.clear();
    remote.filepath = filepath;

    String line;
    IRSignal current_signal;
    bool in_signal = false;
    bool valid_header = false;


    while (file.available()) {
        line = file.readStringUntil('\n');
        line.trim();

        // Skip empty lines and comments
        if (line.length() == 0 || line.startsWith("#")) {
            continue;
        }

        // Check for valid header
        if (line.startsWith("Filetype:")) {
            if (line.indexOf("IR signals file") != -1) {
                valid_header = true;
            }
            continue;
        }

        if (line.startsWith("Version:")) {
            continue;
        }

        // Parse signal fields
        if (line.startsWith("name:")) {
            // Save previous signal if exists
            if (in_signal && current_signal.name.length() > 0) {
                remote.addSignal(current_signal);
            }

            // Start new signal
            current_signal = createDefaultIRSignal();
            current_signal.name = line.substring(5);
            current_signal.name.trim();
            in_signal = true;
        }
        else if (line.startsWith("type:")) {
            String type_str = line.substring(5);
            type_str.trim();
            if (type_str.equalsIgnoreCase("parsed")) {
                current_signal.type = IR_SIGNAL_PARSED;
            } else if (type_str.equalsIgnoreCase("raw")) {
                current_signal.type = IR_SIGNAL_RAW;
            }
        }
        else if (line.startsWith("protocol:")) {
            current_signal.protocol = line.substring(9);
            current_signal.protocol.trim();
        }
        else if (line.startsWith("address:")) {
            String addr_str = line.substring(8);
            current_signal.address = hexStringToUint32(addr_str);

            // Validate address against protocol
            const IRProtocolInfo* proto = getProtocolInfo(current_signal.protocol.c_str());
            if (proto && !validateProtocolValue(current_signal.address, proto->address_mask)) {
                file.close();
                return false;
            }
        }
        else if (line.startsWith("command:")) {
            String cmd_str = line.substring(8);
            current_signal.command = hexStringToUint32(cmd_str);

            // Validate command against protocol
            const IRProtocolInfo* proto = getProtocolInfo(current_signal.protocol.c_str());
            if (proto && !validateProtocolValue(current_signal.command, proto->command_mask)) {
                file.close();
                return false;
            }
        }
        else if (line.startsWith("frequency:")) {
            current_signal.frequency = atoi(line.substring(10).c_str());
        }
        else if (line.startsWith("duty_cycle:")) {
            current_signal.duty_cycle = atof(line.substring(11).c_str());
        }
        else if (line.startsWith("data:")) {
            // Parse raw timing data (space-separated microsecond values)
            String data_str = line.substring(5);
            data_str.trim();

            current_signal.raw_data.clear();
            int start = 0;
            for (int i = 0; i <= data_str.length(); i++) {
                if (i == data_str.length() || data_str[i] == ' ') {
                    if (i > start) {
                        String value_str = data_str.substring(start, i);
                        uint16_t value = atoi(value_str.c_str());
                        if (value > 0 && current_signal.raw_data.size() < MAX_IR_RAW_DATA) {
                            current_signal.raw_data.push_back(value);
                        }
                    }
                    start = i + 1;
                }
            }

        }
    }

    // Add last signal
    if (in_signal && current_signal.name.length() > 0) {
        remote.addSignal(current_signal);
    }

    file.close();

    // Valid if header was found, even if no signals yet
    if (!valid_header) {
        return false;
    }

    return true;
}

// Write Flipper Zero .ir file
bool writeFlipperIRFile(const char* filepath, const IRRemote& remote) {
    if (remote.signals.size() == 0) {
        return false;
    }

    File file = SD.open(filepath, FILE_WRITE);
    if (!file) {
        return false;
    }


    // Write header
    file.println("Filetype: IR signals file");
    file.println("Version: 1");

    // Write each signal
    for (int i = 0; i < remote.signals.size(); i++) {
        const IRSignal& signal = remote.signals[i];

        // Comment separator
        file.println("#");

        // Signal name
        file.print("name: ");
        file.println(signal.name);

        // Signal type and protocol-specific data
        if (signal.type == IR_SIGNAL_PARSED) {
            file.println("type: parsed");
            file.print("protocol: ");
            file.println(signal.protocol);
            file.print("address: ");
            file.println(uint32ToHexString(signal.address));
            file.print("command: ");
            file.println(uint32ToHexString(signal.command));

        }
        else if (signal.type == IR_SIGNAL_RAW) {
            file.println("type: raw");
            file.print("frequency: ");
            file.println(signal.frequency);
            file.print("duty_cycle: ");
            file.println(signal.duty_cycle, 5);  // 5 decimal places to match Flipper format
            file.print("data: ");

            // Write raw data (space-separated)
            for (int j = 0; j < signal.raw_data.size(); j++) {
                file.print(signal.raw_data[j]);
                if (j < signal.raw_data.size() - 1) {
                    file.print(" ");
                }
            }
            file.println();

        }
    }

    file.close();
    return true;
}

uint8_t bitReverse(uint8_t b) {
    b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
    b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
    b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
    return b;
}

uint32_t flipperNECToLibrary(uint8_t addr, uint8_t cmd) {
    uint8_t frame[4] = {addr, (uint8_t)~addr, cmd, (uint8_t)~cmd};

    for (int i = 0; i < 4; i++) {
        frame[i] = bitReverse(frame[i]);
    }

    uint32_t result = (frame[0] << 24) | (frame[1] << 16) | (frame[2] << 8) | frame[3];

    return result;
}

int samsung32ToRawTiming(uint8_t addr, uint8_t cmd, uint16_t* rawData, int maxLen) {
    if (maxLen < 68) return 0;

    uint8_t bytes[4];
    bytes[0] = addr;
    bytes[1] = addr;
    bytes[2] = cmd;
    bytes[3] = ~cmd;

    int idx = 0;
    rawData[idx++] = 4500;
    rawData[idx++] = 4500;

    for (int byte_idx = 0; byte_idx < 4; byte_idx++) {
        for (int bit = 0; bit < 8; bit++) {
            if (bytes[byte_idx] & (1 << bit)) {
                rawData[idx++] = 560;
                rawData[idx++] = 1690;
            } else {
                rawData[idx++] = 560;
                rawData[idx++] = 560;
            }
        }
    }

    rawData[idx++] = 560;

    return idx;
}

bool transmitIRSignal(const IRSignal& signal, IRsend& irsend) {
    irsend.begin();

    if (signal.type == IR_SIGNAL_PARSED) {
        uint16_t rawData[256];
        int rawLen = 0;

        if (signal.protocol.equalsIgnoreCase("NEC")) {
            uint8_t addr = signal.address & 0xFF;
            uint8_t cmd = signal.command & 0xFF;

            // Convert from Flipper format to library format
            uint32_t data = flipperNECToLibrary(addr, cmd);

            irsend.sendNEC(data, 32);
            return true;
        }
        else if (signal.protocol.equalsIgnoreCase("NECext")) {
            // NECext uses 16-bit address (no inverse), 8-bit command + inverse
            uint16_t addr = signal.address & 0xFFFF;
            uint8_t cmd = signal.command & 0xFF;

            // Build extended NEC format
            uint32_t data = ((uint32_t)(~cmd & 0xFF) << 24) | ((uint32_t)cmd << 16) | addr;


            // Use same timing as NEC but with different data format (STANDARD format)
            int idx = 0;
            // Header: 9000µs mark, 4500µs space
            rawData[idx++] = 9000;
            rawData[idx++] = 4500;
            for (int i = 0; i < 32; i++) {
                if (data & (1UL << i)) {
                    // Logical 1: 560µs mark, 1690µs space
                    rawData[idx++] = 560;
                    rawData[idx++] = 1690;
                } else {
                    // Logical 0: 560µs mark, 560µs space
                    rawData[idx++] = 560;
                    rawData[idx++] = 560;
                }
            }
            rawData[idx++] = 560;

            irsend.sendRaw(rawData, idx, 38);
            return true;
        }
        else if (signal.protocol.equalsIgnoreCase("Samsung32") || signal.protocol.equalsIgnoreCase("Samsung")) {
            uint8_t addr = signal.address & 0xFF;
            uint8_t cmd = signal.command & 0xFF;

            // Convert to raw timing with proper 4500/4500 header
            rawLen = samsung32ToRawTiming(addr, cmd, rawData, 256);
            if (rawLen > 0) {
                irsend.sendRaw(rawData, rawLen, 38);
                return true;
            }
        }
        else {
            return false;
        }
    }
    else if (signal.type == IR_SIGNAL_RAW) {
        // Transmit raw signal
        if (signal.raw_data.size() == 0) {
            return false;
        }


        // Convert vector to array for sendRaw
        uint16_t* raw_array = new uint16_t[signal.raw_data.size()];
        for (int i = 0; i < signal.raw_data.size(); i++) {
            raw_array[i] = signal.raw_data[i];
        }

        irsend.sendRaw(raw_array, signal.raw_data.size(), signal.frequency / 1000); // Convert Hz to kHz

        delete[] raw_array;
        return true;
    }

    return false;
}

// Convert IRremoteESP8266 decode_results to IRSignal
bool decodeResultsToIRSignal(const decode_results& results, IRSignal& signal) {
    // Get protocol name
    signal.protocol = typeToString(results.decode_type);

    // Check if it's a known protocol we support
    if (results.decode_type == decode_type_t::NEC) {
        signal.type = IR_SIGNAL_PARSED;
        signal.protocol = "NEC";
        // NEC: Use library's decoded address and command fields
        signal.address = results.address & 0xFF;
        signal.command = results.command & 0xFF;

        return true;
    }
    else if (results.decode_type == decode_type_t::SAMSUNG) {
        signal.type = IR_SIGNAL_PARSED;
        signal.protocol = "Samsung32";
        // Samsung: Use library's decoded address and command fields
        signal.address = results.address & 0xFF;
        signal.command = results.command & 0xFF;

        return true;
    }
    else if (results.decode_type == decode_type_t::UNKNOWN) {
        // Unknown protocol - use raw data
        signal.type = IR_SIGNAL_RAW;
        signal.protocol = "RAW";
        signal.frequency = 38000; // Default IR frequency
        signal.duty_cycle = 0.33f; // Default duty cycle
        signal.raw_data.clear();

        // Copy raw timings
        for (uint16_t i = 1; i < results.rawlen && i < MAX_IR_RAW_DATA; i++) {
            uint16_t timing = results.rawbuf[i] * kRawTick;
            signal.raw_data.push_back(timing);
        }

        return true;
    }
    else {
        // Other protocols - store as raw for now
        signal.type = IR_SIGNAL_RAW;
        signal.frequency = 38000;
        signal.duty_cycle = 0.33f;
        signal.raw_data.clear();

        for (uint16_t i = 1; i < results.rawlen && i < MAX_IR_RAW_DATA; i++) {
            uint16_t timing = results.rawbuf[i] * kRawTick;
            signal.raw_data.push_back(timing);
        }

        return true;
    }

    return false;
}

// Generate sequential remote filename
String generateRemoteName() {
    int index = 0;
    String filename;
    String fullpath;

    do {
        filename = "Remote_" + String(index);
        fullpath = "/ir/" + filename + ".ir";
        index++;
    } while (SD.exists(fullpath));

    return filename;
}

// Generate sequential button name
String generateButtonName(const IRRemote& remote) {
    int index = 0;
    String name;
    bool found;

    do {
        name = "Btn_" + String(index);
        found = false;

        // Check if this name already exists in remote
        for (int i = 0; i < remote.signals.size(); i++) {
            if (remote.signals[i].name == name) {
                found = true;
                break;
            }
        }

        index++;
    } while (found);

    return name;
}

// Create empty remote file
bool createEmptyRemote(const char* filepath) {
    // Create /ir directory if it doesn't exist
    if (!SD.exists("/ir")) {
        SD.mkdir("/ir");
    }

    File file = SD.open(filepath, FILE_WRITE);
    if (!file) {
        return false;
    }

    // Write header only
    file.println("Filetype: IR signals file");
    file.println("Version: 1");
    file.close();

    return true;
}

// Append signal to remote
bool appendSignalToRemote(const char* filepath, const IRSignal& signal) {
    IRRemote remote;

    // Load existing remote (if it exists)
    if (SD.exists(filepath)) {
        if (!parseFlipperIRFile(filepath, remote)) {
            return false;
        }
    }

    // Add new signal
    remote.filepath = filepath;
    remote.addSignal(signal);

    // Write back
    return writeFlipperIRFile(filepath, remote);
}

// Replace signal in remote
bool replaceSignalInRemote(const char* filepath, int index, const IRSignal& signal) {
    IRRemote remote;

    if (!parseFlipperIRFile(filepath, remote)) {
        return false;
    }

    if (index < 0 || index >= remote.signals.size()) {
        return false;
    }

    // Replace signal
    remote.signals[index] = signal;

    // Write back
    return writeFlipperIRFile(filepath, remote);
}

// Delete signal from remote
bool deleteSignalFromRemote(const char* filepath, int index) {
    IRRemote remote;

    if (!parseFlipperIRFile(filepath, remote)) {
        return false;
    }

    if (index < 0 || index >= remote.signals.size()) {
        return false;
    }

    // Remove signal
    remote.signals.erase(remote.signals.begin() + index);

    // Write back (or delete file if empty)
    if (remote.signals.size() == 0) {
        SD.remove(filepath);
        return true;
    }

    return writeFlipperIRFile(filepath, remote);
}
