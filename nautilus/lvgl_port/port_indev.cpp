/**
 * @file lv_port_indev.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "port_indev.h"
#include "lvgl.h"
#include "utilities.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/
RotaryEncoder encoder(ENCODER_INA, ENCODER_INB, RotaryEncoder::LatchMode::TWO03);

volatile bool indev_encoder_enabled = true;
volatile bool indev_keypad_enabled = true;

/**********************
 *  STATIC PROTOTYPES
 **********************/

static void keypad_init(void);
static void keypad_read(lv_indev_drv_t * indev_drv, lv_indev_data_t * data);
static uint32_t keypad_get_key(void);

static void encoder_init(void);
static void encoder_read(lv_indev_drv_t * indev_drv, lv_indev_data_t * data);
static void encoder_handler(void);

/**********************
 *  STATIC VARIABLES
 **********************/
lv_indev_t * indev_keypad;
lv_indev_t * indev_encoder;

static int32_t encoder_diff;
static lv_indev_state_t encoder_state;

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void lv_port_indev_init(void)
{
    static lv_indev_drv_t indev_drv;
    static lv_indev_drv_t indev_drv_keypad;


    /*------------------
     * Encoder
     * -----------------*/

    /*Initialize your encoder if you have*/
    encoder_init();

    /*Register a encoder input device*/
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_ENCODER;
    indev_drv.read_cb = encoder_read;
    indev_encoder = lv_indev_drv_register(&indev_drv);
    lv_group_t *group = lv_group_create();
    lv_indev_set_group(indev_encoder, group);
    lv_group_set_default(group);

    /*Later you should create group(s) with `lv_group_t * group = lv_group_create()`,
     *add objects to the group with `lv_group_add_obj(group, obj)`
     *and assign this input device to group to navigate in it:
     *`lv_indev_set_group(indev_encoder, group);`*/

    /*------------------
     * Keypad
     * -----------------*/

    /*Initialize your keypad or keyboard if you have*/
    keypad_init();

    /*Register a keypad input device*/
    lv_indev_drv_init(&indev_drv_keypad);
    indev_drv_keypad.type = LV_INDEV_TYPE_KEYPAD;
    indev_drv_keypad.read_cb = keypad_read;
    indev_keypad = lv_indev_drv_register(&indev_drv_keypad);
    // lv_group_t *keypad_group = lv_group_create();
    lv_indev_set_group(indev_keypad, group);

    /*Later you should create group(s) with `lv_group_t * group = lv_group_create()`,
     *add objects to the group with `lv_group_add_obj(group, obj)`
     *and assign this input device to group to navigate in it:
     *`lv_indev_set_group(indev_keypad, group);`*/

}

/**********************
 *   STATIC FUNCTIONS
 **********************/

/*------------------
 * Keypad
 * -----------------*/
// extern uint16_t infared_get_cmd(void);

static void keypad_init(void)
{
}

static void keypad_read(lv_indev_drv_t * indev_drv, lv_indev_data_t * data)
{
}

static uint32_t keypad_get_key(void)
{
    return 0;
}
/*------------------
 * Encoder
 * -----------------*/
int indev_encoder_pos = 0;
static void encoder_init(void)
{
    pinMode(ENCODER_KEY, INPUT);
    attachInterrupt(ENCODER_INA, encoder_handler, CHANGE);
    attachInterrupt(ENCODER_INB, encoder_handler, CHANGE);
}

/*Will be called by the library to read the encoder*/
static void encoder_read(lv_indev_drv_t * indev_drv, lv_indev_data_t * data)
{
    // Always update position for volume control during playback
    indev_encoder_pos = encoder.getPosition();

    if(indev_encoder_enabled){
        int encoder_dir = (int16_t)encoder.getDirection();
        if(encoder_dir != 0){
            data->enc_diff = -encoder_dir;
        }
    } else {
        data->enc_diff = 0;
        data->state = LV_INDEV_STATE_REL;
    }

    if (digitalRead(ENCODER_KEY) == LOW) {
        data->state = LV_INDEV_STATE_PR;
    } else if (digitalRead(ENCODER_KEY) == HIGH) {
        data->state = LV_INDEV_STATE_REL;
    }
    // Serial.printf("enc_diff:%d, sta:%d\n",data->enc_diff, data->state);
}

/*Call this function in an interrupt to process encoder events (turn, press)*/
static void encoder_handler(void)
{
    /*Your code comes here*/
    // Always tick the encoder to track position, even when LVGL input is disabled
    // This allows volume control during music playback
    encoder.tick();
}

void lv_port_indev_enabled(bool en)
{
    indev_encoder_enabled = en;
    indev_keypad_enabled = en;
}

int lv_port_indev_get_pos(void)
{
    return indev_encoder_pos;
}


