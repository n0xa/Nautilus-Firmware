
#include "ui.h"
#include <Arduino.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <IRrecv.h>
#include <IRutils.h>
#include "ir.h"
#include "nfc.h"
#include "subghz_remote.h"
#include "peripheral/peripheral.h"
#include "peripheral/peri_config.h"
#include "peripheral/rf_utils.h"
#include "peripheral/subghz/protocols/protocol_secplus_v1.h"
#include "peripheral/subghz/protocols/protocol_secplus_v2.h"
#include "utilities.h"
#include <vector>
#include <map>
#include <dirent.h>
#include <sys/stat.h>
#include <ELECHOUSE_CC1101_SRC_DRV.h>
#include "driver/i2s.h"

#define UI_THEME_DARK  0
#define UI_THEME_LIGHT 1

// Theme color structure
struct ColorTheme {
    uint32_t bg;
    uint32_t focus;
    uint32_t text;
    uint32_t border;
    uint32_t prompt_bg;
    uint32_t prompt_txt;
};

// Theme definitions
const ColorTheme THEMES[] = {
    // Dark theme
    { 0x161823, 0xB4DC38, 0xffffff, 0xBBBBBB, 0xfffee6, 0x1e1e00 },
    // Light theme
    { 0xffffff, 0x49a6fd, 0x161823, 0xBBBBBB, 0x1e1e00, 0xfffee6 }
};

uint8_t display_rotation        = 3;
uint8_t setting_theme           = UI_THEME_DARK;
uint32_t EMBED_COLOR_BG         = 0x161823;
uint32_t EMBED_COLOR_FOCUS_ON   = 0x91B821;
uint32_t EMBED_COLOR_TEXT       = 0xffffff;
uint32_t EMBED_COLOR_BORDER     = 0xBBBBBB;
uint32_t EMBED_COLOR_PROMPT_BG  = 0xfffee6;
uint32_t EMBED_COLOR_PROMPT_TXT = 0x1e1e00;

#define BAT_INFO_LIEN_CHAR 30
#define BAT_INFO_LIEN_NUM  7
lv_obj_t *batt_line[BAT_INFO_LIEN_NUM];
lv_obj_t *sghz_line[BAT_INFO_LIEN_NUM];

lv_timer_t *taskbar_timer = NULL;

#define GLOBAL_BUF_LEN 48
static char global_buf[GLOBAL_BUF_LEN];
//************************************[ Other fun ]******************************************
#if 1
bool prompt_is_busy = false;
lv_timer_t *prompt_time;
lv_obj_t *prompt_label;

const char * ui_get_battert_level()
{
    int percent = bq27220.getStateOfCharge();
    char * str = NULL;
     if(percent < 20)
        str =  LV_SYMBOL_BATTERY_EMPTY;
    else if(percent < 40)
        str =  LV_SYMBOL_BATTERY_1;
    else if(percent < 65)
        str =  LV_SYMBOL_BATTERY_2;
    else if(percent < 90)
        str =  LV_SYMBOL_BATTERY_3;
    else
        str =  LV_SYMBOL_BATTERY_FULL;
    return str;
}

void prompt_label_timer(lv_timer_t *t)
{
    lv_obj_del((lv_obj_t *)t->user_data);
    lv_timer_del(t);
    prompt_is_busy = false;
}

void prompt_create(const char *str, uint16_t time)
{
    prompt_label = lv_label_create(lv_layer_top());
    lv_obj_set_width(prompt_label, DISPALY_WIDTH * 0.8);
    lv_label_set_text(prompt_label, str);
    lv_label_set_long_mode(prompt_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_radius(prompt_label, 5, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(prompt_label, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_pad_hor(prompt_label, 3, LV_PART_MAIN);
    lv_obj_set_style_text_font(prompt_label, FONT_BOLD_14, LV_PART_MAIN);
    lv_obj_set_style_bg_color(prompt_label, lv_color_hex(EMBED_COLOR_PROMPT_BG), LV_PART_MAIN);
    lv_obj_set_style_text_color(prompt_label, lv_color_hex(EMBED_COLOR_PROMPT_TXT), LV_PART_MAIN);
    lv_obj_set_style_text_align(prompt_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_center(prompt_label);
    prompt_time = lv_timer_create(prompt_label_timer, time, prompt_label);
    prompt_is_busy = true;
}

void prompt_info(const char *str, uint16_t time)
{
    if(prompt_is_busy == false){
        prompt_create(str, time);
    } else {
        lv_obj_del(prompt_label);
        lv_timer_del(prompt_time);
        prompt_is_busy = false;
        prompt_create(str, time);
    }
}

// LVGL styling helper functions to reduce code duplication
inline void apply_bg_color(lv_obj_t* obj) {
    lv_obj_set_style_bg_color(obj, lv_color_hex(EMBED_COLOR_BG), LV_PART_MAIN);
}

inline void apply_text_color(lv_obj_t* obj) {
    lv_obj_set_style_text_color(obj, lv_color_hex(EMBED_COLOR_TEXT), LV_PART_MAIN);
}

inline void apply_no_border(lv_obj_t* obj) {
    lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN);
}

inline void apply_border(lv_obj_t* obj, int width) {
    lv_obj_set_style_border_width(obj, width, LV_PART_MAIN);
}

inline void apply_no_padding(lv_obj_t* obj) {
    lv_obj_set_style_pad_all(obj, 0, LV_PART_MAIN);
}

inline void apply_no_radius(lv_obj_t* obj) {
    lv_obj_set_style_radius(obj, 0, LV_PART_MAIN);
}

inline void apply_no_scrollbar(lv_obj_t* obj) {
    lv_obj_set_scrollbar_mode(obj, LV_SCROLLBAR_MODE_OFF);
}

inline void apply_focus_outline(lv_obj_t* obj) {
    lv_obj_remove_style(obj, NULL, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_width(obj, 2, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_color(obj, lv_color_hex(EMBED_COLOR_FOCUS_ON), LV_STATE_FOCUS_KEY);
}

inline void apply_no_shadow(lv_obj_t* obj) {
    lv_obj_set_style_shadow_width(obj, 0, LV_PART_MAIN);
}

// Composite helper for common container styling
inline void apply_container_defaults(lv_obj_t* obj) {
    apply_bg_color(obj);
    apply_no_border(obj);
    apply_no_padding(obj);
    apply_no_radius(obj);
}

// Helper for battery percentage display
inline void update_battery_percent_label(lv_obj_t* label) {
    lv_label_set_text_fmt(label, "#%x %d #", EMBED_COLOR_TEXT, bq27220.getStateOfCharge());
}

// Helper for menu button event callbacks (CLICKED + FOCUSED)
inline void add_menu_button_events(lv_obj_t* btn, lv_event_cb_t cb, void* user_data) {
    lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, user_data);
    lv_obj_add_event_cb(btn, cb, LV_EVENT_FOCUSED, user_data);
}

// Helper for creating full-screen containers with standard styling
inline lv_obj_t* create_screen_container(lv_obj_t* parent) {
    lv_obj_t* cont = lv_obj_create(parent);
    lv_obj_set_size(cont, lv_pct(100), lv_pct(100));
    apply_bg_color(cont);
    apply_no_scrollbar(cont);
    apply_no_border(cont);
    apply_no_padding(cont);
    return cont;
}

lv_obj_t* scr_back_btn_create(lv_obj_t *parent, lv_event_cb_t cb)
{
    lv_obj_t * btn = lv_btn_create(parent);
    lv_group_add_obj(lv_group_get_default(), btn);
    lv_obj_set_style_pad_all(btn, 0, 0);
    lv_obj_set_height(btn, 20);
    lv_obj_align(btn, LV_ALIGN_TOP_LEFT, 8, 8);
    apply_no_border(btn);
    apply_no_shadow(btn);
    apply_bg_color(btn);
    apply_focus_outline(btn);
    lv_obj_set_style_outline_pad(btn, 0, LV_STATE_FOCUS_KEY);
    lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *label2 = lv_label_create(btn);
    lv_obj_align(label2, LV_ALIGN_LEFT_MID, 0, 0);
    apply_text_color(label2);
    lv_label_set_text(label2, " " LV_SYMBOL_LEFT " ");

    return btn;
}

void ui_theme_setting(int ui_theme)
{
    extern uint32_t default_bg_color;

    // Validate theme index and apply theme colors
    if (ui_theme >= 0 && ui_theme < sizeof(THEMES)/sizeof(THEMES[0])) {
        const ColorTheme& theme = THEMES[ui_theme];
        EMBED_COLOR_BG         = theme.bg;
        EMBED_COLOR_FOCUS_ON   = theme.focus;
        EMBED_COLOR_TEXT       = theme.text;
        EMBED_COLOR_BORDER     = theme.border;
        EMBED_COLOR_PROMPT_BG  = theme.prompt_bg;
        EMBED_COLOR_PROMPT_TXT = theme.prompt_txt;
    }

    default_bg_color = EMBED_COLOR_BG;
}

lv_obj_t * scr5_add_info_lab(lv_obj_t *parent, lv_obj_t *label, const char *s)
{
    label = lv_label_create(parent);
    lv_obj_set_width(label, DISPALY_WIDTH);
    apply_bg_color(label);
    lv_obj_set_style_bg_color(label, lv_color_hex(EMBED_COLOR_FOCUS_ON), LV_STATE_FOCUS_KEY);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    apply_text_color(label);
    lv_obj_set_style_bg_opa(label, LV_OPA_100, LV_PART_MAIN);
    lv_obj_set_style_text_font(label, FONT_BOLD_14, LV_PART_MAIN);
    lv_label_set_text(label, s);
    // lv_group_add_obj(lv_group_get_default(), label);
    lv_obj_center(label);
    return label;
}

static const char *line_full_format(int max_c, const char *str1, const char *str2)
{
    int len1 = 0, len2 = 0;
    int j;

    len1 = strlen(str1);

    strncpy(global_buf, str1, len1);

    len2 = strlen(str2);
    for(j = len1; j < max_c -1 - len2; j++){
        global_buf[j] = ' ';
    }
    strncpy(global_buf + j, str2, len2);
    j = j + len2;
    
    global_buf[j] = '\0'; 

    return (const char *)global_buf;
}

void ui_setting_get_sd_capacity(uint64_t *total, uint64_t *used)
{
    if(sd_init_flag)
    {
        if(total)
            *total = SD.totalBytes() / (1024 * 1024);
        if(used)
            *used = SD.usedBytes() / (1024 * 1024);

        printf("total=%lluMB, used=%lluMB\n", *total, *used);

        uint64_t cardSize = SD.cardSize() / (1024 * 1024);

        uint64_t totalSize = SD.totalBytes() / (1024 * 1024);

        uint64_t usedSize = SD.usedBytes() / (1024 * 1024);
    }
}
#endif
//************************************[ screen 0 ]****************************************** menu
#if 1
// 
#define MENU_PROPORTION     (0.55)
#define MENU_LAB_PROPORTION (1 - MENU_PROPORTION)


static lv_obj_t *menu_taskbar = NULL;
static lv_obj_t *menu_taskbar_charge = NULL;
static lv_obj_t *menu_taskbar_battery = NULL;
static lv_obj_t *menu_taskbar_battery_percent = NULL;
static lv_obj_t *menu_taskbar_wifi = NULL;
static lv_obj_t *menu_taskbar_sd = NULL;

lv_obj_t *item_cont;
lv_obj_t *menu_cont;
lv_obj_t *menu_icon;
lv_obj_t *menu_icon_lab;
lv_obj_t *menu_time_lab;

int fouce_item = 0;
int hour = 0;
int minute = 0;
int second = 0;

// Menu item structure for dynamic menu building
struct MenuItem {
    const char* name;
    int screen_id;
    const lv_img_dsc_t* icon;
    uint32_t icon_color;
    bool has_recolor;
    bool (*is_available)(void);  // Function pointer to check if item should be shown
};

// Always available items
static bool always_available(void) { return true; }

// Menu items with their configurations
static const MenuItem menu_items_config[] = {
    {"<- Sub-G",       SCREEN2_ID,  &img_sghz_32,    0x00BCD4, true,  always_available},  // Cyan
    {"<- Wifi",        SCREEN6_ID,  &img_wifi_32,    0, false, always_available},
    {"<- Infrared",    SCREEN7_ID,  &img_dev_32,     0, false, always_available},
    {"<- Microphone",  SCREEN11_ID, &img_dev_32,     0, false, always_available},
    {"<- Music",       SCREEN8_ID,  &img_music_32,   0, false, always_available},
    {"<- WS2812",      SCREEN1_ID,  &img_light_32,   0, false, always_available},
    {"<- NRF24",       SCREEN9_ID,  &img_setting_32, 0, false, nrf24_is_init},  // Only show if NRF24 is detected
    {"<- NFC (beta)",  SCREEN3_ID,  &img_nfc_32,     0x00BCD4, true,  always_available},  // Cyan
    {"<- Battery",     SCREEN5_ID,  &img_battery_32, 0, false, always_available},
    {"<- SD Card",     SCREEN10_ID, &img_dev_32,     0, false, always_available},
    {"<- Setting",     SCREEN4_ID,  &img_setting_32, 0, false, always_available},
};

// Dynamic arrays built at runtime based on available hardware
static std::vector<MenuItem> active_menu_items;
static std::vector<int> menu_index_to_screen_id;

void entry0_anim(void);
void switch_scr0_anim(int user_data);

// Build the active menu items list based on available hardware
static void build_active_menu() {
    active_menu_items.clear();
    menu_index_to_screen_id.clear();

    int num_items = sizeof(menu_items_config) / sizeof(menu_items_config[0]);
    for (int i = 0; i < num_items; i++) {
        // Check if this menu item should be available
        if (menu_items_config[i].is_available()) {
            active_menu_items.push_back(menu_items_config[i]);
            menu_index_to_screen_id.push_back(menu_items_config[i].screen_id);
        }
    }

}

// event
static void clock_upd_event(lv_event_t *e)
{
    lv_obj_t * label = lv_event_get_target(e);
    lv_msg_t * m = lv_event_get_msg(e);
    int msg_id = (int)lv_msg_get_user_data(m);
    const int32_t *v = (int32_t *)lv_msg_get_payload(m);

    switch (msg_id) {
        case MSG_CLOCK_HOUR:   hour = *v;    break;
        case MSG_CLOCK_MINUTE: minute = *v;  break;
        case MSG_CLOCK_SECOND: second = *v;  break;
        default: break;
    }
    lv_label_set_text_fmt(menu_time_lab, "#EE781F %02d# #C0C0C0 :# #AFCA31 %02d#", hour, minute);
}

static void scr0_btn_event_cb(lv_event_t *e)
{
    int menu_item_index = (int)e->user_data;

    if(e->code == LV_EVENT_CLICKED){
        fouce_item = menu_item_index;
        // Use dynamic mapping to get the correct screen ID
        if (menu_item_index >= 0 && menu_item_index < menu_index_to_screen_id.size()) {
            switch_scr0_anim(menu_index_to_screen_id[menu_item_index]);
        }
    }

    if(e->code == LV_EVENT_FOCUSED){
        // Update icon based on the active menu item
        if (menu_item_index >= 0 && menu_item_index < active_menu_items.size()) {
            const MenuItem& item = active_menu_items[menu_item_index];
            lv_img_set_src(menu_icon, item.icon);
            if (item.has_recolor) {
                lv_obj_set_style_img_recolor(menu_icon, lv_color_hex(item.icon_color), LV_PART_MAIN);
                lv_obj_set_style_img_recolor_opa(menu_icon, LV_OPA_100, LV_PART_MAIN);
            } else {
                lv_obj_set_style_img_recolor_opa(menu_icon, LV_OPA_0, LV_PART_MAIN);
            }
            lv_label_set_text(menu_icon_lab, item.name + 3);  // Skip the "<- " prefix
            lv_obj_align_to(menu_icon_lab, menu_cont, LV_ALIGN_BOTTOM_MID, 0, -25);
        }
    }
}

void entry0_anim(void)
{
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, item_cont);
    lv_anim_set_values(&a, DISPALY_WIDTH * MENU_PROPORTION, 0);
    lv_anim_set_time(&a, 400);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_in_out);
    lv_anim_set_exec_cb(&a, [](void * var, int32_t v){
        lv_obj_set_x((lv_obj_t *)var, v);
        lv_port_indev_enabled(false);
    });
    lv_anim_set_ready_cb(&a, [](struct _lv_anim_t *a){
        lv_port_indev_enabled(true);
    });
    lv_anim_start(&a);

    lv_anim_set_time(&a, 400);
    lv_anim_set_var(&a, menu_cont);
    lv_anim_set_values(&a, -DISPALY_WIDTH * MENU_LAB_PROPORTION, 0);
    lv_anim_set_exec_cb(&a, [](void * var, int32_t v){
        lv_obj_set_x((lv_obj_t *)var, v);
        lv_port_indev_enabled(false);
    });
    lv_anim_start(&a);
}

void switch_scr0_anim(int user_data)
{
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, item_cont);
    lv_anim_set_values(&a, 0, DISPALY_WIDTH * MENU_PROPORTION);
    lv_anim_set_time(&a, 400);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_in_out);
    lv_anim_set_exec_cb(&a, [](void * var, int32_t v){
        lv_obj_set_x((lv_obj_t *)var, v);
        lv_port_indev_enabled(false);
    });
    lv_anim_set_ready_cb(&a, [](struct _lv_anim_t *a){
        scr_mgr_switch((int)a->user_data, false);
        lv_port_indev_enabled(true);
    });
    lv_anim_set_user_data(&a, (void *)user_data);
    lv_anim_start(&a);

    lv_anim_set_time(&a, 400);
    lv_anim_set_var(&a, menu_cont);
    lv_anim_set_values(&a, 0, -DISPALY_WIDTH * MENU_LAB_PROPORTION);
    lv_anim_set_exec_cb(&a, [](void * var, int32_t v){
        lv_obj_set_x((lv_obj_t *)var, v);
        lv_port_indev_enabled(false);
    });
    lv_anim_start(&a);
}

static void scroll_event_cb(lv_event_t *e)
{
    lv_obj_t * cont = lv_event_get_target(e);

    lv_area_t cont_a;
    lv_obj_get_coords(cont, &cont_a);
    lv_coord_t cont_y_center = cont_a.y1 + lv_area_get_height(&cont_a) / 2;

    lv_coord_t r = lv_obj_get_height(cont) * 7 / 10;
    uint32_t i;
    uint32_t child_cnt = lv_obj_get_child_cnt(cont);
    for(i = 0; i < child_cnt; i++) {
        lv_obj_t * child = lv_obj_get_child(cont, i);
        lv_area_t child_a;
        lv_obj_get_coords(child, &child_a);

        lv_coord_t child_y_center = child_a.y1 + lv_area_get_height(&child_a) / 2;

        lv_coord_t diff_y = child_y_center - cont_y_center;
        diff_y = LV_ABS(diff_y);

        /*Get the x of diff_y on a circle.*/
        lv_coord_t x;
        /*If diff_y is out of the circle use the last point of the circle (the radius)*/
        if(diff_y >= r) {
            x = r;
        }
        else {
            x =  (diff_y * 0.866);
        }

        /*Translate the item by the calculated X coordinate*/
        lv_obj_set_style_translate_x(child, x, 0);

        /*Use some opacity with larger translations*/
        lv_opa_t opa = lv_map(x, 0, r, LV_OPA_TRANSP, LV_OPA_COVER);
        lv_obj_set_style_opa(child, LV_OPA_COVER - opa, 0);
    }
}

// Helper function to create and style taskbar container
lv_obj_t* create_taskbar_container(lv_obj_t *parent) {
    int status_bar_height = 25;

    lv_obj_t *taskbar = lv_obj_create(parent);
    lv_obj_set_size(taskbar, LV_HOR_RES, status_bar_height);
    apply_no_padding(taskbar);
    apply_no_border(taskbar);
    lv_obj_set_flex_flow(taskbar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(taskbar, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_left(taskbar, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(taskbar, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(taskbar, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(taskbar, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_row(taskbar, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_column(taskbar, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
    apply_no_scrollbar(taskbar);
    lv_obj_clear_flag(taskbar, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_opa(taskbar, 0, LV_PART_MAIN);
    lv_obj_align(taskbar, LV_ALIGN_TOP_MID, 0, 0);

    return taskbar;
}

void create0(lv_obj_t *parent)
{
    menu_cont = lv_obj_create(parent);
    lv_obj_set_size(menu_cont, DISPALY_WIDTH * MENU_LAB_PROPORTION, DISPALY_HEIGHT);
    apply_no_scrollbar(menu_cont);
    lv_obj_set_align(menu_cont, LV_ALIGN_LEFT_MID);
    apply_container_defaults(menu_cont);

    menu_icon = lv_img_create(menu_cont);
    // lv_img_set_src(menu_icon, &Battery_Charging_48);
    lv_obj_align(menu_icon, LV_ALIGN_CENTER, 0, 0);

    menu_icon_lab = lv_label_create(menu_cont);
    lv_obj_set_width(menu_icon_lab, DISPALY_WIDTH * MENU_LAB_PROPORTION);
    lv_obj_set_style_text_align(menu_icon_lab, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    apply_text_color(menu_icon_lab);
    lv_obj_set_style_text_font(menu_icon_lab, FONT_BOLD_18, LV_PART_MAIN);
    // lv_label_set_text(menu_icon_lab, "battery");
    lv_obj_align_to(menu_icon_lab, menu_cont, LV_ALIGN_BOTTOM_MID, 0, -25);
    
    menu_time_lab = lv_label_create(menu_cont);
    lv_obj_set_width(menu_time_lab, DISPALY_WIDTH * MENU_LAB_PROPORTION);
    lv_obj_align(menu_time_lab, LV_ALIGN_TOP_MID, 5, 25);
    lv_obj_set_style_text_align(menu_time_lab, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_text_font(menu_time_lab, FONT_BOLD_18, LV_PART_MAIN);
    // lv_obj_set_style_text_font(menu_time_lab, &lv_font_montserrat_26, LV_PART_MAIN);
    lv_label_set_recolor(menu_time_lab, true);                      /*Enable re-coloring by commands in the text*/
    lv_label_set_text_fmt(menu_time_lab, "#EE781F %02d# #C0C0C0 :# #AFCA31 %02d#", hour, minute);

    ///////////////////////////////////////////////////////
    item_cont = lv_obj_create(parent);
    lv_obj_set_size(item_cont, DISPALY_WIDTH * MENU_PROPORTION, DISPALY_HEIGHT);
    lv_obj_set_align(item_cont, LV_ALIGN_RIGHT_MID);
    lv_obj_set_flex_flow(item_cont, LV_FLEX_FLOW_COLUMN);
    apply_bg_color(item_cont);
    apply_no_border(item_cont);
    lv_obj_add_event_cb(item_cont, scroll_event_cb, LV_EVENT_SCROLL, NULL);
    apply_no_radius(item_cont);
    lv_obj_set_style_clip_corner(item_cont, true, 0);
    lv_obj_set_scroll_dir(item_cont, LV_DIR_VER);
    lv_obj_set_scroll_snap_y(item_cont, LV_SCROLL_SNAP_CENTER);
    apply_no_scrollbar(item_cont);
    lv_obj_set_style_pad_row(item_cont, 10, LV_PART_MAIN);

    // Build the active menu based on available hardware
    build_active_menu();

    // Create buttons for active menu items only
    for(uint32_t i = 0; i < active_menu_items.size(); i++) {
        lv_obj_t * btn = lv_btn_create(item_cont);
        lv_obj_set_width(btn, lv_pct(100));
        lv_obj_set_style_radius(btn, 10, LV_PART_MAIN);
        apply_bg_color(btn);
        apply_no_border(btn);
        apply_no_shadow(btn);
        apply_focus_outline(btn);
        lv_obj_set_style_bg_img_opa(btn, LV_OPA_100, LV_PART_MAIN);
        add_menu_button_events(btn, scr0_btn_event_cb, (void *)i);

        lv_obj_t * label = lv_label_create(btn);
        apply_text_color(label);
        lv_obj_set_style_text_font(label, FONT_BOLD_14, LV_PART_MAIN);
        lv_label_set_text(label, active_menu_items[i].name);
    }

    /*Update the buttons position manually for first*/
    lv_event_send(item_cont, LV_EVENT_SCROLL, NULL);

    /*Be sure the fist button is in the middle*/
    lv_obj_scroll_to_view(lv_obj_get_child(item_cont, 0), LV_ANIM_OFF);

    lv_group_focus_obj(lv_obj_get_child(item_cont, fouce_item));

    lv_group_set_wrap(lv_group_get_default(), false);

    // tackbar
    menu_taskbar = create_taskbar_container(parent);

    menu_taskbar_wifi = lv_label_create(menu_taskbar);
    lv_label_set_recolor(menu_taskbar_wifi, true);
    lv_label_set_text_fmt(menu_taskbar_wifi, "#%x %s #", EMBED_COLOR_TEXT, LV_SYMBOL_WIFI);
    lv_obj_add_flag(menu_taskbar_wifi, LV_OBJ_FLAG_HIDDEN);

    menu_taskbar_sd = lv_label_create(menu_taskbar);
    lv_label_set_recolor(menu_taskbar_sd, true);
    lv_label_set_text_fmt(menu_taskbar_sd, "#%x %s #", EMBED_COLOR_TEXT, LV_SYMBOL_SD_CARD);
    lv_obj_add_flag(menu_taskbar_sd, LV_OBJ_FLAG_HIDDEN);

    menu_taskbar_charge = lv_label_create(menu_taskbar);
    lv_obj_align(menu_taskbar_charge, LV_ALIGN_TOP_RIGHT, -10, 10);
    lv_obj_set_style_text_align(menu_taskbar_charge, LV_TEXT_ALIGN_RIGHT, 0);
    lv_label_set_recolor(menu_taskbar_charge, true);
    lv_label_set_text_fmt(menu_taskbar_charge, "#%x %s #", EMBED_COLOR_TEXT, LV_SYMBOL_CHARGE);
    lv_obj_add_flag(menu_taskbar_charge, LV_OBJ_FLAG_HIDDEN);

    menu_taskbar_battery = lv_label_create(menu_taskbar);
    lv_label_set_recolor(menu_taskbar_battery, true);
    lv_label_set_text_fmt(menu_taskbar_battery, "#%x %s #", EMBED_COLOR_TEXT, LV_SYMBOL_BATTERY_FULL);
    // lv_obj_add_flag(menu_taskbar_battery, LV_OBJ_FLAG_HIDDEN);

    menu_taskbar_battery_percent = lv_label_create(menu_taskbar);
    lv_label_set_recolor(menu_taskbar_battery_percent, true);
    lv_obj_set_style_text_font(menu_taskbar_battery_percent, FONT_BOLD_14, LV_PART_MAIN);
    update_battery_percent_label(menu_taskbar_battery_percent);
    // lv_obj_add_flag(menu_taskbar_battery_percent, LV_OBJ_FLAG_HIDDEN);
}

// Helper function to add status bar to any screen (for consistency across menus)
lv_obj_t* create_status_bar(lv_obj_t *parent) {
    lv_obj_t *taskbar = create_taskbar_container(parent);

    // Move to foreground to ensure it's visible above other containers
    lv_obj_move_foreground(taskbar);

    // WiFi icon
    lv_obj_t *wifi_icon = lv_label_create(taskbar);
    lv_label_set_recolor(wifi_icon, true);
    lv_label_set_text_fmt(wifi_icon, "#%x %s #", EMBED_COLOR_TEXT, LV_SYMBOL_WIFI);
    if(!wifi_is_connect) {
        lv_obj_add_flag(wifi_icon, LV_OBJ_FLAG_HIDDEN);
    }

    // SD card icon
    lv_obj_t *sd_icon = lv_label_create(taskbar);
    lv_label_set_recolor(sd_icon, true);
    lv_label_set_text_fmt(sd_icon, "#%x %s #", EMBED_COLOR_TEXT, LV_SYMBOL_SD_CARD);
    if(!sd_init_flag) {
        lv_obj_add_flag(sd_icon, LV_OBJ_FLAG_HIDDEN);
    }

    // Charge icon
    lv_obj_t *charge_icon = lv_label_create(taskbar);
    lv_obj_set_style_text_align(charge_icon, LV_TEXT_ALIGN_RIGHT, 0);
    lv_label_set_recolor(charge_icon, true);
    if(bq27220.getCharingFinish()) {
        lv_label_set_text_fmt(charge_icon, "#%x %s #", EMBED_COLOR_TEXT, LV_SYMBOL_OK);
    } else {
        lv_label_set_text_fmt(charge_icon, "#%x %s #", EMBED_COLOR_TEXT, LV_SYMBOL_CHARGE);
    }
    if(!PPM.isCharging()) {
        lv_obj_add_flag(charge_icon, LV_OBJ_FLAG_HIDDEN);
    }

    // Battery icon
    lv_obj_t *battery_icon = lv_label_create(taskbar);
    lv_label_set_recolor(battery_icon, true);
    lv_label_set_text_fmt(battery_icon, "#%x %s #", EMBED_COLOR_TEXT, ui_get_battert_level());

    // Battery percent
    lv_obj_t *battery_percent = lv_label_create(taskbar);
    lv_label_set_recolor(battery_percent, true);
    lv_obj_set_style_text_font(battery_percent, FONT_BOLD_14, LV_PART_MAIN);
    update_battery_percent_label(battery_percent);

    return taskbar;
}

void entry0(void) {
    entry0_anim();
    lv_obj_add_event_cb(menu_time_lab, clock_upd_event, LV_EVENT_MSG_RECEIVED, NULL);
    lv_msg_subsribe_obj(MSG_CLOCK_HOUR, menu_time_lab, (void *)MSG_CLOCK_HOUR);
    lv_msg_subsribe_obj(MSG_CLOCK_MINUTE, menu_time_lab, (void *)MSG_CLOCK_MINUTE);
    lv_msg_subsribe_obj(MSG_CLOCK_SECOND, menu_time_lab, (void *)MSG_CLOCK_SECOND);

    lv_timer_resume(taskbar_timer);
    lv_group_set_wrap(lv_group_get_default(), true);
}
void exit0(void) {
    lv_group_set_wrap(lv_group_get_default(), false);
    lv_obj_remove_event_cb(menu_time_lab, clock_upd_event);
    lv_msg_unsubscribe_obj(MSG_CLOCK_HOUR, menu_time_lab);
    lv_msg_unsubscribe_obj(MSG_CLOCK_MINUTE, menu_time_lab);
    lv_msg_unsubscribe_obj(MSG_CLOCK_SECOND, menu_time_lab);

    lv_timer_pause(taskbar_timer);
}
void destroy0(void) {
    // if(menu_taskbar_charge){
    //     lv_obj_del(menu_taskbar_charge);
    //     menu_taskbar_charge = NULL;
    // }
}

scr_lifecycle_t screen0 = {
    .create = create0,
    .entry = entry0,
    .exit  = exit0,
    .destroy = destroy0,
};
#endif
//************************************[ screen 1 ]****************************************** ws2812
#if 1
/*** UI interface ***/
int __attribute__((weak)) ui_scr1_get_led_mode(void) { return 0; }
void __attribute__((weak)) ui_scr1_set_color(lv_color32_t c32) {}
void __attribute__((weak)) ui_scr1_set_light(uint8_t light) {}
void __attribute__((weak)) ui_scr1_set_mode(int mode) {}
// end

lv_obj_t *scr1_cont;
lv_obj_t *light_arc;
int ws2812_light = WS2812_DEFAULT_LIGHT;
lv_color_t ws2812_color ={.full = 0xF800};
static int ui_light_mode = 0;

void entry1_anim(lv_obj_t *obj);
void exit1_anim(int user_data, lv_obj_t *obj);

void colorwheel_focus_event(lv_event_t *e)
{
    lv_obj_t *tgt = lv_event_get_target(e);
    lv_obj_t *label = (lv_obj_t *)lv_event_get_user_data(e);
    if(e->code == LV_EVENT_CLICKED){
        if (!lv_obj_has_flag(tgt, LV_OBJ_FLAG_CHECKABLE)) {
            lv_obj_clear_flag(tgt, LV_OBJ_FLAG_CHECKABLE);
            lv_group_set_editing(lv_group_get_default(), false);
        } else {
            lv_obj_add_flag(tgt, LV_OBJ_FLAG_CHECKABLE);
            lv_group_set_editing(lv_group_get_default(), true);
        }
    }
    else if(e->code == LV_EVENT_VALUE_CHANGED){
        ws2812_color = lv_colorwheel_get_rgb(tgt);
        CRGB c;
        lv_color32_t c32;
        c32.full = lv_color_to32(ws2812_color);
        c.red = c32.ch.red;
        c.green = c32.ch.green;
        c.blue = c32.ch.blue;
        // if(ui_scr1_get_led_mode() == 0){
            // ui_scr1_set_color(c32);
            ws2812_set_color(c);
        // }

        // Save color to config
        g_config.ws2812.color = (c.red << 16) | (c.green << 8) | c.blue;
        lv_label_set_text_fmt(label, "0x%02X%02X%02X", c.red, c.green, c.blue);
        lv_obj_set_style_bg_color(light_arc, ws2812_color, LV_PART_KNOB);
        lv_obj_set_style_arc_color(light_arc, ws2812_color, LV_PART_INDICATOR);
    }
}

static void value_changed_event_cb(lv_event_t *e)
{
    lv_obj_t * arc = lv_event_get_target(e);
    lv_obj_t * label = (lv_obj_t *)lv_event_get_user_data(e);

    ws2812_light = lv_arc_get_value(arc);
    lv_label_set_text_fmt(label, "%d", ws2812_light);
    ui_scr1_set_light(ws2812_light);

    // Save brightness to config
    g_config.ws2812.brightness = ws2812_light;
    /*Rotate the label to the current position of the arc*/
    // lv_arc_rotate_obj_to_angle(arc, label, 25);
}

static void scr1_btn_event_cb(lv_event_t * e)
{
    if(e->code == LV_EVENT_CLICKED){
        // scr_mgr_set_anim(LV_SCR_LOAD_ANIM_FADE_OUT, -1, -1);
        // scr_mgr_switch(SCREEN0_ID, false);
        exit1_anim(SCREEN0_ID, scr1_cont);
    }
}

static void ws2812_mode_event_cb(lv_event_t * e)
{
    lv_obj_t *tgt = (lv_obj_t *)e->target;
    lv_obj_t *lab = (lv_obj_t *)e->user_data;
    if(e->code == LV_EVENT_CLICKED){
        ui_light_mode++;
        ui_light_mode &= 0x3;
        switch (ui_light_mode)
        {
            case 0: lv_label_set_text(lab, " OFF ");    break;
            case 1: lv_label_set_text(lab, " demo1 "); break;
            case 2: lv_label_set_text(lab, " demo2 "); break;
            case 3: lv_label_set_text(lab, " demo3 "); break;
            default:
                break;
        }
        ui_scr1_set_mode(ui_light_mode);

        // Save mode to config
        g_config.ws2812.mode = ui_light_mode;
		config_save();
    }
}

void entry1_anim(lv_obj_t *obj)
{
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, obj);
    lv_anim_set_values(&a, LV_OPA_0, LV_OPA_100);
    lv_anim_set_time(&a, 200);
    lv_anim_set_path_cb(&a, lv_anim_path_linear);
    lv_anim_set_exec_cb(&a, [](void * var, int32_t v){ 
        lv_port_indev_enabled(false);
        lv_obj_set_style_opa((lv_obj_t *)var, v, LV_PART_MAIN);
    });
    lv_anim_set_ready_cb(&a, [](_lv_anim_t *a){ 
        lv_port_indev_enabled(true);
    });
    lv_anim_start(&a);
}

void exit1_anim(int user_data, lv_obj_t *obj)
{
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, obj);
    lv_anim_set_values(&a, LV_OPA_100, LV_OPA_0);
    lv_anim_set_time(&a, 200);
    lv_anim_set_path_cb(&a, lv_anim_path_linear);
    lv_anim_set_exec_cb(&a, [](void * var, int32_t v){ 
        lv_port_indev_enabled(false);
        lv_obj_set_style_opa((lv_obj_t *)var, v, LV_PART_MAIN);
    });
    lv_anim_set_ready_cb(&a, [](_lv_anim_t *a){
        scr_mgr_switch((int)a->user_data, false);
        lv_port_indev_enabled(true);
    });
    lv_anim_set_user_data(&a, (void *)user_data);
    lv_anim_start(&a);
}

void create1(lv_obj_t *parent)
{
    scr1_cont = create_screen_container(parent);

    lv_obj_t *colorwheel = lv_colorwheel_create(scr1_cont, true);
    lv_obj_set_size(colorwheel, 100, 100);
    lv_obj_set_style_arc_width(colorwheel, 6, LV_PART_MAIN);
    lv_obj_set_style_arc_width(colorwheel, 6, LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(colorwheel, 6, LV_PART_KNOB);
    lv_obj_align(colorwheel, LV_ALIGN_RIGHT_MID, -35, 0);
    lv_colorwheel_set_rgb(colorwheel, ws2812_color);
    lv_obj_set_style_outline_pad(colorwheel, 4, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_radius(colorwheel, LV_RADIUS_CIRCLE, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_width(colorwheel, 2, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_color(colorwheel, lv_color_hex(EMBED_COLOR_FOCUS_ON), LV_STATE_FOCUS_KEY);
    lv_group_add_obj(lv_group_get_default(), colorwheel);

    lv_obj_t *label = lv_label_create(colorwheel);
    lv_obj_center(label);
    lv_label_set_text(label, "color");
    apply_text_color(label);
    lv_obj_add_event_cb(colorwheel, colorwheel_focus_event, LV_EVENT_CLICKED, label);
    lv_obj_add_event_cb(colorwheel, colorwheel_focus_event, LV_EVENT_VALUE_CHANGED, label);
    // lv_event_send(colorwheel, LV_EVENT_VALUE_CHANGED, NULL);

    lv_obj_t * mode_btn = lv_btn_create(scr1_cont);
    // lv_group_add_obj(lv_group_get_default(), mode_btn);
    lv_obj_set_style_pad_all(mode_btn, 0, 0);
    lv_obj_set_height(mode_btn, 30);
    lv_obj_align(mode_btn, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_set_style_border_color(mode_btn, lv_color_hex(EMBED_COLOR_BORDER), LV_PART_MAIN);
    lv_obj_set_style_border_width(mode_btn, 1, LV_PART_MAIN);
    apply_no_shadow(mode_btn);
    apply_bg_color(mode_btn);
    lv_obj_remove_style(mode_btn, NULL, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_pad(mode_btn, 2, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_width(mode_btn, 2, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_color(mode_btn, lv_color_hex(EMBED_COLOR_FOCUS_ON), LV_STATE_FOCUS_KEY);
    lv_obj_t * mode_lab = lv_label_create(mode_btn);
    apply_text_color(mode_lab);
    lv_obj_align(mode_lab, LV_ALIGN_LEFT_MID, 0, 0);
    if(ui_light_mode == 0) {
        lv_label_set_text(mode_lab, " OFF ");
    } else {
        lv_label_set_text_fmt(mode_lab, " demo%d ", ui_light_mode);
    }
    lv_obj_add_event_cb(mode_btn, ws2812_mode_event_cb, LV_EVENT_CLICKED, mode_lab);
    lv_event_send(mode_btn, LV_EVENT_VALUE_CHANGED, mode_lab);

    light_arc = lv_arc_create(scr1_cont);
    lv_obj_set_size(light_arc, 100, 100);
    lv_obj_align(light_arc, LV_ALIGN_LEFT_MID, 35, 0);
    lv_obj_set_style_bg_color(light_arc, ws2812_color, LV_PART_KNOB);
    lv_obj_set_style_arc_color(light_arc, ws2812_color, LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(light_arc, 6, LV_PART_MAIN);
    lv_obj_set_style_arc_width(light_arc, 10, LV_PART_INDICATOR);
    lv_arc_set_rotation(light_arc, 90);
    lv_arc_set_bg_angles(light_arc, 0, 360);
    lv_arc_set_range(light_arc, 0, 255);
    lv_arc_set_value(light_arc, ws2812_light);
    lv_obj_set_style_outline_pad(light_arc, 4, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_radius(light_arc, LV_RADIUS_CIRCLE, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_width(light_arc, 2, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_color(light_arc, lv_color_hex(EMBED_COLOR_FOCUS_ON), LV_STATE_FOCUS_KEY);
    lv_group_add_obj(lv_group_get_default(), light_arc);
    lv_obj_t * label1 = lv_label_create(light_arc);
    lv_obj_center(label1);
    apply_text_color(label1);
    lv_obj_add_event_cb(light_arc, value_changed_event_cb, LV_EVENT_VALUE_CHANGED, label1);

    /*Manually update the label for the first time*/
    lv_event_send(light_arc, LV_EVENT_VALUE_CHANGED, NULL);

    // back btn
    scr_back_btn_create(scr1_cont, scr1_btn_event_cb);
}
void entry1(void) {
    entry1_anim(scr1_cont);
    lv_group_set_wrap(lv_group_get_default(), true);
}
void exit1(void) {
    lv_group_set_wrap(lv_group_get_default(), false);
}
void destroy1(void) {}

scr_lifecycle_t screen1 = {
    .create = create1,
    .entry = entry1,
    .exit = exit1,
    .destroy = destroy1,
};
#endif
//************************************[ screen 2 ]****************************************** Sub-G Radio Menu
#if 1
lv_obj_t *scr2_cont;
lv_obj_t *subg_item_cont;
lv_obj_t *subg_menu_cont;
lv_obj_t *subg_menu_icon;
lv_obj_t *subg_menu_icon_lab;
lv_obj_t *subg_first_btn = NULL;  // Store first button to focus it

int subg_focus_item = 0;

const char *subg_menu_items[] = {
    "<- Scan/Record",
    "<- Record Raw",
    "<- Playback",
    "<- Spectrum",
    "<- Remotes",
    "<- Frequencies",
    "<- Back"
};

void entry2_anim(lv_obj_t *obj);
void exit2_anim(int user_data, lv_obj_t *obj);

static void subg_scroll_event_cb(lv_event_t * e)
{
    lv_obj_t * cont = lv_event_get_target(e);

    lv_area_t cont_a;
    lv_obj_get_coords(cont, &cont_a);
    lv_coord_t cont_y_center = cont_a.y1 + lv_area_get_height(&cont_a) / 2;

    lv_coord_t r = lv_obj_get_height(cont) * 7 / 10;
    uint32_t i;
    uint32_t child_cnt = lv_obj_get_child_cnt(cont);
    for(i = 0; i < child_cnt; i++) {
        lv_obj_t * child = lv_obj_get_child(cont, i);
        lv_area_t child_a;
        lv_obj_get_coords(child, &child_a);

        lv_coord_t child_y_center = child_a.y1 + lv_area_get_height(&child_a) / 2;

        lv_coord_t diff_y = child_y_center - cont_y_center;
        diff_y = LV_ABS(diff_y);

        /*Get the x of diff_y on a circle.*/
        lv_coord_t x;
        /*If diff_y is out of the circle use the last point of the circle (the radius)*/
        if(diff_y >= r) {
            x = r;
        }
        else {
            x =  (diff_y * 0.866);
        }

        /*Translate the item by the calculated X coordinate*/
        lv_obj_set_style_translate_x(child, x, 0);

        /*Use some opacity with larger translations*/
        lv_opa_t opa = lv_map(x, 0, r, LV_OPA_TRANSP, LV_OPA_COVER);
        lv_obj_set_style_opa(child, LV_OPA_COVER - opa, 0);
    }
}

static void subg_btn_event_cb(lv_event_t * e)
{
    int button_id = (int)(intptr_t)e->user_data;

    if(e->code == LV_EVENT_CLICKED){
        subg_focus_item = button_id;

        // Navigate to specific Sub-G screens or back
        switch(subg_focus_item) {
            case 0: // Scan/Record
                scr_mgr_switch(SCREEN2_3_ID, false);
                break;
            case 1: // Record Raw
                scr_mgr_switch(SCREEN2_1_ID, false);
                break;
            case 2: // Playback
                scr_mgr_switch(SCREEN2_2_ID, false);
                break;
            case 3: // Spectrum
                scr_mgr_switch(SCREEN2_4_ID, false);
                break;
            case 4: // Remotes
                scr_mgr_switch(SCREEN2_5_ID, false);
                break;
            case 5: // Frequencies
                scr_mgr_switch(SCREEN2_6_ID, false);
                break;
            case 6: // Back
                exit2_anim(SCREEN0_ID, scr2_cont);
                break;
            default:
                break;
        }
    }

    if(e->code == LV_EVENT_FOCUSED){
        // Update icon display based on focused item
        switch(button_id) {
            case 0: // Scan/Record
                lv_img_set_src(subg_menu_icon, &img_sghz_32);
                lv_obj_set_style_img_recolor(subg_menu_icon, lv_palette_main(LV_PALETTE_CYAN), LV_PART_MAIN);
                lv_obj_set_style_img_recolor_opa(subg_menu_icon, LV_OPA_100, LV_PART_MAIN);
                break;
            case 1: // Record Raw
                lv_img_set_src(subg_menu_icon, &img_sghz_32);
                lv_obj_set_style_img_recolor(subg_menu_icon, lv_palette_main(LV_PALETTE_RED), LV_PART_MAIN);
                lv_obj_set_style_img_recolor_opa(subg_menu_icon, LV_OPA_100, LV_PART_MAIN);
                break;
            case 2: // Playback
                lv_img_set_src(subg_menu_icon, &img_sghz_32);
                lv_obj_set_style_img_recolor(subg_menu_icon, lv_palette_main(LV_PALETTE_GREEN), LV_PART_MAIN);
                lv_obj_set_style_img_recolor_opa(subg_menu_icon, LV_OPA_100, LV_PART_MAIN);
                break;
            case 3: // Spectrum
                lv_img_set_src(subg_menu_icon, &img_sghz_32);
                lv_obj_set_style_img_recolor(subg_menu_icon, lv_palette_main(LV_PALETTE_BLUE), LV_PART_MAIN);
                lv_obj_set_style_img_recolor_opa(subg_menu_icon, LV_OPA_100, LV_PART_MAIN);
                break;
            case 4: // Remotes
                lv_img_set_src(subg_menu_icon, &img_setting_32);
                lv_obj_set_style_img_recolor_opa(subg_menu_icon, LV_OPA_0, LV_PART_MAIN);
                break;
            case 5: // Frequencies
                lv_img_set_src(subg_menu_icon, &img_sghz_32);
                lv_obj_set_style_img_recolor(subg_menu_icon, lv_palette_main(LV_PALETTE_YELLOW), LV_PART_MAIN);
                lv_obj_set_style_img_recolor_opa(subg_menu_icon, LV_OPA_100, LV_PART_MAIN);
                break;
            case 6: // Back
                lv_img_set_src(subg_menu_icon, &img_dev_32);
                lv_obj_set_style_img_recolor_opa(subg_menu_icon, LV_OPA_0, LV_PART_MAIN);
                break;
            default:
                break;
        }
        //lv_label_set_text(subg_menu_icon_lab, ((char*)subg_menu_items[button_id] + 3));
        lv_label_set_text(subg_menu_icon_lab, ((char*)subg_menu_items[button_id]));
        lv_obj_align_to(subg_menu_icon_lab, subg_menu_cont, LV_ALIGN_BOTTOM_MID, 0, -25);
    }
}

void entry2_anim(lv_obj_t *obj)
{
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, subg_item_cont);
    lv_anim_set_values(&a, DISPALY_WIDTH * MENU_PROPORTION, 0);
    lv_anim_set_time(&a, 400);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_in_out);
    lv_anim_set_exec_cb(&a, [](void * var, int32_t v){
        lv_obj_set_x((lv_obj_t *)var, v);
        lv_port_indev_enabled(false);
    });
    lv_anim_set_ready_cb(&a, [](struct _lv_anim_t *a){
        lv_port_indev_enabled(true);
    });
    lv_anim_start(&a);

    lv_anim_set_time(&a, 400);
    lv_anim_set_var(&a, subg_menu_cont);
    lv_anim_set_values(&a, -DISPALY_WIDTH * MENU_LAB_PROPORTION, 0);
    lv_anim_set_exec_cb(&a, [](void * var, int32_t v){
        lv_obj_set_x((lv_obj_t *)var, v);
        lv_port_indev_enabled(false);
    });
    lv_anim_start(&a);
}

void exit2_anim(int user_data, lv_obj_t *obj)
{
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, subg_item_cont);
    lv_anim_set_values(&a, 0, DISPALY_WIDTH * MENU_PROPORTION);
    lv_anim_set_time(&a, 400);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_in_out);
    lv_anim_set_exec_cb(&a, [](void * var, int32_t v){
        lv_obj_set_x((lv_obj_t *)var, v);
        lv_port_indev_enabled(false);
    });
    lv_anim_set_ready_cb(&a, [](struct _lv_anim_t *a){
        scr_mgr_switch((int)a->user_data, false);
        lv_port_indev_enabled(true);
    });
    lv_anim_set_user_data(&a, (void *)user_data);
    lv_anim_start(&a);

    lv_anim_set_time(&a, 400);
    lv_anim_set_var(&a, subg_menu_cont);
    lv_anim_set_values(&a, 0, -DISPALY_WIDTH * MENU_LAB_PROPORTION);
    lv_anim_set_exec_cb(&a, [](void * var, int32_t v){
        lv_obj_set_x((lv_obj_t *)var, v);
        lv_port_indev_enabled(false);
    });
    lv_anim_start(&a);
}

void create2(lv_obj_t *parent){
    scr2_cont = create_screen_container(parent);

    // Left side - Icon display panel (matches main menu layout)
    subg_menu_cont = lv_obj_create(scr2_cont);
    lv_obj_set_size(subg_menu_cont, DISPALY_WIDTH * MENU_LAB_PROPORTION, DISPALY_HEIGHT);
    apply_no_scrollbar(subg_menu_cont);
    lv_obj_set_align(subg_menu_cont, LV_ALIGN_LEFT_MID);
    apply_bg_color(subg_menu_cont);
    apply_no_padding(subg_menu_cont);
    apply_no_border(subg_menu_cont);
    apply_no_radius(subg_menu_cont);

    subg_menu_icon = lv_img_create(subg_menu_cont);
    lv_obj_align(subg_menu_icon, LV_ALIGN_CENTER, 0, 0);

    subg_menu_icon_lab = lv_label_create(subg_menu_cont);
    lv_obj_set_width(subg_menu_icon_lab, DISPALY_WIDTH * MENU_LAB_PROPORTION);
    lv_obj_set_style_text_align(subg_menu_icon_lab, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    apply_text_color(subg_menu_icon_lab);
    lv_obj_set_style_text_font(subg_menu_icon_lab, FONT_BOLD_18, LV_PART_MAIN);
    lv_obj_align_to(subg_menu_icon_lab, subg_menu_cont, LV_ALIGN_BOTTOM_MID, 0, -25);

    // Right side - Scrollable menu items (matches main menu layout)
    subg_item_cont = lv_obj_create(scr2_cont);
    lv_obj_set_size(subg_item_cont, DISPALY_WIDTH * MENU_PROPORTION, DISPALY_HEIGHT);
    lv_obj_set_align(subg_item_cont, LV_ALIGN_RIGHT_MID);
    lv_obj_set_flex_flow(subg_item_cont, LV_FLEX_FLOW_COLUMN);
    apply_bg_color(subg_item_cont);
    apply_no_border(subg_item_cont);
    lv_obj_add_event_cb(subg_item_cont, subg_scroll_event_cb, LV_EVENT_SCROLL, NULL);
    apply_no_radius(subg_item_cont);
    lv_obj_set_style_clip_corner(subg_item_cont, true, 0);
    lv_obj_set_scroll_dir(subg_item_cont, LV_DIR_VER);
    lv_obj_set_scroll_snap_y(subg_item_cont, LV_SCROLL_SNAP_CENTER);
    apply_no_scrollbar(subg_item_cont);
    lv_obj_set_style_pad_row(subg_item_cont, 10, LV_PART_MAIN);

    // Create 7 menu item buttons (6 features + Back)
    for(int i = 0; i < 7; i++) {
        lv_obj_t *btn = lv_btn_create(subg_item_cont);
        lv_obj_set_width(btn, lv_pct(100));
        lv_obj_set_style_radius(btn, 10, LV_PART_MAIN);
        apply_bg_color(btn);
        apply_no_border(btn);
        apply_no_shadow(btn);
        lv_obj_remove_style(btn, NULL, LV_STATE_FOCUS_KEY);
        lv_obj_set_style_outline_width(btn, 2, LV_STATE_FOCUS_KEY);
        lv_obj_set_style_outline_color(btn, lv_color_hex(EMBED_COLOR_FOCUS_ON), LV_STATE_FOCUS_KEY);
        lv_obj_set_style_bg_img_opa(btn, LV_OPA_100, LV_PART_MAIN);
        add_menu_button_events(btn, subg_btn_event_cb, (void *)(intptr_t)i);

        lv_obj_t *label = lv_label_create(btn);
        apply_text_color(label);
        lv_obj_set_style_text_font(label, FONT_BOLD_14, LV_PART_MAIN);
        lv_label_set_text(label, subg_menu_items[i]);

        lv_group_add_obj(lv_group_get_default(), btn);

        // Store first button to focus it later
        if(i == 0) {
            subg_first_btn = btn;
        }
    }

    // Add status bar last (on parent, not scr2_cont) so it appears on top
    create_status_bar(parent);
}

void entry2(void) {
    if(subg_first_btn) {
        lv_group_focus_obj(subg_first_btn);
    }
    lv_group_set_wrap(lv_group_get_default(), true);
}

void exit2(void) {
    lv_group_set_wrap(lv_group_get_default(), false);
}

void destroy2(void) {
    // Cleanup if needed
}

scr_lifecycle_t screen2 = {
    .create = create2,
    .entry = entry2,
    .exit = exit2,
    .destroy = destroy2,
};
#endif

//************************************[ Frequency Selector Variables ]******************************************
// Shared variables for SubGHz frequency selection (used by multiple SubGHz screens)
// NOTE: subghz_frequency_list is now defined in rf_utils.cpp and declared extern in rf_utils.h
// 57 common Sub-GHz frequencies
extern const float subghz_frequency_list[];
const int subghz_frequency_count = 57;  // Must match the array size in rf_utils.cpp

extern const float subghz_mhz_list[];
const int subghz_mhz_count = 277;  // Must match the array size in rf_utils.cpp

float selected_frequency = SUBGHZ_DEFAULT_FREQ;
int freq_selector_return_screen = SCREEN2_1_ID;  // Track which screen to return to (reusable for all SubGHz features)

// Custom range helper functions (from peri_subghz.cpp)
extern bool subghz_is_freq_valid(float freq_mhz);
extern float subghz_align_freq_to_step(float freq_mhz, float step_khz);
extern float subghz_next_valid_freq(float current_freq_mhz, float step_khz);

//************************************[ screen 2.1 ]****************************************** SubGHz Record Raw
#if 1

lv_obj_t *scr2_1_cont;
lv_obj_t *recraw_rssi_bar;
lv_obj_t *recraw_rssi_label;
lv_obj_t *recraw_freq_label;
lv_obj_t *recraw_status_label;
lv_obj_t *recraw_pulse_label;
lv_obj_t *recraw_btn_capture;
lv_obj_t *recraw_btn_save;
lv_obj_t *recraw_btn_preset;
lv_obj_t *recraw_led;
lv_timer_t *recraw_update_timer = NULL;
bool recraw_is_capturing = false;

// Global preset selection (shared across all SubGHz screens)
uint8_t subghz_current_preset = SUBGHZ_PRESET_OOK270;  // Default to OOK270 (closest to our 256kHz)

void entry2_1_anim(lv_obj_t *obj) { entry1_anim(obj); }
void exit2_1_anim(int user_data, lv_obj_t *obj) { exit1_anim(user_data, obj); }

static void recraw_update_timer_event(lv_timer_t *t)
{
    if (!recraw_is_capturing) {
        return;
    }

    // Get capture data
    RawRecording *rec = subghz_get_capture();
    RawRecordingStatus *status = subghz_get_status();

    if (status) {
        // Update RSSI bar (map -100 to -30 dBm to 0-100%)
        int rssi_percent = map(status->latestRssi, -100, -30, 0, 100);
        rssi_percent = constrain(rssi_percent, 0, 100);
        lv_bar_set_value(recraw_rssi_bar, rssi_percent, LV_ANIM_ON);

        // Update RSSI label
        lv_label_set_text_fmt(recraw_rssi_label, "RSSI: %d dBm (Peak: %d)",
                              status->latestRssi, status->peakRssi);

        // Update pulse count
        lv_label_set_text_fmt(recraw_pulse_label, "Pulses: %d", status->pulseCount);

        // LED stays on during capture (removed blinking logic)
    }
}

static void recraw_btn_capture_event(lv_event_t *e)
{
    if (e->code == LV_EVENT_CLICKED) {
        if (!recraw_is_capturing) {
            // Start capture
            // (SubGHz initialized in enter2_1)

            // Start capture at selected frequency
            if (subghz_start_capture(selected_frequency, SUBGHZ_CAPTURE_TIMEOUT)) {
                recraw_is_capturing = true;
                lv_label_set_text(lv_obj_get_child(recraw_btn_capture, 0), "Stop");
                lv_label_set_text(recraw_status_label, "Status: Capturing...");
                lv_obj_add_state(recraw_btn_save, LV_STATE_DISABLED);
                lv_led_set_brightness(recraw_led, 255);  // Set to maximum brightness
                lv_led_on(recraw_led);
            } else {
                lv_label_set_text(recraw_status_label, "Status: Start failed!");
            }
        } else {
            // Stop capture
            subghz_stop_capture();
            recraw_is_capturing = false;
            lv_label_set_text(lv_obj_get_child(recraw_btn_capture, 0), "Capture");
            lv_label_set_text(recraw_status_label, "Status: Stopped");
            lv_obj_clear_state(recraw_btn_save, LV_STATE_DISABLED);
            lv_led_off(recraw_led);
        }
    }
}

static void recraw_btn_save_event(lv_event_t *e)
{
    if (e->code == LV_EVENT_CLICKED) {

        RawRecording *rec = subghz_get_capture();
        if (rec && rec->codes.size() > 0) {
            String savedFile = subghz_save_capture(*rec);
            if (savedFile.length() > 0) {
                // Extract just the filename from the full path
                int lastSlash = savedFile.lastIndexOf('/');
                String displayName = (lastSlash >= 0) ? savedFile.substring(lastSlash + 1) : savedFile;

                lv_label_set_text(recraw_status_label, "Status: Saved!");

                // Create message with filename (truncate if too long)
                String msg = "  Saved:\n  ";
                if (displayName.length() > 20) {
                    msg += displayName.substring(0, 17) + "...";
                } else {
                    msg += displayName;
                }
                prompt_info(msg.c_str(), 2000);
            } else {
                lv_label_set_text(recraw_status_label, "Status: Save failed!");
                prompt_info("  Failed to save\n  Check SD card", 2000);
            }
        } else {
            lv_label_set_text(recraw_status_label, "Status: No data!");
            prompt_info("  No signal data\n  to save", 2000);
        }
    }
}

static void recraw_freq_btn_event(lv_event_t *e)
{
    if (e->code == LV_EVENT_CLICKED) {
        freq_selector_return_screen = SCREEN2_1_ID;  // Set return screen for frequency selector
        exit2_1_anim(SCREEN2_1_1_ID, scr2_1_cont);
    }
}

// Helper function to get short preset name for UI display
const char* get_preset_short_name(uint8_t preset) {
    switch(preset) {
        case SUBGHZ_PRESET_OOK270:
            return "AM270";
        case SUBGHZ_PRESET_OOK650:
            return "AM650";
        case SUBGHZ_PRESET_2FSK_DEV238:
            return "FM238";
        case SUBGHZ_PRESET_2FSK_DEV476:
            return "FM476";
        default:
            return "AM270";
    }
}

static void recraw_preset_btn_event(lv_event_t *e)
{
    if (e->code == LV_EVENT_CLICKED) {
        // Cycle through all presets: AM270, AM650, FM238, FM476
        const uint8_t presets[] = {
            SUBGHZ_PRESET_OOK270,      // AM270
            SUBGHZ_PRESET_OOK650,      // AM650
            SUBGHZ_PRESET_2FSK_DEV238, // FM238
            SUBGHZ_PRESET_2FSK_DEV476  // FM476
        };
        const int num_presets = sizeof(presets) / sizeof(presets[0]);

        // Find current preset index
        int current_idx = 0;
        for (int i = 0; i < num_presets; i++) {
            if (subghz_current_preset == presets[i]) {
                current_idx = i;
                break;
            }
        }

        // Move to next preset (wrap around)
        current_idx = (current_idx + 1) % num_presets;
        subghz_current_preset = presets[current_idx];

        // Update button label
        lv_label_set_text(lv_obj_get_child(recraw_btn_preset, 0), get_preset_short_name(subghz_current_preset));


        // Save to config
        const char* mod_str = "am650";
        if (subghz_current_preset == SUBGHZ_PRESET_OOK270) mod_str = "am270";
        else if (subghz_current_preset == SUBGHZ_PRESET_OOK650) mod_str = "am650";
        else if (subghz_current_preset == SUBGHZ_PRESET_2FSK_DEV238) mod_str = "fm238";
        else if (subghz_current_preset == SUBGHZ_PRESET_2FSK_DEV476) mod_str = "fm476";
        config_save_modulation(mod_str);
    }
}

static void scr2_1_back_btn_event_cb(lv_event_t *e)
{
    if (e->code == LV_EVENT_CLICKED) {
        // Stop capture if running
        if (recraw_is_capturing) {
            subghz_stop_capture();
            recraw_is_capturing = false;
        }
        exit2_1_anim(SCREEN2_ID, scr2_1_cont);
    }
}

static void create2_1(lv_obj_t *parent)
{
    scr2_1_cont = create_screen_container(parent);

    // Title
    lv_obj_t *label = lv_label_create(scr2_1_cont);
    apply_text_color(label);
    lv_obj_set_style_text_font(label, FONT_BOLD_18, LV_PART_MAIN);
    lv_label_set_text(label, "Record RAW");
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 10);

    // Frequency button (to select frequency)
    recraw_freq_label = lv_btn_create(scr2_1_cont);
    lv_obj_set_size(recraw_freq_label, 80, 28);
    lv_obj_align(recraw_freq_label, LV_ALIGN_TOP_RIGHT, -10, 40);
    lv_obj_set_style_border_color(recraw_freq_label, lv_color_hex(EMBED_COLOR_BORDER), LV_PART_MAIN);
    lv_obj_set_style_border_width(recraw_freq_label, 1, LV_PART_MAIN);
    apply_no_shadow(recraw_freq_label);
    apply_bg_color(recraw_freq_label);
    lv_obj_remove_style(recraw_freq_label, NULL, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_pad(recraw_freq_label, 2, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_width(recraw_freq_label, 2, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_color(recraw_freq_label, lv_color_hex(EMBED_COLOR_FOCUS_ON), LV_STATE_FOCUS_KEY);
    lv_obj_add_event_cb(recraw_freq_label, recraw_freq_btn_event, LV_EVENT_CLICKED, NULL);
    lv_obj_t *freq_label = lv_label_create(recraw_freq_label);
    apply_text_color(freq_label);
    lv_obj_set_style_text_font(freq_label, FONT_BOLD_14, LV_PART_MAIN);
    lv_label_set_text_fmt(freq_label, "%.2f", selected_frequency);
    lv_obj_center(freq_label);
    lv_group_add_obj(lv_group_get_default(), recraw_freq_label);

    // LED indicator
    recraw_led = lv_led_create(scr2_1_cont);
    lv_obj_align(recraw_led, LV_ALIGN_TOP_RIGHT, -100, 44);
    lv_obj_set_size(recraw_led, 15, 15);
    lv_led_set_color(recraw_led, lv_palette_main(LV_PALETTE_RED));
    lv_led_off(recraw_led);

    // RSSI Bar
    recraw_rssi_bar = lv_bar_create(scr2_1_cont);
    lv_obj_set_size(recraw_rssi_bar, 280, 18);
    lv_obj_align(recraw_rssi_bar, LV_ALIGN_TOP_MID, 0, 75);
    lv_bar_set_range(recraw_rssi_bar, 0, 100);
    lv_bar_set_value(recraw_rssi_bar, 0, LV_ANIM_OFF);

    // RSSI Label
    recraw_rssi_label = lv_label_create(scr2_1_cont);
    apply_text_color(recraw_rssi_label);
    lv_obj_set_style_text_font(recraw_rssi_label, FONT_BOLD_14, LV_PART_MAIN);
    lv_label_set_text(recraw_rssi_label, "RSSI: --- dBm");
    lv_obj_align(recraw_rssi_label, LV_ALIGN_TOP_LEFT, 10, 100);

    // Pulse count label
    recraw_pulse_label = lv_label_create(scr2_1_cont);
    apply_text_color(recraw_pulse_label);
    lv_obj_set_style_text_font(recraw_pulse_label, FONT_BOLD_14, LV_PART_MAIN);
    lv_label_set_text(recraw_pulse_label, "Pulses: 0");
    lv_obj_align(recraw_pulse_label, LV_ALIGN_TOP_RIGHT, -10, 100);

    // Status label (moved to top, same level as LED and freq selector)
    recraw_status_label = lv_label_create(scr2_1_cont);
    apply_text_color(recraw_status_label);
    lv_obj_set_style_text_font(recraw_status_label, FONT_BOLD_14, LV_PART_MAIN);
    lv_label_set_text(recraw_status_label, "Status: Ready");
    lv_obj_align(recraw_status_label, LV_ALIGN_TOP_LEFT, 10, 46);

    // Save button (bottom right - rightmost button)
    recraw_btn_save = lv_btn_create(scr2_1_cont);
    lv_obj_set_size(recraw_btn_save, 70, 32);
    lv_obj_align(recraw_btn_save, LV_ALIGN_BOTTOM_RIGHT, -10, -10);
    lv_obj_set_style_border_color(recraw_btn_save, lv_color_hex(EMBED_COLOR_BORDER), LV_PART_MAIN);
    lv_obj_set_style_border_width(recraw_btn_save, 1, LV_PART_MAIN);
    apply_no_shadow(recraw_btn_save);
    apply_bg_color(recraw_btn_save);
    lv_obj_remove_style(recraw_btn_save, NULL, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_pad(recraw_btn_save, 2, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_width(recraw_btn_save, 2, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_color(recraw_btn_save, lv_color_hex(EMBED_COLOR_FOCUS_ON), LV_STATE_FOCUS_KEY);
    lv_obj_add_event_cb(recraw_btn_save, recraw_btn_save_event, LV_EVENT_CLICKED, NULL);
    lv_obj_t *btn_label = lv_label_create(recraw_btn_save);
    apply_text_color(btn_label);
    lv_obj_set_style_text_font(btn_label, FONT_BOLD_14, LV_PART_MAIN);
    lv_label_set_text(btn_label, "Save");
    lv_obj_center(btn_label);
    lv_group_add_obj(lv_group_get_default(), recraw_btn_save);

    // Capture button (bottom right, next to Save button)
    recraw_btn_capture = lv_btn_create(scr2_1_cont);
    lv_obj_set_size(recraw_btn_capture, 90, 32);
    lv_obj_align(recraw_btn_capture, LV_ALIGN_BOTTOM_RIGHT, -85, -10);  // -85 = -10 (margin) - 70 (save width) - 5 (gap)
    lv_obj_set_style_border_color(recraw_btn_capture, lv_color_hex(EMBED_COLOR_BORDER), LV_PART_MAIN);
    lv_obj_set_style_border_width(recraw_btn_capture, 1, LV_PART_MAIN);
    apply_no_shadow(recraw_btn_capture);
    apply_bg_color(recraw_btn_capture);
    lv_obj_remove_style(recraw_btn_capture, NULL, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_pad(recraw_btn_capture, 2, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_width(recraw_btn_capture, 2, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_color(recraw_btn_capture, lv_color_hex(EMBED_COLOR_FOCUS_ON), LV_STATE_FOCUS_KEY);
    lv_obj_add_event_cb(recraw_btn_capture, recraw_btn_capture_event, LV_EVENT_CLICKED, NULL);
    btn_label = lv_label_create(recraw_btn_capture);
    apply_text_color(btn_label);
    lv_obj_set_style_text_font(btn_label, FONT_BOLD_14, LV_PART_MAIN);
    lv_label_set_text(btn_label, "Capture");
    lv_obj_center(btn_label);
    lv_group_add_obj(lv_group_get_default(), recraw_btn_capture);

    // Preset selector button (bottom left - where Capture used to be)
    recraw_btn_preset = lv_btn_create(scr2_1_cont);
    lv_obj_set_size(recraw_btn_preset, 75, 32);
    lv_obj_align(recraw_btn_preset, LV_ALIGN_BOTTOM_LEFT, 10, -10);
    lv_obj_set_style_border_color(recraw_btn_preset, lv_color_hex(EMBED_COLOR_BORDER), LV_PART_MAIN);
    lv_obj_set_style_border_width(recraw_btn_preset, 1, LV_PART_MAIN);
    apply_no_shadow(recraw_btn_preset);
    apply_bg_color(recraw_btn_preset);
    lv_obj_remove_style(recraw_btn_preset, NULL, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_pad(recraw_btn_preset, 2, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_width(recraw_btn_preset, 2, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_color(recraw_btn_preset, lv_color_hex(EMBED_COLOR_FOCUS_ON), LV_STATE_FOCUS_KEY);
    lv_obj_add_event_cb(recraw_btn_preset, recraw_preset_btn_event, LV_EVENT_CLICKED, NULL);
    btn_label = lv_label_create(recraw_btn_preset);
    apply_text_color(btn_label);
    lv_obj_set_style_text_font(btn_label, FONT_BOLD_14, LV_PART_MAIN);
    lv_label_set_text(btn_label, get_preset_short_name(subghz_current_preset));
    lv_obj_center(btn_label);
    lv_group_add_obj(lv_group_get_default(), recraw_btn_preset);

    // Back button (using standard "<" button in top left)
    scr_back_btn_create(scr2_1_cont, scr2_1_back_btn_event_cb);

    // Create update timer
    recraw_update_timer = lv_timer_create(recraw_update_timer_event, 100, NULL);
    lv_timer_pause(recraw_update_timer);
}

static void enter2_1(void)
{
    // Initialize SubGHz module if needed
    if (!subghz_is_init()) {
        subghz_init();
    }

    entry2_1_anim(scr2_1_cont);
    lv_timer_resume(recraw_update_timer);

    // Load settings from config
    selected_frequency = g_config.subghz.raw.last_frequency;

    // Load modulation preset
    if (strcmp(g_config.subghz.modulation, "am270") == 0) {
        subghz_current_preset = SUBGHZ_PRESET_OOK270;
    } else if (strcmp(g_config.subghz.modulation, "am650") == 0) {
        subghz_current_preset = SUBGHZ_PRESET_OOK650;
    } else if (strcmp(g_config.subghz.modulation, "fm238") == 0) {
        subghz_current_preset = SUBGHZ_PRESET_2FSK_DEV238;
    } else if (strcmp(g_config.subghz.modulation, "fm476") == 0) {
        subghz_current_preset = SUBGHZ_PRESET_2FSK_DEV476;
    }

    // Update UI to reflect loaded settings
    lv_label_set_text(lv_obj_get_child(recraw_btn_preset, 0), get_preset_short_name(subghz_current_preset));
    lv_label_set_text_fmt(lv_obj_get_child(recraw_freq_label, 0), "%.2f MHz", selected_frequency);

    lv_group_set_wrap(lv_group_get_default(), true);
}

static void exit2_1(void)
{
    lv_group_set_wrap(lv_group_get_default(), false);
    // Screen manager handles navigation - don't call lv_scr_load here
}

static void destroy2_1(void)
{
    if (recraw_update_timer) {
        lv_timer_del(recraw_update_timer);
        recraw_update_timer = NULL;
    }

    if (recraw_is_capturing) {
        subghz_stop_capture();
        recraw_is_capturing = false;
    }

    if (scr2_1_cont) {
        lv_obj_del(scr2_1_cont);
        scr2_1_cont = NULL;
    }
}

scr_lifecycle_t screen2_1 = {
    .create = create2_1,
    .entry = enter2_1,
    .exit = exit2_1,
    .destroy = destroy2_1,
};
#endif

//************************************[ screen 2.1.1 ]****************************************** SubGHz Frequency Selector
#if 1
lv_obj_t *scr2_1_1_cont;
lv_obj_t *freq_list;

void entry2_1_1_anim(lv_obj_t *obj) { entry1_anim(obj); }
void exit2_1_1_anim(int user_data, lv_obj_t *obj) { exit1_anim(user_data, obj); }

static void freq_list_event(lv_event_t *e)
{
    if (e->code == LV_EVENT_CLICKED) {
        lv_obj_t *btn = lv_event_get_target(e);
        int idx = (int)(intptr_t)lv_event_get_user_data(e);

        // Update selected frequency (from combined list)
        std::vector<float> freq_list_data = rf_get_combined_frequency_list();
        selected_frequency = freq_list_data[idx];

        // Save to config (save to raw.last_frequency for Record Raw screen)
        config_save_raw_frequency(selected_frequency);

        // Go back to the calling screen
        // Note: Don't update the label directly - the screen will be recreated
        // and will read selected_frequency in its create function
        exit2_1_1_anim(freq_selector_return_screen, scr2_1_1_cont);
    }
}

static void scr2_1_1_back_btn_event_cb(lv_event_t *e)
{
    if (e->code == LV_EVENT_CLICKED) {
        exit2_1_1_anim(freq_selector_return_screen, scr2_1_1_cont);
    }
}

static void create2_1_1(lv_obj_t *parent)
{
    scr2_1_1_cont = create_screen_container(parent);

    // Title
    lv_obj_t *label = lv_label_create(scr2_1_1_cont);
    apply_text_color(label);
    lv_obj_set_style_text_font(label, FONT_BOLD_16, LV_PART_MAIN);
    lv_label_set_text(label, "Select Frequency");
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 10);

    // Frequency list
    freq_list = lv_list_create(scr2_1_1_cont);
    lv_obj_set_size(freq_list, LV_HOR_RES - 10, 125);
    lv_obj_align(freq_list, LV_ALIGN_BOTTOM_MID, 0, -5);
    apply_bg_color(freq_list);
    lv_obj_set_style_pad_top(freq_list, 2, LV_PART_MAIN);
    lv_obj_set_style_pad_row(freq_list, 2, LV_PART_MAIN);
    apply_no_radius(freq_list);
    apply_no_border(freq_list);
    apply_no_shadow(freq_list);

    // Add all frequencies to the list (hardcoded + custom from config)
    std::vector<float> freq_list_data = rf_get_combined_frequency_list();
    for (size_t i = 0; i < freq_list_data.size(); i++) {
        char freq_str[32];
        snprintf(freq_str, sizeof(freq_str), " %.3f MHz", freq_list_data[i]);

        lv_obj_t *item = lv_list_add_btn(freq_list, NULL, freq_str);
        lv_obj_set_style_text_font(item, FONT_BOLD_14, LV_PART_MAIN);
        apply_text_color(item);
        apply_bg_color(item);
        lv_obj_add_event_cb(item, freq_list_event, LV_EVENT_CLICKED, (void*)(intptr_t)i);
        lv_group_add_obj(lv_group_get_default(), item);

        // Highlight currently selected frequency
        if (fabs(freq_list_data[i] - selected_frequency) < 0.01f) {
            lv_obj_set_style_text_color(item, lv_color_hex(0x00FF00), LV_PART_MAIN);
        }

        // Style for focus
        lv_obj_remove_style(item, NULL, LV_STATE_FOCUS_KEY);
        lv_obj_set_style_outline_color(item, lv_color_hex(EMBED_COLOR_FOCUS_ON), LV_STATE_FOCUS_KEY);
        lv_obj_set_style_outline_width(item, 2, LV_STATE_FOCUS_KEY);
    }

    // Back button
    scr_back_btn_create(scr2_1_1_cont, scr2_1_1_back_btn_event_cb);
}

static void enter2_1_1(void)
{
    entry2_1_1_anim(scr2_1_1_cont);
    lv_group_set_wrap(lv_group_get_default(), true);
}

static void exit2_1_1(void)
{
    lv_group_set_wrap(lv_group_get_default(), false);
}

static void destroy2_1_1(void)
{
    if (scr2_1_1_cont) {
        lv_obj_del(scr2_1_1_cont);
        scr2_1_1_cont = NULL;
    }
}

scr_lifecycle_t screen2_1_1 = {
    .create = create2_1_1,
    .entry = enter2_1_1,
    .exit = exit2_1_1,
    .destroy = destroy2_1_1,
};
#endif

//************************************[ screen 2.2 ]****************************************** SubGHz Playback
#if 1
lv_obj_t *scr2_2_cont;
lv_obj_t *playback_path_label;
lv_obj_t *playback_file_list;
lv_obj_t *playback_info_label;
lv_obj_t *playback_led;
char playback_current_path[256] = "/rf";
String playback_selected_file = "";
static bool playback_long_press_occurred = false;
lv_obj_t *playback_rename_keyboard = NULL;  // Keyboard for rename
lv_obj_t *playback_rename_ta = NULL;        // Text area for rename

void entry2_2_anim(lv_obj_t *obj) { entry1_anim(obj); }
void exit2_2_anim(int user_data, lv_obj_t *obj) { exit1_anim(user_data, obj); }

// Forward declarations for rename functionality
static void playback_rename_file(void);
static void playback_rename_keyboard_event(lv_event_t *e);

void playback_load_directory(const char *path);

static void playback_list_event(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *obj = lv_event_get_target(e);

    if (code == LV_EVENT_PRESSED) {
        // Reset flag on new press
        playback_long_press_occurred = false;
    } else if (code == LV_EVENT_LONG_PRESSED) {
        // Set flag to prevent CLICKED from firing
        playback_long_press_occurred = true;

        // Long press on a .sub file - show context menu (Rename/Delete/Cancel)
        const char *item_text = lv_list_get_btn_text(playback_file_list, obj);

        if (item_text && strcmp(item_text, " .. (Parent)") != 0 && strncmp(item_text, " \xF0\x9F\x93\x81", 5) != 0) {
            // It's a file (not parent or folder)
            const char *file_name_with_icon = item_text + 6;
            char file_name[128];
            strncpy(file_name, file_name_with_icon, sizeof(file_name) - 1);
            file_name[sizeof(file_name) - 1] = '\0';

            char *size_start = strstr(file_name, "  ");
            if (size_start) {
                *size_start = '\0';
            }

            char full_path[512];
            snprintf(full_path, sizeof(full_path), "%s/%s", playback_current_path, file_name);
            playback_selected_file = String(full_path);

            // Show context menu with buttons (no close button, just add Cancel as a button)
            static const char * btns[] = {"Rename", "Delete", "Cancel", ""};
            lv_obj_t * mbox = lv_msgbox_create(NULL, "File Options", file_name, btns, false);  // false = no X button
            lv_obj_center(mbox);

            // Add event callback
            lv_obj_add_event_cb(mbox, [](lv_event_t *e) {
                lv_event_code_t code = lv_event_get_code(e);
                if (code == LV_EVENT_VALUE_CHANGED) {
                    lv_obj_t *mbox = lv_event_get_current_target(e);
                    const char *txt = lv_msgbox_get_active_btn_text(mbox);

                    if (txt) {

                        if (strcmp(txt, "Rename") == 0) {
                            playback_rename_file();
                        } else if (strcmp(txt, "Delete") == 0) {
                            // Delete the file
                            if (SD.remove(playback_selected_file.c_str())) {
                                prompt_info("  File deleted", 2000);
                                // Reload directory
                                playback_load_directory(playback_current_path);
                            } else {
                                prompt_info("  Delete failed", 2000);
                            }
                        }
                        // If Cancel or any other button, just close
                    }

                    // Close msgbox and restore navigation
                    lv_group_t *g = lv_group_get_default();
                    lv_group_set_editing(g, false);
                    lv_group_focus_freeze(g, false);
                    lv_msgbox_close(mbox);
                    lv_refr_now(NULL);

                    if (lv_group_get_focused(g) == NULL && lv_obj_get_child_cnt(playback_file_list) > 0) {
                        lv_obj_t *first_item = lv_obj_get_child(playback_file_list, 0);
                        if (first_item) {
                            lv_group_focus_obj(first_item);
                        }
                    }
                }
            }, LV_EVENT_VALUE_CHANGED, NULL);

            // Setup focus
            lv_group_t *g = lv_group_get_default();
            lv_obj_t *btnm = lv_msgbox_get_btns(mbox);

            if (btnm) {
                lv_group_add_obj(g, btnm);
                lv_group_focus_obj(btnm);
                lv_obj_add_state(btnm, LV_STATE_FOCUS_KEY);
                lv_group_set_editing(g, true);
            }
            lv_group_focus_freeze(g, true);
        }
        return;
    } else if (code == LV_EVENT_CLICKED) {
        // Skip if long press just occurred
        if (playback_long_press_occurred) {
            playback_long_press_occurred = false;
            return;
        }
        const char *item_text = lv_list_get_btn_text(playback_file_list, obj);

        if (item_text) {
            if (strcmp(item_text, " .. (Parent)") == 0) {
                char *last_slash = strrchr(playback_current_path, '/');
                if (last_slash && last_slash != playback_current_path) {
                    *last_slash = '\0';
                } else {
                    strcpy(playback_current_path, "/rf");
                }
                playback_load_directory(playback_current_path);
            } else if (strncmp(item_text, " \xF0\x9F\x93\x81", 5) == 0) {
                const char *folder_name = item_text + 6;
                if (strcmp(playback_current_path, "/") == 0 || strcmp(playback_current_path, "/rf") == 0) {
                    snprintf(playback_current_path, sizeof(playback_current_path), "/rf/%s", folder_name);
                } else {
                    snprintf(playback_current_path, sizeof(playback_current_path), "%s/%s", playback_current_path, folder_name);
                }
                playback_load_directory(playback_current_path);
            } else {
                // It's a file - navigate to signal details screen
                const char *file_name_with_icon = item_text + 6;
                char file_name[128];
                strncpy(file_name, file_name_with_icon, sizeof(file_name) - 1);
                file_name[sizeof(file_name) - 1] = '\0';

                char *size_start = strstr(file_name, "  ");
                if (size_start) {
                    *size_start = '\0';
                }

                char full_path[512];
                snprintf(full_path, sizeof(full_path), "%s/%s", playback_current_path, file_name);
                playback_selected_file = String(full_path);

                // Navigate to signal details screen (screen 2.2.1)
                exit2_2_anim(SCREEN2_2_1_ID, scr2_2_cont);
            }
        }
    }
}

static void scr2_2_back_btn_event_cb(lv_event_t *e)
{
    if (e->code == LV_EVENT_CLICKED) {
        exit2_2_anim(SCREEN2_ID, scr2_2_cont);
    }
}

void playback_load_directory(const char *path)
{
    lv_led_on(playback_led);
    lv_refr_now(NULL);

    lv_obj_clean(playback_file_list);
    lv_label_set_text_fmt(playback_path_label, "Path: %s", path);

    if (strcmp(path, "/rf") != 0) {
        lv_obj_t *parent_item = lv_list_add_btn(playback_file_list, NULL, " .. (Parent)");
        lv_obj_add_event_cb(parent_item, playback_list_event, LV_EVENT_CLICKED, NULL);
    }

    // Create directory if it doesn't exist
    if (!SD.exists(path)) {
        SD.mkdir(path);
    }

    // Try multiple mount points (SD card VFS mount location)
    const char* mount_points[] = {"/sd", "/sdcard", "/mnt/sd"};
    DIR *dir = NULL;

    for (int i = 0; i < 3; i++) {
        char test_path[512];
        snprintf(test_path, sizeof(test_path), "%s%s", mount_points[i], path);
        dir = opendir(test_path);
        if (dir) break;
    }

    // Fallback: try direct path
    if (!dir) {
        dir = opendir(path);
    }

    if (!dir) {
        lv_obj_t *item = lv_list_add_btn(playback_file_list, NULL, " Directory not found");
        apply_text_color(item);
        lv_led_off(playback_led);
        return;
    }

    std::vector<String> directories;
    std::vector<String> files;

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        if (entry->d_type == DT_DIR) {
            directories.push_back(String(entry->d_name));
        } else if (entry->d_type == DT_REG) {
            String filename = String(entry->d_name);
            if (filename.endsWith(".sub")) {
                files.push_back(filename);
            }
        }
    }
    closedir(dir);

    for (const auto &dir_name : directories) {
        char item_text[256];
        snprintf(item_text, sizeof(item_text), " \xF0\x9F\x93\x81 %s", dir_name.c_str());
        lv_obj_t *item = lv_list_add_btn(playback_file_list, NULL, item_text);
        lv_obj_add_event_cb(item, playback_list_event, LV_EVENT_CLICKED, NULL);
    }

    for (const auto &file_name : files) {
        char item_text[256];
        snprintf(item_text, sizeof(item_text), " \xF0\x9F\x93\x84 %s", file_name.c_str());
        lv_obj_t *item = lv_list_add_btn(playback_file_list, NULL, item_text);
        lv_obj_add_event_cb(item, playback_list_event, LV_EVENT_CLICKED, NULL);
        lv_obj_add_event_cb(item, playback_list_event, LV_EVENT_PRESSED, NULL);
        lv_obj_add_event_cb(item, playback_list_event, LV_EVENT_LONG_PRESSED, NULL);
    }

    // Apply theme colors to all list items
    for(int i = 0; i < lv_obj_get_child_cnt(playback_file_list); i++) {
        lv_obj_t *item = lv_obj_get_child(playback_file_list, i);
        lv_obj_set_style_text_font(item, FONT_BOLD_14, LV_PART_MAIN);
        apply_bg_color(item);
        apply_text_color(item);
        lv_obj_remove_style(item, NULL, LV_STATE_FOCUS_KEY);
        lv_obj_set_style_radius(item, 5, LV_STATE_FOCUS_KEY);
        lv_obj_set_style_outline_color(item, lv_color_hex(EMBED_COLOR_FOCUS_ON), LV_STATE_FOCUS_KEY);
        lv_obj_set_style_outline_width(item, 2, LV_STATE_FOCUS_KEY);
    }

    lv_led_off(playback_led);
}

static void playback_rename_keyboard_event(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_READY) {
        const char *new_name = lv_textarea_get_text(playback_rename_ta);

        if (strlen(new_name) > 0 && playback_selected_file.length() > 0) {
            String old_path = playback_selected_file;

            char new_path[512];
            if (strcmp(playback_current_path, "/rf") == 0) {
                snprintf(new_path, sizeof(new_path), "/rf/%s", new_name);
            } else {
                snprintf(new_path, sizeof(new_path), "%s/%s", playback_current_path, new_name);
            }

            if (SD.rename(old_path.c_str(), new_path)) {
                prompt_info("  Renamed successfully", 1500);
                playback_load_directory(playback_current_path);
            } else {
                prompt_info("  Rename failed!", 2000);
            }
        }

        lv_group_t *g = lv_group_get_default();
        lv_group_focus_freeze(g, false);

        if (playback_rename_keyboard) {
            lv_obj_del(playback_rename_keyboard);
            playback_rename_keyboard = NULL;
        }
        if (playback_rename_ta) {
            lv_obj_del(playback_rename_ta);
            playback_rename_ta = NULL;
        }

        playback_selected_file = "";

        if (lv_group_get_focused(g) == NULL && lv_obj_get_child_cnt(playback_file_list) > 0) {
            lv_obj_t *first_item = lv_obj_get_child(playback_file_list, 0);
            if (first_item) {
                lv_group_focus_obj(first_item);
            }
        }
    } else if (code == LV_EVENT_CANCEL) {
        lv_group_t *g = lv_group_get_default();
        lv_group_focus_freeze(g, false);

        if (playback_rename_keyboard) {
            lv_obj_del(playback_rename_keyboard);
            playback_rename_keyboard = NULL;
        }
        if (playback_rename_ta) {
            lv_obj_del(playback_rename_ta);
            playback_rename_ta = NULL;
        }

        playback_selected_file = "";

        if (lv_group_get_focused(g) == NULL && lv_obj_get_child_cnt(playback_file_list) > 0) {
            lv_obj_t *first_item = lv_obj_get_child(playback_file_list, 0);
            if (first_item) {
                lv_group_focus_obj(first_item);
            }
        }
    }
}

static void playback_rename_file(void)
{
    if (playback_selected_file.length() == 0) return;

    const char *full_path = playback_selected_file.c_str();
    const char *file_name = strrchr(full_path, '/');
    if (file_name) {
        file_name++;
    } else {
        file_name = full_path;
    }

    playback_rename_ta = lv_textarea_create(lv_scr_act());
    lv_obj_set_size(playback_rename_ta, LV_PCT(90), 40);
    lv_obj_align(playback_rename_ta, LV_ALIGN_TOP_MID, 0, 40);
    lv_textarea_set_one_line(playback_rename_ta, true);
    lv_textarea_set_placeholder_text(playback_rename_ta, "Enter new name");
    lv_textarea_set_text(playback_rename_ta, file_name);

    lv_obj_set_style_anim_time(playback_rename_ta, 400, LV_PART_CURSOR);
    lv_obj_set_style_bg_opa(playback_rename_ta, LV_OPA_COVER, LV_PART_CURSOR);
    lv_obj_set_style_bg_color(playback_rename_ta, lv_color_hex(EMBED_COLOR_FOCUS_ON), LV_PART_CURSOR);
    lv_obj_set_style_border_width(playback_rename_ta, 1, LV_PART_CURSOR);
    lv_obj_set_style_border_color(playback_rename_ta, lv_color_hex(EMBED_COLOR_FOCUS_ON), LV_PART_CURSOR);
    lv_textarea_set_cursor_pos(playback_rename_ta, LV_TEXTAREA_CURSOR_LAST);

    playback_rename_keyboard = lv_keyboard_create(lv_scr_act());
    lv_keyboard_set_textarea(playback_rename_keyboard, playback_rename_ta);
    lv_obj_set_size(playback_rename_keyboard, LV_HOR_RES, LV_VER_RES / 2);
    lv_obj_align(playback_rename_keyboard, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_add_event_cb(playback_rename_keyboard, playback_rename_keyboard_event, LV_EVENT_ALL, NULL);

    lv_group_t *g = lv_group_get_default();
    lv_group_focus_freeze(g, true);
    lv_group_focus_obj(playback_rename_keyboard);
    lv_group_set_editing(g, true);
}

static void create2_2(lv_obj_t *parent)
{
    scr2_2_cont = create_screen_container(parent);

    lv_obj_t *label = lv_label_create(scr2_2_cont);
    apply_text_color(label);
    lv_obj_set_style_text_font(label, FONT_BOLD_18, LV_PART_MAIN);
    lv_label_set_text(label, "Playback");
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 10);

    playback_path_label = lv_label_create(scr2_2_cont);
    apply_text_color(playback_path_label);
    lv_obj_set_style_text_font(playback_path_label, FONT_BOLD_14, LV_PART_MAIN);
    lv_label_set_text(playback_path_label, "Path: /rf");
    lv_obj_align(playback_path_label, LV_ALIGN_TOP_LEFT, 10, 40);

    playback_led = lv_led_create(scr2_2_cont);
    lv_obj_align(playback_led, LV_ALIGN_TOP_RIGHT, -10, 43);
    lv_obj_set_size(playback_led, 15, 15);
    lv_led_set_color(playback_led, lv_palette_main(LV_PALETTE_GREEN));
    lv_led_off(playback_led);

    playback_file_list = lv_list_create(scr2_2_cont);
    lv_obj_set_size(playback_file_list, 300, 105);
    lv_obj_align(playback_file_list, LV_ALIGN_TOP_MID, 0, 65);
    apply_bg_color(playback_file_list);
    lv_obj_set_style_border_width(playback_file_list, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(playback_file_list, lv_color_hex(EMBED_COLOR_BORDER), LV_PART_MAIN);

    playback_info_label = lv_label_create(scr2_2_cont);
    apply_text_color(playback_info_label);
    lv_obj_set_style_text_font(playback_info_label, FONT_BOLD_14, LV_PART_MAIN);
    lv_label_set_text(playback_info_label, "Select a .sub file to transmit");
    lv_obj_set_width(playback_info_label, 300);
    lv_label_set_long_mode(playback_info_label, LV_LABEL_LONG_WRAP);
    lv_obj_align(playback_info_label, LV_ALIGN_TOP_MID, 0, 180);

    playback_load_directory(playback_current_path);

    // Back button (using standard "<" button in top left) - Added last for default focus
    scr_back_btn_create(scr2_2_cont, scr2_2_back_btn_event_cb);
}

static void enter2_2(void)
{
    // Initialize SubGHz module if needed
    if (!subghz_is_init()) {
        subghz_init();
    }

    entry2_2_anim(scr2_2_cont);
    lv_group_set_wrap(lv_group_get_default(), true);
}

static void exit2_2(void)
{
    lv_group_set_wrap(lv_group_get_default(), false);
}

static void destroy2_2(void)
{
    if (scr2_2_cont) {
        lv_obj_del(scr2_2_cont);
        scr2_2_cont = NULL;
    }
}

scr_lifecycle_t screen2_2 = {
    .create = create2_2,
    .entry = enter2_2,
    .exit = exit2_2,
    .destroy = destroy2_2,
};
#endif

//************************************[ screen 2.2.1 ]************************************** SubGHz Signal Details
#if 1
lv_obj_t *scr2_2_1_cont;
lv_obj_t *details_protocol_label;
lv_obj_t *details_metadata_label1;
lv_obj_t *details_metadata_label2;
lv_obj_t *details_metadata_label3;
lv_obj_t *details_metadata_label4;
lv_obj_t *details_send_btn;
lv_obj_t *details_send_label;
lv_obj_t *details_inc_btn;
lv_obj_t *details_dec_btn;

// State
RfCodes current_signal_codes;
bool details_transmitting = false;
bool details_long_press_active = false;

void entry2_2_1_anim(lv_obj_t *obj) { entry1_anim(obj); }
void exit2_2_1_anim(int user_data, lv_obj_t *obj) { exit1_anim(user_data, obj); }

/**
 * Load and display signal details
 */
static void load_signal_details() {
    // (SubGHz initialized in enter2_2_1)

    if (!subghz_load_file(playback_selected_file.c_str(), current_signal_codes)) {
        prompt_info("  Error loading file", 2000);
        return;
    }

    // Extract filename from path
    const char *filename = strrchr(playback_selected_file.c_str(), '/');
    filename = filename ? filename + 1 : playback_selected_file.c_str();

    // Build title line: ${PROTOCOL_NAME} ${FREQ} ${Modulation}
    char title[128];
    float freq_mhz = current_signal_codes.frequency / 1000000.0f;

    // Determine modulation from preset
    const char *modulation = "AM650";
    if (current_signal_codes.preset.indexOf("270") >= 0) {
        modulation = "AM270";
    } else if (current_signal_codes.preset.indexOf("476") >= 0) {
        modulation = "FM476";
    } else if (current_signal_codes.preset.indexOf("238") >= 0) {
        modulation = "FM238";
    }

    snprintf(title, sizeof(title), "%s %.2f %s",
             current_signal_codes.protocol.c_str(), freq_mhz, modulation);
    lv_label_set_text(details_protocol_label, title);

    // Build metadata lines based on protocol
    char meta1[128] = "";
    char meta2[128] = "";
    char meta3[128] = "";

    if (current_signal_codes.protocol == "CAME" || current_signal_codes.protocol == "CAME 12bit") {
        // CAME protocol: Key only
        snprintf(meta1, sizeof(meta1), "Key: 0x%llX", current_signal_codes.key);
        lv_label_set_text(details_metadata_label1, meta1);
        lv_label_set_text(details_metadata_label2, "");
        lv_label_set_text(details_metadata_label3, "");
        lv_label_set_text(details_metadata_label4, "");

        // Hide increment/decrement buttons
        lv_obj_add_flag(details_inc_btn, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(details_dec_btn, LV_OBJ_FLAG_HIDDEN);

    } else if (current_signal_codes.protocol == "Security+ 2.0" && current_signal_codes.secplus_valid) {
        // Security+ 2.0: Show packets, serial, button, counter on 4 lines
        char meta4[128] = "";

        // Packets are 40 bits (10 hex), but display as 12 hex to match Flipper format
        snprintf(meta1, sizeof(meta1), "Pk1:0x%012llX",
                 current_signal_codes.secplus_packet1);
        snprintf(meta2, sizeof(meta2), "Pk2:0x%012llX",
                 current_signal_codes.secplus_packet2);
        // Serial is stored in secplus_fixed field (32 bits = 8 hex digits)
        snprintf(meta3, sizeof(meta3), "Sn: 0x%08llX Btn: 0x%02X",
                 current_signal_codes.secplus_fixed, current_signal_codes.secplus_button);
        snprintf(meta4, sizeof(meta4), "Cnt:  0x%07X", current_signal_codes.secplus_rolling);

        lv_label_set_text(details_metadata_label1, meta1);
        lv_label_set_text(details_metadata_label2, meta2);
        lv_label_set_text(details_metadata_label3, meta3);
        lv_label_set_text(details_metadata_label4, meta4);

        // Show increment/decrement buttons for rolling codes
        lv_obj_clear_flag(details_inc_btn, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(details_dec_btn, LV_OBJ_FLAG_HIDDEN);

    } else if (current_signal_codes.protocol == "Security+ 1.0") {
        // Security+ 1.0: Show fixed, rolling, decoded parameters on 4 lines
        char meta4[128] = "";

        // Extract fixed (upper 32 bits) and rolling (lower 32 bits) from 64-bit key
        uint32_t fixed = (current_signal_codes.key >> 32) & 0xFFFFFFFF;
        uint32_t rolling = current_signal_codes.key & 0xFFFFFFFF;

        // Calculate display fields (matches protocol_secplus_v1.cpp:106-109)
        uint8_t btn = fixed % 3;
        uint8_t id0 = (fixed / 3) % 3;
        uint8_t id1 = (fixed / 9) % 3;
        uint32_t serial = fixed / 27;

        const char* btn_name = (btn == 1) ? "Left" : (btn == 0) ? "Middle" : "Right";

        // Display format similar to Security+ 2.0
        snprintf(meta1, sizeof(meta1), "Fix: 0x%08X", fixed);
        snprintf(meta2, sizeof(meta2), "Key: 0x%08X", rolling);
        snprintf(meta3, sizeof(meta3), "Sn: 0x%07X Btn: %s", serial, btn_name);
        snprintf(meta4, sizeof(meta4), "id1:%d id0:%d", id1, id0);

        lv_label_set_text(details_metadata_label1, meta1);
        lv_label_set_text(details_metadata_label2, meta2);
        lv_label_set_text(details_metadata_label3, meta3);
        lv_label_set_text(details_metadata_label4, meta4);

        // Show increment/decrement buttons for rolling codes
        lv_obj_clear_flag(details_inc_btn, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(details_dec_btn, LV_OBJ_FLAG_HIDDEN);

    } else if (current_signal_codes.protocol == "RAW") {
        // RAW protocol: just show it's RAW
        lv_label_set_text(details_metadata_label1, "");
        lv_label_set_text(details_metadata_label2, "");
        lv_label_set_text(details_metadata_label3, "");
        lv_label_set_text(details_metadata_label4, "");

        // Hide increment/decrement buttons
        lv_obj_add_flag(details_inc_btn, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(details_dec_btn, LV_OBJ_FLAG_HIDDEN);

    } else {
        // Generic protocol: Show key, TE, bit count if available
        if (current_signal_codes.key != 0) {
            snprintf(meta1, sizeof(meta1), "Key: 0x%llX", current_signal_codes.key);
            lv_label_set_text(details_metadata_label1, meta1);
        } else {
            lv_label_set_text(details_metadata_label1, "");
        }

        if (current_signal_codes.te > 0) {
            snprintf(meta2, sizeof(meta2), "Te:  %dus", current_signal_codes.te);
            lv_label_set_text(details_metadata_label2, meta2);
        } else {
            lv_label_set_text(details_metadata_label2, "");
        }

        if (current_signal_codes.Bit > 0) {
            snprintf(meta3, sizeof(meta3), "Bits: %d", current_signal_codes.Bit);
            lv_label_set_text(details_metadata_label3, meta3);
        } else {
            lv_label_set_text(details_metadata_label3, "");
        }

        lv_label_set_text(details_metadata_label4, "");

        // Hide increment/decrement buttons
        lv_obj_add_flag(details_inc_btn, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(details_dec_btn, LV_OBJ_FLAG_HIDDEN);
    }
}

/**
 * Re-encode and save Security+ 2.0 signal with updated counter
 */
static bool reencode_secplus_signal() {
    if (current_signal_codes.protocol != "Security+ 2.0" || !current_signal_codes.secplus_valid) {
        return false;
    }


    // Reconstruct 40-bit fixed code from button (8 bits) + serial (32 bits)
    // Fixed structure: [button(8)] [serial(32)]
    uint64_t fixed = ((uint64_t)current_signal_codes.secplus_button << 32) |
                     (current_signal_codes.secplus_fixed & 0xFFFFFFFFULL);

    // Re-encode the packets with new counter
    SecPlusV2Protocol secplus;
    if (!secplus.encodeFromCodes(fixed, current_signal_codes.secplus_rolling)) {
        return false;
    }

    // Get the encoded packet data
    uint64_t packet1, packet2;
    uint64_t serial_check;
    uint8_t button_check;
    uint32_t counter_check;
    if (!secplus.getDecodedParams(serial_check, button_check, counter_check, packet1, packet2)) {
        return false;
    }

    // Update the .sub file on SD card
    File file = SD.open(playback_selected_file.c_str(), FILE_WRITE);
    if (!file) {
        return false;
    }

    // Write updated .sub file
    file.seek(0);  // Start from beginning
    file.printf("Filetype: Flipper SubGhz Key File\n");
    file.printf("Version: 1\n");
    file.printf("Frequency: %u\n", current_signal_codes.frequency);
    file.printf("Preset: FuriHalSubGhzPresetOok650Async\n");
    file.printf("Protocol: Security+ 2.0\n");
    file.printf("Bit: 62\n");

    // Write Key (packet2) in Flipper format
    file.printf("Key: ");
    for (int i = 7; i >= 0; i--) {
        uint8_t byte = (packet2 >> (i * 8)) & 0xFF;
        file.printf("%02X", byte);
        if (i > 0) file.printf(" ");
    }
    file.printf("\n");

    // Write Secplus_packet_1
    file.printf("Secplus_packet_1: ");
    for (int i = 7; i >= 0; i--) {
        uint8_t byte = (packet1 >> (i * 8)) & 0xFF;
        file.printf("%02X", byte);
        if (i > 0) file.printf(" ");
    }
    file.printf("\n");

    file.close();


    // Update current_signal_codes with new packet values
    current_signal_codes.secplus_packet1 = packet1;
    current_signal_codes.secplus_packet2 = packet2;

    // Update display labels for packets
    char meta1[128], meta2[128];
    snprintf(meta1, sizeof(meta1), "Pk1:0x%012llX", packet1);
    snprintf(meta2, sizeof(meta2), "Pk2:0x%012llX", packet2);
    lv_label_set_text(details_metadata_label1, meta1);
    lv_label_set_text(details_metadata_label2, meta2);

    return true;
}

/**
 * Re-encode and save Security+ 1.0 signal with updated counter
 */
static bool reencode_secplus_v1_signal() {
    if (current_signal_codes.protocol != "Security+ 1.0") {
        return false;
    }


    uint32_t fixed = (current_signal_codes.key >> 32) & 0xFFFFFFFF;
    uint32_t rolling = current_signal_codes.key & 0xFFFFFFFF;

    // Re-encode with updated counter
    SecPlusV1Protocol secplus;
    if (!secplus.encodeFromCodes(fixed, rolling)) {
        return false;
    }

    // Get the incremented counter
    uint32_t new_fixed, new_rolling;
    if (!secplus.getEncodedCodes(new_fixed, new_rolling)) {
        return false;
    }

    // Update the .sub file on SD card
    File file = SD.open(playback_selected_file.c_str(), FILE_WRITE);
    if (!file) {
        return false;
    }

    // Build new 64-bit key
    uint64_t new_key = ((uint64_t)new_fixed << 32) | new_rolling;

    // Write updated .sub file
    file.seek(0);  // Start from beginning
    file.printf("Filetype: Flipper SubGhz Key File\n");
    file.printf("Version: 1\n");
    file.printf("Frequency: %u\n", current_signal_codes.frequency);
    file.printf("Preset: FuriHalSubGhzPresetOok650Async\n");
    file.printf("Protocol: Security+ 1.0\n");
    file.printf("Bit: 42\n");

    // Write Key in Flipper format (8 bytes, space-separated)
    file.printf("Key: ");
    for (int i = 7; i >= 0; i--) {
        uint8_t byte = (new_key >> (i * 8)) & 0xFF;
        file.printf("%02X", byte);
        if (i > 0) file.printf(" ");
    }
    file.printf("\n");

    file.close();


    // Update current_signal_codes with new key
    current_signal_codes.key = new_key;

    return true;
}

/**
 * Transmit the current signal
 * @param increment_counter If true, increment Security+ 2.0/1.0 counter after transmission
 */
static bool transmit_signal(bool increment_counter = true) {
    details_transmitting = true;

    // Change button color to red and update text
    lv_obj_set_style_bg_color(details_send_btn, lv_color_hex(0xFF0000), LV_PART_MAIN);
    lv_label_set_text(details_send_label, "TX...");
    lv_obj_invalidate(details_send_btn);
    lv_refr_now(NULL);
    lv_task_handler();

    // (SubGHz initialized in enter2_2_1)
    bool tx_result = subghz_transmit_file(playback_selected_file.c_str());

    // Auto-increment Security+ 2.0 counter after successful transmission (only if requested)
    if (increment_counter && tx_result && current_signal_codes.protocol == "Security+ 2.0" && current_signal_codes.secplus_valid) {
        current_signal_codes.secplus_rolling++;

        // Update display
        char meta4[128];
        snprintf(meta4, sizeof(meta4), "Cnt:  0x%07X", current_signal_codes.secplus_rolling);
        lv_label_set_text(details_metadata_label4, meta4);

        // Re-encode and save with new counter
        reencode_secplus_signal();

    }
    // Auto-increment Security+ 1.0 counter after successful transmission (only if requested)
    else if (increment_counter && tx_result && current_signal_codes.protocol == "Security+ 1.0") {
        uint32_t old_rolling = current_signal_codes.key & 0xFFFFFFFF;

        // Re-encode and save with incremented counter (encoding increments by 2)
        reencode_secplus_v1_signal();

        uint32_t new_rolling = current_signal_codes.key & 0xFFFFFFFF;

        // Update display to show new rolling code
        char meta2[128];
        snprintf(meta2, sizeof(meta2), "Key: 0x%08X", new_rolling);
        lv_label_set_text(details_metadata_label2, meta2);

    }

    // Restore button color and text
    lv_obj_set_style_bg_color(details_send_btn, lv_color_hex(EMBED_COLOR_FOCUS_ON), LV_PART_MAIN);
    lv_label_set_text(details_send_label, "Send");
    lv_obj_invalidate(details_send_btn);
    lv_refr_now(NULL);

    details_transmitting = false;

    return tx_result;
}

/**
 * Send button event handler
 */
static void send_btn_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_PRESSED) {
        details_long_press_active = false;
    } else if (code == LV_EVENT_LONG_PRESSED) {
        // Long press: continuous transmission until released
        details_long_press_active = true;


        // Keep transmitting while encoder button is held down
        // Note: For RAW signals, rf_sendRaw checks for button press internally
        // which can cause early termination. We loop here to ensure continuous TX.
        int transmission_count = 0;
        bool any_success = false;
        while (digitalRead(ENCODER_KEY) == LOW) {
            transmission_count++;
            bool result = transmit_signal(false);  // false = don't increment counter
            if (result) any_success = true;
        }

        details_long_press_active = false;

    } else if (code == LV_EVENT_CLICKED) {
        // Short press: single transmission
        if (!details_long_press_active) {
            bool result = transmit_signal();
            if (result) {
                prompt_info("  Transmitted!", 1000);
            } else {
                prompt_info("  TX Error", 2000);
            }
        }
    }
}

/**
 * Increment button event handler
 */
static void inc_btn_event_cb(lv_event_t *e) {
    if (e->code == LV_EVENT_CLICKED) {
        if (current_signal_codes.protocol == "Security+ 2.0" && current_signal_codes.secplus_valid) {
            current_signal_codes.secplus_rolling++;

            // Update display (label4 shows counter)
            char meta4[128];
            snprintf(meta4, sizeof(meta4), "Cnt:  0x%07X", current_signal_codes.secplus_rolling);
            lv_label_set_text(details_metadata_label4, meta4);

            // Re-encode the signal with new counter value
            if (reencode_secplus_signal()) {
            } else {
            }
        } else if (current_signal_codes.protocol == "Security+ 1.0") {
            // Extract current fixed and rolling codes
            uint32_t fixed = (current_signal_codes.key >> 32) & 0xFFFFFFFF;
            uint32_t rolling = current_signal_codes.key & 0xFFFFFFFF;

            // Don't modify rolling code - let protocol's +2 do the work
            // Update key (rolling stays the same)
            current_signal_codes.key = ((uint64_t)fixed << 32) | rolling;

            // Re-encode the signal (protocol adds +2, so 0+2 = +2 net)
            if (reencode_secplus_v1_signal()) {
                // Get the actual new rolling code after encoding
                uint32_t actual_rolling = current_signal_codes.key & 0xFFFFFFFF;

                // Update display with the actual encoded value
                char meta2[128];
                snprintf(meta2, sizeof(meta2), "Key: 0x%08X", actual_rolling);
                lv_label_set_text(details_metadata_label2, meta2);

            } else {
            }
        }
    }
}

/**
 * Decrement button event handler
 */
static void dec_btn_event_cb(lv_event_t *e) {
    if (e->code == LV_EVENT_CLICKED) {
        if (current_signal_codes.protocol == "Security+ 2.0" && current_signal_codes.secplus_valid) {
            if (current_signal_codes.secplus_rolling > 0) {
                current_signal_codes.secplus_rolling--;

                // Update display (label4 shows counter)
                char meta4[128];
                snprintf(meta4, sizeof(meta4), "Cnt:  0x%07X", current_signal_codes.secplus_rolling);
                lv_label_set_text(details_metadata_label4, meta4);

                // Re-encode the signal with new counter value
                if (reencode_secplus_signal()) {
                } else {
                }
            }
        } else if (current_signal_codes.protocol == "Security+ 1.0") {
            // Extract current fixed and rolling codes
            uint32_t fixed = (current_signal_codes.key >> 32) & 0xFFFFFFFF;
            uint32_t rolling = current_signal_codes.key & 0xFFFFFFFF;

            if (rolling > 3) {  // Need at least 4 to safely decrement by 4
                // Decrement by 4 to offset protocol's +2, resulting in net -2
                rolling -= 4;

                // Update key with adjusted rolling code
                current_signal_codes.key = ((uint64_t)fixed << 32) | rolling;

                // Re-encode the signal (protocol adds +2, so -4+2 = -2 net)
                if (reencode_secplus_v1_signal()) {
                    // Get the actual new rolling code after encoding
                    uint32_t actual_rolling = current_signal_codes.key & 0xFFFFFFFF;

                    // Update display with the actual encoded value
                    char meta2[128];
                    snprintf(meta2, sizeof(meta2), "Key: 0x%08X", actual_rolling);
                    lv_label_set_text(details_metadata_label2, meta2);

                } else {
                }
            }
        }
    }
}

/**
 * Back button event handler
 */
static void scr2_2_1_back_btn_event_cb(lv_event_t *e) {
    if (e->code == LV_EVENT_CLICKED) {
        exit2_2_1_anim(SCREEN2_2_ID, scr2_2_1_cont);
    }
}

/**
 * Create screen 2.2.1
 */
static void create2_2_1(lv_obj_t *parent) {
    scr2_2_1_cont = create_screen_container(parent);

    // Protocol and frequency line (top)
    details_protocol_label = lv_label_create(scr2_2_1_cont);
    apply_text_color(details_protocol_label);
    lv_obj_set_style_text_font(details_protocol_label, FONT_BOLD_14, LV_PART_MAIN);
    lv_label_set_text(details_protocol_label, "Loading...");
    lv_obj_align(details_protocol_label, LV_ALIGN_TOP_MID, 0, 10);

    // Metadata line 1
    details_metadata_label1 = lv_label_create(scr2_2_1_cont);
    apply_text_color(details_metadata_label1);
    lv_obj_set_style_text_font(details_metadata_label1, FONT_BOLD_14, LV_PART_MAIN);
    lv_label_set_text(details_metadata_label1, "");
    lv_obj_set_width(details_metadata_label1, 300);
    lv_label_set_long_mode(details_metadata_label1, LV_LABEL_LONG_WRAP);
    lv_obj_align(details_metadata_label1, LV_ALIGN_TOP_LEFT, 10, 40);

    // Metadata line 2
    details_metadata_label2 = lv_label_create(scr2_2_1_cont);
    apply_text_color(details_metadata_label2);
    lv_obj_set_style_text_font(details_metadata_label2, FONT_BOLD_14, LV_PART_MAIN);
    lv_label_set_text(details_metadata_label2, "");
    lv_obj_set_width(details_metadata_label2, 300);
    lv_label_set_long_mode(details_metadata_label2, LV_LABEL_LONG_WRAP);
    lv_obj_align(details_metadata_label2, LV_ALIGN_TOP_LEFT, 10, 60);

    // Metadata line 3
    details_metadata_label3 = lv_label_create(scr2_2_1_cont);
    apply_text_color(details_metadata_label3);
    lv_obj_set_style_text_font(details_metadata_label3, FONT_BOLD_14, LV_PART_MAIN);
    lv_label_set_text(details_metadata_label3, "");
    lv_obj_set_width(details_metadata_label3, 300);
    lv_label_set_long_mode(details_metadata_label3, LV_LABEL_LONG_WRAP);
    lv_obj_align(details_metadata_label3, LV_ALIGN_TOP_LEFT, 10, 80);

    // Metadata line 4
    details_metadata_label4 = lv_label_create(scr2_2_1_cont);
    apply_text_color(details_metadata_label4);
    lv_obj_set_style_text_font(details_metadata_label4, FONT_BOLD_14, LV_PART_MAIN);
    lv_label_set_text(details_metadata_label4, "");
    lv_obj_set_width(details_metadata_label4, 300);
    lv_label_set_long_mode(details_metadata_label4, LV_LABEL_LONG_WRAP);
    lv_obj_align(details_metadata_label4, LV_ALIGN_TOP_LEFT, 10, 100);

    // Back button (top left) - Added last for default focus
    scr_back_btn_create(scr2_2_1_cont, scr2_2_1_back_btn_event_cb);

	// Decrement button (right - for rolling codes)
    details_dec_btn = lv_btn_create(scr2_2_1_cont);
    lv_obj_set_size(details_dec_btn, 90, 35);
    lv_obj_align(details_dec_btn, LV_ALIGN_BOTTOM_RIGHT, -10, -10);
    lv_obj_set_style_border_color(details_dec_btn, lv_color_hex(EMBED_COLOR_BORDER), LV_PART_MAIN);
    lv_obj_set_style_border_width(details_dec_btn, 1, LV_PART_MAIN);
    apply_no_shadow(details_dec_btn);
    apply_bg_color(details_dec_btn);
    lv_obj_remove_style(details_dec_btn, NULL, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_pad(details_dec_btn, 2, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_width(details_dec_btn, 2, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_color(details_dec_btn, lv_color_hex(EMBED_COLOR_FOCUS_ON), LV_STATE_FOCUS_KEY);
    lv_obj_add_event_cb(details_dec_btn, dec_btn_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_flag(details_dec_btn, LV_OBJ_FLAG_HIDDEN);  // Hidden by default

    lv_obj_t *dec_label = lv_label_create(details_dec_btn);
    lv_label_set_text(dec_label, "Dec");
    apply_text_color(dec_label);
    lv_obj_set_style_text_font(dec_label, FONT_BOLD_14, LV_PART_MAIN);
    lv_obj_center(dec_label);

    // Increment button (center - for rolling codes)
    details_inc_btn = lv_btn_create(scr2_2_1_cont);
    lv_obj_set_size(details_inc_btn, 90, 35);
    lv_obj_align(details_inc_btn, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_set_style_border_color(details_inc_btn, lv_color_hex(EMBED_COLOR_BORDER), LV_PART_MAIN);
    lv_obj_set_style_border_width(details_inc_btn, 1, LV_PART_MAIN);
    apply_no_shadow(details_inc_btn);
    apply_bg_color(details_inc_btn);
    lv_obj_remove_style(details_inc_btn, NULL, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_pad(details_inc_btn, 2, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_width(details_inc_btn, 2, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_color(details_inc_btn, lv_color_hex(EMBED_COLOR_FOCUS_ON), LV_STATE_FOCUS_KEY);
    lv_obj_add_event_cb(details_inc_btn, inc_btn_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_flag(details_inc_btn, LV_OBJ_FLAG_HIDDEN);  // Hidden by default

    lv_obj_t *inc_label = lv_label_create(details_inc_btn);
    lv_label_set_text(inc_label, "Inc");
    apply_text_color(inc_label);
    lv_obj_set_style_text_font(inc_label, FONT_BOLD_14, LV_PART_MAIN);
    lv_obj_center(inc_label);

    // Send button (left)
    details_send_btn = lv_btn_create(scr2_2_1_cont);
    lv_obj_set_size(details_send_btn, 90, 35);
    lv_obj_align(details_send_btn, LV_ALIGN_BOTTOM_LEFT, 10, -10);
    lv_obj_set_style_border_color(details_send_btn, lv_color_hex(EMBED_COLOR_BORDER), LV_PART_MAIN);
    lv_obj_set_style_border_width(details_send_btn, 1, LV_PART_MAIN);
    apply_no_shadow(details_send_btn);
    apply_bg_color(details_send_btn);
    lv_obj_remove_style(details_send_btn, NULL, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_pad(details_send_btn, 2, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_width(details_send_btn, 2, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_color(details_send_btn, lv_color_hex(EMBED_COLOR_FOCUS_ON), LV_STATE_FOCUS_KEY);
    lv_obj_add_event_cb(details_send_btn, send_btn_event_cb, LV_EVENT_PRESSED, NULL);
    lv_obj_add_event_cb(details_send_btn, send_btn_event_cb, LV_EVENT_LONG_PRESSED, NULL);
    lv_obj_add_event_cb(details_send_btn, send_btn_event_cb, LV_EVENT_CLICKED, NULL);

    details_send_label = lv_label_create(details_send_btn);
    lv_label_set_text(details_send_label, "Send");
    apply_text_color(details_send_label);
    lv_obj_set_style_text_font(details_send_label, FONT_BOLD_14, LV_PART_MAIN);
    lv_obj_center(details_send_label);
}

/**
 * Enter screen 2.2.1
 */
static void enter2_2_1(void) {
    // Initialize SubGHz module if needed
    if (!subghz_is_init()) {
        subghz_init();
    }

    entry2_2_1_anim(scr2_2_1_cont);
    load_signal_details();
    lv_group_set_wrap(lv_group_get_default(), true);
}

/**
 * Exit screen 2.2.1
 */
static void exit2_2_1(void) {
    lv_group_set_wrap(lv_group_get_default(), false);
}

/**
 * Destroy screen 2.2.1
 */
static void destroy2_2_1(void) {
    if (scr2_2_1_cont) {
        lv_obj_del(scr2_2_1_cont);
        scr2_2_1_cont = NULL;
    }
}

scr_lifecycle_t screen2_2_1 = {
    .create = create2_2_1,
    .entry = enter2_2_1,
    .exit = exit2_2_1,
    .destroy = destroy2_2_1,
};
#endif

//************************************[ screen 2.3 ]************************************** SubGHz Scan/Record
#if 1
lv_obj_t *scr2_3_cont;
lv_obj_t *scan_status_label;
lv_obj_t *scan_freq_btn;
lv_obj_t *scan_preset_btn;
lv_obj_t *scan_thresh_btn;
lv_obj_t *scan_rssi_bar;
lv_obj_t *scan_rssi_label;
lv_obj_t *scan_signal_label;
lv_obj_t *scan_btn_start;
lv_obj_t *scan_btn_replay;
lv_obj_t *scan_btn_save;
lv_obj_t *scan_btn_auto;
lv_obj_t *scan_led;
lv_timer_t *scan_update_timer = NULL;
bool scan_is_active = false;
bool scan_auto_save = false;  // Auto-save mode - automatically saves each captured signal
int scan_consecutive_captures = 0;  // Track consecutive captures to detect off-frequency lock
int scan_saved_count = 0;  // Track total number of saved files in auto mode
unsigned long scan_led_on_time = 0;  // Track when LED was turned on for timed fade
bool scan_protocol_detected = false;  // Pause auto-save after protocol detected
unsigned long scan_protocol_detect_time = 0;  // Time when protocol was detected
int scan_last_encoder_pos = 0;  // Track encoder position for frequency tuning

// Scan configuration
int scan_range_mode = 3;  // 0=300-348, 1=387-464, 2=779-928, 3=Full, 4=Single freq, 5=Custom
float scan_single_freq = 433.92f;
float scan_custom_start_freq = 300.00f;
float scan_custom_end_freq = 928.00f;
float scan_step_size_khz = 25.0f;  // Step size in kHz (default 25 kHz)
int scan_rssi_threshold = -65;
int freq_selector_mode = 0;  // Track what we're selecting: 0=single freq, 1=custom start, 2=custom end

// Incremental scanning state
bool scan_in_progress = false;
int scan_current_idx = 0;
int scan_range_start = 0;
int scan_range_end = 55;  // Last index of hardcoded list (56 frequencies, indices 0-55)
int scan_attempt = 0;
struct {
    float freq;
    int rssi;
} scan_best_freqs[5];  // FREQ_SCAN_MAX_TRIES = 5

// Custom range scanning state
float scan_custom_current_freq = 0.0f;
bool scan_custom_mode = false;

// Auto-resume scanning state (for all modes)
unsigned long scan_weak_signal_start = 0;  // When signal dropped below threshold
bool scan_signal_was_strong = false;  // Track if we've seen a strong signal
const unsigned long SCAN_WEAK_SIGNAL_TIMEOUT = 5000;  // 5 seconds in milliseconds
unsigned long scan_listen_start_time = 0;  // When we started listening on current frequency
const unsigned long SCAN_MAX_LISTEN_TIMEOUT = 5000;  // Maximum time to stay on one frequency (5 seconds)

void entry2_3_anim(lv_obj_t *obj) { entry1_anim(obj); }
void exit2_3_anim(int user_data, lv_obj_t *obj) { exit1_anim(user_data, obj); }

static void scan_update_timer_event(lv_timer_t *t)
{
    if (!scan_is_active) {
        return;
    }

    // Handle encoder-based frequency tuning when active
    extern int indev_encoder_pos;
    int encoder_diff = indev_encoder_pos - scan_last_encoder_pos;

    if (encoder_diff != 0) {
        // Find the closest frequency in the list to the current frequency
        ScanRecordStatus *status = subghz_get_scan_status();
        if (status && status->listening) {
            float current_freq = status->frequency;
            float new_freq = current_freq;
            bool freq_changed = false;

            if (scan_custom_mode) {
                // Custom range mode: use step-based navigation
                // Note: encoder_diff direction is inverted (minus = up)
                float step_mhz = scan_step_size_khz / 1000.0f;
                int steps = -encoder_diff;  // Invert to match expected direction

                // Calculate new frequency
                new_freq = current_freq + (steps * step_mhz);

                // Clamp to custom range bounds
                if (new_freq < scan_custom_start_freq) {
                    new_freq = scan_custom_start_freq;
                } else if (new_freq > scan_custom_end_freq) {
                    new_freq = scan_custom_end_freq;
                }

                // Ensure frequency is valid (handle CC1101 band gaps)
                if (!subghz_is_freq_valid(new_freq)) {
                    // Find next valid frequency in the direction we're moving
                    if (steps > 0) {
                        new_freq = subghz_next_valid_freq(new_freq - step_mhz, scan_step_size_khz);
                    } else {
                        // For backward movement, we need to find previous valid freq
                        // Start from current and go backwards
                        float test_freq = current_freq - step_mhz;
                        while (test_freq >= scan_custom_start_freq && !subghz_is_freq_valid(test_freq)) {
                            test_freq -= step_mhz;
                        }
                        new_freq = test_freq >= scan_custom_start_freq ? test_freq : current_freq;
                    }
                }

                freq_changed = (fabs(new_freq - current_freq) > 0.001f);

            } else {
                // Predefined list mode: use index-based navigation (with custom frequencies)
                // Find current frequency index in the list
                std::vector<float> freq_list_data = rf_get_combined_frequency_list();
                int freq_count = freq_list_data.size();
                int current_idx = -1;
                float min_diff = 999.0f;
                for (int i = 0; i < freq_count; i++) {
                    float diff = abs(freq_list_data[i] - current_freq);
                    if (diff < min_diff) {
                        min_diff = diff;
                        current_idx = i;
                    }
                }

                // Move to next/prev frequency (inverted direction)
                if (current_idx >= 0) {
                    int new_idx = current_idx - encoder_diff;  // Inverted: minus instead of plus
                    // Clamp to valid range
                    if (new_idx < 0) new_idx = 0;
                    if (new_idx >= freq_count) new_idx = freq_count - 1;

                    if (new_idx != current_idx) {
                        new_freq = freq_list_data[new_idx];
                        freq_changed = true;
                    }
                }
            }

            // Apply frequency change if needed
            if (freq_changed) {

                // Stop current listening
                subghz_stop_scan_record();

                // Update frequency and restart listening
                RawRecordingStatus *raw_status = subghz_get_status();
                if (raw_status) {
                    raw_status->frequency = new_freq;
                }
                scan_single_freq = new_freq;  // Update for single freq mode

                // Update custom current freq if in custom mode
                if (scan_custom_mode) {
                    scan_custom_current_freq = new_freq;
                }

                // Restart listening on new frequency
                if (subghz_start_scan_record(-1)) {
                    // Reset consecutive counter when manually tuning
                    scan_consecutive_captures = 0;
                    // Restart dwell timer for new frequency
                    scan_listen_start_time = millis();
                }
            }
        }

        scan_last_encoder_pos = indev_encoder_pos;
    }

    // Handle incremental frequency scanning
    if (scan_in_progress) {
        // Scan one frequency per timer tick
        if (scan_attempt < 5) {  // FREQ_SCAN_MAX_TRIES
            float check_freq;

            if (scan_custom_mode) {
                // Custom range scanning with configurable step
                check_freq = scan_custom_current_freq;

                // Check if we've reached the end - wrap around to start
                if (check_freq > scan_custom_end_freq || check_freq == 0.0f) {
                    // Align start frequency to step boundary to prevent drift
                    scan_custom_current_freq = subghz_align_freq_to_step(scan_custom_start_freq, scan_step_size_khz);

                    // Ensure start frequency is valid
                    if (!subghz_is_freq_valid(scan_custom_current_freq)) {
                        scan_custom_current_freq = subghz_next_valid_freq(scan_custom_current_freq - (scan_step_size_khz / 1000.0f), scan_step_size_khz);
                    }
                    check_freq = scan_custom_current_freq;
                }
            } else {
                // Predefined frequency list scanning (with custom frequencies)
                std::vector<float> freq_list_data = rf_get_combined_frequency_list();

                // Wrap around if we reach the end
                if (scan_current_idx > scan_range_end) {
                    scan_current_idx = scan_range_start;
                }

                check_freq = freq_list_data[scan_current_idx];
            }

            // Update status for display
            ScanRecordStatus *status = subghz_get_scan_status();
            if (status) {
                status->scanning = true;
                status->frequency = check_freq;
            }

            // Configure radio and check RSSI
            if (rf_initModule("rx", check_freq)) {
                delay(5);  // Let radio settle
                int rssi = ELECHOUSE_cc1101.getRssi();

                if (status) {
                    status->rssi = rssi;
                }

                // Update UI directly during scan
                int rssi_percent = map(rssi, -100, -30, 0, 100);
                rssi_percent = constrain(rssi_percent, 0, 100);
                lv_bar_set_value(scan_rssi_bar, rssi_percent, LV_ANIM_OFF);
                lv_label_set_text_fmt(scan_rssi_label, "RSSI: %d dBm", rssi);
                lv_label_set_text_fmt(scan_status_label, "Scan: %.4f MHz", check_freq);

                if (rssi > scan_rssi_threshold) {
                    // Strong signal found - stop scanning and start listening
                    scan_in_progress = false;
                    scan_consecutive_captures = 0;
                    scan_signal_was_strong = true;  // Mark that we found a strong signal
                    scan_weak_signal_start = 0;  // Reset weak signal timer
                    scan_listen_start_time = millis();  // Start maximum dwell timer

                    // Start listening mode on this frequency
                    RawRecordingStatus *raw_status = subghz_get_status();
                    if (raw_status) {
                        raw_status->frequency = check_freq;
                    }

                    subghz_start_scan_record(-1);  // -1 = no scan, just listen

                    return;  // Exit timer callback early
                }
            }

            if (scan_custom_mode) {
                // Move to next frequency in custom range, skipping invalid gaps
                scan_custom_current_freq = subghz_next_valid_freq(scan_custom_current_freq, scan_step_size_khz);
            } else {
                scan_current_idx++;
            }

            // Refresh display after each frequency check to keep UI responsive
            lv_timer_handler();
            lv_refr_now(NULL);  // Force immediate display refresh
        } else {
            // Scanning complete - find best frequency
            scan_in_progress = false;
            rf_deinitModule();

            float best_freq = 0.0f;
            int best_rssi = -100;

            if (scan_attempt > 0) {
                int best_idx = 0;
                for (int i = 1; i < scan_attempt; i++) {
                    if (scan_best_freqs[i].rssi > scan_best_freqs[best_idx].rssi) {
                        best_idx = i;
                    }
                }
                best_freq = scan_best_freqs[best_idx].freq;
                best_rssi = scan_best_freqs[best_idx].rssi;
            }

            // Now start listening mode
            if (best_freq > 0.0f) {
                subghz_start_scan_record(-1);  // -1 = no scan, just listen at the frequency we found
                scan_listen_start_time = millis();  // Start maximum dwell timer
                scan_signal_was_strong = (best_rssi > scan_rssi_threshold);  // Track if signal is strong
                ScanRecordStatus *status = subghz_get_scan_status();
                if (status) {
                    status->frequency = best_freq;
                    status->rssi = best_rssi;
                    status->scanning = false;
                    status->listening = true;
                }
            } else {
                // No frequency found, stop
                scan_is_active = false;
                lv_label_set_text(lv_obj_get_child(scan_btn_start, 0), "Start");
                lv_label_set_text(scan_status_label, "No signal found");
            }
        }
    }

    // Update display
    ScanRecordStatus *status = subghz_get_scan_status();
    if (status) {
        // Update RSSI bar (map -100 to -30 dBm to 0-100%)
        int rssi_percent = map(status->rssi, -100, -30, 0, 100);
        rssi_percent = constrain(rssi_percent, 0, 100);
        lv_bar_set_value(scan_rssi_bar, rssi_percent, LV_ANIM_OFF);

        // Update RSSI label
        lv_label_set_text_fmt(scan_rssi_label, "RSSI: %d dBm", status->rssi);

        // Update status and signal counter
        if (status->scanning) {
            lv_label_set_text_fmt(scan_status_label, "Scan: %.4f MHz", status->frequency);
            // In auto mode, show saved count; otherwise show signal count
            if (scan_auto_save) {
                lv_label_set_text_fmt(scan_signal_label, "Auto-Saved: %d", scan_saved_count);
            } else {
                lv_label_set_text(scan_signal_label, "Sig: 0");
            }
            lv_led_off(scan_led);
        } else if (status->listening) {
            lv_label_set_text_fmt(scan_status_label, "Listen: %.4f MHz", status->frequency);
            // In auto mode, show saved count; otherwise show signal count
            if (scan_auto_save) {
                lv_label_set_text_fmt(scan_signal_label, "Auto-Saved: %d", scan_saved_count);
            } else {
                lv_label_set_text_fmt(scan_signal_label, "Sig: %d", status->signals);
            }

            if (status->signalDetected) {
                lv_led_on(scan_led);
                scan_led_on_time = millis();  // Record when LED was turned on
                status->signalDetected = false;  // Reset flag

                // Auto-save if enabled AND RSSI is above threshold AND not paused
                if (scan_auto_save && status->rssi > scan_rssi_threshold && !scan_protocol_detected) {
                    scan_consecutive_captures++;

                    String protocol_name;
                    String filepath = subghz_save_last_signal(nullptr, &protocol_name);
                    if (filepath.length() > 0) {
                        scan_saved_count++;  // Increment saved file counter

                        // Display protocol notification
                        if (!protocol_name.isEmpty()) {
                            String msg = "Saved: " + protocol_name;
                            prompt_info(msg.c_str(), 2000);  // Show for 2 seconds
                        }

                        // Check if this was a decoded protocol (not RAW)
                        File check = SD.open(filepath);
                        if (check) {
                            String firstLine = check.readStringUntil('\n');
                            check.close();

                            if (firstLine.indexOf("Key File") >= 0) {
                                // Protocol detected! Pause capture until RSSI drops
                                scan_protocol_detected = true;
                                scan_protocol_detect_time = millis();
                            }
                        }
                    } else {
                    }

                    // If we've captured 5 signals in a row, might be on adjacent frequency
                    // Resume scanning to find the exact frequency
                    if (scan_consecutive_captures >= 5 && scan_range_mode != 4) {
                        subghz_stop_scan_record();
                        scan_in_progress = true;
                        scan_attempt = 0;
                        scan_consecutive_captures = 0;
                        scan_listen_start_time = 0;
                    }
                } else if (scan_auto_save && status->rssi <= scan_rssi_threshold) {
                    // Only resume scanning if we're in auto mode and not in single-frequency mode
                    if (scan_range_mode != 4) {  // Not single frequency mode
                        // Stop listening and resume scanning
                        subghz_stop_scan_record();
                        scan_in_progress = true;
                        scan_attempt = 0;
                        scan_consecutive_captures = 0;
                        scan_listen_start_time = 0;
                    }
                } else if (!scan_auto_save && status->rssi > scan_rssi_threshold) {
                    // Manual mode: Show protocol pop-up when signal detected
                    RfCodes *last_signal = subghz_get_last_signal();
                    if (last_signal != nullptr && !last_signal->protocol.isEmpty()) {
                        String msg = "Detected: " + last_signal->protocol;
                        prompt_info(msg.c_str(), 2000);  // Show for 2 seconds
                    }
                }
            } else {
                // Fade LED after 200ms (4 timer ticks at 50ms each)
                if (scan_led_on_time > 0 && (millis() - scan_led_on_time) > 200) {
                    lv_led_off(scan_led);
                    scan_led_on_time = 0;
                }
            }

            // Resume auto-save if protocol was detected but RSSI has dropped
            if (scan_protocol_detected && status->rssi <= scan_rssi_threshold) {
                // RSSI dropped below threshold - resume capture for next signal
                scan_protocol_detected = false;
                scan_consecutive_captures = 0;
            }

            // Auto-resume scanning if signal has been weak for too long
            // (Only applies when we're listening after finding a strong signal)
            if (scan_signal_was_strong && scan_range_mode != 4) {  // Not in single-frequency mode
                if (status->rssi <= scan_rssi_threshold) {
                    // Signal is weak
                    if (scan_weak_signal_start == 0) {
                        // Just dropped below threshold - start timer
                        scan_weak_signal_start = millis();
                    } else if (millis() - scan_weak_signal_start >= SCAN_WEAK_SIGNAL_TIMEOUT) {
                        // Signal has been weak for 5+ seconds - resume scanning
                        subghz_stop_scan_record();
                        scan_in_progress = true;
                        scan_attempt = 0;
                        scan_consecutive_captures = 0;
                        scan_signal_was_strong = false;
                        scan_weak_signal_start = 0;
                    }
                } else {
                    // Signal is strong again - reset timer
                    scan_weak_signal_start = 0;
                }
            }

            // Maximum dwell time timeout - prevents getting stuck on strong carrier signals
            // that aren't being decoded (e.g., NXDN, DMR, P25, etc.)
            // Applies to Scan List and Frequency Range modes (not single-frequency mode)
            // Also applies to Custom range mode (mode 5)
            if (scan_range_mode != 4) {  // Not in single-frequency mode
                // Check if we've been on this frequency too long
                unsigned long listen_duration = (scan_listen_start_time > 0) ?
                    (millis() - scan_listen_start_time) : millis();

                if (listen_duration >= SCAN_MAX_LISTEN_TIMEOUT) {
                    // Been on this frequency for 5+ seconds - resume scanning
                    subghz_stop_scan_record();

                    // Skip to next frequency to avoid re-checking the same one
                    if (scan_custom_mode) {
                        // Custom range mode: advance to next frequency
                        scan_custom_current_freq = subghz_next_valid_freq(scan_custom_current_freq, scan_step_size_khz);
                    } else {
                        // Predefined list mode: advance index
                        scan_current_idx++;
                    }

                    scan_in_progress = true;
                    scan_attempt = 0;
                    scan_consecutive_captures = 0;
                    scan_signal_was_strong = false;
                    scan_weak_signal_start = 0;
                    scan_listen_start_time = 0;
                }
            }
        }
    }
}

static void scan_freq_btn_event(lv_event_t *e)
{
    if (e->code == LV_EVENT_CLICKED) {
        exit2_3_anim(SCREEN2_3_1_ID, scr2_3_cont);
    }
}

static void scan_preset_btn_event(lv_event_t *e)
{
    if (e->code == LV_EVENT_CLICKED) {
        // Cycle through all presets: AM270, AM650, FM238, FM476
        const uint8_t presets[] = {
            SUBGHZ_PRESET_OOK270,      // AM270
            SUBGHZ_PRESET_OOK650,      // AM650
            SUBGHZ_PRESET_2FSK_DEV238, // FM238
            SUBGHZ_PRESET_2FSK_DEV476  // FM476
        };
        const int num_presets = sizeof(presets) / sizeof(presets[0]);

        // Find current preset index
        int current_idx = 0;
        for (int i = 0; i < num_presets; i++) {
            if (subghz_current_preset == presets[i]) {
                current_idx = i;
                break;
            }
        }

        // Move to next preset (wrap around)
        current_idx = (current_idx + 1) % num_presets;
        subghz_current_preset = presets[current_idx];

        // Update button label
        lv_label_set_text(lv_obj_get_child(scan_preset_btn, 0), get_preset_short_name(subghz_current_preset));


        // Save to config
        const char* mod_str = "am650";
        if (subghz_current_preset == SUBGHZ_PRESET_OOK270) mod_str = "am270";
        else if (subghz_current_preset == SUBGHZ_PRESET_OOK650) mod_str = "am650";
        else if (subghz_current_preset == SUBGHZ_PRESET_2FSK_DEV238) mod_str = "fm238";
        else if (subghz_current_preset == SUBGHZ_PRESET_2FSK_DEV476) mod_str = "fm476";
        config_save_modulation(mod_str);
    }
}

static void scan_thresh_btn_event(lv_event_t *e)
{
    if (e->code == LV_EVENT_CLICKED) {
        // Cycle through common thresholds: -75, -70, -65, -60, -55, -50
        const int thresholds[] = {-75, -70, -65, -60, -55, -50};
        const int num_thresh = sizeof(thresholds) / sizeof(thresholds[0]);

        // Find current threshold index
        int current_idx = 2;  // Default to -65
        for (int i = 0; i < num_thresh; i++) {
            if (scan_rssi_threshold == thresholds[i]) {
                current_idx = i;
                break;
            }
        }

        // Move to next threshold (wrap around)
        current_idx = (current_idx + 1) % num_thresh;
        scan_rssi_threshold = thresholds[current_idx];

        // Update button label
        lv_label_set_text_fmt(lv_obj_get_child(scan_thresh_btn, 0), "%d dBm", scan_rssi_threshold);

        // Save to config based on current scan mode
        const char* type_str;
        char range_str[16];

        if (scan_range_mode == 4) {  // Single frequency
            type_str = "single";
            snprintf(range_str, sizeof(range_str), "%.3f", scan_single_freq);
        } else if (scan_range_mode == 5) {  // Custom range
            type_str = "custom";
            snprintf(range_str, sizeof(range_str), "%.0f-%.0f", scan_custom_start_freq, scan_custom_end_freq);
        } else {  // Band mode
            type_str = "band";
            if (scan_range_mode == 0) strcpy(range_str, "low");
            else if (scan_range_mode == 1) strcpy(range_str, "mid");
            else if (scan_range_mode == 2) strcpy(range_str, "high");
            else strcpy(range_str, "full");
        }
        config_save_scan_settings(type_str, range_str, scan_rssi_threshold);
    }
}

static void scan_btn_start_event(lv_event_t *e)
{
    if (e->code == LV_EVENT_CLICKED) {
        if (!scan_is_active) {
            // Start scan/record
            // (SubGHz initialized in enter2_3)

            scan_is_active = true;
            lv_label_set_text(lv_obj_get_child(scan_btn_start, 0), "Stop");

            // Change button to red with black text when active
            lv_obj_set_style_bg_color(scan_btn_start, lv_palette_main(LV_PALETTE_RED), LV_PART_MAIN);
            lv_obj_set_style_text_color(lv_obj_get_child(scan_btn_start, 0), lv_color_black(), LV_PART_MAIN);

            // Initialize encoder position for frequency tuning
            extern int indev_encoder_pos;
            scan_last_encoder_pos = indev_encoder_pos;

            // Disable encoder navigation while active (encoder will be used for frequency tuning)
            extern volatile bool indev_encoder_enabled;
            indev_encoder_enabled = false;

            // Reset auto-resume state
            scan_signal_was_strong = false;
            scan_weak_signal_start = 0;
            scan_listen_start_time = 0;
            scan_protocol_detected = false;  // Reset protocol detection pause flag
            scan_protocol_detect_time = 0;

            // Determine scanning range based on mode
            if (scan_range_mode == 4) {
                // Single frequency mode - skip scanning, go straight to listening
                scan_in_progress = false;

                // Set the desired frequency in subghz_status BEFORE calling subghz_start_scan_record
                // This ensures the frequency is picked up correctly at line 787 in peri_subghz.cpp
                RawRecordingStatus *raw_status = subghz_get_status();
                if (raw_status) {
                    raw_status->frequency = scan_single_freq;
                }

                if (subghz_start_scan_record(-1)) {  // -1 = no scan, just listen at previously set frequency
                } else {
                    scan_is_active = false;
                    lv_label_set_text(lv_obj_get_child(scan_btn_start, 0), "Start");
                }
            } else if (scan_range_mode == 5) {
                // Custom range mode - use incremental scanning

                // Validate custom range
                if (scan_custom_start_freq >= scan_custom_end_freq) {
                    prompt_info("Error: Start freq must be < End freq", 2000);
                    scan_is_active = false;
                    lv_label_set_text(lv_obj_get_child(scan_btn_start, 0), "Start");
                    apply_bg_color(scan_btn_start);
                    apply_text_color(lv_obj_get_child(scan_btn_start, 0));

                    // Re-enable encoder navigation
                    extern volatile bool indev_encoder_enabled;
                    indev_encoder_enabled = true;
                    return;
                }

                // Initialize custom range scanning
                scan_custom_mode = true;
                // Align start frequency to step boundary to prevent drift
                scan_custom_current_freq = subghz_align_freq_to_step(scan_custom_start_freq, scan_step_size_khz);

                // Ensure start frequency is valid
                if (!subghz_is_freq_valid(scan_custom_current_freq)) {
                    scan_custom_current_freq = subghz_next_valid_freq(scan_custom_current_freq - (scan_step_size_khz / 1000.0f), scan_step_size_khz);
                }

                scan_attempt = 0;
                scan_in_progress = true;

                // Initialize best frequencies
                for (int i = 0; i < 5; i++) {
                    scan_best_freqs[i].freq = scan_custom_start_freq;
                    scan_best_freqs[i].rssi = -100;
                }

                lv_label_set_text(scan_status_label, "Custom Scan: Starting...");
            } else {
                // Range scanning mode - initialize incremental scan
                // Get combined frequency list size for Full range mode
                int combined_freq_count = rf_get_combined_frequency_count();

                const int range_limits_local[][2] = {
                    {0, 22},   // 300-348 MHz (23 frequencies)
                    {23, 46},  // 387-464 MHz (24 frequencies)
                    {47, 55},  // 779-928 MHz (9 frequencies)
                    {0, combined_freq_count - 1}    // Full range (includes custom frequencies)
                };

                int range_idx = scan_range_mode;
                scan_range_start = range_limits_local[range_idx][0];
                scan_range_end = range_limits_local[range_idx][1];
                scan_current_idx = scan_range_start;
                scan_attempt = 0;
                scan_in_progress = true;
                scan_custom_mode = false;  // Using predefined list

                // Initialize best frequencies
                for (int i = 0; i < 5; i++) {
                    scan_best_freqs[i].freq = 433.92f;
                    scan_best_freqs[i].rssi = -100;
                }

                lv_label_set_text(scan_status_label, "Scan: Starting...");
            }
        } else {
            // Stop scan/record
            scan_in_progress = false;
            scan_custom_mode = false;
            subghz_stop_scan_record();
            scan_is_active = false;
            scan_auto_save = false;
            scan_consecutive_captures = 0;
            scan_signal_was_strong = false;
            scan_weak_signal_start = 0;
            scan_listen_start_time = 0;
            scan_protocol_detected = false;  // Reset protocol detection pause flag
            scan_protocol_detect_time = 0;
            lv_label_set_text(lv_obj_get_child(scan_btn_start, 0), "Start");
            lv_label_set_text(lv_obj_get_child(scan_btn_auto, 0), "Auto");

            // Restore button colors to default
            apply_bg_color(scan_btn_start);
            apply_text_color(lv_obj_get_child(scan_btn_start, 0));
            apply_bg_color(scan_btn_auto);
            apply_text_color(lv_obj_get_child(scan_btn_auto, 0));

            // Re-enable encoder navigation
            extern volatile bool indev_encoder_enabled;
            indev_encoder_enabled = true;

            lv_label_set_text(scan_status_label, "Status: Stopped");
            lv_led_off(scan_led);
        }
    }
}

static void scan_btn_replay_event(lv_event_t *e)
{
    if (e->code == LV_EVENT_CLICKED) {

        // Debug: Check scan status
        ScanRecordStatus *status = subghz_get_scan_status();
        if (status) {
        }

        // Get the last captured signal
        RfCodes *signal = subghz_get_last_signal();
        if (signal == nullptr) {
            prompt_info("  No signal\n  to replay", 1500);
            return;
        }


        // Stop listening while we transmit
        bool was_active = scan_is_active;
        if (scan_is_active) {
            subghz_stop_scan_record();
        }


        // Transmit the signal
        rf_sendCommand(*signal);

        prompt_info("  Signal\n  transmitted", 1500);

        // Resume listening if it was active
        if (was_active) {
            delay(100);  // Brief delay before resuming
            if (scan_range_mode == 4) {
                // Single frequency mode
                RawRecordingStatus *raw_status = subghz_get_status();
                if (raw_status) {
                    raw_status->frequency = scan_single_freq;
                }
                subghz_start_scan_record(-1);
            }
        }
    }
}

static void scan_btn_save_event(lv_event_t *e)
{
    if (e->code == LV_EVENT_CLICKED) {

        // Check if there's a signal to save
        ScanRecordStatus *status = subghz_get_scan_status();
        if (status == nullptr || status->signals == 0) {
            prompt_info("  No signal\n  to save", 1500);
            return;
        }

        // Save the last captured signal
        String protocol_name;
        String filepath = subghz_save_last_signal(nullptr, &protocol_name);

        if (filepath.length() > 0) {

            // Display protocol notification
            if (!protocol_name.isEmpty()) {
                String msg = "Saved: " + protocol_name;
                prompt_info(msg.c_str(), 2000);  // Show for 2 seconds
            }

            // Extract just the filename for display
            int lastSlash = filepath.lastIndexOf('/');
            String filename = (lastSlash >= 0) ? filepath.substring(lastSlash + 1) : filepath;

            // Show success message with filename (truncate if too long)
            if (filename.length() > 16) {
                filename = filename.substring(0, 13) + "...";
            }
            String message = "  Saved:\n  " + filename;
            prompt_info(message.c_str(), 2000);
        } else {
            prompt_info("  Save failed\n  Check SD card", 2000);
        }
    }
}

static void scan_btn_auto_event(lv_event_t *e)
{
    if (e->code == LV_EVENT_CLICKED) {
        if (!scan_is_active) {
            // Start in auto-save mode
            scan_auto_save = true;
            scan_saved_count = 0;  // Reset saved counter for new auto session

            // Trigger the same start logic as normal start button
            lv_event_send(scan_btn_start, LV_EVENT_CLICKED, NULL);

            // Update button to red with black text to show Auto is active
            if (scan_is_active) {
                lv_obj_set_style_bg_color(scan_btn_auto, lv_palette_main(LV_PALETTE_RED), LV_PART_MAIN);
                lv_obj_set_style_text_color(lv_obj_get_child(scan_btn_auto, 0), lv_color_black(), LV_PART_MAIN);
            }
        } else {
            // Stop auto mode
            scan_auto_save = false;

            // Stop scanning (this will also restore button colors)
            lv_event_send(scan_btn_start, LV_EVENT_CLICKED, NULL);
        }
    }
}

static void scan_btn_back_event(lv_event_t *e)
{
    if (e->code == LV_EVENT_CLICKED) {
        if (scan_is_active) {
            subghz_stop_scan_record();
            scan_is_active = false;
            scan_auto_save = false;
            scan_consecutive_captures = 0;

            // Restore button colors to default
            apply_bg_color(scan_btn_start);
            apply_text_color(lv_obj_get_child(scan_btn_start, 0));
            apply_bg_color(scan_btn_auto);
            apply_text_color(lv_obj_get_child(scan_btn_auto, 0));

            // Re-enable encoder navigation
            extern volatile bool indev_encoder_enabled;
            indev_encoder_enabled = true;
        }
        exit2_3_anim(SCREEN2_ID, scr2_3_cont);
    }
}

static const char* get_range_text(int mode) {
    switch(mode) {
        case 0: return "300-348";
        case 1: return "387-464";
        case 2: return "779-928";
        case 3: return "Scan List";
        case 4: return "Single";
        case 5: return "Custom";
        default: return "Scan List";
    }
}

static void get_freq_display_text(char* buffer, size_t buf_size) {
    switch(scan_range_mode) {
        case 0: // 300-348 MHz
            snprintf(buffer, buf_size, "300-348");
            break;
        case 1: // 387-464 MHz
            snprintf(buffer, buf_size, "387-464");
            break;
        case 2: // 779-928 MHz
            snprintf(buffer, buf_size, "779-928");
            break;
        case 3: // Full range
            snprintf(buffer, buf_size, "Scan List");
            break;
        case 4: // Single frequency
            snprintf(buffer, buf_size, "%.3f", scan_single_freq);
            break;
        case 5: // Custom range
            snprintf(buffer, buf_size, "%.1f-%.1f", scan_custom_start_freq, scan_custom_end_freq);
            break;
        default:
            snprintf(buffer, buf_size, "Full");
            break;
    }
}

static void create2_3(lv_obj_t *parent)
{
    scr2_3_cont = create_screen_container(parent);

    // Title
    lv_obj_t *label = lv_label_create(scr2_3_cont);
    apply_text_color(label);
    lv_obj_set_style_text_font(label, FONT_BOLD_18, LV_PART_MAIN);
    lv_label_set_text(label, "Scan / Record");
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 10);

    // Status label (left side, under back button area)
    scan_status_label = lv_label_create(scr2_3_cont);
    apply_text_color(scan_status_label);
    lv_obj_set_style_text_font(scan_status_label, FONT_BOLD_14, LV_PART_MAIN);
    lv_label_set_text(scan_status_label, "Status: Ready");
    lv_obj_align(scan_status_label, LV_ALIGN_TOP_LEFT, 35, 46);

    // Signal count label (right side, same row as status)
    scan_signal_label = lv_label_create(scr2_3_cont);
    apply_text_color(scan_signal_label);
    lv_obj_set_style_text_font(scan_signal_label, FONT_BOLD_14, LV_PART_MAIN);
    lv_label_set_text(scan_signal_label, "Sig: 0");
    lv_obj_align(scan_signal_label, LV_ALIGN_TOP_RIGHT, -10, 46);

    // Frequency selector button (top row, left)
    scan_freq_btn = lv_btn_create(scr2_3_cont);
    lv_obj_set_size(scan_freq_btn, 90, 28);
    lv_obj_align(scan_freq_btn, LV_ALIGN_TOP_LEFT, 10, 70);
    lv_obj_set_style_border_color(scan_freq_btn, lv_color_hex(EMBED_COLOR_BORDER), LV_PART_MAIN);
    lv_obj_set_style_border_width(scan_freq_btn, 1, LV_PART_MAIN);
    apply_no_shadow(scan_freq_btn);
    apply_bg_color(scan_freq_btn);
    lv_obj_remove_style(scan_freq_btn, NULL, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_pad(scan_freq_btn, 2, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_width(scan_freq_btn, 2, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_color(scan_freq_btn, lv_color_hex(EMBED_COLOR_FOCUS_ON), LV_STATE_FOCUS_KEY);
    lv_obj_add_event_cb(scan_freq_btn, scan_freq_btn_event, LV_EVENT_CLICKED, NULL);
    lv_obj_t *freq_label = lv_label_create(scan_freq_btn);
    apply_text_color(freq_label);
    lv_obj_set_style_text_font(freq_label, FONT_BOLD_14, LV_PART_MAIN);
    char freq_text[32];
    get_freq_display_text(freq_text, sizeof(freq_text));
    lv_label_set_text(freq_label, freq_text);
    lv_obj_center(freq_label);
    lv_group_add_obj(lv_group_get_default(), scan_freq_btn);

    // Preset selector button (top row, center)
    scan_preset_btn = lv_btn_create(scr2_3_cont);
    lv_obj_set_size(scan_preset_btn, 75, 28);
    lv_obj_align(scan_preset_btn, LV_ALIGN_TOP_MID, 0, 70);
    lv_obj_set_style_border_color(scan_preset_btn, lv_color_hex(EMBED_COLOR_BORDER), LV_PART_MAIN);
    lv_obj_set_style_border_width(scan_preset_btn, 1, LV_PART_MAIN);
    apply_no_shadow(scan_preset_btn);
    apply_bg_color(scan_preset_btn);
    lv_obj_remove_style(scan_preset_btn, NULL, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_pad(scan_preset_btn, 2, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_width(scan_preset_btn, 2, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_color(scan_preset_btn, lv_color_hex(EMBED_COLOR_FOCUS_ON), LV_STATE_FOCUS_KEY);
    lv_obj_add_event_cb(scan_preset_btn, scan_preset_btn_event, LV_EVENT_CLICKED, NULL);
    lv_obj_t *preset_label = lv_label_create(scan_preset_btn);
    apply_text_color(preset_label);
    lv_obj_set_style_text_font(preset_label, FONT_BOLD_14, LV_PART_MAIN);
    lv_label_set_text(preset_label, get_preset_short_name(subghz_current_preset));
    lv_obj_center(preset_label);
    lv_group_add_obj(lv_group_get_default(), scan_preset_btn);

    // Threshold selector button (top row, right)
    scan_thresh_btn = lv_btn_create(scr2_3_cont);
    lv_obj_set_size(scan_thresh_btn, 70, 28);
    lv_obj_align(scan_thresh_btn, LV_ALIGN_TOP_RIGHT, -10, 70);
    lv_obj_set_style_border_color(scan_thresh_btn, lv_color_hex(EMBED_COLOR_BORDER), LV_PART_MAIN);
    lv_obj_set_style_border_width(scan_thresh_btn, 1, LV_PART_MAIN);
    apply_no_shadow(scan_thresh_btn);
    apply_bg_color(scan_thresh_btn);
    lv_obj_remove_style(scan_thresh_btn, NULL, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_pad(scan_thresh_btn, 2, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_width(scan_thresh_btn, 2, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_color(scan_thresh_btn, lv_color_hex(EMBED_COLOR_FOCUS_ON), LV_STATE_FOCUS_KEY);
    lv_obj_add_event_cb(scan_thresh_btn, scan_thresh_btn_event, LV_EVENT_CLICKED, NULL);
    lv_obj_t *thresh_label = lv_label_create(scan_thresh_btn);
    apply_text_color(thresh_label);
    lv_obj_set_style_text_font(thresh_label, FONT_BOLD_14, LV_PART_MAIN);
    lv_label_set_text_fmt(thresh_label, "%d dBm", scan_rssi_threshold);
    lv_obj_center(thresh_label);
    lv_group_add_obj(lv_group_get_default(), scan_thresh_btn);

    // LED indicator (middle row, right side)
    scan_led = lv_led_create(scr2_3_cont);
    lv_obj_align(scan_led, LV_ALIGN_TOP_RIGHT, -10, 105);
    lv_obj_set_size(scan_led, 15, 15);
    lv_led_set_color(scan_led, lv_palette_main(LV_PALETTE_RED));
    lv_led_off(scan_led);

    // RSSI Bar (middle row, wide)
    scan_rssi_bar = lv_bar_create(scr2_3_cont);
    lv_obj_set_size(scan_rssi_bar, 265, 18);
    lv_obj_align(scan_rssi_bar, LV_ALIGN_TOP_MID, 0, 105);
    lv_bar_set_range(scan_rssi_bar, 0, 100);
    lv_bar_set_value(scan_rssi_bar, 0, LV_ANIM_OFF);

    // RSSI Label (overlaid on top of bar, centered)
    scan_rssi_label = lv_label_create(scr2_3_cont);
    apply_text_color(scan_rssi_label);
    lv_obj_set_style_text_font(scan_rssi_label, FONT_BOLD_14, LV_PART_MAIN);
    lv_label_set_text(scan_rssi_label, "RSSI: --- dBm");
    lv_obj_align(scan_rssi_label, LV_ALIGN_TOP_MID, 0, 107);  // Centered vertically on bar
    // Add semi-transparent background for better readability
    lv_obj_set_style_bg_opa(scan_rssi_label, LV_OPA_70, LV_PART_MAIN);
    apply_bg_color(scan_rssi_label);
    lv_obj_set_style_pad_left(scan_rssi_label, 3, LV_PART_MAIN);
    lv_obj_set_style_pad_right(scan_rssi_label, 3, LV_PART_MAIN);

    // Bottom row buttons - 4 buttons: [Save][Replay][Start][Auto]
    // Adjust button width to fit 4 buttons with spacing
    // NOTE: Order of creation matters for navigation group order!
    const int btn_width = 60;
    const int btn_spacing = 10;
    const int total_width = (btn_width * 4) + (btn_spacing * 3);
    const int start_x = -(total_width / 2) + 15;  // Shift all buttons 15px to the right

    // Auto button (4th position from left) - Created BEFORE Start for correct nav order
    scan_btn_auto = lv_btn_create(scr2_3_cont);
    lv_obj_set_size(scan_btn_auto, btn_width, 32);
    lv_obj_align(scan_btn_auto, LV_ALIGN_BOTTOM_MID, start_x + (btn_width + btn_spacing) * 3, -10);
    lv_obj_set_style_border_color(scan_btn_auto, lv_color_hex(EMBED_COLOR_BORDER), LV_PART_MAIN);
    lv_obj_set_style_border_width(scan_btn_auto, 1, LV_PART_MAIN);
    apply_no_shadow(scan_btn_auto);
    apply_bg_color(scan_btn_auto);
    lv_obj_remove_style(scan_btn_auto, NULL, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_pad(scan_btn_auto, 2, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_width(scan_btn_auto, 2, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_color(scan_btn_auto, lv_color_hex(EMBED_COLOR_FOCUS_ON), LV_STATE_FOCUS_KEY);
    lv_obj_add_event_cb(scan_btn_auto, scan_btn_auto_event, LV_EVENT_CLICKED, NULL);
    lv_obj_t *auto_label = lv_label_create(scan_btn_auto);
    apply_text_color(auto_label);
    lv_obj_set_style_text_font(auto_label, FONT_BOLD_14, LV_PART_MAIN);
    lv_label_set_text(auto_label, "Auto");
    lv_obj_center(auto_label);
    lv_group_add_obj(lv_group_get_default(), scan_btn_auto);

    // Start button (3rd position from left)
    scan_btn_start = lv_btn_create(scr2_3_cont);
    lv_obj_set_size(scan_btn_start, btn_width, 32);
    lv_obj_align(scan_btn_start, LV_ALIGN_BOTTOM_MID, start_x + (btn_width + btn_spacing) * 2, -10);
    lv_obj_set_style_border_color(scan_btn_start, lv_color_hex(EMBED_COLOR_BORDER), LV_PART_MAIN);
    lv_obj_set_style_border_width(scan_btn_start, 1, LV_PART_MAIN);
    apply_no_shadow(scan_btn_start);
    apply_bg_color(scan_btn_start);
    lv_obj_remove_style(scan_btn_start, NULL, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_pad(scan_btn_start, 2, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_width(scan_btn_start, 2, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_color(scan_btn_start, lv_color_hex(EMBED_COLOR_FOCUS_ON), LV_STATE_FOCUS_KEY);
    lv_obj_add_event_cb(scan_btn_start, scan_btn_start_event, LV_EVENT_CLICKED, NULL);
    lv_obj_t *start_label = lv_label_create(scan_btn_start);
    apply_text_color(start_label);
    lv_obj_set_style_text_font(start_label, FONT_BOLD_14, LV_PART_MAIN);
    lv_label_set_text(start_label, "Start");
    lv_obj_center(start_label);
    lv_group_add_obj(lv_group_get_default(), scan_btn_start);

    // Replay button (2nd position from left)
    scan_btn_replay = lv_btn_create(scr2_3_cont);
    lv_obj_set_size(scan_btn_replay, btn_width, 32);
    lv_obj_align(scan_btn_replay, LV_ALIGN_BOTTOM_MID, start_x + (btn_width + btn_spacing) * 1, -10);
    lv_obj_set_style_border_color(scan_btn_replay, lv_color_hex(EMBED_COLOR_BORDER), LV_PART_MAIN);
    lv_obj_set_style_border_width(scan_btn_replay, 1, LV_PART_MAIN);
    apply_no_shadow(scan_btn_replay);
    apply_bg_color(scan_btn_replay);
    lv_obj_remove_style(scan_btn_replay, NULL, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_pad(scan_btn_replay, 2, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_width(scan_btn_replay, 2, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_color(scan_btn_replay, lv_color_hex(EMBED_COLOR_FOCUS_ON), LV_STATE_FOCUS_KEY);
    lv_obj_add_event_cb(scan_btn_replay, scan_btn_replay_event, LV_EVENT_CLICKED, NULL);
    lv_obj_t *replay_label = lv_label_create(scan_btn_replay);
    apply_text_color(replay_label);
    lv_obj_set_style_text_font(replay_label, FONT_BOLD_14, LV_PART_MAIN);
    lv_label_set_text(replay_label, "Replay");
    lv_obj_center(replay_label);
    lv_group_add_obj(lv_group_get_default(), scan_btn_replay);

    // Save button (1st position from left)
    scan_btn_save = lv_btn_create(scr2_3_cont);
    lv_obj_set_size(scan_btn_save, btn_width, 32);
    lv_obj_align(scan_btn_save, LV_ALIGN_BOTTOM_MID, start_x, -10);
    lv_obj_set_style_border_color(scan_btn_save, lv_color_hex(EMBED_COLOR_BORDER), LV_PART_MAIN);
    lv_obj_set_style_border_width(scan_btn_save, 1, LV_PART_MAIN);
    apply_no_shadow(scan_btn_save);
    apply_bg_color(scan_btn_save);
    lv_obj_remove_style(scan_btn_save, NULL, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_pad(scan_btn_save, 2, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_width(scan_btn_save, 2, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_color(scan_btn_save, lv_color_hex(EMBED_COLOR_FOCUS_ON), LV_STATE_FOCUS_KEY);
    lv_obj_add_event_cb(scan_btn_save, scan_btn_save_event, LV_EVENT_CLICKED, NULL);
    lv_obj_t *save_label = lv_label_create(scan_btn_save);
    apply_text_color(save_label);
    lv_obj_set_style_text_font(save_label, FONT_BOLD_14, LV_PART_MAIN);
    lv_label_set_text(save_label, "Save");
    lv_obj_center(save_label);
    lv_group_add_obj(lv_group_get_default(), scan_btn_save);

    // Back button (using standard "<" button in top left) - Added last for default focus
    scr_back_btn_create(scr2_3_cont, scan_btn_back_event);

    // Create update timer (50ms - scans one frequency per tick, keeps UI responsive)
    scan_update_timer = lv_timer_create(scan_update_timer_event, 50, NULL);
    lv_timer_pause(scan_update_timer);
}

static void enter2_3(void)
{
    // Initialize SubGHz module if needed
    if (!subghz_is_init()) {
        subghz_init();
    }

    entry2_3_anim(scr2_3_cont);
    scan_is_active = false;
    lv_led_off(scan_led);
    lv_timer_resume(scan_update_timer);

    // Initialize encoder position for frequency tuning
    extern int indev_encoder_pos;
    scan_last_encoder_pos = indev_encoder_pos;

    // Load settings from config
    scan_rssi_threshold = g_config.subghz.scan.threshold;

    // Load scan type and range
    if (strcmp(g_config.subghz.scan.type, "single") == 0) {
        scan_range_mode = 4;  // Single frequency mode
        scan_single_freq = atof(g_config.subghz.scan.range);
    } else if (strcmp(g_config.subghz.scan.type, "custom") == 0) {
        scan_range_mode = 5;  // Custom range mode
        // Parse "start-end" format
        char range_copy[16];
        strlcpy(range_copy, g_config.subghz.scan.range, sizeof(range_copy));
        char* dash = strchr(range_copy, '-');
        if (dash) {
            *dash = '\0';
            scan_custom_start_freq = atof(range_copy);
            scan_custom_end_freq = atof(dash + 1);
        }
    } else {  // "band"
        if (strcmp(g_config.subghz.scan.range, "low") == 0) {
            scan_range_mode = 0;  // 300-348 MHz
        } else if (strcmp(g_config.subghz.scan.range, "mid") == 0) {
            scan_range_mode = 1;  // 387-464 MHz
        } else if (strcmp(g_config.subghz.scan.range, "high") == 0) {
            scan_range_mode = 2;  // 779-928 MHz
        } else {  // "full"
            scan_range_mode = 3;  // Full scan
        }
    }

    // Load modulation preset
    if (strcmp(g_config.subghz.modulation, "am270") == 0) {
        subghz_current_preset = SUBGHZ_PRESET_OOK270;
    } else if (strcmp(g_config.subghz.modulation, "am650") == 0) {
        subghz_current_preset = SUBGHZ_PRESET_OOK650;
    } else if (strcmp(g_config.subghz.modulation, "fm238") == 0) {
        subghz_current_preset = SUBGHZ_PRESET_2FSK_DEV238;
    } else if (strcmp(g_config.subghz.modulation, "fm476") == 0) {
        subghz_current_preset = SUBGHZ_PRESET_2FSK_DEV476;
    }

    // Update UI to reflect loaded settings
    lv_label_set_text(lv_obj_get_child(scan_preset_btn, 0), get_preset_short_name(subghz_current_preset));
    lv_label_set_text_fmt(lv_obj_get_child(scan_thresh_btn, 0), "%d dBm", scan_rssi_threshold);

    // Update frequency button label with current selection
    char freq_text[32];
    get_freq_display_text(freq_text, sizeof(freq_text));
    lv_label_set_text(lv_obj_get_child(scan_freq_btn, 0), freq_text);

    lv_group_set_wrap(lv_group_get_default(), true);
}

static void exit2_3(void)
{
    lv_group_set_wrap(lv_group_get_default(), false);

    // Stop scanning if active
    if (scan_is_active) {
        subghz_stop_scan_record();
        scan_is_active = false;
        scan_in_progress = false;
        scan_custom_mode = false;
        scan_signal_was_strong = false;
        scan_weak_signal_start = 0;
        scan_listen_start_time = 0;

        // Re-enable encoder navigation
        extern volatile bool indev_encoder_enabled;
        indev_encoder_enabled = true;
    }
}

static void destroy2_3(void)
{
    // Delete timer
    if (scan_update_timer) {
        lv_timer_del(scan_update_timer);
        scan_update_timer = NULL;
    }

    if (scan_is_active) {
        subghz_stop_scan_record();
        scan_is_active = false;
        scan_in_progress = false;
        scan_custom_mode = false;
        scan_signal_was_strong = false;
        scan_weak_signal_start = 0;
        scan_listen_start_time = 0;

        // Re-enable encoder navigation
        extern volatile bool indev_encoder_enabled;
        indev_encoder_enabled = true;
    }

    if (scr2_3_cont) {
        lv_obj_del(scr2_3_cont);
        scr2_3_cont = NULL;
    }
}

scr_lifecycle_t screen2_3 = {
    .create = create2_3,
    .entry = enter2_3,
    .exit = exit2_3,
    .destroy = destroy2_3,
};
#endif

//************************************[ screen 2.3.1 ]************************************** Scan/Record Frequency Mode Selector
#if 1
lv_obj_t *scr2_3_1_cont;
lv_obj_t *freq_mode_list;

void entry2_3_1_anim(lv_obj_t *obj) { entry1_anim(obj); }
void exit2_3_1_anim(int user_data, lv_obj_t *obj) { exit1_anim(user_data, obj); }

static void freq_mode_list_event(lv_event_t *e)
{
    if (e->code == LV_EVENT_CLICKED) {
        int mode = (int)(intptr_t)lv_event_get_user_data(e);


        switch(mode) {
            case 0: // Single frequency
                freq_selector_mode = 0;  // Set to single freq mode
                exit2_3_1_anim(SCREEN2_3_1_1_ID, scr2_3_1_cont);
                break;
            case 1: // Scan List (Full Range)
                scan_range_mode = 3;  // Set to Full mode (all 57 frequencies)
                config_save_scan_settings("band", "full", scan_rssi_threshold);
                exit2_3_1_anim(SCREEN2_3_ID, scr2_3_1_cont);
                break;
            case 2: // Custom
                exit2_3_1_anim(SCREEN2_3_1_3_ID, scr2_3_1_cont);
                break;
        }
    }
}

static void scr2_3_1_back_btn_event_cb(lv_event_t *e)
{
    if (e->code == LV_EVENT_CLICKED) {
        exit2_3_1_anim(SCREEN2_3_ID, scr2_3_1_cont);
    }
}

static void create2_3_1(lv_obj_t *parent)
{
    scr2_3_1_cont = create_screen_container(parent);

    // Title
    lv_obj_t *label = lv_label_create(scr2_3_1_cont);
    apply_text_color(label);
    lv_obj_set_style_text_font(label, FONT_BOLD_16, LV_PART_MAIN);
    lv_label_set_text(label, "Frequency Mode");
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 10);

    // Mode list
    freq_mode_list = lv_list_create(scr2_3_1_cont);
    lv_obj_set_size(freq_mode_list, LV_HOR_RES - 10, 125);
    lv_obj_align(freq_mode_list, LV_ALIGN_BOTTOM_MID, 0, -5);
    apply_bg_color(freq_mode_list);
    lv_obj_set_style_pad_top(freq_mode_list, 2, LV_PART_MAIN);
    lv_obj_set_style_pad_row(freq_mode_list, 2, LV_PART_MAIN);
    apply_no_radius(freq_mode_list);
    apply_no_border(freq_mode_list);
    apply_no_shadow(freq_mode_list);

    const char* modes[] = {"Single Frequency", "Scan List", "Custom Range"};
    for (int i = 0; i < 3; i++) {
        lv_obj_t *item = lv_list_add_btn(freq_mode_list, NULL, modes[i]);
        lv_obj_set_style_text_font(item, FONT_BOLD_14, LV_PART_MAIN);
        apply_text_color(item);
        apply_bg_color(item);
        lv_obj_add_event_cb(item, freq_mode_list_event, LV_EVENT_CLICKED, (void*)(intptr_t)i);
        lv_group_add_obj(lv_group_get_default(), item);

        lv_obj_remove_style(item, NULL, LV_STATE_FOCUS_KEY);
        lv_obj_set_style_outline_color(item, lv_color_hex(EMBED_COLOR_FOCUS_ON), LV_STATE_FOCUS_KEY);
        lv_obj_set_style_outline_width(item, 2, LV_STATE_FOCUS_KEY);
    }

    // Back button
    scr_back_btn_create(scr2_3_1_cont, scr2_3_1_back_btn_event_cb);
}

static void enter2_3_1(void)
{
    entry2_3_1_anim(scr2_3_1_cont);
    lv_group_set_wrap(lv_group_get_default(), true);
}

static void exit2_3_1(void)
{
    lv_group_set_wrap(lv_group_get_default(), false);
}

static void destroy2_3_1(void)
{
    if (scr2_3_1_cont) {
        lv_obj_del(scr2_3_1_cont);
        scr2_3_1_cont = NULL;
    }
}

scr_lifecycle_t screen2_3_1 = {
    .create = create2_3_1,
    .entry = enter2_3_1,
    .exit = exit2_3_1,
    .destroy = destroy2_3_1,
};
#endif

//************************************[ screen 2.3.1.1 ]********************************** Single Frequency Selector
#if 1
lv_obj_t *scr2_3_1_1_cont;
lv_obj_t *scan_single_freq_list;

void entry2_3_1_1_anim(lv_obj_t *obj) { entry1_anim(obj); }
void exit2_3_1_1_anim(int user_data, lv_obj_t *obj) { exit1_anim(user_data, obj); }

static void scan_single_freq_list_event(lv_event_t *e)
{
    if (e->code == LV_EVENT_CLICKED) {
        int idx = (int)(intptr_t)lv_event_get_user_data(e);
        std::vector<float> freq_list_data = rf_get_combined_frequency_list();
        float selected_freq = freq_list_data[idx];

        if (freq_selector_mode == 0) {
            // Single frequency mode
            float selected_freq = freq_list_data[idx];
            scan_single_freq = selected_freq;
            scan_range_mode = 4;  // Set to Single mode

            // Save to config
            char freq_str[16];
            snprintf(freq_str, sizeof(freq_str), "%.3f", scan_single_freq);
            config_save_scan_settings("single", freq_str, scan_rssi_threshold);

            exit2_3_1_1_anim(SCREEN2_3_ID, scr2_3_1_1_cont);
        } else if (freq_selector_mode == 1) {
            // Custom start frequency
            float selected_freq = subghz_mhz_list[idx];
            scan_custom_start_freq = selected_freq;
            exit2_3_1_1_anim(SCREEN2_3_1_3_ID, scr2_3_1_1_cont);  // Back to custom menu
        } else if (freq_selector_mode == 2) {
            // Custom end frequency
            float selected_freq = subghz_mhz_list[idx];
            scan_custom_end_freq = selected_freq;
            scan_range_mode = 5;  // Set to Custom mode
            exit2_3_1_1_anim(SCREEN2_3_1_3_ID, scr2_3_1_1_cont);  // Back to Custom Range screen
        }
    }
}

static void scr2_3_1_1_back_btn_event_cb(lv_event_t *e)
{
    if (e->code == LV_EVENT_CLICKED) {
        if (freq_selector_mode == 0) {
            exit2_3_1_1_anim(SCREEN2_3_1_ID, scr2_3_1_1_cont);  // Back to mode selector
        } else {
            exit2_3_1_1_anim(SCREEN2_3_1_3_ID, scr2_3_1_1_cont);  // Back to custom menu
        }
    }
}

static void create2_3_1_1(lv_obj_t *parent)
{
    scr2_3_1_1_cont = create_screen_container(parent);

    // Title
    lv_obj_t *label = lv_label_create(scr2_3_1_1_cont);
    apply_text_color(label);
    lv_obj_set_style_text_font(label, FONT_BOLD_16, LV_PART_MAIN);
    const char* title = (freq_selector_mode == 0) ? "Select Frequency" :
                        (freq_selector_mode == 1) ? "Start Frequency" : "End Frequency";
    lv_label_set_text(label, title);
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 10);

    // Frequency list
    scan_single_freq_list = lv_list_create(scr2_3_1_1_cont);
    lv_obj_set_size(scan_single_freq_list, LV_HOR_RES - 10, 125);
    lv_obj_align(scan_single_freq_list, LV_ALIGN_BOTTOM_MID, 0, -5);
    apply_bg_color(scan_single_freq_list);
    lv_obj_set_style_pad_top(scan_single_freq_list, 2, LV_PART_MAIN);
    lv_obj_set_style_pad_row(scan_single_freq_list, 2, LV_PART_MAIN);
    apply_no_radius(scan_single_freq_list);
    apply_no_border(scan_single_freq_list);
    apply_no_shadow(scan_single_freq_list);

    // Add all frequencies to the list (hardcoded + custom from config)
    lv_obj_t *selected_item = NULL;
    if (freq_selector_mode == 0){
        std::vector<float> freq_list_data = rf_get_combined_frequency_list();
    	for (size_t i = 0; i < freq_list_data.size(); i++) {
            char freq_str[32];
            snprintf(freq_str, sizeof(freq_str), " %.3f MHz", freq_list_data[i]);

            lv_obj_t *item = lv_list_add_btn(scan_single_freq_list, NULL, freq_str);
            lv_obj_set_style_text_font(item, FONT_BOLD_14, LV_PART_MAIN);
            apply_text_color(item);
            apply_bg_color(item);
            lv_obj_add_event_cb(item, scan_single_freq_list_event, LV_EVENT_CLICKED, (void*)(intptr_t)i);
            lv_group_add_obj(lv_group_get_default(), item);

            // Highlight currently selected frequency based on mode
            bool is_selected = false;
            if (freq_selector_mode == 0 && scan_range_mode == 4) {
                is_selected = fabs(freq_list_data[i] - scan_single_freq) < 0.01f;
            }
            if (is_selected) {
                lv_obj_set_style_text_color(item, lv_color_hex(0x00FF00), LV_PART_MAIN);
                selected_item = item;  // Remember this item to focus on it
            }

            lv_obj_remove_style(item, NULL, LV_STATE_FOCUS_KEY);
            lv_obj_set_style_outline_color(item, lv_color_hex(EMBED_COLOR_FOCUS_ON), LV_STATE_FOCUS_KEY);
            lv_obj_set_style_outline_width(item, 2, LV_STATE_FOCUS_KEY);
        }
	}else{
        for (int i = 0; i < subghz_mhz_count; i++) {
            char freq_str[32];
            snprintf(freq_str, sizeof(freq_str), " %.3f MHz", subghz_mhz_list[i]);

            lv_obj_t *item = lv_list_add_btn(scan_single_freq_list, NULL, freq_str);
            lv_obj_set_style_text_font(item, FONT_BOLD_14, LV_PART_MAIN);
            apply_text_color(item);
            apply_bg_color(item);
            lv_obj_add_event_cb(item, scan_single_freq_list_event, LV_EVENT_CLICKED, (void*)(intptr_t)i);
            lv_group_add_obj(lv_group_get_default(), item);

            // Highlight currently selected frequency based on mode
            bool is_selected = false;
            if (freq_selector_mode == 0 && scan_range_mode == 4) {
                is_selected = fabs(subghz_mhz_list[i] - scan_single_freq) < 0.01f;
            } else if (freq_selector_mode == 1) {
                is_selected = fabs(subghz_mhz_list[i] - scan_custom_start_freq) < 0.01f;
            } else if (freq_selector_mode == 2) {
                is_selected = fabs(subghz_mhz_list[i] - scan_custom_end_freq) < 0.01f;
            }
            if (is_selected) {
                lv_obj_set_style_text_color(item, lv_color_hex(0x00FF00), LV_PART_MAIN);
                selected_item = item;  // Remember this item to focus on it
            }

            lv_obj_remove_style(item, NULL, LV_STATE_FOCUS_KEY);
            lv_obj_set_style_outline_color(item, lv_color_hex(EMBED_COLOR_FOCUS_ON), LV_STATE_FOCUS_KEY);
            lv_obj_set_style_outline_width(item, 2, LV_STATE_FOCUS_KEY);
        }
	}

    // Scroll to and focus on the previously selected frequency
    if (selected_item) {
        lv_obj_scroll_to_view(selected_item, LV_ANIM_ON);
        lv_group_focus_obj(selected_item);
    }

    // Back button
    scr_back_btn_create(scr2_3_1_1_cont, scr2_3_1_1_back_btn_event_cb);
}

static void enter2_3_1_1(void)
{
    entry2_3_1_1_anim(scr2_3_1_1_cont);
    lv_group_set_wrap(lv_group_get_default(), true);
}

static void exit2_3_1_1(void)
{
    lv_group_set_wrap(lv_group_get_default(), false);
}

static void destroy2_3_1_1(void)
{
    if (scr2_3_1_1_cont) {
        lv_obj_del(scr2_3_1_1_cont);
        scr2_3_1_1_cont = NULL;
    }
}

scr_lifecycle_t screen2_3_1_1 = {
    .create = create2_3_1_1,
    .entry = enter2_3_1_1,
    .exit = exit2_3_1_1,
    .destroy = destroy2_3_1_1,
};
#endif

//************************************[ screen 2.3.1.2 ]********************************** Range Selector
#if 1
lv_obj_t *scr2_3_1_2_cont;
lv_obj_t *scan_range_list;

void entry2_3_1_2_anim(lv_obj_t *obj) { entry1_anim(obj); }
void exit2_3_1_2_anim(int user_data, lv_obj_t *obj) { exit1_anim(user_data, obj); }

static void scan_range_list_event(lv_event_t *e)
{
    if (e->code == LV_EVENT_CLICKED) {
        int range_idx = (int)(intptr_t)lv_event_get_user_data(e);

        // Update selected range
        scan_range_mode = range_idx;  // 0=300-348, 1=387-464, 2=779-928, 3=Full

        // Save to config
        const char* range_str = "full";
        if (range_idx == 0) range_str = "low";
        else if (range_idx == 1) range_str = "mid";
        else if (range_idx == 2) range_str = "high";
        config_save_scan_settings("band", range_str, scan_rssi_threshold);

        // Go back to Scan/Record screen
        exit2_3_1_2_anim(SCREEN2_3_ID, scr2_3_1_2_cont);
    }
}

static void scr2_3_1_2_back_btn_event_cb(lv_event_t *e)
{
    if (e->code == LV_EVENT_CLICKED) {
        exit2_3_1_2_anim(SCREEN2_3_1_ID, scr2_3_1_2_cont);
    }
}

static void create2_3_1_2(lv_obj_t *parent)
{
    scr2_3_1_2_cont = create_screen_container(parent);

    // Title
    lv_obj_t *label = lv_label_create(scr2_3_1_2_cont);
    apply_text_color(label);
    lv_obj_set_style_text_font(label, FONT_BOLD_16, LV_PART_MAIN);
    lv_label_set_text(label, "Select Range");
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 10);

    // Range list
    scan_range_list = lv_list_create(scr2_3_1_2_cont);
    lv_obj_set_size(scan_range_list, LV_HOR_RES - 10, 125);
    lv_obj_align(scan_range_list, LV_ALIGN_BOTTOM_MID, 0, -5);
    apply_bg_color(scan_range_list);
    lv_obj_set_style_pad_top(scan_range_list, 2, LV_PART_MAIN);
    lv_obj_set_style_pad_row(scan_range_list, 2, LV_PART_MAIN);
    apply_no_radius(scan_range_list);
    apply_no_border(scan_range_list);
    apply_no_shadow(scan_range_list);

    const char* ranges[] = {"300-348 MHz", "387-464 MHz", "779-928 MHz", "Full Range"};
    for (int i = 0; i < 4; i++) {
        lv_obj_t *item = lv_list_add_btn(scan_range_list, NULL, ranges[i]);
        lv_obj_set_style_text_font(item, FONT_BOLD_14, LV_PART_MAIN);
        apply_text_color(item);
        apply_bg_color(item);
        lv_obj_add_event_cb(item, scan_range_list_event, LV_EVENT_CLICKED, (void*)(intptr_t)i);
        lv_group_add_obj(lv_group_get_default(), item);

        // Highlight currently selected range
        if (scan_range_mode == i) {
            lv_obj_set_style_text_color(item, lv_color_hex(0x00FF00), LV_PART_MAIN);
        }

        lv_obj_remove_style(item, NULL, LV_STATE_FOCUS_KEY);
        lv_obj_set_style_outline_color(item, lv_color_hex(EMBED_COLOR_FOCUS_ON), LV_STATE_FOCUS_KEY);
        lv_obj_set_style_outline_width(item, 2, LV_STATE_FOCUS_KEY);
    }

    // Back button
    scr_back_btn_create(scr2_3_1_2_cont, scr2_3_1_2_back_btn_event_cb);
}

static void enter2_3_1_2(void)
{
    entry2_3_1_2_anim(scr2_3_1_2_cont);
    lv_group_set_wrap(lv_group_get_default(), true);
}

static void exit2_3_1_2(void)
{
    lv_group_set_wrap(lv_group_get_default(), false);
}

static void destroy2_3_1_2(void)
{
    if (scr2_3_1_2_cont) {
        lv_obj_del(scr2_3_1_2_cont);
        scr2_3_1_2_cont = NULL;
    }
}

scr_lifecycle_t screen2_3_1_2 = {
    .create = create2_3_1_2,
    .entry = enter2_3_1_2,
    .exit = exit2_3_1_2,
    .destroy = destroy2_3_1_2,
};
#endif

//************************************[ screen 2.3.1.3 ]********************************** Custom Frequency Selector
#if 1
lv_obj_t *scr2_3_1_3_cont;
lv_obj_t *custom_freq_list;

void entry2_3_1_3_anim(lv_obj_t *obj) { entry1_anim(obj); }
void exit2_3_1_3_anim(int user_data, lv_obj_t *obj) { exit1_anim(user_data, obj); }

static void custom_freq_list_event(lv_event_t *e)
{
    if (e->code == LV_EVENT_CLICKED) {
        int idx = (int)(intptr_t)lv_event_get_user_data(e);

        if (idx == 0) {
            // Navigate to start frequency selector (reuse single freq screen pattern)
            freq_selector_mode = 1;  // Set to custom start mode
            exit2_3_1_3_anim(SCREEN2_3_1_1_ID, scr2_3_1_3_cont);
        } else if (idx == 1) {
            // Navigate to end frequency selector
            freq_selector_mode = 2;  // Set to custom end mode
            exit2_3_1_3_anim(SCREEN2_3_1_1_ID, scr2_3_1_3_cont);
        } else if (idx == 2) {
            // Navigate to step size selector
            exit2_3_1_3_anim(SCREEN2_3_1_4_ID, scr2_3_1_3_cont);
        }
    }
}

static void scr2_3_1_3_back_btn_event_cb(lv_event_t *e)
{
    if (e->code == LV_EVENT_CLICKED) {
        // Save custom range settings when navigating back
        char range_str[32];
        snprintf(range_str, sizeof(range_str), "%.0f-%.0f", scan_custom_start_freq, scan_custom_end_freq);
        config_save_scan_settings("custom", range_str, scan_rssi_threshold);

        exit2_3_1_3_anim(SCREEN2_3_ID, scr2_3_1_3_cont);
    }
}

static void create2_3_1_3(lv_obj_t *parent)
{
    scr2_3_1_3_cont = create_screen_container(parent);

    // Title
    lv_obj_t *label = lv_label_create(scr2_3_1_3_cont);
    apply_text_color(label);
    lv_obj_set_style_text_font(label, FONT_BOLD_16, LV_PART_MAIN);
    lv_label_set_text(label, "Custom Range");
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 10);

    // Custom frequency options list
    custom_freq_list = lv_list_create(scr2_3_1_3_cont);
    lv_obj_set_size(custom_freq_list, LV_HOR_RES - 10, 125);
    lv_obj_align(custom_freq_list, LV_ALIGN_BOTTOM_MID, 0, -5);
    apply_bg_color(custom_freq_list);
    lv_obj_set_style_pad_top(custom_freq_list, 2, LV_PART_MAIN);
    lv_obj_set_style_pad_row(custom_freq_list, 2, LV_PART_MAIN);
    apply_no_radius(custom_freq_list);
    apply_no_border(custom_freq_list);
    apply_no_shadow(custom_freq_list);

    char start_freq_str[64];
    char end_freq_str[64];
    char step_size_str[64];
    snprintf(start_freq_str, sizeof(start_freq_str), "Start Freq: %.3f MHz", scan_custom_start_freq);
    snprintf(end_freq_str, sizeof(end_freq_str), "End Freq: %.3f MHz", scan_custom_end_freq);
    snprintf(step_size_str, sizeof(step_size_str), "Step: %.2f kHz", scan_step_size_khz);

    const char* options[] = {start_freq_str, end_freq_str, step_size_str};
    for (int i = 0; i < 3; i++) {
        lv_obj_t *item = lv_list_add_btn(custom_freq_list, NULL, options[i]);
        lv_obj_set_style_text_font(item, FONT_BOLD_14, LV_PART_MAIN);
        apply_text_color(item);
        apply_bg_color(item);
        lv_obj_add_event_cb(item, custom_freq_list_event, LV_EVENT_CLICKED, (void*)(intptr_t)i);
        lv_group_add_obj(lv_group_get_default(), item);

        lv_obj_remove_style(item, NULL, LV_STATE_FOCUS_KEY);
        lv_obj_set_style_outline_color(item, lv_color_hex(EMBED_COLOR_FOCUS_ON), LV_STATE_FOCUS_KEY);
        lv_obj_set_style_outline_width(item, 2, LV_STATE_FOCUS_KEY);
    }

    // Back button
    scr_back_btn_create(scr2_3_1_3_cont, scr2_3_1_3_back_btn_event_cb);
}

static void enter2_3_1_3(void)
{
    entry2_3_1_3_anim(scr2_3_1_3_cont);
    lv_group_set_wrap(lv_group_get_default(), true);
}

static void exit2_3_1_3(void)
{
    lv_group_set_wrap(lv_group_get_default(), false);
}

static void destroy2_3_1_3(void)
{
    if (scr2_3_1_3_cont) {
        lv_obj_del(scr2_3_1_3_cont);
        scr2_3_1_3_cont = NULL;
    }
}

scr_lifecycle_t screen2_3_1_3 = {
    .create = create2_3_1_3,
    .entry = enter2_3_1_3,
    .exit = exit2_3_1_3,
    .destroy = destroy2_3_1_3,
};
#endif

//************************************[ screen 2.3.1.4 ]********************************** Step Size Selector
#if 1
lv_obj_t *scr2_3_1_4_cont;
lv_obj_t *step_size_list;

// Available step sizes in kHz
const float step_sizes_khz[] = {0.1f, 1.0f, 5.0f, 10.0f, 12.5f, 25.0f, 50.0f};
const char* step_size_labels[] = {"0.1 kHz", "1 kHz", "5 kHz", "10 kHz", "12.5 kHz", "25 kHz", "50 kHz"};
const int num_step_sizes = 8;

void entry2_3_1_4_anim(lv_obj_t *obj) { entry1_anim(obj); }
void exit2_3_1_4_anim(int user_data, lv_obj_t *obj) { exit1_anim(user_data, obj); }

static void step_size_list_event(lv_event_t *e)
{
    if (e->code == LV_EVENT_CLICKED) {
        int idx = (int)(intptr_t)lv_event_get_user_data(e);
        scan_step_size_khz = step_sizes_khz[idx];
        scan_range_mode = 5;  // Set to Custom mode
        exit2_3_1_4_anim(SCREEN2_3_1_3_ID, scr2_3_1_4_cont);  // Back to Custom Range screen
    }
}

static void scr2_3_1_4_back_btn_event_cb(lv_event_t *e)
{
    if (e->code == LV_EVENT_CLICKED) {
        exit2_3_1_4_anim(SCREEN2_3_1_3_ID, scr2_3_1_4_cont);  // Back to custom range menu
    }
}

static void create2_3_1_4(lv_obj_t *parent)
{
    scr2_3_1_4_cont = create_screen_container(parent);

    // Title
    lv_obj_t *label = lv_label_create(scr2_3_1_4_cont);
    apply_text_color(label);
    lv_obj_set_style_text_font(label, FONT_BOLD_16, LV_PART_MAIN);
    lv_label_set_text(label, "Select Step Size");
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 10);

    // Step size list
    step_size_list = lv_list_create(scr2_3_1_4_cont);
    lv_obj_set_size(step_size_list, LV_HOR_RES - 10, 125);
    lv_obj_align(step_size_list, LV_ALIGN_BOTTOM_MID, 0, -5);
    apply_bg_color(step_size_list);
    lv_obj_set_style_pad_top(step_size_list, 2, LV_PART_MAIN);
    lv_obj_set_style_pad_row(step_size_list, 2, LV_PART_MAIN);
    apply_no_radius(step_size_list);
    apply_no_border(step_size_list);
    apply_no_shadow(step_size_list);

    // Add all step sizes to the list
    for (int i = 0; i < num_step_sizes; i++) {
        lv_obj_t *item = lv_list_add_btn(step_size_list, NULL, step_size_labels[i]);
        lv_obj_set_style_text_font(item, FONT_BOLD_14, LV_PART_MAIN);
        apply_text_color(item);
        apply_bg_color(item);
        lv_obj_add_event_cb(item, step_size_list_event, LV_EVENT_CLICKED, (void*)(intptr_t)i);
        lv_group_add_obj(lv_group_get_default(), item);

        lv_obj_remove_style(item, NULL, LV_STATE_FOCUS_KEY);
        lv_obj_set_style_outline_color(item, lv_color_hex(EMBED_COLOR_FOCUS_ON), LV_STATE_FOCUS_KEY);
        lv_obj_set_style_outline_width(item, 2, LV_STATE_FOCUS_KEY);
    }

    // Back button
    scr_back_btn_create(scr2_3_1_4_cont, scr2_3_1_4_back_btn_event_cb);
}

static void enter2_3_1_4(void)
{
    entry2_3_1_4_anim(scr2_3_1_4_cont);
    lv_group_set_wrap(lv_group_get_default(), true);
}

static void exit2_3_1_4(void)
{
    lv_group_set_wrap(lv_group_get_default(), false);
}

static void destroy2_3_1_4(void)
{
    if (scr2_3_1_4_cont) {
        lv_obj_del(scr2_3_1_4_cont);
        scr2_3_1_4_cont = NULL;
    }
}

scr_lifecycle_t screen2_3_1_4 = {
    .create = create2_3_1_4,
    .entry = enter2_3_1_4,
    .exit = exit2_3_1_4,
    .destroy = destroy2_3_1_4,
};
#endif

//************************************[ screen 2.4 ]************************************** SubGHz Spectrum Analyzer
#if 1
lv_obj_t *scr2_4_cont;
lv_obj_t *spectrum_range_btn;
lv_obj_t *spectrum_start_btn;
lv_obj_t *spectrum_freq_label_low;
lv_obj_t *spectrum_freq_label_high;
lv_obj_t *spectrum_freq_current;  // Current frequency being measured
lv_obj_t *spectrum_peak_label;    // Peak frequency display
lv_obj_t *spectrum_canvas;  // Canvas for drawing spectrum
lv_color_t *spectrum_canvas_buf;  // Canvas buffer
lv_timer_t *spectrum_update_timer = NULL;
bool spectrum_is_active = false;
bool spectrum_needs_update = false;  // Flag to trigger update from main loop
unsigned long spectrum_last_update = 0;  // Timestamp of last update

// Spectrum configuration
int spectrum_range_mode = 3;  // 0=300-348, 1=387-464, 2=779-928, 3=Full
#define SPECTRUM_WIDTH 320
#define SPECTRUM_HEIGHT 90
#define SPECTRUM_MAX_BARS 320

// Spectrum data storage
float spectrum_frequencies[SPECTRUM_MAX_BARS];
int spectrum_rssi_values[SPECTRUM_MAX_BARS];
int spectrum_bar_count = 0;
int spectrum_current_bar = 0;

// Peak tracking for each sweep
float spectrum_peak_freq = 0.0f;
int spectrum_peak_rssi = -100;

// Frequency sweep state
float spectrum_start_freq = 300.0f;
float spectrum_end_freq = 928.0f;
float spectrum_step_freq = 0.25f;  // 0.25 MHz steps for range scans

void entry2_4_anim(lv_obj_t *obj) { entry1_anim(obj); }
void exit2_4_anim(int user_data, lv_obj_t *obj) { exit1_anim(user_data, obj); }

// Helper function to get frequency step and range based on mode
static void spectrum_calculate_range(void)
{
    const float range_starts[] = {300.0f, 387.0f, 779.0f, 300.0f};
    const float range_ends[] = {348.0f, 464.0f, 928.0f, 928.0f};

    if (spectrum_range_mode >= 0 && spectrum_range_mode < 4) {
        spectrum_start_freq = range_starts[spectrum_range_mode];
        spectrum_end_freq = range_ends[spectrum_range_mode];
    }

    // Calculate number of bars and step size
    float total_range = spectrum_end_freq - spectrum_start_freq;
    spectrum_bar_count = SPECTRUM_WIDTH / 2;  // 2 pixels per bar for better visibility
    spectrum_step_freq = total_range / spectrum_bar_count;
}

// Get color based on RSSI value
static lv_color_t get_rssi_color(int rssi)
{
    if (rssi > -40) return lv_color_hex(0xFF0000);      // Red - very strong
    if (rssi > -55) return lv_color_hex(0xFF8000);      // Orange - strong
    if (rssi > -70) return lv_color_hex(0xFFFF00);      // Yellow - medium
    if (rssi > -85) return lv_color_hex(0x00FF00);      // Green - weak
    return lv_color_hex(0x0000FF);                       // Blue - very weak
}

// Update spectrum canvas with current RSSI values
static void spectrum_update_chart(void)
{
    if (!spectrum_canvas) return;

    // Clear canvas
    lv_canvas_fill_bg(spectrum_canvas, lv_color_hex(EMBED_COLOR_BG), LV_OPA_COVER);

    // Draw each bar with color based on RSSI
    int bar_width = SPECTRUM_WIDTH / spectrum_bar_count;
    if (bar_width < 1) bar_width = 1;

    for (int i = 0; i < spectrum_bar_count && i < SPECTRUM_MAX_BARS; i++) {
        int rssi = spectrum_rssi_values[i];

        // Map -100 to -40 dBm to bar height (0 to SPECTRUM_HEIGHT)
        int bar_height = map(rssi, -100, -40, 0, SPECTRUM_HEIGHT);
        bar_height = constrain(bar_height, 0, SPECTRUM_HEIGHT);

        if (bar_height > 0) {
            // Get color based on RSSI (or white if current bar)
            lv_color_t bar_color;
            if (i == spectrum_current_bar) {
                bar_color = lv_color_hex(0xFFFFFF);  // White for current bar being measured
            } else {
                bar_color = get_rssi_color(rssi);
            }

            // Draw bar from bottom up
            lv_draw_rect_dsc_t rect_dsc;
            lv_draw_rect_dsc_init(&rect_dsc);
            rect_dsc.bg_color = bar_color;
            rect_dsc.bg_opa = LV_OPA_COVER;
            rect_dsc.border_width = 0;

            int x = i * bar_width;
            int y = SPECTRUM_HEIGHT - bar_height;

            lv_canvas_draw_rect(spectrum_canvas, x, y, bar_width, bar_height, &rect_dsc);
        }
    }

    // Force invalidation
    lv_obj_invalidate(spectrum_canvas);
}

// Timer callback just sets a flag (like other async UI patterns)
static void spectrum_update_timer_event(lv_timer_t *t)
{
    if (!spectrum_is_active) {
        return;
    }

    // Update frequency label to show what's being measured
    float current_freq = spectrum_frequencies[spectrum_current_bar];
    lv_label_set_text_fmt(spectrum_freq_current, "%.3f MHz", current_freq);

    // Just update the chart with current data - don't do RF work here
    spectrum_update_chart();

    // CRITICAL: Force display refresh (matching Scan/Record pattern)
    lv_timer_handler();
    lv_refr_now(NULL);
}

// Process spectrum updates in main loop (OUTSIDE lv_timer_handler context)
void spectrum_process_update(void)
{
    if (!spectrum_is_active) {
        return;
    }

    // Throttle measurements to ~50ms intervals (matching timer period)
    static unsigned long last_measurement_time = 0;
    unsigned long current_time = millis();
    if (current_time - last_measurement_time < 50) {
        return;  // Skip this iteration
    }
    last_measurement_time = current_time;

    // Measure current frequency
    float current_freq = spectrum_frequencies[spectrum_current_bar];

    // CRITICAL: Must hold radioLock during ALL RF operations
    if (xSemaphoreTake(radioLock, pdMS_TO_TICKS(1)) == pdTRUE) {
        // Initialize radio for this frequency (EXACTLY like Scan/Record does)
        if (rf_initModule("rx", current_freq)) {
            delay(5);  // Reduced delay for faster scanning - CC1101 settles in ~1-2ms
            int rssi = ELECHOUSE_cc1101.getRssi();
            spectrum_rssi_values[spectrum_current_bar] = rssi;
        } else {
            spectrum_rssi_values[spectrum_current_bar] = -100;
        }

        xSemaphoreGive(radioLock);

        // CRITICAL: Squid workaround AFTER releasing lock
        // "To make sure CC1101 shared with TFT works properly"
        tft.drawPixel(0, 0, 0);
    } else {
        // Lock timeout - skip this measurement
        spectrum_rssi_values[spectrum_current_bar] = -100;
    }

    // Move to next bar
    spectrum_current_bar++;
    if (spectrum_current_bar >= spectrum_bar_count) {
        spectrum_current_bar = 0;

        // Completed a full sweep - find peak frequency
        spectrum_peak_rssi = -100;
        spectrum_peak_freq = 0.0f;
        for (int i = 0; i < spectrum_bar_count && i < SPECTRUM_MAX_BARS; i++) {
            if (spectrum_rssi_values[i] > spectrum_peak_rssi) {
                spectrum_peak_rssi = spectrum_rssi_values[i];
                spectrum_peak_freq = spectrum_frequencies[i];
            }
        }

        // Update peak label
        if (spectrum_peak_rssi > -100) {
            lv_label_set_text_fmt(spectrum_peak_label, "Peak: %.3f MHz (%d dBm)",
                                  spectrum_peak_freq, spectrum_peak_rssi);
        } else {
            lv_label_set_text(spectrum_peak_label, "Peak: ---");
        }
    }
}

static void spectrum_range_btn_event(lv_event_t *e)
{
    if (e->code == LV_EVENT_CLICKED) {
        if (!spectrum_is_active) {
            exit2_4_anim(SCREEN2_4_1_ID, scr2_4_cont);
        }
    }
}

static void spectrum_start_btn_event(lv_event_t *e)
{
    if (e->code == LV_EVENT_CLICKED) {
        if (!spectrum_is_active) {
            // Start spectrum analyzer
            // (SubGHz initialized in enter2_4)

            // Calculate frequency range
            spectrum_calculate_range();

            // Initialize frequency array
            if (spectrum_range_mode == 3) {
                // Full range - use actual frequency list (includes custom frequencies)
                std::vector<float> freq_list_data = rf_get_combined_frequency_list();
                spectrum_bar_count = freq_list_data.size();
                for (int i = 0; i < spectrum_bar_count && i < SPECTRUM_MAX_BARS; i++) {
                    spectrum_frequencies[i] = freq_list_data[i];
                    spectrum_rssi_values[i] = -100;
                }
            } else {
                // Single range - use linear steps
                for (int i = 0; i < spectrum_bar_count && i < SPECTRUM_MAX_BARS; i++) {
                    spectrum_frequencies[i] = spectrum_start_freq + (i * spectrum_step_freq);
                    spectrum_rssi_values[i] = -100;
                }
            }
            spectrum_current_bar = 0;

            // CRITICAL: Initialize radio ONCE at start (like Scan/Record does)
            // Start at first frequency
            rf_initModule("rx", spectrum_frequencies[0]);

            spectrum_is_active = true;
            lv_label_set_text(lv_obj_get_child(spectrum_start_btn, 0), "Stop");
            lv_obj_set_style_bg_color(spectrum_start_btn, lv_color_hex(0xFF0000), LV_PART_MAIN);
            lv_obj_set_style_text_color(lv_obj_get_child(spectrum_start_btn, 0), lv_color_black(), LV_PART_MAIN);

            // Timer is already running (resumed on screen entry)
        } else {
            // Stop spectrum analyzer
            spectrum_is_active = false;

            // CRITICAL: Deinitialize radio when stopping (like Scan/Record does)
            rf_deinitModule();

            // Don't pause timer - leave it running like Scan/Record does
            lv_label_set_text(lv_obj_get_child(spectrum_start_btn, 0), "Start");
            apply_bg_color(spectrum_start_btn);
            apply_text_color(lv_obj_get_child(spectrum_start_btn, 0));
        }
    }
}

static void spectrum_btn_back_event(lv_event_t *e)
{
    if (e->code == LV_EVENT_CLICKED) {
        if (spectrum_is_active) {
            spectrum_is_active = false;
            lv_timer_pause(spectrum_update_timer);
        }
        exit2_4_anim(SCREEN2_ID, scr2_4_cont);
    }
}

static void create2_4(lv_obj_t *parent)
{
    scr2_4_cont = create_screen_container(parent);

    // Title
    lv_obj_t *label = lv_label_create(scr2_4_cont);
    apply_text_color(label);
    lv_obj_set_style_text_font(label, FONT_BOLD_16, LV_PART_MAIN);
    lv_label_set_text(label, "Spectrum Analyzer");
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 10);

    // Range button (top row, left)
    spectrum_range_btn = lv_btn_create(scr2_4_cont);
    lv_obj_set_size(spectrum_range_btn, 90, 28);
    lv_obj_align(spectrum_range_btn, LV_ALIGN_TOP_LEFT, 35, 38);
    lv_obj_set_style_border_color(spectrum_range_btn, lv_color_hex(EMBED_COLOR_BORDER), LV_PART_MAIN);
    lv_obj_set_style_border_width(spectrum_range_btn, 1, LV_PART_MAIN);
    apply_no_shadow(spectrum_range_btn);
    apply_bg_color(spectrum_range_btn);
    lv_obj_remove_style(spectrum_range_btn, NULL, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_pad(spectrum_range_btn, 2, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_width(spectrum_range_btn, 2, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_color(spectrum_range_btn, lv_color_hex(EMBED_COLOR_FOCUS_ON), LV_STATE_FOCUS_KEY);
    lv_obj_add_event_cb(spectrum_range_btn, spectrum_range_btn_event, LV_EVENT_CLICKED, NULL);
    lv_obj_t *range_label = lv_label_create(spectrum_range_btn);
    apply_text_color(range_label);
    lv_obj_set_style_text_font(range_label, FONT_BOLD_14, LV_PART_MAIN);
    lv_label_set_text(range_label, "Range");
    lv_obj_center(range_label);
    lv_group_add_obj(lv_group_get_default(), spectrum_range_btn);

    // Start/Stop button (top row, right)
    spectrum_start_btn = lv_btn_create(scr2_4_cont);
    lv_obj_set_size(spectrum_start_btn, 70, 28);
    lv_obj_align(spectrum_start_btn, LV_ALIGN_TOP_RIGHT, -10, 38);
    lv_obj_set_style_border_color(spectrum_start_btn, lv_color_hex(EMBED_COLOR_BORDER), LV_PART_MAIN);
    lv_obj_set_style_border_width(spectrum_start_btn, 1, LV_PART_MAIN);
    apply_no_shadow(spectrum_start_btn);
    apply_bg_color(spectrum_start_btn);
    lv_obj_remove_style(spectrum_start_btn, NULL, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_pad(spectrum_start_btn, 2, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_width(spectrum_start_btn, 2, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_color(spectrum_start_btn, lv_color_hex(EMBED_COLOR_FOCUS_ON), LV_STATE_FOCUS_KEY);
    lv_obj_add_event_cb(spectrum_start_btn, spectrum_start_btn_event, LV_EVENT_CLICKED, NULL);
    lv_obj_t *start_label = lv_label_create(spectrum_start_btn);
    apply_text_color(start_label);
    lv_obj_set_style_text_font(start_label, FONT_BOLD_14, LV_PART_MAIN);
    lv_label_set_text(start_label, "Start");
    lv_obj_center(start_label);
    lv_group_add_obj(lv_group_get_default(), spectrum_start_btn);

    // Canvas for spectrum display with colored bars
    spectrum_canvas_buf = (lv_color_t *)malloc(SPECTRUM_WIDTH * SPECTRUM_HEIGHT * sizeof(lv_color_t));
    spectrum_canvas = lv_canvas_create(scr2_4_cont);
    lv_canvas_set_buffer(spectrum_canvas, spectrum_canvas_buf, SPECTRUM_WIDTH, SPECTRUM_HEIGHT, LV_IMG_CF_TRUE_COLOR);
    lv_obj_align(spectrum_canvas, LV_ALIGN_TOP_MID, 0, 72);
    lv_canvas_fill_bg(spectrum_canvas, lv_color_hex(EMBED_COLOR_BG), LV_OPA_COVER);

    // Peak frequency label (drawn after canvas so it appears on top)
    spectrum_peak_label = lv_label_create(scr2_4_cont);
    apply_text_color(spectrum_peak_label);
    lv_obj_set_style_text_font(spectrum_peak_label, FONT_BOLD_14, LV_PART_MAIN);
    lv_label_set_text(spectrum_peak_label, "Peak: ---");
    lv_obj_align(spectrum_peak_label, LV_ALIGN_TOP_MID, 0, 73);

    // Frequency labels below canvas
    spectrum_freq_label_low = lv_label_create(scr2_4_cont);
    apply_text_color(spectrum_freq_label_low);
    lv_obj_set_style_text_font(spectrum_freq_label_low, FONT_BOLD_14, LV_PART_MAIN);
    lv_label_set_text(spectrum_freq_label_low, "300MHz");
    lv_obj_align(spectrum_freq_label_low, LV_ALIGN_BOTTOM_LEFT, 10, -10);

    spectrum_freq_label_high = lv_label_create(scr2_4_cont);
    apply_text_color(spectrum_freq_label_high);
    lv_obj_set_style_text_font(spectrum_freq_label_high, FONT_BOLD_14, LV_PART_MAIN);
    lv_label_set_text(spectrum_freq_label_high, "928MHz");
    lv_obj_align(spectrum_freq_label_high, LV_ALIGN_BOTTOM_RIGHT, -10, -10);

    // Current frequency label
    spectrum_freq_current = lv_label_create(scr2_4_cont);
    apply_text_color(spectrum_freq_current);
    lv_obj_set_style_text_font(spectrum_freq_current, FONT_BOLD_14, LV_PART_MAIN);
    lv_label_set_text(spectrum_freq_current, "---");
    lv_obj_align(spectrum_freq_current, LV_ALIGN_BOTTOM_MID, 0, -10);

    // Back button - Added last for default focus
    scr_back_btn_create(scr2_4_cont, spectrum_btn_back_event);

    spectrum_update_timer = lv_timer_create(spectrum_update_timer_event, 50, NULL);
    lv_timer_pause(spectrum_update_timer);
}

static void enter2_4(void)
{
    // Initialize SubGHz module if needed
    if (!subghz_is_init()) {
        subghz_init();
    }

    // EXPERIMENT: Skip entry animation to test if it affects display updates
    // entry2_4_anim(scr2_4_cont);
    lv_obj_set_style_opa(scr2_4_cont, LV_OPA_100, LV_PART_MAIN);  // Set directly to visible
    spectrum_is_active = false;

    // CRITICAL: Resume timer on screen entry (like Scan/Record does)
    lv_timer_resume(spectrum_update_timer);

    // Update frequency labels based on current range
    spectrum_calculate_range();
    lv_label_set_text_fmt(spectrum_freq_label_low, "%.0fMHz", spectrum_start_freq);
    lv_label_set_text_fmt(spectrum_freq_label_high, "%.0fMHz", spectrum_end_freq);

    // Update range button text
    const char* range_names[] = {"300-348", "387-464", "779-928", "Scan List"};
    lv_label_set_text(lv_obj_get_child(spectrum_range_btn, 0), range_names[spectrum_range_mode]);

    // Initialize RSSI values to noise floor
    for (int i = 0; i < SPECTRUM_MAX_BARS; i++) {
        spectrum_rssi_values[i] = -100;  // Noise floor
    }

    // Initialize peak tracking
    spectrum_peak_rssi = -100;
    spectrum_peak_freq = 0.0f;
    lv_label_set_text(spectrum_peak_label, "Peak: ---");

    // Initialize chart display
    spectrum_update_chart();

    lv_group_set_wrap(lv_group_get_default(), true);
}

static void exit2_4(void)
{
    if (spectrum_is_active) {
        spectrum_is_active = false;
        lv_timer_pause(spectrum_update_timer);
    }
    lv_group_set_wrap(lv_group_get_default(), false);
}

static void destroy2_4(void)
{
    if (spectrum_update_timer) {
        lv_timer_del(spectrum_update_timer);
        spectrum_update_timer = NULL;
    }

    if (spectrum_is_active) {
        spectrum_is_active = false;
    }

    // Free canvas buffer
    if (spectrum_canvas_buf) {
        free(spectrum_canvas_buf);
        spectrum_canvas_buf = NULL;
    }

    if (scr2_4_cont) {
        lv_obj_del(scr2_4_cont);
        scr2_4_cont = NULL;
    }
}

scr_lifecycle_t screen2_4 = {
    .create = create2_4,
    .entry = enter2_4,
    .exit = exit2_4,
    .destroy = destroy2_4,
};
#endif

//************************************[ screen 2.4.1 ]************************************** Spectrum Range Selector
#if 1
lv_obj_t *scr2_4_1_cont;
lv_obj_t *spectrum_range_list;

void entry2_4_1_anim(lv_obj_t *obj) { entry1_anim(obj); }
void exit2_4_1_anim(int user_data, lv_obj_t *obj) { exit1_anim(user_data, obj); }

static void spectrum_range_list_event(lv_event_t *e)
{
    if (e->code == LV_EVENT_CLICKED) {
        int range = (int)(intptr_t)lv_event_get_user_data(e);
        spectrum_range_mode = range;
        exit2_4_1_anim(SCREEN2_4_ID, scr2_4_1_cont);
    }
}

static void scr2_4_1_back_btn_event_cb(lv_event_t *e)
{
    if (e->code == LV_EVENT_CLICKED) {
        exit2_4_1_anim(SCREEN2_4_ID, scr2_4_1_cont);
    }
}

static void create2_4_1(lv_obj_t *parent)
{
    scr2_4_1_cont = create_screen_container(parent);

    // Title
    lv_obj_t *label = lv_label_create(scr2_4_1_cont);
    apply_text_color(label);
    lv_obj_set_style_text_font(label, FONT_BOLD_16, LV_PART_MAIN);
    lv_label_set_text(label, "Select Range");
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 10);

    // Range list
    spectrum_range_list = lv_list_create(scr2_4_1_cont);
    lv_obj_set_size(spectrum_range_list, LV_HOR_RES - 10, 100);
    lv_obj_align(spectrum_range_list, LV_ALIGN_BOTTOM_MID, 0, -5);
    apply_bg_color(spectrum_range_list);
    lv_obj_set_style_pad_top(spectrum_range_list, 2, LV_PART_MAIN);
    lv_obj_set_style_pad_row(spectrum_range_list, 2, LV_PART_MAIN);
    apply_no_radius(spectrum_range_list);
    apply_no_border(spectrum_range_list);
    apply_no_shadow(spectrum_range_list);

    const char* ranges[] = {"300-348 MHz", "387-464 MHz", "779-928 MHz", "Scan List"};
    for (int i = 0; i < 4; i++) {
        lv_obj_t *item = lv_list_add_btn(spectrum_range_list, NULL, ranges[i]);
        lv_obj_set_style_text_font(item, FONT_BOLD_14, LV_PART_MAIN);
        apply_text_color(item);
        apply_bg_color(item);
        lv_obj_add_event_cb(item, spectrum_range_list_event, LV_EVENT_CLICKED, (void*)(intptr_t)i);
        lv_group_add_obj(lv_group_get_default(), item);

        // Highlight currently selected range
        if (spectrum_range_mode == i) {
            lv_obj_set_style_text_color(item, lv_color_hex(0x00FF00), LV_PART_MAIN);
        }

        lv_obj_remove_style(item, NULL, LV_STATE_FOCUS_KEY);
        lv_obj_set_style_outline_color(item, lv_color_hex(EMBED_COLOR_FOCUS_ON), LV_STATE_FOCUS_KEY);
        lv_obj_set_style_outline_width(item, 2, LV_STATE_FOCUS_KEY);
    }

    // Back button
    scr_back_btn_create(scr2_4_1_cont, scr2_4_1_back_btn_event_cb);
}

static void enter2_4_1(void)
{
    entry2_4_1_anim(scr2_4_1_cont);
    lv_group_set_wrap(lv_group_get_default(), true);
}

static void exit2_4_1(void)
{
    lv_group_set_wrap(lv_group_get_default(), false);
}

static void destroy2_4_1(void)
{
    if (scr2_4_1_cont) {
        lv_obj_del(scr2_4_1_cont);
        scr2_4_1_cont = NULL;
    }
}

scr_lifecycle_t screen2_4_1 = {
    .create = create2_4_1,
    .entry = enter2_4_1,
    .exit = exit2_4_1,
    .destroy = destroy2_4_1,
};
#endif

//************************************[ screen 2.5 ]************************************** SubGHz Remotes
#if 1
// View modes for SubGHz Remotes
enum SubGHzRemoteMode {
    SUBGHZ_REMOTE_FILE_BROWSER,      // Browse .txt remote files in /sgremotes
    SUBGHZ_REMOTE_BUTTON_LIST,       // Show buttons within selected remote
    SUBGHZ_REMOTE_EDIT_BUTTON_LIST   // Edit mode - show buttons with edit context menu
};

lv_obj_t *scr2_5_cont;
lv_obj_t *subghz_remote_path_label;
lv_obj_t *subghz_remote_file_list;
lv_obj_t *subghz_remote_led;
lv_obj_t *subghz_remote_keyboard = NULL;  // Keyboard for text input
lv_obj_t *subghz_remote_textarea = NULL;  // Text area for keyboard
char subghz_remote_current_path[256] = "/sgremotes";  // Default SubGHz remotes directory
SubGHzRemoteMode subghz_remote_mode = SUBGHZ_REMOTE_FILE_BROWSER;
SubGHzRemote subghz_remote_current;  // Currently loaded remote
bool subghz_remote_long_press_occurred = false;  // Flag to prevent CLICKED after LONG_PRESSED
char subghz_selected_remote[256] = "";  // Selected remote filename (for context menu)
int subghz_selected_button_index = -1;  // Selected button index (for context menu)

void entry2_5_anim(lv_obj_t *obj)
{
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, obj);
    lv_anim_set_values(&a, LV_OPA_0, LV_OPA_100);
    lv_anim_set_time(&a, 200);
    lv_anim_set_path_cb(&a, lv_anim_path_linear);
    lv_anim_set_exec_cb(&a, [](void * var, int32_t v){
        lv_port_indev_enabled(false);
        lv_obj_set_style_opa((lv_obj_t *)var, v, LV_PART_MAIN);
    });
    lv_anim_set_ready_cb(&a, [](_lv_anim_t *a){
        lv_port_indev_enabled(true);
        extern TFT_eSPI tft;
        tft.drawPixel(0, 0, 0);  // Dummy write to sync SPI after rotation transform
    });
    lv_anim_start(&a);
}

void exit2_5_anim(int user_data, lv_obj_t *obj)
{
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, obj);
    lv_anim_set_values(&a, LV_OPA_100, LV_OPA_0);
    lv_anim_set_time(&a, 200);
    lv_anim_set_path_cb(&a, lv_anim_path_linear);
    lv_anim_set_exec_cb(&a, [](void * var, int32_t v){
        lv_port_indev_enabled(false);
        lv_obj_set_style_opa((lv_obj_t *)var, v, LV_PART_MAIN);
    });
    lv_anim_set_ready_cb(&a, [](_lv_anim_t *a){
        scr_mgr_switch((int)a->user_data, false);
        lv_port_indev_enabled(true);
        extern TFT_eSPI tft;
        tft.drawPixel(0, 0, 0);  // Dummy write to sync SPI after rotation transform
    });
    lv_anim_set_user_data(&a, (void *)user_data);
    lv_anim_start(&a);
}

// Forward declarations
void subghz_remote_load_directory(const char *path);
void subghz_remote_show_buttons();
static void subghz_create_remote_keyboard();
static void subghz_rename_remote_keyboard();
static void subghz_rename_button_keyboard();
static void subghz_delete_remote();
static void subghz_remote_keyboard_event(lv_event_t *e);
static void subghz_button_rename_keyboard_event(lv_event_t *e);
static void subghz_create_button_event(lv_event_t *e);
static void subghz_edit_button_event(int button_index);
void enter_subghz_file_browser(int button_index);  // Entry to Screen 2.5.1 (file browser)
static void subghz_transmit_button(SubGHzButton& button, bool continuous);

// Helper function to transmit a SubGHz button signal
// Precondition: button.filepath must be valid
static void subghz_transmit_button(SubGHzButton& button, bool continuous) {
    if (!validateSubFile(button.filepath.c_str())) {
        return;
    }

    // (SubGHz initialized in enter2_5)

    lv_led_set_brightness(subghz_remote_led, 255);
    lv_led_on(subghz_remote_led);
    lv_task_handler();  // Process LVGL tasks
    lv_refr_now(NULL);  // Force display update for LED
    delay(10);  // Give display time to update
    tft.drawPixel(0, 0, 0);  // Sync SPI

    if (continuous) {
        // Keep transmitting while encoder button is held down
        // Note: rf_sendRaw checks for button press to allow cancellation,
        // which conflicts with our continuous transmission. We ignore
        // the return value and keep going as long as button is held.
        int transmission_count = 0;
        while (digitalRead(ENCODER_KEY) == LOW) {
            transmission_count++;
            rf_transmitFile(String(button.filepath.c_str()));
            // No delay - send continuously
        }
    } else {
        // Single transmission
        rf_transmitFile(String(button.filepath.c_str()));
    }

    lv_led_off(subghz_remote_led);
    lv_task_handler();  // Process LVGL tasks
    lv_refr_now(NULL);  // Force display update for LED
    delay(10);  // Give display time to update
    tft.drawPixel(0, 0, 0);  // Sync SPI
}

// Keyboard event handler for create/rename remote
static void subghz_remote_keyboard_event(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_READY) {
        // User pressed OK
        const char *new_name = lv_textarea_get_text(subghz_remote_textarea);

        if (strlen(new_name) > 0) {
            // Build full path with .txt extension
            char full_path[512];
            snprintf(full_path, sizeof(full_path), "/sgremotes/%s.txt", new_name);

            // Check if we're renaming or creating new
            if (strlen(subghz_selected_remote) > 0) {
                // Rename existing remote
                char old_path[512];
                snprintf(old_path, sizeof(old_path), "/sgremotes/%s", subghz_selected_remote);

                // Rename directly (SD card is mounted at root)
                SD.rename(old_path, full_path);

                subghz_selected_remote[0] = '\0';  // Clear selected remote
            } else {
                // Create new remote
                if (createEmptySubGHzRemote(full_path)) {
                }
            }

            // Reload directory
            subghz_remote_load_directory(subghz_remote_current_path);
        }

        // Clean up keyboard
        lv_group_t *g = lv_group_get_default();
        lv_group_set_editing(g, false);
        lv_group_focus_freeze(g, false);

        if (subghz_remote_keyboard) {
            lv_obj_del(subghz_remote_keyboard);
            subghz_remote_keyboard = NULL;
        }
        if (subghz_remote_textarea) {
            lv_obj_del(subghz_remote_textarea);
            subghz_remote_textarea = NULL;
        }

        lv_refr_now(NULL);
        tft.drawPixel(0, 0, 0); // Dummy write to sync SPI
        if (lv_group_get_focused(g) == NULL && lv_obj_get_child_cnt(subghz_remote_file_list) > 0) {
            lv_obj_t *first_item = lv_obj_get_child(subghz_remote_file_list, 0);
            if (first_item) {
                lv_group_focus_obj(first_item);
            }
        }

        subghz_selected_remote[0] = '\0';

    } else if (code == LV_EVENT_CANCEL) {
        // User cancelled
        lv_group_t *g = lv_group_get_default();
        lv_group_set_editing(g, false);
        lv_group_focus_freeze(g, false);

        if (subghz_remote_keyboard) {
            lv_obj_del(subghz_remote_keyboard);
            subghz_remote_keyboard = NULL;
        }
        if (subghz_remote_textarea) {
            lv_obj_del(subghz_remote_textarea);
            subghz_remote_textarea = NULL;
        }

        lv_refr_now(NULL);
        tft.drawPixel(0, 0, 0); // Dummy write to sync SPI
        if (lv_group_get_focused(g) == NULL && lv_obj_get_child_cnt(subghz_remote_file_list) > 0) {
            lv_obj_t *first_item = lv_obj_get_child(subghz_remote_file_list, 0);
            if (first_item) {
                lv_group_focus_obj(first_item);
            }
        }

        subghz_selected_remote[0] = '\0';
    }
}

// Create remote keyboard (for new remote creation)
static void subghz_create_remote_keyboard()
{
    // Generate default name
    String default_name = generateSubGHzRemoteName();
    default_name.replace(".txt", "");  // Remove extension for display

    // Create text area for input
    subghz_remote_textarea = lv_textarea_create(lv_scr_act());
    lv_obj_set_size(subghz_remote_textarea, LV_PCT(90), 40);
    lv_obj_align(subghz_remote_textarea, LV_ALIGN_TOP_MID, 0, 40);
    lv_textarea_set_one_line(subghz_remote_textarea, true);
    lv_textarea_set_text(subghz_remote_textarea, default_name.c_str());

    // Enable and style the cursor for visibility
    lv_obj_set_style_anim_time(subghz_remote_textarea, 400, LV_PART_CURSOR);
    lv_obj_set_style_bg_opa(subghz_remote_textarea, LV_OPA_COVER, LV_PART_CURSOR);
    lv_obj_set_style_bg_color(subghz_remote_textarea, lv_color_hex(EMBED_COLOR_FOCUS_ON), LV_PART_CURSOR);
    lv_obj_set_style_border_width(subghz_remote_textarea, 1, LV_PART_CURSOR);
    lv_obj_set_style_border_color(subghz_remote_textarea, lv_color_hex(EMBED_COLOR_FOCUS_ON), LV_PART_CURSOR);
    lv_textarea_set_cursor_pos(subghz_remote_textarea, LV_TEXTAREA_CURSOR_LAST);

    // Create keyboard
    subghz_remote_keyboard = lv_keyboard_create(lv_scr_act());
    lv_keyboard_set_textarea(subghz_remote_keyboard, subghz_remote_textarea);
    lv_obj_add_event_cb(subghz_remote_keyboard, subghz_remote_keyboard_event, LV_EVENT_ALL, NULL);

    // Setup focus and navigation
    lv_group_t *g = lv_group_get_default();
    lv_group_add_obj(g, subghz_remote_textarea);
    lv_group_add_obj(g, subghz_remote_keyboard);
    lv_group_focus_obj(subghz_remote_keyboard);
    lv_obj_add_state(subghz_remote_keyboard, LV_STATE_FOCUS_KEY);
    lv_group_set_editing(g, true);
    lv_group_focus_freeze(g, true);

    subghz_selected_remote[0] = '\0';  // Clear selected remote (we're creating new)
}

// Rename remote keyboard (for renaming existing remote)
// Precondition: subghz_selected_remote must be set by caller
static void subghz_rename_remote_keyboard()
{
    // Extract current name without extension
    String current_name = String(subghz_selected_remote);
    current_name.replace(".txt", "");

    // Create text area for input
    subghz_remote_textarea = lv_textarea_create(lv_scr_act());
    lv_obj_set_size(subghz_remote_textarea, LV_PCT(90), 40);
    lv_obj_align(subghz_remote_textarea, LV_ALIGN_TOP_MID, 0, 40);
    lv_textarea_set_one_line(subghz_remote_textarea, true);
    lv_textarea_set_text(subghz_remote_textarea, current_name.c_str());

    // Enable and style the cursor for visibility
    lv_obj_set_style_anim_time(subghz_remote_textarea, 400, LV_PART_CURSOR);
    lv_obj_set_style_bg_opa(subghz_remote_textarea, LV_OPA_COVER, LV_PART_CURSOR);
    lv_obj_set_style_bg_color(subghz_remote_textarea, lv_color_hex(EMBED_COLOR_FOCUS_ON), LV_PART_CURSOR);
    lv_obj_set_style_border_width(subghz_remote_textarea, 1, LV_PART_CURSOR);
    lv_obj_set_style_border_color(subghz_remote_textarea, lv_color_hex(EMBED_COLOR_FOCUS_ON), LV_PART_CURSOR);
    lv_textarea_set_cursor_pos(subghz_remote_textarea, LV_TEXTAREA_CURSOR_LAST);

    // Create keyboard
    subghz_remote_keyboard = lv_keyboard_create(lv_scr_act());
    lv_keyboard_set_textarea(subghz_remote_keyboard, subghz_remote_textarea);
    lv_obj_add_event_cb(subghz_remote_keyboard, subghz_remote_keyboard_event, LV_EVENT_ALL, NULL);

    // Setup focus and navigation
    lv_group_t *g = lv_group_get_default();
    lv_group_add_obj(g, subghz_remote_textarea);
    lv_group_add_obj(g, subghz_remote_keyboard);
    lv_group_focus_obj(subghz_remote_keyboard);
    lv_obj_add_state(subghz_remote_keyboard, LV_STATE_FOCUS_KEY);
    lv_group_set_editing(g, true);
    lv_group_focus_freeze(g, true);
}

// Delete remote
// Precondition: subghz_selected_remote must be set by caller
static void subghz_delete_remote()
{
    char full_path[512];
    snprintf(full_path, sizeof(full_path), "/sgremotes/%s", subghz_selected_remote);

    // Delete directly (SD card is mounted at root)
    SD.remove(full_path);

    subghz_selected_remote[0] = '\0';

    // Reload directory
    subghz_remote_load_directory(subghz_remote_current_path);
}

// Rename button keyboard (for renaming existing button)
// Precondition: subghz_selected_button_index must be set by caller
static void subghz_rename_button_keyboard()
{
    if (subghz_selected_button_index < 0 || subghz_selected_button_index >= subghz_remote_current.buttons.size()) {
        return;
    }

    // Get current button name
    String current_name = subghz_remote_current.buttons[subghz_selected_button_index].name;

    // Create text area for input
    subghz_remote_textarea = lv_textarea_create(lv_scr_act());
    lv_obj_set_size(subghz_remote_textarea, LV_PCT(90), 40);
    lv_obj_align(subghz_remote_textarea, LV_ALIGN_TOP_MID, 0, 40);
    lv_textarea_set_one_line(subghz_remote_textarea, true);
    lv_textarea_set_text(subghz_remote_textarea, current_name.c_str());

    // Enable and style the cursor for visibility
    lv_obj_set_style_anim_time(subghz_remote_textarea, 400, LV_PART_CURSOR);
    lv_obj_set_style_bg_opa(subghz_remote_textarea, LV_OPA_COVER, LV_PART_CURSOR);
    lv_obj_set_style_bg_color(subghz_remote_textarea, lv_color_hex(EMBED_COLOR_FOCUS_ON), LV_PART_CURSOR);
    lv_obj_set_style_border_width(subghz_remote_textarea, 1, LV_PART_CURSOR);
    lv_obj_set_style_border_color(subghz_remote_textarea, lv_color_hex(EMBED_COLOR_FOCUS_ON), LV_PART_CURSOR);
    lv_textarea_set_cursor_pos(subghz_remote_textarea, LV_TEXTAREA_CURSOR_LAST);

    // Create keyboard
    subghz_remote_keyboard = lv_keyboard_create(lv_scr_act());
    lv_keyboard_set_textarea(subghz_remote_keyboard, subghz_remote_textarea);
    lv_obj_add_event_cb(subghz_remote_keyboard, subghz_button_rename_keyboard_event, LV_EVENT_ALL, NULL);

    // Setup focus and navigation
    lv_group_t *g = lv_group_get_default();
    lv_group_add_obj(g, subghz_remote_textarea);
    lv_group_add_obj(g, subghz_remote_keyboard);
    lv_group_focus_obj(subghz_remote_keyboard);
    lv_obj_add_state(subghz_remote_keyboard, LV_STATE_FOCUS_KEY);
    lv_group_set_editing(g, true);
    lv_group_focus_freeze(g, true);
}

// Keyboard event handler for renaming button
static void subghz_button_rename_keyboard_event(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_READY) {
        // User pressed OK
        const char *new_name = lv_textarea_get_text(subghz_remote_textarea);

        if (strlen(new_name) > 0 && subghz_selected_button_index >= 0 && subghz_selected_button_index < subghz_remote_current.buttons.size()) {
            // Create updated button with new name but same filepath
            SubGHzButton updated_button;
            updated_button.name = String(new_name);
            updated_button.filepath = subghz_remote_current.buttons[subghz_selected_button_index].filepath;

            // Replace button
            if (replaceButtonInSubGHzRemote(subghz_remote_current.filepath.c_str(), subghz_selected_button_index, updated_button)) {
                // Reload the remote
                // Save filepath before parsing since parseSubGHzRemoteFile calls remote.clear()
                String remote_filepath = subghz_remote_current.filepath;
                parseSubGHzRemoteFile(remote_filepath.c_str(), subghz_remote_current);
                subghz_remote_show_buttons();
            }
        }

        subghz_selected_button_index = -1;

        // Clean up keyboard
        lv_group_t *g = lv_group_get_default();
        lv_group_set_editing(g, false);
        lv_group_focus_freeze(g, false);

        if (subghz_remote_keyboard) {
            lv_obj_del(subghz_remote_keyboard);
            subghz_remote_keyboard = NULL;
        }
        if (subghz_remote_textarea) {
            lv_obj_del(subghz_remote_textarea);
            subghz_remote_textarea = NULL;
        }

        lv_refr_now(NULL);
        tft.drawPixel(0, 0, 0); // Dummy write to sync SPI
        if (lv_group_get_focused(g) == NULL && lv_obj_get_child_cnt(subghz_remote_file_list) > 0) {
            lv_obj_t *first_item = lv_obj_get_child(subghz_remote_file_list, 0);
            if (first_item) {
                lv_group_focus_obj(first_item);
            }
        }

    } else if (code == LV_EVENT_CANCEL) {
        // User cancelled
        subghz_selected_button_index = -1;

        lv_group_t *g = lv_group_get_default();
        lv_group_set_editing(g, false);
        lv_group_focus_freeze(g, false);

        if (subghz_remote_keyboard) {
            lv_obj_del(subghz_remote_keyboard);
            subghz_remote_keyboard = NULL;
        }
        if (subghz_remote_textarea) {
            lv_obj_del(subghz_remote_textarea);
            subghz_remote_textarea = NULL;
        }

        lv_refr_now(NULL);
        tft.drawPixel(0, 0, 0); // Dummy write to sync SPI
        if (lv_group_get_focused(g) == NULL && lv_obj_get_child_cnt(subghz_remote_file_list) > 0) {
            lv_obj_t *first_item = lv_obj_get_child(subghz_remote_file_list, 0);
            if (first_item) {
                lv_group_focus_obj(first_item);
            }
        }
    }
}

// (Context menu handlers moved inline to avoid null pointer issues)

// File/remote list item event handler
static void subghz_remote_list_event(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *btn = lv_event_get_target(e);
    const char *label_text = lv_list_get_btn_text(subghz_remote_file_list, btn);

    if (code == LV_EVENT_PRESSED) {
        subghz_remote_long_press_occurred = false;
    }

    if (code == LV_EVENT_LONG_PRESSED) {
        subghz_remote_long_press_occurred = true;

        if (subghz_remote_mode == SUBGHZ_REMOTE_FILE_BROWSER) {
            // Long-press on remote file -> show context menu
            if (strstr(label_text, "(New Remote)") || strstr(label_text, "<-") || strstr(label_text, "(Parent)")) {
                return;  // Ignore special items
            }

            // Store selected remote
            snprintf(subghz_selected_remote, sizeof(subghz_selected_remote), "%s.txt", label_text);

            // Show context menu
            static const char *btns[] = {"Name", "Edit", "Del", "Cancel", ""};
            lv_obj_t *mbox = lv_msgbox_create(NULL, "Remote Options", NULL, btns, false);
            lv_obj_center(mbox);

            // Add event callback using lambda
            lv_obj_add_event_cb(mbox, [](lv_event_t *e) {
                lv_event_code_t code = lv_event_get_code(e);
                if (code == LV_EVENT_VALUE_CHANGED) {
                    lv_obj_t *mbox = lv_event_get_current_target(e);
                    const char *txt = lv_msgbox_get_active_btn_text(mbox);

                    if (txt) {
                        bool keep_selection = false;  // Track if we should keep subghz_selected_remote

                        if (strcmp(txt, "Name") == 0) {
                            subghz_rename_remote_keyboard();
                            keep_selection = true;  // Keep for rename keyboard handler
                        } else if (strcmp(txt, "Edit") == 0) {
                            // Edit remote -> load remote and enter edit mode
                            char full_path[512];
                            snprintf(full_path, sizeof(full_path), "/sgremotes/%s", subghz_selected_remote);
                            if (parseSubGHzRemoteFile(full_path, subghz_remote_current)) {
                                subghz_remote_mode = SUBGHZ_REMOTE_EDIT_BUTTON_LIST;
                                subghz_remote_show_buttons();  // Reuse same display function
                            }
                        } else if (strcmp(txt, "Del") == 0) {
                            subghz_delete_remote();
                        }
                        // If Cancel, just close

                        // Close msgbox and restore navigation
                        lv_group_t *g = lv_group_get_default();
                        lv_group_set_editing(g, false);
                        lv_group_focus_freeze(g, false);
                        lv_msgbox_close(mbox);
                        lv_refr_now(NULL);
                        tft.drawPixel(0, 0, 0); // Dummy write to sync SPI

                        if (lv_group_get_focused(g) == NULL && lv_obj_get_child_cnt(subghz_remote_file_list) > 0) {
                            lv_obj_t *first_item = lv_obj_get_child(subghz_remote_file_list, 0);
                            if (first_item) {
                                lv_group_focus_obj(first_item);
                            }
                        }

                        // Only clear selection if not needed by subsequent handlers
                        if (!keep_selection) {
                            subghz_selected_remote[0] = '\0';
                        }
                    }
                }
            }, LV_EVENT_VALUE_CHANGED, NULL);

            // Setup focus
            lv_group_t *g = lv_group_get_default();
            lv_obj_t *btnm = lv_msgbox_get_btns(mbox);

            if (btnm) {
                lv_group_add_obj(g, btnm);
                lv_group_focus_obj(btnm);
                lv_obj_add_state(btnm, LV_STATE_FOCUS_KEY);
                lv_group_set_editing(g, true);
            }
            lv_group_focus_freeze(g, true);

        } else if (subghz_remote_mode == SUBGHZ_REMOTE_BUTTON_LIST) {
            // Long-press on button -> continuous transmit
            if (strstr(label_text, "(New Button)") || strstr(label_text, "<-")) {
                return;  // Ignore special items
            }

            // Get button index and transmit continuously
            int button_index = (int)(intptr_t)lv_obj_get_user_data(btn);
            if (button_index >= 0 && button_index < subghz_remote_current.buttons.size()) {
                SubGHzButton& button = subghz_remote_current.buttons[button_index];
                subghz_transmit_button(button, true);  // Continuous transmission
            }
        } else if (subghz_remote_mode == SUBGHZ_REMOTE_EDIT_BUTTON_LIST) {
            // In edit mode, long-press does nothing (use click for menu)
            return;
        }
    }

    if (code == LV_EVENT_CLICKED) {
        if (subghz_remote_long_press_occurred) {
            subghz_remote_long_press_occurred = false;
            return;  // Ignore click after long press
        }

        if (subghz_remote_mode == SUBGHZ_REMOTE_FILE_BROWSER) {
            // Clicked on file browser item
            if (strstr(label_text, "(New Remote)")) {
                // Create new remote
                subghz_create_remote_keyboard();
            } else if (strstr(label_text, "(Parent)")) {
                // Navigate up directory (currently not used since we're fixed to /sgremotes)
                return;
            } else {
                // Load remote file and show buttons
                char full_path[512];
                snprintf(full_path, sizeof(full_path), "/sgremotes/%s.txt", label_text);

                if (parseSubGHzRemoteFile(full_path, subghz_remote_current)) {
                    subghz_remote_mode = SUBGHZ_REMOTE_BUTTON_LIST;
                    subghz_remote_show_buttons();
                }
            }
        } else if (subghz_remote_mode == SUBGHZ_REMOTE_BUTTON_LIST) {
            // Clicked on button list item
            if (strstr(label_text, "(New Button)")) {
                // Create new button -> go to file browser
                enter_subghz_file_browser(-1);  // -1 indicates new button
            } else {
                // Transmit button
                int button_index = (int)(intptr_t)lv_obj_get_user_data(btn);
                if (button_index >= 0 && button_index < subghz_remote_current.buttons.size()) {
                    SubGHzButton& button = subghz_remote_current.buttons[button_index];
                    subghz_transmit_button(button, false);  // Single transmission
                }
            }
        } else if (subghz_remote_mode == SUBGHZ_REMOTE_EDIT_BUTTON_LIST) {
            // Clicked on button in edit mode -> show edit context menu
            int button_index = (int)(intptr_t)lv_obj_get_user_data(btn);
            if (button_index >= 0 && button_index < subghz_remote_current.buttons.size()) {
                subghz_selected_button_index = button_index;

                // Show edit context menu
                static const char *btns[] = {"Name", ".sub", "Del", "Cancel", ""};
                lv_obj_t *mbox = lv_msgbox_create(NULL, "Edit Button", NULL, btns, false);
                lv_obj_center(mbox);

                // Add event callback using lambda
                lv_obj_add_event_cb(mbox, [](lv_event_t *e) {
                    lv_event_code_t code = lv_event_get_code(e);
                    if (code == LV_EVENT_VALUE_CHANGED) {
                        lv_obj_t *mbox = lv_event_get_current_target(e);
                        const char *txt = lv_msgbox_get_active_btn_text(mbox);

                        if (txt) {
                            if (strcmp(txt, "Name") == 0) {
                                // Rename button - show keyboard
                                subghz_rename_button_keyboard();
                            } else if (strcmp(txt, ".sub") == 0) {
                                // Change .sub file - go to file browser
                                enter_subghz_file_browser(subghz_selected_button_index);
                            } else if (strcmp(txt, "Del") == 0) {
                                // Delete button
                                if (deleteButtonFromSubGHzRemote(subghz_remote_current.filepath.c_str(), subghz_selected_button_index)) {
                                    // Reload the remote
                                    parseSubGHzRemoteFile(subghz_remote_current.filepath.c_str(), subghz_remote_current);
                                    subghz_remote_show_buttons();
                                }
                            }
                            // If Cancel, just close
                        }

                        // Close msgbox and restore navigation
                        lv_group_t *g = lv_group_get_default();
                        lv_group_set_editing(g, false);
                        lv_group_focus_freeze(g, false);
                        lv_msgbox_close(mbox);
                        lv_refr_now(NULL);
                        tft.drawPixel(0, 0, 0); // Dummy write to sync SPI

                        if (lv_group_get_focused(g) == NULL && lv_obj_get_child_cnt(subghz_remote_file_list) > 0) {
                            lv_obj_t *first_item = lv_obj_get_child(subghz_remote_file_list, 0);
                            if (first_item) {
                                lv_group_focus_obj(first_item);
                            }
                        }

                        subghz_selected_button_index = -1;
                    }
                }, LV_EVENT_VALUE_CHANGED, NULL);

                // Setup focus
                lv_group_t *g = lv_group_get_default();
                lv_obj_t *btnm = lv_msgbox_get_btns(mbox);

                if (btnm) {
                    lv_group_add_obj(g, btnm);
                    lv_group_focus_obj(btnm);
                    lv_obj_add_state(btnm, LV_STATE_FOCUS_KEY);
                    lv_group_set_editing(g, true);
                }
                lv_group_focus_freeze(g, true);
            }
        }
    }
}

// Load directory contents (file browser mode)
void subghz_remote_load_directory(const char *path)
{
    // Clear existing list
    lv_obj_clean(subghz_remote_file_list);

    // Update path label
    char path_display[300];
    snprintf(path_display, sizeof(path_display), "Path: %s", path);
    lv_label_set_text(subghz_remote_path_label, path_display);

    // Create /sgremotes directory if it doesn't exist
    if (!SD.exists(path)) {
        SD.mkdir(path);
    }

    // Try multiple mount points (SD card VFS mount location)
    const char* mount_points[] = {"/sd", "/sdcard", "/mnt/sd"};
    DIR *dir = NULL;

    for (int i = 0; i < 3; i++) {
        char test_path[512];
        snprintf(test_path, sizeof(test_path), "%s%s", mount_points[i], path);
        dir = opendir(test_path);
        if (dir) break;
    }

    // Fallback: try direct path
    if (!dir) {
        dir = opendir(path);
    }

    if (dir) {
        struct dirent *entry;
        std::vector<String> files;

        // Read all .txt files
        while ((entry = readdir(dir)) != NULL) {
            String filename = entry->d_name;
            if (filename.endsWith(".txt") && !filename.startsWith(".")) {
                filename.replace(".txt", "");  // Remove extension for display
                files.push_back(filename);
            }
        }
        closedir(dir);

        // Sort files alphabetically
        std::sort(files.begin(), files.end());

        // Add file items to list
        for (const String& file : files) {
            lv_obj_t *item = lv_list_add_btn(subghz_remote_file_list, NULL, file.c_str());
            lv_obj_set_style_text_font(item, FONT_BOLD_14, LV_PART_MAIN);
            apply_text_color(item);
            apply_bg_color(item);
            lv_obj_add_event_cb(item, subghz_remote_list_event, LV_EVENT_CLICKED, NULL);
            lv_obj_add_event_cb(item, subghz_remote_list_event, LV_EVENT_LONG_PRESSED, NULL);
            lv_obj_add_event_cb(item, subghz_remote_list_event, LV_EVENT_PRESSED, NULL);
            lv_group_add_obj(lv_group_get_default(), item);
            lv_obj_remove_style(item, NULL, LV_STATE_FOCUS_KEY);
            lv_obj_set_style_outline_color(item, lv_color_hex(EMBED_COLOR_FOCUS_ON), LV_STATE_FOCUS_KEY);
            lv_obj_set_style_outline_width(item, 2, LV_STATE_FOCUS_KEY);
        }
    } else {
    }

    // Add "(New Remote)" button
    lv_obj_t *new_item = lv_list_add_btn(subghz_remote_file_list, NULL, "(New Remote)");
    lv_obj_set_style_text_font(new_item, FONT_BOLD_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(new_item, lv_color_hex(0x00FF00), LV_PART_MAIN);
    apply_bg_color(new_item);
    lv_obj_add_event_cb(new_item, subghz_remote_list_event, LV_EVENT_CLICKED, NULL);
    lv_group_add_obj(lv_group_get_default(), new_item);
    lv_obj_remove_style(new_item, NULL, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_color(new_item, lv_color_hex(EMBED_COLOR_FOCUS_ON), LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_width(new_item, 2, LV_STATE_FOCUS_KEY);

    // Set focus to the first item in the list
    if (lv_obj_get_child_cnt(subghz_remote_file_list) > 0) {
        lv_obj_t *first_item = lv_obj_get_child(subghz_remote_file_list, 0);
        if (first_item) {
            lv_group_focus_obj(first_item);
        }
    }

    // Force display refresh for portrait mode SPI sync
    lv_refr_now(NULL);
    tft.drawPixel(0, 0, 0);
}

// Show buttons from loaded remote (button list mode)
void subghz_remote_show_buttons()
{
    // Clear existing list
    lv_obj_clean(subghz_remote_file_list);

    // Update path label to show filename
    String filename = subghz_remote_current.filepath;
    int last_slash = filename.lastIndexOf('/');
    if (last_slash >= 0) {
        filename = filename.substring(last_slash + 1);
    }
    filename.replace(".txt", "");  // Remove extension

    // Update label based on mode
    char path_display[300];
    if (subghz_remote_mode == SUBGHZ_REMOTE_EDIT_BUTTON_LIST) {
        snprintf(path_display, sizeof(path_display), "Edit: %s", filename.c_str());
        lv_label_set_text(subghz_remote_path_label, path_display);
        lv_obj_set_style_text_color(subghz_remote_path_label, lv_color_hex(0xFF0000), LV_PART_MAIN);  // Red
    } else {
        snprintf(path_display, sizeof(path_display), "Remote: %s", filename.c_str());
        lv_label_set_text(subghz_remote_path_label, path_display);
        apply_text_color(subghz_remote_path_label);  // Normal color
    }

    // Add button items
    for (int i = 0; i < subghz_remote_current.buttons.size(); i++) {
        const SubGHzButton& button = subghz_remote_current.buttons[i];

        char item_text[256];
        snprintf(item_text, sizeof(item_text), " %s", button.name.c_str());

        lv_obj_t *item = lv_list_add_btn(subghz_remote_file_list, NULL, item_text);
        lv_obj_set_style_text_font(item, FONT_BOLD_14, LV_PART_MAIN);
        apply_text_color(item);
        apply_bg_color(item);
        lv_obj_set_user_data(item, (void*)(intptr_t)i);  // Store button index
        lv_obj_add_event_cb(item, subghz_remote_list_event, LV_EVENT_CLICKED, NULL);
        lv_obj_add_event_cb(item, subghz_remote_list_event, LV_EVENT_LONG_PRESSED, NULL);
        lv_obj_add_event_cb(item, subghz_remote_list_event, LV_EVENT_PRESSED, NULL);
        lv_group_add_obj(lv_group_get_default(), item);
        lv_obj_remove_style(item, NULL, LV_STATE_FOCUS_KEY);
        lv_obj_set_style_outline_color(item, lv_color_hex(EMBED_COLOR_FOCUS_ON), LV_STATE_FOCUS_KEY);
        lv_obj_set_style_outline_width(item, 2, LV_STATE_FOCUS_KEY);
    }

    // Add "(New Button)" item (only in normal button list mode, not in edit mode)
    if (subghz_remote_mode == SUBGHZ_REMOTE_BUTTON_LIST) {
        lv_obj_t *new_btn = lv_list_add_btn(subghz_remote_file_list, NULL, "(New Button)");
        lv_obj_set_style_text_font(new_btn, FONT_BOLD_14, LV_PART_MAIN);
        lv_obj_set_style_text_color(new_btn, lv_color_hex(0x00FF00), LV_PART_MAIN);
        apply_bg_color(new_btn);
        lv_obj_add_event_cb(new_btn, subghz_remote_list_event, LV_EVENT_CLICKED, NULL);
        lv_group_add_obj(lv_group_get_default(), new_btn);
        lv_obj_remove_style(new_btn, NULL, LV_STATE_FOCUS_KEY);
        lv_obj_set_style_outline_color(new_btn, lv_color_hex(EMBED_COLOR_FOCUS_ON), LV_STATE_FOCUS_KEY);
        lv_obj_set_style_outline_width(new_btn, 2, LV_STATE_FOCUS_KEY);
    }

    // Set focus to the first button item
    if (lv_obj_get_child_cnt(subghz_remote_file_list) > 0) {
        lv_obj_t *first_item = lv_obj_get_child(subghz_remote_file_list, 0);
        if (first_item) {
            lv_group_focus_obj(first_item);
        }
    }

    // Force display refresh (matching IR Playback pattern)
    tft.drawPixel(0, 0, 0);
}

// Back button event
static void scr2_5_back_btn_event_cb(lv_event_t *e)
{
    if (e->code == LV_EVENT_CLICKED) {
        if (subghz_remote_mode == SUBGHZ_REMOTE_BUTTON_LIST || subghz_remote_mode == SUBGHZ_REMOTE_EDIT_BUTTON_LIST) {
            // Go back to file browser
            subghz_remote_mode = SUBGHZ_REMOTE_FILE_BROWSER;
            subghz_remote_load_directory(subghz_remote_current_path);
        } else {
            // Go back to SubGHz menu
            exit2_5_anim(SCREEN2_ID, scr2_5_cont);
        }
    }
}

static void create2_5(lv_obj_t *parent)
{
    scr2_5_cont = lv_obj_create(parent);
    lv_obj_set_size(scr2_5_cont, 170, 320);
	lv_obj_set_style_transform_pivot_x(scr2_5_cont, 90, LV_PART_MAIN);
	lv_obj_set_style_transform_pivot_y(scr2_5_cont, 80, LV_PART_MAIN);
	lv_obj_set_style_transform_angle(scr2_5_cont, 2700, LV_PART_MAIN);  // 270° = 2700 (tenths of degree)

    apply_bg_color(scr2_5_cont);
    apply_no_scrollbar(scr2_5_cont);
    apply_no_border(scr2_5_cont);
    apply_no_padding(scr2_5_cont);

    // Title label
    lv_obj_t *label = lv_label_create(scr2_5_cont);
    apply_text_color(label);
    lv_obj_set_style_text_font(label, FONT_BOLD_18, LV_PART_MAIN);
    lv_label_set_text(label, "SubGHz");
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 10);

    // SubGHz LED indicator
    subghz_remote_led = lv_led_create(scr2_5_cont);
    lv_obj_align(subghz_remote_led, LV_ALIGN_TOP_RIGHT, -13, 13);
    lv_obj_set_size(subghz_remote_led, 15, 15);
    lv_led_set_color(subghz_remote_led, lv_palette_main(LV_PALETTE_GREEN));
    lv_led_off(subghz_remote_led);

    // Path label
    subghz_remote_path_label = lv_label_create(scr2_5_cont);
    apply_text_color(subghz_remote_path_label);
    lv_obj_set_style_text_font(subghz_remote_path_label, FONT_BOLD_14, LV_PART_MAIN);
    lv_label_set_text(subghz_remote_path_label, "Path: /sgremotes");
    lv_obj_set_width(subghz_remote_path_label, 160);
    lv_label_set_long_mode(subghz_remote_path_label, LV_LABEL_LONG_DOT);
    lv_obj_align(subghz_remote_path_label, LV_ALIGN_TOP_LEFT, 10, 35);

    // File/button list
    subghz_remote_file_list = lv_list_create(scr2_5_cont);
    lv_obj_set_size(subghz_remote_file_list, 170, 250);
    lv_obj_align(subghz_remote_file_list, LV_ALIGN_TOP_LEFT, 0, 60);
    apply_bg_color(subghz_remote_file_list);
    lv_obj_set_style_border_color(subghz_remote_file_list, lv_color_hex(EMBED_COLOR_BORDER), LV_PART_MAIN);

    subghz_remote_load_directory(subghz_remote_current_path);

    // Back button (using standard "<" button in top left) - Added last for default focus
    scr_back_btn_create(scr2_5_cont, scr2_5_back_btn_event_cb);
}

static void enter2_5(void)
{
    // Initialize SubGHz module if needed
    if (!subghz_is_init()) {
        subghz_init();
    }

    entry2_5_anim(scr2_5_cont);
    lv_group_set_wrap(lv_group_get_default(), true);

    // Refresh the UI based on current mode
    if (subghz_remote_mode == SUBGHZ_REMOTE_BUTTON_LIST || subghz_remote_mode == SUBGHZ_REMOTE_EDIT_BUTTON_LIST) {
        // Returning from Screen 2.5.1 - show button list (or edit button list)
        subghz_remote_show_buttons();
    } else {
        // Coming from elsewhere - show file browser
        subghz_remote_mode = SUBGHZ_REMOTE_FILE_BROWSER;
        subghz_remote_load_directory(subghz_remote_current_path);
    }

    // Prevent parent screen from scrolling when rotated container has focused items
    lv_obj_t *parent = lv_obj_get_parent(scr2_5_cont);
    if (parent) {
        lv_obj_clear_flag(parent, LV_OBJ_FLAG_SCROLLABLE);
    }
}

static void exit2_5(void)
{
    lv_group_set_wrap(lv_group_get_default(), false);

    // Re-enable scrolling on parent when exiting
    lv_obj_t *parent = lv_obj_get_parent(scr2_5_cont);
    if (parent) {
        lv_obj_add_flag(parent, LV_OBJ_FLAG_SCROLLABLE);
    }
}

static void destroy2_5(void)
{
    if (scr2_5_cont) {
        lv_obj_del(scr2_5_cont);
        scr2_5_cont = NULL;
    }
}

scr_lifecycle_t screen2_5 = {
    .create = create2_5,
    .entry = enter2_5,
    .exit = exit2_5,
    .destroy = destroy2_5,
};
#endif

//************************************[ screen 2.6 ]********************************** Custom Frequencies
#if 1
lv_obj_t *scr2_6_cont = NULL;
lv_obj_t *custom_freq_mgmt_list = NULL;
static bool custom_freq_long_press_occurred = false;
static int custom_freq_selected_index = -1;

void entry2_6_anim(lv_obj_t *obj) { entry1_anim(obj); }
void exit2_6_anim(int user_data, lv_obj_t *obj) { exit1_anim(user_data, obj); }

static void custom_freq_mgmt_list_event(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    int idx = (int)(intptr_t)lv_event_get_user_data(e);

    if (code == LV_EVENT_LONG_PRESSED) {
        custom_freq_long_press_occurred = true;

        // Only show context menu for actual frequencies, not the "Add Frequency" button
        if (idx < (int)g_config.subghz.custom_frequencies.size()) {
            custom_freq_selected_index = idx;

            char freq_str[32];
            snprintf(freq_str, sizeof(freq_str), "%.3f MHz", g_config.subghz.custom_frequencies[idx]);

            static const char * btns[] = {"Delete", "Cancel", ""};
            lv_obj_t * mbox = lv_msgbox_create(NULL, "Frequency Options", freq_str, btns, false);
            lv_obj_center(mbox);

            lv_obj_add_event_cb(mbox, [](lv_event_t *e) {
                lv_event_code_t code = lv_event_get_code(e);
                if (code == LV_EVENT_VALUE_CHANGED) {
                    lv_obj_t *mbox = lv_event_get_current_target(e);
                    const char *txt = lv_msgbox_get_active_btn_text(mbox);

                    if (txt && strcmp(txt, "Delete") == 0) {
                        if (custom_freq_selected_index >= 0 &&
                            custom_freq_selected_index < (int)g_config.subghz.custom_frequencies.size()) {
                            g_config.subghz.custom_frequencies.erase(
                                g_config.subghz.custom_frequencies.begin() + custom_freq_selected_index);
                            rf_invalidate_frequency_cache();
                            config_save();
                            prompt_info("  Frequency deleted", 1500);

                            lv_obj_clean(custom_freq_mgmt_list);

                            for (size_t i = 0; i < g_config.subghz.custom_frequencies.size(); i++) {
                                char freq_str[32];
                                snprintf(freq_str, sizeof(freq_str), " %.3f MHz", g_config.subghz.custom_frequencies[i]);

                                lv_obj_t *item = lv_list_add_btn(custom_freq_mgmt_list, NULL, freq_str);
                                lv_obj_set_style_text_font(item, FONT_BOLD_14, LV_PART_MAIN);
                                apply_text_color(item);
                                apply_bg_color(item);
                                lv_obj_add_event_cb(item, custom_freq_mgmt_list_event, LV_EVENT_CLICKED, (void*)(intptr_t)i);
                                lv_obj_add_event_cb(item, custom_freq_mgmt_list_event, LV_EVENT_LONG_PRESSED, (void*)(intptr_t)i);
                                lv_group_add_obj(lv_group_get_default(), item);

                                lv_obj_remove_style(item, NULL, LV_STATE_FOCUS_KEY);
                                lv_obj_set_style_outline_color(item, lv_color_hex(EMBED_COLOR_FOCUS_ON), LV_STATE_FOCUS_KEY);
                                lv_obj_set_style_outline_width(item, 2, LV_STATE_FOCUS_KEY);
                            }

                            lv_obj_t *add_item = lv_list_add_btn(custom_freq_mgmt_list, NULL, " + Add Frequency");
                            lv_obj_set_style_text_font(add_item, FONT_BOLD_14, LV_PART_MAIN);
                            lv_obj_set_style_text_color(add_item, lv_color_hex(0x00FF00), LV_PART_MAIN);
                            apply_bg_color(add_item);
                            lv_obj_add_event_cb(add_item, custom_freq_mgmt_list_event, LV_EVENT_CLICKED,
                                                (void*)(intptr_t)g_config.subghz.custom_frequencies.size());
                            lv_group_add_obj(lv_group_get_default(), add_item);

                            lv_obj_remove_style(add_item, NULL, LV_STATE_FOCUS_KEY);
                            lv_obj_set_style_outline_color(add_item, lv_color_hex(EMBED_COLOR_FOCUS_ON), LV_STATE_FOCUS_KEY);
                            lv_obj_set_style_outline_width(add_item, 2, LV_STATE_FOCUS_KEY);
                        }
                    }

                    lv_group_t *g = lv_group_get_default();
                    lv_group_set_editing(g, false);
                    lv_group_focus_freeze(g, false);
                    lv_msgbox_close(mbox);
                    lv_refr_now(NULL);

                    if (lv_group_get_focused(g) == NULL && lv_obj_get_child_cnt(custom_freq_mgmt_list) > 0) {
                        lv_obj_t *first_item = lv_obj_get_child(custom_freq_mgmt_list, 0);
                        if (first_item) {
                            lv_group_focus_obj(first_item);
                        }
                    }
                }
            }, LV_EVENT_VALUE_CHANGED, NULL);

            lv_group_t *g = lv_group_get_default();
            lv_obj_t *btnm = lv_msgbox_get_btns(mbox);

            if (btnm) {
                lv_group_add_obj(g, btnm);
                lv_group_focus_obj(btnm);
                lv_obj_add_state(btnm, LV_STATE_FOCUS_KEY);
                lv_group_set_editing(g, true);
            }
            lv_group_focus_freeze(g, true);
        }
    } else if (code == LV_EVENT_CLICKED) {
        if (custom_freq_long_press_occurred) {
            custom_freq_long_press_occurred = false;
            return;
        }

        // Check if this is the "Add Frequency" item (last item)
        if (idx == (int)g_config.subghz.custom_frequencies.size()) {
            exit2_6_anim(SCREEN2_6_1_ID, scr2_6_cont);
        }
    }
}

static void scr2_6_back_btn_event_cb(lv_event_t *e)
{
    if (e->code == LV_EVENT_CLICKED) {
        exit2_6_anim(SCREEN2_ID, scr2_6_cont);
    }
}

static void create2_6(lv_obj_t *parent)
{
    scr2_6_cont = create_screen_container(parent);

    // Title
    lv_obj_t *label = lv_label_create(scr2_6_cont);
    apply_text_color(label);
    lv_obj_set_style_text_font(label, FONT_BOLD_16, LV_PART_MAIN);
    lv_label_set_text(label, "Custom Frequencies");
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 10);

    // Frequency list
    custom_freq_mgmt_list = lv_list_create(scr2_6_cont);
    lv_obj_set_size(custom_freq_mgmt_list, LV_HOR_RES - 10, 125);
    lv_obj_align(custom_freq_mgmt_list, LV_ALIGN_BOTTOM_MID, 0, -5);
    apply_bg_color(custom_freq_mgmt_list);
    lv_obj_set_style_pad_top(custom_freq_mgmt_list, 2, LV_PART_MAIN);
    lv_obj_set_style_pad_row(custom_freq_mgmt_list, 2, LV_PART_MAIN);
    apply_no_radius(custom_freq_mgmt_list);
    apply_no_border(custom_freq_mgmt_list);
    apply_no_shadow(custom_freq_mgmt_list);

    // Add all custom frequencies
    for (size_t i = 0; i < g_config.subghz.custom_frequencies.size(); i++) {
        char freq_str[32];
        snprintf(freq_str, sizeof(freq_str), " %.3f MHz", g_config.subghz.custom_frequencies[i]);

        lv_obj_t *item = lv_list_add_btn(custom_freq_mgmt_list, NULL, freq_str);
        lv_obj_set_style_text_font(item, FONT_BOLD_14, LV_PART_MAIN);
        apply_text_color(item);
        apply_bg_color(item);
        lv_obj_add_event_cb(item, custom_freq_mgmt_list_event, LV_EVENT_CLICKED, (void*)(intptr_t)i);
        lv_obj_add_event_cb(item, custom_freq_mgmt_list_event, LV_EVENT_LONG_PRESSED, (void*)(intptr_t)i);
        lv_group_add_obj(lv_group_get_default(), item);

        lv_obj_remove_style(item, NULL, LV_STATE_FOCUS_KEY);
        lv_obj_set_style_outline_color(item, lv_color_hex(EMBED_COLOR_FOCUS_ON), LV_STATE_FOCUS_KEY);
        lv_obj_set_style_outline_width(item, 2, LV_STATE_FOCUS_KEY);
    }

    // Add "Add Frequency" button at the end
    lv_obj_t *add_item = lv_list_add_btn(custom_freq_mgmt_list, NULL, " + Add Frequency");
    lv_obj_set_style_text_font(add_item, FONT_BOLD_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(add_item, lv_color_hex(0x00FF00), LV_PART_MAIN);
    apply_bg_color(add_item);
    lv_obj_add_event_cb(add_item, custom_freq_mgmt_list_event, LV_EVENT_CLICKED,
                        (void*)(intptr_t)g_config.subghz.custom_frequencies.size());
    lv_group_add_obj(lv_group_get_default(), add_item);

    lv_obj_remove_style(add_item, NULL, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_color(add_item, lv_color_hex(EMBED_COLOR_FOCUS_ON), LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_width(add_item, 2, LV_STATE_FOCUS_KEY);

    // Back button
    scr_back_btn_create(scr2_6_cont, scr2_6_back_btn_event_cb);
}

static void enter2_6(void)
{
    // Initialize SubGHz module if needed
    if (!subghz_is_init()) {
        subghz_init();
    }

    entry2_6_anim(scr2_6_cont);
    lv_group_set_wrap(lv_group_get_default(), true);
}

static void exit2_6(void)
{
    lv_group_set_wrap(lv_group_get_default(), false);
}

static void destroy2_6(void)
{
    if (scr2_6_cont) {
        lv_obj_del(scr2_6_cont);
        scr2_6_cont = NULL;
    }
}

scr_lifecycle_t screen2_6 = {
    .create = create2_6,
    .entry = enter2_6,
    .exit = exit2_6,
    .destroy = destroy2_6,
};
#endif

//************************************[ screen 2.6.1 ]********************************** Add Custom Frequency
#if 1
lv_obj_t *scr2_6_1_cont = NULL;
lv_obj_t *add_freq_textarea = NULL;
lv_obj_t *add_freq_keyboard = NULL;

void entry2_6_1_anim(lv_obj_t *obj) { entry1_anim(obj); }
void exit2_6_1_anim(int user_data, lv_obj_t *obj) { exit1_anim(user_data, obj); }

static void add_freq_keyboard_event(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *kb = lv_event_get_target(e);

    if (code == LV_EVENT_READY) {
        // User pressed OK - save the frequency
        const char *text = lv_textarea_get_text(add_freq_textarea);
        float freq = atof(text);

        // Validate frequency is within CC1101's supported ranges
        bool valid_range = (freq >= 300.0f && freq <= 348.0f) ||
                          (freq >= 387.0f && freq <= 464.0f) ||
                          (freq >= 779.0f && freq <= 928.0f);

        if (valid_range) {
            // Check for duplicates (both in hardcoded and custom lists)
            bool is_duplicate = false;

            // Check hardcoded frequencies (56 total)
            for (int i = 0; i < 56; i++) {
                if (fabs(subghz_frequency_list[i] - freq) < 0.01f) {
                    is_duplicate = true;
                    break;
                }
            }

            // Check custom frequencies
            if (!is_duplicate) {
                for (size_t i = 0; i < g_config.subghz.custom_frequencies.size(); i++) {
                    if (fabs(g_config.subghz.custom_frequencies[i] - freq) < 0.01f) {
                        is_duplicate = true;
                        break;
                    }
                }
            }

            if (!is_duplicate) {
                g_config.subghz.custom_frequencies.push_back(freq);
                rf_invalidate_frequency_cache();
                config_save();
                prompt_info("  Frequency added", 1500);
            } else {
                prompt_info("  Frequency already exists", 2000);
            }
        } else {
            prompt_info("  Invalid frequency range", 2000);
        }

        // Clean up keyboard and textarea
        lv_obj_del(add_freq_keyboard);
        lv_obj_del(add_freq_textarea);
        add_freq_keyboard = NULL;
        add_freq_textarea = NULL;

        // Go back to Custom Frequencies screen
        exit2_6_1_anim(SCREEN2_6_ID, scr2_6_1_cont);
    }
    else if (code == LV_EVENT_CANCEL) {
        // User pressed Cancel
        lv_obj_del(add_freq_keyboard);
        lv_obj_del(add_freq_textarea);
        add_freq_keyboard = NULL;
        add_freq_textarea = NULL;

        // Go back to Custom Frequencies screen
        exit2_6_1_anim(SCREEN2_6_ID, scr2_6_1_cont);
    }
}

static void create2_6_1(lv_obj_t *parent)
{
    scr2_6_1_cont = create_screen_container(parent);

    // Title
    lv_obj_t *label = lv_label_create(scr2_6_1_cont);
    apply_text_color(label);
    lv_obj_set_style_text_font(label, FONT_BOLD_16, LV_PART_MAIN);
    lv_label_set_text(label, "Add Frequency (MHz)");
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 10);
}

static void enter2_6_1(void)
{
    entry2_6_1_anim(scr2_6_1_cont);

    // Get the correct parent screen (scr2_6_1_cont's parent)
    lv_obj_t *parent = lv_obj_get_parent(scr2_6_1_cont);

    // Create text area for frequency input
    add_freq_textarea = lv_textarea_create(parent);
    lv_obj_set_size(add_freq_textarea, LV_PCT(90), 40);
    lv_obj_align(add_freq_textarea, LV_ALIGN_TOP_MID, 0, 40);
    lv_textarea_set_one_line(add_freq_textarea, true);
    lv_textarea_set_text(add_freq_textarea, "");
    lv_textarea_set_placeholder_text(add_freq_textarea, "e.g. 433.92");

    // Enable and style the cursor for visibility
    lv_obj_set_style_anim_time(add_freq_textarea, 400, LV_PART_CURSOR);
    lv_obj_set_style_bg_opa(add_freq_textarea, LV_OPA_COVER, LV_PART_CURSOR);
    lv_obj_set_style_bg_color(add_freq_textarea, lv_color_hex(EMBED_COLOR_FOCUS_ON), LV_PART_CURSOR);
    lv_obj_set_style_border_width(add_freq_textarea, 1, LV_PART_CURSOR);
    lv_obj_set_style_border_color(add_freq_textarea, lv_color_hex(EMBED_COLOR_FOCUS_ON), LV_PART_CURSOR);
    lv_textarea_set_cursor_pos(add_freq_textarea, LV_TEXTAREA_CURSOR_LAST);

    // Create numeric keyboard
    add_freq_keyboard = lv_keyboard_create(parent);
    lv_keyboard_set_mode(add_freq_keyboard, LV_KEYBOARD_MODE_NUMBER);
    lv_keyboard_set_textarea(add_freq_keyboard, add_freq_textarea);
    lv_obj_add_event_cb(add_freq_keyboard, add_freq_keyboard_event, LV_EVENT_ALL, NULL);

    // Setup focus and navigation
    lv_group_t *g = lv_group_get_default();
    lv_group_add_obj(g, add_freq_textarea);
    lv_group_add_obj(g, add_freq_keyboard);
    lv_group_focus_obj(add_freq_keyboard);
    lv_obj_add_state(add_freq_keyboard, LV_STATE_FOCUS_KEY);
    lv_group_set_editing(g, true);
    lv_group_focus_freeze(g, true);
}

static void exit2_6_1(void)
{
    // Unfreeze focus when leaving
    lv_group_t *g = lv_group_get_default();
    lv_group_focus_freeze(g, false);
    lv_group_set_editing(g, false);

    // Remove objects from group before they're destroyed
    if (add_freq_textarea != NULL) {
        lv_group_remove_obj(add_freq_textarea);
    }
    if (add_freq_keyboard != NULL) {
        lv_group_remove_obj(add_freq_keyboard);
    }
}

static void destroy2_6_1(void)
{
    // Clean up keyboard and textarea if they exist
    // Note: These are created on lv_scr_act(), not on scr2_6_1_cont,
    // so they need to be deleted first before the container
    if (add_freq_keyboard != NULL) {
        lv_obj_del(add_freq_keyboard);
        add_freq_keyboard = NULL;
    }
    if (add_freq_textarea != NULL) {
        lv_obj_del(add_freq_textarea);
        add_freq_textarea = NULL;
    }

    if (scr2_6_1_cont != NULL) {
        lv_obj_del(scr2_6_1_cont);
        scr2_6_1_cont = NULL;
    }
}

scr_lifecycle_t screen2_6_1 = {
    .create = create2_6_1,
    .entry = enter2_6_1,
    .exit = exit2_6_1,
    .destroy = destroy2_6_1,
};
#endif

//************************************[ screen 2.5.1 ]************************************** SubGHz File Browser
#if 1
// File browser for selecting .sub files when creating/editing buttons
lv_obj_t *scr2_5_1_cont;
lv_obj_t *subghz_fb_title_label;
lv_obj_t *subghz_fb_file_list;
lv_obj_t *subghz_fb_keyboard = NULL;
lv_obj_t *subghz_fb_textarea = NULL;
int subghz_fb_editing_button_index = -1;  // -1 = new button, >= 0 = editing existing
String subghz_fb_selected_file = "";      // Selected .sub file path
bool subghz_fb_long_press_occurred = false;
bool subghz_fb_file_selected = false;     // Flag to indicate if file has been selected
bool subghz_fb_from_edit_mode = false;    // Track if we came from edit mode

void entry2_5_1_anim(lv_obj_t *obj) { entry1_anim(obj); }
void exit2_5_1_anim(int user_data, lv_obj_t *obj) { exit1_anim(user_data, obj); }

// Forward declarations
void subghz_fb_load_files();
static void subghz_fb_name_keyboard_event(lv_event_t *e);

// Keyboard event handler for button naming
static void subghz_fb_name_keyboard_event(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_READY) {
        // User pressed OK - save button with entered name
        const char *button_name = lv_textarea_get_text(subghz_fb_textarea);

        if (strlen(button_name) > 0 && subghz_fb_selected_file.length() > 0) {
            SubGHzButton new_button;
            new_button.name = String(button_name);
            new_button.filepath = subghz_fb_selected_file;

            // Add or replace button
            if (subghz_fb_editing_button_index >= 0) {
                // Replace existing button
                replaceButtonInSubGHzRemote(subghz_remote_current.filepath.c_str(),
                                           subghz_fb_editing_button_index,
                                           new_button);
            } else {
                // Append new button
                appendButtonToSubGHzRemote(subghz_remote_current.filepath.c_str(), new_button);
            }

            // Reload remote to get updated button list
            // Save filepath before parsing since parseSubGHzRemoteFile calls remote.clear()
            String remote_filepath = subghz_remote_current.filepath;
            parseSubGHzRemoteFile(remote_filepath.c_str(), subghz_remote_current);

            // Ensure we stay in button list mode
            subghz_remote_mode = SUBGHZ_REMOTE_BUTTON_LIST;
        }

        // Clean up keyboard
        lv_group_t *g = lv_group_get_default();
        lv_group_set_editing(g, false);
        lv_group_focus_freeze(g, false);

        if (subghz_fb_keyboard) {
            lv_obj_del(subghz_fb_keyboard);
            subghz_fb_keyboard = NULL;
        }
        if (subghz_fb_textarea) {
            lv_obj_del(subghz_fb_textarea);
            subghz_fb_textarea = NULL;
        }

        lv_refr_now(NULL);
        tft.drawPixel(0, 0, 0); // Dummy write to sync SPI

        // Return to button list screen (enter2_5 will call subghz_remote_show_buttons)
        exit2_5_1_anim(SCREEN2_5_ID, scr2_5_1_cont);

    } else if (code == LV_EVENT_CANCEL) {
        // User cancelled - just clean up keyboard and stay on file browser
        lv_group_t *g = lv_group_get_default();
        lv_group_set_editing(g, false);
        lv_group_focus_freeze(g, false);

        if (subghz_fb_keyboard) {
            lv_obj_del(subghz_fb_keyboard);
            subghz_fb_keyboard = NULL;
        }
        if (subghz_fb_textarea) {
            lv_obj_del(subghz_fb_textarea);
            subghz_fb_textarea = NULL;
        }

        lv_refr_now(NULL);
        tft.drawPixel(0, 0, 0); // Dummy write to sync SPI

        // Reset file selection flag so user can select again
        subghz_fb_file_selected = false;

        // Restore focus to file list
        if (lv_group_get_focused(g) == NULL && lv_obj_get_child_cnt(subghz_fb_file_list) > 0) {
            lv_obj_t *first_item = lv_obj_get_child(subghz_fb_file_list, 0);
            if (first_item) {
                lv_group_focus_obj(first_item);
            }
        }
    }
}

// Show keyboard for naming the button
static void subghz_fb_show_name_keyboard()
{
    // Generate default name
    String default_name;
    if (subghz_fb_editing_button_index >= 0) {
        // Editing existing button - use current name
        default_name = subghz_remote_current.buttons[subghz_fb_editing_button_index].name;
    } else {
        // New button - generate sequential name
        default_name = generateSubGHzButtonName(subghz_remote_current);
    }

    // Create text area for input
    subghz_fb_textarea = lv_textarea_create(lv_scr_act());
    lv_obj_set_size(subghz_fb_textarea, LV_PCT(90), 40);
    lv_obj_align(subghz_fb_textarea, LV_ALIGN_TOP_MID, 0, 40);
    lv_textarea_set_one_line(subghz_fb_textarea, true);
    lv_textarea_set_text(subghz_fb_textarea, default_name.c_str());

    // Enable and style the cursor for visibility
    lv_obj_set_style_anim_time(subghz_fb_textarea, 400, LV_PART_CURSOR);
    lv_obj_set_style_bg_opa(subghz_fb_textarea, LV_OPA_COVER, LV_PART_CURSOR);
    lv_obj_set_style_bg_color(subghz_fb_textarea, lv_color_hex(EMBED_COLOR_FOCUS_ON), LV_PART_CURSOR);
    lv_obj_set_style_border_width(subghz_fb_textarea, 1, LV_PART_CURSOR);
    lv_obj_set_style_border_color(subghz_fb_textarea, lv_color_hex(EMBED_COLOR_FOCUS_ON), LV_PART_CURSOR);
    lv_textarea_set_cursor_pos(subghz_fb_textarea, LV_TEXTAREA_CURSOR_LAST);

    // Create keyboard
    subghz_fb_keyboard = lv_keyboard_create(lv_scr_act());
    lv_keyboard_set_textarea(subghz_fb_keyboard, subghz_fb_textarea);
    lv_obj_add_event_cb(subghz_fb_keyboard, subghz_fb_name_keyboard_event, LV_EVENT_ALL, NULL);

    // Setup focus and navigation
    lv_group_t *g = lv_group_get_default();
    lv_group_add_obj(g, subghz_fb_textarea);
    lv_group_add_obj(g, subghz_fb_keyboard);
    lv_group_focus_obj(subghz_fb_keyboard);
    lv_obj_add_state(subghz_fb_keyboard, LV_STATE_FOCUS_KEY);
    lv_group_set_editing(g, true);
    lv_group_focus_freeze(g, true);
}

// File list event handler
static void subghz_fb_list_event(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *btn = lv_event_get_target(e);
    const char *label_text = lv_list_get_btn_text(subghz_fb_file_list, btn);

    if (code == LV_EVENT_PRESSED) {
        subghz_fb_long_press_occurred = false;
    }

    if (code == LV_EVENT_CLICKED) {
        if (subghz_fb_long_press_occurred) {
            subghz_fb_long_press_occurred = false;
            return;
        }

        // Check for back button
        if (strstr(label_text, "<-")) {
            // Return to button list without saving
            exit2_5_1_anim(SCREEN2_5_ID, scr2_5_1_cont);
            return;
        }

        // File was selected
        subghz_fb_selected_file = String("/rf/") + label_text;
        subghz_fb_file_selected = true;

        // If editing button from edit mode, skip naming and just update the .sub file
        if (subghz_fb_from_edit_mode && subghz_fb_editing_button_index >= 0) {
            // Keep existing button name, just change the .sub file
            SubGHzButton updated_button;
            updated_button.name = subghz_remote_current.buttons[subghz_fb_editing_button_index].name;
            updated_button.filepath = subghz_fb_selected_file;

            // Replace button
            replaceButtonInSubGHzRemote(subghz_remote_current.filepath.c_str(),
                                       subghz_fb_editing_button_index,
                                       updated_button);

            // Reload remote
            String remote_filepath = subghz_remote_current.filepath;
            parseSubGHzRemoteFile(remote_filepath.c_str(), subghz_remote_current);

            // Restore edit mode
            subghz_remote_mode = SUBGHZ_REMOTE_EDIT_BUTTON_LIST;

            // Return to edit mode
            exit2_5_1_anim(SCREEN2_5_ID, scr2_5_1_cont);
        } else {
            // Show keyboard to name the button (for new buttons or normal mode editing)
            subghz_fb_show_name_keyboard();
        }
    }
}

// Load .sub files from /rf directory
void subghz_fb_load_files()
{
    // Clear existing list
    lv_obj_clean(subghz_fb_file_list);

    // Add back button
    lv_obj_t *back_item = lv_list_add_btn(subghz_fb_file_list, NULL, "<- Back");
    lv_obj_set_style_text_font(back_item, FONT_BOLD_14, LV_PART_MAIN);
    apply_text_color(back_item);
    apply_bg_color(back_item);
    lv_obj_add_event_cb(back_item, subghz_fb_list_event, LV_EVENT_CLICKED, NULL);
    lv_group_add_obj(lv_group_get_default(), back_item);
    lv_obj_remove_style(back_item, NULL, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_color(back_item, lv_color_hex(EMBED_COLOR_FOCUS_ON), LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_width(back_item, 2, LV_STATE_FOCUS_KEY);

    // Ensure /rf directory exists (create if needed)
    if (!SD.exists("/rf")) {
        if (!SD.mkdir("/rf")) {
            return;
        }
    }

    // Try multiple mount points (SD card VFS mount location)
    const char* mount_points[] = {"/sd", "/sdcard", "/mnt/sd"};
    DIR *dir = NULL;

    for (int i = 0; i < 3; i++) {
        char test_path[512];
        snprintf(test_path, sizeof(test_path), "%s/rf", mount_points[i]);
        dir = opendir(test_path);
        if (dir) {
            break;
        }
    }

    // Fallback: try direct path
    if (!dir) {
        dir = opendir("/rf");
        if (dir) {
        }
    }

    if (dir) {
        struct dirent *entry;
        std::vector<String> files;

        // Read all .sub files (top-level only, no subdirectories)
        while ((entry = readdir(dir)) != NULL) {
            String filename = entry->d_name;

            // Skip hidden files and directories
            if (filename.startsWith(".")) continue;
            if (entry->d_type == DT_DIR) continue;  // Skip directories

            // Only show .sub files
            if (filename.endsWith(".sub")) {
                files.push_back(filename);
            }
        }
        closedir(dir);

        // Sort files alphabetically
        std::sort(files.begin(), files.end());

        // Add file items to list
        for (const String& file : files) {
            lv_obj_t *item = lv_list_add_btn(subghz_fb_file_list, NULL, file.c_str());
            lv_obj_set_style_text_font(item, FONT_BOLD_14, LV_PART_MAIN);
            apply_text_color(item);
            apply_bg_color(item);
            lv_obj_add_event_cb(item, subghz_fb_list_event, LV_EVENT_CLICKED, NULL);
            lv_obj_add_event_cb(item, subghz_fb_list_event, LV_EVENT_PRESSED, NULL);
            lv_group_add_obj(lv_group_get_default(), item);
            lv_obj_remove_style(item, NULL, LV_STATE_FOCUS_KEY);
            lv_obj_set_style_outline_color(item, lv_color_hex(EMBED_COLOR_FOCUS_ON), LV_STATE_FOCUS_KEY);
            lv_obj_set_style_outline_width(item, 2, LV_STATE_FOCUS_KEY);
        }

    } else {
    }
}

// Back button event
static void scr2_5_1_back_btn_event_cb(lv_event_t *e)
{
    if (e->code == LV_EVENT_CLICKED) {
        exit2_5_1_anim(SCREEN2_5_ID, scr2_5_1_cont);
    }
}

static void create2_5_1(lv_obj_t *parent)
{
    scr2_5_1_cont = create_screen_container(parent);

    // Title
    subghz_fb_title_label = lv_label_create(scr2_5_1_cont);
    apply_text_color(subghz_fb_title_label);
    lv_obj_set_style_text_font(subghz_fb_title_label, FONT_BOLD_16, LV_PART_MAIN);
    lv_label_set_text(subghz_fb_title_label, "Select .sub File");
    lv_obj_align(subghz_fb_title_label, LV_ALIGN_TOP_MID, 0, 10);

    // File list
    subghz_fb_file_list = lv_list_create(scr2_5_1_cont);
    lv_obj_set_size(subghz_fb_file_list, LV_HOR_RES - 10, LV_VER_RES - 70);
    lv_obj_align(subghz_fb_file_list, LV_ALIGN_TOP_MID, 0, 35);
    apply_bg_color(subghz_fb_file_list);
    lv_obj_set_style_pad_top(subghz_fb_file_list, 2, LV_PART_MAIN);
    lv_obj_set_style_pad_row(subghz_fb_file_list, 2, LV_PART_MAIN);
    apply_no_radius(subghz_fb_file_list);
    apply_no_border(subghz_fb_file_list);
    apply_no_shadow(subghz_fb_file_list);

    // Back button
    scr_back_btn_create(scr2_5_1_cont, scr2_5_1_back_btn_event_cb);
}

static void enter2_5_1(void)
{
    entry2_5_1_anim(scr2_5_1_cont);

    // Load .sub files from /rf directory
    subghz_fb_load_files();

    lv_group_set_wrap(lv_group_get_default(), true);
}

static void exit2_5_1(void)
{
    lv_group_set_wrap(lv_group_get_default(), false);
}

static void destroy2_5_1(void)
{
    if (scr2_5_1_cont) {
        lv_obj_del(scr2_5_1_cont);
        scr2_5_1_cont = NULL;
    }
}

scr_lifecycle_t screen2_5_1 = {
    .create = create2_5_1,
    .entry = enter2_5_1,
    .exit = exit2_5_1,
    .destroy = destroy2_5_1,
};

// Entry point function - called when user clicks "(New Button)" or "Edit" on button
void enter_subghz_file_browser(int button_index)
{
    subghz_fb_editing_button_index = button_index;
    subghz_fb_selected_file = "";
    subghz_fb_file_selected = false;
    subghz_fb_from_edit_mode = (subghz_remote_mode == SUBGHZ_REMOTE_EDIT_BUTTON_LIST);

    // Navigate to file browser screen
    scr_mgr_push(SCREEN2_5_1_ID, true);
}
#endif

//************************************[ screen 3 ]****************************************** NFC File Browser
#if 1
lv_obj_t *scr3_cont;
lv_obj_t *nfc_file_list;
lv_obj_t *nfc_path_label;
lv_obj_t *nfc_status_led;
char nfc_current_path[256] = "/nfc";
NFCTag nfc_current_tag;
char nfc_selected_file[128] = "";

void entry3_anim(lv_obj_t *obj) { entry1_anim(obj); }
void exit3_anim(int user_data, lv_obj_t *obj) { exit1_anim(user_data, obj); }

// Forward declarations
void nfc_load_directory(const char *path);

static void nfc_list_event_cb(lv_event_t *e)
{
    if (e->code == LV_EVENT_CLICKED) {
        lv_obj_t *btn = lv_event_get_target(e);
        const char *item_text = lv_list_get_btn_text(nfc_file_list, btn);

        if (item_text) {

            // Handle "(Read NFC Tag)" item
            if (strcmp(item_text, "(Read NFC Tag)") == 0) {
                scr_mgr_switch(SCREEN3_1_ID, false);
                return;
            }

            // Handle parent directory
            if (strstr(item_text, ".. (Parent)")) {
                // Go up one directory
                char *last_slash = strrchr(nfc_current_path, '/');
                if (last_slash && last_slash != nfc_current_path) {
                    *last_slash = '\0';
                } else {
                    strcpy(nfc_current_path, "/nfc");
                }
                nfc_load_directory(nfc_current_path);
                return;
            }

            // Handle file/folder (strip leading space)
            const char *display_name = item_text + 1;

            // Check if it's a folder
            char test_path[512];
            snprintf(test_path, sizeof(test_path), "%s/%s", nfc_current_path, display_name);
            DIR *test_dir = opendir(test_path);

            if (test_dir) {
                // It's a folder
                closedir(test_dir);
                strncpy(nfc_current_path, test_path, sizeof(nfc_current_path) - 1);
                nfc_load_directory(nfc_current_path);
            } else {
                // It's a file - add .nfc extension and load
                char file_name[128];
                snprintf(file_name, sizeof(file_name), "%s.nfc", display_name);

                char full_path[512];
                snprintf(full_path, sizeof(full_path), "%s/%s", nfc_current_path, file_name);

                // Parse NFC file
                lv_led_on(nfc_status_led);
                if (parseFlipperNFCFile(full_path, nfc_current_tag)) {
                    lv_led_off(nfc_status_led);
                    // Switch to tag detail screen
                    scr_mgr_switch(SCREEN3_2_ID, false);
                } else {
                    lv_led_off(nfc_status_led);
                    prompt_info("  Error parsing NFC file", 2000);
                }
            }
        }
    }
}

static void scr3_back_btn_event_cb(lv_event_t *e)
{
    if (e->code == LV_EVENT_CLICKED) {
        exit3_anim(SCREEN0_ID, scr3_cont);
    }
}

void nfc_load_directory(const char *path)
{
    lv_led_on(nfc_status_led);
    lv_refr_now(NULL);

    lv_obj_clean(nfc_file_list);
    lv_label_set_text_fmt(nfc_path_label, "Path: %s", path);

    // Add parent directory option if not at root
    if (strcmp(path, "/nfc") != 0) {
        lv_obj_t *parent_item = lv_list_add_btn(nfc_file_list, NULL, " .. (Parent)");
        lv_obj_add_event_cb(parent_item, nfc_list_event_cb, LV_EVENT_CLICKED, NULL);
    }

    // Create directory if it doesn't exist
    if (!SD.exists(path)) {
        SD.mkdir(path);
    }

    // Try multiple mount points (SD card VFS mount location)
    const char* mount_points[] = {"/sd", "/sdcard", "/mnt/sd"};
    DIR *dir = NULL;

    for (int i = 0; i < 3; i++) {
        char test_path[512];
        snprintf(test_path, sizeof(test_path), "%s%s", mount_points[i], path);
        dir = opendir(test_path);
        if (dir) {
            break;
        }
    }

    // Fallback: try direct path
    if (!dir) {
        dir = opendir(path);
        if (dir) {
        }
    }

    if (!dir) {
        lv_obj_t *item = lv_list_add_btn(nfc_file_list, NULL, " Directory not found");
        apply_text_color(item);
        lv_led_off(nfc_status_led);
        return;
    }

    // Read directory contents
    std::vector<String> folders;
    std::vector<String> files;

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.') continue;  // Skip hidden files

        // Check if it's a directory
        char full_path[512];
        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);

        DIR *test_dir = opendir(full_path);
        if (test_dir) {
            closedir(test_dir);
            folders.push_back(String(entry->d_name));
        } else if (strstr(entry->d_name, ".nfc")) {
            // Remove .nfc extension for display
            String fname = String(entry->d_name);
            fname.replace(".nfc", "");
            files.push_back(fname);
        }
    }
    closedir(dir);

    // Add folders first
    for (const auto& folder : folders) {
        char item_text[128];
        snprintf(item_text, sizeof(item_text), " %s", folder.c_str());
        lv_obj_t *item = lv_list_add_btn(nfc_file_list, NULL, item_text);
        lv_obj_add_event_cb(item, nfc_list_event_cb, LV_EVENT_CLICKED, NULL);
    }

    // Add files
    for (const auto& file : files) {
        char item_text[128];
        snprintf(item_text, sizeof(item_text), " %s", file.c_str());
        lv_obj_t *item = lv_list_add_btn(nfc_file_list, NULL, item_text);
        lv_obj_add_event_cb(item, nfc_list_event_cb, LV_EVENT_CLICKED, NULL);
    }

    // Add "(Read NFC Tag)" option at the end
    lv_obj_t *read_item = lv_list_add_btn(nfc_file_list, NULL, "(Read NFC Tag)");
    lv_obj_add_event_cb(read_item, nfc_list_event_cb, LV_EVENT_CLICKED, NULL);

    // Style all list items
    for(int i = 0; i < lv_obj_get_child_cnt(nfc_file_list); i++) {
        lv_obj_t *item = lv_obj_get_child(nfc_file_list, i);
        apply_bg_color(item);
        apply_text_color(item);
        lv_obj_set_style_text_font(item, FONT_BOLD_14, LV_PART_MAIN);
        lv_obj_set_style_bg_color(item, lv_color_hex(EMBED_COLOR_FOCUS_ON), LV_STATE_FOCUS_KEY);
        lv_group_add_obj(lv_group_get_default(), item);
    }

    lv_led_off(nfc_status_led);
}

static void create3(lv_obj_t *parent)
{
    scr3_cont = create_screen_container(parent);

    // Title label
    lv_obj_t *label = lv_label_create(scr3_cont);
    apply_text_color(label);
    lv_obj_set_style_text_font(label, FONT_BOLD_14, LV_PART_MAIN);
    lv_label_set_text(label, "NFC Tags (beta)");
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 10);

    // Status LED
    nfc_status_led = lv_led_create(scr3_cont);
    lv_obj_align(nfc_status_led, LV_ALIGN_TOP_RIGHT, -15, 10);
    lv_obj_set_size(nfc_status_led, 16, 16);
    lv_led_set_color(nfc_status_led, lv_palette_main(LV_PALETTE_BLUE));
    lv_led_off(nfc_status_led);

    // Path label
    nfc_path_label = lv_label_create(scr3_cont);
    apply_text_color(nfc_path_label);
    lv_obj_set_style_text_font(nfc_path_label, FONT_BOLD_14, LV_PART_MAIN);
    lv_label_set_text(nfc_path_label, "Path: /nfc");
    lv_obj_set_width(nfc_path_label, 280);
    lv_label_set_long_mode(nfc_path_label, LV_LABEL_LONG_DOT);
    lv_obj_align(nfc_path_label, LV_ALIGN_TOP_LEFT, 10, 35);

    // File list
    nfc_file_list = lv_list_create(scr3_cont);
    lv_obj_set_size(nfc_file_list, LV_HOR_RES, 135);
    lv_obj_align(nfc_file_list, LV_ALIGN_BOTTOM_MID, 0, 0);
    apply_bg_color(nfc_file_list);
    lv_obj_set_style_pad_row(nfc_file_list, 2, LV_PART_MAIN);
    apply_no_radius(nfc_file_list);
    apply_no_border(nfc_file_list);
    apply_no_shadow(nfc_file_list);

    nfc_load_directory(nfc_current_path);

    // Back button
    scr_back_btn_create(scr3_cont, scr3_back_btn_event_cb);
    entry3_anim(scr3_cont);
}

static void entry3(void)
{
    lv_group_set_wrap(lv_group_get_default(), true);

    // Reload directory
    nfc_load_directory(nfc_current_path);
}

static void exit3(void)
{
    lv_group_set_wrap(lv_group_get_default(), false);
}

static void destroy3(void)
{
    if (scr3_cont) {
        lv_obj_del(scr3_cont);
        scr3_cont = NULL;
    }
}

scr_lifecycle_t screen3 = {
    .create = create3,
    .entry = entry3,
    .exit = exit3,
    .destroy = destroy3,
};
#endif

//************************************[ screen 3.1 ]****************************************** Read NFC Tag
#if 1
lv_obj_t *scr3_1_cont;
lv_obj_t *nfc_read_status_label;
lv_obj_t *nfc_read_protocol_label;
lv_obj_t *nfc_read_data_label;
lv_obj_t *nfc_read_name_label;
lv_obj_t *nfc_read_pages_label;
lv_obj_t *nfc_btn_read;
lv_obj_t *nfc_btn_emulate;
lv_obj_t *nfc_btn_name;
lv_obj_t *nfc_btn_save;
lv_obj_t *nfc_btn_back;
lv_obj_t *nfc_keyboard = NULL;
lv_obj_t *nfc_textarea = NULL;
NFCTag nfc_read_tag;
bool nfc_tag_captured = false;
String nfc_tag_name = "";

void entry3_1_anim(lv_obj_t *obj) { entry1_anim(obj); }
void exit3_1_anim(int user_data, lv_obj_t *obj) { exit1_anim(user_data, obj); }

extern Adafruit_PN532 nfc;

// Forward declaration
static void nfc_name_keyboard_event(lv_event_t *e);

// Keyboard event handler for NFC tag name
static void nfc_name_keyboard_event(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_READY) {
        // User pressed OK
        const char *new_name = lv_textarea_get_text(nfc_textarea);

        if (strlen(new_name) > 0) {
            nfc_tag_name = new_name;

            // Update the name label
            char buf[128];
            snprintf(buf, sizeof(buf), "Name: %s", nfc_tag_name.c_str());
            lv_label_set_text(nfc_read_name_label, buf);
        }

        // Clean up keyboard
        lv_group_t *g = lv_group_get_default();
        lv_group_set_editing(g, false);
        lv_group_focus_freeze(g, false);

        if (nfc_keyboard) {
            lv_obj_del(nfc_keyboard);
            nfc_keyboard = NULL;
        }
        if (nfc_textarea) {
            lv_obj_del(nfc_textarea);
            nfc_textarea = NULL;
        }
    }
    else if (code == LV_EVENT_CANCEL) {
        // User pressed Cancel

        // Clean up keyboard
        lv_group_t *g = lv_group_get_default();
        lv_group_set_editing(g, false);
        lv_group_focus_freeze(g, false);

        if (nfc_keyboard) {
            lv_obj_del(nfc_keyboard);
            nfc_keyboard = NULL;
        }
        if (nfc_textarea) {
            lv_obj_del(nfc_textarea);
            nfc_textarea = NULL;
        }
    }
}

static void nfc_btn_read_event(lv_event_t *e)
{
    if (e->code != LV_EVENT_CLICKED) return;

    // Start reading NFC tag
    lv_label_set_text(nfc_read_status_label, "Status: Reading...");
    lv_refr_now(NULL);

    if (readNFCTag(nfc_read_tag, 15000)) {
        nfc_tag_captured = true;
        lv_label_set_text(nfc_read_status_label, "Status: Tag Read!");

        // Display tag info
        char buf[128];
        snprintf(buf, sizeof(buf), "Type: %s", nfc_read_tag.device_type.c_str());
        lv_label_set_text(nfc_read_protocol_label, buf);

        snprintf(buf, sizeof(buf), "UID: %s", nfc_read_tag.getUIDString().c_str());
        lv_label_set_text(nfc_read_data_label, buf);

        // Generate default name if not set
        if (nfc_tag_name == "") {
            nfc_tag_name = generateNFCName();
        }
        snprintf(buf, sizeof(buf), "Name: %s", nfc_tag_name.c_str());
        lv_label_set_text(nfc_read_name_label, buf);

        // Display pages count
        if (nfc_read_tag.pages_read > 0) {
            snprintf(buf, sizeof(buf), "Pages: %d / %d", nfc_read_tag.pages_read, nfc_read_tag.pages_total);
        } else {
            snprintf(buf, sizeof(buf), "Pages: ---");
        }
        lv_label_set_text(nfc_read_pages_label, buf);

        prompt_info("  Tag read successfully", 1500);
    } else {
        lv_label_set_text(nfc_read_status_label, "Status: No tag detected");
        prompt_info("  No tag detected", 2000);
    }
}

static void nfc_btn_emulate_event(lv_event_t *e)
{
    if (e->code != LV_EVENT_CLICKED) return;

    if (!nfc_tag_captured) {
        prompt_info("  Read a tag first!", 2000);
        return;
    }

    // Auto-detect emulation mode based on NDEF data
    NFCTagType tag_type = nfc_current_tag.getTagType();
    bool has_ndef_data = (tag_type == NFC_TAG_NTAG_ULTRALIGHT && nfc_current_tag.pages_read > 4);

    if (has_ndef_data) {
        // Has NDEF data - use full emulation
        lv_label_set_text(nfc_read_status_label, "Status: Emulating full...");
        lv_refr_now(NULL);

        bool success = emulateNFCTagFull(nfc_current_tag, 15000);  // 15 second timeout

        if (success) {
            lv_label_set_text(nfc_read_status_label, "Status: Emulation done");
            prompt_info("  Full emulation completed", 2000);
        } else {
            lv_label_set_text(nfc_read_status_label, "Status: No reader");
            prompt_info("  No reader detected", 2000);
        }
    } else {
        // No NDEF data - use UID only emulation
        lv_label_set_text(nfc_read_status_label, "Status: Emulating UID...");
        lv_refr_now(NULL);

        bool success = emulateNFCTagUID(nfc_current_tag, 15000);  // 15 second timeout

        if (success) {
            lv_label_set_text(nfc_read_status_label, "Status: UID emulated");
            prompt_info("  UID emulation completed", 2000);
        } else {
            lv_label_set_text(nfc_read_status_label, "Status: No reader");
            prompt_info("  No reader detected", 2000);
        }
    }

    // Re-enable input processing after emulation
    lv_group_t *g = lv_group_get_default();
    lv_group_set_editing(g, false);
    lv_refr_now(NULL);
}

static void nfc_btn_name_event(lv_event_t *e)
{
    if (e->code != LV_EVENT_CLICKED) return;

    if (!nfc_tag_captured) {
        prompt_info("  Read a tag first!", 2000);
        return;
    }


    // Create text area for input
    nfc_textarea = lv_textarea_create(lv_scr_act());
    lv_obj_set_size(nfc_textarea, LV_PCT(90), 40);
    lv_obj_align(nfc_textarea, LV_ALIGN_TOP_MID, 0, 40);
    lv_textarea_set_one_line(nfc_textarea, true);
    lv_textarea_set_text(nfc_textarea, nfc_tag_name.c_str());

    // Enable and style the cursor for visibility
    lv_obj_set_style_anim_time(nfc_textarea, 400, LV_PART_CURSOR);
    lv_obj_set_style_bg_opa(nfc_textarea, LV_OPA_COVER, LV_PART_CURSOR);
    lv_obj_set_style_bg_color(nfc_textarea, lv_color_hex(EMBED_COLOR_FOCUS_ON), LV_PART_CURSOR);
    lv_obj_set_style_border_width(nfc_textarea, 1, LV_PART_CURSOR);
    lv_obj_set_style_border_color(nfc_textarea, lv_color_hex(EMBED_COLOR_FOCUS_ON), LV_PART_CURSOR);
    lv_textarea_set_cursor_pos(nfc_textarea, LV_TEXTAREA_CURSOR_LAST);

    // Create keyboard
    nfc_keyboard = lv_keyboard_create(lv_scr_act());
    lv_keyboard_set_textarea(nfc_keyboard, nfc_textarea);
    lv_obj_add_event_cb(nfc_keyboard, nfc_name_keyboard_event, LV_EVENT_ALL, NULL);

    // Setup focus and navigation
    lv_group_t *g = lv_group_get_default();
    lv_group_add_obj(g, nfc_textarea);
    lv_group_add_obj(g, nfc_keyboard);
    lv_group_focus_obj(nfc_keyboard);
    lv_obj_add_state(nfc_keyboard, LV_STATE_FOCUS_KEY);
    lv_group_set_editing(g, true);
    lv_group_focus_freeze(g, true);
}

static void nfc_btn_save_event(lv_event_t *e)
{
    if (e->code != LV_EVENT_CLICKED) return;

    if (!nfc_tag_captured) {
        prompt_info("  Read a tag first!", 2000);
        return;
    }

    // Use the current name or generate a new one
    if (nfc_tag_name == "") {
        nfc_tag_name = generateNFCName();
    }

    char full_path[256];
    snprintf(full_path, sizeof(full_path), "/nfc/%s.nfc", nfc_tag_name.c_str());

    lv_label_set_text(nfc_read_status_label, "Status: Saving...");
    lv_refr_now(NULL);

    if (writeFlipperNFCFile(full_path, nfc_read_tag)) {
        lv_label_set_text(nfc_read_status_label, "Status: Saved!");
        prompt_info("  Tag saved successfully", 1500);

        // Reset for next read
        delay(1000);
        nfc_tag_captured = false;
        nfc_tag_name = "";
        lv_label_set_text(nfc_read_status_label, "Status: Ready");
        lv_label_set_text(nfc_read_protocol_label, "Type: ---");
        lv_label_set_text(nfc_read_data_label, "UID: ---");
        lv_label_set_text(nfc_read_name_label, "Name: ---");
        lv_label_set_text(nfc_read_pages_label, "Pages: ---");
    } else {
        lv_label_set_text(nfc_read_status_label, "Status: Save failed!");
        prompt_info("  Save failed!", 2000);
    }
}

static void scr3_1_back_btn_event_cb(lv_event_t *e)
{
    if (e->code == LV_EVENT_CLICKED) {
        exit3_1_anim(SCREEN3_ID, scr3_1_cont);
    }
}

static void create3_1(lv_obj_t *parent)
{
    scr3_1_cont = create_screen_container(parent);

    // Title
    lv_obj_t *label = lv_label_create(scr3_1_cont);
    apply_text_color(label);
    lv_obj_set_style_text_font(label, FONT_BOLD_18, LV_PART_MAIN);
    lv_label_set_text(label, "Read NFC Tag");
    lv_obj_align(label, LV_ALIGN_TOP_LEFT, 40, 10);

    // Status label
    nfc_read_status_label = lv_label_create(scr3_1_cont);
    apply_text_color(nfc_read_status_label);
    lv_obj_set_style_text_font(nfc_read_status_label, FONT_BOLD_14, LV_PART_MAIN);
    lv_label_set_text(nfc_read_status_label, "Status: Ready");
    lv_obj_align(nfc_read_status_label, LV_ALIGN_TOP_LEFT, 10, 46);

    // Protocol/Type label
    nfc_read_protocol_label = lv_label_create(scr3_1_cont);
    apply_text_color(nfc_read_protocol_label);
    lv_obj_set_style_text_font(nfc_read_protocol_label, FONT_BOLD_14, LV_PART_MAIN);
    lv_label_set_text(nfc_read_protocol_label, "Type: ---");
    lv_obj_align(nfc_read_protocol_label, LV_ALIGN_TOP_LEFT, 10, 70);

    // Data/UID label
    nfc_read_data_label = lv_label_create(scr3_1_cont);
    lv_obj_set_width(nfc_read_data_label, DISPALY_WIDTH - 20);
    apply_text_color(nfc_read_data_label);
    lv_obj_set_style_text_font(nfc_read_data_label, FONT_BOLD_14, LV_PART_MAIN);
    lv_label_set_text(nfc_read_data_label, "UID: ---");
    lv_obj_align(nfc_read_data_label, LV_ALIGN_TOP_LEFT, 10, 94);

    // Tag Name label
    nfc_read_name_label = lv_label_create(scr3_1_cont);
    lv_obj_set_width(nfc_read_name_label, DISPALY_WIDTH - 20);
    apply_text_color(nfc_read_name_label);
    lv_obj_set_style_text_font(nfc_read_name_label, FONT_BOLD_14, LV_PART_MAIN);
    lv_label_set_text(nfc_read_name_label, "Name: ---");
    lv_obj_align(nfc_read_name_label, LV_ALIGN_TOP_LEFT, 10, 118);

    // Pages label
    nfc_read_pages_label = lv_label_create(scr3_1_cont);
    lv_obj_set_width(nfc_read_pages_label, DISPALY_WIDTH - 20);
    apply_text_color(nfc_read_pages_label);
    lv_obj_set_style_text_font(nfc_read_pages_label, FONT_BOLD_14, LV_PART_MAIN);
    lv_label_set_text(nfc_read_pages_label, "Pages: ---");
    lv_obj_align(nfc_read_pages_label, LV_ALIGN_TOP_LEFT, 10, 142);

    // Read button
    nfc_btn_read = lv_btn_create(scr3_1_cont);
    lv_obj_set_size(nfc_btn_read, 100, 30);
    lv_obj_align(nfc_btn_read, LV_ALIGN_TOP_RIGHT, -10, 30);
    lv_obj_set_style_border_color(nfc_btn_read, lv_color_hex(EMBED_COLOR_BORDER), LV_PART_MAIN);
    lv_obj_set_style_border_width(nfc_btn_read, 1, LV_PART_MAIN);
    apply_no_shadow(nfc_btn_read);
    apply_bg_color(nfc_btn_read);
    lv_obj_remove_style(nfc_btn_read, NULL, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_pad(nfc_btn_read, 2, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_width(nfc_btn_read, 2, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_color(nfc_btn_read, lv_color_hex(EMBED_COLOR_FOCUS_ON), LV_STATE_FOCUS_KEY);
    lv_obj_add_event_cb(nfc_btn_read, nfc_btn_read_event, LV_EVENT_CLICKED, NULL);
    lv_obj_t *btn_label = lv_label_create(nfc_btn_read);
    apply_text_color(btn_label);
    lv_obj_set_style_text_font(btn_label, FONT_BOLD_14, LV_PART_MAIN);
    lv_label_set_text(btn_label, "Read");
    lv_obj_center(btn_label);
    lv_group_add_obj(lv_group_get_default(), nfc_btn_read);

    // Emulate button
    nfc_btn_emulate = lv_btn_create(scr3_1_cont);
    lv_obj_set_size(nfc_btn_emulate, 100, 30);
    lv_obj_align(nfc_btn_emulate, LV_ALIGN_TOP_RIGHT, -10, 65);
    lv_obj_set_style_border_color(nfc_btn_emulate, lv_color_hex(EMBED_COLOR_BORDER), LV_PART_MAIN);
    lv_obj_set_style_border_width(nfc_btn_emulate, 1, LV_PART_MAIN);
    apply_no_shadow(nfc_btn_emulate);
    apply_bg_color(nfc_btn_emulate);
    lv_obj_remove_style(nfc_btn_emulate, NULL, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_pad(nfc_btn_emulate, 2, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_width(nfc_btn_emulate, 2, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_color(nfc_btn_emulate, lv_color_hex(EMBED_COLOR_FOCUS_ON), LV_STATE_FOCUS_KEY);
    lv_obj_add_event_cb(nfc_btn_emulate, nfc_btn_emulate_event, LV_EVENT_CLICKED, NULL);
    btn_label = lv_label_create(nfc_btn_emulate);
    apply_text_color(btn_label);
    lv_obj_set_style_text_font(btn_label, FONT_BOLD_14, LV_PART_MAIN);
    lv_label_set_text(btn_label, "Emulate");
    lv_obj_center(btn_label);
    lv_group_add_obj(lv_group_get_default(), nfc_btn_emulate);

    // Name button
    nfc_btn_name = lv_btn_create(scr3_1_cont);
    lv_obj_set_size(nfc_btn_name, 100, 30);
    lv_obj_align(nfc_btn_name, LV_ALIGN_TOP_RIGHT, -10, 100);
    lv_obj_set_style_border_color(nfc_btn_name, lv_color_hex(EMBED_COLOR_BORDER), LV_PART_MAIN);
    lv_obj_set_style_border_width(nfc_btn_name, 1, LV_PART_MAIN);
    apply_no_shadow(nfc_btn_name);
    apply_bg_color(nfc_btn_name);
    lv_obj_remove_style(nfc_btn_name, NULL, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_pad(nfc_btn_name, 2, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_width(nfc_btn_name, 2, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_color(nfc_btn_name, lv_color_hex(EMBED_COLOR_FOCUS_ON), LV_STATE_FOCUS_KEY);
    lv_obj_add_event_cb(nfc_btn_name, nfc_btn_name_event, LV_EVENT_CLICKED, NULL);
    btn_label = lv_label_create(nfc_btn_name);
    apply_text_color(btn_label);
    lv_obj_set_style_text_font(btn_label, FONT_BOLD_14, LV_PART_MAIN);
    lv_label_set_text(btn_label, "Name");
    lv_obj_center(btn_label);
    lv_group_add_obj(lv_group_get_default(), nfc_btn_name);

    // Save button
    nfc_btn_save = lv_btn_create(scr3_1_cont);
    lv_obj_set_size(nfc_btn_save, 100, 30);
    lv_obj_align(nfc_btn_save, LV_ALIGN_TOP_RIGHT, -10, 135);
    lv_obj_set_style_border_color(nfc_btn_save, lv_color_hex(EMBED_COLOR_BORDER), LV_PART_MAIN);
    lv_obj_set_style_border_width(nfc_btn_save, 1, LV_PART_MAIN);
    apply_no_shadow(nfc_btn_save);
    apply_bg_color(nfc_btn_save);
    lv_obj_remove_style(nfc_btn_save, NULL, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_pad(nfc_btn_save, 2, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_width(nfc_btn_save, 2, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_color(nfc_btn_save, lv_color_hex(EMBED_COLOR_FOCUS_ON), LV_STATE_FOCUS_KEY);
    lv_obj_add_event_cb(nfc_btn_save, nfc_btn_save_event, LV_EVENT_CLICKED, NULL);
    btn_label = lv_label_create(nfc_btn_save);
    apply_text_color(btn_label);
    lv_obj_set_style_text_font(btn_label, FONT_BOLD_14, LV_PART_MAIN);
    lv_label_set_text(btn_label, "Save");
    lv_obj_center(btn_label);
    lv_group_add_obj(lv_group_get_default(), nfc_btn_save);

    // Back button
    nfc_btn_back = scr_back_btn_create(scr3_1_cont, scr3_1_back_btn_event_cb);
}

static void entry3_1(void)
{
    entry3_1_anim(scr3_1_cont);
    lv_group_set_wrap(lv_group_get_default(), true);

    // Reset state
    nfc_tag_captured = false;
    nfc_tag_name = "";
    lv_label_set_text(nfc_read_status_label, "Status: Press Read");
    lv_label_set_text(nfc_read_protocol_label, "Type: ---");
    lv_label_set_text(nfc_read_data_label, "UID: ---");
    lv_label_set_text(nfc_read_name_label, "Name: ---");
    lv_label_set_text(nfc_read_pages_label, "Pages: ---");
}

static void exit3_1(void)
{
    lv_group_set_wrap(lv_group_get_default(), false);
}

static void destroy3_1(void)
{
    if (scr3_1_cont) {
        lv_obj_del(scr3_1_cont);
        scr3_1_cont = NULL;
    }
}

scr_lifecycle_t screen3_1 = {
    .create = create3_1,
    .entry = entry3_1,
    .exit = exit3_1,
    .destroy = destroy3_1,
};
#endif

//************************************[ screen 3.2 ]****************************************** NFC Tag Detail
#if 1
lv_obj_t *scr3_2_cont;
lv_obj_t *nfc_detail_title_label;
lv_obj_t *nfc_detail_status_label;
lv_obj_t *nfc_detail_type_label;
lv_obj_t *nfc_detail_uid_label;
lv_obj_t *nfc_detail_pages_label;
lv_obj_t *nfc_btn_emulate_detail;
lv_obj_t *nfc_btn_detail;
lv_obj_t *nfc_btn_rename;
lv_obj_t *nfc_btn_delete;
lv_obj_t *nfc_btn_detail_back;
lv_obj_t *nfc_detail_keyboard = NULL;
lv_obj_t *nfc_detail_textarea = NULL;

void entry3_2_anim(lv_obj_t *obj) { entry1_anim(obj); }
void exit3_2_anim(int user_data, lv_obj_t *obj) { exit1_anim(user_data, obj); }

// Forward declaration
static void nfc_detail_rename_keyboard_event(lv_event_t *e);

// Keyboard event handler for NFC tag rename in detail screen
static void nfc_detail_rename_keyboard_event(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_READY) {
        // User pressed OK
        const char *new_name = lv_textarea_get_text(nfc_detail_textarea);

        if (strlen(new_name) > 0) {
            // Extract directory path
            String filepath = nfc_current_tag.filepath;
            int last_slash = filepath.lastIndexOf('/');
            String dir_path = filepath.substring(0, last_slash + 1);

            // Create new filepath with new name
            String new_filepath = dir_path + new_name + ".nfc";


            // Rename the file
            if (SD.rename(nfc_current_tag.filepath.c_str(), new_filepath.c_str())) {
                nfc_current_tag.filepath = new_filepath;

                // Update the title with new filename
                char buf[128];
                snprintf(buf, sizeof(buf), "%s.nfc", new_name);
                lv_label_set_text(nfc_detail_title_label, buf);

                prompt_info("  Renamed successfully", 1500);
            } else {
                prompt_info("  Rename failed!", 2000);
            }
        }

        // Clean up keyboard
        lv_group_t *g = lv_group_get_default();
        lv_group_set_editing(g, false);
        lv_group_focus_freeze(g, false);

        if (nfc_detail_keyboard) {
            lv_obj_del(nfc_detail_keyboard);
            nfc_detail_keyboard = NULL;
        }
        if (nfc_detail_textarea) {
            lv_obj_del(nfc_detail_textarea);
            nfc_detail_textarea = NULL;
        }
    }
    else if (code == LV_EVENT_CANCEL) {
        // User pressed Cancel

        // Clean up keyboard
        lv_group_t *g = lv_group_get_default();
        lv_group_set_editing(g, false);
        lv_group_focus_freeze(g, false);

        if (nfc_detail_keyboard) {
            lv_obj_del(nfc_detail_keyboard);
            nfc_detail_keyboard = NULL;
        }
        if (nfc_detail_textarea) {
            lv_obj_del(nfc_detail_textarea);
            nfc_detail_textarea = NULL;
        }
    }
}

static void nfc_btn_emulate_detail_event(lv_event_t *e)
{
    if (e->code != LV_EVENT_CLICKED) return;

    // Auto-detect emulation mode based on NDEF data
    NFCTagType tag_type = nfc_current_tag.getTagType();
    bool has_ndef_data = (tag_type == NFC_TAG_NTAG_ULTRALIGHT && nfc_current_tag.pages_read > 4);

    if (has_ndef_data) {
        // Has NDEF data - use full emulation
        lv_label_set_text(nfc_detail_status_label, "Status: Emulating full...");
        lv_refr_now(NULL);

        bool success = emulateNFCTagFull(nfc_current_tag, 15000);  // 15 second timeout

        if (success) {
            lv_label_set_text(nfc_detail_status_label, "Status: Emulation done");
            prompt_info("  Full emulation completed", 2000);
        } else {
            lv_label_set_text(nfc_detail_status_label, "Status: No reader");
            prompt_info("  No reader detected", 2000);
        }
    } else {
        // No NDEF data - use UID only emulation
        lv_label_set_text(nfc_detail_status_label, "Status: Emulating UID...");
        lv_refr_now(NULL);

        bool success = emulateNFCTagUID(nfc_current_tag, 15000);  // 15 second timeout

        if (success) {
            lv_label_set_text(nfc_detail_status_label, "Status: UID emulated");
            prompt_info("  UID emulation completed", 2000);
        } else {
            lv_label_set_text(nfc_detail_status_label, "Status: No reader");
            prompt_info("  No reader detected", 2000);
        }
    }

    // Re-enable input processing after emulation
    lv_group_t *g = lv_group_get_default();
    lv_group_set_editing(g, false);
    lv_refr_now(NULL);
}

static void nfc_btn_detail_event(lv_event_t *e)
{
    if (e->code != LV_EVENT_CLICKED) return;

    // Navigate to NDEF record detail screen
    scr_mgr_push(SCREEN3_2_1_ID, true);
}

static void nfc_btn_rename_detail_event(lv_event_t *e)
{
    if (e->code != LV_EVENT_CLICKED) return;


    // Extract current filename without extension
    String filepath = nfc_current_tag.filepath;
    int last_slash = filepath.lastIndexOf('/');
    int last_dot = filepath.lastIndexOf('.');
    String current_name = filepath.substring(last_slash + 1, last_dot);

    // Create text area for input
    nfc_detail_textarea = lv_textarea_create(lv_scr_act());
    lv_obj_set_size(nfc_detail_textarea, LV_PCT(90), 40);
    lv_obj_align(nfc_detail_textarea, LV_ALIGN_TOP_MID, 0, 40);
    lv_textarea_set_one_line(nfc_detail_textarea, true);
    lv_textarea_set_text(nfc_detail_textarea, current_name.c_str());

    // Enable and style the cursor for visibility
    lv_obj_set_style_anim_time(nfc_detail_textarea, 400, LV_PART_CURSOR);
    lv_obj_set_style_bg_opa(nfc_detail_textarea, LV_OPA_COVER, LV_PART_CURSOR);
    lv_obj_set_style_bg_color(nfc_detail_textarea, lv_color_hex(EMBED_COLOR_FOCUS_ON), LV_PART_CURSOR);
    lv_obj_set_style_border_width(nfc_detail_textarea, 1, LV_PART_CURSOR);
    lv_obj_set_style_border_color(nfc_detail_textarea, lv_color_hex(EMBED_COLOR_FOCUS_ON), LV_PART_CURSOR);
    lv_textarea_set_cursor_pos(nfc_detail_textarea, LV_TEXTAREA_CURSOR_LAST);

    // Create keyboard
    nfc_detail_keyboard = lv_keyboard_create(lv_scr_act());
    lv_keyboard_set_textarea(nfc_detail_keyboard, nfc_detail_textarea);
    lv_obj_add_event_cb(nfc_detail_keyboard, nfc_detail_rename_keyboard_event, LV_EVENT_ALL, NULL);

    // Setup focus and navigation
    lv_group_t *g = lv_group_get_default();
    lv_group_add_obj(g, nfc_detail_textarea);
    lv_group_add_obj(g, nfc_detail_keyboard);
    lv_group_focus_obj(nfc_detail_keyboard);
    lv_obj_add_state(nfc_detail_keyboard, LV_STATE_FOCUS_KEY);
    lv_group_set_editing(g, true);
    lv_group_focus_freeze(g, true);
}

static void nfc_btn_delete_event(lv_event_t *e)
{
    if (e->code != LV_EVENT_CLICKED) return;

    // Show confirmation dialog
    static const char *btns[] = {"Delete", "Cancel", ""};
    lv_obj_t *mbox = lv_msgbox_create(NULL, "Delete Tag", "Delete this tag file?",
                                      btns, true);

    lv_obj_add_event_cb(mbox, [](lv_event_t *e) {
        if (e->code == LV_EVENT_VALUE_CHANGED) {
            lv_obj_t *mbox = lv_event_get_current_target(e);
            uint16_t btn_id = lv_msgbox_get_active_btn(mbox);

            if (btn_id == 0) {  // Delete
                // Delete the file
                if (SD.remove(nfc_current_tag.filepath.c_str())) {
                    prompt_info("  Deleted successfully", 1500);
                    delay(500);
                    scr_mgr_switch(SCREEN3_ID, false);
                } else {
                    prompt_info("  Delete failed!", 2000);
                }
            }

            lv_msgbox_close(mbox);
        }
    }, LV_EVENT_VALUE_CHANGED, NULL);

    lv_group_t *g = lv_group_get_default();
    lv_obj_t *btnm = lv_msgbox_get_btns(mbox);
    if (btnm) {
        lv_group_add_obj(g, btnm);
        lv_group_focus_obj(btnm);
        lv_group_set_editing(g, true);
    }
}

static void scr3_2_back_btn_event_cb(lv_event_t *e)
{
    if (e->code == LV_EVENT_CLICKED) {
        exit3_2_anim(SCREEN3_ID, scr3_2_cont);
    }
}

static void create3_2(lv_obj_t *parent)
{
    scr3_2_cont = create_screen_container(parent);

    // Title (will be set to filename in entry)
    nfc_detail_title_label = lv_label_create(scr3_2_cont);
    lv_obj_set_width(nfc_detail_title_label, DISPALY_WIDTH - 80);
    apply_text_color(nfc_detail_title_label);
    lv_obj_set_style_text_font(nfc_detail_title_label, FONT_BOLD_18, LV_PART_MAIN);
    lv_label_set_text(nfc_detail_title_label, "Tag Info");
    lv_label_set_long_mode(nfc_detail_title_label, LV_LABEL_LONG_DOT);
    lv_obj_align(nfc_detail_title_label, LV_ALIGN_TOP_LEFT, 40, 8);

    // Status label
    nfc_detail_status_label = lv_label_create(scr3_2_cont);
    apply_text_color(nfc_detail_status_label);
    lv_obj_set_style_text_font(nfc_detail_status_label, FONT_BOLD_14, LV_PART_MAIN);
    lv_label_set_text(nfc_detail_status_label, "Status: Ready");
    lv_obj_align(nfc_detail_status_label, LV_ALIGN_TOP_LEFT, 10, 46);

    // Type label
    nfc_detail_type_label = lv_label_create(scr3_2_cont);
    apply_text_color(nfc_detail_type_label);
    lv_obj_set_style_text_font(nfc_detail_type_label, FONT_BOLD_14, LV_PART_MAIN);
    lv_label_set_text(nfc_detail_type_label, "Type: ---");
    lv_obj_align(nfc_detail_type_label, LV_ALIGN_TOP_LEFT, 10, 70);

    // UID label
    nfc_detail_uid_label = lv_label_create(scr3_2_cont);
    lv_obj_set_width(nfc_detail_uid_label, DISPALY_WIDTH - 20);
    apply_text_color(nfc_detail_uid_label);
    lv_obj_set_style_text_font(nfc_detail_uid_label, FONT_BOLD_14, LV_PART_MAIN);
    lv_label_set_text(nfc_detail_uid_label, "UID: ---");
    lv_obj_align(nfc_detail_uid_label, LV_ALIGN_TOP_LEFT, 10, 94);

    // Pages label
    nfc_detail_pages_label = lv_label_create(scr3_2_cont);
    lv_obj_set_width(nfc_detail_pages_label, DISPALY_WIDTH - 20);
    apply_text_color(nfc_detail_pages_label);
    lv_obj_set_style_text_font(nfc_detail_pages_label, FONT_BOLD_14, LV_PART_MAIN);
    lv_label_set_text(nfc_detail_pages_label, "Pages: ---");
    lv_obj_align(nfc_detail_pages_label, LV_ALIGN_TOP_LEFT, 10, 118);

    // Emulate button
    nfc_btn_emulate_detail = lv_btn_create(scr3_2_cont);
    lv_obj_set_size(nfc_btn_emulate_detail, 100, 30);
    lv_obj_align(nfc_btn_emulate_detail, LV_ALIGN_TOP_RIGHT, -10, 30);
    lv_obj_set_style_border_color(nfc_btn_emulate_detail, lv_color_hex(EMBED_COLOR_BORDER), LV_PART_MAIN);
    lv_obj_set_style_border_width(nfc_btn_emulate_detail, 1, LV_PART_MAIN);
    apply_no_shadow(nfc_btn_emulate_detail);
    apply_bg_color(nfc_btn_emulate_detail);
    lv_obj_remove_style(nfc_btn_emulate_detail, NULL, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_pad(nfc_btn_emulate_detail, 2, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_width(nfc_btn_emulate_detail, 2, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_color(nfc_btn_emulate_detail, lv_color_hex(EMBED_COLOR_FOCUS_ON), LV_STATE_FOCUS_KEY);
    lv_obj_add_event_cb(nfc_btn_emulate_detail, nfc_btn_emulate_detail_event, LV_EVENT_CLICKED, NULL);
    lv_obj_t *btn_label = lv_label_create(nfc_btn_emulate_detail);
    apply_text_color(btn_label);
    lv_obj_set_style_text_font(btn_label, FONT_BOLD_14, LV_PART_MAIN);
    lv_label_set_text(btn_label, "Emulate");
    lv_obj_center(btn_label);
    lv_group_add_obj(lv_group_get_default(), nfc_btn_emulate_detail);

    // Detail button
    nfc_btn_detail = lv_btn_create(scr3_2_cont);
    lv_obj_set_size(nfc_btn_detail, 100, 30);
    lv_obj_align(nfc_btn_detail, LV_ALIGN_TOP_RIGHT, -10, 65);
    lv_obj_set_style_border_color(nfc_btn_detail, lv_color_hex(EMBED_COLOR_BORDER), LV_PART_MAIN);
    lv_obj_set_style_border_width(nfc_btn_detail, 1, LV_PART_MAIN);
    apply_no_shadow(nfc_btn_detail);
    apply_bg_color(nfc_btn_detail);
    lv_obj_remove_style(nfc_btn_detail, NULL, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_pad(nfc_btn_detail, 2, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_width(nfc_btn_detail, 2, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_color(nfc_btn_detail, lv_color_hex(EMBED_COLOR_FOCUS_ON), LV_STATE_FOCUS_KEY);
    lv_obj_add_event_cb(nfc_btn_detail, nfc_btn_detail_event, LV_EVENT_CLICKED, NULL);
    btn_label = lv_label_create(nfc_btn_detail);
    apply_text_color(btn_label);
    lv_obj_set_style_text_font(btn_label, FONT_BOLD_14, LV_PART_MAIN);
    lv_label_set_text(btn_label, "Detail");
    lv_obj_center(btn_label);
    lv_group_add_obj(lv_group_get_default(), nfc_btn_detail);

    // Rename button
    nfc_btn_rename = lv_btn_create(scr3_2_cont);
    lv_obj_set_size(nfc_btn_rename, 100, 30);
    lv_obj_align(nfc_btn_rename, LV_ALIGN_TOP_RIGHT, -10, 100);
    lv_obj_set_style_border_color(nfc_btn_rename, lv_color_hex(EMBED_COLOR_BORDER), LV_PART_MAIN);
    lv_obj_set_style_border_width(nfc_btn_rename, 1, LV_PART_MAIN);
    apply_no_shadow(nfc_btn_rename);
    apply_bg_color(nfc_btn_rename);
    lv_obj_remove_style(nfc_btn_rename, NULL, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_pad(nfc_btn_rename, 2, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_width(nfc_btn_rename, 2, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_color(nfc_btn_rename, lv_color_hex(EMBED_COLOR_FOCUS_ON), LV_STATE_FOCUS_KEY);
    lv_obj_add_event_cb(nfc_btn_rename, nfc_btn_rename_detail_event, LV_EVENT_CLICKED, NULL);
    btn_label = lv_label_create(nfc_btn_rename);
    apply_text_color(btn_label);
    lv_obj_set_style_text_font(btn_label, FONT_BOLD_14, LV_PART_MAIN);
    lv_label_set_text(btn_label, "Rename");
    lv_obj_center(btn_label);
    lv_group_add_obj(lv_group_get_default(), nfc_btn_rename);

    // Delete button
    nfc_btn_delete = lv_btn_create(scr3_2_cont);
    lv_obj_set_size(nfc_btn_delete, 100, 30);
    lv_obj_align(nfc_btn_delete, LV_ALIGN_TOP_RIGHT, -10, 135);
    lv_obj_set_style_border_color(nfc_btn_delete, lv_color_hex(EMBED_COLOR_BORDER), LV_PART_MAIN);
    lv_obj_set_style_border_width(nfc_btn_delete, 1, LV_PART_MAIN);
    apply_no_shadow(nfc_btn_delete);
    apply_bg_color(nfc_btn_delete);
    lv_obj_remove_style(nfc_btn_delete, NULL, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_pad(nfc_btn_delete, 2, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_width(nfc_btn_delete, 2, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_color(nfc_btn_delete, lv_color_hex(EMBED_COLOR_FOCUS_ON), LV_STATE_FOCUS_KEY);
    lv_obj_add_event_cb(nfc_btn_delete, nfc_btn_delete_event, LV_EVENT_CLICKED, NULL);
    btn_label = lv_label_create(nfc_btn_delete);
    apply_text_color(btn_label);
    lv_obj_set_style_text_font(btn_label, FONT_BOLD_14, LV_PART_MAIN);
    lv_label_set_text(btn_label, "Delete");
    lv_obj_center(btn_label);
    lv_group_add_obj(lv_group_get_default(), nfc_btn_delete);

    // Back button
    nfc_btn_detail_back = scr_back_btn_create(scr3_2_cont, scr3_2_back_btn_event_cb);
}

static void entry3_2(void)
{
    entry3_2_anim(scr3_2_cont);
    lv_group_set_wrap(lv_group_get_default(), true);

    // Display tag information
    char buf[128];

    // Extract filename from filepath
    const char *filename = strrchr(nfc_current_tag.filepath.c_str(), '/');
    if (filename) filename++;
    else filename = nfc_current_tag.filepath.c_str();

    // Update title with filename
    lv_label_set_text(nfc_detail_title_label, filename);

    // Update labels
    lv_label_set_text(nfc_detail_status_label, "Status: Ready");

    snprintf(buf, sizeof(buf), "Type: %s", nfc_current_tag.device_type.c_str());
    lv_label_set_text(nfc_detail_type_label, buf);

    snprintf(buf, sizeof(buf), "UID: %s", nfc_current_tag.getUIDString().c_str());
    lv_label_set_text(nfc_detail_uid_label, buf);

    // Display pages count
    if (nfc_current_tag.pages_read > 0) {
        snprintf(buf, sizeof(buf), "Pages: %d / %d", nfc_current_tag.pages_read, nfc_current_tag.pages_total);
    } else {
        snprintf(buf, sizeof(buf), "Pages: ---");
    }
    lv_label_set_text(nfc_detail_pages_label, buf);
}

static void exit3_2(void)
{
    lv_group_set_wrap(lv_group_get_default(), false);
}

static void destroy3_2(void)
{
    if (scr3_2_cont) {
        lv_obj_del(scr3_2_cont);
        scr3_2_cont = NULL;
    }
}

scr_lifecycle_t screen3_2 = {
    .create = create3_2,
    .entry = entry3_2,
    .exit = exit3_2,
    .destroy = destroy3_2,
};
#endif

//************************************[ screen 3.2.1 ]****************************************** Details (NDEF Records)
#if 1
lv_obj_t *scr3_2_1_cont;
lv_obj_t *nfc_ndef_list;
std::vector<NDEFRecord> nfc_ndef_records;

void entry3_2_1_anim(lv_obj_t *obj) { entry1_anim(obj); }
void exit3_2_1_anim(int user_data, lv_obj_t *obj) { exit1_anim(user_data, obj); }

static void scr3_2_1_back_btn_event_cb(lv_event_t *e)
{
    if (e->code == LV_EVENT_CLICKED) {
        exit3_2_1_anim(SCREEN3_2_ID, scr3_2_1_cont);
    }
}

static void create3_2_1(lv_obj_t *parent)
{
    scr3_2_1_cont = create_screen_container(parent);

    // Title
    lv_obj_t *label = lv_label_create(scr3_2_1_cont);
    apply_text_color(label);
    lv_obj_set_style_text_font(label, FONT_BOLD_18, LV_PART_MAIN);
    lv_label_set_text(label, "NFC Details");
    lv_obj_align(label, LV_ALIGN_TOP_LEFT, 40, 10);

    // Create scrollable list
    nfc_ndef_list = lv_list_create(scr3_2_1_cont);
    lv_obj_set_size(nfc_ndef_list, DISPALY_WIDTH - 20, DISPALY_HEIGHT - 50);
    lv_obj_align(nfc_ndef_list, LV_ALIGN_TOP_LEFT, 10, 40);
    apply_bg_color(nfc_ndef_list);
    lv_obj_set_style_border_color(nfc_ndef_list, lv_color_hex(EMBED_COLOR_BORDER), LV_PART_MAIN);
    apply_no_border(nfc_ndef_list);
    apply_no_padding(nfc_ndef_list);

    // Back button
    scr_back_btn_create(scr3_2_1_cont, scr3_2_1_back_btn_event_cb);
}

static void entry3_2_1(void)
{
    entry3_2_1_anim(scr3_2_1_cont);

    // Clear the focus group to remove previous screen's objects
    lv_group_t *g = lv_group_get_default();
    lv_group_remove_all_objs(g);

    lv_group_set_wrap(g, true);

    // Clear existing list items
    lv_obj_clean(nfc_ndef_list);

    // Re-add back button to focus group first
    lv_obj_t *back_btn = lv_obj_get_child(scr3_2_1_cont, -1);  // Back button is last child
    if (back_btn) {
        lv_group_add_obj(g, back_btn);
    }

    // Parse NDEF records from current tag
    parseNDEFRecords(nfc_current_tag, nfc_ndef_records);

    if (nfc_ndef_records.empty()) {
        // No NDEF records found - add simple text item
        lv_obj_t *item = lv_list_add_btn(nfc_ndef_list, NULL, "No NDEF records found");
        apply_bg_color(item);
        apply_text_color(item);
        lv_obj_set_style_text_font(item, FONT_BOLD_14, LV_PART_MAIN);
        lv_group_add_obj(g, item);
    } else {
        // Display each NDEF record as a list item
        for (size_t i = 0; i < nfc_ndef_records.size(); i++) {
            const NDEFRecord& rec = nfc_ndef_records[i];

            // Build multi-line text for the item
            String item_text = rec.label + "\n" + rec.value;
            if (rec.details.length() > 0) {
                item_text += "\n" + rec.details;
            }

            // Create list item
            lv_obj_t *item = lv_list_add_btn(nfc_ndef_list, NULL, item_text.c_str());
            apply_bg_color(item);
            apply_text_color(item);
            lv_obj_set_style_text_font(item, FONT_BOLD_14, LV_PART_MAIN);
            lv_obj_set_style_bg_color(item, lv_color_hex(EMBED_COLOR_FOCUS_ON), LV_STATE_FOCUS_KEY);
            lv_obj_set_height(item, LV_SIZE_CONTENT);

            // Make text wrap
            lv_obj_t *label = lv_obj_get_child(item, 0);
            if (label) {
                lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
                lv_obj_set_width(label, DISPALY_WIDTH - 60);
            }

            // Add to focus group
            lv_group_add_obj(g, item);
        }
    }
}

static void exit3_2_1(void)
{
    lv_group_set_wrap(lv_group_get_default(), false);
}

static void destroy3_2_1(void)
{
    if (scr3_2_1_cont) {
        lv_obj_del(scr3_2_1_cont);
        scr3_2_1_cont = NULL;
    }
    nfc_ndef_records.clear();
}

scr_lifecycle_t screen3_2_1 = {
    .create = create3_2_1,
    .entry = entry3_2_1,
    .exit = exit3_2_1,
    .destroy = destroy3_2_1,
};
#endif

//************************************[ screen 4 ]****************************************** setting
/*** UI interface ***/
bool __attribute__((weak)) ui_scr4_get_sghz_st(void) { return false; }
bool __attribute__((weak)) ui_scr4_get_pmu_st(void) {  return false; }
bool __attribute__((weak)) ui_scr4_get_nfc_st(void) {  return false; }
bool __attribute__((weak)) ui_scr4_get_sd_st(void) {   return false; }
bool __attribute__((weak)) ui_scr4_get_nrf24_st(void) {return false; }
// end
// --------------------- screen 4.1 --------------------- About System
#if 1
static lv_obj_t *scr4_1_cont;

static void entry4_1_anim(lv_obj_t *obj) { entry1_anim(obj); }
static void exit4_1_anim(int user_data, lv_obj_t *obj) { exit1_anim(user_data, obj); }

static void scr4_1_btn_event_cb(lv_event_t * e)
{
    if(e->code == LV_EVENT_CLICKED){
        exit4_1_anim(SCREEN4_ID, scr4_1_cont);
    }
}

static void create4_1(lv_obj_t *parent) 
{   
    scr4_1_cont = create_screen_container(parent);

    lv_obj_t *label = lv_label_create(scr4_1_cont);
    apply_text_color(label);
    lv_obj_set_style_text_font(label, &Font_Mono_Bold_14, LV_PART_MAIN);
    lv_label_set_text(label, "About System");
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 10);

    lv_obj_t *info = lv_label_create(scr4_1_cont);
    lv_obj_set_width(info, DISPALY_WIDTH * 0.9);
    apply_text_color(info);
    lv_obj_set_style_text_font(info, &Font_Mono_Bold_14, LV_PART_MAIN);
    lv_label_set_long_mode(info, LV_LABEL_LONG_WRAP);

    String str = "";

    str += line_full_format(36, "SF Version:", T_EMBED_CC1101_SF_VER);
    str += "\n";

    str += line_full_format(36, "HD Version:", T_EMBED_CC1101_HD_VER);
    str += "\n";

    str += line_full_format(36, "CC1101 Init:", (ui_scr4_get_sghz_st() ? "PASS" : "FAIL"));
    str += "\n";

    str += line_full_format(36, "NFC Init:", (ui_scr4_get_nfc_st() ? "PASS" : "FAIL"));
    str += "\n";

    str += line_full_format(36, "SD Card Init:", (ui_scr4_get_sd_st() ? "PASS" : "FAIL"));
    str += "\n";

    char buf[30];
    uint64_t total=0, used=0;
    ui_setting_get_sd_capacity(&total, &used);
    lv_snprintf(buf, 30, "%llu/%llu MB", used, total);
    str += line_full_format(36, "SD Card Cap:", (const char *)buf);
    str += "\n";

    lv_label_set_text(info, str.c_str());

    lv_obj_align_to(info, label, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);

    // back bottom
    scr_back_btn_create(scr4_1_cont, scr4_1_btn_event_cb);
}
static void entry4_1(void) {
    entry4_1_anim(scr4_1_cont);
}
static void exit4_1(void) {}
static void destroy4_1(void) {}

static scr_lifecycle_t screen4_1 = {
    .create = create4_1,
    .entry = entry4_1,
    .exit  = exit4_1,
    .destroy = destroy4_1,
};
#endif
// --------------------- screen 4 ----------------------- 
#if 1

lv_obj_t *scr4_cont;
lv_obj_t *setting_list;
lv_obj_t *theme_label;
int rotation_setting = 0;
static lv_obj_t *shutdown_up;
static lv_obj_t *shutdown_dp;

static void transform_angle_cb(void * var, int32_t v) {
    lv_obj_set_style_transform_angle((lv_obj_t *)var, v , LV_PART_MAIN);
}

static void transform_angle_anim(lv_obj_t *obj, int angle)
{
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, obj);
    lv_anim_set_values(&a, angle*1800, (angle+1) * 1800);
    lv_anim_set_time(&a, 500);
    lv_anim_set_path_cb(&a, lv_anim_path_linear);
    lv_anim_set_exec_cb(&a, transform_angle_cb);
    lv_anim_start(&a);
}

static void shotdown_anim_cb(void * var, int32_t v){
    lv_obj_set_y((lv_obj_t *)shutdown_up, v-LV_VER_RES/2);
    lv_obj_set_y((lv_obj_t *)shutdown_dp, LV_VER_RES-v);
}

static void shutdown_anim_ready_cb(lv_anim_t *a)
{
    PPM.shutdown();
}

void setting_scr_event(lv_event_t *e)
{
    lv_obj_t *tgt = (lv_obj_t *)e->target;
    int data = (int)e->user_data;

    if(e->code == LV_EVENT_CLICKED) {
        switch (data)
        {
        case 0: // "Rotation"
            lv_obj_set_style_transform_pivot_x(scr4_cont, LV_HOR_RES/2, LV_PART_MAIN);
            lv_obj_set_style_transform_pivot_y(scr4_cont, LV_VER_RES/2, LV_PART_MAIN);

            if(display_rotation == 3){
                transform_angle_anim(scr4_cont, rotation_setting);
                display_rotation = 1;
            } else if(display_rotation == 1){
                display_rotation = 3;
                transform_angle_anim(scr4_cont, rotation_setting);
            }
            rotation_setting = !rotation_setting;
            eeprom_wr(UI_ROTATION_EEPROM_ADDR, display_rotation);

            // Save to config
            g_config.display.rotation = display_rotation;
            config_save();

            // lv_obj_invalidate(scr4_cont);
            break;
        case 1: { // "Deep Sleep"
            // Save configuration before sleeping
            config_save();

            // Clear LVGL screen to prevent artifacts on wake
            lv_obj_t *scr = lv_scr_act();
            lv_obj_clean(scr);
            lv_obj_set_style_bg_color(scr, lv_color_black(), LV_PART_MAIN);
            lv_refr_now(NULL);

            ui_scr1_set_light(0);
            digitalWrite(BOARD_PN532_RF_REST, LOW);     // PN532 sleep
            tft.writecommand(ST7789_SLPIN);             // ST7798 sleep

            radio.sleep();                              // CC1101 sleep

            digitalWrite(BOARD_PWR_EN, LOW);            // Power off CC1101 and LED

            // esp_sleep_enable_ext0_wakeup((gpio_num_t)ENCODER_KEY, 0);
            esp_sleep_enable_ext1_wakeup((1UL << BOARD_USER_KEY), ESP_EXT1_WAKEUP_ANY_LOW);   // Hibernate using user keys
            esp_deep_sleep_start();
            break;
        }
        case 2: // "UI Theme"
            if(setting_theme == UI_THEME_DARK) {
                setting_theme = UI_THEME_LIGHT;
                lv_label_set_text(theme_label, "LIGHT");
            } else if(setting_theme == UI_THEME_LIGHT) {
                setting_theme = UI_THEME_DARK;
                lv_label_set_text(theme_label, "DARK");
            }
            ui_theme_setting(setting_theme);
            eeprom_wr(UI_THEME_EEPROM_ADDR, setting_theme);

            // Save to config
            g_config.display.theme = setting_theme;
            config_save();
            break;
        case 3: // "Shutdown"
                // Save configuration before shutdown
                config_save();

                lv_obj_clear_flag(shutdown_up, LV_OBJ_FLAG_HIDDEN);
                lv_obj_clear_flag(shutdown_dp, LV_OBJ_FLAG_HIDDEN);

                lv_anim_t a;
                lv_anim_init(&a);
                lv_anim_set_var(&a, shutdown_up);
                lv_anim_set_values(&a, 0, LV_VER_RES/2);
                lv_anim_set_time(&a, 1000);
                lv_anim_set_path_cb(&a, lv_anim_path_linear);
                lv_anim_set_exec_cb(&a, shotdown_anim_cb);
                lv_anim_set_ready_cb(&a, shutdown_anim_ready_cb);
                lv_anim_start(&a);
            break;
        case 4: {// "Create Config Example"
            if (config_sd_available()) {
                if (config_create_example()) {
                    prompt_info("  Example config created!\n  nautilus.json", 2000);
                } else {
                    prompt_info("  Failed to create\n  example config", 2000);
                }
            } else {
                prompt_info("  SD card not available", 1500);
            }
            } break;
        case 5: {// "About System"
            //                       (ui_scr4_get_sd_st() ? "PASS" : "FAIL"));
            // prompt_info(buf, 1000);
            scr_mgr_switch(SCREEN4_1_ID, false);
            } break;
		case 6: {// Config Reload
				 config_load();
			} break;
        default:
            break;
        }
    }
}

void entry4_anim(lv_obj_t *obj)
{
    entry1_anim(obj);
}

void exit4_anim(int user_data, lv_obj_t *obj)
{
    exit1_anim(user_data, obj);
}

static void scr4_btn_event_cb(lv_event_t * e)
{
    if(e->code == LV_EVENT_CLICKED){
        // scr_mgr_set_anim(LV_SCR_LOAD_ANIM_FADE_OUT, -1, -1);
        // scr_mgr_switch(SCREEN0_ID, false);
        exit4_anim(SCREEN0_ID, scr4_cont);
    }
}

void create4(lv_obj_t *parent){
    scr4_cont = create_screen_container(parent);

    lv_obj_t *label = lv_label_create(scr4_cont);
    apply_text_color(label);
    lv_obj_set_style_text_font(label, FONT_BOLD_16, LV_PART_MAIN);
    lv_label_set_text(label, "Setting");
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 10);

    shutdown_up = lv_obj_create(parent);
    lv_obj_set_size(shutdown_up, lv_pct(100), lv_pct(50));
    lv_obj_set_style_bg_color(shutdown_up, lv_color_hex(0x000000), LV_PART_MAIN);
    apply_no_scrollbar(shutdown_up);
    apply_no_border(shutdown_up);
    lv_obj_set_style_radius(shutdown_up, 0, 0);
    apply_no_padding(shutdown_up);
    lv_obj_add_flag(shutdown_up, LV_OBJ_FLAG_HIDDEN);

    shutdown_dp = lv_obj_create(parent);
    lv_obj_set_size(shutdown_dp, lv_pct(100), lv_pct(50));
    lv_obj_set_style_bg_color(shutdown_dp, lv_color_hex(0x000000), LV_PART_MAIN);
    apply_no_scrollbar(shutdown_dp);
    apply_no_border(shutdown_dp);
    lv_obj_set_style_radius(shutdown_dp, 0, 0);
    apply_no_padding(shutdown_dp);
    lv_obj_add_flag(shutdown_dp, LV_OBJ_FLAG_HIDDEN);

    setting_list = lv_list_create(scr4_cont);
    lv_obj_set_size(setting_list, LV_HOR_RES, 135);
    lv_obj_align(setting_list, LV_ALIGN_BOTTOM_MID, 0, 0);
    apply_bg_color(setting_list);
    lv_obj_set_style_pad_top(setting_list, 2, LV_PART_MAIN);
    lv_obj_set_style_pad_row(setting_list, 2, LV_PART_MAIN);
    apply_no_radius(setting_list);
    // lv_obj_set_style_outline_pad(setting_list, 2, LV_PART_MAIN);
    apply_no_border(setting_list);
    apply_no_shadow(setting_list);

    lv_obj_t *setting1 = lv_list_add_btn(setting_list, NULL, "- Screen Rotation");
    lv_obj_t *setting2 = lv_list_add_btn(setting_list, NULL, "- Deep Sleep");
    lv_obj_t *setting3 = lv_list_add_btn(setting_list, NULL, "- UI Theme");
    lv_obj_t *setting4 = lv_list_add_btn(setting_list, NULL, "- Shutdown");
    lv_obj_t *setting5 = lv_list_add_btn(setting_list, NULL, "- Create Config Example");
    lv_obj_t *setting6 = lv_list_add_btn(setting_list, NULL, "- About System");
    lv_obj_t *setting7 = lv_list_add_btn(setting_list, NULL, "- Config Reload");

    for(int i = 0; i < lv_obj_get_child_cnt(setting_list); i++) {
        lv_obj_t *item = lv_obj_get_child(setting_list, i);
        lv_obj_set_style_text_font(item, FONT_BOLD_14, LV_PART_MAIN);
        apply_bg_color(item);
        apply_text_color(item);

        // Grey out "Create Config Example" if SD card is not available
        if (i == 4 && !config_sd_available()) {
            lv_obj_set_style_text_color(item, lv_color_hex(0x808080), LV_PART_MAIN);  // Grey text
        }

        // lv_obj_set_style_bg_color(item, lv_color_hex(EMBED_COLOR_FOCUS_ON), LV_STATE_FOCUS_KEY);

        lv_obj_remove_style(item, NULL, LV_STATE_FOCUS_KEY);
        lv_obj_set_style_radius(item, 5, LV_STATE_FOCUS_KEY);
        lv_obj_set_style_outline_color(item, lv_color_hex(EMBED_COLOR_FOCUS_ON), LV_STATE_FOCUS_KEY);
        lv_obj_set_style_outline_width(item, 2, LV_STATE_FOCUS_KEY);

        lv_obj_add_event_cb(item, setting_scr_event, LV_EVENT_CLICKED, (void *)i);
    }

    // setting3 - Theme label
    theme_label = lv_label_create(setting3);
    lv_obj_set_style_text_font(theme_label, FONT_BOLD_14, LV_PART_MAIN);
    lv_obj_align(theme_label, LV_ALIGN_RIGHT_MID, 0, 0);
    switch (setting_theme) {
        case UI_THEME_DARK:  lv_label_set_text(theme_label, "DARK");  break;
        case UI_THEME_LIGHT: lv_label_set_text(theme_label, "LIGHT"); break;
        default:
            break;
    }

    // setting5 - SD card status label
    lv_obj_t *sd_status_label = lv_label_create(setting5);
    lv_obj_set_style_text_font(sd_status_label, FONT_BOLD_14, LV_PART_MAIN);
    lv_obj_align(sd_status_label, LV_ALIGN_RIGHT_MID, 0, 0);
    if (config_sd_available()) {
        lv_label_set_text(sd_status_label, "SD OK");
        lv_obj_set_style_text_color(sd_status_label, lv_color_hex(0x00FF00), LV_PART_MAIN);  // Green
    } else {
        lv_label_set_text(sd_status_label, "NO SD");
        lv_obj_set_style_text_color(sd_status_label, lv_color_hex(0xFF0000), LV_PART_MAIN);  // Red
    }
    
    scr_back_btn_create(scr4_cont, scr4_btn_event_cb);
}
void entry4(void) {
    entry4_anim(scr4_cont);
    lv_group_set_wrap(lv_group_get_default(), true);
}
void exit4(void) {
    lv_group_set_wrap(lv_group_get_default(), false);
}
void destroy4(void) {
    tft.setRotation(display_rotation);
    rotation_setting = 0;
}
scr_lifecycle_t screen4 = {
    .create = create4,
    .entry = entry4,
    .exit = exit4,
    .destroy = destroy4,
};
#endif
//************************************[ screen 5 ]****************************************** battery
#if 1

lv_obj_t *scr5_cont;
lv_obj_t *batt_cont;
lv_timer_t *batt_timer;

lv_obj_t *batt_label;
lv_obj_t *batt_trans;

bool show_batt_type = true;

static void battery_set_line(lv_obj_t *label, const char *str1, const char *str2)
{
    int w2 = strlen(str2);
    int w1 = 30 - w2;
    lv_label_set_text_fmt(label, "%-*s%-*s", w1, str1, w2, str2);
}

void batt_trans_event_cb(lv_event_t *e)
{
    if(e->code == LV_EVENT_CLICKED) {
        show_batt_type = !show_batt_type;
    }
}

void batt_timer_event(lv_timer_t *t)
{
    char buf[16];
    if(show_batt_type) {
        lv_label_set_text(batt_label, "BQ25896");

        lv_label_set_text_fmt(batt_line[0], "VBUS --- %3.2f | VSYS --- %3.2f", (PPM.getVbusVoltage() *1.0 / 1000.0), (PPM.getSystemVoltage() * 1.0 / 1000.0));
        lv_label_set_text_fmt(batt_line[1], "VBAT --- %3.2f | ICHG --- %3.1f", (PPM.getBattVoltage() *1.0 / 1000.0), (PPM.getChargeCurrent() * 1.0));

        lv_snprintf(buf, 16, "%.2f", (PPM.getChargeTargetVoltage() * 1.0 / 1000.0));
        battery_set_line(batt_line[2], "VBAT Target:", buf);

        lv_snprintf(buf, 16, "%s", (PPM.isVbusIn() == true ? "Charging" : "Not charged"));
        battery_set_line(batt_line[3], "Charging Statu:", buf);

        lv_snprintf(buf, 16, "%.2fmA", PPM.getPrechargeCurr());
        battery_set_line(batt_line[4], "Precharge Curr:", buf);

        lv_snprintf(buf, 16, "%s", PPM.getChargeStatusString());
        battery_set_line(batt_line[5], "CHG Status:", buf);

        lv_snprintf(buf, 16, "%s", PPM.getBusStatusString());
        battery_set_line(batt_line[6], "VBUS Status:", buf);
    } else {
        lv_label_set_text(batt_label, "BQ27220");

        lv_snprintf(buf, 16, "%s", (bq27220.getIsCharging() == true ? "Connected" : "Disonnected"));
        battery_set_line(batt_line[0], "VBUS Input:", buf);

        if(bq27220.getIsCharging() == true ){
            lv_snprintf(buf, 16, "%s", (bq27220.getCharingFinish()? "Finsish":"Charging"));
        } else {
            lv_snprintf(buf, 16, "%s", "Discharge");
        }
        battery_set_line(batt_line[1], "Charing Status:", buf);

        lv_snprintf(buf, 16, "%dmV", bq27220.getVoltage());
        battery_set_line(batt_line[2], "Voltage:", buf);

        lv_snprintf(buf, 16, "%dmA", bq27220.getCurrent());
        battery_set_line(batt_line[3], "Current:", buf);

        lv_snprintf(buf, 16, "%.2f", (float)(bq27220.getTemperature() / 10.0 - 273.0));
        battery_set_line(batt_line[4], "Temperature:", buf);

        lv_snprintf(buf, 16, "%d/%d", bq27220.getRemainingCapacity(), bq27220.getFullChargeCapacity());
        battery_set_line(batt_line[5], "Capacity:", buf);

        lv_snprintf(buf, 16, "%d", bq27220.getStateOfCharge());
        battery_set_line(batt_line[6], "Capacity Percent:", buf);
    }
}

void entry5_anim(lv_obj_t *obj)
{
    entry1_anim(obj);
}

void exit5_anim(int user_data, lv_obj_t *obj)
{
    exit1_anim(user_data, obj);
}

static void scr5_btn_event_cb(lv_event_t * e)
{
    if(e->code == LV_EVENT_CLICKED){
        // scr_mgr_set_anim(LV_SCR_LOAD_ANIM_FADE_OUT, -1, -1);
        // scr_mgr_switch(SCREEN0_ID, false);
        exit5_anim(SCREEN0_ID, scr5_cont);
    }
}

void create5(lv_obj_t *parent) 
{
    scr5_cont = create_screen_container(parent);

    batt_label = lv_label_create(scr5_cont);
    apply_text_color(batt_label);
    lv_obj_set_style_text_font(batt_label, FONT_BOLD_16, LV_PART_MAIN);
    lv_label_set_text(batt_label, "BQ25896");
    lv_obj_align(batt_label, LV_ALIGN_TOP_MID, 0, 10);

    batt_cont = lv_obj_create(scr5_cont);
    lv_obj_set_size(batt_cont, DISPALY_WIDTH, 130);
    lv_obj_align(batt_cont, LV_ALIGN_BOTTOM_MID, 0, 0);
    apply_no_radius(batt_cont);
    apply_no_padding(batt_cont);
    lv_obj_set_flex_flow(batt_cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(batt_cont, 1, LV_PART_MAIN);
    apply_no_border(batt_cont);
    apply_bg_color(batt_cont);
    apply_no_scrollbar(batt_cont);
    
    lv_obj_t *label1; // init label style
    for(int i = 0; i < BAT_INFO_LIEN_NUM; i++) {
        batt_line[i] = scr5_add_info_lab(batt_cont, label1, " ");
    }

    batt_trans = lv_btn_create(parent);
    // lv_group_add_obj(lv_group_get_default(), btn);
    lv_obj_set_style_pad_all(batt_trans, 0, 0);
    lv_obj_set_height(batt_trans, 20);
    lv_obj_align(batt_trans, LV_ALIGN_TOP_RIGHT, -8, 8);
    apply_no_border(batt_trans);
    apply_no_shadow(batt_trans);
    apply_bg_color(batt_trans);
    lv_obj_remove_style(batt_trans, NULL, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_pad(batt_trans, 0, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_width(batt_trans, 2, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_color(batt_trans, lv_color_hex(EMBED_COLOR_FOCUS_ON), LV_STATE_FOCUS_KEY);
    lv_obj_add_event_cb(batt_trans, batt_trans_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *label2 = lv_label_create(batt_trans);
    lv_obj_align(label2, LV_ALIGN_LEFT_MID, 0, 0);
    apply_text_color(label2);
    lv_label_set_text(label2, " " LV_SYMBOL_REFRESH " ");

    // back btn
    scr_back_btn_create(scr5_cont, scr5_btn_event_cb);
}
void entry5(void) {   
    entry5_anim(scr5_cont);
    batt_timer = lv_timer_create(batt_timer_event, 1000, NULL);
    lv_group_set_wrap(lv_group_get_default(), true);
}
void exit5(void) {
    lv_timer_del(batt_timer);
    batt_timer = NULL;
    lv_group_set_wrap(lv_group_get_default(), false);
}
void destroy5(void) { 
}

scr_lifecycle_t screen5 = {
    .create = create5,
    .entry = entry5,
    .exit  = exit5,
    .destroy = destroy5,
};
#endif
//************************************[ screen 6 ]****************************************** WiFi Tools Menu
#if 1
lv_obj_t *scr6_cont;
lv_obj_t *wifi_item_cont;
lv_obj_t *wifi_menu_cont;
lv_obj_t *wifi_menu_icon;
lv_obj_t *wifi_menu_icon_lab;
lv_obj_t *wifi_config_status_lab = NULL;  // Status label for SmartConfig in left panel
lv_obj_t *wifi_first_btn = NULL;  // Store first button to focus it

int wifi_focus_item = 0;

const char *wifi_menu_items[] = {
    "<- Scanner",
    "<- Nautilus Portal",
    "<- Deauth Hunter",
    "<- PineAP Hunter",
    "<- Config",
    "<- Help",
    "<- Back"
};

void entry6_anim(lv_obj_t *obj);
void exit6_anim(int user_data, lv_obj_t *obj);

// SmartConfig state (used in WiFi menu)
static volatile bool smartConfigStart      = false;
static lv_timer_t   *wifi_timer            = NULL;
static uint32_t      wifi_timer_counter    = 0;
static uint32_t      wifi_connnect_timeout = 60;

static void wifi_scroll_event_cb(lv_event_t * e)
{
    lv_obj_t * cont = lv_event_get_target(e);

    lv_area_t cont_a;
    lv_obj_get_coords(cont, &cont_a);
    lv_coord_t cont_y_center = cont_a.y1 + lv_area_get_height(&cont_a) / 2;

    lv_coord_t r = lv_obj_get_height(cont) * 7 / 10;
    uint32_t i;
    uint32_t child_cnt = lv_obj_get_child_cnt(cont);
    for(i = 0; i < child_cnt; i++) {
        lv_obj_t * child = lv_obj_get_child(cont, i);
        lv_area_t child_a;
        lv_obj_get_coords(child, &child_a);

        lv_coord_t child_y_center = child_a.y1 + lv_area_get_height(&child_a) / 2;

        lv_coord_t diff_y = child_y_center - cont_y_center;
        diff_y = LV_ABS(diff_y);

        /*Get the x of diff_y on a circle.*/
        lv_coord_t x;
        /*If diff_y is out of the circle use the last point of the circle (the radius)*/
        if(diff_y >= r) {
            x = r;
        }
        else {
            x =  (diff_y * 0.866);
        }

        /*Translate the item by the calculated X coordinate*/
        lv_obj_set_style_translate_x(child, x, 0);

        /*Use some opacity with larger translations*/
        lv_opa_t opa = lv_map(x, 0, r, LV_OPA_TRANSP, LV_OPA_COVER);
        lv_obj_set_style_opa(child, LV_OPA_COVER - opa, 0);
    }
}

static void wifi_btn_event_cb(lv_event_t * e)
{
    int data = (int)(intptr_t)e->user_data;

    if(e->code == LV_EVENT_CLICKED){
        wifi_focus_item = data;

        // Navigate to specific WiFi screens
        switch(wifi_focus_item) {
            case 0: // Scanner
                scr_mgr_switch(SCREEN6_1_ID, false);
                break;
            case 1: // Nautilus Portal
                scr_mgr_switch(SCREEN12_ID, false);
                break;
            case 2: // Deauth Hunter
                scr_mgr_switch(SCREEN6_2_ID, false);
                break;
            case 3: // PineAP Hunter
                scr_mgr_switch(SCREEN6_3_ID, false);
                break;
            case 4: // Config - Launch SmartConfig directly

                if(wifi_is_connect){
                    prompt_info(" WiFi is connected\n Already configured.", 2000);
                    break;
                }

                if (smartConfigStart) {
                    // Stop SmartConfig if already running
                    if (wifi_timer) {
                        lv_timer_del(wifi_timer);
                        wifi_timer = NULL;
                    }
                    WiFi.stopSmartConfig();
                    smartConfigStart = false;
                    if(wifi_config_status_lab) {
                        lv_label_set_text(wifi_config_status_lab, "Stopped");
                    }
                } else {
                    // Start SmartConfig
                    WiFi.disconnect();
                    smartConfigStart = true;
                    WiFi.beginSmartConfig();

                    if(wifi_config_status_lab) {
                        lv_label_set_text(wifi_config_status_lab, "Starting...");
                    }

                    wifi_timer = lv_timer_create([](lv_timer_t *t) {
                        static int step = 0;
                        bool destory = false;
                        wifi_timer_counter++;

                        if (wifi_timer_counter > wifi_connnect_timeout && !WiFi.isConnected()) {
                            destory = true;
                            if(wifi_config_status_lab) {
                                lv_label_set_text(wifi_config_status_lab, "Timeout");
                            }
                        } else {
                            // Animate status
                            if(wifi_config_status_lab) {
                                switch (step) {
                                    case 0: lv_label_set_text(wifi_config_status_lab, "Config\n-"); break;
                                    case 1: lv_label_set_text(wifi_config_status_lab, "Config\n/"); break;
                                    case 2: lv_label_set_text(wifi_config_status_lab, "Config\n-"); break;
                                    case 3: lv_label_set_text(wifi_config_status_lab, "Config\n\\"); break;
                                    default: break;
                                }
                            }
                            step++;
                            step &= 0x3;
                        }

                        if (WiFi.isConnected()) {

                            String ssid = WiFi.SSID();
                            String pwsd = WiFi.psk();
                            if(strcmp(wifi_ssid, ssid.c_str()) != 0 ||
                               strcmp(wifi_password, pwsd.c_str()) != 0) {
                                memcpy(wifi_ssid, ssid.c_str(), WIFI_SSID_MAX_LEN);
                                memcpy(wifi_password, pwsd.c_str(), WIFI_PSWD_MAX_LEN);
                                eeprom_wr_wifi(ssid.c_str(), ssid.length(), pwsd.c_str(), pwsd.length());

                                // Save to config file
                                g_config.wifi.ssid[0] = '\0';
                                g_config.wifi.password[0] = '\0';
                                strncat(g_config.wifi.ssid, ssid.c_str(), sizeof(g_config.wifi.ssid) - 1);
                                strncat(g_config.wifi.password, pwsd.c_str(), sizeof(g_config.wifi.password) - 1);
                                config_save();
                            }

                            destory = true;
                            wifi_is_connect = true;
                            if(wifi_config_status_lab) {
                                lv_label_set_text(wifi_config_status_lab, "Connected!");
                            }
                            prompt_info("  WiFi Connected!", 2000);
                        }

                        if (destory) {
                            WiFi.stopSmartConfig();
                            smartConfigStart = false;
                            lv_timer_del(wifi_timer);
                            wifi_timer = NULL;
                            wifi_timer_counter = 0;
                        }
                    }, 500, NULL);
                }
                break;
            case 5: // Help
                prompt_info(" You need to download\n 'EspTouch' app to\n configure WiFi.", 2000);
                break;
            case 6: // Back
                exit6_anim(SCREEN0_ID, scr6_cont);
                break;
            default:
                break;
        }
    }

    if(e->code == LV_EVENT_FOCUSED){
        // Update icon display based on focused item
        switch(data) {
            case 0: // Scanner
                lv_img_set_src(wifi_menu_icon, &img_wifi_32);
                lv_obj_set_style_img_recolor_opa(wifi_menu_icon, LV_OPA_0, LV_PART_MAIN);
                break;
            case 1: // Nautilus Portal
                lv_img_set_src(wifi_menu_icon, &img_wifi_32);
                lv_obj_set_style_img_recolor(wifi_menu_icon, lv_palette_main(LV_PALETTE_BLUE), LV_PART_MAIN);
                lv_obj_set_style_img_recolor_opa(wifi_menu_icon, LV_OPA_100, LV_PART_MAIN);
                break;
            case 2: // Deauth Hunter
                lv_img_set_src(wifi_menu_icon, &img_wifi_32);
                lv_obj_set_style_img_recolor(wifi_menu_icon, lv_palette_main(LV_PALETTE_RED), LV_PART_MAIN);
                lv_obj_set_style_img_recolor_opa(wifi_menu_icon, LV_OPA_100, LV_PART_MAIN);
                break;
            case 3: // PineAP Hunter
                lv_img_set_src(wifi_menu_icon, &img_wifi_32);
                lv_obj_set_style_img_recolor(wifi_menu_icon, lv_palette_main(LV_PALETTE_ORANGE), LV_PART_MAIN);
                lv_obj_set_style_img_recolor_opa(wifi_menu_icon, LV_OPA_100, LV_PART_MAIN);
                break;
            case 4: // Config
                lv_img_set_src(wifi_menu_icon, &img_setting_32);
                lv_obj_set_style_img_recolor_opa(wifi_menu_icon, LV_OPA_0, LV_PART_MAIN);
                break;
            case 5: // Help
                lv_img_set_src(wifi_menu_icon, &img_dev_32);
                lv_obj_set_style_img_recolor_opa(wifi_menu_icon, LV_OPA_0, LV_PART_MAIN);
                break;
            case 6: // Back
                lv_img_set_src(wifi_menu_icon, &img_dev_32);
                lv_obj_set_style_img_recolor_opa(wifi_menu_icon, LV_OPA_0, LV_PART_MAIN);
                break;
            default:
                break;
        }
        lv_label_set_text(wifi_menu_icon_lab, ((char*)wifi_menu_items[data] + 3));
        lv_obj_align_to(wifi_menu_icon_lab, wifi_menu_cont, LV_ALIGN_BOTTOM_MID, 0, -25);
    }
}

void entry6_anim(lv_obj_t *obj)
{
    // No animation for now - menu displays directly
}

void exit6_anim(int user_data, lv_obj_t *obj)
{
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, wifi_item_cont);
    lv_anim_set_values(&a, 0, DISPALY_WIDTH * MENU_PROPORTION);
    lv_anim_set_time(&a, 400);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_in_out);
    lv_anim_set_exec_cb(&a, [](void * var, int32_t v){
        lv_obj_set_x((lv_obj_t *)var, v);
        lv_port_indev_enabled(false);
    });
    lv_anim_set_ready_cb(&a, [](struct _lv_anim_t *a){
        scr_mgr_switch((int)a->user_data, false);
        lv_port_indev_enabled(true);
    });
    lv_anim_set_user_data(&a, (void *)user_data);
    lv_anim_start(&a);

    lv_anim_set_time(&a, 400);
    lv_anim_set_var(&a, wifi_menu_cont);
    lv_anim_set_values(&a, 0, -DISPALY_WIDTH * MENU_LAB_PROPORTION);
    lv_anim_set_exec_cb(&a, [](void * var, int32_t v){
        lv_obj_set_x((lv_obj_t *)var, v);
        lv_port_indev_enabled(false);
    });
    lv_anim_start(&a);
}

void create6(lv_obj_t *parent){
    scr6_cont = create_screen_container(parent);

    // Left side - Icon display panel (matches main menu layout)
    wifi_menu_cont = lv_obj_create(scr6_cont);
    lv_obj_set_size(wifi_menu_cont, DISPALY_WIDTH * MENU_LAB_PROPORTION, DISPALY_HEIGHT);
    apply_no_scrollbar(wifi_menu_cont);
    lv_obj_set_align(wifi_menu_cont, LV_ALIGN_LEFT_MID);
    apply_bg_color(wifi_menu_cont);
    apply_no_padding(wifi_menu_cont);
    apply_no_border(wifi_menu_cont);
    apply_no_radius(wifi_menu_cont);

    wifi_menu_icon = lv_img_create(wifi_menu_cont);
    lv_obj_align(wifi_menu_icon, LV_ALIGN_CENTER, 0, 0);

    wifi_menu_icon_lab = lv_label_create(wifi_menu_cont);
    lv_obj_set_width(wifi_menu_icon_lab, DISPALY_WIDTH * MENU_LAB_PROPORTION);
    lv_obj_set_style_text_align(wifi_menu_icon_lab, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    apply_text_color(wifi_menu_icon_lab);
    lv_obj_set_style_text_font(wifi_menu_icon_lab, FONT_BOLD_18, LV_PART_MAIN);
    lv_obj_align_to(wifi_menu_icon_lab, wifi_menu_cont, LV_ALIGN_BOTTOM_MID, 0, -25);

    // Status label for SmartConfig (hidden by default, shown when Config is selected)
    wifi_config_status_lab = lv_label_create(wifi_menu_cont);
    lv_obj_set_width(wifi_config_status_lab, DISPALY_WIDTH * MENU_LAB_PROPORTION - 10);
    lv_obj_set_style_text_align(wifi_config_status_lab, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    apply_text_color(wifi_config_status_lab);
    lv_obj_set_style_text_font(wifi_config_status_lab, FONT_BOLD_14, LV_PART_MAIN);
    lv_label_set_text(wifi_config_status_lab, "");
    lv_obj_align(wifi_config_status_lab, LV_ALIGN_TOP_MID, 0, 5);

    // Right side - Scrollable menu items (matches main menu layout)
    wifi_item_cont = lv_obj_create(scr6_cont);
    lv_obj_set_size(wifi_item_cont, DISPALY_WIDTH * MENU_PROPORTION, DISPALY_HEIGHT);
    lv_obj_set_align(wifi_item_cont, LV_ALIGN_RIGHT_MID);
    lv_obj_set_flex_flow(wifi_item_cont, LV_FLEX_FLOW_COLUMN);
    apply_bg_color(wifi_item_cont);
    apply_no_border(wifi_item_cont);
    lv_obj_add_event_cb(wifi_item_cont, wifi_scroll_event_cb, LV_EVENT_SCROLL, NULL);
    apply_no_radius(wifi_item_cont);
    lv_obj_set_style_clip_corner(wifi_item_cont, true, 0);
    lv_obj_set_scroll_dir(wifi_item_cont, LV_DIR_VER);
    lv_obj_set_scroll_snap_y(wifi_item_cont, LV_SCROLL_SNAP_CENTER);
    apply_no_scrollbar(wifi_item_cont);
    lv_obj_set_style_pad_row(wifi_item_cont, 10, LV_PART_MAIN);

    // Create 7 menu item buttons (6 features + Back)
    for(int i = 0; i < 7; i++) {
        lv_obj_t *btn = lv_btn_create(wifi_item_cont);
        lv_obj_set_width(btn, lv_pct(100));
        lv_obj_set_style_radius(btn, 10, LV_PART_MAIN);
        apply_bg_color(btn);
        apply_no_border(btn);
        apply_no_shadow(btn);
        lv_obj_remove_style(btn, NULL, LV_STATE_FOCUS_KEY);
        lv_obj_set_style_outline_width(btn, 2, LV_STATE_FOCUS_KEY);
        lv_obj_set_style_outline_color(btn, lv_color_hex(EMBED_COLOR_FOCUS_ON), LV_STATE_FOCUS_KEY);
        lv_obj_set_style_bg_img_opa(btn, LV_OPA_100, LV_PART_MAIN);
        add_menu_button_events(btn, wifi_btn_event_cb, (void *)(intptr_t)i);

        lv_obj_t *label = lv_label_create(btn);
        apply_text_color(label);
        lv_obj_set_style_text_font(label, FONT_BOLD_14, LV_PART_MAIN);
        lv_label_set_text(label, wifi_menu_items[i]);

        lv_group_add_obj(lv_group_get_default(), btn);

        // Store first button to focus it later
        if(i == 0) {
            wifi_first_btn = btn;
        }
    }

    // Add status bar last (on parent, not scr6_cont) so it appears on top
    create_status_bar(parent);
}

void entry6(void) {
    if(wifi_first_btn) {
        lv_group_focus_obj(wifi_first_btn);
    }
    lv_group_set_wrap(lv_group_get_default(), true);
}

void exit6(void) {
    lv_group_set_wrap(lv_group_get_default(), false);

    // Clean up SmartConfig if running
    if (smartConfigStart) {
        if (wifi_timer) {
            lv_timer_del(wifi_timer);
            wifi_timer = NULL;
        }
        WiFi.stopSmartConfig();
        smartConfigStart = false;
        wifi_timer_counter = 0;
    }

    // Clear status label
    if(wifi_config_status_lab) {
        lv_label_set_text(wifi_config_status_lab, "");
    }
}

void destroy6(void) {
    // Cleanup if needed
    wifi_config_status_lab = NULL;
}

scr_lifecycle_t screen6 = {
    .create = create6,
    .entry = entry6,
    .exit  = exit6,
    .destroy = destroy6,
};
#endif

// --------------------- screen 6.1 --------------------- WiFi Scanner
#if 1
// WiFi Scanner UI elements
lv_obj_t *scr6_1_cont;
lv_obj_t *wifi_scan_list;

// WiFi AP detail screen elements
lv_obj_t *scr6_1_1_cont;
lv_obj_t *wifi_detail_cont;  // Scrollable details container
lv_obj_t *wifi_detail_ssid_label;
lv_obj_t *wifi_detail_bssid_label;
lv_obj_t *wifi_detail_rssi_label;
lv_obj_t *wifi_detail_channel_label;
lv_obj_t *wifi_detail_encryption_label;

// WiFi Scanner state
bool wifi_scan_in_progress = false;
int wifi_selected_ap_index = -1;

// Structure to store AP information
struct WiFiAPInfo {
    String ssid;
    String bssid;
    int32_t rssi;
    uint8_t channel;
    uint8_t encryption;
};

std::vector<WiFiAPInfo> scanned_aps;

// Get encryption type as string
const char* getEncryptionType(uint8_t encType) {
    switch(encType) {
        case WIFI_AUTH_OPEN: return "Open";
        case WIFI_AUTH_WEP: return "WEP";
        case WIFI_AUTH_WPA_PSK: return "WPA-PSK";
        case WIFI_AUTH_WPA2_PSK: return "WPA2-PSK";
        case WIFI_AUTH_WPA_WPA2_PSK: return "WPA/WPA2-PSK";
        case WIFI_AUTH_WPA2_ENTERPRISE: return "WPA2-Enterprise";
        case WIFI_AUTH_WPA3_PSK: return "WPA3-PSK";
        case WIFI_AUTH_WPA2_WPA3_PSK: return "WPA2/WPA3-PSK";
        default: return "Unknown";
    }
}

// WiFi scan list event handler
static void wifi_scan_list_event(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *obj = lv_event_get_target(e);

    if (code == LV_EVENT_CLICKED) {
        const char *item_text = lv_list_get_btn_text(wifi_scan_list, obj);

        if (item_text) {
            // Check if it's the scan button
            if (strcmp(item_text, " [Scan]") == 0) {
                // Trigger scan
                if (!wifi_scan_in_progress) {
                    wifi_scan_in_progress = true;
                    prompt_info("  Scanning...", 500);

                    // Clear previous results
                    scanned_aps.clear();
                    lv_obj_clean(wifi_scan_list);

                    // Add scanning message
                    lv_obj_t *item = lv_list_add_btn(wifi_scan_list, NULL, " Scanning WiFi...");
                    lv_obj_set_style_text_font(item, FONT_BOLD_14, LV_PART_MAIN);
                    apply_bg_color(item);
                    apply_text_color(item);
                    lv_refr_now(NULL);

                    // Perform scan
                    int n = WiFi.scanNetworks();

                    // Clear scanning message
                    lv_obj_clean(wifi_scan_list);

                    if (n == 0) {
                        lv_obj_t *no_nets = lv_list_add_btn(wifi_scan_list, NULL, " No networks found");
                        lv_obj_set_style_text_font(no_nets, FONT_BOLD_14, LV_PART_MAIN);
                        apply_bg_color(no_nets);
                        apply_text_color(no_nets);
                    } else {
                        // Store AP information
                        for (int i = 0; i < n; i++) {
                            WiFiAPInfo ap;
                            ap.ssid = WiFi.SSID(i);
                            ap.bssid = WiFi.BSSIDstr(i);
                            ap.rssi = WiFi.RSSI(i);
                            ap.channel = WiFi.channel(i);
                            ap.encryption = WiFi.encryptionType(i);
                            scanned_aps.push_back(ap);
                        }

                        // Sort by RSSI (strongest first)
                        std::sort(scanned_aps.begin(), scanned_aps.end(),
                            [](const WiFiAPInfo& a, const WiFiAPInfo& b) {
                                return a.rssi > b.rssi;
                            });

                        // Create list items with compact format: [dBm] SSID
                        for (size_t i = 0; i < scanned_aps.size(); i++) {
                            char buf[128];
                            String ssid_display = scanned_aps[i].ssid;
                            if (ssid_display.length() == 0) {
                                ssid_display = "<Hidden>";
                            }
                            // Limit SSID length for display
                            if (ssid_display.length() > 20) {
                                ssid_display = ssid_display.substring(0, 17) + "...";
                            }
                            snprintf(buf, sizeof(buf), " [%d] %s",
                                     scanned_aps[i].rssi, ssid_display.c_str());

                            lv_obj_t *item = lv_list_add_btn(wifi_scan_list, NULL, buf);
                            lv_obj_add_event_cb(item, wifi_scan_list_event, LV_EVENT_ALL, (void*)(intptr_t)i);
                        }

                        // Style all list items
                        for (int i = 0; i < lv_obj_get_child_cnt(wifi_scan_list); i++) {
                            lv_obj_t *item = lv_obj_get_child(wifi_scan_list, i);
                            lv_obj_set_style_text_font(item, FONT_BOLD_14, LV_PART_MAIN);
                            apply_bg_color(item);
                            apply_text_color(item);
                            lv_obj_remove_style(item, NULL, LV_STATE_FOCUS_KEY);
                            lv_obj_set_style_radius(item, 5, LV_STATE_FOCUS_KEY);
                            lv_obj_set_style_outline_color(item, lv_color_hex(EMBED_COLOR_FOCUS_ON), LV_STATE_FOCUS_KEY);
                            lv_obj_set_style_outline_width(item, 2, LV_STATE_FOCUS_KEY);
                        }
                    }

                    WiFi.scanDelete();

                    // Add scan button at the end
                    lv_obj_t *scan_item = lv_list_add_btn(wifi_scan_list, NULL, " [Scan]");
                    lv_obj_add_event_cb(scan_item, wifi_scan_list_event, LV_EVENT_ALL, NULL);
                    lv_obj_set_style_text_font(scan_item, FONT_BOLD_14, LV_PART_MAIN);
                    apply_bg_color(scan_item);
                    apply_text_color(scan_item);
                    lv_obj_remove_style(scan_item, NULL, LV_STATE_FOCUS_KEY);
                    lv_obj_set_style_radius(scan_item, 5, LV_STATE_FOCUS_KEY);
                    lv_obj_set_style_outline_color(scan_item, lv_color_hex(EMBED_COLOR_FOCUS_ON), LV_STATE_FOCUS_KEY);
                    lv_obj_set_style_outline_width(scan_item, 2, LV_STATE_FOCUS_KEY);

                    wifi_scan_in_progress = false;
                }
            } else {
                // It's an AP item - get the index from user_data
                int idx = (int)(intptr_t)e->user_data;
                if (idx >= 0 && idx < (int)scanned_aps.size()) {
                    wifi_selected_ap_index = idx;
                    scr_mgr_switch(SCREEN6_1_1_ID, false);
                }
            }
        }
    }
}

// Back button callback for scanner
static void wifi_scanner_back_btn_cb(lv_event_t *e) {
    scr_mgr_switch(SCREEN6_ID, false);
}

// Back button callback for detail screen
static void wifi_detail_back_btn_cb(lv_event_t *e) {
    scr_mgr_switch(SCREEN6_1_ID, false);
}

void create6_1(lv_obj_t *parent) {
    scr6_1_cont = lv_obj_create(parent);
    lv_obj_set_size(scr6_1_cont, LV_PCT(100), LV_PCT(100));
    apply_bg_color(scr6_1_cont);
    apply_no_scrollbar(scr6_1_cont);
    apply_no_border(scr6_1_cont);
    apply_no_padding(scr6_1_cont);

    // Back button
    scr_back_btn_create(scr6_1_cont, wifi_scanner_back_btn_cb);

    // Title
    lv_obj_t *title = lv_label_create(scr6_1_cont);
    lv_label_set_text(title, "WiFi Scanner");
    lv_obj_set_style_text_font(title, FONT_BOLD_20, 0);
    apply_text_color(title);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

    // Create list widget (like file browser)
    wifi_scan_list = lv_list_create(scr6_1_cont);
    lv_obj_set_size(wifi_scan_list, LV_PCT(90), 175);
    lv_obj_align(wifi_scan_list, LV_ALIGN_TOP_MID, 0, 35);
    apply_bg_color(wifi_scan_list);
    apply_no_border(wifi_scan_list);
    apply_no_padding(wifi_scan_list);

    // Add initial "Scan" button
    lv_obj_t *scan_item = lv_list_add_btn(wifi_scan_list, NULL, " [Scan]");
    lv_obj_add_event_cb(scan_item, wifi_scan_list_event, LV_EVENT_ALL, NULL);
    lv_obj_set_style_text_font(scan_item, FONT_BOLD_14, LV_PART_MAIN);
    apply_bg_color(scan_item);
    apply_text_color(scan_item);
    lv_obj_remove_style(scan_item, NULL, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_radius(scan_item, 5, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_color(scan_item, lv_color_hex(EMBED_COLOR_FOCUS_ON), LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_width(scan_item, 2, LV_STATE_FOCUS_KEY);
}

void entry6_1(void) {
    lv_group_set_wrap(lv_group_get_default(), true);
}

void exit6_1(void) {
    lv_group_set_wrap(lv_group_get_default(), false);
}

void destroy6_1(void) {
    // Don't clear scanned_aps here - we need it for the detail screen
    // It will be cleared when starting a new scan
}

scr_lifecycle_t screen6_1 = {
    .create = create6_1,
    .entry = entry6_1,
    .exit  = exit6_1,
    .destroy = destroy6_1,
};

// --------------------- screen 6.1.1 --------------------- WiFi AP Details
void create6_1_1(lv_obj_t *parent) {
    scr6_1_1_cont = lv_obj_create(parent);
    lv_obj_set_size(scr6_1_1_cont, LV_PCT(100), LV_PCT(100));
    apply_bg_color(scr6_1_1_cont);
    apply_no_scrollbar(scr6_1_1_cont);
    apply_no_border(scr6_1_1_cont);
    apply_no_padding(scr6_1_1_cont);

    // Back button
    scr_back_btn_create(scr6_1_1_cont, wifi_detail_back_btn_cb);

    // Title
    lv_obj_t *title = lv_label_create(scr6_1_1_cont);
    lv_label_set_text(title, "AP Details");
    lv_obj_set_style_text_font(title, FONT_BOLD_20, 0);
    apply_text_color(title);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

    // Details container - made tall enough to show all content
    wifi_detail_cont = lv_obj_create(scr6_1_1_cont);
    lv_obj_set_size(wifi_detail_cont, LV_PCT(90), 195);
    lv_obj_align(wifi_detail_cont, LV_ALIGN_TOP_MID, 0, 35);
    lv_obj_set_flex_flow(wifi_detail_cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(wifi_detail_cont, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(wifi_detail_cont, 8, 0);
    lv_obj_set_style_pad_row(wifi_detail_cont, 3, 0);  // Reduce spacing between labels
    apply_no_border(wifi_detail_cont);
    apply_bg_color(wifi_detail_cont);
    apply_no_scrollbar(wifi_detail_cont);

    // SSID label
    wifi_detail_ssid_label = lv_label_create(wifi_detail_cont);
    lv_label_set_text(wifi_detail_ssid_label, "SSID: ");
    lv_obj_set_style_text_font(wifi_detail_ssid_label, FONT_BOLD_14, 0);
    apply_text_color(wifi_detail_ssid_label);
    lv_label_set_long_mode(wifi_detail_ssid_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(wifi_detail_ssid_label, LV_PCT(100));

    // BSSID label
    wifi_detail_bssid_label = lv_label_create(wifi_detail_cont);
    lv_label_set_text(wifi_detail_bssid_label, "BSSID: ");
    lv_obj_set_style_text_font(wifi_detail_bssid_label, FONT_BOLD_14, 0);
    apply_text_color(wifi_detail_bssid_label);

    // RSSI label
    wifi_detail_rssi_label = lv_label_create(wifi_detail_cont);
    lv_label_set_text(wifi_detail_rssi_label, "Signal: ");
    lv_obj_set_style_text_font(wifi_detail_rssi_label, FONT_BOLD_14, 0);
    apply_text_color(wifi_detail_rssi_label);

    // Channel label
    wifi_detail_channel_label = lv_label_create(wifi_detail_cont);
    lv_label_set_text(wifi_detail_channel_label, "Channel: ");
    lv_obj_set_style_text_font(wifi_detail_channel_label, FONT_BOLD_14, 0);
    apply_text_color(wifi_detail_channel_label);

    // Encryption label
    wifi_detail_encryption_label = lv_label_create(wifi_detail_cont);
    lv_label_set_text(wifi_detail_encryption_label, "Encryption: ");
    lv_obj_set_style_text_font(wifi_detail_encryption_label, FONT_BOLD_14, 0);
    apply_text_color(wifi_detail_encryption_label);
    lv_label_set_long_mode(wifi_detail_encryption_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(wifi_detail_encryption_label, LV_PCT(100));
}

void entry6_1_1(void) {
    lv_group_set_wrap(lv_group_get_default(), true);

    // Update labels with selected AP info
    if (wifi_selected_ap_index >= 0 && wifi_selected_ap_index < (int)scanned_aps.size()) {
        WiFiAPInfo &ap = scanned_aps[wifi_selected_ap_index];

        char buf[256];

        // SSID (two lines - label and value)
        String ssid_display = ap.ssid;
        if (ssid_display.length() == 0) {
            ssid_display = "<Hidden SSID>";
        }
        snprintf(buf, sizeof(buf), "SSID:\n  %s", ssid_display.c_str());
        lv_label_set_text(wifi_detail_ssid_label, buf);

        // BSSID (one line)
        snprintf(buf, sizeof(buf), "BSSID: %s", ap.bssid.c_str());
        lv_label_set_text(wifi_detail_bssid_label, buf);

        // Signal (one line)
        snprintf(buf, sizeof(buf), "Signal: %d dBm", ap.rssi);
        lv_label_set_text(wifi_detail_rssi_label, buf);

        // Channel (one line)
        snprintf(buf, sizeof(buf), "Channel: %d", ap.channel);
        lv_label_set_text(wifi_detail_channel_label, buf);

        // Encryption (one line, will wrap if needed)
        snprintf(buf, sizeof(buf), "Encryption: %s", getEncryptionType(ap.encryption));
        lv_label_set_text(wifi_detail_encryption_label, buf);
    }
}

void exit6_1_1(void) {
    lv_group_set_wrap(lv_group_get_default(), false);
}

void destroy6_1_1(void) {
}

scr_lifecycle_t screen6_1_1 = {
    .create = create6_1_1,
    .entry = entry6_1_1,
    .exit  = exit6_1_1,
    .destroy = destroy6_1_1,
};
#endif

// --------------------- screen 6.2 --------------------- Deauth Hunter
#if 1
#include "esp_wifi.h"
#include <vector>

// Deauth Hunter UI elements
lv_obj_t *scr6_2_cont;
lv_obj_t *deauth_channel_label;
lv_obj_t *deauth_aps_label;
lv_obj_t *deauth_packets_label;
lv_obj_t *deauth_threshold_label;
lv_obj_t *deauth_rssi_scale_label;
lv_obj_t *deauth_rssi_bar;
lv_obj_t *deauth_rssi_label;
lv_obj_t *deauth_btn_start;
lv_obj_t *deauth_btn_threshold;
lv_obj_t *deauth_btn_rssi_scale;
lv_timer_t *deauth_update_timer;

// Deauth Hunter state
bool deauth_hunter_active = false;
bool deauth_channel_hopping_active = true;  // Controls whether we hop channels or stay on current
int deauth_current_channel_idx = 0;
int deauth_rssi_scale_dbm = -20;  // RSSI scale (max for bar) in dBm - loaded from config
int deauth_packet_threshold = 13;  // Packet count threshold (Fibonacci) - loaded from config
int deauth_packet_threshold_idx = 5;  // Index in Fibonacci sequence (13 is at index 5)
unsigned long deauth_last_channel_change = 0;
unsigned long deauth_stats_reset_time = 0;

// Fibonacci sequence for packet threshold
const int deauth_fib_sequence[] = {1, 2, 3, 5, 8, 13, 21, 34, 55, 89, 144};
const int deauth_fib_count = sizeof(deauth_fib_sequence) / sizeof(deauth_fib_sequence[0]);

// WiFi frame structure definitions (from ESP32Marauder)
typedef struct {
    int16_t fctl;
    int16_t duration;
    uint8_t da[6];
    uint8_t sa[6];
    uint8_t bssid[6];
    int16_t seqctl;
    unsigned char payload[];
} __attribute__((packed)) WifiMgmtHdr;

typedef struct {
    WifiMgmtHdr hdr;
    uint8_t payload[0];
} wifi_ieee80211_packet_t;

// Deauth statistics
struct DeauthStats {
    uint32_t total_deauths = 0;
    uint32_t unique_aps = 0;
    int32_t last_rssi = -100;  // RSSI of last seen deauth packet
    uint32_t last_reset_time = 0;
} deauth_stats;

// WiFi channels to scan
const uint8_t WIFI_CHANNELS[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13};
const uint8_t NUM_WIFI_CHANNELS = sizeof(WIFI_CHANNELS) / sizeof(WIFI_CHANNELS[0]);

// Unique AP tracking
std::vector<String> deauth_seen_ap_macs;

// Helper to extract MAC address from packet
static void deauth_extract_mac(char *addr, uint8_t* data, uint16_t offset) {
    sprintf(addr, "%02x:%02x:%02x:%02x:%02x:%02x",
            data[offset], data[offset+1], data[offset+2],
            data[offset+3], data[offset+4], data[offset+5]);
}

// Add unique AP to tracking list
void deauth_add_unique_ap(const char* mac) {
    String mac_str(mac);
    for (const auto& seen_mac : deauth_seen_ap_macs) {
        if (seen_mac == mac_str) {
            return;  // Already seen
        }
    }
    deauth_seen_ap_macs.push_back(mac_str);
    deauth_stats.unique_aps = deauth_seen_ap_macs.size();
}

// Deauth packet sniffer callback (adapted from ESP32Marauder)
static void deauth_sniffer_callback(void* buf, wifi_promiscuous_pkt_type_t type) {
    if(!deauth_hunter_active) return;

    wifi_promiscuous_pkt_t *snifferPacket = (wifi_promiscuous_pkt_t*)buf;

    if (type == WIFI_PKT_MGMT) {
        wifi_ieee80211_packet_t *ipkt = (wifi_ieee80211_packet_t *)snifferPacket->payload;
        WifiMgmtHdr *hdr = &ipkt->hdr;

        // Check for deauth (0xC0) or disassoc (0xA0) frames
        uint8_t frame_type = (hdr->fctl & 0xFF);
        if (frame_type == 0xC0 || frame_type == 0xA0) {
            deauth_stats.total_deauths++;

            // Extract BSSID and add to unique AP list
            char mac[18];
            deauth_extract_mac(mac, snifferPacket->payload, 16);
            deauth_add_unique_ap(mac);

            // Store RSSI of last seen packet
            deauth_stats.last_rssi = snifferPacket->rx_ctrl.rssi;
        }
    }
}

// Start deauth monitoring
void deauth_start_monitoring() {
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100);

    // Enable promiscuous mode
    esp_wifi_set_promiscuous(true);
    esp_wifi_set_promiscuous_rx_cb(&deauth_sniffer_callback);

    // Set initial channel
    esp_wifi_set_channel(WIFI_CHANNELS[deauth_current_channel_idx], WIFI_SECOND_CHAN_NONE);

    deauth_hunter_active = true;
    deauth_stats_reset_time = millis();
}

// Stop deauth monitoring
void deauth_stop_monitoring() {
    esp_wifi_set_promiscuous(false);
    deauth_hunter_active = false;
    WiFi.mode(WIFI_MODE_STA);
}

// Channel hopping
void deauth_hop_channel() {
    deauth_current_channel_idx = (deauth_current_channel_idx + 1) % NUM_WIFI_CHANNELS;
    esp_wifi_set_channel(WIFI_CHANNELS[deauth_current_channel_idx], WIFI_SECOND_CHAN_NONE);
}

// Reset stats if needed (every 60 seconds)
void deauth_reset_stats_if_needed() {
    unsigned long now = millis();
    if (now - deauth_stats_reset_time >= 60000) {
        deauth_stats.total_deauths = 0;
        deauth_stats.unique_aps = 0;
        deauth_stats.last_rssi = -100;
        deauth_seen_ap_macs.clear();
        deauth_stats_reset_time = now;
    }
}

// Calculate RSSI bar value (0-100) based on scale
int deauth_calculate_rssi_bars(int rssi, int scale_max) {
    // Map RSSI from -100 to scale_max dBm to 0-100%
    // Example: if scale is -70 dBm, then -100 to -70 maps to 0-100%
    if (rssi >= scale_max) return 100;
    if (rssi <= -100) return 0;

    // Linear mapping: (-100 to scale_max) -> (0 to 100)
    int range = scale_max - (-100);  // e.g., -70 - (-100) = 30
    int offset = rssi - (-100);  // e.g., -85 - (-100) = 15
    return (100 * offset) / range;  // (15 * 100) / 30 = 50
}

// Update timer callback
static void deauth_update_timer_event(lv_timer_t *timer) {
    if (!deauth_hunter_active) return;

    // Channel hopping every 250ms (only if hopping is active)
    unsigned long now = millis();
    if (deauth_channel_hopping_active && now - deauth_last_channel_change >= 250) {
        deauth_hop_channel();
        deauth_last_channel_change = now;
    }

    // Reset stats if needed
    deauth_reset_stats_if_needed();

    // Update UI
    lv_label_set_text_fmt(deauth_channel_label, "Channel: %d", WIFI_CHANNELS[deauth_current_channel_idx]);
    lv_label_set_text_fmt(deauth_aps_label, "APs: %lu", deauth_stats.unique_aps);
    lv_label_set_text_fmt(deauth_packets_label, "Packets: %lu", deauth_stats.total_deauths);

    // Update RSSI bar and label
    int rssi_val = deauth_calculate_rssi_bars(deauth_stats.last_rssi, deauth_rssi_scale_dbm);
    lv_bar_set_value(deauth_rssi_bar, rssi_val, LV_ANIM_OFF);
    lv_label_set_text_fmt(deauth_rssi_label, "RSSI: %d dBm", deauth_stats.last_rssi);

    // Change bar color based on RSSI
    if (rssi_val > 70) {
        lv_obj_set_style_bg_color(deauth_rssi_bar, lv_palette_main(LV_PALETTE_GREEN), LV_PART_INDICATOR);
    } else if (rssi_val > 40) {
        lv_obj_set_style_bg_color(deauth_rssi_bar, lv_palette_main(LV_PALETTE_YELLOW), LV_PART_INDICATOR);
    } else {
        lv_obj_set_style_bg_color(deauth_rssi_bar, lv_palette_main(LV_PALETTE_RED), LV_PART_INDICATOR);
    }

    // Check if packet count threshold exceeded for alarm (only beep while scanning/hopping)
    static unsigned long last_beep_time = 0;
    static const unsigned long BEEP_INTERVAL = 1000;  // Beep every 1 second while threshold exceeded

    if (deauth_channel_hopping_active && deauth_stats.total_deauths >= (uint32_t)deauth_packet_threshold) {
        unsigned long now = millis();

        // Beep periodically (every BEEP_INTERVAL ms)
        if (now - last_beep_time >= BEEP_INTERVAL) {
            // Simple beep using LEDC PWM
            // 1000Hz tone for 100ms on LEDC channel 0
            ledcSetup(0, 1000, 8);  // channel, freq, resolution
            ledcAttachPin(7, 0);     // pin 7 (BOARD_VOICE_DIN), channel 0
            ledcWrite(0, 128);       // 50% duty cycle
            delay(100);
            ledcWrite(0, 0);         // Stop tone
            ledcDetachPin(7);        // Detach pin

            last_beep_time = now;
        }
    } else {
        // Reset beep timer when below threshold or when paused
        last_beep_time = 0;
    }
}

// Start/Pause button event
static void deauth_btn_start_event(lv_event_t *e) {
    if (e->code == LV_EVENT_CLICKED) {
        if (!deauth_hunter_active) {
            // Start monitoring and channel hopping
            deauth_start_monitoring();
            deauth_channel_hopping_active = true;
            lv_obj_set_style_bg_img_src(deauth_btn_start, &img_play_32, 0);
        } else {
            // Toggle channel hopping (keep monitoring on current channel)
            deauth_channel_hopping_active = !deauth_channel_hopping_active;
            if (deauth_channel_hopping_active) {
                lv_obj_set_style_bg_img_src(deauth_btn_start, &img_play_32, 0);
            } else {
                lv_obj_set_style_bg_img_src(deauth_btn_start, &img_pause_32, 0);
            }
        }
    }
}

// Packet threshold adjustment button event (Fibonacci sequence)
static void deauth_btn_threshold_event(lv_event_t *e) {
    if (e->code == LV_EVENT_CLICKED) {
        // Cycle through Fibonacci sequence: 1, 2, 3, 5, 8, 13, 21, 34, 55, 89, 144
        deauth_packet_threshold_idx = (deauth_packet_threshold_idx + 1) % deauth_fib_count;
        deauth_packet_threshold = deauth_fib_sequence[deauth_packet_threshold_idx];
        lv_label_set_text_fmt(deauth_threshold_label, "%d", deauth_packet_threshold);

        // Save to config
        g_config.wifi.deauth.packet_threshold = deauth_packet_threshold;
        config_save();
    }
}

// RSSI Scale adjustment button event
static void deauth_btn_rssi_scale_event(lv_event_t *e) {
    if (e->code == LV_EVENT_CLICKED) {
        // Cycle through RSSI scales: -90, -85, -80, -75, -70, -65, -60, -55, -50, -45, -40, -35, -30, -25, -20, -15, -10
        deauth_rssi_scale_dbm += 5;
        if (deauth_rssi_scale_dbm > -10) {
            deauth_rssi_scale_dbm = -90;
        }
        lv_label_set_text_fmt(deauth_rssi_scale_label, "%d dBm", deauth_rssi_scale_dbm);

        // Save to config
        g_config.wifi.deauth.rssi_scale_dbm = deauth_rssi_scale_dbm;
        config_save();
    }
}

// Back button event
static void deauth_btn_back_event(lv_event_t *e) {
    if (e->code == LV_EVENT_CLICKED) {
        scr_mgr_switch(SCREEN6_ID, false);
    }
}

static void create6_2(lv_obj_t *parent) {
    // Load settings from config
    deauth_rssi_scale_dbm = g_config.wifi.deauth.rssi_scale_dbm;
    deauth_packet_threshold = g_config.wifi.deauth.packet_threshold;

    // Calculate threshold index from threshold value
    for (int i = 0; i < deauth_fib_count; i++) {
        if (deauth_fib_sequence[i] == deauth_packet_threshold) {
            deauth_packet_threshold_idx = i;
            break;
        }
    }

    scr6_2_cont = create_screen_container(parent);

    // Title
    lv_obj_t *label = lv_label_create(scr6_2_cont);
    apply_text_color(label);
    lv_obj_set_style_text_font(label, FONT_BOLD_18, LV_PART_MAIN);
    lv_label_set_text(label, "Deauth Hunter");
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 10);

    // Start/Pause button (under back button, left side) - using music player style
    deauth_btn_start = lv_btn_create(scr6_2_cont);
    lv_obj_set_size(deauth_btn_start, 40, 40);
    lv_obj_align(deauth_btn_start, LV_ALIGN_TOP_LEFT, 10, 40);
    apply_no_border(deauth_btn_start);
    apply_no_shadow(deauth_btn_start);
    lv_obj_set_style_radius(deauth_btn_start, 10, 0);
    lv_obj_set_style_bg_img_src(deauth_btn_start, &img_pause_32, 0);
    lv_obj_remove_style(deauth_btn_start, NULL, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_pad(deauth_btn_start, 2, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_width(deauth_btn_start, 2, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_color(deauth_btn_start, lv_color_hex(EMBED_COLOR_FOCUS_ON), LV_STATE_FOCUS_KEY);
    lv_obj_add_event_cb(deauth_btn_start, deauth_btn_start_event, LV_EVENT_CLICKED, NULL);
    lv_group_add_obj(lv_group_get_default(), deauth_btn_start);

    // Channel label (top row, left)
    deauth_channel_label = lv_label_create(scr6_2_cont);
    apply_text_color(deauth_channel_label);
    lv_obj_set_style_text_font(deauth_channel_label, FONT_BOLD_14, LV_PART_MAIN);
    lv_label_set_text(deauth_channel_label, "Channel: --");
    lv_obj_align(deauth_channel_label, LV_ALIGN_TOP_LEFT, 60, 45);

    // APs label (top row, center)
    deauth_aps_label = lv_label_create(scr6_2_cont);
    apply_text_color(deauth_aps_label);
    lv_obj_set_style_text_font(deauth_aps_label, FONT_BOLD_14, LV_PART_MAIN);
    lv_label_set_text(deauth_aps_label, "APs: 0");
    lv_obj_align(deauth_aps_label, LV_ALIGN_TOP_LEFT, 60, 65);

    // Packets label (top row, right side)
    deauth_packets_label = lv_label_create(scr6_2_cont);
    apply_text_color(deauth_packets_label);
    lv_obj_set_style_text_font(deauth_packets_label, FONT_BOLD_14, LV_PART_MAIN);
    lv_label_set_text(deauth_packets_label, "Packets: 0");
    lv_obj_align(deauth_packets_label, LV_ALIGN_TOP_RIGHT, -10, 45);

    // Row 2: "RSSI Scale:" label (left side, static text)
    lv_obj_t *rssi_scale_label_text = lv_label_create(scr6_2_cont);
    apply_text_color(rssi_scale_label_text);
    lv_obj_set_style_text_font(rssi_scale_label_text, FONT_BOLD_14, LV_PART_MAIN);
    lv_label_set_text(rssi_scale_label_text, "Scale:");
    lv_obj_align(rssi_scale_label_text, LV_ALIGN_TOP_LEFT, 10, 95);

    // RSSI Scale selector button (next to label)
    deauth_btn_rssi_scale = lv_btn_create(scr6_2_cont);
    lv_obj_set_size(deauth_btn_rssi_scale, 70, 28);
    lv_obj_align(deauth_btn_rssi_scale, LV_ALIGN_TOP_LEFT, 70, 90);
    lv_obj_set_style_border_color(deauth_btn_rssi_scale, lv_color_hex(EMBED_COLOR_BORDER), LV_PART_MAIN);
    lv_obj_set_style_border_width(deauth_btn_rssi_scale, 1, LV_PART_MAIN);
    apply_no_shadow(deauth_btn_rssi_scale);
    apply_bg_color(deauth_btn_rssi_scale);
    lv_obj_remove_style(deauth_btn_rssi_scale, NULL, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_pad(deauth_btn_rssi_scale, 2, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_width(deauth_btn_rssi_scale, 2, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_color(deauth_btn_rssi_scale, lv_color_hex(EMBED_COLOR_FOCUS_ON), LV_STATE_FOCUS_KEY);
    lv_obj_add_event_cb(deauth_btn_rssi_scale, deauth_btn_rssi_scale_event, LV_EVENT_CLICKED, NULL);
    lv_group_add_obj(lv_group_get_default(), deauth_btn_rssi_scale);

    deauth_rssi_scale_label = lv_label_create(deauth_btn_rssi_scale);
    apply_text_color(deauth_rssi_scale_label);
    lv_obj_set_style_text_font(deauth_rssi_scale_label, FONT_BOLD_14, LV_PART_MAIN);
    lv_label_set_text_fmt(deauth_rssi_scale_label, "%d dBm", deauth_rssi_scale_dbm);
    lv_obj_center(deauth_rssi_scale_label);

    // "Threshold:" label (right side, static text)
    lv_obj_t *threshold_label_text = lv_label_create(scr6_2_cont);
    apply_text_color(threshold_label_text);
    lv_obj_set_style_text_font(threshold_label_text, FONT_BOLD_14, LV_PART_MAIN);
    lv_label_set_text(threshold_label_text, "Threshold:");
    lv_obj_align(threshold_label_text, LV_ALIGN_TOP_RIGHT, -80, 95);

    // Packet Threshold selector button (right side, Fibonacci sequence)
    deauth_btn_threshold = lv_btn_create(scr6_2_cont);
    lv_obj_set_size(deauth_btn_threshold, 60, 28);
    lv_obj_align(deauth_btn_threshold, LV_ALIGN_TOP_RIGHT, -10, 90);
    lv_obj_set_style_border_color(deauth_btn_threshold, lv_color_hex(EMBED_COLOR_BORDER), LV_PART_MAIN);
    lv_obj_set_style_border_width(deauth_btn_threshold, 1, LV_PART_MAIN);
    apply_no_shadow(deauth_btn_threshold);
    apply_bg_color(deauth_btn_threshold);
    lv_obj_remove_style(deauth_btn_threshold, NULL, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_pad(deauth_btn_threshold, 2, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_width(deauth_btn_threshold, 2, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_color(deauth_btn_threshold, lv_color_hex(EMBED_COLOR_FOCUS_ON), LV_STATE_FOCUS_KEY);
    lv_obj_add_event_cb(deauth_btn_threshold, deauth_btn_threshold_event, LV_EVENT_CLICKED, NULL);
    lv_group_add_obj(lv_group_get_default(), deauth_btn_threshold);

    deauth_threshold_label = lv_label_create(deauth_btn_threshold);
    apply_text_color(deauth_threshold_label);
    lv_obj_set_style_text_font(deauth_threshold_label, FONT_BOLD_14, LV_PART_MAIN);
    lv_label_set_text_fmt(deauth_threshold_label, "%d", deauth_packet_threshold);
    lv_obj_center(deauth_threshold_label);

    // RSSI Bar (at bottom, matching SubGHz Scan/Record style)
    deauth_rssi_bar = lv_bar_create(scr6_2_cont);
    lv_obj_set_size(deauth_rssi_bar, 285, 18);
    lv_obj_align(deauth_rssi_bar, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_bar_set_range(deauth_rssi_bar, 0, 100);
    lv_bar_set_value(deauth_rssi_bar, 0, LV_ANIM_OFF);

    // RSSI Label (overlaid on top of bar, centered)
    deauth_rssi_label = lv_label_create(scr6_2_cont);
    apply_text_color(deauth_rssi_label);
    lv_obj_set_style_text_font(deauth_rssi_label, FONT_BOLD_14, LV_PART_MAIN);
    lv_label_set_text(deauth_rssi_label, "RSSI: --- dBm");
    lv_obj_align(deauth_rssi_label, LV_ALIGN_BOTTOM_MID, 0, -12);
    // Add semi-transparent background for better readability
    lv_obj_set_style_bg_opa(deauth_rssi_label, LV_OPA_70, LV_PART_MAIN);
    apply_bg_color(deauth_rssi_label);
    lv_obj_set_style_pad_left(deauth_rssi_label, 3, LV_PART_MAIN);
    lv_obj_set_style_pad_right(deauth_rssi_label, 3, LV_PART_MAIN);

    // Back button (using standard "<" button in top left)
    scr_back_btn_create(scr6_2_cont, deauth_btn_back_event);

    // Create update timer (100ms for smooth updates)
    deauth_update_timer = lv_timer_create(deauth_update_timer_event, 100, NULL);
    lv_timer_pause(deauth_update_timer);
}

static void enter6_2(void) {
    // Reset state
    deauth_hunter_active = false;
    deauth_channel_hopping_active = true;
    deauth_current_channel_idx = 0;
    deauth_stats.total_deauths = 0;
    deauth_stats.unique_aps = 0;
    deauth_stats.last_rssi = -100;
    deauth_seen_ap_macs.clear();
    deauth_stats_reset_time = millis();
    deauth_last_channel_change = millis();

    // Update button to show play icon
    lv_obj_set_style_bg_img_src(deauth_btn_start, &img_pause_32, 0);

    // Resume timer
    lv_timer_resume(deauth_update_timer);

    lv_group_set_wrap(lv_group_get_default(), true);

}

static void exit6_2(void) {
    lv_group_set_wrap(lv_group_get_default(), false);

    // Stop monitoring if active
    if (deauth_hunter_active) {
        deauth_stop_monitoring();
    }

    // Pause timer
    if (deauth_update_timer) {
        lv_timer_pause(deauth_update_timer);
    }

}

static void destroy6_2(void) {
    // Delete timer
    if (deauth_update_timer) {
        lv_timer_del(deauth_update_timer);
        deauth_update_timer = NULL;
    }

    // Stop monitoring
    if (deauth_hunter_active) {
        deauth_stop_monitoring();
    }

    // Clear seen APs
    deauth_seen_ap_macs.clear();

    if (scr6_2_cont) {
        lv_obj_del(scr6_2_cont);
        scr6_2_cont = NULL;
    }
}

scr_lifecycle_t screen6_2 = {
    .create = create6_2,
    .entry = enter6_2,
    .exit = exit6_2,
    .destroy = destroy6_2,
};

//===============================================================
// PINEAP HUNTER (Screen 6.3)
//===============================================================

// PineAP Hunter data structures
struct SSIDRecord {
    String essid;
    int32_t rssi;
    uint32_t last_seen;

    SSIDRecord(const String& ssid, int32_t signal) : essid(ssid), rssi(signal), last_seen(millis()) {}
};

struct PineRecord {
    uint8_t bssid[6];
    std::vector<SSIDRecord> essids;
    int32_t rssi;  // Most recent RSSI
    uint32_t last_seen;

    PineRecord() : rssi(-100), last_seen(0) {
        memset(bssid, 0, 6);
    }
};

struct PineAPHunterStats {
    std::vector<PineRecord> detected_pineaps;
    std::map<String, std::vector<SSIDRecord>> scan_buffer;
    uint32_t total_scans = 0;
    uint32_t last_scan_time = 0;
    bool list_changed = false;
    int view_mode = 0;  // 0=main list, 1=SSID detail list
};

// PineAP Hunter UI elements
lv_obj_t *scr6_3_cont;
lv_obj_t *pineap_scan_led;  // LED indicator for scanning activity
lv_obj_t *pineap_rssi_bar;  // RSSI bar indicator (shown in detail view)
lv_obj_t *pineap_rssi_label;  // RSSI dBm label overlay
lv_obj_t *pineap_threshold_label;
lv_obj_t *pineap_rssi_scale_label;
lv_obj_t *pineap_btn_start;
lv_obj_t *pineap_btn_threshold;
lv_obj_t *pineap_btn_rssi_scale;
lv_obj_t *pineap_list_cont = NULL;  // Container for scrollable list
lv_obj_t *pineap_detail_header = NULL;  // Header for detail view
lv_timer_t *pineap_update_timer = NULL;

// PineAP Hunter state
PineAPHunterStats pineap_stats;
bool pineap_hunter_active = false;
int pineap_rssi_scale_dbm = -20;  // Default RSSI scale
int pineap_ssid_threshold = 5;  // Default threshold for detection
int pineap_selected_index = 0;  // Selected item in list

// Helper functions
String bssid_to_string(const uint8_t* bssid) {
    char str[18];
    snprintf(str, sizeof(str), "%02x:%02x:%02x:%02x:%02x:%02x",
             bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5]);
    return String(str);
}

void string_to_bssid(const String& bssid_str, uint8_t* bssid) {
    sscanf(bssid_str.c_str(), "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
           &bssid[0], &bssid[1], &bssid[2], &bssid[3], &bssid[4], &bssid[5]);
}

void add_scan_result(const String& bssid_str, const String& essid, int32_t rssi) {
    if (pineap_stats.scan_buffer.find(bssid_str) == pineap_stats.scan_buffer.end()) {
        pineap_stats.scan_buffer[bssid_str] = std::vector<SSIDRecord>();
    }

    auto& essid_list = pineap_stats.scan_buffer[bssid_str];
    bool found = false;
    for (auto& existing_record : essid_list) {
        if (existing_record.essid == essid) {
            existing_record.rssi = rssi;
            existing_record.last_seen = millis();
            found = true;
            break;
        }
    }

    if (!found) {
        essid_list.push_back(SSIDRecord(essid, rssi));
    }
}

void maintain_buffer_size() {
    if (pineap_stats.scan_buffer.size() > 50) {
        auto it = pineap_stats.scan_buffer.begin();
        std::advance(it, pineap_stats.scan_buffer.size() - 50);
        pineap_stats.scan_buffer.erase(pineap_stats.scan_buffer.begin(), it);
    }
}

void process_scan_results() {
    std::vector<PineRecord> new_pineaps;

    for (const auto& entry : pineap_stats.scan_buffer) {
        const String& bssid_str = entry.first;
        const std::vector<SSIDRecord>& ssid_records = entry.second;

        if (ssid_records.size() >= (size_t)pineap_ssid_threshold) {
            PineRecord pine;
            string_to_bssid(bssid_str, pine.bssid);
            pine.essids = ssid_records;
            pine.last_seen = millis();

            // Get most recent RSSI
            int32_t latest_rssi = -100;
            uint32_t latest_time = 0;
            for (const auto& rec : ssid_records) {
                if (rec.last_seen > latest_time) {
                    latest_time = rec.last_seen;
                    latest_rssi = rec.rssi;
                }
            }
            pine.rssi = latest_rssi;

            // Sort SSIDs by most recent first
            std::sort(pine.essids.begin(), pine.essids.end(),
                [](const SSIDRecord& a, const SSIDRecord& b) {
                    return a.last_seen > b.last_seen;
                });

            new_pineaps.push_back(pine);
        }
    }

    if (new_pineaps.size() != pineap_stats.detected_pineaps.size()) {
        pineap_stats.list_changed = true;
    }

    pineap_stats.detected_pineaps = new_pineaps;
}

void scan_and_analyze_pineap() {
    static uint32_t last_wifi_scan = 0;

    if (millis() - last_wifi_scan > 2000) {  // Scan every 2 seconds
        // Turn LED on during scan
        lv_led_on(pineap_scan_led);
        lv_refr_now(NULL);
        tft.drawPixel(0, 0, 0);

        int n = WiFi.scanNetworks();
        if (n > 0) {
            for (int i = 0; i < n; i++) {
                String bssid_str = WiFi.BSSIDstr(i);
                String essid = WiFi.SSID(i);
                int32_t rssi = WiFi.RSSI(i);

                if (essid.length() > 0 && bssid_str.length() > 0) {
                    add_scan_result(bssid_str, essid, rssi);
                }
            }

            pineap_stats.total_scans++;
            process_scan_results();
            maintain_buffer_size();
        }

        WiFi.scanDelete();
        last_wifi_scan = millis();

        // Turn LED off after scan
        lv_led_off(pineap_scan_led);
        lv_refr_now(NULL);
        tft.drawPixel(0, 0, 0);
    }
}

// Calculate RSSI bar value (0-100) based on scale
int pineap_calculate_rssi_bars(int rssi, int scale_max) {
    if (rssi >= scale_max) return 100;
    if (rssi <= -100) return 0;

    int range = scale_max - (-100);
    int offset = rssi - (-100);
    return (100 * offset) / range;
}

// Update display for main list view
void pineap_draw_main_list() {
    if (!pineap_list_cont) return;

    // Restore main header
    if (pineap_detail_header) {
        lv_label_set_text(pineap_detail_header, "PineAP Hunter");
    }

    // Show threshold button, hide RSSI bar and label in main view
    if (pineap_btn_threshold) lv_obj_clear_flag(pineap_btn_threshold, LV_OBJ_FLAG_HIDDEN);
    if (pineap_rssi_bar) lv_obj_add_flag(pineap_rssi_bar, LV_OBJ_FLAG_HIDDEN);
    if (pineap_rssi_label) lv_obj_add_flag(pineap_rssi_label, LV_OBJ_FLAG_HIDDEN);

    // Clear existing list
    lv_obj_clean(pineap_list_cont);

    if (pineap_stats.detected_pineaps.empty()) {
        lv_obj_t *empty_label = lv_label_create(pineap_list_cont);
        apply_text_color(empty_label);
        lv_obj_set_style_text_font(empty_label, FONT_LIGHT_14, LV_PART_MAIN);
        lv_label_set_text(empty_label, "No PineAPs detected");
        lv_obj_align(empty_label, LV_ALIGN_TOP_LEFT, 5, 5);
    } else {
        // Create list items for detected PineAPs - show full BSSID and SSID count
        lv_obj_t *first_btn = NULL;
        for (size_t i = 0; i < pineap_stats.detected_pineaps.size(); i++) {
            const auto& pine = pineap_stats.detected_pineaps[i];
            String bssid_str = bssid_to_string(pine.bssid);

            lv_obj_t *item_btn = lv_btn_create(pineap_list_cont);
            lv_obj_set_size(item_btn, 295, 30);
            apply_bg_color(item_btn);
            lv_obj_set_style_border_color(item_btn, lv_color_hex(EMBED_COLOR_BORDER), LV_PART_MAIN);
            lv_obj_set_style_border_width(item_btn, 1, LV_PART_MAIN);
            apply_no_shadow(item_btn);
            lv_obj_remove_style(item_btn, NULL, LV_STATE_FOCUS_KEY);
            lv_obj_set_style_outline_pad(item_btn, 2, LV_STATE_FOCUS_KEY);
            lv_obj_set_style_outline_width(item_btn, 2, LV_STATE_FOCUS_KEY);
            lv_obj_set_style_outline_color(item_btn, lv_color_hex(EMBED_COLOR_FOCUS_ON), LV_STATE_FOCUS_KEY);

            lv_obj_t *item_label = lv_label_create(item_btn);
            apply_text_color(item_label);
            lv_obj_set_style_text_font(item_label, FONT_LIGHT_14, LV_PART_MAIN);

            // Show full BSSID and SSID count: "00:aa:33:44:ee:44 17"
            lv_label_set_text_fmt(item_label, "%s %d", bssid_str.c_str(), pine.essids.size());
            lv_obj_center(item_label);

            // Store index as user data
            lv_obj_set_user_data(item_btn, (void*)(uintptr_t)i);

            // Click event to show SSID list
            lv_obj_add_event_cb(item_btn, [](lv_event_t *e) {
                if (e->code == LV_EVENT_CLICKED) {
                    lv_obj_t *btn = lv_event_get_target(e);
                    pineap_selected_index = (int)(uintptr_t)lv_obj_get_user_data(btn);
                    pineap_stats.view_mode = 1;  // Switch to SSID detail view
                }
            }, LV_EVENT_CLICKED, NULL);

            lv_group_add_obj(lv_group_get_default(), item_btn);

            // Save first button for focus
            if (i == 0) {
                first_btn = item_btn;
            }
        }

        // Focus on first item in the list when PineAPs are detected
        if (first_btn) {
            lv_group_focus_obj(first_btn);
        }

        // Trigger beep when PineAP detected on main screen
        if (pineap_hunter_active) {
            static unsigned long last_beep_time = 0;
            static const unsigned long BEEP_INTERVAL = 1000;  // Beep every 1 second
            unsigned long now = millis();

            if (now - last_beep_time >= BEEP_INTERVAL) {
                // Simple beep using LEDC PWM - 1000Hz tone for 100ms
                ledcSetup(0, 1000, 8);    // channel, freq, resolution
                ledcAttachPin(7, 0);       // pin 7 (BOARD_VOICE_DIN), channel 0
                ledcWrite(0, 128);         // 50% duty cycle
                delay(100);
                ledcWrite(0, 0);           // Stop tone
                ledcDetachPin(7);          // Detach pin

                last_beep_time = now;
            }
        }
    }
}

// Update display for SSID detail view
void pineap_draw_ssid_list() {
    if (!pineap_list_cont) return;
    if (!pineap_detail_header) return;
    if (pineap_selected_index >= (int)pineap_stats.detected_pineaps.size()) {
        pineap_stats.view_mode = 0;
        return;
    }

    const auto& pine = pineap_stats.detected_pineaps[pineap_selected_index];
    String bssid_str = bssid_to_string(pine.bssid);

    // Update BSSID header
    lv_label_set_text(pineap_detail_header, bssid_str.c_str());

    // Hide threshold button, show RSSI bar and label in detail view
    if (pineap_btn_threshold) lv_obj_add_flag(pineap_btn_threshold, LV_OBJ_FLAG_HIDDEN);
    if (pineap_rssi_bar) {
        lv_obj_clear_flag(pineap_rssi_bar, LV_OBJ_FLAG_HIDDEN);
        // Update RSSI bar with current signal strength
        int rssi_percent = pineap_calculate_rssi_bars(pine.rssi, pineap_rssi_scale_dbm);
        lv_bar_set_value(pineap_rssi_bar, rssi_percent, LV_ANIM_OFF);
    }
    if (pineap_rssi_label) {
        lv_obj_clear_flag(pineap_rssi_label, LV_OBJ_FLAG_HIDDEN);
        lv_label_set_text_fmt(pineap_rssi_label, "RSSI: %d dBm", pine.rssi);
    }

    // Clear existing list
    lv_obj_clean(pineap_list_cont);

    // List all SSIDs with RSSI - format: "[dBm] SSID_Name"
    for (const auto& ssid_rec : pine.essids) {
        lv_obj_t *ssid_label = lv_label_create(pineap_list_cont);
        apply_text_color(ssid_label);
        lv_obj_set_style_text_font(ssid_label, FONT_LIGHT_14, LV_PART_MAIN);
        lv_label_set_text_fmt(ssid_label, "[%d] %s", ssid_rec.rssi, ssid_rec.essid.c_str());
        lv_obj_set_width(ssid_label, lv_pct(100));
    }
}

// Update timer callback
static void pineap_update_timer_event(lv_timer_t *timer) {
    if (!pineap_hunter_active) return;

    // Continuous scanning
    scan_and_analyze_pineap();

    // Update display
    static uint32_t last_display_update = 0;
    if (pineap_stats.list_changed || (millis() - last_display_update > 1000)) {
        if (pineap_stats.view_mode == 0) {
            pineap_draw_main_list();
        } else {
            pineap_draw_ssid_list();
        }
        last_display_update = millis();
        pineap_stats.list_changed = false;
    }
}

// Start/Stop button event
static void pineap_btn_start_event(lv_event_t *e) {
    if (e->code == LV_EVENT_CLICKED) {
        pineap_hunter_active = !pineap_hunter_active;

        if (pineap_hunter_active) {
            // Started - show pause icon
            lv_obj_set_style_bg_img_src(pineap_btn_start, &img_play_32, 0);
            WiFi.mode(WIFI_STA);
            WiFi.disconnect();
            lv_timer_resume(pineap_update_timer);
        } else {
            // Stopped - show play icon
            lv_obj_set_style_bg_img_src(pineap_btn_start, &img_pause_32, 0);
            lv_timer_pause(pineap_update_timer);
            // Turn LED off when stopped
            lv_led_off(pineap_scan_led);
            lv_refr_now(NULL);
            tft.drawPixel(0, 0, 0);
        }
    }
}

// Threshold adjustment button event
static void pineap_btn_threshold_event(lv_event_t *e) {
    if (e->code == LV_EVENT_CLICKED) {
        // Cycle through thresholds: 2, 3, 5, 8, 13
        const int thresholds[] = {2, 3, 5, 8, 13};
        const int num_thresholds = sizeof(thresholds) / sizeof(thresholds[0]);

        int current_idx = 0;
        for (int i = 0; i < num_thresholds; i++) {
            if (pineap_ssid_threshold == thresholds[i]) {
                current_idx = i;
                break;
            }
        }

        current_idx = (current_idx + 1) % num_thresholds;
        pineap_ssid_threshold = thresholds[current_idx];

        lv_label_set_text_fmt(lv_obj_get_child(pineap_btn_threshold, 0), "%d", pineap_ssid_threshold);
    }
}

// RSSI Scale adjustment button event
static void pineap_btn_rssi_scale_event(lv_event_t *e) {
    if (e->code == LV_EVENT_CLICKED) {
        pineap_rssi_scale_dbm += 5;
        if (pineap_rssi_scale_dbm > -10) {
            pineap_rssi_scale_dbm = -90;
        }
        lv_label_set_text_fmt(pineap_rssi_scale_label, "%d dBm", pineap_rssi_scale_dbm);

        // Save to config
        g_config.wifi.pineap.rssi_scale_dbm = pineap_rssi_scale_dbm;
        config_save();
    }
}

// Back button event
static void pineap_btn_back_event(lv_event_t *e) {
    if (e->code == LV_EVENT_CLICKED) {
        if (pineap_stats.view_mode == 1) {
            // If in detail view, go back to main list
            pineap_stats.view_mode = 0;
            pineap_stats.list_changed = true;  // Force update
        } else {
            // If in main view, go back to WiFi menu
            scr_mgr_switch(SCREEN6_ID, false);
        }
    }
}

static void create6_3(lv_obj_t *parent) {
    // Load settings from config
    pineap_rssi_scale_dbm = g_config.wifi.pineap.rssi_scale_dbm;

    scr6_3_cont = create_screen_container(parent);

    // Header (also used for detail view BSSID)
    pineap_detail_header = lv_label_create(scr6_3_cont);
    apply_text_color(pineap_detail_header);
    lv_obj_set_style_text_font(pineap_detail_header, FONT_BOLD_18, LV_PART_MAIN);
    lv_label_set_text(pineap_detail_header, "PineAP Hunter");
    lv_obj_align(pineap_detail_header, LV_ALIGN_TOP_MID, 0, 5);

    // Back button (using standard "<" button in top left)
    scr_back_btn_create(scr6_3_cont, pineap_btn_back_event);

    // Start/Pause button (below back button, left side)
    pineap_btn_start = lv_btn_create(scr6_3_cont);
    lv_obj_set_size(pineap_btn_start, 50, 50);
    lv_obj_align(pineap_btn_start, LV_ALIGN_TOP_LEFT, 8, 35);
    apply_bg_color(pineap_btn_start);
    lv_obj_set_style_border_color(pineap_btn_start, lv_color_hex(EMBED_COLOR_BORDER), LV_PART_MAIN);
    lv_obj_set_style_border_width(pineap_btn_start, 1, LV_PART_MAIN);
    apply_no_shadow(pineap_btn_start);
    lv_obj_set_style_bg_img_src(pineap_btn_start, &img_pause_32, 0);
    lv_obj_remove_style(pineap_btn_start, NULL, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_pad(pineap_btn_start, 2, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_width(pineap_btn_start, 2, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_color(pineap_btn_start, lv_color_hex(EMBED_COLOR_FOCUS_ON), LV_STATE_FOCUS_KEY);
    lv_obj_add_event_cb(pineap_btn_start, pineap_btn_start_event, LV_EVENT_CLICKED, NULL);
    lv_group_add_obj(lv_group_get_default(), pineap_btn_start);

    // Scan LED indicator (to the right of start button)
    pineap_scan_led = lv_led_create(scr6_3_cont);
    lv_obj_set_size(pineap_scan_led, 20, 20);
    lv_obj_align(pineap_scan_led, LV_ALIGN_TOP_LEFT, 70, 50);
    lv_led_set_color(pineap_scan_led, lv_palette_main(LV_PALETTE_GREEN));
    lv_led_off(pineap_scan_led);

    // Threshold label and button (right side, upper)
    lv_obj_t *thresh_label_text = lv_label_create(scr6_3_cont);
    apply_text_color(thresh_label_text);
    lv_obj_set_style_text_font(thresh_label_text, FONT_BOLD_14, LV_PART_MAIN);
    lv_label_set_text(thresh_label_text, "Threshold:");
    lv_obj_align(thresh_label_text, LV_ALIGN_TOP_RIGHT, -80, 38);

    pineap_btn_threshold = lv_btn_create(scr6_3_cont);
    lv_obj_set_size(pineap_btn_threshold, 50, 28);
    lv_obj_align(pineap_btn_threshold, LV_ALIGN_TOP_RIGHT, -15, 33);
    lv_obj_set_style_border_color(pineap_btn_threshold, lv_color_hex(EMBED_COLOR_BORDER), LV_PART_MAIN);
    lv_obj_set_style_border_width(pineap_btn_threshold, 1, LV_PART_MAIN);
    apply_no_shadow(pineap_btn_threshold);
    apply_bg_color(pineap_btn_threshold);
    lv_obj_remove_style(pineap_btn_threshold, NULL, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_pad(pineap_btn_threshold, 2, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_width(pineap_btn_threshold, 2, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_color(pineap_btn_threshold, lv_color_hex(EMBED_COLOR_FOCUS_ON), LV_STATE_FOCUS_KEY);
    lv_obj_add_event_cb(pineap_btn_threshold, pineap_btn_threshold_event, LV_EVENT_CLICKED, NULL);
    lv_group_add_obj(lv_group_get_default(), pineap_btn_threshold);

    pineap_threshold_label = lv_label_create(pineap_btn_threshold);
    apply_text_color(pineap_threshold_label);
    lv_obj_set_style_text_font(pineap_threshold_label, FONT_BOLD_14, LV_PART_MAIN);
    lv_label_set_text_fmt(pineap_threshold_label, "%d", pineap_ssid_threshold);
    lv_obj_center(pineap_threshold_label);

    // RSSI bar indicator (shown in detail view, hidden in main view)
    pineap_rssi_bar = lv_bar_create(scr6_3_cont);
    lv_obj_set_size(pineap_rssi_bar, 200, 15);
    lv_obj_align(pineap_rssi_bar, LV_ALIGN_TOP_LEFT, 100, 42);
    lv_bar_set_range(pineap_rssi_bar, 0, 100);
    lv_bar_set_value(pineap_rssi_bar, 0, LV_ANIM_OFF);
    apply_bg_color(pineap_rssi_bar);
    lv_obj_set_style_bg_color(pineap_rssi_bar, lv_color_hex(0x00FF00), LV_PART_INDICATOR);
    lv_obj_set_style_border_color(pineap_rssi_bar, lv_color_hex(EMBED_COLOR_BORDER), LV_PART_MAIN);
    lv_obj_set_style_border_width(pineap_rssi_bar, 1, LV_PART_MAIN);
    lv_obj_add_flag(pineap_rssi_bar, LV_OBJ_FLAG_HIDDEN);  // Hidden initially

    // RSSI dBm label overlay (centered on the RSSI bar)
    pineap_rssi_label = lv_label_create(scr6_3_cont);
    apply_text_color(pineap_rssi_label);
    lv_obj_set_style_text_font(pineap_rssi_label, FONT_BOLD_14, LV_PART_MAIN);
    lv_label_set_text(pineap_rssi_label, "RSSI: --- dBm");
    lv_obj_align(pineap_rssi_label, LV_ALIGN_TOP_LEFT, 150, 40);
    // Add semi-transparent background for better readability
    lv_obj_set_style_bg_opa(pineap_rssi_label, LV_OPA_70, LV_PART_MAIN);
    apply_bg_color(pineap_rssi_label);
    lv_obj_set_style_pad_left(pineap_rssi_label, 3, LV_PART_MAIN);
    lv_obj_set_style_pad_right(pineap_rssi_label, 3, LV_PART_MAIN);
    lv_obj_add_flag(pineap_rssi_label, LV_OBJ_FLAG_HIDDEN);  // Hidden initially

    // Scale label and button (right side, lower - 4px down from threshold)
    lv_obj_t *scale_label_text = lv_label_create(scr6_3_cont);
    apply_text_color(scale_label_text);
    lv_obj_set_style_text_font(scale_label_text, FONT_BOLD_14, LV_PART_MAIN);
    lv_label_set_text(scale_label_text, "Scale:");
    lv_obj_align(scale_label_text, LV_ALIGN_TOP_RIGHT, -95, 70);

    pineap_btn_rssi_scale = lv_btn_create(scr6_3_cont);
    lv_obj_set_size(pineap_btn_rssi_scale, 70, 28);
    lv_obj_align(pineap_btn_rssi_scale, LV_ALIGN_TOP_RIGHT, -15, 65);
    lv_obj_set_style_border_color(pineap_btn_rssi_scale, lv_color_hex(EMBED_COLOR_BORDER), LV_PART_MAIN);
    lv_obj_set_style_border_width(pineap_btn_rssi_scale, 1, LV_PART_MAIN);
    apply_no_shadow(pineap_btn_rssi_scale);
    apply_bg_color(pineap_btn_rssi_scale);
    lv_obj_remove_style(pineap_btn_rssi_scale, NULL, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_pad(pineap_btn_rssi_scale, 2, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_width(pineap_btn_rssi_scale, 2, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_color(pineap_btn_rssi_scale, lv_color_hex(EMBED_COLOR_FOCUS_ON), LV_STATE_FOCUS_KEY);
    lv_obj_add_event_cb(pineap_btn_rssi_scale, pineap_btn_rssi_scale_event, LV_EVENT_CLICKED, NULL);
    lv_group_add_obj(lv_group_get_default(), pineap_btn_rssi_scale);

    pineap_rssi_scale_label = lv_label_create(pineap_btn_rssi_scale);
    apply_text_color(pineap_rssi_scale_label);
    lv_obj_set_style_text_font(pineap_rssi_scale_label, FONT_BOLD_14, LV_PART_MAIN);
    lv_label_set_text_fmt(pineap_rssi_scale_label, "%d dBm", pineap_rssi_scale_dbm);
    lv_obj_center(pineap_rssi_scale_label);

    // Scrollable list container (full width, extends to bottom)
    pineap_list_cont = lv_obj_create(scr6_3_cont);
    lv_obj_set_size(pineap_list_cont, 305, 145);  // Extended height to reach bottom
    lv_obj_align(pineap_list_cont, LV_ALIGN_TOP_MID, 0, 95);
    apply_bg_color(pineap_list_cont);
    lv_obj_set_style_border_color(pineap_list_cont, lv_color_hex(EMBED_COLOR_BORDER), LV_PART_MAIN);
    lv_obj_set_style_border_width(pineap_list_cont, 1, LV_PART_MAIN);
    lv_obj_set_flex_flow(pineap_list_cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(pineap_list_cont, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_all(pineap_list_cont, 2, LV_PART_MAIN);
    lv_obj_set_style_pad_row(pineap_list_cont, 3, LV_PART_MAIN);

    // Create update timer
    pineap_update_timer = lv_timer_create(pineap_update_timer_event, 100, NULL);
    lv_timer_pause(pineap_update_timer);
}

static void enter6_3(void) {
    // Reset state
    pineap_hunter_active = false;
    pineap_stats = PineAPHunterStats();
    pineap_stats.view_mode = 0;
    pineap_selected_index = 0;

    // Update button to show play icon
    lv_obj_set_style_bg_img_src(pineap_btn_start, &img_pause_32, 0);

    // Reset header
    if (pineap_detail_header) {
        lv_label_set_text(pineap_detail_header, "PineAP Hunter");
    }

    // Clear list
    if (pineap_list_cont) {
        lv_obj_clean(pineap_list_cont);
    }

    lv_group_set_wrap(lv_group_get_default(), true);
    lv_group_focus_obj(pineap_btn_start);

}

static void exit6_3(void) {
    lv_group_set_wrap(lv_group_get_default(), false);

    // Stop monitoring if active
    if (pineap_hunter_active) {
        pineap_hunter_active = false;
        WiFi.scanDelete();
    }

    // Pause timer
    if (pineap_update_timer) {
        lv_timer_pause(pineap_update_timer);
    }

}

static void destroy6_3(void) {
    // Delete timer
    if (pineap_update_timer) {
        lv_timer_del(pineap_update_timer);
        pineap_update_timer = NULL;
    }

    // Stop monitoring
    if (pineap_hunter_active) {
        pineap_hunter_active = false;
        WiFi.scanDelete();
    }

    // Clear data
    pineap_stats.detected_pineaps.clear();
    pineap_stats.scan_buffer.clear();

    if (scr6_3_cont) {
        lv_obj_del(scr6_3_cont);
        scr6_3_cont = NULL;
    }
}

scr_lifecycle_t screen6_3 = {
    .create = create6_3,
    .entry = enter6_3,
    .exit = exit6_3,
    .destroy = destroy6_3,
};

#endif

// --------------------- screen 7 --------------------- Infrared Menu
#if 1
lv_obj_t *scr7_cont;
lv_obj_t *ir_item_cont;
lv_obj_t *ir_menu_cont;
lv_obj_t *ir_menu_icon;
lv_obj_t *ir_menu_icon_lab;
lv_obj_t *ir_first_btn = NULL;  // Store first button to focus it

int ir_focus_item = 0;

const char *ir_menu_items[] = {
    "<- TV-B-Gone",
    "<- IR Remotes",
    "<- Back"
};

extern uint64_t IR_recv_value;
extern IRsend irsend;
extern IRrecv irrecv;
extern decode_results results;

void entry7_anim(lv_obj_t *obj);
void exit7_anim(int user_data, lv_obj_t *obj);

static void ir_scroll_event_cb(lv_event_t * e)
{
    lv_obj_t * cont = lv_event_get_target(e);

    lv_area_t cont_a;
    lv_obj_get_coords(cont, &cont_a);
    lv_coord_t cont_y_center = cont_a.y1 + lv_area_get_height(&cont_a) / 2;

    lv_coord_t r = lv_obj_get_height(cont) * 7 / 10;
    uint32_t i;
    uint32_t child_cnt = lv_obj_get_child_cnt(cont);
    for(i = 0; i < child_cnt; i++) {
        lv_obj_t * child = lv_obj_get_child(cont, i);
        lv_area_t child_a;
        lv_obj_get_coords(child, &child_a);

        lv_coord_t child_y_center = child_a.y1 + lv_area_get_height(&child_a) / 2;

        lv_coord_t diff_y = child_y_center - cont_y_center;
        diff_y = LV_ABS(diff_y);

        /*Get the x of diff_y on a circle.*/
        lv_coord_t x;
        /*If diff_y is out of the circle use the last point of the circle (the radius)*/
        if(diff_y >= r) {
            x = r;
        }
        else {
            x =  (diff_y * 0.866);
        }

        /*Translate the item by the calculated X coordinate*/
        lv_obj_set_style_translate_x(child, x, 0);

        /*Use some opacity with larger translations*/
        lv_opa_t opa = lv_map(x, 0, r, LV_OPA_TRANSP, LV_OPA_COVER);
        lv_obj_set_style_opa(child, LV_OPA_COVER - opa, 0);
    }
}

static void ir_btn_event_cb(lv_event_t * e)
{
    int data = (int)(intptr_t)e->user_data;

    if(e->code == LV_EVENT_CLICKED){
        ir_focus_item = data;

        // Navigate to specific IR screens or back
        switch(ir_focus_item) {
            case 0: // TV-B-Gone
                scr_mgr_switch(SCREEN7_1_ID, false);
                break;
            case 1: // IR Capture
                scr_mgr_switch(SCREEN7_3_ID, false);
                break;
            case 2: // Back
                exit7_anim(SCREEN0_ID, scr7_cont);
                break;
            default:
                break;
        }
    }

    if(e->code == LV_EVENT_FOCUSED){
        // Update icon display based on focused item
        switch(data) {
            case 0: // TV-B-Gone
                lv_img_set_src(ir_menu_icon, &img_dev_32);
                lv_obj_set_style_img_recolor(ir_menu_icon, lv_palette_main(LV_PALETTE_RED), LV_PART_MAIN);
                lv_obj_set_style_img_recolor_opa(ir_menu_icon, LV_OPA_100, LV_PART_MAIN);
                break;
            case 1: // IR Capture
                lv_img_set_src(ir_menu_icon, &img_dev_32);
                lv_obj_set_style_img_recolor(ir_menu_icon, lv_palette_main(LV_PALETTE_BLUE), LV_PART_MAIN);
                lv_obj_set_style_img_recolor_opa(ir_menu_icon, LV_OPA_100, LV_PART_MAIN);
                break;
            case 2: // Back
                lv_img_set_src(ir_menu_icon, &img_dev_32);
                lv_obj_set_style_img_recolor_opa(ir_menu_icon, LV_OPA_0, LV_PART_MAIN);
                break;
            default:
                break;
        }
        lv_label_set_text(ir_menu_icon_lab, ((char*)ir_menu_items[data]));
        lv_obj_align_to(ir_menu_icon_lab, ir_menu_cont, LV_ALIGN_BOTTOM_MID, 0, -25);
    }
}

void entry7_anim(lv_obj_t *obj)
{
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, ir_item_cont);
    lv_anim_set_values(&a, DISPALY_WIDTH * MENU_PROPORTION, 0);
    lv_anim_set_time(&a, 400);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_in_out);
    lv_anim_set_exec_cb(&a, [](void * var, int32_t v){
        lv_obj_set_x((lv_obj_t *)var, v);
        lv_port_indev_enabled(false);
    });
    lv_anim_set_ready_cb(&a, [](struct _lv_anim_t *a){
        lv_port_indev_enabled(true);
    });
    lv_anim_start(&a);

    lv_anim_set_time(&a, 400);
    lv_anim_set_var(&a, ir_menu_cont);
    lv_anim_set_values(&a, -DISPALY_WIDTH * MENU_LAB_PROPORTION, 0);
    lv_anim_set_exec_cb(&a, [](void * var, int32_t v){
        lv_obj_set_x((lv_obj_t *)var, v);
        lv_port_indev_enabled(false);
    });
    lv_anim_start(&a);
}

void exit7_anim(int user_data, lv_obj_t *obj)
{
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, ir_item_cont);
    lv_anim_set_values(&a, 0, DISPALY_WIDTH * MENU_PROPORTION);
    lv_anim_set_time(&a, 400);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_in_out);
    lv_anim_set_exec_cb(&a, [](void * var, int32_t v){
        lv_obj_set_x((lv_obj_t *)var, v);
        lv_port_indev_enabled(false);
    });
    lv_anim_set_ready_cb(&a, [](struct _lv_anim_t *a){
        scr_mgr_switch((int)a->user_data, false);
        lv_port_indev_enabled(true);
    });
    lv_anim_set_user_data(&a, (void *)user_data);
    lv_anim_start(&a);

    lv_anim_set_time(&a, 400);
    lv_anim_set_var(&a, ir_menu_cont);
    lv_anim_set_values(&a, 0, -DISPALY_WIDTH * MENU_LAB_PROPORTION);
    lv_anim_set_exec_cb(&a, [](void * var, int32_t v){
        lv_obj_set_x((lv_obj_t *)var, v);
        lv_port_indev_enabled(false);
    });
    lv_anim_start(&a);
}

void create7(lv_obj_t *parent){
    scr7_cont = create_screen_container(parent);

    // Left side - Icon display panel (matches main menu layout)
    ir_menu_cont = lv_obj_create(scr7_cont);
    lv_obj_set_size(ir_menu_cont, DISPALY_WIDTH * MENU_LAB_PROPORTION, DISPALY_HEIGHT);
    apply_no_scrollbar(ir_menu_cont);
    lv_obj_set_align(ir_menu_cont, LV_ALIGN_LEFT_MID);
    apply_bg_color(ir_menu_cont);
    apply_no_padding(ir_menu_cont);
    apply_no_border(ir_menu_cont);
    apply_no_radius(ir_menu_cont);

    ir_menu_icon = lv_img_create(ir_menu_cont);
    lv_obj_align(ir_menu_icon, LV_ALIGN_CENTER, 0, 0);

    ir_menu_icon_lab = lv_label_create(ir_menu_cont);
    lv_obj_set_width(ir_menu_icon_lab, DISPALY_WIDTH * MENU_LAB_PROPORTION);
    lv_obj_set_style_text_align(ir_menu_icon_lab, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    apply_text_color(ir_menu_icon_lab);
    lv_obj_set_style_text_font(ir_menu_icon_lab, FONT_BOLD_18, LV_PART_MAIN);
    lv_obj_align_to(ir_menu_icon_lab, ir_menu_cont, LV_ALIGN_BOTTOM_MID, 0, -25);

    // Right side - Scrollable menu items (matches main menu layout)
    ir_item_cont = lv_obj_create(scr7_cont);
    lv_obj_set_size(ir_item_cont, DISPALY_WIDTH * MENU_PROPORTION, DISPALY_HEIGHT);
    lv_obj_set_align(ir_item_cont, LV_ALIGN_RIGHT_MID);
    lv_obj_set_flex_flow(ir_item_cont, LV_FLEX_FLOW_COLUMN);
    apply_bg_color(ir_item_cont);
    apply_no_border(ir_item_cont);
    lv_obj_add_event_cb(ir_item_cont, ir_scroll_event_cb, LV_EVENT_SCROLL, NULL);
    apply_no_radius(ir_item_cont);
    lv_obj_set_style_clip_corner(ir_item_cont, true, 0);
    lv_obj_set_scroll_dir(ir_item_cont, LV_DIR_VER);
    lv_obj_set_scroll_snap_y(ir_item_cont, LV_SCROLL_SNAP_CENTER);
    apply_no_scrollbar(ir_item_cont);
    lv_obj_set_style_pad_row(ir_item_cont, 10, LV_PART_MAIN);

    // Create 3 menu item buttons (2 features + Back)
    for(int i = 0; i < 3; i++) {
        lv_obj_t *btn = lv_btn_create(ir_item_cont);
        lv_obj_set_width(btn, lv_pct(100));
        lv_obj_set_style_radius(btn, 10, LV_PART_MAIN);
        apply_bg_color(btn);
        apply_no_border(btn);
        apply_no_shadow(btn);
        lv_obj_remove_style(btn, NULL, LV_STATE_FOCUS_KEY);
        lv_obj_set_style_outline_width(btn, 2, LV_STATE_FOCUS_KEY);
        lv_obj_set_style_outline_color(btn, lv_color_hex(EMBED_COLOR_FOCUS_ON), LV_STATE_FOCUS_KEY);
        lv_obj_set_style_bg_img_opa(btn, LV_OPA_100, LV_PART_MAIN);
        add_menu_button_events(btn, ir_btn_event_cb, (void *)(intptr_t)i);

        lv_obj_t *label = lv_label_create(btn);
        apply_text_color(label);
        lv_obj_set_style_text_font(label, FONT_BOLD_14, LV_PART_MAIN);
        lv_label_set_text(label, ir_menu_items[i]);

        lv_group_add_obj(lv_group_get_default(), btn);

        // Store first button to focus it later
        if(i == 0) {
            ir_first_btn = btn;
        }
    }

    // Trigger first scroll event to position items
    lv_event_send(ir_item_cont, LV_EVENT_SCROLL, NULL);

    // Add status bar last (on parent, not scr7_cont) so it appears on top
    create_status_bar(parent);
}

void entry7(void) {
    if(ir_first_btn) {
        lv_group_focus_obj(ir_first_btn);
    }
    lv_group_set_wrap(lv_group_get_default(), true);
}

void exit7(void) {
    lv_group_set_wrap(lv_group_get_default(), false);
}
void destroy7(void) {}

scr_lifecycle_t screen7 = {
    .create = create7,
    .entry = entry7,
    .exit  = exit7,
    .destroy = destroy7,
};
#endif

// --------------------- screen 7_1 --------------------- TV-B-Gone
#if 1
#include "WORLD_IR_CODES.h"

lv_obj_t *scr7_1_cont;
lv_obj_t *tvbg_progress_bar;
lv_obj_t *tvbg_progress_label;
lv_obj_t *tvbg_btn_americas;
lv_obj_t *tvbg_btn_emea;
lv_timer_t *tvbg_timer = NULL;

// TV-B-Gone state
bool tvbg_transmitting = false;
uint8_t tvbg_region = NA; // 0 = NA (Americas), 1 = EU (EMEA)
uint16_t tvbg_current_code = 0;
uint16_t tvbg_total_codes = 0;

extern IRsend irsend;

void entry7_1_anim(lv_obj_t *obj) { entry1_anim(obj); }
void exit7_1_anim(int user_data, lv_obj_t *obj) { exit1_anim(user_data, obj); }

static void scr7_1_back_btn_event_cb(lv_event_t * e)
{
    if(e->code == LV_EVENT_CLICKED){
        // Stop transmission if running
        if(tvbg_timer) {
            lv_timer_del(tvbg_timer);
            tvbg_timer = NULL;
        }
        tvbg_transmitting = false;
        exit7_1_anim(SCREEN7_ID, scr7_1_cont);
    }
}

void tvbg_timer_event(lv_timer_t *t);

static void tvbg_btn_americas_event(lv_event_t * e)
{
    if(e->code == LV_EVENT_CLICKED){
        // If already transmitting, stop it
        if(tvbg_transmitting) {
            tvbg_transmitting = false;
            if(tvbg_timer) {
                lv_timer_del(tvbg_timer);
                tvbg_timer = NULL;
            }
            lv_label_set_text(tvbg_progress_label, "Stopped");
            return;
        }

        tvbg_region = NA;
        tvbg_current_code = 0;
        tvbg_total_codes = num_NAcodes;
        tvbg_transmitting = true;

        // Update UI
        lv_label_set_text(tvbg_progress_label, "Transmitting: Americas");
        lv_bar_set_value(tvbg_progress_bar, 0, LV_ANIM_OFF);

        // Start timer if not already running
        if(!tvbg_timer) {
            tvbg_timer = lv_timer_create(tvbg_timer_event, 50, NULL);
        }
    }
}

static void tvbg_btn_emea_event(lv_event_t * e)
{
    if(e->code == LV_EVENT_CLICKED){
        // If already transmitting, stop it
        if(tvbg_transmitting) {
            tvbg_transmitting = false;
            if(tvbg_timer) {
                lv_timer_del(tvbg_timer);
                tvbg_timer = NULL;
            }
            lv_label_set_text(tvbg_progress_label, "Stopped");
            return;
        }

        tvbg_region = EU;
        tvbg_current_code = 0;
        tvbg_total_codes = num_EUcodes;
        tvbg_transmitting = true;

        // Update UI
        lv_label_set_text(tvbg_progress_label, "Transmitting: EMEA");
        lv_bar_set_value(tvbg_progress_bar, 0, LV_ANIM_OFF);

        // Start timer if not already running
        if(!tvbg_timer) {
            tvbg_timer = lv_timer_create(tvbg_timer_event, 50, NULL);
        }
    }
}

void tvbg_timer_event(lv_timer_t *t)
{
    if(!tvbg_transmitting || tvbg_current_code >= tvbg_total_codes) {
        // Transmission complete
        if(tvbg_transmitting) {
            lv_label_set_text(tvbg_progress_label, "Complete!");
            lv_bar_set_value(tvbg_progress_bar, 100, LV_ANIM_ON);
            tvbg_transmitting = false;
        }
        return;
    }

    // Set the global power code (used by read_bits function)
    if(tvbg_region == NA) {
        powerCode = NApowerCodes[tvbg_current_code];
    } else {
        powerCode = EUpowerCodes[tvbg_current_code];
    }

    const uint8_t freq = powerCode->timer_val;
    const uint8_t numpairs = powerCode->numpairs;
    const uint8_t bitcompression = powerCode->bitcompression;

    // Decode the IR code into raw data
    uint16_t rawData[300];
    code_ptr = 0;
    bitsleft_r = 0;

    for (uint8_t k = 0; k < numpairs; k++) {
        uint16_t ti = (read_bits(bitcompression)) * 2;
        // Swap ontime/offtime for active-low IR LED (like NEMO ACTIVE_LOW_IR)
        uint16_t offtime = powerCode->times[ti];      // read word 1 - actually offtime for active-low
        uint16_t ontime = powerCode->times[ti + 1];   // read word 2 - actually ontime for active-low

        rawData[k * 2] = offtime * 10;
        rawData[(k * 2) + 1] = ontime * 10;
    }

    // Re-initialize IR sender before transmitting
    irsend.begin();

    // Send the IR code
    irsend.sendRaw(rawData, (numpairs * 2), freq);

    // Update progress
    tvbg_current_code++;
    uint8_t progress = (tvbg_current_code * 100) / tvbg_total_codes;
    lv_bar_set_value(tvbg_progress_bar, progress, LV_ANIM_OFF);

}

static void create7_1(lv_obj_t *parent)
{
    scr7_1_cont = create_screen_container(parent);

    // Title
    lv_obj_t *label = lv_label_create(scr7_1_cont);
    apply_text_color(label);
    lv_obj_set_style_text_font(label, FONT_BOLD_18, LV_PART_MAIN);
    lv_label_set_text(label, "TV-B-Gone");
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 10);

    // Progress bar
    tvbg_progress_bar = lv_bar_create(scr7_1_cont);
    lv_obj_set_size(tvbg_progress_bar, 280, 20);
    lv_obj_align(tvbg_progress_bar, LV_ALIGN_CENTER, 0, -20);
    lv_bar_set_range(tvbg_progress_bar, 0, 100);
    lv_bar_set_value(tvbg_progress_bar, 0, LV_ANIM_OFF);

    // Progress label (above progress bar)
    tvbg_progress_label = lv_label_create(scr7_1_cont);
    apply_text_color(tvbg_progress_label);
    lv_obj_set_style_text_font(tvbg_progress_label, FONT_BOLD_14, LV_PART_MAIN);
    lv_label_set_text(tvbg_progress_label, "Ready");
    lv_obj_align(tvbg_progress_label, LV_ALIGN_CENTER, 0, -50);

    // EMEA button (bottom right)
    tvbg_btn_emea = lv_btn_create(scr7_1_cont);
    lv_obj_set_size(tvbg_btn_emea, 100, 40);
    lv_obj_align(tvbg_btn_emea, LV_ALIGN_BOTTOM_RIGHT, -10, -10);
    lv_obj_set_style_border_color(tvbg_btn_emea, lv_color_hex(EMBED_COLOR_BORDER), LV_PART_MAIN);
    lv_obj_set_style_border_width(tvbg_btn_emea, 1, LV_PART_MAIN);
    apply_no_shadow(tvbg_btn_emea);
    apply_bg_color(tvbg_btn_emea);
    lv_obj_remove_style(tvbg_btn_emea, NULL, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_pad(tvbg_btn_emea, 2, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_width(tvbg_btn_emea, 2, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_color(tvbg_btn_emea, lv_color_hex(EMBED_COLOR_FOCUS_ON), LV_STATE_FOCUS_KEY);
    lv_obj_add_event_cb(tvbg_btn_emea, tvbg_btn_emea_event, LV_EVENT_CLICKED, NULL);
    lv_obj_t *btn_label = lv_label_create(tvbg_btn_emea);
    apply_text_color(btn_label);
    lv_obj_set_style_text_font(btn_label, FONT_BOLD_14, LV_PART_MAIN);
    lv_label_set_text(btn_label, "EMEA");
    lv_obj_center(btn_label);
    lv_group_add_obj(lv_group_get_default(), tvbg_btn_emea);

    // Americas button (bottom left)
    tvbg_btn_americas = lv_btn_create(scr7_1_cont);
    lv_obj_set_size(tvbg_btn_americas, 100, 40);
    lv_obj_align(tvbg_btn_americas, LV_ALIGN_BOTTOM_LEFT, 10, -10);
    lv_obj_set_style_border_color(tvbg_btn_americas, lv_color_hex(EMBED_COLOR_BORDER), LV_PART_MAIN);
    lv_obj_set_style_border_width(tvbg_btn_americas, 1, LV_PART_MAIN);
    apply_no_shadow(tvbg_btn_americas);
    apply_bg_color(tvbg_btn_americas);
    lv_obj_remove_style(tvbg_btn_americas, NULL, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_pad(tvbg_btn_americas, 2, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_width(tvbg_btn_americas, 2, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_color(tvbg_btn_americas, lv_color_hex(EMBED_COLOR_FOCUS_ON), LV_STATE_FOCUS_KEY);
    lv_obj_add_event_cb(tvbg_btn_americas, tvbg_btn_americas_event, LV_EVENT_CLICKED, NULL);
    btn_label = lv_label_create(tvbg_btn_americas);
    apply_text_color(btn_label);
    lv_obj_set_style_text_font(btn_label, FONT_BOLD_14, LV_PART_MAIN);
    lv_label_set_text(btn_label, "Americas");
    lv_obj_center(btn_label);
    lv_group_add_obj(lv_group_get_default(), tvbg_btn_americas);

    // Back button (using standard "<" button in top left)
    scr_back_btn_create(scr7_1_cont, scr7_1_back_btn_event_cb);
}

static void entry7_1(void)
{
    lv_group_set_wrap(lv_group_get_default(), true);
}

static void exit7_1(void)
{
    lv_group_set_wrap(lv_group_get_default(), false);

    // Clean up timer
    if(tvbg_timer) {
        lv_timer_del(tvbg_timer);
        tvbg_timer = NULL;
    }
    tvbg_transmitting = false;
}

void destroy7_1(void) {}

scr_lifecycle_t screen7_1 = {
    .create = create7_1,
    .entry = entry7_1,
    .exit  = exit7_1,
    .destroy = destroy7_1,
};
#endif

//************************************[ screen 7.3 ]****************************************** IR Playback
#if 1
// View modes for IR Playback
enum IRPlaybackMode {
    IR_PLAYBACK_FILE_BROWSER,
    IR_PLAYBACK_BUTTON_LIST
};

lv_obj_t *scr7_3_cont;
lv_obj_t *ir_playback_path_label;
lv_obj_t *ir_playback_file_list;
lv_obj_t *ir_playback_led;
lv_obj_t *ir_playback_keyboard = NULL;  // Keyboard for text input
lv_obj_t *ir_playback_textarea = NULL;  // Text area for keyboard
char ir_playback_current_path[256] = "/ir";  // Default IR directory
IRPlaybackMode ir_playback_mode = IR_PLAYBACK_FILE_BROWSER;
IRRemote ir_playback_current_remote;  // Currently loaded remote
bool ir_long_press_occurred = false;  // Flag to prevent CLICKED after LONG_PRESSED
char ir_selected_remote[256] = "";  // Selected remote filename (for context menu)
int ir_selected_button_index = -1;  // Selected button index (for context menu)
bool ir_deferred_capture_dialog = false;  // Flag to defer capture dialog entry after msgbox cleanup
int ir_deferred_button_index = -1;  // Button index for deferred capture dialog

void entry7_3_anim(lv_obj_t *obj) { entry1_anim(obj); }
void exit7_3_anim(int user_data, lv_obj_t *obj) { exit1_anim(user_data, obj); }

// Forward declarations
void ir_playback_load_directory(const char *path);
void ir_playback_show_buttons();
static void ir_create_remote_keyboard();
static void ir_rename_remote_keyboard();
static void ir_delete_remote();
static void ir_remote_keyboard_event(lv_event_t *e);
static void ir_create_button_event(lv_event_t *e);  // "+" button in button list
static void ir_edit_button_event(int button_index);  // Long-press on button
void enter_capture_dialog(bool add_mode, int button_index = -1);  // Entry to Screen 7.4

// Keyboard event handler for create/rename remote
static void ir_remote_keyboard_event(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_READY) {
        // User pressed OK
        const char *new_name = lv_textarea_get_text(ir_playback_textarea);

        if (strlen(new_name) > 0) {
            // Build full path with .ir extension
            char full_path[512];
            snprintf(full_path, sizeof(full_path), "/ir/%s.ir", new_name);

            // Check if we're renaming or creating new
            if (strlen(ir_selected_remote) > 0) {
                // Rename existing remote
                char old_path[512];
                snprintf(old_path, sizeof(old_path), "%s/%s", ir_playback_current_path, ir_selected_remote);


                if (SD.rename(old_path, full_path)) {
                    prompt_info("  Renamed successfully", 1500);
                } else {
                    prompt_info("  Rename failed!", 2000);
                }
            } else {
                // Create new remote

                if (createEmptyRemote(full_path)) {
                    prompt_info("  Remote created!", 1500);
                } else {
                    prompt_info("  Create failed!", 2000);
                }
            }

            // Reload directory
            ir_playback_load_directory(ir_playback_current_path);
        }

        // Clean up keyboard
        lv_group_t *g = lv_group_get_default();
        lv_group_set_editing(g, false);
        lv_group_focus_freeze(g, false);

        if (ir_playback_keyboard) {
            lv_obj_del(ir_playback_keyboard);
            ir_playback_keyboard = NULL;
        }
        if (ir_playback_textarea) {
            lv_obj_del(ir_playback_textarea);
            ir_playback_textarea = NULL;
        }

        lv_refr_now(NULL);
        if (lv_group_get_focused(g) == NULL && lv_obj_get_child_cnt(ir_playback_file_list) > 0) {
            lv_obj_t *first_item = lv_obj_get_child(ir_playback_file_list, 0);
            if (first_item) {
                lv_group_focus_obj(first_item);
            }
        }

        ir_selected_remote[0] = '\0';
    } else if (code == LV_EVENT_CANCEL) {
        // User cancelled
        lv_group_t *g = lv_group_get_default();
        lv_group_set_editing(g, false);
        lv_group_focus_freeze(g, false);

        if (ir_playback_keyboard) {
            lv_obj_del(ir_playback_keyboard);
            ir_playback_keyboard = NULL;
        }
        if (ir_playback_textarea) {
            lv_obj_del(ir_playback_textarea);
            ir_playback_textarea = NULL;
        }

        lv_refr_now(NULL);
        if (lv_group_get_focused(g) == NULL && lv_obj_get_child_cnt(ir_playback_file_list) > 0) {
            lv_obj_t *first_item = lv_obj_get_child(ir_playback_file_list, 0);
            if (first_item) {
                lv_group_focus_obj(first_item);
            }
        }

        ir_selected_remote[0] = '\0';
    }
}

// Show keyboard for creating new remote
static void ir_create_remote_keyboard()
{
    // Generate default name
    String default_name = generateRemoteName();

    // Create text area for input
    ir_playback_textarea = lv_textarea_create(lv_scr_act());
    lv_obj_set_size(ir_playback_textarea, LV_PCT(90), 40);
    lv_obj_align(ir_playback_textarea, LV_ALIGN_TOP_MID, 0, 40);
    lv_textarea_set_one_line(ir_playback_textarea, true);
    lv_textarea_set_text(ir_playback_textarea, default_name.c_str());

    // Enable and style the cursor for visibility
    lv_obj_set_style_anim_time(ir_playback_textarea, 400, LV_PART_CURSOR);
    lv_obj_set_style_bg_opa(ir_playback_textarea, LV_OPA_COVER, LV_PART_CURSOR);
    lv_obj_set_style_bg_color(ir_playback_textarea, lv_color_hex(EMBED_COLOR_FOCUS_ON), LV_PART_CURSOR);
    lv_obj_set_style_border_width(ir_playback_textarea, 1, LV_PART_CURSOR);
    lv_obj_set_style_border_color(ir_playback_textarea, lv_color_hex(EMBED_COLOR_FOCUS_ON), LV_PART_CURSOR);
    lv_textarea_set_cursor_pos(ir_playback_textarea, LV_TEXTAREA_CURSOR_LAST);

    // Create keyboard
    ir_playback_keyboard = lv_keyboard_create(lv_scr_act());
    lv_keyboard_set_textarea(ir_playback_keyboard, ir_playback_textarea);
    lv_obj_add_event_cb(ir_playback_keyboard, ir_remote_keyboard_event, LV_EVENT_ALL, NULL);

    // Setup focus and navigation
    lv_group_t *g = lv_group_get_default();
    lv_group_add_obj(g, ir_playback_textarea);
    lv_group_add_obj(g, ir_playback_keyboard);
    lv_group_focus_obj(ir_playback_keyboard);
    lv_obj_add_state(ir_playback_keyboard, LV_STATE_FOCUS_KEY);
    lv_group_set_editing(g, true);
    lv_group_focus_freeze(g, true);

    ir_selected_remote[0] = '\0';  // Clear selected remote (we're creating new)
}

// Show keyboard for renaming remote
// Precondition: ir_selected_remote must be set by caller
static void ir_rename_remote_keyboard()
{
    // Extract name without .ir extension
    char name_without_ext[256];
    strncpy(name_without_ext, ir_selected_remote, sizeof(name_without_ext) - 1);
    char *ext = strstr(name_without_ext, ".ir");
    if (ext) *ext = '\0';

    // Create text area for input
    ir_playback_textarea = lv_textarea_create(lv_scr_act());
    lv_obj_set_size(ir_playback_textarea, LV_PCT(90), 40);
    lv_obj_align(ir_playback_textarea, LV_ALIGN_TOP_MID, 0, 40);
    lv_textarea_set_one_line(ir_playback_textarea, true);
    lv_textarea_set_text(ir_playback_textarea, name_without_ext);

    // Enable and style the cursor for visibility
    lv_obj_set_style_anim_time(ir_playback_textarea, 400, LV_PART_CURSOR);
    lv_obj_set_style_bg_opa(ir_playback_textarea, LV_OPA_COVER, LV_PART_CURSOR);
    lv_obj_set_style_bg_color(ir_playback_textarea, lv_color_hex(EMBED_COLOR_FOCUS_ON), LV_PART_CURSOR);
    lv_obj_set_style_border_width(ir_playback_textarea, 1, LV_PART_CURSOR);
    lv_obj_set_style_border_color(ir_playback_textarea, lv_color_hex(EMBED_COLOR_FOCUS_ON), LV_PART_CURSOR);
    lv_textarea_set_cursor_pos(ir_playback_textarea, LV_TEXTAREA_CURSOR_LAST);

    // Create keyboard
    ir_playback_keyboard = lv_keyboard_create(lv_scr_act());
    lv_keyboard_set_textarea(ir_playback_keyboard, ir_playback_textarea);
    lv_obj_add_event_cb(ir_playback_keyboard, ir_remote_keyboard_event, LV_EVENT_ALL, NULL);

    // Setup focus and navigation
    lv_group_t *g = lv_group_get_default();
    lv_group_add_obj(g, ir_playback_textarea);
    lv_group_add_obj(g, ir_playback_keyboard);
    lv_group_focus_obj(ir_playback_keyboard);
    lv_obj_add_state(ir_playback_keyboard, LV_STATE_FOCUS_KEY);
    lv_group_set_editing(g, true);
    lv_group_focus_freeze(g, true);
}

// Delete remote file
// Precondition: ir_selected_remote must be set by caller
static void ir_delete_remote()
{
    char full_path[512];
    snprintf(full_path, sizeof(full_path), "%s/%s", ir_playback_current_path, ir_selected_remote);


    if (SD.remove(full_path)) {
        prompt_info("  Deleted successfully", 1500);
        ir_playback_load_directory(ir_playback_current_path);
    } else {
        prompt_info("  Delete failed!", 2000);
    }

    ir_selected_remote[0] = '\0';
}

// Event handler for back-to-files button
static void ir_playback_back_to_files_event(lv_event_t *e)
{
    if (e->code == LV_EVENT_CLICKED) {
        ir_playback_mode = IR_PLAYBACK_FILE_BROWSER;
        ir_playback_load_directory(ir_playback_current_path);
    }
}

// Event handler for button transmission
static void ir_playback_button_event(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *btn = lv_event_get_target(e);
    int signal_index = (int)(intptr_t)lv_obj_get_user_data(btn);

    if (code == LV_EVENT_PRESSED) {
        // Reset long press flag
        ir_long_press_occurred = false;
    }
    else if (code == LV_EVENT_LONG_PRESSED) {
        // Set flag to prevent CLICKED from firing
        ir_long_press_occurred = true;

        if (signal_index >= 0 && signal_index < ir_playback_current_remote.size()) {
            ir_edit_button_event(signal_index);
        }
    }
    else if (code == LV_EVENT_CLICKED) {
        // Ignore CLICKED if it followed a LONG_PRESSED
        if (ir_long_press_occurred) {
            ir_long_press_occurred = false;
            return;
        }

        if (signal_index >= 0 && signal_index < ir_playback_current_remote.size()) {
            const IRSignal& signal = ir_playback_current_remote.signals[signal_index];

            // Show transmission feedback
            lv_led_on(ir_playback_led);
            lv_refr_now(NULL);

            // Transmit the signal
            bool result = transmitIRSignal(signal, irsend);

            lv_led_off(ir_playback_led);

            if (!result) {
                prompt_info("  Transmission failed", 2000);
            }
        }
    }
}

// Event handler for "+ Add Button" click
static void ir_create_button_event(lv_event_t *e)
{
    if (e->code == LV_EVENT_CLICKED) {
        enter_capture_dialog(true);  // ADD mode
    }
}

// Event handler for long-press on button (edit/delete)
static void ir_edit_button_event(int button_index)
{
    if (button_index < 0 || button_index >= ir_playback_current_remote.size()) {
        return;
    }

    ir_selected_button_index = button_index;
    const IRSignal& signal = ir_playback_current_remote.signals[button_index];


    // Show context menu (no close button, just add Cancel as a button)
    static const char * btns[] = {"Edit", "Del", "Test", "Cancel", ""};
    lv_obj_t * mbox = lv_msgbox_create(NULL, "Button Options", NULL, btns, false);  // false = no X button
    lv_obj_center(mbox);

    // Add event callback
    lv_obj_add_event_cb(mbox, [](lv_event_t *e) {
        lv_event_code_t code = lv_event_get_code(e);
        if (code == LV_EVENT_VALUE_CHANGED) {
            lv_obj_t *mbox = lv_event_get_current_target(e);
            const char *txt = lv_msgbox_get_active_btn_text(mbox);

            if (txt) {

                if (strcmp(txt, "Test") == 0) {
                    // Test/transmit the signal
                    const IRSignal& signal = ir_playback_current_remote.signals[ir_selected_button_index];
                    lv_led_on(ir_playback_led);
                    transmitIRSignal(signal, irsend);
                    lv_led_off(ir_playback_led);
                    prompt_info("  Signal transmitted!", 1500);
                }
                else if (strcmp(txt, "Edit") == 0) {
                    // Edit the button (allows rename and recapture)
                    // Defer entry to capture dialog until after msgbox cleanup
                    ir_deferred_capture_dialog = true;
                    ir_deferred_button_index = ir_selected_button_index;
                }
                else if (strcmp(txt, "Del") == 0) {
                    // Delete the button
                    // Save filepath before it gets cleared
                    String filepath = ir_playback_current_remote.filepath;

                    if (deleteSignalFromRemote(filepath.c_str(), ir_selected_button_index)) {
                        prompt_info("  Button deleted!", 1500);

                        // Check if the remote file still exists (it gets deleted if it was the last button)
                        if (!SD.exists(filepath.c_str())) {
                            // Remote file was deleted, return to remotes list
                            ir_playback_mode = IR_PLAYBACK_FILE_BROWSER;
                            ir_playback_load_directory(ir_playback_current_path);
                        } else {
                            // Reload the remote and refresh button list
                            parseFlipperIRFile(filepath.c_str(), ir_playback_current_remote);
                            ir_playback_show_buttons();
                        }
                    } else {
                        prompt_info("  Delete failed!", 2000);
                    }
                }
                // If Cancel or any other button, just close
            }

            // Close msgbox and restore navigation
            lv_group_t *g = lv_group_get_default();
            lv_group_set_editing(g, false);
            lv_group_focus_freeze(g, false);
            lv_msgbox_close(mbox);
            lv_refr_now(NULL);

            if (lv_group_get_focused(g) == NULL && lv_obj_get_child_cnt(ir_playback_file_list) > 0) {
                lv_obj_t *first_item = lv_obj_get_child(ir_playback_file_list, 0);
                if (first_item) {
                    lv_group_focus_obj(first_item);
                }
            }

            ir_selected_button_index = -1;

            // Check if we need to enter capture dialog after cleanup
            if (ir_deferred_capture_dialog) {
                ir_deferred_capture_dialog = false;
                enter_capture_dialog(false, ir_deferred_button_index);  // EDIT mode
                ir_deferred_button_index = -1;
            }
        }
    }, LV_EVENT_VALUE_CHANGED, NULL);

    // Setup focus
    lv_group_t *g = lv_group_get_default();
    lv_obj_t *btnm = lv_msgbox_get_btns(mbox);

    if (btnm) {
        lv_group_add_obj(g, btnm);
        lv_group_focus_obj(btnm);
        lv_obj_add_state(btnm, LV_STATE_FOCUS_KEY);
        lv_group_set_editing(g, true);
    }
    lv_group_focus_freeze(g, true);
}

// Show button list from currently loaded remote
void ir_playback_show_buttons()
{
    lv_led_on(ir_playback_led);
    lv_obj_clean(ir_playback_file_list);

    // Extract just the filename from the full path
    const char *filename = strrchr(ir_playback_current_remote.filepath.c_str(), '/');
    if (filename) filename++;
    else filename = ir_playback_current_remote.filepath.c_str();

    lv_label_set_text_fmt(ir_playback_path_label, "%s", filename);

    // Add each button from the remote
    for (int i = 0; i < ir_playback_current_remote.size(); i++) {
        const IRSignal& signal = ir_playback_current_remote.signals[i];

        // Show button name only, without protocol label
        char item_text[64];
        snprintf(item_text, sizeof(item_text), " %s", signal.name.c_str());

        lv_obj_t *item = lv_list_add_btn(ir_playback_file_list, NULL, item_text);
        lv_obj_set_user_data(item, (void*)(intptr_t)i);  // Store button index
        lv_obj_add_event_cb(item, ir_playback_button_event, LV_EVENT_PRESSED, NULL);
        lv_obj_add_event_cb(item, ir_playback_button_event, LV_EVENT_LONG_PRESSED, NULL);
        lv_obj_add_event_cb(item, ir_playback_button_event, LV_EVENT_CLICKED, NULL);
    }

    // Add "+" button to add new button
    lv_obj_t *add_item = lv_list_add_btn(ir_playback_file_list, NULL, "(New Button)");
    lv_obj_add_event_cb(add_item, ir_create_button_event, LV_EVENT_CLICKED, NULL);

    // Style list items and add to focus group
    for(int i = 0; i < lv_obj_get_child_cnt(ir_playback_file_list); i++) {
        lv_obj_t *item = lv_obj_get_child(ir_playback_file_list, i);
        apply_bg_color(item);
        apply_text_color(item);
        lv_obj_set_style_text_font(item, FONT_BOLD_14, LV_PART_MAIN);
        lv_obj_remove_style(item, NULL, LV_STATE_FOCUS_KEY);
        lv_obj_set_style_radius(item, 5, LV_STATE_FOCUS_KEY);
        lv_obj_set_style_outline_color(item, lv_color_hex(EMBED_COLOR_FOCUS_ON), LV_STATE_FOCUS_KEY);
        lv_obj_set_style_outline_width(item, 2, LV_STATE_FOCUS_KEY);
        lv_group_add_obj(lv_group_get_default(), item);
    }

    // Set focus to the first item (back button)
    if (lv_obj_get_child_cnt(ir_playback_file_list) > 0) {
        lv_obj_t *first_item = lv_obj_get_child(ir_playback_file_list, 0);
        if (first_item) {
            lv_group_focus_obj(first_item);
        }
    }

    lv_led_off(ir_playback_led);
    tft.drawPixel(0, 0, 0);
}

static void ir_playback_list_event(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *obj = lv_event_get_target(e);

    if (code == LV_EVENT_PRESSED) {
        // Reset flag on new press
        ir_long_press_occurred = false;
    }
    else if (code == LV_EVENT_LONG_PRESSED) {
        // Set flag to prevent CLICKED from firing
        ir_long_press_occurred = true;

        const char *item_text = lv_list_get_btn_text(ir_playback_file_list, obj);

        if (item_text && strcmp(item_text, " .. (Parent)") != 0 &&
            strcmp(item_text, "(New Button)") != 0) {
            // Long-press on .ir file - show rename/delete menu
            // Check if this is NOT a folder (folders don't get long-press menu)
            const char *display_name = item_text + 1; // Skip leading space

            // Reconstruct full filename with .ir extension
            snprintf(ir_selected_remote, sizeof(ir_selected_remote), "%s.ir", display_name);


            // Only show menu if not a folder (simple heuristic: folders won't parse as .ir files)
            bool is_folder = false;
            for (const char *c = display_name; *c; c++) {
                if (*c == '/') {
                    is_folder = true;
                    break;
                }
            }

            if (!is_folder) {

                // Show context menu with buttons (no close button, just add Cancel as a button)
                static const char * btns[] = {"Rename", "Delete", "Cancel", ""};
                lv_obj_t * mbox = lv_msgbox_create(NULL, "Remote Options", NULL, btns, false);  // false = no X button
                lv_obj_center(mbox);

                // Add event callback
                lv_obj_add_event_cb(mbox, [](lv_event_t *e) {
                    lv_event_code_t code = lv_event_get_code(e);
                    if (code == LV_EVENT_VALUE_CHANGED) {
                        lv_obj_t *mbox = lv_event_get_current_target(e);
                        const char *txt = lv_msgbox_get_active_btn_text(mbox);

                        if (txt) {

                            if (strcmp(txt, "Rename") == 0) {
                                ir_rename_remote_keyboard();
                            } else if (strcmp(txt, "Delete") == 0) {
                                ir_delete_remote();
                            }
                            // If Cancel or any other button, just close
                        }

                        // Close msgbox and restore navigation
                        lv_group_t *g = lv_group_get_default();
                        lv_group_set_editing(g, false);
                        lv_group_focus_freeze(g, false);
                        lv_msgbox_close(mbox);
                        lv_refr_now(NULL);

                        if (lv_group_get_focused(g) == NULL && lv_obj_get_child_cnt(ir_playback_file_list) > 0) {
                            lv_obj_t *first_item = lv_obj_get_child(ir_playback_file_list, 0);
                            if (first_item) {
                                lv_group_focus_obj(first_item);
                            }
                        }
                    }
                }, LV_EVENT_VALUE_CHANGED, NULL);

                // Setup focus
                lv_group_t *g = lv_group_get_default();
                lv_obj_t *btnm = lv_msgbox_get_btns(mbox);

                if (btnm) {
                    lv_group_add_obj(g, btnm);
                    lv_group_focus_obj(btnm);
                    lv_obj_add_state(btnm, LV_STATE_FOCUS_KEY);
                    lv_group_set_editing(g, true);
                }
                lv_group_focus_freeze(g, true);
            }
        }
    }
    else if (code == LV_EVENT_CLICKED) {
        // Ignore CLICKED if it followed a LONG_PRESSED
        if (ir_long_press_occurred) {
            ir_long_press_occurred = false;
            return;
        }

        const char *item_text = lv_list_get_btn_text(ir_playback_file_list, obj);

        if (item_text) {
            // Handle "Create New Remote" button
            if (strcmp(item_text, "(New Remote)") == 0) {
                ir_create_remote_keyboard();
                return;
            }

            // Handle parent directory navigation
            if (strcmp(item_text, " .. (Parent)") == 0) {
                char *last_slash = strrchr(ir_playback_current_path, '/');
                if (last_slash && last_slash != ir_playback_current_path) {
                    *last_slash = '\0';
                } else {
                    strcpy(ir_playback_current_path, "/ir");
                }
                ir_playback_load_directory(ir_playback_current_path);
            }
            // Handle folder or file
            else {
                const char *display_name = item_text + 1; // Skip leading space

                // Check if it's a folder by trying to read it as a directory
                char test_path[512];
                snprintf(test_path, sizeof(test_path), "%s/%s", ir_playback_current_path, display_name);
                DIR *test_dir = opendir(test_path);

                if (test_dir) {
                    // It's a folder
                    closedir(test_dir);
                    char new_path[256];
                    snprintf(new_path, sizeof(new_path), "%s/%s", ir_playback_current_path, display_name);
                    strncpy(ir_playback_current_path, new_path, sizeof(ir_playback_current_path) - 1);
                    ir_playback_load_directory(ir_playback_current_path);
                } else {
                    // It's a file - add .ir extension
                    char file_name[128];
                    snprintf(file_name, sizeof(file_name), "%s.ir", display_name);

                    // Build full path
                    char full_path[512];
                    snprintf(full_path, sizeof(full_path), "%s/%s", ir_playback_current_path, file_name);

                // Parse Flipper Zero .ir file
                lv_led_on(ir_playback_led);
                if (parseFlipperIRFile(full_path, ir_playback_current_remote)) {
                    lv_led_off(ir_playback_led);


                    // Switch to button list mode
                    ir_playback_mode = IR_PLAYBACK_BUTTON_LIST;
                    ir_playback_show_buttons();
                } else {
                    lv_led_off(ir_playback_led);
                    prompt_info("  Error parsing IR file", 2000);
                }
                }
            }
        }
    }
}

static void scr7_3_back_btn_event_cb(lv_event_t *e)
{
    if (e->code == LV_EVENT_CLICKED) {
        // If we're in button list mode, go back to file browser
        if (ir_playback_mode == IR_PLAYBACK_BUTTON_LIST) {
            ir_playback_mode = IR_PLAYBACK_FILE_BROWSER;
            ir_playback_load_directory(ir_playback_current_path);
        }
        // Otherwise, exit to IR menu
        else {
            exit7_3_anim(SCREEN7_ID, scr7_3_cont);
        }
    }
}

void ir_playback_load_directory(const char *path)
{
    lv_led_on(ir_playback_led);
    lv_refr_now(NULL);

    lv_obj_clean(ir_playback_file_list);
    lv_label_set_text_fmt(ir_playback_path_label, "Path: %s", path);

    if (strcmp(path, "/ir") != 0) {
        lv_obj_t *parent_item = lv_list_add_btn(ir_playback_file_list, NULL, " .. (Parent)");
        lv_obj_add_event_cb(parent_item, ir_playback_list_event, LV_EVENT_CLICKED, NULL);
    }

    // Create directory if it doesn't exist
    if (!SD.exists(path)) {
        SD.mkdir(path);
    }

    // Try multiple mount points (SD card VFS mount location)
    const char* mount_points[] = {"/sd", "/sdcard", "/mnt/sd"};
    DIR *dir = NULL;

    for (int i = 0; i < 3; i++) {
        char test_path[512];
        snprintf(test_path, sizeof(test_path), "%s%s", mount_points[i], path);
        dir = opendir(test_path);
        if (dir) break;
    }

    // Fallback: try direct path
    if (!dir) {
        dir = opendir(path);
    }

    if (!dir) {
        lv_obj_t *item = lv_list_add_btn(ir_playback_file_list, NULL, " Directory not found");
        apply_text_color(item);
        lv_led_off(ir_playback_led);
        return;
    }

    std::vector<String> directories;
    std::vector<String> files;

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        if (entry->d_type == DT_DIR) {
            directories.push_back(String(entry->d_name));
        } else if (entry->d_type == DT_REG) {
            String filename = String(entry->d_name);
            if (filename.endsWith(".ir")) {
                files.push_back(filename);
            }
        }
    }
    closedir(dir);

    for (const auto &dir_name : directories) {
        char item_text[256];
        snprintf(item_text, sizeof(item_text), " %s", dir_name.c_str());
        lv_obj_t *item = lv_list_add_btn(ir_playback_file_list, NULL, item_text);
        lv_obj_add_event_cb(item, ir_playback_list_event, LV_EVENT_CLICKED, NULL);
    }

    for (const auto &file_name : files) {
        // Remove .ir extension for display
        String display_name = file_name;
        if (display_name.endsWith(".ir")) {
            display_name = display_name.substring(0, display_name.length() - 3);
        }

        char item_text[256];
        snprintf(item_text, sizeof(item_text), " %s", display_name.c_str());
        lv_obj_t *item = lv_list_add_btn(ir_playback_file_list, NULL, item_text);
        lv_obj_add_event_cb(item, ir_playback_list_event, LV_EVENT_PRESSED, NULL);
        lv_obj_add_event_cb(item, ir_playback_list_event, LV_EVENT_LONG_PRESSED, NULL);
        lv_obj_add_event_cb(item, ir_playback_list_event, LV_EVENT_CLICKED, NULL);
    }

    // Add "+" button to create new remote
    lv_obj_t *add_item = lv_list_add_btn(ir_playback_file_list, NULL, "(New Remote)");
    lv_obj_add_event_cb(add_item, ir_playback_list_event, LV_EVENT_CLICKED, NULL);

    // Apply theme colors to all list items and add to group
    for(int i = 0; i < lv_obj_get_child_cnt(ir_playback_file_list); i++) {
        lv_obj_t *item = lv_obj_get_child(ir_playback_file_list, i);
        lv_obj_set_style_text_font(item, FONT_BOLD_14, LV_PART_MAIN);
        apply_bg_color(item);
        apply_text_color(item);
        lv_obj_remove_style(item, NULL, LV_STATE_FOCUS_KEY);
        lv_obj_set_style_radius(item, 5, LV_STATE_FOCUS_KEY);
        lv_obj_set_style_outline_color(item, lv_color_hex(EMBED_COLOR_FOCUS_ON), LV_STATE_FOCUS_KEY);
        lv_obj_set_style_outline_width(item, 2, LV_STATE_FOCUS_KEY);
        lv_group_add_obj(lv_group_get_default(), item);
    }

    // Set focus to the first item
    if (lv_obj_get_child_cnt(ir_playback_file_list) > 0) {
        lv_obj_t *first_item = lv_obj_get_child(ir_playback_file_list, 0);
        if (first_item) {
            lv_group_focus_obj(first_item);
        }
    }

    lv_led_off(ir_playback_led);
}

static void create7_3(lv_obj_t *parent)
{
    scr7_3_cont = lv_obj_create(parent);
    lv_obj_set_size(scr7_3_cont, 170, 320);
	lv_obj_set_style_transform_pivot_x(scr7_3_cont, 90, LV_PART_MAIN);
	lv_obj_set_style_transform_pivot_y(scr7_3_cont, 80, LV_PART_MAIN);
	lv_obj_set_style_transform_angle(scr7_3_cont, 2700, LV_PART_MAIN);  // 270° = 2700 (tenths of degree)

    apply_bg_color(scr7_3_cont);
    apply_no_scrollbar(scr7_3_cont);
    apply_no_border(scr7_3_cont);
    apply_no_padding(scr7_3_cont);

    // Title label
    lv_obj_t *label = lv_label_create(scr7_3_cont);
    apply_text_color(label);
    lv_obj_set_style_text_font(label, FONT_BOLD_18, LV_PART_MAIN);
    lv_label_set_text(label, "IR Remote");
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 10);

    // IR LED indicator
    ir_playback_led = lv_led_create(scr7_3_cont);
    lv_obj_align(ir_playback_led, LV_ALIGN_TOP_RIGHT, -13, 13);
    lv_obj_set_size(ir_playback_led, 15, 15);
    lv_led_set_color(ir_playback_led, lv_palette_main(LV_PALETTE_RED));
    lv_led_off(ir_playback_led);

    // Path label
    ir_playback_path_label = lv_label_create(scr7_3_cont);
    apply_text_color(ir_playback_path_label);
    lv_obj_set_style_text_font(ir_playback_path_label, FONT_BOLD_14, LV_PART_MAIN);
    lv_label_set_text(ir_playback_path_label, "Path: /ir");
    //lv_obj_set_width(ir_playback_path_label, 200);
    lv_obj_set_width(ir_playback_path_label, 160);
    lv_label_set_long_mode(ir_playback_path_label, LV_LABEL_LONG_DOT);
    lv_obj_align(ir_playback_path_label, LV_ALIGN_TOP_LEFT, 10, 35);

    // File/button list
    ir_playback_file_list = lv_list_create(scr7_3_cont);
    lv_obj_set_size(ir_playback_file_list, 170, 250);
    lv_obj_align(ir_playback_file_list, LV_ALIGN_TOP_LEFT, 0, 60);
    //lv_obj_set_size(ir_playback_file_list, 300, 105);
    //lv_obj_align(ir_playback_file_list, LV_ALIGN_TOP_MID, 0, 60);
    apply_bg_color(ir_playback_file_list);
    //lv_obj_set_style_border_width(ir_playback_file_list, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(ir_playback_file_list, lv_color_hex(EMBED_COLOR_BORDER), LV_PART_MAIN);

    ir_playback_load_directory(ir_playback_current_path);

    // Back button (using standard "<" button in top left) - Added last for default focus
    scr_back_btn_create(scr7_3_cont, scr7_3_back_btn_event_cb);
}

static void enter7_3(void)
{
    entry7_3_anim(scr7_3_cont);
    lv_group_set_wrap(lv_group_get_default(), true);

    // Refresh the UI based on current mode
    if (ir_playback_mode == IR_PLAYBACK_BUTTON_LIST) {
        // Returning from Screen 7.4 - show button list
        ir_playback_show_buttons();
    } else {
        // Coming from elsewhere - show file browser
        ir_playback_mode = IR_PLAYBACK_FILE_BROWSER;
        ir_playback_load_directory(ir_playback_current_path);
    }

    // Prevent parent screen from scrolling when rotated container has focused items
    lv_obj_t *parent = lv_obj_get_parent(scr7_3_cont);
    if (parent) {
        lv_obj_clear_flag(parent, LV_OBJ_FLAG_SCROLLABLE);
    }
}

static void exit7_3(void)
{
    lv_group_set_wrap(lv_group_get_default(), false);

    // Re-enable scrolling on parent when exiting
    lv_obj_t *parent = lv_obj_get_parent(scr7_3_cont);
    if (parent) {
        lv_obj_add_flag(parent, LV_OBJ_FLAG_SCROLLABLE);
    }
}

static void destroy7_3(void)
{
    if (scr7_3_cont) {
        lv_obj_del(scr7_3_cont);
        scr7_3_cont = NULL;
    }
}

scr_lifecycle_t screen7_3 = {
    .create = create7_3,
    .entry = enter7_3,
    .exit = exit7_3,
    .destroy = destroy7_3,
};
#endif

//************************************[ screen 7.4 ]****************************************** IR Button Capture/Edit Dialog
#if 1
lv_obj_t *scr7_4_cont;
lv_obj_t *capture_status_label;
lv_obj_t *capture_protocol_label;
lv_obj_t *capture_data_label;
lv_obj_t *capture_name_label;
lv_obj_t *capture_btn_capture;
lv_obj_t *capture_btn_test;
lv_obj_t *capture_btn_name;
lv_obj_t *capture_btn_save;
lv_obj_t *capture_btn_back;
lv_obj_t *capture_keyboard = NULL;
lv_obj_t *capture_textarea = NULL;
lv_timer_t *capture_update_timer = NULL;

IRSignal capture_current_signal;  // Currently captured/edited signal
bool capture_mode_add = true;      // true = add new, false = edit existing
int capture_edit_index = -1;      // Index if editing
bool capture_is_capturing = false; // Currently capturing IR signal
bool capture_has_signal = false;   // Has a valid signal to save

void entry7_4_anim(lv_obj_t *obj) { entry1_anim(obj); }
void exit7_4_anim(int user_data, lv_obj_t *obj) { exit1_anim(user_data, obj); }

// Forward declarations
static void capture_btn_capture_event(lv_event_t *e);
static void capture_btn_test_event(lv_event_t *e);
static void capture_btn_name_event(lv_event_t *e);
static void capture_btn_save_event(lv_event_t *e);
static void scr7_4_back_btn_event_cb(lv_event_t *e);
static void capture_update_timer_event(lv_timer_t *timer);
static void capture_name_keyboard_event(lv_event_t *e);

// Timer event for IR capture polling
static void capture_update_timer_event(lv_timer_t *timer)
{
    if (!capture_is_capturing) return;

    decode_results results;
    if (irrecv.decode(&results)) {

        // Convert decode_results to IRSignal
        if (decodeResultsToIRSignal(results, capture_current_signal)) {
            capture_has_signal = true;
            capture_is_capturing = false;

            // Update UI
            lv_label_set_text(lv_obj_get_child(capture_btn_capture, 0), "Capture");
            lv_label_set_text(capture_status_label, "Status: Captured!");

            // Update protocol and data labels
            char buf[128];
            if (capture_current_signal.type == IR_SIGNAL_PARSED) {
                snprintf(buf, sizeof(buf), "Protocol: %s", capture_current_signal.protocol.c_str());
                lv_label_set_text(capture_protocol_label, buf);
                snprintf(buf, sizeof(buf), "Addr: 0x%X Cmd: 0x%X",
                         capture_current_signal.address, capture_current_signal.command);
                lv_label_set_text(capture_data_label, buf);
            } else {
                lv_label_set_text(capture_protocol_label, "Protocol: RAW");
                snprintf(buf, sizeof(buf), "Pulses: %d", capture_current_signal.raw_data.size());
                lv_label_set_text(capture_data_label, buf);
            }

            // Update button name label
            snprintf(buf, sizeof(buf), "Name: %s", capture_current_signal.name.c_str());
            lv_label_set_text(capture_name_label, buf);

            // Enable test and save buttons
            lv_obj_clear_state(capture_btn_test, LV_STATE_DISABLED);
            lv_obj_clear_state(capture_btn_save, LV_STATE_DISABLED);

            irrecv.disableIRIn();
        } else {
        }

        irrecv.resume();
    }
}

// Keyboard event handler for button name
static void capture_name_keyboard_event(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_READY) {
        // User pressed OK
        const char *new_name = lv_textarea_get_text(capture_textarea);

        if (strlen(new_name) > 0) {
            capture_current_signal.name = new_name;

            // Update the name label
            char buf[128];
            snprintf(buf, sizeof(buf), "Name: %s", capture_current_signal.name.c_str());
            lv_label_set_text(capture_name_label, buf);
        }

        // Clean up keyboard
        lv_group_t *g = lv_group_get_default();
        lv_group_set_editing(g, false);
        lv_group_focus_freeze(g, false);

        if (capture_keyboard) {
            lv_obj_del(capture_keyboard);
            capture_keyboard = NULL;
        }
        if (capture_textarea) {
            lv_obj_del(capture_textarea);
            capture_textarea = NULL;
        }

        lv_refr_now(NULL);
    } else if (code == LV_EVENT_CANCEL) {
        // User cancelled
        lv_group_t *g = lv_group_get_default();
        lv_group_set_editing(g, false);
        lv_group_focus_freeze(g, false);

        if (capture_keyboard) {
            lv_obj_del(capture_keyboard);
            capture_keyboard = NULL;
        }
        if (capture_textarea) {
            lv_obj_del(capture_textarea);
            capture_textarea = NULL;
        }

        lv_refr_now(NULL);
    }
}

// Capture button event handler
static void capture_btn_capture_event(lv_event_t *e)
{
    if (e->code == LV_EVENT_CLICKED) {
        if (!capture_is_capturing) {
            // Start capture

            // Enable IR receiver
            irrecv.enableIRIn();

            capture_is_capturing = true;
            lv_label_set_text(lv_obj_get_child(capture_btn_capture, 0), "Stop");
            lv_label_set_text(capture_status_label, "Status: Listening...");
            lv_label_set_text(capture_protocol_label, "Protocol: ---");
            lv_label_set_text(capture_data_label, "Data: ---");
        } else {
            // Stop capture
            irrecv.disableIRIn();
            capture_is_capturing = false;
            lv_label_set_text(lv_obj_get_child(capture_btn_capture, 0), "Capture");
            lv_label_set_text(capture_status_label, "Status: Stopped");
        }
    }
}

// Test button event handler
static void capture_btn_test_event(lv_event_t *e)
{
    if (e->code == LV_EVENT_CLICKED) {
        if (capture_has_signal) {
            prompt_info("  Transmitting...", 100);
            lv_refr_now(NULL);

            bool result = transmitIRSignal(capture_current_signal, irsend);

            if (result) {
                prompt_info("  Test OK!", 1500);
            } else {
                prompt_info("  Test failed!", 2000);
            }
        }
    }
}

// Name button event handler
static void capture_btn_name_event(lv_event_t *e)
{
    if (e->code == LV_EVENT_CLICKED) {

        // Create text area for input
        capture_textarea = lv_textarea_create(lv_scr_act());
        lv_obj_set_size(capture_textarea, LV_PCT(90), 40);
        lv_obj_align(capture_textarea, LV_ALIGN_TOP_MID, 0, 40);
        lv_textarea_set_one_line(capture_textarea, true);
        lv_textarea_set_text(capture_textarea, capture_current_signal.name.c_str());

        // Enable and style the cursor for visibility
        lv_obj_set_style_anim_time(capture_textarea, 400, LV_PART_CURSOR);
        lv_obj_set_style_bg_opa(capture_textarea, LV_OPA_COVER, LV_PART_CURSOR);
        lv_obj_set_style_bg_color(capture_textarea, lv_color_hex(EMBED_COLOR_FOCUS_ON), LV_PART_CURSOR);
        lv_obj_set_style_border_width(capture_textarea, 1, LV_PART_CURSOR);
        lv_obj_set_style_border_color(capture_textarea, lv_color_hex(EMBED_COLOR_FOCUS_ON), LV_PART_CURSOR);
        lv_textarea_set_cursor_pos(capture_textarea, LV_TEXTAREA_CURSOR_LAST);

        // Create keyboard
        capture_keyboard = lv_keyboard_create(lv_scr_act());
        lv_keyboard_set_textarea(capture_keyboard, capture_textarea);
        lv_obj_add_event_cb(capture_keyboard, capture_name_keyboard_event, LV_EVENT_ALL, NULL);

        // Setup focus and navigation
        lv_group_t *g = lv_group_get_default();
        lv_group_add_obj(g, capture_textarea);
        lv_group_add_obj(g, capture_keyboard);
        lv_group_focus_obj(capture_keyboard);
        lv_obj_add_state(capture_keyboard, LV_STATE_FOCUS_KEY);
        lv_group_set_editing(g, true);
        lv_group_focus_freeze(g, true);
    }
}

// Save button event handler
static void capture_btn_save_event(lv_event_t *e)
{
    if (e->code == LV_EVENT_CLICKED) {
        if (!capture_has_signal) return;


        // Check if filepath is empty
        if (ir_playback_current_remote.filepath.length() == 0) {
            prompt_info("  No remote loaded!", 2000);
            return;
        }

        // Save filepath before calling functions that may clear it
        String filepath = ir_playback_current_remote.filepath;

        bool success = false;
        if (capture_mode_add) {
            // Add new button
            success = appendSignalToRemote(filepath.c_str(),
                                          capture_current_signal);
            if (success) {
                prompt_info("  Button added!", 1500);
            } else {
                prompt_info("  Save failed!", 2000);
            }
        } else {
            // Replace existing button
            success = replaceSignalInRemote(filepath.c_str(),
                                           capture_edit_index, capture_current_signal);
            if (success) {
                prompt_info("  Button updated!", 1500);
            } else {
                prompt_info("  Update failed!", 2000);
            }
        }

        if (success) {
            // Reload the remote and return to button list
            parseFlipperIRFile(filepath.c_str(), ir_playback_current_remote);
            ir_playback_mode = IR_PLAYBACK_BUTTON_LIST;
            exit7_4_anim(SCREEN7_3_ID, scr7_4_cont);
        }
    }
}

// Back button event handler
static void scr7_4_back_btn_event_cb(lv_event_t *e)
{
    if (e->code == LV_EVENT_CLICKED) {
        // Stop capture if active
        if (capture_is_capturing) {
            irrecv.disableIRIn();
            capture_is_capturing = false;
        }

        exit7_4_anim(SCREEN7_3_ID, scr7_4_cont);
    }
}

// Entry function called from other screens
void enter_capture_dialog(bool add_mode, int button_index)
{
    capture_mode_add = add_mode;
    capture_edit_index = button_index;
    capture_has_signal = false;

    if (!add_mode && button_index >= 0 && button_index < ir_playback_current_remote.size()) {
        // Load existing signal for editing
        capture_current_signal = ir_playback_current_remote.signals[button_index];
        capture_has_signal = true;
    } else {
        // Create new signal with default name
        capture_current_signal = createDefaultIRSignal();
        capture_current_signal.name = generateButtonName(ir_playback_current_remote);
    }

    scr_mgr_push(SCREEN7_4_ID, true);
}

static void create7_4(lv_obj_t *parent)
{
    scr7_4_cont = create_screen_container(parent);

    // Title
    lv_obj_t *label = lv_label_create(scr7_4_cont);
    apply_text_color(label);
    lv_obj_set_style_text_font(label, FONT_BOLD_18, LV_PART_MAIN);
    lv_label_set_text(label, "IR Button");
    lv_obj_align(label, LV_ALIGN_TOP_LEFT, 40, 10);

    // Status label
    capture_status_label = lv_label_create(scr7_4_cont);
    apply_text_color(capture_status_label);
    lv_obj_set_style_text_font(capture_status_label, FONT_BOLD_14, LV_PART_MAIN);
    lv_label_set_text(capture_status_label, "Status: Ready");
    lv_obj_align(capture_status_label, LV_ALIGN_TOP_LEFT, 10, 46);

    // Protocol label
    capture_protocol_label = lv_label_create(scr7_4_cont);
    apply_text_color(capture_protocol_label);
    lv_obj_set_style_text_font(capture_protocol_label, FONT_BOLD_14, LV_PART_MAIN);
    lv_label_set_text(capture_protocol_label, "Protocol: ---");
    lv_obj_align(capture_protocol_label, LV_ALIGN_TOP_LEFT, 10, 70);

    // Data label
    capture_data_label = lv_label_create(scr7_4_cont);
    lv_obj_set_width(capture_data_label, DISPALY_WIDTH - 20);
    apply_text_color(capture_data_label);
    lv_obj_set_style_text_font(capture_data_label, FONT_BOLD_14, LV_PART_MAIN);
    lv_label_set_text(capture_data_label, "Data: ---");
    lv_obj_align(capture_data_label, LV_ALIGN_TOP_LEFT, 10, 94);

    // Button Name label
    capture_name_label = lv_label_create(scr7_4_cont);
    lv_obj_set_width(capture_name_label, DISPALY_WIDTH - 20);
    apply_text_color(capture_name_label);
    lv_obj_set_style_text_font(capture_name_label, FONT_BOLD_14, LV_PART_MAIN);
    lv_label_set_text(capture_name_label, "Name: ---");
    lv_obj_align(capture_name_label, LV_ALIGN_TOP_LEFT, 10, 118);

    // Capture button (row 1 left)
    capture_btn_capture = lv_btn_create(scr7_4_cont);
    lv_obj_set_size(capture_btn_capture, 100, 30);
    lv_obj_align(capture_btn_capture, LV_ALIGN_TOP_RIGHT, -10, 30);
    lv_obj_set_style_border_color(capture_btn_capture, lv_color_hex(EMBED_COLOR_BORDER), LV_PART_MAIN);
    lv_obj_set_style_border_width(capture_btn_capture, 1, LV_PART_MAIN);
    apply_no_shadow(capture_btn_capture);
    apply_bg_color(capture_btn_capture);
    lv_obj_remove_style(capture_btn_capture, NULL, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_pad(capture_btn_capture, 2, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_width(capture_btn_capture, 2, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_color(capture_btn_capture, lv_color_hex(EMBED_COLOR_FOCUS_ON), LV_STATE_FOCUS_KEY);
    lv_obj_add_event_cb(capture_btn_capture, capture_btn_capture_event, LV_EVENT_CLICKED, NULL);
    lv_obj_t *btn_label = lv_label_create(capture_btn_capture);
    apply_text_color(btn_label);
    lv_obj_set_style_text_font(btn_label, FONT_BOLD_14, LV_PART_MAIN);
    lv_label_set_text(btn_label, "Capture");
    lv_obj_center(btn_label);
    lv_group_add_obj(lv_group_get_default(), capture_btn_capture);

    // Test button (row 1 right)
    capture_btn_test = lv_btn_create(scr7_4_cont);
    lv_obj_set_size(capture_btn_test, 100, 30);
    lv_obj_align(capture_btn_test, LV_ALIGN_TOP_RIGHT, -10, 65);
    lv_obj_set_style_border_color(capture_btn_test, lv_color_hex(EMBED_COLOR_BORDER), LV_PART_MAIN);
    lv_obj_set_style_border_width(capture_btn_test, 1, LV_PART_MAIN);
    apply_no_shadow(capture_btn_test);
    apply_bg_color(capture_btn_test);
    lv_obj_remove_style(capture_btn_test, NULL, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_pad(capture_btn_test, 2, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_width(capture_btn_test, 2, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_color(capture_btn_test, lv_color_hex(EMBED_COLOR_FOCUS_ON), LV_STATE_FOCUS_KEY);
    lv_obj_add_event_cb(capture_btn_test, capture_btn_test_event, LV_EVENT_CLICKED, NULL);
    btn_label = lv_label_create(capture_btn_test);
    apply_text_color(btn_label);
    lv_obj_set_style_text_font(btn_label, FONT_BOLD_14, LV_PART_MAIN);
    lv_label_set_text(btn_label, "Test");
    lv_obj_center(btn_label);
    lv_group_add_obj(lv_group_get_default(), capture_btn_test);

    // Name button (row 2 left)
    capture_btn_name = lv_btn_create(scr7_4_cont);
    lv_obj_set_size(capture_btn_name, 100, 30);
    lv_obj_align(capture_btn_name, LV_ALIGN_TOP_RIGHT, -10, 100);
    lv_obj_set_style_border_color(capture_btn_name, lv_color_hex(EMBED_COLOR_BORDER), LV_PART_MAIN);
    lv_obj_set_style_border_width(capture_btn_name, 1, LV_PART_MAIN);
    apply_no_shadow(capture_btn_name);
    apply_bg_color(capture_btn_name);
    lv_obj_remove_style(capture_btn_name, NULL, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_pad(capture_btn_name, 2, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_width(capture_btn_name, 2, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_color(capture_btn_name, lv_color_hex(EMBED_COLOR_FOCUS_ON), LV_STATE_FOCUS_KEY);
    lv_obj_add_event_cb(capture_btn_name, capture_btn_name_event, LV_EVENT_CLICKED, NULL);
    btn_label = lv_label_create(capture_btn_name);
    apply_text_color(btn_label);
    lv_obj_set_style_text_font(btn_label, FONT_BOLD_14, LV_PART_MAIN);
    lv_label_set_text(btn_label, "Name");
    lv_obj_center(btn_label);
    lv_group_add_obj(lv_group_get_default(), capture_btn_name);

    // Save button (row 2 right)
    capture_btn_save = lv_btn_create(scr7_4_cont);
    lv_obj_set_size(capture_btn_save, 100, 30);
    lv_obj_align(capture_btn_save, LV_ALIGN_TOP_RIGHT, -10, 135);
    lv_obj_set_style_border_color(capture_btn_save, lv_color_hex(EMBED_COLOR_BORDER), LV_PART_MAIN);
    lv_obj_set_style_border_width(capture_btn_save, 1, LV_PART_MAIN);
    apply_no_shadow(capture_btn_save);
    apply_bg_color(capture_btn_save);
    lv_obj_remove_style(capture_btn_save, NULL, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_pad(capture_btn_save, 2, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_width(capture_btn_save, 2, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_color(capture_btn_save, lv_color_hex(EMBED_COLOR_FOCUS_ON), LV_STATE_FOCUS_KEY);
    lv_obj_add_event_cb(capture_btn_save, capture_btn_save_event, LV_EVENT_CLICKED, NULL);
    btn_label = lv_label_create(capture_btn_save);
    apply_text_color(btn_label);
    lv_obj_set_style_text_font(btn_label, FONT_BOLD_14, LV_PART_MAIN);
    lv_label_set_text(btn_label, "Save");
    lv_obj_center(btn_label);
    lv_group_add_obj(lv_group_get_default(), capture_btn_save);

    // Back button
    capture_btn_back = scr_back_btn_create(scr7_4_cont, scr7_4_back_btn_event_cb);

    // Create update timer
    capture_update_timer = lv_timer_create(capture_update_timer_event, 100, NULL);
    lv_timer_pause(capture_update_timer);
}

static void enter7_4(void)
{
    entry7_4_anim(scr7_4_cont);
    lv_group_set_wrap(lv_group_get_default(), true);

    // Update UI based on mode and current signal
    if (capture_has_signal) {
        lv_label_set_text(capture_status_label, "Status: Ready");

        // Update protocol and data labels
        char buf[128];
        if (capture_current_signal.type == IR_SIGNAL_PARSED) {
            snprintf(buf, sizeof(buf), "Protocol: %s", capture_current_signal.protocol.c_str());
            lv_label_set_text(capture_protocol_label, buf);
            snprintf(buf, sizeof(buf), "Addr: 0x%X Cmd: 0x%X",
                     capture_current_signal.address, capture_current_signal.command);
            lv_label_set_text(capture_data_label, buf);
        } else {
            lv_label_set_text(capture_protocol_label, "Protocol: RAW");
            snprintf(buf, sizeof(buf), "Pulses: %d", capture_current_signal.raw_data.size());
            lv_label_set_text(capture_data_label, buf);
        }

        // Update button name label
        snprintf(buf, sizeof(buf), "Name: %s", capture_current_signal.name.c_str());
        lv_label_set_text(capture_name_label, buf);

        lv_obj_clear_state(capture_btn_test, LV_STATE_DISABLED);
        lv_obj_clear_state(capture_btn_save, LV_STATE_DISABLED);
    } else {
        lv_label_set_text(capture_status_label, "Status: Ready");
        lv_label_set_text(capture_protocol_label, "Protocol: ---");
        lv_label_set_text(capture_data_label, "Data: ---");
        lv_label_set_text(capture_name_label, "Name: ---");
        lv_obj_add_state(capture_btn_test, LV_STATE_DISABLED);
        lv_obj_add_state(capture_btn_save, LV_STATE_DISABLED);
    }

    // Remove all objects from group to clear ghost items from previous screen (screen7_3 list items)
    lv_group_t *g = lv_group_get_default();
    lv_group_remove_all_objs(g);

    // Re-add screen7_4 buttons to the group in the correct order
    lv_group_add_obj(g, capture_btn_back);      // Back button first
    lv_group_add_obj(g, capture_btn_capture);
    lv_group_add_obj(g, capture_btn_test);
    lv_group_add_obj(g, capture_btn_name);
    lv_group_add_obj(g, capture_btn_save);

    // Focus the first button (Capture)
    lv_group_focus_obj(capture_btn_capture);

    // Resume update timer
    if (capture_update_timer) {
        lv_timer_resume(capture_update_timer);
    }
}

static void exit7_4(void)
{
    lv_group_set_wrap(lv_group_get_default(), false);

    // Pause update timer
    if (capture_update_timer) {
        lv_timer_pause(capture_update_timer);
    }

    // Ensure IR receiver is disabled
    if (capture_is_capturing) {
        irrecv.disableIRIn();
        capture_is_capturing = false;
    }
}

static void destroy7_4(void)
{
    if (scr7_4_cont) {
        lv_obj_del(scr7_4_cont);
        scr7_4_cont = NULL;
    }
}

scr_lifecycle_t screen7_4 = {
    .create = create7_4,
    .entry = enter7_4,
    .exit = exit7_4,
    .destroy = destroy7_4,
};
#endif


// --------------------- screen 11 --------------------- MIC
#if 1
#include <arduinoFFT.h>

// Audio Spectrogram Configuration
#define AUDIO_FFT_SIZE 512              // FFT window size
#define AUDIO_CANVAS_WIDTH 320          // Full screen width (time axis)
#define AUDIO_CANVAS_HEIGHT 200         // Height (frequency axis) - reduced to avoid overlap
#define AUDIO_UPDATE_PERIOD_MS 50       // 20 Hz update rate
#define AUDIO_MIN_FREQ 20               // Minimum frequency (Hz)
#define AUDIO_MAX_FREQ 16000            // Maximum frequency (Hz)

// Audio spectrogram state
lv_obj_t *scr11_cont;
lv_obj_t *audio_spectrum_canvas = NULL;
lv_obj_t *audio_freq_label = NULL;       // Display frequency range
lv_obj_t *audio_amplitude_label = NULL;  // Display peak amplitude
lv_timer_t *audio_spectrum_timer = NULL;

// FFT and audio processing
ArduinoFFT<float> *audio_fft = NULL;
float *audio_vReal = NULL;               // FFT input (real values)
float *audio_vImag = NULL;               // FFT input (imaginary values)
uint8_t *audio_canvas_buffer = NULL;
int audio_current_column = 0;            // Current X position for drawing

void entry11_anim(lv_obj_t *obj) { entry1_anim(obj); }
void exit11_anim(int user_data, lv_obj_t *obj) { exit1_anim(user_data, obj); }

static void scr11_btn_event_cb(lv_event_t * e)
{
    if(e->code == LV_EVENT_CLICKED){
        exit11_anim(SCREEN0_ID, scr11_cont);
    }
}

// Read audio samples from I2S microphone
bool read_audio_samples(float *buffer, size_t num_samples) {
    size_t bytes_read = 0;
    int16_t *i2s_buffer = (int16_t *)malloc(num_samples * sizeof(int16_t));
    if (!i2s_buffer) {
        return false;
    }

    esp_err_t result = i2s_read(
        (i2s_port_t)EXAMPLE_I2S_CH,
        i2s_buffer,
        num_samples * sizeof(int16_t),
        &bytes_read,
        pdMS_TO_TICKS(100)
    );

    if (result == ESP_OK && bytes_read > 0) {
        size_t samples_read = bytes_read / sizeof(int16_t);
        // Convert int16_t to float and normalize to -1.0 to 1.0
        for (size_t i = 0; i < samples_read && i < num_samples; i++) {
            buffer[i] = (float)i2s_buffer[i] / 32768.0f;
        }
        free(i2s_buffer);
        return true;
    }
    free(i2s_buffer);
    return false;
}

// Convert FFT magnitude to color (heatmap style)
lv_color_t magnitude_to_color(float magnitude) {
    // Normalize magnitude with high sensitivity (lower divisor = more sensitive)
    float normalized = magnitude / 3.0f;  // Adjusted sensitivity - was 3.0f
    normalized = constrain(normalized, 0.0f, 1.0f);

    // Create a heatmap: black -> blue -> cyan -> green -> yellow -> red
    if (normalized < 0.2f) {
        // Black to blue
        uint8_t b = (uint8_t)(normalized * 5.0f * 255);
        return lv_color_make(0, 0, b);
    } else if (normalized < 0.4f) {
        // Blue to cyan
        uint8_t g = (uint8_t)((normalized - 0.2f) * 5.0f * 255);
        return lv_color_make(0, g, 255);
    } else if (normalized < 0.6f) {
        // Cyan to green
        uint8_t b = (uint8_t)((0.6f - normalized) * 5.0f * 255);
        return lv_color_make(0, 255, b);
    } else if (normalized < 0.8f) {
        // Green to yellow
        uint8_t r = (uint8_t)((normalized - 0.6f) * 5.0f * 255);
        return lv_color_make(r, 255, 0);
    } else {
        // Yellow to red
        uint8_t g = (uint8_t)((1.0f - normalized) * 5.0f * 255);
        return lv_color_make(255, g, 0);
    }
}

// Update spectrogram with waterfall display
void audio_spectrum_update_chart() {
    if (!audio_spectrum_canvas || !audio_fft || !audio_vReal || !audio_vImag) {
        return;
    }

    // Read audio samples
    if (!read_audio_samples(audio_vReal, AUDIO_FFT_SIZE)) {
        return;
    }

    // Clear imaginary array
    for (int i = 0; i < AUDIO_FFT_SIZE; i++) {
        audio_vImag[i] = 0.0f;
    }

    // Perform FFT
    audio_fft->windowing(FFTWindow::Hamming, FFTDirection::Forward);
    audio_fft->compute(FFTDirection::Forward);
    audio_fft->complexToMagnitude();

    // Calculate frequency bin indices for our range (20 Hz - 16 kHz)
    float sample_rate = 44100.0f;
    int min_bin = (int)(AUDIO_MIN_FREQ * AUDIO_FFT_SIZE / sample_rate);
    int max_bin = (int)(AUDIO_MAX_FREQ * AUDIO_FFT_SIZE / sample_rate);
    max_bin = constrain(max_bin, min_bin + 1, AUDIO_FFT_SIZE / 2);

    float max_amplitude = 0.0f;

    // Logarithmic frequency scaling
    float log_min = log10f(AUDIO_MIN_FREQ);
    float log_max = log10f(AUDIO_MAX_FREQ);
    float log_range = log_max - log_min;

    // Draw one vertical column of pixels representing current spectrum
    for (int y = 0; y < AUDIO_CANVAS_HEIGHT; y++) {
        // Bounds check for Y coordinate
        if (y < 0 || y >= AUDIO_CANVAS_HEIGHT) continue;

        // Map Y pixel to frequency bin using logarithmic scale
        // (bottom = low freq, top = high freq)
        int flipped_y = AUDIO_CANVAS_HEIGHT - 1 - y;
        float y_ratio = (float)flipped_y / (float)(AUDIO_CANVAS_HEIGHT - 1);

        // Logarithmic mapping: convert Y position to frequency
        float log_freq = log_min + (y_ratio * log_range);
        float freq = powf(10.0f, log_freq);

        // Convert frequency to FFT bin
        int bin = (int)(freq * AUDIO_FFT_SIZE / sample_rate);
        bin = constrain(bin, min_bin, max_bin - 1);

        // Get magnitude at this frequency
        float magnitude = audio_vReal[bin];
        if (magnitude > max_amplitude) {
            max_amplitude = magnitude;
        }

        // Convert to color and draw pixel - bounds check X coordinate too
        if (audio_current_column >= 0 && audio_current_column < AUDIO_CANVAS_WIDTH) {
            lv_color_t pixel_color = magnitude_to_color(magnitude);
            lv_canvas_set_px(audio_spectrum_canvas, audio_current_column, y, pixel_color);
        }
    }

    // Draw a black vertical line at next column to show cursor position
    // Make sure it's constrained to canvas dimensions only
    int next_column = (audio_current_column + 1) % AUDIO_CANVAS_WIDTH;
    if (next_column >= 0 && next_column < AUDIO_CANVAS_WIDTH) {
        for (int y = 0; y < AUDIO_CANVAS_HEIGHT; y++) {
            // Double-check Y bounds to prevent buffer overflow
            if (y >= 0 && y < AUDIO_CANVAS_HEIGHT) {
                lv_canvas_set_px(audio_spectrum_canvas, next_column, y, lv_color_hex(EMBED_COLOR_BG));
            }
        }
    }

    // Move to next column
    audio_current_column = next_column;

    // Update amplitude label
    if (audio_amplitude_label) {
        static char amp_buf[32];
        snprintf(amp_buf, sizeof(amp_buf), "Peak: %.1f", max_amplitude);
        lv_label_set_text(audio_amplitude_label, amp_buf);
    }

    lv_obj_invalidate(audio_spectrum_canvas);
}

// Timer callback for spectrum updates
static void audio_spectrum_timer_event(lv_timer_t *timer) {
    audio_spectrum_update_chart();
}

void create11(lv_obj_t *parent)
{
    scr11_cont = create_screen_container(parent);

    // Title
    lv_obj_t *label = lv_label_create(scr11_cont);
    apply_text_color(label);
    lv_obj_set_style_text_font(label, FONT_BOLD_16, LV_PART_MAIN);
    lv_label_set_text(label, "Audio Spectrum");
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 10);

    // Allocate canvas buffer
    audio_canvas_buffer = (uint8_t *)malloc(LV_CANVAS_BUF_SIZE_TRUE_COLOR(AUDIO_CANVAS_WIDTH, AUDIO_CANVAS_HEIGHT));

    if (audio_canvas_buffer) {
        // Create canvas for spectrum display (moved up 15 pixels from 60 to 45)
        audio_spectrum_canvas = lv_canvas_create(scr11_cont);
        lv_canvas_set_buffer(audio_spectrum_canvas, audio_canvas_buffer,
                            AUDIO_CANVAS_WIDTH, AUDIO_CANVAS_HEIGHT,
                            LV_IMG_CF_TRUE_COLOR);
        lv_obj_align(audio_spectrum_canvas, LV_ALIGN_TOP_MID, 0, 45);
        lv_canvas_fill_bg(audio_spectrum_canvas, lv_color_hex(EMBED_COLOR_BG), LV_OPA_COVER);
    }

    // Frequency range label (bottom left)
    audio_freq_label = lv_label_create(scr11_cont);
    apply_text_color(audio_freq_label);
    lv_obj_set_style_text_font(audio_freq_label, FONT_BOLD_14, LV_PART_MAIN);
    lv_label_set_text(audio_freq_label, "20 Hz - 16 kHz");
    lv_obj_align(audio_freq_label, LV_ALIGN_BOTTOM_LEFT, 10, -5);

    // Amplitude label (bottom right)
    audio_amplitude_label = lv_label_create(scr11_cont);
    apply_text_color(audio_amplitude_label);
    lv_obj_set_style_text_font(audio_amplitude_label, FONT_BOLD_14, LV_PART_MAIN);
    lv_label_set_text(audio_amplitude_label, "Peak: 0.0");
    lv_obj_align(audio_amplitude_label, LV_ALIGN_BOTTOM_RIGHT, -10, -5);

    // Allocate FFT buffers
    audio_vReal = (float *)malloc(AUDIO_FFT_SIZE * sizeof(float));
    audio_vImag = (float *)malloc(AUDIO_FFT_SIZE * sizeof(float));

    if (audio_vReal && audio_vImag) {
        audio_fft = new ArduinoFFT<float>(audio_vReal, audio_vImag, AUDIO_FFT_SIZE, 44100);

        // Initialize arrays
        for (int i = 0; i < AUDIO_FFT_SIZE; i++) {
            audio_vReal[i] = 0.0f;
            audio_vImag[i] = 0.0f;
        }
    }

    // Create update timer (paused initially)
    if (!audio_spectrum_timer) {
        audio_spectrum_timer = lv_timer_create(audio_spectrum_timer_event, AUDIO_UPDATE_PERIOD_MS, NULL);
        lv_timer_pause(audio_spectrum_timer);
    }

    // Back button
    scr_back_btn_create(scr11_cont, scr11_btn_event_cb);
}
void entry11(void) {
    entry11_anim(scr11_cont);

    // Reset column position to start from left
    audio_current_column = 0;

    // Clear canvas
    if (audio_spectrum_canvas) {
        lv_canvas_fill_bg(audio_spectrum_canvas, lv_color_hex(EMBED_COLOR_BG), LV_OPA_COVER);
    }

    // Resume spectrum update timer
    if (audio_spectrum_timer) {
        lv_timer_resume(audio_spectrum_timer);
    }
}

void exit11(void) {
    // Pause spectrum update timer
    if (audio_spectrum_timer) {
        lv_timer_pause(audio_spectrum_timer);
    }
}

void destroy11(void) {
    // Delete timer
    if (audio_spectrum_timer) {
        lv_timer_del(audio_spectrum_timer);
        audio_spectrum_timer = NULL;
    }

    // Free FFT resources
    if (audio_fft) {
        delete audio_fft;
        audio_fft = NULL;
    }
    if (audio_vReal) {
        free(audio_vReal);
        audio_vReal = NULL;
    }
    if (audio_vImag) {
        free(audio_vImag);
        audio_vImag = NULL;
    }

    // Free canvas buffer
    if (audio_canvas_buffer) {
        free(audio_canvas_buffer);
        audio_canvas_buffer = NULL;
    }

    audio_spectrum_canvas = NULL;
    audio_freq_label = NULL;
    audio_amplitude_label = NULL;
}

scr_lifecycle_t screen11 = {
    .create = create11,
    .entry = entry11,
    .exit  = exit11,
    .destroy = destroy11,
};
#endif
// --------------------- screen 10 f--------------------- SD
#if 1
lv_obj_t *scr10_cont;
lv_obj_t *sd_err_info;
lv_obj_t *sd_slider;
lv_obj_t *sd_total;
lv_obj_t *sd_used;
lv_obj_t *sd_mount_btn;
lv_obj_t *sd_unmount_btn;
lv_obj_t *sd_browse_btn;
lv_obj_t *sd_status_label;

void entry10_anim(lv_obj_t *obj) { entry1_anim(obj); }
void exit10_anim(int user_data, lv_obj_t *obj) { exit1_anim(user_data, obj); }

void entry10(void);  // Forward declaration

static void sd_mount_event_cb(lv_event_t * e)
{
    if(e->code == LV_EVENT_CLICKED){
        if(sd_mount()) {
            // Refresh UI after successful mount
            entry10();
        } else {
            lv_label_set_text(sd_err_info, "Failed to mount SD card");
            lv_obj_clear_flag(sd_err_info, LV_OBJ_FLAG_HIDDEN);
            lv_obj_center(sd_err_info);
        }
    }
}

static void sd_unmount_event_cb(lv_event_t * e)
{
    if(e->code == LV_EVENT_CLICKED){
        sd_unmount();
        // Refresh UI after unmount
        entry10();
    }
}

static void sd_browse_event_cb(lv_event_t * e)
{
    if(e->code == LV_EVENT_CLICKED){
        if(sd_is_valid()) {
            scr_mgr_switch(SCREEN10_1_ID, false);
        }
    }
}

static void scr10_btn_event_cb(lv_event_t * e)
{
    if(e->code == LV_EVENT_CLICKED){
        exit10_anim(SCREEN0_ID, scr10_cont);
    }
}

void create10(lv_obj_t *parent) 
{
    scr10_cont = create_screen_container(parent);

    lv_obj_t *label = lv_label_create(scr10_cont);
    apply_text_color(label);
    lv_obj_set_style_text_font(label, FONT_BOLD_16, LV_PART_MAIN);
    lv_label_set_text(label, "SD Card");
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 10);

    sd_err_info = lv_label_create(scr10_cont);
    lv_obj_set_width(sd_err_info, DISPALY_WIDTH * 0.9);
    apply_text_color(sd_err_info);
    lv_obj_set_style_text_font(sd_err_info, FONT_BOLD_14, LV_PART_MAIN);
    lv_label_set_long_mode(sd_err_info, LV_LABEL_LONG_WRAP);

    static lv_style_t style_bg;
    static lv_style_t style_indic;
    lv_style_init(&style_bg);
    lv_style_set_border_color(&style_bg, lv_color_hex(EMBED_COLOR_FOCUS_ON));
    lv_style_set_border_width(&style_bg, 2);
    lv_style_set_pad_all(&style_bg, 6); /*To make the indicator smaller*/
    lv_style_set_radius(&style_bg, 6);
    lv_style_set_anim_time(&style_bg, 500);
    lv_style_init(&style_indic);
    lv_style_set_bg_opa(&style_indic, LV_OPA_COVER);
    lv_style_set_bg_color(&style_indic, lv_color_hex(EMBED_COLOR_FOCUS_ON));
    lv_style_set_radius(&style_indic, 3);

    sd_slider = lv_bar_create(scr10_cont);
    lv_obj_remove_style_all(sd_slider);  /*To have a clean start*/
    lv_obj_add_style(sd_slider, &style_bg, 0);
    lv_obj_add_style(sd_slider, &style_indic, LV_PART_INDICATOR);
    lv_obj_set_size(sd_slider, DISPALY_WIDTH * 0.85, 20);
    lv_obj_align(sd_slider, LV_ALIGN_CENTER, 0, -10);

    sd_used = lv_label_create(scr10_cont);
    lv_obj_set_width(sd_used, DISPALY_WIDTH * 0.42);
    apply_text_color(sd_used);
    lv_obj_set_style_text_font(sd_used, FONT_BOLD_14, LV_PART_MAIN);
    lv_label_set_long_mode(sd_used, LV_LABEL_LONG_CLIP);
    lv_obj_set_style_text_align(sd_used, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
    lv_obj_align_to(sd_used, sd_slider, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 8);

    sd_total = lv_label_create(scr10_cont);
    lv_obj_set_width(sd_total, DISPALY_WIDTH * 0.42);
    apply_text_color(sd_total);
    lv_obj_set_style_text_font(sd_total, FONT_BOLD_14, LV_PART_MAIN);
    lv_label_set_long_mode(sd_total, LV_LABEL_LONG_CLIP);
    lv_obj_set_style_text_align(sd_total, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN);
    lv_obj_align_to(sd_total, sd_slider, LV_ALIGN_OUT_BOTTOM_RIGHT, 0, 8);

    // Status label (positioned below title)
    sd_status_label = lv_label_create(scr10_cont);
    apply_text_color(sd_status_label);
    lv_obj_set_style_text_font(sd_status_label, FONT_BOLD_14, LV_PART_MAIN);
    lv_obj_align(sd_status_label, LV_ALIGN_TOP_MID, 0, 45);

    // Mount button (lower left corner when unmounted)
    sd_mount_btn = lv_btn_create(scr10_cont);
    lv_obj_set_size(sd_mount_btn, 90, 32);
    lv_obj_align(sd_mount_btn, LV_ALIGN_BOTTOM_LEFT, 8, -8);
    apply_bg_color(sd_mount_btn);
    lv_obj_set_style_radius(sd_mount_btn, 5, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_color(sd_mount_btn, lv_color_hex(EMBED_COLOR_FOCUS_ON), LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_width(sd_mount_btn, 2, LV_STATE_FOCUS_KEY);
    lv_obj_t *mount_lab = lv_label_create(sd_mount_btn);
    lv_label_set_text(mount_lab, "Mount");
    apply_text_color(mount_lab);
    lv_obj_set_style_text_font(mount_lab, FONT_BOLD_14, LV_PART_MAIN);
    lv_obj_center(mount_lab);
    lv_obj_add_event_cb(sd_mount_btn, sd_mount_event_cb, LV_EVENT_CLICKED, NULL);

    // Browse button (lower right corner when mounted) - created first for rolling menu
    sd_browse_btn = lv_btn_create(scr10_cont);
    lv_obj_set_size(sd_browse_btn, 90, 32);
    lv_obj_align(sd_browse_btn, LV_ALIGN_BOTTOM_RIGHT, -8, -8);
    apply_bg_color(sd_browse_btn);
    lv_obj_set_style_radius(sd_browse_btn, 5, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_color(sd_browse_btn, lv_color_hex(EMBED_COLOR_FOCUS_ON), LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_width(sd_browse_btn, 2, LV_STATE_FOCUS_KEY);
    lv_obj_t *browse_lab = lv_label_create(sd_browse_btn);
    lv_label_set_text(browse_lab, "Browse");
    apply_text_color(browse_lab);
    lv_obj_set_style_text_font(browse_lab, FONT_BOLD_14, LV_PART_MAIN);
    lv_obj_center(browse_lab);
    lv_obj_add_event_cb(sd_browse_btn, sd_browse_event_cb, LV_EVENT_CLICKED, NULL);

    // Unmount button (lower left corner when mounted) - created second for rolling menu
    sd_unmount_btn = lv_btn_create(scr10_cont);
    lv_obj_set_size(sd_unmount_btn, 90, 32);
    lv_obj_align(sd_unmount_btn, LV_ALIGN_BOTTOM_LEFT, 8, -8);
    apply_bg_color(sd_unmount_btn);
    lv_obj_set_style_radius(sd_unmount_btn, 5, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_color(sd_unmount_btn, lv_color_hex(EMBED_COLOR_FOCUS_ON), LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_width(sd_unmount_btn, 2, LV_STATE_FOCUS_KEY);
    lv_obj_t *unmount_lab = lv_label_create(sd_unmount_btn);
    lv_label_set_text(unmount_lab, "Unmount");
    apply_text_color(unmount_lab);
    lv_obj_set_style_text_font(unmount_lab, FONT_BOLD_14, LV_PART_MAIN);
    lv_obj_center(unmount_lab);
    lv_obj_add_event_cb(sd_unmount_btn, sd_unmount_event_cb, LV_EVENT_CLICKED, NULL);

    // back bottom
    scr_back_btn_create(scr10_cont, scr10_btn_event_cb);
}
void entry10(void) {
    uint32_t total = sd_get_sum_Mbyte();
    uint32_t used = sd_get_used_Mbyte();
    uint32_t usage_percentage = 1;

    entry10_anim(scr10_cont);
    lv_group_set_wrap(lv_group_get_default(), true);

    if(sd_is_valid()) {
        // SD card is mounted
        lv_label_set_text(sd_status_label, "Status: Mounted");
        lv_label_set_text_fmt(sd_total, "%dM total", total);
        lv_label_set_text_fmt(sd_used, "%dM used", used);

        if(used > total / 100) {
            usage_percentage = lv_map(used, 0, total, 0, 100);
        }
        lv_bar_set_value(sd_slider, usage_percentage, LV_ANIM_ON);

        // Show capacity info
        lv_obj_clear_flag(sd_status_label, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(sd_slider, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(sd_total, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(sd_used, LV_OBJ_FLAG_HIDDEN);

        // Show unmount and browse buttons, hide mount button
        lv_obj_add_flag(sd_mount_btn, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(sd_unmount_btn, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(sd_browse_btn, LV_OBJ_FLAG_HIDDEN);

        // Hide error message
        lv_obj_add_flag(sd_err_info, LV_OBJ_FLAG_HIDDEN);
    } else {
        // SD card is not mounted
        lv_label_set_text(sd_status_label, "Status: Not Mounted");
        lv_label_set_text(sd_err_info, "Insert SD card");
        lv_obj_align(sd_err_info, LV_ALIGN_CENTER, 0, 5);

        // Show status label and error message
        lv_obj_clear_flag(sd_status_label, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(sd_err_info, LV_OBJ_FLAG_HIDDEN);

        // Hide capacity info
        lv_obj_add_flag(sd_slider, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(sd_total, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(sd_used, LV_OBJ_FLAG_HIDDEN);

        // Show mount button, hide unmount and browse buttons
        lv_obj_clear_flag(sd_mount_btn, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(sd_unmount_btn, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(sd_browse_btn, LV_OBJ_FLAG_HIDDEN);
    }
}
void exit10(void) {
    lv_group_set_wrap(lv_group_get_default(), false);
}
void destroy10(void) {}

scr_lifecycle_t screen10 = {
    .create = create10,
    .entry = entry10,
    .exit  = exit10,
    .destroy = destroy10,
};
#endif
// --------------------- screen 11.1 --------------------- File Browser
#if 1
#include "Audio.h"
extern Audio audio;

lv_obj_t *scr10_1_cont;
lv_obj_t *fb_path_label;
lv_obj_t *fb_file_list;
lv_obj_t *fb_led;
lv_obj_t *fb_context_menu = NULL;  // Context menu for file operations
lv_obj_t *fb_keyboard = NULL;      // Keyboard for text input
lv_obj_t *fb_rename_ta = NULL;     // Text area for renaming
lv_obj_t *fb_move_ta = NULL;       // Text area for move/copy destination
lv_obj_t *fb_action_menu = NULL;   // Action menu for Move/Copy choice
char fb_current_path[256] = "/";
char fb_selected_item[128] = "";   // Currently selected file/folder
char fb_destination_path[256] = ""; // Destination path for move/copy
char fb_playing_file[512] = "";     // Currently playing MP3 file path
bool fb_selected_is_dir = false;   // Is selected item a directory?
bool fb_long_press_occurred = false; // Flag to prevent CLICKED after LONG_PRESSED
char fb_working_mount[64] = "/sd";  // SD card mount point (detected during directory loading)

void entry10_1_anim(lv_obj_t *obj) { entry1_anim(obj); }
void exit10_1_anim(int user_data, lv_obj_t *obj) { exit1_anim(user_data, obj); }

void fb_load_directory(const char *path);

// Forward declarations for context menu callbacks
static void fb_delete_file(void);
static void fb_rename_file(void);
static void fb_move_copy_file(void);
static void fb_keyboard_event(lv_event_t *e);
static void fb_move_copy_keyboard_event(lv_event_t *e);
static void fb_move_copy_action_event(lv_event_t *e);

// Forward declaration for audio playback screen variable
extern char fb_audio_playing_file[];

// Context menu event handler
static void fb_context_menu_event(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *mbox = lv_event_get_current_target(e);

    if(code == LV_EVENT_VALUE_CHANGED) {
        const char *txt = lv_msgbox_get_active_btn_text(mbox);

        if(txt) {

            if(strcmp(txt, "Del") == 0) {
                fb_delete_file();
            } else if(strcmp(txt, "Name") == 0) {
                fb_rename_file();
            } else if(strcmp(txt, "Cp/Mv") == 0) {
                fb_move_copy_file();
            }
            // No "Cancel" button anymore - user can use X button instead
        }

        // Close context menu and restore navigation
        lv_group_t *g = lv_group_get_default();

        // Restore navigation state before closing msgbox
        lv_group_set_editing(g, false);
        lv_group_focus_freeze(g, false);

        // Close the msgbox (this will remove its buttons from group automatically)
        if(fb_context_menu) {
            lv_msgbox_close(fb_context_menu);
            fb_context_menu = NULL;

            // Force LVGL refresh to ensure msgbox is fully cleaned up
            lv_refr_now(NULL);
        }

        // Ensure the group has a focused object (prevents dangling references)
        if(lv_group_get_focused(g) == NULL && lv_obj_get_child_cnt(fb_file_list) > 0) {
            lv_obj_t *first_item = lv_obj_get_child(fb_file_list, 0);
            if(first_item) {
                lv_group_focus_obj(first_item);
            }
        }
    }
}

// Delete file/folder implementation
static void fb_delete_file(void)
{
    if(strlen(fb_selected_item) == 0) return;

    // Build the path relative to SD card root (NOT filesystem mount point)
    // SD library expects paths like "/music/file.mp3", not "/sd/music/file.mp3"
    char sd_path[512];
    if(strcmp(fb_current_path, "/") == 0) {
        snprintf(sd_path, sizeof(sd_path), "/%s", fb_selected_item);
    } else {
        snprintf(sd_path, sizeof(sd_path), "%s/%s", fb_current_path, fb_selected_item);
    }


    bool success = false;
    if(fb_selected_is_dir) {
        success = SD.rmdir(sd_path);
    } else {
        success = SD.remove(sd_path);
    }

    if(success) {
        prompt_info("  Deleted successfully", 1500);
        fb_load_directory(fb_current_path);  // Refresh list
    } else {
        if(fb_selected_is_dir) {
            prompt_info("  Delete failed!\n  (Dir not empty?)", 2000);
        } else {
            prompt_info("  Delete failed!", 2000);
        }
    }

    fb_selected_item[0] = '\0';  // Clear selection
}

// Keyboard event handler for rename
static void fb_keyboard_event(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);

    if(code == LV_EVENT_READY) {
        // User pressed OK - perform rename
        const char *new_name = lv_textarea_get_text(fb_rename_ta);

        if(strlen(new_name) > 0 && strlen(fb_selected_item) > 0) {
            // Build paths relative to SD card root (NOT filesystem mount point)
            // SD library expects paths like "/music/file.mp3", not "/sd/music/file.mp3"
            char old_path[512], new_path[512];
            if(strcmp(fb_current_path, "/") == 0) {
                snprintf(old_path, sizeof(old_path), "/%s", fb_selected_item);
                snprintf(new_path, sizeof(new_path), "/%s", new_name);
            } else {
                snprintf(old_path, sizeof(old_path), "%s/%s", fb_current_path, fb_selected_item);
                snprintf(new_path, sizeof(new_path), "%s/%s", fb_current_path, new_name);
            }


            if(SD.rename(old_path, new_path)) {
                prompt_info("  Renamed successfully", 1500);
                fb_load_directory(fb_current_path);  // Refresh list
            } else {
                prompt_info("  Rename failed!", 2000);
            }
        }

        // Clean up keyboard and textarea
        lv_group_t *g = lv_group_get_default();
        lv_group_set_editing(g, false);
        lv_group_focus_freeze(g, false);

        if(fb_keyboard) {
            lv_obj_del(fb_keyboard);
            fb_keyboard = NULL;
        }
        if(fb_rename_ta) {
            lv_obj_del(fb_rename_ta);
            fb_rename_ta = NULL;
        }

        // Force refresh and refocus
        lv_refr_now(NULL);
        if(lv_group_get_focused(g) == NULL && lv_obj_get_child_cnt(fb_file_list) > 0) {
            lv_obj_t *first_item = lv_obj_get_child(fb_file_list, 0);
            if(first_item) {
                lv_group_focus_obj(first_item);
            }
        }

        fb_selected_item[0] = '\0';
    } else if(code == LV_EVENT_CANCEL) {
        // User cancelled - cleanup keyboard and textarea
        lv_group_t *g = lv_group_get_default();
        lv_group_set_editing(g, false);
        lv_group_focus_freeze(g, false);

        if(fb_keyboard) {
            lv_obj_del(fb_keyboard);
            fb_keyboard = NULL;
        }
        if(fb_rename_ta) {
            lv_obj_del(fb_rename_ta);
            fb_rename_ta = NULL;
        }

        // Force refresh and refocus
        lv_refr_now(NULL);
        if(lv_group_get_focused(g) == NULL && lv_obj_get_child_cnt(fb_file_list) > 0) {
            lv_obj_t *first_item = lv_obj_get_child(fb_file_list, 0);
            if(first_item) {
                lv_group_focus_obj(first_item);
            }
        }

        fb_selected_item[0] = '\0';
    }
}

// Show rename keyboard
static void fb_rename_file(void)
{
    if(strlen(fb_selected_item) == 0) return;

    // Create text area for input
    fb_rename_ta = lv_textarea_create(lv_scr_act());
    lv_obj_set_size(fb_rename_ta, LV_PCT(90), 40);
    lv_obj_align(fb_rename_ta, LV_ALIGN_TOP_MID, 0, 40);
    lv_textarea_set_one_line(fb_rename_ta, true);
    lv_textarea_set_placeholder_text(fb_rename_ta, "Enter new name");
    lv_textarea_set_text(fb_rename_ta, fb_selected_item);  // Pre-fill with current name

    // Enable and style the cursor for visibility
    lv_obj_set_style_anim_time(fb_rename_ta, 400, LV_PART_CURSOR);  // Blinking cursor
    lv_obj_set_style_bg_opa(fb_rename_ta, LV_OPA_COVER, LV_PART_CURSOR);
    lv_obj_set_style_bg_color(fb_rename_ta, lv_color_hex(EMBED_COLOR_FOCUS_ON), LV_PART_CURSOR);
    lv_obj_set_style_border_width(fb_rename_ta, 1, LV_PART_CURSOR);
    lv_obj_set_style_border_color(fb_rename_ta, lv_color_hex(EMBED_COLOR_FOCUS_ON), LV_PART_CURSOR);

    // Position cursor at end of text
    lv_textarea_set_cursor_pos(fb_rename_ta, LV_TEXTAREA_CURSOR_LAST);

    // Create keyboard
    fb_keyboard = lv_keyboard_create(lv_scr_act());
    lv_keyboard_set_textarea(fb_keyboard, fb_rename_ta);
    lv_obj_set_size(fb_keyboard, LV_HOR_RES, LV_VER_RES / 2);
    lv_obj_align(fb_keyboard, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_add_event_cb(fb_keyboard, fb_keyboard_event, LV_EVENT_ALL, NULL);

    // Focus keyboard and enable editing mode
    lv_group_t *g = lv_group_get_default();
    lv_group_focus_freeze(g, true);
    lv_group_focus_obj(fb_keyboard);
    lv_group_set_editing(g, true);  // Enable editing mode for immediate key selection
}

// Move/Copy keyboard event handler
static void fb_move_copy_keyboard_event(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);

    if(code == LV_EVENT_READY) {
        // User pressed OK - save destination path and show Move/Copy choice
        const char *dest = lv_textarea_get_text(fb_move_ta);

        if(strlen(dest) > 0) {
            strncpy(fb_destination_path, dest, sizeof(fb_destination_path) - 1);
            fb_destination_path[sizeof(fb_destination_path) - 1] = '\0';


            // Close keyboard and text area
            lv_group_t *g = lv_group_get_default();
            lv_group_focus_freeze(g, false);

            if(fb_keyboard) {
                lv_obj_del(fb_keyboard);
                fb_keyboard = NULL;
            }
            if(fb_move_ta) {
                lv_obj_del(fb_move_ta);
                fb_move_ta = NULL;
            }

            // Show Move/Copy action menu
            static const char *action_btns[] = {"Move", "Copy", "Cancel", ""};
            fb_action_menu = lv_msgbox_create(NULL, "Choose Action", "Move or Copy to destination?", action_btns, true);
            lv_obj_add_event_cb(fb_action_menu, fb_move_copy_action_event, LV_EVENT_VALUE_CHANGED, NULL);
            lv_obj_center(fb_action_menu);

            // Focus on the action menu buttons
            lv_obj_t *btnm = lv_msgbox_get_btns(fb_action_menu);
            lv_group_focus_obj(btnm);
            lv_obj_add_state(btnm, LV_STATE_FOCUS_KEY);
            lv_group_set_editing(g, true);
            lv_group_focus_freeze(g, true);
        }
    } else if(code == LV_EVENT_CANCEL) {
        // User pressed Cancel - close keyboard
        lv_group_t *g = lv_group_get_default();
        lv_group_focus_freeze(g, false);

        if(fb_keyboard) {
            lv_obj_del(fb_keyboard);
            fb_keyboard = NULL;
        }
        if(fb_move_ta) {
            lv_obj_del(fb_move_ta);
            fb_move_ta = NULL;
        }

        // Force refresh and refocus
        lv_refr_now(NULL);
        if(lv_group_get_focused(g) == NULL && lv_obj_get_child_cnt(fb_file_list) > 0) {
            lv_obj_t *first_item = lv_obj_get_child(fb_file_list, 0);
            if(first_item) {
                lv_group_focus_obj(first_item);
            }
        }

        fb_selected_item[0] = '\0';
        fb_destination_path[0] = '\0';
    }
}

// Show move/copy destination input
static void fb_move_copy_file(void)
{
    if(strlen(fb_selected_item) == 0) return;

    // Create text area for destination path input
    fb_move_ta = lv_textarea_create(lv_scr_act());
    lv_obj_set_size(fb_move_ta, LV_PCT(90), 40);
    lv_obj_align(fb_move_ta, LV_ALIGN_TOP_MID, 0, 40);
    lv_textarea_set_one_line(fb_move_ta, true);
    lv_textarea_set_placeholder_text(fb_move_ta, "Enter destination path");
    lv_textarea_set_text(fb_move_ta, fb_current_path);  // Pre-fill with current path

    // Enable and style the cursor for visibility
    lv_obj_set_style_anim_time(fb_move_ta, 400, LV_PART_CURSOR);
    lv_obj_set_style_bg_opa(fb_move_ta, LV_OPA_COVER, LV_PART_CURSOR);
    lv_obj_set_style_bg_color(fb_move_ta, lv_color_hex(EMBED_COLOR_FOCUS_ON), LV_PART_CURSOR);
    lv_obj_set_style_border_width(fb_move_ta, 1, LV_PART_CURSOR);
    lv_obj_set_style_border_color(fb_move_ta, lv_color_hex(EMBED_COLOR_FOCUS_ON), LV_PART_CURSOR);

    // Position cursor at end of text
    lv_textarea_set_cursor_pos(fb_move_ta, LV_TEXTAREA_CURSOR_LAST);

    // Create keyboard
    fb_keyboard = lv_keyboard_create(lv_scr_act());
    lv_keyboard_set_textarea(fb_keyboard, fb_move_ta);
    lv_obj_set_size(fb_keyboard, LV_HOR_RES, LV_VER_RES / 2);
    lv_obj_align(fb_keyboard, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_add_event_cb(fb_keyboard, fb_move_copy_keyboard_event, LV_EVENT_ALL, NULL);

    // Freeze group and focus on keyboard
    lv_group_t *g = lv_group_get_default();
    lv_group_focus_freeze(g, true);
    lv_group_focus_obj(fb_keyboard);
}

// Action menu event handler for Move/Copy choice
static void fb_move_copy_action_event(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *mbox = lv_event_get_current_target(e);

    if(code == LV_EVENT_VALUE_CHANGED) {
        const char *txt = lv_msgbox_get_active_btn_text(mbox);

        if(txt) {

            if(strcmp(txt, "Move") == 0 || strcmp(txt, "Copy") == 0) {
                bool is_move = (strcmp(txt, "Move") == 0);

                // Build source and destination paths
                char src_path[512], dest_path[512];
                if(strcmp(fb_current_path, "/") == 0) {
                    snprintf(src_path, sizeof(src_path), "/%s", fb_selected_item);
                } else {
                    snprintf(src_path, sizeof(src_path), "%s/%s", fb_current_path, fb_selected_item);
                }

                if(strcmp(fb_destination_path, "/") == 0) {
                    snprintf(dest_path, sizeof(dest_path), "/%s", fb_selected_item);
                } else {
                    snprintf(dest_path, sizeof(dest_path), "%s/%s", fb_destination_path, fb_selected_item);
                }


                bool success = false;
                if(is_move) {
                    // Use rename for move operation
                    success = SD.rename(src_path, dest_path);
                    if(success) {
                        prompt_info("  Moved successfully", 1500);
                    } else {
                        prompt_info("  Move failed!", 2000);
                    }
                } else {
                    // Copy operation - need to read and write
                    File src = SD.open(src_path);
                    if(src) {
                        if(fb_selected_is_dir) {
                            src.close();
                            prompt_info("  Copy dir not supported", 2000);
                        } else {
                            File dest = SD.open(dest_path, FILE_WRITE);
                            if(dest) {
                                uint8_t buf[512];
                                size_t len;
                                while((len = src.read(buf, sizeof(buf))) > 0) {
                                    dest.write(buf, len);
                                    // Feed watchdog to prevent timeout on large files
                                    delay(1);
                                }
                                dest.close();
                                success = true;
                                prompt_info("  Copied successfully", 1500);
                            } else {
                                prompt_info("  Copy failed!", 2000);
                            }
                            src.close();
                        }
                    } else {
                        prompt_info("  Copy failed!", 2000);
                    }
                }

                if(success) {
                    fb_load_directory(fb_current_path);  // Refresh list
                }
            }
        }

        // Close action menu and restore navigation
        lv_group_t *g = lv_group_get_default();
        lv_group_set_editing(g, false);
        lv_group_focus_freeze(g, false);

        if(fb_action_menu) {
            lv_msgbox_close(fb_action_menu);
            fb_action_menu = NULL;
            lv_refr_now(NULL);
        }

        // Ensure the group has a focused object
        if(lv_group_get_focused(g) == NULL && lv_obj_get_child_cnt(fb_file_list) > 0) {
            lv_obj_t *first_item = lv_obj_get_child(fb_file_list, 0);
            if(first_item) {
                lv_group_focus_obj(first_item);
            }
        }

        fb_selected_item[0] = '\0';
        fb_destination_path[0] = '\0';
    }
}

static void fb_list_event(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *obj = lv_event_get_target(e);

    if(code == LV_EVENT_PRESSED) {
        // Reset flag on new press
        fb_long_press_occurred = false;
    } else if(code == LV_EVENT_LONG_PRESSED) {
        // Set flag to prevent CLICKED from firing
        fb_long_press_occurred = true;
        // Long press - show context menu
        const char *item_text = lv_list_get_btn_text(fb_file_list, obj);

        if(item_text && strcmp(item_text, " .. (Parent)") != 0) {
            // Extract filename/foldername (no icons now)
            const char *name_with_size = item_text + 1; // Skip leading space

            // Extract name (before size info if present)
            char name[128];
            strncpy(name, name_with_size, sizeof(name) - 1);
            name[sizeof(name) - 1] = '\0';
            char *size_start = strstr(name, "  ");
            if(size_start) *size_start = '\0';

            // Determine if it's a directory by testing the path
            char test_path[512];
            if(strcmp(fb_current_path, "/") == 0) {
                snprintf(test_path, sizeof(test_path), "/%s", name);
            } else {
                snprintf(test_path, sizeof(test_path), "%s/%s", fb_current_path, name);
            }

            DIR *test_dir = opendir(test_path);
            if(test_dir) {
                closedir(test_dir);
                fb_selected_is_dir = true;
            } else {
                fb_selected_is_dir = false;
            }

            strncpy(fb_selected_item, name, sizeof(fb_selected_item) - 1);
            fb_selected_item[sizeof(fb_selected_item) - 1] = '\0';

            static const char *btns[] = {"Del", "Name", "Cp/Mv", "Cancel", ""};
            fb_context_menu = lv_msgbox_create(NULL, "File Options", fb_selected_item, btns, false);  // false = no X button
            lv_obj_add_event_cb(fb_context_menu, fb_context_menu_event, LV_EVENT_VALUE_CHANGED, NULL);
            lv_obj_center(fb_context_menu);

            // Get the button matrix from msgbox
            lv_obj_t *btnm = lv_msgbox_get_btns(fb_context_menu);
            lv_group_t *g = lv_group_get_default();

            if(btnm) {
                lv_group_add_obj(g, btnm);
                lv_group_focus_obj(btnm);
                lv_obj_add_state(btnm, LV_STATE_FOCUS_KEY);
                lv_group_set_editing(g, true);
            }

            // Freeze the group to prevent background widgets from receiving input
            lv_group_focus_freeze(g, true);
        }
    } else if(code == LV_EVENT_CLICKED) {
        // Ignore CLICKED if it followed a LONG_PRESSED
        if(fb_long_press_occurred) {
            fb_long_press_occurred = false;
            return;
        }

        const char *item_text = lv_list_get_btn_text(fb_file_list, obj);

        if(item_text) {
            // Check if it's the parent directory entry
            if(strcmp(item_text, " .. (Parent)") == 0) {
                // Navigate to parent directory
                char *last_slash = strrchr(fb_current_path, '/');
                if(last_slash && last_slash != fb_current_path) {
                    *last_slash = '\0';
                } else if(strcmp(fb_current_path, "/") != 0) {
                    strcpy(fb_current_path, "/");
                }
                fb_load_directory(fb_current_path);
            } else {
                // Extract name (no icons)
                const char *name_with_size = item_text + 1; // Skip leading space

                // Extract just the filename/foldername (before the size info)
                char name[128];
                strncpy(name, name_with_size, sizeof(name) - 1);
                name[sizeof(name) - 1] = '\0';
                char *size_start = strstr(name, "  "); // Find double space before size
                if(size_start) {
                    *size_start = '\0'; // Truncate at size info
                }

                // Test if it's a directory (use fb_working_mount prefix for proper SD card access)
                char test_path[512];
                if(strcmp(fb_current_path, "/") == 0) {
                    snprintf(test_path, sizeof(test_path), "%s/%s", fb_working_mount, name);
                } else {
                    snprintf(test_path, sizeof(test_path), "%s%s/%s", fb_working_mount, fb_current_path, name);
                }

                DIR *test_dir = opendir(test_path);
                if(test_dir) {
                    // It's a folder, navigate into it
                    closedir(test_dir);
                    if(strcmp(fb_current_path, "/") == 0) {
                        snprintf(fb_current_path, sizeof(fb_current_path), "/%s", name);
                    } else {
                        snprintf(fb_current_path, sizeof(fb_current_path), "%s/%s", fb_current_path, name);
                    }
                    fb_load_directory(fb_current_path);
                } else {
                    // It's a file, handle it
                    char file_name[128];
                    strncpy(file_name, name, sizeof(file_name));

                    char full_path[512];
                    if(strcmp(fb_current_path, "/") == 0) {
                        snprintf(full_path, sizeof(full_path), "/%s", file_name);
                    } else {
                        snprintf(full_path, sizeof(full_path), "%s/%s", fb_current_path, file_name);
                    }


                    // Check file type and handle accordingly
                    size_t name_len = strlen(file_name);
                    if(name_len > 4 && strcmp(file_name + name_len - 4, ".sub") == 0) {
                        // SubGHz file - navigate to playback screen
                        playback_selected_file = String(full_path);
                        scr_mgr_switch(SCREEN2_2_1_ID, false);
                    } else if(name_len > 4 && strcmp(file_name + name_len - 4, ".mp3") == 0) {
                        // MP3 file - navigate to audio playback screen

                        // Stop any currently playing audio
                        audio.stopSong();

                        // Start playing the selected file
                        if(audio.connecttoFS(SD, full_path)) {
                            // Remember which file is playing
                            strncpy(fb_playing_file, full_path, sizeof(fb_playing_file) - 1);
                            fb_playing_file[sizeof(fb_playing_file) - 1] = '\0';
                            strncpy(fb_audio_playing_file, full_path, 511);  // 512 - 1
                            fb_audio_playing_file[511] = '\0';


                            // Navigate to audio playback screen
                            scr_mgr_push(SCREEN10_2_ID, true);
                        } else {
                            prompt_info("  Playback failed!", 2000);
                        }
                    } else if(name_len > 4 && (strcasecmp(file_name + name_len - 4, ".txt") == 0 ||
                                                strcasecmp(file_name + name_len - 5, ".json") == 0 ||
                                                strcasecmp(file_name + name_len - 5, ".html") == 0)) {
                        // Text, JSON, or HTML file - navigate to text viewer screen
                        extern char fb_text_file_path[];
                        strncpy(fb_text_file_path, full_path, 511);
                        fb_text_file_path[511] = '\0';
                        scr_mgr_push(SCREEN10_3_ID, true);
                    } else {
                        // Regular file - show info
                        File file = SD.open(full_path);
                        if(file) {
                            char info[128];
                            snprintf(info, sizeof(info), "  %s\n  Size: %lu bytes", file_name, (unsigned long)file.size());
                            file.close();
                            prompt_info(info, 2000);
                        } else {
                            prompt_info("  Failed to open file", 2000);
                        }

                        // Force refresh of top layer where prompt lives
                        lv_obj_invalidate(lv_layer_top());
                        lv_timer_create([](lv_timer_t *t) {
                            lv_obj_invalidate(lv_layer_top());
                            lv_refr_now(NULL);
                            tft.drawPixel(0, 0, 0);
                            lv_timer_del(t);
                        }, 1, NULL);
                    }
                }
            }
        }
    }
}

static void scr10_1_btn_event_cb(lv_event_t * e)
{
    if(e->code == LV_EVENT_CLICKED){
        exit10_1_anim(SCREEN10_ID, scr10_1_cont);
    }
}

void fb_load_directory(const char *path)
{
    // Show loading message
    prompt_info("  Loading...", 500);

    // Turn on activity LED
    lv_led_on(fb_led);
    lv_refr_now(NULL);  // Force immediate display refresh to show LED on

    // Clear the current list
    lv_obj_clean(fb_file_list);

    // Update path label
    lv_label_set_text_fmt(fb_path_label, "Path: %s", path);

    // Add parent directory entry if not at root
    if(strcmp(path, "/") != 0) {
        lv_obj_t *parent_item = lv_list_add_btn(fb_file_list, NULL, " .. (Parent)");
        lv_obj_add_event_cb(parent_item, fb_list_event, LV_EVENT_ALL, NULL);
    }

    // Use POSIX dirent functions instead of Arduino SD library's openNextFile()
    // This avoids the ESP32 bug that stops iteration after ~11 files
    std::vector<String> directories;
    std::vector<String> files;
    std::vector<unsigned long> file_sizes;

    // Try multiple mount points (SD card VFS mount location)
    const char* mount_points[] = {"/sd", "/sdcard", "/mnt/sd"};
    DIR *dir = NULL;

    for (int i = 0; i < 3; i++) {
        char test_path[512];
        snprintf(test_path, sizeof(test_path), "%s%s", mount_points[i], path);
        dir = opendir(test_path);
        if (dir) {
            strncpy(fb_working_mount, mount_points[i], sizeof(fb_working_mount) - 1);
            fb_working_mount[sizeof(fb_working_mount) - 1] = '\0';
            break;
        }
    }

    // Fallback: try direct path
    if (!dir) {
        dir = opendir(path);
        if (dir) {
            fb_working_mount[0] = '\0';
        }
    }

    if(!dir) {
        lv_obj_t *item = lv_list_add_btn(fb_file_list, NULL, " Error opening directory");
        apply_text_color(item);
        lv_led_off(fb_led);
        return;
    }

    struct dirent *entry;
    int count = 0;
    while((entry = readdir(dir)) != NULL && count < 500) {
        // Skip . and .. entries
        if(strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        // Use d_type for reliable file/directory detection (same as SubGHz playback browser)
        if(entry->d_type == DT_DIR) {
            directories.push_back(String(entry->d_name));
            count++;
        } else if(entry->d_type == DT_REG) {
            files.push_back(String(entry->d_name));

            // Get file size for display (fallback to stat if needed)
            char full_path[512];
            if(strcmp(fb_working_mount, "/") == 0 || (strcmp(fb_working_mount, path) == 0 && strcmp(path, "/") == 0)) {
                snprintf(full_path, sizeof(full_path), "%s/%s", fb_working_mount, entry->d_name);
            } else if(strcmp(fb_working_mount, path) == 0) {
                snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);
            } else {
                if(strcmp(path, "/") == 0) {
                    snprintf(full_path, sizeof(full_path), "%s/%s", fb_working_mount, entry->d_name);
                } else {
                    snprintf(full_path, sizeof(full_path), "%s%s/%s", fb_working_mount, path, entry->d_name);
                }
            }

            struct stat file_stat;
            if(stat(full_path, &file_stat) == 0) {
                file_sizes.push_back(file_stat.st_size);
            } else {
                file_sizes.push_back(0);  // Unknown size
            }
            count++;
        }

        // Yield to prevent watchdog timeout when scanning large directories
        if(count % 10 == 0) delay(1);
    }

    closedir(dir);


    // Display directories first
    for(size_t i = 0; i < directories.size(); i++) {
        char item_text[128];
        snprintf(item_text, sizeof(item_text), " %s", directories[i].c_str());
        lv_obj_t *item = lv_list_add_btn(fb_file_list, NULL, item_text);
        lv_obj_add_event_cb(item, fb_list_event, LV_EVENT_ALL, NULL);
    }

    // Display files
    for(size_t i = 0; i < files.size(); i++) {
        char item_text[128];
        unsigned long size = file_sizes[i];
        const char *size_unit = "B";
        float display_size = size;

        if(size >= 1024) {
            display_size = size / 1024.0;
            size_unit = "KB";
        }
        if(display_size >= 1024) {
            display_size = display_size / 1024.0;
            size_unit = "MB";
        }

        snprintf(item_text, sizeof(item_text), " %s  %.1f%s",
                 files[i].c_str(), display_size, size_unit);
        lv_obj_t *item = lv_list_add_btn(fb_file_list, NULL, item_text);
        lv_obj_add_event_cb(item, fb_list_event, LV_EVENT_ALL, NULL);
    }

    // Style all list items
    for(int i = 0; i < lv_obj_get_child_cnt(fb_file_list); i++) {
        lv_obj_t *item = lv_obj_get_child(fb_file_list, i);
        lv_obj_set_style_text_font(item, FONT_BOLD_14, LV_PART_MAIN);
        apply_bg_color(item);
        apply_text_color(item);
        lv_obj_remove_style(item, NULL, LV_STATE_FOCUS_KEY);
        lv_obj_set_style_radius(item, 5, LV_STATE_FOCUS_KEY);
        lv_obj_set_style_outline_color(item, lv_color_hex(EMBED_COLOR_FOCUS_ON), LV_STATE_FOCUS_KEY);
        lv_obj_set_style_outline_width(item, 2, LV_STATE_FOCUS_KEY);
    }

    // Turn off activity LED
    lv_led_off(fb_led);
    lv_refr_now(NULL);  // Force immediate display refresh to show LED off
}

void create10_1(lv_obj_t *parent)
{
    scr10_1_cont = create_screen_container(parent);

    lv_obj_t *label = lv_label_create(scr10_1_cont);
    apply_text_color(label);
    lv_obj_set_style_text_font(label, FONT_BOLD_16, LV_PART_MAIN);
    lv_label_set_text(label, "File Browser");
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 10);

    fb_led = lv_led_create(scr10_1_cont);
    lv_obj_set_size(fb_led, 16, 16);
    lv_obj_align(fb_led, LV_ALIGN_TOP_RIGHT, -15, 10);
    lv_led_off(fb_led);

    fb_path_label = lv_label_create(scr10_1_cont);
    lv_obj_set_width(fb_path_label, DISPALY_WIDTH * 0.9);
    apply_text_color(fb_path_label);
    lv_obj_set_style_text_font(fb_path_label, FONT_BOLD_14, LV_PART_MAIN);
    lv_label_set_long_mode(fb_path_label, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_align(fb_path_label, LV_ALIGN_TOP_LEFT, 10, 35);

    fb_file_list = lv_list_create(scr10_1_cont);
    lv_obj_set_size(fb_file_list, LV_HOR_RES - 10, 110);
    lv_obj_align(fb_file_list, LV_ALIGN_BOTTOM_MID, 0, -5);
    apply_bg_color(fb_file_list);
    lv_obj_set_style_pad_top(fb_file_list, 2, LV_PART_MAIN);
    lv_obj_set_style_pad_row(fb_file_list, 2, LV_PART_MAIN);
    apply_no_radius(fb_file_list);
    apply_no_border(fb_file_list);
    apply_no_shadow(fb_file_list);

    // back button
    scr_back_btn_create(scr10_1_cont, scr10_1_btn_event_cb);
}

void entry10_1(void) {
    strcpy(fb_current_path, "/");
    entry10_1_anim(scr10_1_cont);
    fb_load_directory(fb_current_path);
    lv_group_set_wrap(lv_group_get_default(), true);
}

void exit10_1(void) {
    lv_group_set_wrap(lv_group_get_default(), false);
}

void destroy10_1(void) {}

scr_lifecycle_t screen10_1 = {
    .create = create10_1,
    .entry = entry10_1,
    .exit  = exit10_1,
    .destroy = destroy10_1,
};
#endif
// --------------------- screen 10.2 --------------------- File Browser Audio Playback
#if 1
lv_obj_t *scr10_2_cont;
lv_obj_t *fb_audio_label;
char fb_audio_playing_file[512] = "";  // File being played on this screen
int fb_audio_last_encoder_pos = 0;     // For volume control

void entry10_2_anim(lv_obj_t *obj) { entry1_anim(obj); }
void exit10_2_anim(int user_data, lv_obj_t *obj) { exit1_anim(user_data, obj); }

static void scr10_2_btn_event_cb(lv_event_t * e)
{
    if(e->code == LV_EVENT_CLICKED){
        // Stop playback and return to file browser
        audio.stopSong();
        fb_audio_playing_file[0] = '\0';
        fb_playing_file[0] = '\0';  // Clear file browser tracking too

        // Save volume if changed
        extern bool volume_changed_during_playback;
        extern uint8_t current_volume;
        if(volume_changed_during_playback) {
            g_config.audio.volume = current_volume;
            config_save();
            volume_changed_during_playback = false;
        }

        exit10_2_anim(SCREEN10_1_ID, scr10_2_cont);
    }
}

void create10_2(lv_obj_t *parent)
{
    scr10_2_cont = create_screen_container(parent);

    lv_obj_t *label = lv_label_create(scr10_2_cont);
    apply_text_color(label);
    lv_obj_set_style_text_font(label, FONT_BOLD_20, LV_PART_MAIN);
    lv_label_set_text(label, "Now Playing");
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 10);

    fb_audio_label = lv_label_create(scr10_2_cont);
    lv_obj_set_width(fb_audio_label, DISPALY_WIDTH * 0.75);  // Constrain width to avoid pushing elements
    apply_text_color(fb_audio_label);
    lv_obj_set_style_text_font(fb_audio_label, &Font_Mono_Bold_20, LV_PART_MAIN);
    lv_label_set_long_mode(fb_audio_label, LV_LABEL_LONG_DOT);  // No scrolling to avoid choppiness
    lv_obj_set_style_text_align(fb_audio_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);  // Center text
    lv_label_set_text(fb_audio_label, "");
    lv_obj_align(fb_audio_label, LV_ALIGN_CENTER, 0, 0);

    // Back/Close button
    scr_back_btn_create(scr10_2_cont, scr10_2_btn_event_cb);
}

void entry10_2(void) {
    entry10_2_anim(scr10_2_cont);

    // Update label with filename
    if(strlen(fb_audio_playing_file) > 0 && fb_audio_label) {
        // Extract just the filename from the path
        const char *filename = strrchr(fb_audio_playing_file, '/');
        if(filename) {
            filename++;  // Skip the '/'
        } else {
            filename = fb_audio_playing_file;
        }
        lv_label_set_text(fb_audio_label, filename);
    }

    // Clear the input group and add only the back button
    // This ensures encoder select button only works on back button
    lv_group_t *g = lv_group_get_default();

    // Remove all objects from group
    while(lv_group_get_obj_count(g) > 0) {
        lv_group_remove_obj(lv_group_get_focused(g));
    }

    // Find and add only the back button to the group
    // scr_back_btn_create already added it, so we need to add it back
    lv_obj_t *back_btn = lv_obj_get_child(scr10_2_cont, -1);  // Last child is back button
    if(back_btn) {
        lv_group_add_obj(g, back_btn);
        lv_group_focus_obj(back_btn);
    }

    // Reset encoder position for volume control
    extern int indev_encoder_pos;
    fb_audio_last_encoder_pos = indev_encoder_pos;
}

void exit10_2(void) {
    // Group will be repopulated when returning to file browser
    // No special cleanup needed
}

void destroy10_2(void) {}

scr_lifecycle_t screen10_2 = {
    .create = create10_2,
    .entry = entry10_2,
    .exit  = exit10_2,
    .destroy = destroy10_2,
};
#endif
//************************************[ screen 10_3 ]*************************************** Text Viewer
#if 1
lv_obj_t *scr10_3_cont;
lv_obj_t *text_viewer_title;
lv_obj_t *text_viewer_content;
char fb_text_file_path[512] = "";

static void text_viewer_back_btn_cb(lv_event_t *e) {
    scr_mgr_pop(false);
}

// Event handler for text viewer line items - enables scrolling only when focused
static void text_viewer_line_event(lv_event_t *e) {
    lv_event_code_t code = e->code;
    lv_obj_t *btn = lv_event_get_target(e);

    if (code == LV_EVENT_FOCUSED) {
        // Get the label inside the button
        lv_obj_t *label = lv_obj_get_child(btn, 0);
        if (label) {
            lv_label_set_long_mode(label, LV_LABEL_LONG_SCROLL_CIRCULAR);
        }
    } else if (code == LV_EVENT_DEFOCUSED) {
        // Stop scrolling when unfocused
        lv_obj_t *label = lv_obj_get_child(btn, 0);
        if (label) {
            lv_label_set_long_mode(label, LV_LABEL_LONG_CLIP);
        }
    }
}

void create10_3(lv_obj_t *parent) {
    scr10_3_cont = lv_obj_create(parent);
    lv_obj_set_size(scr10_3_cont, LV_PCT(100), LV_PCT(100));
    apply_bg_color(scr10_3_cont);
    apply_no_scrollbar(scr10_3_cont);
    apply_no_border(scr10_3_cont);
    apply_no_padding(scr10_3_cont);

    // Back button
    scr_back_btn_create(scr10_3_cont, text_viewer_back_btn_cb);

    // Title - shows filename
    text_viewer_title = lv_label_create(scr10_3_cont);
    lv_obj_set_width(text_viewer_title, DISPALY_WIDTH * 0.85);
    lv_obj_set_style_text_font(text_viewer_title, FONT_BOLD_16, 0);
    apply_text_color(text_viewer_title);
    lv_label_set_long_mode(text_viewer_title, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_align(text_viewer_title, LV_ALIGN_TOP_MID, 0, 10);

    // Create list for line-by-line text display
    text_viewer_content = lv_list_create(scr10_3_cont);
    lv_obj_set_size(text_viewer_content, LV_HOR_RES - 10, 125);
    lv_obj_align(text_viewer_content, LV_ALIGN_TOP_MID, 0, 35);
    apply_bg_color(text_viewer_content);
    lv_obj_set_style_pad_top(text_viewer_content, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_row(text_viewer_content, 0, LV_PART_MAIN);
    apply_no_radius(text_viewer_content);
    apply_no_border(text_viewer_content);
    apply_no_shadow(text_viewer_content);
}

void entry10_3(void) {
    entry1_anim(scr10_3_cont);
    lv_group_set_wrap(lv_group_get_default(), true);

    // Extract filename from path
    const char *filename = strrchr(fb_text_file_path, '/');
    filename = filename ? filename + 1 : fb_text_file_path;
    lv_label_set_text(text_viewer_title, filename);

    // Clear existing list items
    lv_obj_clean(text_viewer_content);

    // Load file contents
    if (!sd_is_valid()) {
        lv_obj_t *item = lv_list_add_btn(text_viewer_content, NULL, "SD card not available");
        apply_text_color(item);
        lv_obj_set_style_text_font(item, FONT_LIGHT_14, LV_PART_MAIN);
        lv_obj_add_event_cb(item, text_viewer_line_event, LV_EVENT_FOCUSED, NULL);
        lv_obj_add_event_cb(item, text_viewer_line_event, LV_EVENT_DEFOCUSED, NULL);
        return;
    }

    File file = SD.open(fb_text_file_path, FILE_READ);
    if (!file) {
        lv_obj_t *item = lv_list_add_btn(text_viewer_content, NULL, "Failed to open file");
        apply_text_color(item);
        lv_obj_set_style_text_font(item, FONT_LIGHT_14, LV_PART_MAIN);
        lv_obj_add_event_cb(item, text_viewer_line_event, LV_EVENT_FOCUSED, NULL);
        lv_obj_add_event_cb(item, text_viewer_line_event, LV_EVENT_DEFOCUSED, NULL);
        return;
    }

    // Read file contents (limit to 8KB for memory safety)
    size_t file_size = file.size();
    if (file_size > 8192) {
        file_size = 8192;
    }

    char *buffer = (char *)malloc(file_size + 1);
    if (!buffer) {
        file.close();
        lv_obj_t *item = lv_list_add_btn(text_viewer_content, NULL, "Out of memory");
        apply_text_color(item);
        lv_obj_set_style_text_font(item, FONT_LIGHT_14, LV_PART_MAIN);
        lv_obj_add_event_cb(item, text_viewer_line_event, LV_EVENT_FOCUSED, NULL);
        lv_obj_add_event_cb(item, text_viewer_line_event, LV_EVENT_DEFOCUSED, NULL);
        return;
    }

    size_t bytes_read = file.read((uint8_t *)buffer, file_size);
    buffer[bytes_read] = '\0';
    file.close();

    // Parse and display content line by line
    char *line_start = buffer;
    char *line_end;
    int line_count = 0;

    while (line_start && *line_start != '\0' && line_count < 500) {  // Limit to 500 lines
        // Find end of line
        line_end = strchr(line_start, '\n');

        if (line_end) {
            *line_end = '\0';  // Null-terminate the line
        }

        // Remove carriage return if present
        size_t len = strlen(line_start);
        if (len > 0 && line_start[len - 1] == '\r') {
            line_start[len - 1] = '\0';
        }

        // Add line to list (even if empty, to preserve structure)
        lv_obj_t *item = lv_list_add_btn(text_viewer_content, NULL, line_start);
        apply_text_color(item);
        lv_obj_set_style_text_font(item, FONT_LIGHT_14, LV_PART_MAIN);
        apply_bg_color(item);

        // Remove all padding and spacing for compact display
        lv_obj_set_style_pad_top(item, 0, LV_PART_MAIN);
        lv_obj_set_style_pad_bottom(item, 0, LV_PART_MAIN);
        lv_obj_set_style_pad_left(item, 2, LV_PART_MAIN);
        lv_obj_set_style_pad_right(item, 2, LV_PART_MAIN);
        apply_no_border(item);
        lv_obj_set_style_outline_width(item, 0, LV_PART_MAIN);

        // Style for focused state
        lv_obj_set_style_bg_color(item, lv_color_hex(EMBED_COLOR_FOCUS_ON), LV_STATE_FOCUS_KEY);
        lv_obj_set_style_text_color(item, lv_color_hex(EMBED_COLOR_BG), LV_STATE_FOCUS_KEY);

        // Add event handler for focus-based scrolling
        lv_obj_add_event_cb(item, text_viewer_line_event, LV_EVENT_FOCUSED, NULL);
        lv_obj_add_event_cb(item, text_viewer_line_event, LV_EVENT_DEFOCUSED, NULL);

        // Add to input group for encoder navigation
        lv_group_add_obj(lv_group_get_default(), item);

        line_count++;

        // Move to next line
        if (line_end) {
            line_start = line_end + 1;
        } else {
            break;  // No more lines
        }
    }

    free(buffer);

    // Force display refresh
    lv_refr_now(NULL);
    tft.drawPixel(0, 0, 0);
}

void exit10_3(void) {
    lv_group_set_wrap(lv_group_get_default(), false);
}

void destroy10_3(void) {}

scr_lifecycle_t screen10_3 = {
    .create = create10_3,
    .entry = entry10_3,
    .exit  = exit10_3,
    .destroy = destroy10_3,
};
#endif
//************************************[ screen 8 ]****************************************** music 
#if 1
#include "Audio.h"
extern Audio audio;
extern void scr8_read_music_from_SD(void);

static lv_obj_t *scr8_cont;
lv_obj_t *music_lab;
lv_obj_t *pause_btn;
static lv_obj_t *next_btn;
static lv_obj_t *prev_btn;
static lv_obj_t *mic_led;
bool music_is_running = false;
static lv_timer_t *mic_chk_timer = NULL;
static lv_timer_t *music_load_timer = NULL;
static lv_timer_t *music_refresh_timer = NULL;
int music_idx = 0;
std::vector<String> music_list;
volatile bool music_label_needs_update = false;  // Flag for safe LVGL updates from audio callback

void entry8_anim(lv_obj_t *obj)
{
    entry1_anim(obj);
}

void exit8_anim(int user_data, lv_obj_t *obj)
{
    exit1_anim(user_data, obj);
}

// Forward declarations
void mic_chk_timer_event(lv_timer_t *t);

// Timer to force display refresh after loading completes
void music_refresh_timer_event(lv_timer_t *t)
{
    if(music_refresh_timer) {
        lv_timer_del(music_refresh_timer);
        music_refresh_timer = NULL;
    }

    // Force a complete display refresh
    lv_obj_invalidate(lv_scr_act());
}

// Timer callback to load music files after screen is visible
void music_load_timer_event(lv_timer_t *t)
{
    // This is a one-shot timer, delete it first
    if(music_load_timer) {
        lv_timer_del(music_load_timer);
        music_load_timer = NULL;
    }

    // Safety check: ensure widgets are created
    if(!music_lab || !mic_led) {
        return;
    }

    // Delete mic_chk_timer temporarily to prevent interference
    if(mic_chk_timer) {
        lv_timer_del(mic_chk_timer);
        mic_chk_timer = NULL;
    }

    // Show loading indicator
    lv_led_on(mic_led);

    // Load files
    scr8_read_music_from_SD();

    // Update label
    if(music_list.empty()) {
        lv_label_set_text(music_lab, "NO MUSIC FILES");
    } else {
        if(music_idx >= music_list.size()) {
            music_idx = 0;
        }
        if(music_idx < music_list.size()) {
            lv_label_set_text(music_lab, music_list[music_idx].c_str());
        }
    }

    // Turn off indicator
    lv_led_off(mic_led);

    // Recreate mic_chk_timer to resume normal operation
    mic_chk_timer = lv_timer_create(mic_chk_timer_event, 10, NULL);

    // Create a one-shot timer to force refresh after we exit this callback
    // This ensures refresh happens outside of the timer handler call stack
    music_refresh_timer = lv_timer_create(music_refresh_timer_event, 50, NULL);
    lv_timer_set_repeat_count(music_refresh_timer, 1);

}

extern int i2s_mic_cnt;
void mic_chk_timer_event(lv_timer_t *t)
{
    if(i2s_mic_cnt > 3)
    {
        i2s_mic_cnt = 0;
        lv_led_on(mic_led);
    } else 
    {
        lv_led_off(mic_led);
    }
}

// Helper function to save volume if changed
void save_volume_if_changed()
{
    extern bool volume_changed_during_playback;
    extern uint8_t current_volume;

    if(volume_changed_during_playback) {
        g_config.audio.volume = current_volume;
        config_save();
        volume_changed_during_playback = false;
    }
}

void music_player_event(lv_event_t * e)
{
    char buf[64];
    lv_obj_t *tgt = (lv_obj_t *)e->target;

    // Safety check: don't process events if music_list is empty
    if(music_list.empty()) {
        return;
    }

    // Ensure music_idx is valid
    if(music_idx >= music_list.size()) {
        music_idx = 0;
    }

    if(e->code == LV_EVENT_CLICKED) {
        if(tgt == pause_btn) {
            if(music_is_running == false) {
                lv_snprintf(buf, 64, "/music/%s", music_list[music_idx].c_str());
                audio.connecttoFS(SD, buf);
                music_is_running = true;
                lv_obj_set_style_bg_img_src(pause_btn, &img_play_32, 0);
                lv_port_indev_enabled(false);
                // Disable scrolling during playback to avoid choppy audio
                lv_label_set_long_mode(music_lab, LV_LABEL_LONG_DOT);
                // Reset encoder position for volume control
                extern int last_encoder_pos;
                extern int indev_encoder_pos;
                last_encoder_pos = indev_encoder_pos;
            } else {
                audio.stopSong();
                music_is_running = false;
                lv_port_indev_enabled(true);
                lv_obj_set_style_bg_img_src(pause_btn, &img_pause_32, 0);
                lv_label_set_long_mode(music_lab, LV_LABEL_LONG_SCROLL_CIRCULAR);
                save_volume_if_changed();
            }
        } else if(tgt == next_btn) {
            if(music_idx + 1 < music_list.size()) {
                music_idx++;
                lv_label_set_text(music_lab, music_list[music_idx].c_str());
                lv_obj_invalidate(music_lab);  // Force redraw
                lv_refr_now(NULL);             // Immediate refresh
            }
        } else if(tgt == prev_btn) {
            if(music_idx > 0) {
                music_idx--;
                lv_label_set_text(music_lab, music_list[music_idx].c_str());
                lv_obj_invalidate(music_lab);  // Force redraw
                lv_refr_now(NULL);             // Immediate refresh
            }
        }
    }
}

static void scr8_btn_event_cb(lv_event_t * e)
{
    if(e->code == LV_EVENT_CLICKED){
        audio.stopSong();
        music_is_running = false;
        lv_port_indev_enabled(true);
        save_volume_if_changed();
        exit8_anim(SCREEN0_ID, scr8_cont);
    }
}

static lv_obj_t * scr8_music_btn_create(void)
{
    lv_obj_t *btn = lv_btn_create(scr8_cont);
    lv_obj_set_size(btn, 40, 40);
    apply_no_border(btn);
    apply_no_shadow(btn);
    lv_obj_set_style_radius(btn, 10, 0);
    lv_obj_remove_style(btn, NULL, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_pad(btn, 2, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_width(btn, 2, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_color(btn, lv_color_hex(EMBED_COLOR_FOCUS_ON), LV_STATE_FOCUS_KEY);
    lv_obj_add_event_cb(btn, music_player_event, LV_EVENT_CLICKED, NULL);
    return btn;
}

static void create8(lv_obj_t *parent) {
    scr8_cont = create_screen_container(parent);

    mic_led  = lv_led_create(scr8_cont);
    lv_obj_set_size(mic_led, 16, 16);
    lv_obj_align(mic_led, LV_ALIGN_TOP_RIGHT, -10, 10);
    lv_led_off(mic_led);

    lv_obj_t *label = lv_label_create(scr8_cont);
    apply_text_color(label);
    lv_obj_set_style_text_font(label, &Font_Mono_Bold_18, LV_PART_MAIN);
    lv_label_set_text(label, "Music");
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 10);

    music_lab = lv_label_create(scr8_cont);
    lv_obj_set_width(music_lab, DISPALY_WIDTH * 0.9);
    apply_text_color(music_lab);
    lv_obj_set_style_text_font(music_lab, &Font_Mono_Bold_20, LV_PART_MAIN);
    lv_label_set_long_mode(music_lab, LV_LABEL_LONG_SCROLL_CIRCULAR);

    if(sd_is_valid() == false) {
        lv_label_set_text(music_lab, "NO FIND SD CARD");
        goto CREATE8_END;
    }

    if(music_list.empty()) {
        lv_label_set_text(music_lab, "Loading...");
    } else {
        lv_label_set_text(music_lab, music_list[music_idx].c_str());
    }

    next_btn = scr8_music_btn_create();
    lv_obj_set_style_bg_img_src(next_btn, &img_next_32, 0);
    lv_obj_align(next_btn, LV_ALIGN_BOTTOM_RIGHT, -30, -15);

    pause_btn = scr8_music_btn_create();
    lv_obj_set_style_bg_img_src(pause_btn, &img_pause_32, 0);
    lv_obj_align(pause_btn, LV_ALIGN_BOTTOM_MID, 0, -15);

    prev_btn = scr8_music_btn_create();
    lv_obj_set_style_bg_img_src(prev_btn, &img_prev_32, 0);
    lv_obj_align(prev_btn, LV_ALIGN_BOTTOM_LEFT, 30, -15);

CREATE8_END:
    lv_obj_align(music_lab, LV_ALIGN_CENTER, 0, 0);
    // back btn
    scr_back_btn_create(scr8_cont, scr8_btn_event_cb);
}
static void entry8(void) {
    music_is_running = false;
    mic_chk_timer = lv_timer_create(mic_chk_timer_event, 10, NULL);

    // Load music files on first entry (use timer to wait for screen to be visible)
    if(music_list.empty() && sd_is_valid()) {
        // Create a one-shot timer to load music after 500ms (screen transition complete)
        music_load_timer = lv_timer_create(music_load_timer_event, 500, NULL);
        lv_timer_set_repeat_count(music_load_timer, 1);
    } else if(!music_list.empty() && music_lab) {
        // Update label on subsequent entries
        if(music_idx >= music_list.size()) {
            music_idx = 0;
        }
        if(music_idx < music_list.size()) {
            lv_label_set_text(music_lab, music_list[music_idx].c_str());
        }
    }
}
static void exit8(void) {
    if(mic_chk_timer) {
        lv_timer_del(mic_chk_timer);
        mic_chk_timer = NULL;
    }
    if(music_load_timer) {
        lv_timer_del(music_load_timer);
        music_load_timer = NULL;
    }
    if(music_refresh_timer) {
        lv_timer_del(music_refresh_timer);
        music_refresh_timer = NULL;
    }
}
static void destroy8(void) { 
}

static scr_lifecycle_t screen8 = {
    .create = create8,
    .entry = entry8,
    .exit  = exit8,
    .destroy = destroy8,
};
#endif
//************************************[ screen 9 ]****************************************** NRF24
#if 1

lv_obj_t *scr9_cont;
lv_obj_t *nrf24_mode_btn;
lv_obj_t *nrf24_info;
lv_obj_t *nrf24_label;
lv_timer_t *nrf24_timer;

int nrf24_cont = 0;

void entry9_anim(lv_obj_t *obj) { entry1_anim(obj); }
void exit9_anim(int user_data, lv_obj_t *obj) { exit1_anim(user_data, obj); }

void nrf_recv_event(lv_timer_t *t)
{
    if(nrf24_get_mode() == NRF24_MODE_SEND) {
        String str = "Hello World! #" + String(nrf24_cont++);
        nrf24_send(str.c_str());
        // lv_label_set_text_fmt(nrf24_label, "%s", str.c_str());
        ws2812_pos_demo(nrf24_cont);
    } else if(nrf24_get_mode() == NRF24_MODE_RECV)
    {
        String str = "recv #" + String(nrf24_cont++);
        // lv_label_set_text_fmt(nrf24_label, "%s", str.c_str());
        // ws2812_pos_demo1();
    }
    lv_timer_handler();
}

static void scr9_btn_event_cb(lv_event_t * e)
{
    if(e->code == LV_EVENT_CLICKED){
        // scr_mgr_set_anim(LV_SCR_LOAD_ANIM_FADE_OUT, -1, -1);
        // scr_mgr_switch(SCREEN0_ID, false);
        exit9_anim(SCREEN0_ID, scr9_cont);
    }
}

void nrf24_mode_sw_event(lv_event_t *e)
{
    lv_obj_t *tgt =  (lv_obj_t *)e->target;
    lv_obj_t *data = (lv_obj_t *)e->user_data;
    
    if(e->code == LV_EVENT_CLICKED) {
        switch (nrf24_get_mode())
        {
            case NRF24_MODE_SEND: 
                nrf24_set_mode(NRF24_MODE_RECV); 
                lv_label_set_text(data, "recv");
                break;
                
                // lv_label_set_text_fmt(sghz_label, "# Recv - 0");
            case NRF24_MODE_RECV: 
                nrf24_set_mode(NRF24_MODE_SEND); 
                lv_label_set_text(data, "send"); 
                break;
                
                // lv_label_set_text_fmt(sghz_label, "# Send - 0");
            default: break;
        }
    }
}

static void create9(lv_obj_t *parent) {
    scr9_cont = create_screen_container(parent);

    if(nrf24_is_init() == false)
    {
        nrf24_label = lv_label_create(scr9_cont);
        apply_text_color(nrf24_label);
        lv_obj_set_style_text_font(nrf24_label, FONT_BOLD_16, LV_PART_MAIN);
        lv_label_set_text(nrf24_label, "NRF24 is invalid.");
        lv_obj_align(nrf24_label, LV_ALIGN_CENTER, 0, 0);

        scr_back_btn_create(scr9_cont, scr9_btn_event_cb);
    } else {

        nrf24_label = lv_label_create(scr9_cont);
        apply_text_color(nrf24_label);
        lv_obj_set_style_text_font(nrf24_label, FONT_BOLD_16, LV_PART_MAIN);
        lv_label_set_text(nrf24_label, "Freq:2400 M \n"
                                        "BitRate:1000 kbps\n"
                                        "Power:0 dBm");
        lv_obj_align(nrf24_label, LV_ALIGN_LEFT_MID, 10, 0);

        nrf24_mode_btn = lv_btn_create(scr9_cont);
        lv_obj_set_height(nrf24_mode_btn, 50);
        apply_no_shadow(nrf24_mode_btn);
        lv_obj_set_style_pad_row(nrf24_mode_btn, 0, LV_PART_MAIN);
        lv_obj_remove_style(nrf24_mode_btn, NULL, LV_STATE_FOCUS_KEY);
        lv_obj_set_style_outline_pad(nrf24_mode_btn, 2, LV_STATE_FOCUS_KEY);
        lv_obj_set_style_outline_width(nrf24_mode_btn, 2, LV_STATE_FOCUS_KEY);
        lv_obj_set_style_outline_color(nrf24_mode_btn, lv_color_hex(EMBED_COLOR_FOCUS_ON), LV_STATE_FOCUS_KEY);
        // lv_obj_align(nrf24_mode_btn, LV_ALIGN_TOP_MID, 0, 6);
        lv_obj_align(nrf24_mode_btn, LV_ALIGN_CENTER, 80, 0);
        lv_obj_t *nrf24_info = lv_label_create(nrf24_mode_btn);
        lv_obj_center(nrf24_info);
        switch (nrf24_get_mode())
        {
            case NRF24_MODE_SEND: lv_label_set_text(nrf24_info, "send"); break;
            case NRF24_MODE_RECV: lv_label_set_text(nrf24_info, "recv"); break;
            default: break;
        }
        lv_obj_add_event_cb(nrf24_mode_btn, nrf24_mode_sw_event, LV_EVENT_CLICKED, nrf24_info);

        // back btn
        scr_back_btn_create(scr9_cont, scr9_btn_event_cb);
        lv_group_set_wrap(lv_group_get_default(), true);
        entry9_anim(scr9_cont);
        nrf24_timer = lv_timer_create(nrf_recv_event, 500, NULL);
    }
}
static void entry9(void) {
    vTaskResume(nrf24_handle);
}
static void exit9(void) {
    vTaskSuspend(nrf24_handle);
}
static void destroy9(void) { 
    if(nrf24_timer) {
        lv_timer_del(nrf24_timer);
        nrf24_timer = NULL;
    }
    ws2812_set_color(CRGB::Black);
}

static scr_lifecycle_t screen9 = {
    .create = create9,
    .entry = entry9,
    .exit  = exit9,
    .destroy = destroy9,
};
#endif
//************************************[ screen 12 ]****************************************** Portal
#if 1
#include "portal.h"

lv_obj_t *scr12_cont;
lv_obj_t *portal_back_btn;
lv_obj_t *portal_status_lab;
lv_obj_t *portal_ssid_lab;
lv_obj_t *portal_ip_lab;
lv_obj_t *portal_victims_lab;
lv_obj_t *portal_start_btn;
lv_obj_t *portal_stop_btn;
lv_obj_t *portal_view_btn;
lv_obj_t *portal_edit_ssid_btn;
lv_timer_t *portal_update_timer = NULL;

// SSID editing keyboard variables
static lv_obj_t *portal_ssid_textarea = NULL;
static lv_obj_t *portal_ssid_keyboard = NULL;

// Data viewer screen variables
lv_obj_t *scr12_1_cont;
lv_obj_t *portal_data_display;
lv_obj_t *portal_record_label;
int current_record_index = 0;
int total_records = 0;

static void portal_update_display() {
    if (!portal_status_lab) return;

    // Update status
    if (portal_is_running) {
        lv_label_set_text(portal_status_lab, "#00FF00 RUNNING#");
    } else {
        lv_label_set_text(portal_status_lab, "#FF0000 STOPPED#");
    }

    // Update SSID
    lv_label_set_text_fmt(portal_ssid_lab, "SSID: %s", portalSsidName.c_str());

    // Update IP
    lv_label_set_text_fmt(portal_ip_lab, "IP: %s", AP_GATEWAY.toString().c_str());

    // Update victim count
    lv_label_set_text_fmt(portal_victims_lab, "Victims: %d", totalCapturedCredentials);

    // Update button visibility
    if (portal_is_running) {
        lv_obj_add_flag(portal_start_btn, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(portal_stop_btn, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_clear_flag(portal_start_btn, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(portal_stop_btn, LV_OBJ_FLAG_HIDDEN);
    }
}

static void portal_timer_cb(lv_timer_t *timer) {
    portal_update_display();
}

// Keyboard event handler for editing portal SSID
static void portal_ssid_keyboard_event(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_READY) {
        // User pressed OK - save new SSID
        const char *new_ssid = lv_textarea_get_text(portal_ssid_textarea);

        if (strlen(new_ssid) > 0 && strlen(new_ssid) < 64) {
            // Update config structure
            strncpy(g_config.wifi.portal.ssid, new_ssid, sizeof(g_config.wifi.portal.ssid) - 1);
            g_config.wifi.portal.ssid[sizeof(g_config.wifi.portal.ssid) - 1] = '\0';

            // Update portal SSID name
            portalSsidName = String(new_ssid);

            // Save to SD card
            if (config_save()) {
                prompt_info("  SSID updated!", 2000);
            } else {
                prompt_info("  Failed to save", 2000);
            }

            // Update display
            portal_update_display();
        }

        // Clean up keyboard and textarea
        if (portal_ssid_keyboard) {
            lv_obj_del(portal_ssid_keyboard);
            portal_ssid_keyboard = NULL;
        }
        if (portal_ssid_textarea) {
            lv_obj_del(portal_ssid_textarea);
            portal_ssid_textarea = NULL;
        }

        // Rebuild the group with screen objects
        lv_group_t *g = lv_group_get_default();
        lv_group_set_editing(g, false);  // Exit editing mode
        lv_group_remove_all_objs(g);

        // Re-add portal screen objects to group (in navigation order)
        lv_group_add_obj(g, portal_back_btn);
        lv_group_add_obj(g, portal_edit_ssid_btn);
        lv_group_add_obj(g, portal_view_btn);
        lv_group_add_obj(g, portal_start_btn);
        lv_group_add_obj(g, portal_stop_btn);

        // Focus back on Edit button
        lv_group_focus_obj(portal_edit_ssid_btn);
    } else if (code == LV_EVENT_CANCEL) {
        // User pressed Cancel
        if (portal_ssid_keyboard) {
            lv_obj_del(portal_ssid_keyboard);
            portal_ssid_keyboard = NULL;
        }
        if (portal_ssid_textarea) {
            lv_obj_del(portal_ssid_textarea);
            portal_ssid_textarea = NULL;
        }

        // Rebuild the group with screen objects
        lv_group_t *g = lv_group_get_default();
        lv_group_set_editing(g, false);  // Exit editing mode
        lv_group_remove_all_objs(g);

        // Re-add portal screen objects to group (in navigation order)
        lv_group_add_obj(g, portal_back_btn);
        lv_group_add_obj(g, portal_edit_ssid_btn);
        lv_group_add_obj(g, portal_view_btn);
        lv_group_add_obj(g, portal_start_btn);
        lv_group_add_obj(g, portal_stop_btn);

        // Focus back on Edit button
        lv_group_focus_obj(portal_edit_ssid_btn);
    }
}

static void portal_edit_ssid_btn_cb(lv_event_t *e) {
    // Create text area for SSID input
    portal_ssid_textarea = lv_textarea_create(lv_scr_act());
    lv_obj_set_size(portal_ssid_textarea, LV_PCT(90), 40);
    lv_obj_align(portal_ssid_textarea, LV_ALIGN_TOP_MID, 0, 40);
    lv_textarea_set_one_line(portal_ssid_textarea, true);
    lv_textarea_set_placeholder_text(portal_ssid_textarea, "Enter Portal SSID");
    lv_textarea_set_text(portal_ssid_textarea, portalSsidName.c_str());  // Pre-fill with current SSID
    lv_textarea_set_max_length(portal_ssid_textarea, 63);  // Max SSID length

    // Enable and style the cursor for visibility
    lv_obj_set_style_anim_time(portal_ssid_textarea, 400, LV_PART_CURSOR);
    lv_obj_set_style_bg_opa(portal_ssid_textarea, LV_OPA_COVER, LV_PART_CURSOR);
    lv_obj_set_style_bg_color(portal_ssid_textarea, lv_color_hex(EMBED_COLOR_FOCUS_ON), LV_PART_CURSOR);
    lv_obj_set_style_border_width(portal_ssid_textarea, 1, LV_PART_CURSOR);
    lv_obj_set_style_border_color(portal_ssid_textarea, lv_color_hex(EMBED_COLOR_FOCUS_ON), LV_PART_CURSOR);

    // Position cursor at end of text
    lv_textarea_set_cursor_pos(portal_ssid_textarea, LV_TEXTAREA_CURSOR_LAST);

    // Create keyboard
    portal_ssid_keyboard = lv_keyboard_create(lv_scr_act());
    lv_keyboard_set_textarea(portal_ssid_keyboard, portal_ssid_textarea);
    lv_obj_set_size(portal_ssid_keyboard, LV_HOR_RES, LV_VER_RES / 2);
    lv_obj_align(portal_ssid_keyboard, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_add_event_cb(portal_ssid_keyboard, portal_ssid_keyboard_event, LV_EVENT_ALL, NULL);

    // Temporarily remove all objects from group and add only keyboard
    lv_group_t *g = lv_group_get_default();
    lv_group_remove_all_objs(g);
    lv_group_add_obj(g, portal_ssid_keyboard);
    lv_group_focus_obj(portal_ssid_keyboard);
    lv_group_set_editing(g, true);  // Enable editing mode for immediate key selection
}

static void portal_start_btn_cb(lv_event_t *e) {
    portal_start();
    portal_update_display();
}

static void portal_stop_btn_cb(lv_event_t *e) {
    portal_stop();
    portal_update_display();
}

static void portal_view_btn_cb(lv_event_t *e) {
    scr_mgr_switch(SCREEN12_1_ID, false);  // Switch to data viewer
}

static void portal_back_btn_cb(lv_event_t *e) {
    scr_mgr_switch(SCREEN6_ID, false);  // Back to WiFi menu
}

static void create12(lv_obj_t *parent) {
    scr12_cont = lv_obj_create(parent);
    lv_obj_set_size(scr12_cont, LV_PCT(100), LV_PCT(100));
    apply_bg_color(scr12_cont);
    apply_no_scrollbar(scr12_cont);
    apply_no_border(scr12_cont);
    apply_no_padding(scr12_cont);

    // Back button (standard "<" button in upper left)
    portal_back_btn = scr_back_btn_create(scr12_cont, portal_back_btn_cb);

    // Title
    lv_obj_t *title = lv_label_create(scr12_cont);
    lv_label_set_text(title, "Nautilus Portal");
    lv_obj_set_style_text_font(title, FONT_BOLD_20, 0);
    apply_text_color(title);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

    // Status container with border
    lv_obj_t *status_cont = lv_obj_create(scr12_cont);
    lv_obj_set_size(status_cont, LV_PCT(60), 120);
    lv_obj_align(status_cont, LV_ALIGN_TOP_LEFT, 10, 40);
    lv_obj_set_flex_flow(status_cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(status_cont, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER);  // Left-align
    lv_obj_set_style_pad_all(status_cont, 10, 0);
    apply_no_border(status_cont);
    apply_bg_color(status_cont);

    // Status label
    portal_status_lab = lv_label_create(status_cont);
    lv_label_set_text(portal_status_lab, "#FF0000 STOPPED#");
    lv_label_set_recolor(portal_status_lab, true);
    lv_obj_set_style_text_font(portal_status_lab, FONT_BOLD_16, 0);

    // SSID label
    portal_ssid_lab = lv_label_create(status_cont);
    lv_label_set_text(portal_ssid_lab, "SSID: Nautilus WiFi");
    lv_obj_set_style_text_font(portal_ssid_lab, FONT_BOLD_14, 0);
    apply_text_color(portal_ssid_lab);

    // IP label
    portal_ip_lab = lv_label_create(status_cont);
    lv_label_set_text(portal_ip_lab, "IP: 172.0.0.1");
    lv_obj_set_style_text_font(portal_ip_lab, FONT_BOLD_14, 0);
    apply_text_color(portal_ip_lab);

    // Victims label
    portal_victims_lab = lv_label_create(status_cont);
    lv_label_set_text(portal_victims_lab, "Victims: 0");
    lv_obj_set_style_text_font(portal_victims_lab, FONT_BOLD_14, 0);
    apply_text_color(portal_victims_lab);

    // Edit SSID button (above View button)
    portal_edit_ssid_btn = lv_btn_create(scr12_cont);
    lv_obj_set_size(portal_edit_ssid_btn, 70, 32);
    lv_obj_align(portal_edit_ssid_btn, LV_ALIGN_BOTTOM_RIGHT, -10, -90);  // Above View button
    lv_obj_set_style_border_color(portal_edit_ssid_btn, lv_color_hex(EMBED_COLOR_BORDER), LV_PART_MAIN);
    lv_obj_set_style_border_width(portal_edit_ssid_btn, 1, LV_PART_MAIN);
    apply_no_shadow(portal_edit_ssid_btn);
    apply_bg_color(portal_edit_ssid_btn);
    lv_obj_remove_style(portal_edit_ssid_btn, NULL, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_pad(portal_edit_ssid_btn, 2, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_width(portal_edit_ssid_btn, 2, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_color(portal_edit_ssid_btn, lv_color_hex(EMBED_COLOR_FOCUS_ON), LV_STATE_FOCUS_KEY);
    lv_obj_add_event_cb(portal_edit_ssid_btn, portal_edit_ssid_btn_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *edit_ssid_label = lv_label_create(portal_edit_ssid_btn);
    apply_text_color(edit_ssid_label);
    lv_obj_set_style_text_font(edit_ssid_label, FONT_BOLD_14, LV_PART_MAIN);
    lv_label_set_text(edit_ssid_label, "Edit");
    lv_obj_center(edit_ssid_label);
    lv_group_add_obj(lv_group_get_default(), portal_edit_ssid_btn);

    // View Data button (above start/stop button)
    portal_view_btn = lv_btn_create(scr12_cont);
    lv_obj_set_size(portal_view_btn, 70, 32);
    lv_obj_align(portal_view_btn, LV_ALIGN_BOTTOM_RIGHT, -10, -50);  // Above start/stop
    lv_obj_set_style_border_color(portal_view_btn, lv_color_hex(EMBED_COLOR_BORDER), LV_PART_MAIN);
    lv_obj_set_style_border_width(portal_view_btn, 1, LV_PART_MAIN);
    apply_no_shadow(portal_view_btn);
    apply_bg_color(portal_view_btn);
    lv_obj_remove_style(portal_view_btn, NULL, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_pad(portal_view_btn, 2, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_width(portal_view_btn, 2, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_color(portal_view_btn, lv_color_hex(EMBED_COLOR_FOCUS_ON), LV_STATE_FOCUS_KEY);
    lv_obj_add_event_cb(portal_view_btn, portal_view_btn_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *view_label = lv_label_create(portal_view_btn);
    apply_text_color(view_label);
    lv_obj_set_style_text_font(view_label, FONT_BOLD_14, LV_PART_MAIN);
    lv_label_set_text(view_label, "View");
    lv_obj_center(view_label);
    lv_group_add_obj(lv_group_get_default(), portal_view_btn);

    // Start button (bottom right, styled like Record RAW buttons)
    portal_start_btn = lv_btn_create(scr12_cont);
    lv_obj_set_size(portal_start_btn, 70, 32);
    lv_obj_align(portal_start_btn, LV_ALIGN_BOTTOM_RIGHT, -10, -10);
    lv_obj_set_style_border_color(portal_start_btn, lv_color_hex(EMBED_COLOR_BORDER), LV_PART_MAIN);
    lv_obj_set_style_border_width(portal_start_btn, 1, LV_PART_MAIN);
    apply_no_shadow(portal_start_btn);
    apply_bg_color(portal_start_btn);
    lv_obj_remove_style(portal_start_btn, NULL, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_pad(portal_start_btn, 2, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_width(portal_start_btn, 2, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_color(portal_start_btn, lv_color_hex(EMBED_COLOR_FOCUS_ON), LV_STATE_FOCUS_KEY);
    lv_obj_add_event_cb(portal_start_btn, portal_start_btn_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *start_label = lv_label_create(portal_start_btn);
    apply_text_color(start_label);
    lv_obj_set_style_text_font(start_label, FONT_BOLD_14, LV_PART_MAIN);
    lv_label_set_text(start_label, "Start");
    lv_obj_center(start_label);
    lv_group_add_obj(lv_group_get_default(), portal_start_btn);

    // Stop button (same position as start button)
    portal_stop_btn = lv_btn_create(scr12_cont);
    lv_obj_set_size(portal_stop_btn, 70, 32);
    lv_obj_align(portal_stop_btn, LV_ALIGN_BOTTOM_RIGHT, -10, -10);
    lv_obj_set_style_border_color(portal_stop_btn, lv_color_hex(EMBED_COLOR_BORDER), LV_PART_MAIN);
    lv_obj_set_style_border_width(portal_stop_btn, 1, LV_PART_MAIN);
    apply_no_shadow(portal_stop_btn);
    apply_bg_color(portal_stop_btn);
    lv_obj_remove_style(portal_stop_btn, NULL, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_pad(portal_stop_btn, 2, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_width(portal_stop_btn, 2, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_color(portal_stop_btn, lv_color_hex(EMBED_COLOR_FOCUS_ON), LV_STATE_FOCUS_KEY);
    lv_obj_add_event_cb(portal_stop_btn, portal_stop_btn_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *stop_label = lv_label_create(portal_stop_btn);
    apply_text_color(stop_label);
    lv_obj_set_style_text_font(stop_label, FONT_BOLD_14, LV_PART_MAIN);
    lv_label_set_text(stop_label, "Stop");
    lv_obj_center(stop_label);
    lv_group_add_obj(lv_group_get_default(), portal_stop_btn);
    lv_obj_add_flag(portal_stop_btn, LV_OBJ_FLAG_HIDDEN);  // Hidden by default
}

void entry12(void) {
    portal_update_display();
    portal_update_timer = lv_timer_create(portal_timer_cb, 1000, NULL);
    lv_group_set_wrap(lv_group_get_default(), true);
}

void exit12(void) {
    if (portal_update_timer) {
        lv_timer_del(portal_update_timer);
        portal_update_timer = NULL;
    }

    // Clean up keyboard and textarea if they exist
    if (portal_ssid_keyboard) {
        lv_obj_del(portal_ssid_keyboard);
        portal_ssid_keyboard = NULL;
    }
    if (portal_ssid_textarea) {
        lv_obj_del(portal_ssid_textarea);
        portal_ssid_textarea = NULL;
    }

    lv_group_set_wrap(lv_group_get_default(), false);
}

void destroy12(void) {
}

static scr_lifecycle_t screen12 = {
    .create = create12,
    .entry = entry12,
    .exit  = exit12,
    .destroy = destroy12,
};

//************************************[ screen 12_1 ]**************************************** Portal Data Viewer

// Load a specific record from post.json
String loadJsonRecord(int index) {
    if (!sd_is_valid()) {
        return "SD card not available";
    }

    File file = SD.open(PORTAL_JSON_PATH, FILE_READ);
    if (!file) {
        return "No data file found";
    }

    String record = "";
    int currentLine = 0;

    while (file.available()) {
        String line = file.readStringUntil('\n');
        if (currentLine == index) {
            record = line;
            break;
        }
        currentLine++;
    }

    file.close();

    if (record.length() == 0) {
        return "Record not found";
    }

    return record;
}

// Count total records in post.json
int countJsonRecords() {
    if (!sd_is_valid()) {
        return 0;
    }

    File file = SD.open(PORTAL_JSON_PATH, FILE_READ);
    if (!file) {
        return 0;
    }

    int count = 0;
    while (file.available()) {
        file.readStringUntil('\n');
        count++;
    }

    file.close();
    return count;
}

// Update the data viewer display
static void update_data_viewer() {
    total_records = countJsonRecords();

    if (total_records == 0) {
        lv_label_set_text(portal_record_label, "Record: 0/0");
        lv_label_set_text(portal_data_display, "No data captured yet");
        return;
    }

    // Ensure index is in bounds
    if (current_record_index >= total_records) {
        current_record_index = total_records - 1;
    }
    if (current_record_index < 0) {
        current_record_index = 0;
    }

    // Update record counter
    char record_text[32];
    snprintf(record_text, sizeof(record_text), "Record: %d/%d", current_record_index + 1, total_records);
    lv_label_set_text(portal_record_label, record_text);

    // Load and display the record
    String record = loadJsonRecord(current_record_index);
    lv_label_set_text(portal_data_display, record.c_str());
}

static void portal_data_prev_cb(lv_event_t *e) {
    if (current_record_index > 0) {
        current_record_index--;
        update_data_viewer();
    }
}

static void portal_data_next_cb(lv_event_t *e) {
    if (current_record_index < total_records - 1) {
        current_record_index++;
        update_data_viewer();
    }
}

static void portal_data_back_cb(lv_event_t *e) {
    scr_mgr_switch(SCREEN12_ID, false);  // Back to portal screen
}

static void create12_1(lv_obj_t *parent) {
    scr12_1_cont = lv_obj_create(parent);
    lv_obj_set_size(scr12_1_cont, LV_PCT(100), LV_PCT(100));
    apply_bg_color(scr12_1_cont);
    apply_no_scrollbar(scr12_1_cont);
    apply_no_border(scr12_1_cont);
    apply_no_padding(scr12_1_cont);

    // Back button
    scr_back_btn_create(scr12_1_cont, portal_data_back_cb);

    // Title
    lv_obj_t *title = lv_label_create(scr12_1_cont);
    lv_label_set_text(title, "Portal Data");
    lv_obj_set_style_text_font(title, FONT_BOLD_20, 0);
    apply_text_color(title);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

    // Record counter label
    portal_record_label = lv_label_create(scr12_1_cont);
    lv_label_set_text(portal_record_label, "Record: 0/0");
    lv_obj_set_style_text_font(portal_record_label, FONT_BOLD_14, 0);
    apply_text_color(portal_record_label);
    lv_obj_align(portal_record_label, LV_ALIGN_TOP_MID, 0, 35);

    // Data display area (scrollable)
    lv_obj_t *data_cont = lv_obj_create(scr12_1_cont);
    lv_obj_set_size(data_cont, LV_PCT(90), 90);
    lv_obj_align(data_cont, LV_ALIGN_TOP_MID, 0, 60);
    lv_obj_set_style_border_width(data_cont, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(data_cont, lv_color_hex(EMBED_COLOR_BORDER), LV_PART_MAIN);
    apply_bg_color(data_cont);
    lv_obj_set_style_pad_all(data_cont, 8, LV_PART_MAIN);

    portal_data_display = lv_label_create(data_cont);
    lv_label_set_text(portal_data_display, "Loading...");
    lv_label_set_long_mode(portal_data_display, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(portal_data_display, LV_PCT(95));
    lv_obj_set_style_text_font(portal_data_display, FONT_BOLD_14, 0);
    apply_text_color(portal_data_display);

    // Previous button (bottom left)
    lv_obj_t *prev_btn = lv_btn_create(scr12_1_cont);
    lv_obj_set_size(prev_btn, 70, 32);
    lv_obj_align(prev_btn, LV_ALIGN_BOTTOM_LEFT, 10, -10);
    lv_obj_set_style_border_color(prev_btn, lv_color_hex(EMBED_COLOR_BORDER), LV_PART_MAIN);
    lv_obj_set_style_border_width(prev_btn, 1, LV_PART_MAIN);
    apply_no_shadow(prev_btn);
    apply_bg_color(prev_btn);
    lv_obj_remove_style(prev_btn, NULL, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_pad(prev_btn, 2, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_width(prev_btn, 2, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_color(prev_btn, lv_color_hex(EMBED_COLOR_FOCUS_ON), LV_STATE_FOCUS_KEY);
    lv_obj_add_event_cb(prev_btn, portal_data_prev_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *prev_label = lv_label_create(prev_btn);
    apply_text_color(prev_label);
    lv_obj_set_style_text_font(prev_label, FONT_BOLD_14, LV_PART_MAIN);
    lv_label_set_text(prev_label, "Prev");
    lv_obj_center(prev_label);
    lv_group_add_obj(lv_group_get_default(), prev_btn);

    // Next button (bottom right)
    lv_obj_t *next_btn = lv_btn_create(scr12_1_cont);
    lv_obj_set_size(next_btn, 70, 32);
    lv_obj_align(next_btn, LV_ALIGN_BOTTOM_RIGHT, -10, -10);
    lv_obj_set_style_border_color(next_btn, lv_color_hex(EMBED_COLOR_BORDER), LV_PART_MAIN);
    lv_obj_set_style_border_width(next_btn, 1, LV_PART_MAIN);
    apply_no_shadow(next_btn);
    apply_bg_color(next_btn);
    lv_obj_remove_style(next_btn, NULL, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_pad(next_btn, 2, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_width(next_btn, 2, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_color(next_btn, lv_color_hex(EMBED_COLOR_FOCUS_ON), LV_STATE_FOCUS_KEY);
    lv_obj_add_event_cb(next_btn, portal_data_next_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *next_label = lv_label_create(next_btn);
    apply_text_color(next_label);
    lv_obj_set_style_text_font(next_label, FONT_BOLD_14, LV_PART_MAIN);
    lv_label_set_text(next_label, "Next");
    lv_obj_center(next_label);
    lv_group_add_obj(lv_group_get_default(), next_btn);
}

void entry12_1(void) {
    current_record_index = 0;  // Start at first record
    update_data_viewer();
    lv_group_set_wrap(lv_group_get_default(), true);

    // Force display refresh
    lv_refr_now(NULL);
    tft.drawPixel(0, 0, 0);  // Dummy write to sync SPI
}

void exit12_1(void) {
    lv_group_set_wrap(lv_group_get_default(), false);
}

void destroy12_1(void) {
}

static scr_lifecycle_t screen12_1 = {
    .create = create12_1,
    .entry = entry12_1,
    .exit  = exit12_1,
    .destroy = destroy12_1,
};

// Override the weak function from portal.cpp to update UI
void printPortalHomeToScreen() {
    Serial.printf("[Portal] SSID: %s\n", portalSsidName.c_str());
    Serial.printf("[Portal] IP: %s\n", AP_GATEWAY.toString().c_str());
    Serial.printf("[Portal] Victims: %d\n", totalCapturedCredentials);

    // Update UI if we're on the portal screen
    portal_update_display();
}
#endif
//************************************[ UI ENTRY ]******************************************

void charge_detection_timer_cb(lv_timer_t *t)
{
    // wifi
    if(wifi_is_connect) {
        lv_obj_clear_flag(menu_taskbar_wifi, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(menu_taskbar_wifi, LV_OBJ_FLAG_HIDDEN);
    }

    // wifi
    if(sd_init_flag) {
        lv_obj_clear_flag(menu_taskbar_sd, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(menu_taskbar_sd, LV_OBJ_FLAG_HIDDEN);
    }

    // charge
    if(PPM.isCharging()) {
        lv_obj_clear_flag(menu_taskbar_charge, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(menu_taskbar_charge, LV_OBJ_FLAG_HIDDEN);
    }
    if(bq27220.getCharingFinish()){
        lv_label_set_text_fmt(menu_taskbar_charge, "#%x %s #", EMBED_COLOR_TEXT, LV_SYMBOL_OK);
    } else {
        lv_label_set_text_fmt(menu_taskbar_charge, "#%x %s #", EMBED_COLOR_TEXT, LV_SYMBOL_CHARGE);
    }

    // battery
    lv_label_set_text_fmt(menu_taskbar_battery, "#%x %s #", EMBED_COLOR_TEXT, ui_get_battert_level());
    
    // battery percent
    update_battery_percent_label(menu_taskbar_battery_percent);
}

void ui_entry(void)
{
    lv_init();
    lv_port_disp_init();
    lv_port_indev_init();

    taskbar_timer = lv_timer_create(charge_detection_timer_cb, 1000, NULL);
    lv_timer_pause(taskbar_timer);

    scr_mgr_init();
    ui_theme_setting(setting_theme);
    scr_mgr_register(SCREEN0_ID, &screen0);      // menu
    scr_mgr_register(SCREEN1_ID, &screen1);      // ws2812
    scr_mgr_register(SCREEN2_ID, &screen2);      // SubGHz menu
    scr_mgr_register(SCREEN2_1_ID, &screen2_1);  //   -Record Raw
    scr_mgr_register(SCREEN2_1_1_ID, &screen2_1_1);  //     -Frequency Selector
    scr_mgr_register(SCREEN2_2_ID, &screen2_2);  //   -Playback
    scr_mgr_register(SCREEN2_2_1_ID, &screen2_2_1);  //     -Signal Details
    scr_mgr_register(SCREEN2_3_ID, &screen2_3);  //   -Scan/Record
    scr_mgr_register(SCREEN2_3_1_ID, &screen2_3_1);  //     -Scan/Record Frequency Mode Selector
    scr_mgr_register(SCREEN2_3_1_1_ID, &screen2_3_1_1);  //       -Single Frequency Selector
    scr_mgr_register(SCREEN2_3_1_2_ID, &screen2_3_1_2);  //       -Range Selector
    scr_mgr_register(SCREEN2_3_1_3_ID, &screen2_3_1_3);  //       -Custom Frequency Selector
    scr_mgr_register(SCREEN2_3_1_4_ID, &screen2_3_1_4);  //       -Step Size Selector
    scr_mgr_register(SCREEN2_4_ID, &screen2_4);  //   -Spectrum Analyzer
    scr_mgr_register(SCREEN2_4_1_ID, &screen2_4_1);  //     -Spectrum Range Selector
    scr_mgr_register(SCREEN2_5_ID, &screen2_5);  //   -SubGHz Remotes
    scr_mgr_register(SCREEN2_5_1_ID, &screen2_5_1);  //     -SubGHz File Browser
    scr_mgr_register(SCREEN2_6_ID, &screen2_6);  //   -Custom Frequencies
    scr_mgr_register(SCREEN2_6_1_ID, &screen2_6_1);  //     -Add Frequency (keyboard)
    scr_mgr_register(SCREEN3_ID, &screen3);      // nfc file browser
    scr_mgr_register(SCREEN3_1_ID, &screen3_1);  //   -Read NFC Tag
    scr_mgr_register(SCREEN3_2_ID, &screen3_2);  //   -NFC Tag Detail
    scr_mgr_register(SCREEN3_2_1_ID, &screen3_2_1);  //     -NDEF Record Detail
    scr_mgr_register(SCREEN4_ID, &screen4);      // setting
    scr_mgr_register(SCREEN4_1_ID,  &screen4_1); // setting - about
    scr_mgr_register(SCREEN5_ID, &screen5);      // battery
    scr_mgr_register(SCREEN6_ID, &screen6);      // wifi menu
    scr_mgr_register(SCREEN6_1_ID, &screen6_1);  //   -WiFi Scanner
    scr_mgr_register(SCREEN6_1_1_ID, &screen6_1_1);  //     -AP Details
    scr_mgr_register(SCREEN6_2_ID, &screen6_2);  //   -Deauth Hunter
    scr_mgr_register(SCREEN6_3_ID, &screen6_3);  //   -PineAP Hunter
    scr_mgr_register(SCREEN7_ID, &screen7);      // IR menu
    scr_mgr_register(SCREEN7_1_ID, &screen7_1);  //   -TV-B-Gone
    scr_mgr_register(SCREEN7_3_ID, &screen7_3);  //   -IR Playback
    scr_mgr_register(SCREEN7_4_ID, &screen7_4);  //     -IR Button Capture/Edit
    scr_mgr_register(SCREEN11_ID, &screen11);    // MIC
    scr_mgr_register(SCREEN10_ID, &screen10);    // SD Card
    scr_mgr_register(SCREEN10_1_ID, &screen10_1);//   -File Browser
    scr_mgr_register(SCREEN10_2_ID, &screen10_2);//     -Audio Playback
    scr_mgr_register(SCREEN10_3_ID, &screen10_3);//     -Text Viewer
    scr_mgr_register(SCREEN12_ID, &screen12);    // Portal
    scr_mgr_register(SCREEN12_1_ID, &screen12_1);//   -Portal Data Viewer
    scr_mgr_register(SCREEN8_ID, &screen8);      // music
    scr_mgr_register(SCREEN9_ID, &screen9);      // nrf24

    scr_mgr_switch(SCREEN0_ID, false); // main scr
}
