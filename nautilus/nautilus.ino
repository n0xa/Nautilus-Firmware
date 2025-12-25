
#include "utilities.h"
#define XPOWERS_CHIP_BQ25896
#include <XPowersLib.h>
#include "ui.h"
#include "TFT_eSPI.h"
#include "Audio.h"
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <IRrecv.h>
#include <IRutils.h>
#include <vector>
#include "portal.h"

const uint16_t kIrLed = 2;  // ESP8266 GPIO pin to use. Recommended: 4 (D2).
const uint16_t kRecvPin = 1;
IRsend irsend(kIrLed);  // Set the GPIO to be used to sending the message.
IRrecv irrecv(kRecvPin);
decode_results results;

uint64_t IR_recv_value = 0x10000000UL;

/*********************************************************************************
 *                               DEFINE
 *********************************************************************************/
#define FORMAT_SPIFFS_IF_FAILED true

#define NFC_PRIORITY     (configMAX_PRIORITIES - 1)
#define SGHZ_PRIORITY    (configMAX_PRIORITIES - 2)
#define WS2812_PRIORITY  (configMAX_PRIORITIES - 3)
#define NRF24_PRIORITY   (configMAX_PRIORITIES - 5)

/*********************************************************************************
 *                              EXTERN
 *********************************************************************************/
extern uint8_t display_rotation;
extern uint8_t setting_theme;
extern float selected_frequency;

/*********************************************************************************
 *                              GLOBAL VARIABLES
 *********************************************************************************/
// Volume control variables
uint8_t current_volume = 21;  // 0-21, start at max
int last_encoder_pos = 0;
bool volume_changed_during_playback = false;

/*********************************************************************************
 *                              TYPEDEFS
 *********************************************************************************/

Audio audio(false, 3, I2S_NUM_1);
XPowersPPM PPM;
BQ27220 bq27220;
uint32_t cycleInterval;
SemaphoreHandle_t radioLock;

TaskHandle_t nfc_handle;
TaskHandle_t sghz_handle;
TaskHandle_t ws2812_handle;
TaskHandle_t nrf24_handle;

// wifi
// char wifi_ssid[WIFI_SSID_MAX_LEN] = "xinyuandianzi";
// char wifi_password[WIFI_PSWD_MAX_LEN] = "AA15994823428";
char wifi_ssid[WIFI_SSID_MAX_LEN] = {0};
char wifi_password[WIFI_PSWD_MAX_LEN] = {0};
const char *ntpServer1 = "pool.ntp.org";
const char* ntpServer2 = "time.nist.gov";
bool wifi_is_connect = false;
bool wifi_eeprom_upd = false;
static struct tm timeinfo;
static uint32_t last_tick;

// eeprom
uint8_t eeprom_ssid[WIFI_SSID_MAX_LEN];
uint8_t eeprom_pswd[WIFI_PSWD_MAX_LEN];

// mic
int16_t i2s_readraw_buff[SAMPLE_SIZE] = {0};
size_t bytes_read = 0;
int i2s_mic_cnt = 0;

// BOARD_USER_KEY (GPIO 6) - Back/Sleep Button
volatile unsigned long userKeyPressStart = 0;
volatile bool userKeyPressed = false;
volatile bool userKeyLongPressTriggered = false;
bool backButtonPressedFlag = false;

#define LONG_PRESS_DURATION 3000  // 3 seconds in milliseconds
#define DEBOUNCE_DELAY 50         // 50ms debounce

// Sleep countdown overlay
lv_obj_t *sleep_countdown_label = NULL;

// External variables from ui.cpp
extern bool music_is_running;

// External functions from ui_interface.cpp
extern void ui_scr1_set_mode(int mode);

/*********************************************************************************
 *                              FUNCTION
 *********************************************************************************/

// Helper function to stop music playback and update state
void stop_music_playback() {
    if (music_is_running) {
        audio.stopSong();
        music_is_running = false;
        Serial.println("Stopped music playback");
    }
}

