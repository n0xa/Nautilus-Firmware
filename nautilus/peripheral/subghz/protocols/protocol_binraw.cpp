/**
 * BinRAW Protocol Implementation
 */

#include "protocol_binraw.h"

/**
 * BinRAW does not support RX decoding
 */
bool BinRAWProtocol::decode(const rmt_item32_t* data, size_t len, ProtocolDecodeResult& result) {
    (void)data;
    (void)len;
    result.valid = false;
    return false;
}

/**
 * Encode BinRAW data for transmission
 */
bool BinRAWProtocol::encode(const ProtocolEncodeParams& params) {
    if (blocks.empty()) {
        return false;
    }

    rmt_items.clear();

    for (size_t i = 0; i < blocks.size(); i++) {
        const BinRAW_Block& block = blocks[i];
        bitsToRMT(block.data, block.bit_count, te, false, rmt_items);
    }

    return true;
}

/**
 * Get encoded RMT data for transmission
 */
bool BinRAWProtocol::getEncodedData(const rmt_item32_t** out_data, size_t* out_len) {
    if (rmt_items.empty()) {
        return false;
    }

    *out_data = rmt_items.data();
    *out_len = rmt_items.size();
    return true;
}

/**
 * BinRAW serialization not supported
 */
bool BinRAWProtocol::serializeToFile(File& file, const ProtocolDecodeResult& result) {
    (void)file;
    (void)result;
    return false;
}

/**
 * Deserialize BinRAW data from .sub file
 */
bool BinRAWProtocol::deserializeFromFile(File& file, ProtocolEncodeParams& params) {
    String line;
    bool found_bit = false;
    bool found_te = false;

    blocks.clear();
    total_bits = 0;
    te = BINRAW_TE_DEFAULT;

    while (file.available()) {
        line = file.readStringUntil('\n');
        line.trim();

        if (line.startsWith("Bit:") && !found_bit) {
            total_bits = line.substring(4).toInt();
            found_bit = true;
        } else if (line.startsWith("TE:")) {
            te = line.substring(3).toInt();
            found_te = true;
            break;
        }
    }

    if (!found_bit) {
        return false;
    }

    if (!found_te) {
    }

    while (file.available() && blocks.size() < BINRAW_MAX_BLOCKS) {
        line = file.readStringUntil('\n');
        line.trim();

        if (line.startsWith("Bit_RAW:")) {
            BinRAW_Block block;
            block.bit_count = line.substring(8).toInt();
            block.byte_count = (block.bit_count + 7) / 8;

            if (block.byte_count > BINRAW_MAX_DATA_SIZE) {
                return false;
            }

            if (!file.available()) {
                return false;
            }

            line = file.readStringUntil('\n');
            line.trim();

            if (!line.startsWith("Data_RAW:")) {
                return false;
            }

            String data_str = line.substring(9);
            data_str.trim();

            memset(block.data, 0, sizeof(block.data));
            size_t byte_idx = 0;
            int start_pos = 0;

            while (start_pos < data_str.length() && byte_idx < block.byte_count) {
                int space_pos = data_str.indexOf(' ', start_pos);
                if (space_pos == -1) {
                    space_pos = data_str.length();
                }

                String hex_byte = data_str.substring(start_pos, space_pos);
                hex_byte.trim();

                if (hex_byte.length() > 0) {
                    unsigned long value = strtoul(hex_byte.c_str(), NULL, 16);
                    block.data[byte_idx++] = (uint8_t)value;
                }

                start_pos = space_pos + 1;
            }

            if (byte_idx != block.byte_count) {
            }

            blocks.push_back(block);
        }
    }

    if (blocks.empty()) {
        return false;
    }

    params.key = 0;
    params.bit_count = 0;
    params.te = te;
    params.repeat_count = 1;

    return true;
}

/**
 * Convert bit array to RMT items using run-length encoding
 * Based on Flipper Zero bin_raw.c
 */
void BinRAWProtocol::bitsToRMT(const uint8_t* data, uint16_t bit_count, uint32_t te_us,
                               bool start_level, std::vector<rmt_item32_t>& items) {
    (void)start_level;

    if (bit_count == 0) {
        return;
    }

    // Calculate bit alignment offset
    uint16_t bias_bit = 0;
    if (bit_count & 0x7) {
        bias_bit = 8 - (bit_count & 0x7);
    }

    rmt_item32_t current_item;
    bool item_has_duration0 = false;

    uint16_t i = bias_bit;
    uint16_t end_bit = bias_bit + bit_count;

    while (i < end_bit) {
        bool current_bit = getBit(data, i);
        uint32_t run_length = 1;

        while (i + run_length < end_bit && getBit(data, i + run_length) == current_bit) {
            run_length++;
        }

        uint32_t duration = run_length * te_us;
        bool level = current_bit;

        if (!item_has_duration0) {
            current_item.duration0 = US_TO_RMT_TICKS(duration);
            current_item.level0 = level;
            item_has_duration0 = true;
        } else {
            current_item.duration1 = US_TO_RMT_TICKS(duration);
            current_item.level1 = level;
            items.push_back(current_item);
            item_has_duration0 = false;
        }

        i += run_length;
    }

    if (item_has_duration0) {
        current_item.duration1 = 0;
        current_item.level1 = 0;
        items.push_back(current_item);
    }
}
