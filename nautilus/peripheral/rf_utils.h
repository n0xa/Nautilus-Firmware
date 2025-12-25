/**
 * RF Utils - CC1101 Radio Utilities
 * Provides CC1101 initialization with SPI bus sharing support
 */

#ifndef __RF_UTILS_H__
#define __RF_UTILS_H__

#include <Arduino.h>
#include <ELECHOUSE_CC1101_SRC_DRV.h>
#include <driver/rmt.h>
#include <vector>

// RMT Configuration
#define SUBGHZ_RMT_RX_CHANNEL RMT_CHANNEL_6
#define SUBGHZ_RMT_TX_CHANNEL RMT_CHANNEL_7
#define SUBGHZ_RMT_MAX_PULSES 10000
#define SUBGHZ_RMT_CLK_DIV 80
#define SUBGHZ_RMT_1US_TICKS (80000000 / SUBGHZ_RMT_CLK_DIV / 1000000)
#define SUBGHZ_RMT_1MS_TICKS (SUBGHZ_RMT_1US_TICKS * 1000)

// Frequency lists
extern const float subghz_frequency_list[57];
extern const float subghz_mhz_list[277];
extern const char *subghz_frequency_ranges[];
extern const int range_limits[4][2];

// Combined frequency list (hardcoded + custom from config)
std::vector<float> rf_get_combined_frequency_list();
int rf_get_combined_frequency_count();
void rf_invalidate_frequency_cache();  // Call when custom frequencies change

// RfCodes structure is defined in peri_subghz.h
// (We use Nautilus's existing definition to avoid duplication)

// Error tracking
enum RfTransmitError {
    RF_TX_SUCCESS = 0,
    RF_TX_USER_STOPPED,
    RF_TX_UNSUPPORTED_PROTOCOL,
    RF_TX_FILE_ERROR,
    RF_TX_ENCODING_ERROR
};

extern RfTransmitError rf_last_error;
const char* rf_get_error_string(RfTransmitError error);

// Core functions
bool rf_initModule(String mode = "", float frequency = 0);
void rf_deinitModule();
void rf_initCC1101(SPIClass *SSPI);
void rf_setFrequency(float frequency);

// Transmission functions
bool rf_sendRaw(int *ptrtransmittimings);
bool rf_sendRMT(const rmt_item32_t* items, size_t len, bool skip_reset = false);
void rf_sendRCSwitch(uint64_t data, unsigned int bits, int pulse = 0, int protocol = 1, int repeat = 10);
bool rf_transmitFile(String filepath);
bool rf_sendCommand(struct RfCodes rfcode);

// Utility functions
int find_pulse_index(const std::vector<int> &indexed_durations, int duration);
uint64_t crc64_ecma(const std::vector<int> &data);

#endif // __RF_UTILS_H__