void eeprom_default_val(void)
{
    char wifi_ssid[WIFI_SSID_MAX_LEN] = "xinyuandianzi";
    char wifi_password[WIFI_PSWD_MAX_LEN] = "AA15994823428";

    EEPROM.write(0, EEPROM_UPDATA_FLAG_NUM);
    for(int i = WIFI_SSID_EEPROM_ADDR; i < WIFI_SSID_EEPROM_ADDR + WIFI_SSID_MAX_LEN; i++) {
        int k = i - WIFI_SSID_EEPROM_ADDR;
        if(k < WIFI_SSID_MAX_LEN) {
            EEPROM.write(i, wifi_ssid[k]);
        } else {
            EEPROM.write(i, 0x00);
        }
    }
    for(int i = WIFI_PSWD_EEPROM_ADDR; i < WIFI_PSWD_EEPROM_ADDR + WIFI_PSWD_MAX_LEN; i++) {
        int k = i - WIFI_PSWD_EEPROM_ADDR;
        if(k < WIFI_PSWD_MAX_LEN) {
            EEPROM.write(i, wifi_password[k]);
        } else {
            EEPROM.write(i, 0x00);
        }
    }
    EEPROM.commit();
    wifi_eeprom_upd = true;
}

void eeprom_wr(int addr, uint8_t val)
{
    if(wifi_eeprom_upd == false) {
        eeprom_default_val();
    }
    EEPROM.write(addr, val);
    EEPROM.commit();
    Serial.printf("eeprom_wr %d:%d\n", addr, val);
}

void eeprom_wr_wifi(const char *ssid, uint16_t ssid_len, const char *pwsd, uint16_t pwsd_len)
{
    Serial.printf("eeprom_wr_wifi \n%s:%d\n%s:%d\n", ssid, ssid_len, pwsd, pwsd_len);
    if(ssid_len > WIFI_SSID_MAX_LEN) 
        ssid_len = WIFI_SSID_MAX_LEN;
    if(pwsd_len > WIFI_PSWD_MAX_LEN)
        pwsd_len = WIFI_PSWD_MAX_LEN;

    if(wifi_eeprom_upd == false) {
        EEPROM.write(0, EEPROM_UPDATA_FLAG_NUM);
        wifi_eeprom_upd = true;
    }

    for(int i = WIFI_SSID_EEPROM_ADDR; i < WIFI_SSID_EEPROM_ADDR + WIFI_SSID_MAX_LEN; i++) {
        int k = i - WIFI_SSID_EEPROM_ADDR;
        if(k < ssid_len) {
            EEPROM.write(i, ssid[k]);
        } else {
            EEPROM.write(i, 0x00);
        }
    }
    for(int i = WIFI_PSWD_EEPROM_ADDR; i < WIFI_PSWD_EEPROM_ADDR + WIFI_PSWD_MAX_LEN; i++) {
        int k = i - WIFI_PSWD_EEPROM_ADDR;
        if(k < pwsd_len) {
            EEPROM.write(i, pwsd[k]);
        } else {
            EEPROM.write(i, 0x00);
        }
    }
    EEPROM.commit();
}

void eeprom_init()
{
    if (!EEPROM.begin(EEPROM_SIZE_MAX)) {
        Serial.println("failed to initialise EEPROM"); delay(1000000);
    }
    uint8_t frist_flag = EEPROM.read(0);
    if(frist_flag == EEPROM_UPDATA_FLAG_NUM) {
        for(int i = WIFI_SSID_EEPROM_ADDR; i < WIFI_SSID_EEPROM_ADDR + WIFI_SSID_MAX_LEN; i++) {
            wifi_ssid[i - WIFI_SSID_EEPROM_ADDR] = EEPROM.read(i);
        }
        for(int i = WIFI_PSWD_EEPROM_ADDR; i < WIFI_PSWD_EEPROM_ADDR + WIFI_PSWD_MAX_LEN; i++) {
            wifi_password[i - WIFI_PSWD_EEPROM_ADDR] = EEPROM.read(i);
        }

        wifi_eeprom_upd = true;
        

        uint8_t theme = EEPROM.read(UI_THEME_EEPROM_ADDR);
        uint8_t rotation = EEPROM.read(UI_ROTATION_EEPROM_ADDR);

        setting_theme = (theme == 255 ? 0 : theme);
        display_rotation = (rotation == 1 ? 1 : 3);

        Serial.println("*************** eeprom ****************");
        Serial.printf("eeprom flag: %d\n", frist_flag);
        Serial.printf("eeprom SSID: %s\n", wifi_ssid);
        Serial.printf("eeprom PWSD: %s\n", wifi_password);
        Serial.printf("eeprom theme: %d\n", theme);
        Serial.printf("eeprom rotation: %d\n", rotation);
        Serial.println("***************************************");
    }
}

