
#include "peripheral.h"
#include "../lvgl_port/port_disp.h"
#include "radio_flags.h"

float sghz_freq = 315.0;

CC1101 radio = new Module(BOARD_SGHZ_CS, BOARD_SGHZ_IO0, -1, BOARD_SGHZ_IO2);
int sghz_mode = SGHZ_MODE_SEND;
int sghz_recv_success = 0;
int sghz_recv_rssi = 0;
int sghz_init_st = false;
String sghz_recv_str;

// Radio interrupt flags
DECLARE_RADIO_FLAG(received)
DECLARE_RADIO_FLAG(transmitted)
static int transmissionState = RADIOLIB_ERR_NONE;


void sghz_init(void)
{
    pinMode(BOARD_SGHZ_SW1, OUTPUT);
    pinMode(BOARD_SGHZ_SW0, OUTPUT);
    digitalWrite(BOARD_SGHZ_SW1, HIGH);
    digitalWrite(BOARD_SGHZ_SW0, LOW);
    sghz_freq = 315.0;

    SPI.end();
    SPI.begin(BOARD_SPI_SCK, BOARD_SPI_MISO, BOARD_SPI_MOSI);

    int state = radio.begin(sghz_freq);
    if (state == RADIOLIB_ERR_NONE) {
        sghz_init_st = true;
    } else {
        sghz_init_st = false;
        return;
    }

    if (radio.setFrequency(sghz_freq) == RADIOLIB_ERR_INVALID_FREQUENCY) {
        while (true);
    }

    radio.setOOK(true);
    if (state == RADIOLIB_ERR_INVALID_BIT_RATE) {
        while (true);
    }

    state = radio.setBitRate(1.2);
    if (state == RADIOLIB_ERR_INVALID_BIT_RATE) {
        while (true);
    }

    if (radio.setRxBandwidth(58.0) == RADIOLIB_ERR_INVALID_RX_BANDWIDTH) {
        while (true);
    }
    if (radio.setFrequencyDeviation(5.2) == RADIOLIB_ERR_INVALID_FREQUENCY_DEVIATION) {
        while (true);
    }

    // set output power to 5 dBm
    if (radio.setOutputPower(10) == RADIOLIB_ERR_INVALID_OUTPUT_POWER) {
        while (true);
    }

    // 2 bytes can be set as sync word
    if (radio.setSyncWord(0x01, 0x23) == RADIOLIB_ERR_INVALID_SYNC_WORD) {
        while (true);
    }

    if(sghz_mode == SGHZ_MODE_SEND){     // send
        radio.setPacketSentAction(transmittedSetFlag);

        // you can transmit C-string or Arduino string up to
        // 64 characters long
        transmissionState = radio.startTransmit("Hello World!");
    } else if(sghz_mode == SGHZ_MODE_RECV) { // recv

        radio.setPacketReceivedAction(receivedSetFlag);
        // start listening for packets
        state = radio.startReceive();
        if (state == RADIOLIB_ERR_NONE) {
        } else {
            while (true);
        }
    }
}

void sghz_mode_sw(int m)
{
    if(m == SGHZ_MODE_RECV) {
        radio.setPacketReceivedAction(receivedSetFlag);
        // start listening for packets
        int state = radio.startReceive();
        if (state == RADIOLIB_ERR_NONE) {
        } else {
            while (true);
        }
    } else if(m == SGHZ_MODE_SEND) {
        radio.setPacketSentAction(transmittedSetFlag);
    }
    sghz_mode = m;
}

int sghz_get_mode(void)
{
    return sghz_mode;
}

bool sghz_is_init(void)
{
    return sghz_init_st;
}

void sghz_send(const char *str)
{
    // check if the previous transmission finished
    if(xSemaphoreTake(radioLock, portMAX_DELAY) != pdTRUE){
        return;
    }

    if(CHECK_FLAG(transmitted)) {
        // reset flag
        RESET_FLAG(transmitted);

        if (transmissionState == RADIOLIB_ERR_NONE) {
            // packet was successfully sent

            // NOTE: when using interrupt-driven transmit method,
            //       it is not possible to automatically measure
            //       transmission data rate using getDataRate()
        } else {
        }

        // clean up after transmission is finished
        // this will ensure transmitter is disabled,
        // RF switch is powered down etc.
        radio.finishTransmit();

        // wait a second before transmitting again

        // send another one

        // you can transmit C-string or Arduino string up to
        // 256 characters long
        transmissionState = radio.startTransmit(str);
        if (transmissionState == RADIOLIB_ERR_NONE) {
            // packet was successfully sent

            // NOTE: when using interrupt-driven transmit method,
            //       it is not possible to automatically measure
            //       transmission data rate using getDataRate()
        } else {
        }
    }
    xSemaphoreGive(radioLock);
}



void sghz_recv(void)
{
    // check if the flag is set
    if(xSemaphoreTake(radioLock, portMAX_DELAY) != pdTRUE){
        return;
    }

    if(CHECK_FLAG(received)) {
        // reset flag
        RESET_FLAG(received);

        disp_disable_update();

        // you can read received data as an Arduino String
        int state = radio.readData(sghz_recv_str);

        disp_enable_update();

        if (state == RADIOLIB_ERR_NONE) {
            sghz_recv_success = 1;
        // packet was successfully received

        // print data of the packet

        // print RSSI (Received Signal Strength Indicator)
        // of the last received packet
        sghz_recv_rssi = (int) radio.getRSSI();

        // print LQI (Link Quality Indicator)
        // of the last received packet, lower is better

        } else if (state == RADIOLIB_ERR_CRC_MISMATCH) {
        // packet was received, but is malformed

        } else {
        // some other error occurred

        }

        // put module back to listen mode
        radio.startReceive();
    }
    xSemaphoreGive(radioLock);
}

void sghz_task(void *param)
{
    vTaskSuspend(sghz_handle);
    while (1)
    {
        if(sghz_mode == SGHZ_MODE_RECV) {
            sghz_recv();
        }
        // String str = "Hello World! #" + String(count++);
        // sghz_send(str.c_str());
        // sghz_recv();
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
