#pragma once
#include "../utilities.h"
#include "FS.h"
#include "SPIFFS.h"

/**----------------------------- WS2812 ----------------------------------**/
// Configure FastLED to use ESP core RMT driver instead of taking over interrupt handler
// This allows other RMT applications (SubGHz) to coexist
#define FASTLED_RMT_BUILTIN_DRIVER 1
// Limit FastLED to 1 RMT channel (channel 0), leaving channels 6 and 7 for SubGHz
#define FASTLED_RMT_MAX_CHANNELS 1
#define FASTLED_ESP32_RMT_CHANNEL_0 0
#include <FastLED.h>
#define WS2812_DEFAULT_LIGHT 15
extern TaskHandle_t ws2812_handle;

void ws2812_init(void);
void ws2812_set_color(CRGB c);
void ws2812_set_light(uint8_t light);
void ws2812_set_mode(int m);
int ws2812_get_mode(void);
void ws2812_task(void *param);
void ws2812_pos_demo(int pos);
void ws2812_pos_demo1(void);
/**------------------------------- NFC -----------------------------------**/
// PN532
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_PN532.h>
extern TaskHandle_t nfc_handle;
void nfc_init(void);
bool nfc_is_init(void);
uint32_t nfc_get_ver_data(void);
void nfc_task(void *param);

/**------------------------------ SGHZ -----------------------------------**/
// CC1101
#include <RadioLib.h>

#define SGHZ_MODE_SEND 1
#define SGHZ_MODE_RECV 2

extern TaskHandle_t sghz_handle;
extern SemaphoreHandle_t radioLock;
extern CC1101 radio;

void sghz_init(void);
void sghz_mode_sw(int m);
int sghz_get_mode(void);
bool sghz_is_init(void);
void sghz_send(const char *str);
void sghz_task(void *param);

/**----------------------------- SUBGHZ -----------------------------------**/
// SubGHz Radio Module - RF Signal Capture and Playback
#include "peripheral/peri_subghz.h"

/**------------------------------ NRF24 -----------------------------------**/
// NRF24
#define NRF24_MODE_SEND 0
#define NRF24_MODE_RECV 1

extern TaskHandle_t nrf24_handle;

void nrf24_init(void);
bool nrf24_is_init();
int nrf24_get_mode(void);
void nrf24_set_mode(int mode);
void nrf24_task(void *param);
void nrf24_send(const char *str);

/**---------------------------- BATTERY ----------------------------------**/
#define XPOWERS_CHIP_BQ25896
#include "XPowersLib.h"

extern XPowersPPM PPM;

#include "bq27220.h"
extern BQ27220 bq27220;

/**------------------------------ MIC ------------------------------------**/
#define SAMPLE_SIZE         (20)
#define EXAMPLE_I2S_CH      0        // I2S Channel Number

void init_microphone(void);

/**------------------------------- IR ------------------------------------**/
#define IR_MODE_SEND 1
#define IR_MODE_RECV 2

/**------------------------------- SD ------------------------------------**/
#include "FS.h"
#include "SD.h"
#include "SPI.h"
extern bool sd_init_flag;
void sd_init(void);
bool sd_is_valid(void);
uint32_t sd_get_sum_Mbyte(void);
uint32_t sd_get_used_Mbyte(void);
bool sd_mount(void);
void sd_unmount(void);

/**---------------------------- CONFIG -----------------------------------**/
// Persistent configuration stored in nautilus.json on SD card
#include "peripheral/peri_config.h"

/**----------------------------- WIFI ------------------------------------**/
#include <WiFi.h>
#define WIFI_SSID_MAX_LEN 30
#define WIFI_PSWD_MAX_LEN 30
extern char wifi_ssid[WIFI_SSID_MAX_LEN];
extern char wifi_password[WIFI_PSWD_MAX_LEN];
extern bool wifi_is_connect;

/**---------------------------- EEPROM -----------------------------------**/
#include <EEPROM.h>
#define EEPROM_UPDATA_FLAG_NUM   0xAA
#define WIFI_SSID_EEPROM_ADDR    (1)
#define WIFI_PSWD_EEPROM_ADDR    (WIFI_SSID_EEPROM_ADDR + WIFI_SSID_MAX_LEN)
#define UI_THEME_EEPROM_ADDR     (WIFI_PSWD_EEPROM_ADDR + WIFI_PSWD_MAX_LEN)
#define UI_ROTATION_EEPROM_ADDR  (UI_THEME_EEPROM_ADDR + 1)
#define EEPROM_SIZE_MAX 128

void eeprom_wr(int addr, uint8_t val);
void eeprom_wr_wifi(const char *ssid, uint16_t ssid_len, const char *pwsd, uint16_t pwsd_len);