void multi_thread_create(void)
{
    xTaskCreate(nfc_task, "nfc_task", 1024 * 3, NULL, NFC_PRIORITY, &nfc_handle);
    xTaskCreate(sghz_task, "sghz_task", 1024 * 2, NULL, SGHZ_PRIORITY, &sghz_handle);
    xTaskCreate(ws2812_task, "ws2812_task", 1024 * 2, NULL, WS2812_PRIORITY, &ws2812_handle);
    xTaskCreate(nrf24_task, "nrf24_task", 1024 * 10, NULL, NRF24_PRIORITY, &nrf24_handle);
}

void wifi_init(void)
{
    Serial.printf("SSID len: %d\n", strlen(wifi_ssid));
    Serial.printf("PWSD len: %d\n", strlen(wifi_password));
    if(strlen(wifi_ssid) == 0 || strlen(wifi_password) == 0) {
        return;
    }

    WiFi.begin(wifi_ssid, wifi_password);
    wl_status_t wifi_state = WiFi.status();
    last_tick = millis();
    while (wifi_state != WL_CONNECTED){
        delay(500);
        Serial.print(".");
        wifi_state = WiFi.status();
        if(wifi_state == WL_CONNECTED){
            wifi_is_connect = true;
            Serial.println("WiFi connected!");
            configTime(8 * 3600, 0, ntpServer1, ntpServer2);
            break;
        }
        if (millis() - last_tick > 5000) {
            Serial.println("WiFi connected falied!");
            last_tick = millis();
            break;
        }
    }
}

static void msg_send_event(lv_timer_t *t)
{
    if(wifi_is_connect == true){
        if (!getLocalTime(&timeinfo)){
            Serial.println("Failed to obtain time");
            return;
        }
        // Serial.println(&timeinfo, "%F %T %A");
        timeinfo.tm_hour = timeinfo.tm_hour % 12;
        lv_msg_send(MSG_CLOCK_HOUR, &timeinfo.tm_hour);
        lv_msg_send(MSG_CLOCK_MINUTE, &timeinfo.tm_min);
        lv_msg_send(MSG_CLOCK_SECOND, &timeinfo.tm_sec);
    }
}

static void msg_subsribe_event(void * s, lv_msg_t * msg)
{
    LV_UNUSED(s);

    switch (msg->id)
    {
        case MSG_UI_ROTATION_ST:{ 
            
        }break;
        case MSG_UI_THEME_MODE:{
            
        }break;
    
        default:
            break;
    }
}

void listDir(fs::FS &fs, const char * dirname, uint8_t levels){
    Serial.printf("Listing spiffs directory: %s\n", dirname);

    File root = fs.open(dirname);
    if(!root){
        Serial.println("- failed to open directory");
        return;
    }
    if(!root.isDirectory()){
        Serial.println(" - not a directory");
        return;
    }

    File file = root.openNextFile();
    while(file){
        if(file.isDirectory()){
            Serial.print("  DIR : ");
            Serial.println(file.name());
            if(levels){
                listDir(fs, file.path(), levels -1);
            }
        } else {
            Serial.print("  FILE: ");
            Serial.print(file.name());
            Serial.print("\tSIZE: ");
            Serial.println(file.size());
        }
        file = root.openNextFile();
    }
}

extern int music_idx;
extern std::vector<String> music_list;
extern TFT_eSPI tft;

void scr8_read_music_from_SD(void)
{
    // Try /music first, fall back to root if it doesn't exist
    File root = SD.open("/music");
    if(!root){
        Serial.println("/music not found, trying root directory");
        root = SD.open("/");
        if(!root) {
            Serial.println("Failed to open directory");
            return;
        }
    }

    music_list.clear();  // Clear any existing entries

    File file = root.openNextFile();
    // Load all music files - no artificial limit!
    // Add watchdog reset to prevent timeout with many files
    while(file) {
        if(!file.isDirectory()) {
            char *file_name = (char *)file.name();
            // Only add .mp3 files
            if(strlen(file_name) > 4 && strcmp(file_name + strlen(file_name) - 4, ".mp3") == 0) {
                music_list.push_back(String(file_name));
                Serial.println(file_name);
            }
        }
        file.close();

        // Yield to prevent watchdog timeout when scanning large directories
        delay(1);

        file = root.openNextFile();
    }
    root.close();
    Serial.printf("Loaded %d music files\n", music_list.size());
}

/*********************************************************************************
 *                   BOARD_USER_KEY (Back/Sleep Button) Functions
 *********************************************************************************/

