
#pragma once
#include "lvgl.h"
#include "lvgl_port/port_disp.h"
#include "lvgl_port/port_indev.h"
#include "lvgl_port/port_scr_mrg.h"
#include "assets/assets.h"
#include "peripheral/peripheral.h"

// The default is landscape screen, HEIGHT and WIDTH swap
#define DISPALY_WIDTH  TFT_HEIGHT
#define DISPALY_HEIGHT TFT_WIDTH


#define FONT_BOLD_14  &Font_Mono_Bold_14
#define FONT_BOLD_16  &Font_Mono_Bold_16
#define FONT_BOLD_18  &Font_Mono_Bold_18
#define FONT_BOLD_20  &Font_Mono_Bold_20
#define FONT_LIGHT_14 &Font_Mono_Light_14
// #define FONT_LIGHT_16 
// #define FONT_LIGHT_18 

// screen id
enum{
    SCREEN0_ID = 0,
    SCREEN1_ID,
    SCREEN2_ID,
    SCREEN2_1_ID,     // SubGHz Record Raw screen
    SCREEN2_1_1_ID,   // SubGHz Frequency Selector screen
    SCREEN2_2_ID,     // SubGHz Playback screen
    SCREEN2_2_1_ID,   // SubGHz Signal Details screen
    SCREEN2_3_ID,     // SubGHz Scan/Record screen
    SCREEN2_3_1_ID,   //   -Scan/Record Frequency Mode Selector (Single/Range/Custom)
    SCREEN2_3_1_1_ID, //     -Single Frequency Selector
    SCREEN2_3_1_2_ID, //     -Range Selector
    SCREEN2_3_1_3_ID, //     -Custom Frequency Selector
    SCREEN2_3_1_4_ID, //     -Step Size Selector
    SCREEN2_4_ID,     // SubGHz Spectrum Analyzer screen
    SCREEN2_4_1_ID,   //   -Spectrum Range Selector
    SCREEN3_ID,       // NFC File Browser screen
    SCREEN3_1_ID,     // NFC Read Tag screen
    SCREEN3_2_ID,     // NFC Tag Detail/Action screen
    SCREEN3_2_1_ID,   //   -NDEF Record Detail screen
    SCREEN4_ID,
    SCREEN4_1_ID,
    SCREEN5_ID,
    SCREEN6_ID,
    SCREEN6_1_ID,     // WiFi Scanner screen
    SCREEN6_1_1_ID,   //   -WiFi AP Details screen
    SCREEN6_2_ID,     // WiFi Deauth Hunter screen
    SCREEN6_3_ID,     // WiFi PineAP Hunter screen
    SCREEN7_ID,
    SCREEN7_1_ID,     // TV-B-Gone screen
    SCREEN7_2_ID,     // IR Capture screen
    SCREEN7_3_ID,     // IR Playback screen
    SCREEN7_4_ID,     // IR Button Capture/Edit Dialog
    SCREEN7_4_1_ID,   //   -Remote buttons screen
    SCREEN7_4_2_ID,   //   -Edit Remote screen
    SCREEN7_4_3_ID,   //     -Add/Edit Button (text input)
    SCREEN7_4_4_ID,   //     -IR File Picker
    SCREEN10_ID,
    SCREEN10_1_ID,
    SCREEN10_2_ID,    // File Browser Audio Playback screen
    SCREEN10_3_ID,    // File Browser Text Viewer screen
    SCREEN11_ID,
    SCREEN8_ID,
    SCREEN9_ID,
    SCREEN2_5_ID,     // SubGHz Remotes screen
    SCREEN2_5_1_ID,   //   -SubGHz File Browser (for selecting .sub files)
    SCREEN2_6_ID,     // SubGHz Custom Frequencies screen
    SCREEN2_6_1_ID,   //   -Add Frequency (keyboard input)
    SCREEN12_ID,      // Portal screen
    SCREEN12_1_ID,    //   -Portal Data Viewer screen
    SCREEN_ID_MAX,
};

// msg id
enum{
    // clock msg subsribe
    MSG_CLOCK_HOUR,
    MSG_CLOCK_MINUTE,
    MSG_CLOCK_SECOND,

    // ui rotation send
    MSG_UI_ROTATION_ST, // uint8_t
    // ui theme send
    MSG_UI_THEME_MODE,  // uint8_t
};

void ui_entry(void);
void spectrum_process_update(void);  // Process spectrum analyzer updates from main loop