// Show sleep countdown overlay
void show_sleep_countdown(int seconds_remaining) {
    if (sleep_countdown_label == NULL) {
        // Create overlay label on first call
        sleep_countdown_label = lv_label_create(lv_scr_act());
        lv_obj_set_style_text_color(sleep_countdown_label, lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_bg_color(sleep_countdown_label, lv_color_hex(0x000000), 0);
        lv_obj_set_style_bg_opa(sleep_countdown_label, LV_OPA_80, 0);
        lv_obj_set_style_pad_all(sleep_countdown_label, 20, 0);
        lv_obj_set_style_radius(sleep_countdown_label, 10, 0);  // Rounded corners
        lv_obj_center(sleep_countdown_label);
    }

    // Update countdown text
    char buf[32];
    lv_snprintf(buf, sizeof(buf), "Sleep in %d...", seconds_remaining);
    lv_label_set_text(sleep_countdown_label, buf);
    lv_obj_center(sleep_countdown_label);
}

// Hide sleep countdown overlay
void hide_sleep_countdown() {
    if (sleep_countdown_label != NULL) {
        lv_obj_del(sleep_countdown_label);
        sleep_countdown_label = NULL;
    }
}

// Parent screen mapping for smart back navigation
// Maps screen ID -> parent screen ID for logical navigation hierarchy
int get_parent_screen(int current_screen) {
    // SubGHz screens -> SubGHz menu (note: SCREEN2_5/2_6 are after SCREEN11 in enum!)
    if ((current_screen >= SCREEN2_1_ID && current_screen <= SCREEN2_4_1_ID) ||
        (current_screen >= SCREEN2_5_ID && current_screen <= SCREEN2_6_1_ID)) {
        return SCREEN2_ID;
    }
    // NFC screens -> NFC menu
    if (current_screen >= SCREEN3_1_ID && current_screen <= SCREEN3_2_1_ID) {
        return SCREEN3_ID;
    }
    // Settings screens -> Main menu
    if (current_screen == SCREEN4_1_ID) {
        return SCREEN4_ID;  // About screen -> Settings menu
    }
    // WiFi screens -> WiFi menu (includes AP Details SCREEN6_1_1_ID)
    if (current_screen >= SCREEN6_1_ID && current_screen <= SCREEN6_3_ID) {
        return SCREEN6_ID;
    }
    // IR screens -> IR menu
    if (current_screen >= SCREEN7_1_ID && current_screen <= SCREEN7_4_4_ID) {
        return SCREEN7_ID;
    }
    // File browser screens -> SD Card menu
    if (current_screen >= SCREEN10_1_ID && current_screen <= SCREEN10_3_ID) {
        return SCREEN10_ID;
    }
    // Portal screens -> WiFi menu
    if (current_screen == SCREEN12_ID) {
        return SCREEN6_ID;  // Portal -> WiFi menu
    }
    if (current_screen == SCREEN12_1_ID) {
        return SCREEN12_ID;  // Portal Data Viewer -> Portal screen
    }

    // Default: return to main menu
    // This handles: SCREEN1, 4, 5, 8, 9, 11 (WS2812, Settings, Battery, Music, NRF24, Mic)
    return SCREEN0_ID;
}

// Handle back button press - go back one screen or to parent menu
void handle_back_button() {
    Serial.println("Back button pressed - going back");

    // Stop music if playing
    stop_music_playback();

    // Try to pop back to previous screen
    extern bool scr_mgr_pop(bool anim);
    extern bool scr_mgr_switch(int id, bool anim);
    extern scr_card_t *scr_stack_top;

    if (!scr_mgr_pop(true)) {
        // If pop fails (we're at root), go to parent menu based on current screen
        int current_screen = (scr_stack_top != NULL) ? scr_stack_top->id : SCREEN0_ID;
        int parent_screen = get_parent_screen(current_screen);

        Serial.printf("At root screen %d, returning to parent screen %d\n", current_screen, parent_screen);
        scr_mgr_switch(parent_screen, true);
    }

    // Set flag that other parts of the code can check if needed
    backButtonPressedFlag = true;
}

// Handle sleep request - enter deep sleep mode
void handle_sleep_request() {
    Serial.println("Entering sleep mode...");

    // Save configuration before sleeping
    if (config_save()) {
        Serial.println("[CONFIG] Saved configuration before sleep");
    }

    // Gracefully shutdown peripherals
    Serial.println("Shutting down peripherals...");

    // Stop audio if playing
    stop_music_playback();

    // Clear the LVGL screen and display framebuffer so we don't see old content on wake
    lv_obj_t *scr = lv_scr_act();
    lv_obj_clean(scr);  // Remove all children from active screen
    lv_obj_set_style_bg_color(scr, lv_color_black(), LV_PART_MAIN);
    lv_refr_now(NULL);  // Force immediate redraw to clear framebuffer

    extern TFT_eSPI tft;
    tft.fillScreen(TFT_BLACK);

    // Turn off LEDs
    ws2812_set_color(CRGB::Black);

    // Turn off display backlight
    digitalWrite(TFT_BL, LOW);

    // IMPORTANT: Wait for button to be released before sleeping
    // Otherwise we'll wake immediately since button is still pressed
    Serial.println("Waiting for button release...");
    while (digitalRead(BOARD_USER_KEY) == LOW) {
        delay(10);
    }
    delay(100);  // Extra delay to ensure clean release

    // Configure wake source - wake on BOARD_USER_KEY press
    esp_sleep_enable_ext1_wakeup((1ULL << BOARD_USER_KEY), ESP_EXT1_WAKEUP_ANY_LOW);

    Serial.println("Going to deep sleep...");
    delay(100);  // Give serial time to print

    // Enter deep sleep
    esp_deep_sleep_start();
}

// Monitor user key for short press (back) and long press (sleep)
void monitor_user_key() {
    static unsigned long lastCheckTime = 0;
    static unsigned long lastDebounceTime = 0;
    static bool lastKeyState = HIGH;

    // Throttle: only check button every 20ms to avoid interfering with audio/display
    unsigned long currentTime = millis();
    if (currentTime - lastCheckTime < 20) {
        return;
    }
    lastCheckTime = currentTime;

    bool currentKeyState = digitalRead(BOARD_USER_KEY);

    // Debounce
    if (currentKeyState != lastKeyState) {
        lastDebounceTime = millis();
    }

    if ((millis() - lastDebounceTime) > DEBOUNCE_DELAY) {
        // Key pressed (LOW = pressed on ESP32)
        if (currentKeyState == LOW && !userKeyPressed) {
            userKeyPressed = true;
            userKeyPressStart = millis();
            userKeyLongPressTriggered = false;
            Serial.println("User key pressed");
        }
        // Key released
        else if (currentKeyState == HIGH && userKeyPressed) {
            unsigned long pressDuration = millis() - userKeyPressStart;

            // Hide countdown first (before any screen changes)
            hide_sleep_countdown();

            if (!userKeyLongPressTriggered) {
                if (pressDuration < LONG_PRESS_DURATION) {
                    // Short press - go back
                    Serial.printf("Short press detected (%lu ms)\n", pressDuration);
                    handle_back_button();
                }
            }

            userKeyPressed = false;
            userKeyLongPressTriggered = false;
        }
        // Check for long press while held
        else if (currentKeyState == LOW && userKeyPressed) {
            unsigned long pressDuration = millis() - userKeyPressStart;

            // Show countdown only after 1 second (to avoid flashing on short presses)
            if (pressDuration >= 1000) {
                int seconds_remaining = (LONG_PRESS_DURATION - pressDuration) / 1000 + 1;
                if (seconds_remaining > 0 && seconds_remaining <= 3) {
                    show_sleep_countdown(seconds_remaining);
                }
            }

            if (pressDuration >= LONG_PRESS_DURATION && !userKeyLongPressTriggered) {
                userKeyLongPressTriggered = true;
                Serial.printf("Long press detected (%lu ms)\n", pressDuration);
                hide_sleep_countdown();
                handle_sleep_request();
            }
        }
    }

    lastKeyState = currentKeyState;
}

void setup(void)
{
    bool pmu_ret = false;
    bool nfc_ret = false;
    bool sghz_ret = false;

    // SGHZ、SD and LCD use the same spi, in order to avoid mutual influence; 
    // before powering on, all CS signals should be pulled high and in an unselected state;
    pinMode(TFT_CS, OUTPUT);
    digitalWrite(TFT_CS, HIGH);
    pinMode(BOARD_SD_CS, OUTPUT);
    digitalWrite(BOARD_SD_CS, HIGH);
    pinMode(BOARD_SGHZ_CS, OUTPUT);
    digitalWrite(BOARD_SGHZ_CS, HIGH);

    // Init system
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, LOW);

    pinMode(BOARD_PWR_EN, OUTPUT);
    digitalWrite(BOARD_PWR_EN, HIGH);  // Power on CC1101 and LED

    pinMode(BOARD_PN532_RF_REST, OUTPUT);
    digitalWrite(BOARD_PN532_RF_REST, HIGH); 

    pinMode(ENCODER_KEY, INPUT);
    pinMode(BOARD_USER_KEY, INPUT);

    pinMode(BOARD_PN532_IRQ, OPEN_DRAIN);

    Serial.begin(115200);
    Serial.print("setup() running core ID: ");
    Serial.println(xPortGetCoreID());

    // Check if we woke from deep sleep
    esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
    if (wakeup_reason == ESP_SLEEP_WAKEUP_EXT1) {
        Serial.println("Woke from deep sleep via BOARD_USER_KEY");
        // Re-enable display backlight
        digitalWrite(TFT_BL, HIGH);
        // The rest of the peripherals will be initialized normally
        // through the standard setup sequence
    } else {
        Serial.println("Normal power-on or reset");
    }

    irsend.begin();
    // irrecv.enableIRIn();  // Don't enable IR at startup - only enable when on IR screen 

    radioLock = xSemaphoreCreateBinary();
    assert(radioLock);
    xSemaphoreGive(radioLock);

    init_microphone();

    // iic scan
    byte error, address;
    int nDevices = 0;
    Serial.println("Scanning for I2C devices ...");
    Wire.begin(BOARD_I2C_SDA, BOARD_I2C_SCL);
    for(address = 0x01; address < 0x7F; address++){
        Wire.beginTransmission(address);
        // 0: success.
        // 1: data too long to fit in transmit buffer.
        // 2: received NACK on transmit of address.
        // 3: received NACK on transmit of data.
        // 4: other error.
        // 5: timeout
        error = Wire.endTransmission();
        if(error == 0){ // 0: success.
            nDevices++;
            if(address == BOARD_I2C_ADDR_1) {
                nfc_ret = true;
                log_i("I2C device found PN532 at address 0x%x\n", address);
            } else if(address == BOARD_I2C_ADDR_2) {
                pmu_ret = true;
                log_i("I2C device found at BQ27220 address 0x%x\n", address);
            } else if(address == BOARD_I2C_ADDR_3) {
                sghz_ret = true;
                log_i("I2C device found at BQ25896 address 0x%x\n", address);
            }
        }
    }
    if (nDevices == 0){
        Serial.println("No I2C devices found");
    }

    if(!SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED)){
        Serial.println("SPIFFS Mount Failed");
        return;
    }

    Serial.println("*************** SPIFFS ****************");
    listDir(SPIFFS, "/", 0);
    Serial.println("**************************************");

    eeprom_init();

    // wifi_init();
    configTime(8 * 3600, 0, ntpServer1, ntpServer2);

    pmu_ret = PPM.init(Wire, BOARD_I2C_SDA, BOARD_I2C_SCL, BQ25896_SLAVE_ADDRESS);
    if(pmu_ret) {
        // PPM.setSysPowerDownVoltage(3300);
        // PPM.setInputCurrentLimit(3250);
        // Serial.printf("getInputCurrentLimit: %d mA\n",PPM.getInputCurrentLimit());
        // PPM.disableCurrentLimitPin();
        PPM.setChargeTargetVoltage(4208);
        // PPM.setPrechargeCurr(64);
        // PPM.setChargerConstantCurr(832);
        // PPM.getChargerConstantCurr();
        // Serial.printf("getChargerConstantCurr: %d mA\n",PPM.getChargerConstantCurr());
        PPM.enableMeasure();
        // PPM.enableCharge();
        // Turn off charging function
        // If USB is used as the only power input, it is best to turn off the charging function,
        // otherwise the VSYS power supply will have a sawtooth wave, affecting the discharge output capability.
        // PPM.disableCharge();


        // The OTG function needs to enable OTG, and set the OTG control pin to HIGH
        // After OTG is enabled, if an external power supply is plugged in, OTG will be turned off

        // PPM.enableOTG();
        // PPM.disableOTG();
        // pinMode(OTG_ENABLE_PIN, OUTPUT);
        // digitalWrite(OTG_ENABLE_PIN, HIGH);
    }

    bq27220.init();
   
    sghz_init(); 

    if(nfc_ret)
        nfc_init();

    ws2812_init();

    // infared_init();

    nrf24_init();

    multi_thread_create();

    audio.setPinout(BOARD_VOICE_BCLK, BOARD_VOICE_LRCLK, BOARD_VOICE_DIN);
    audio.setVolume(21); // 0...21
    audio.connecttoFS(SPIFFS, "/001.mp3");

    // audio.connecttoFS(SD, "/music/My Anata.mp3");

    // init UI and display
    ui_entry(); 
    lv_timer_create(msg_send_event, 5000, NULL);
    // lvgl msg
    lv_msg_subsribe(MSG_UI_ROTATION_ST, msg_subsribe_event, NULL);
    lv_msg_subsribe(MSG_UI_THEME_MODE, msg_subsribe_event, NULL);

    digitalWrite(TFT_BL, HIGH);

    sd_init();

    // Load configuration from nautilus.json on SD card
    Serial.println("\n========== Loading Configuration ==========");
    if (config_load()) {
        Serial.println("[CONFIG] Applying loaded settings...");

        // Apply display settings
        display_rotation = g_config.display.rotation;
        setting_theme = g_config.display.theme;

        // Apply WiFi settings
        strncpy(wifi_ssid, g_config.wifi.ssid, WIFI_SSID_MAX_LEN);
        strncpy(wifi_password, g_config.wifi.password, WIFI_PSWD_MAX_LEN);

        // Apply SubGHz settings
        selected_frequency = g_config.subghz.last_frequency;

        // Apply audio settings
        current_volume = g_config.audio.volume;
        audio.setVolume(current_volume);

        // Apply WS2812 settings
        CRGB color;
        color.red = (g_config.ws2812.color >> 16) & 0xFF;
        color.green = (g_config.ws2812.color >> 8) & 0xFF;
        color.blue = g_config.ws2812.color & 0xFF;
        ws2812_set_color(color);
        ws2812_set_light(g_config.ws2812.brightness);
        // Use ui_scr1_set_mode instead of ws2812_set_mode to properly resume the task
        ui_scr1_set_mode(g_config.ws2812.mode);
        Serial.printf("[CONFIG] Applied WS2812: color=0x%06X, brightness=%d, mode=%d\n",
                      g_config.ws2812.color, g_config.ws2812.brightness, g_config.ws2812.mode);

        Serial.println("[CONFIG] All settings applied successfully");
    } else {
        Serial.println("[CONFIG] Using default settings");
    }
    Serial.println("===========================================\n");

    // Music loading moved to entry8() to avoid blocking boot

    // for(int i = 0; i < WS2812_NUM_LEDS*10; i++) {
    //     ws2812_pos_demo(i);
    //     delay(15);
    // }
    // ws2812_set_color(CRGB::Black);

    
}

int file_cnt = 0;

// Handle volume control with encoder during playback
void handle_volume_control()
{
    extern int indev_encoder_pos;  // From port_indev.cpp

    int encoder_diff = indev_encoder_pos - last_encoder_pos;

    if(encoder_diff != 0) {

        // Adjust volume based on encoder direction
        if(encoder_diff < 0) {
            // Clockwise - increase volume
            if(current_volume < 21) {
                current_volume++;
            }
        } else {
            // Counter-clockwise - decrease volume
            if(current_volume > 0) {
                current_volume--;
            }
        }

        audio.setVolume(current_volume);
        last_encoder_pos = indev_encoder_pos;
        volume_changed_during_playback = true;
    }
}

// Audio library callback - called when song finishes
void audio_eof_mp3(const char *info)
{
    Serial.println("Song finished!");
    delay(5);
    if(music_is_running) {
        extern int music_idx;
        extern std::vector<String> music_list;

        // Auto-advance to next song
        if(music_idx + 1 < music_list.size()) {
            music_idx++;
            Serial.printf("Auto-playing next song: %s\n", music_list[music_idx].c_str());

            char buf[64];
            lv_snprintf(buf, 64, "/music/%s", music_list[music_idx].c_str());
            audio.connecttoFS(SD, buf);

            // Signal that UI needs update (will be handled in main loop)
            extern volatile bool music_label_needs_update;
            music_label_needs_update = true;
        } else {
            // End of playlist - stop playback
            Serial.println("End of playlist");
            music_is_running = false;

            // Signal that UI needs update (will be handled in main loop)
            extern volatile bool music_label_needs_update;
            music_label_needs_update = true;
        }
    }
}

void loop(void)
{
    audio.loop();

    // Process spectrum analyzer updates (must be before lv_timer_handler)
    spectrum_process_update();

    lv_timer_handler();

    // Handle portal DNS/HTTP requests when running
    portal_loop();

    // Handle music label updates from audio callback
    extern volatile bool music_label_needs_update;
    if(music_label_needs_update) {
        music_label_needs_update = false;  // Clear flag first

        extern lv_obj_t *music_lab;
        extern lv_obj_t *pause_btn;
        extern int music_idx;
        extern std::vector<String> music_list;

        if(music_is_running) {
            // Update label for new song
            if(music_lab && music_idx < music_list.size()) {
                Serial.printf("Updating label: '%s'\n", music_list[music_idx].c_str());
                Serial.printf("Label visible: %d, hidden: %d\n",
                    lv_obj_is_visible(music_lab),
                    lv_obj_has_flag(music_lab, LV_OBJ_FLAG_HIDDEN));

                // Check if we're on the music screen
                lv_obj_t *parent = lv_obj_get_parent(music_lab);
                lv_obj_t *active_scr = lv_scr_act();
                Serial.printf("Parent: %p, Active screen: %p, Match: %d\n",
                    parent, active_scr, (parent == active_scr));

                // Update label text
                lv_label_set_text(music_lab, music_list[music_idx].c_str());
                lv_obj_invalidate(music_lab);

                // CRITICAL: Same fix as spectrum analyzer (see spectrum-troubleshooting.txt)
                // The Audio library uses the shared SPI bus to read from SD card during playback.
                // This puts TFT_eSPI in a bad transaction state. Force reset with a dummy pixel draw.
                extern TFT_eSPI tft;
                tft.drawPixel(0, 0, 0);

                // Now force LVGL refresh
                lv_refr_now(NULL);

                Serial.printf("Label updated: '%s'\n", lv_label_get_text(music_lab));
            }
        } else {
            // Playlist ended - restore UI state
            lv_port_indev_enabled(true);

            if(pause_btn) {
                extern const lv_img_dsc_t img_pause_32;
                lv_obj_set_style_bg_img_src(pause_btn, &img_pause_32, 0);
            }
            if(music_lab) {
                lv_label_set_long_mode(music_lab, LV_LABEL_LONG_SCROLL_CIRCULAR);
            }

            // Save volume if changed
            extern bool volume_changed_during_playback;
            extern uint8_t current_volume;
            if(volume_changed_during_playback) {
                g_config.audio.volume = current_volume;
                if(config_sd_available()) {
                    config_save();
                    Serial.printf("[CONFIG] Saved volume: %d\n", current_volume);
                }
                volume_changed_during_playback = false;
            }
        }
    }

    // Monitor back/sleep button (BOARD_USER_KEY)
    monitor_user_key();

    // Handle volume control during playback
    if(music_is_running) {
        handle_volume_control();
    }

    // Handle volume control for file browser audio playback screen
    extern char fb_audio_playing_file[];
    extern int fb_audio_last_encoder_pos;
    if(strlen(fb_audio_playing_file) > 0) {
        extern int indev_encoder_pos;
        int encoder_diff = indev_encoder_pos - fb_audio_last_encoder_pos;

        if(encoder_diff != 0) {
            // Adjust volume based on encoder direction
            if(encoder_diff < 0) {
                // Clockwise - increase volume
                if(current_volume < 21) {
                    current_volume++;
                }
            } else {
                // Counter-clockwise - decrease volume
                if(current_volume > 0) {
                    current_volume--;
                }
            }

            audio.setVolume(current_volume);
            fb_audio_last_encoder_pos = indev_encoder_pos;
            volume_changed_during_playback = true;
        }
    }

    if(music_is_running == false) {
        // IR decode moved to IR screen lifecycle - only active when on IR screen
        // if (irrecv.decode(&results)) {
        //     // print() & println() can't handle printing long longs. (uint64_t)
        //     serialPrintUint64(results.value, HEX);
        //     IR_recv_value = results.value;
        //     Serial.println("");
        //     irrecv.resume();  // Receive the next value
        // }

        i2s_read((i2s_port_t)EXAMPLE_I2S_CH, (char *)i2s_readraw_buff, SAMPLE_SIZE, &bytes_read, 100);
        for(int i = 0; i < 10; i++) {
            // Serial.printf("%d  ", i2s_readraw_buff[i]);
            // if(i == 9) {
            //     Serial.println(" ");
            // }
            if(i2s_readraw_buff[i] > 0) i2s_mic_cnt++;
        }
    }

    delay(1);
}
