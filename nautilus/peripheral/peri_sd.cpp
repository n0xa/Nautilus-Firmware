#include "peripheral.h"

bool sd_init_flag = false;
uint32_t sd_sum_Mbyte = 0;
uint32_t sd_used_Mbyte = 0;

void sd_init(void)
{
    SPI.end();
    SPI.begin(BOARD_SPI_SCK, BOARD_SPI_MISO, BOARD_SPI_MOSI); 

    if(!SD.begin(BOARD_SD_CS)){
        return;
    }

    uint8_t cardType = SD.cardType();

    if(cardType == CARD_NONE){
        return;
    }

    if(cardType == CARD_MMC){
    } else if(cardType == CARD_SD){
    } else if(cardType == CARD_SDHC){
    } else {
    }

    uint64_t cardSize = SD.cardSize() / (1024 * 1024);
    sd_sum_Mbyte = (SD.totalBytes() / (1024 * 1024));

    // SD.usedBytes() uses FATFS f_getfree() - gets stats from filesystem metadata (no file scanning needed)
    sd_used_Mbyte = SD.usedBytes() / (1024 * 1024);


    sd_init_flag = true;
}

bool sd_is_valid(void)
{
    return sd_init_flag;
}

uint32_t sd_get_sum_Mbyte(void)
{
    return sd_sum_Mbyte;
}

uint32_t sd_get_used_Mbyte(void)
{
    return sd_used_Mbyte;
}

bool sd_mount(void)
{
    // If already mounted, unmount first
    if(sd_init_flag) {
        sd_unmount();
    }

    SPI.end();
    SPI.begin(BOARD_SPI_SCK, BOARD_SPI_MISO, BOARD_SPI_MOSI);

    if(!SD.begin(BOARD_SD_CS)){
        sd_init_flag = false;
        return false;
    }

    uint8_t cardType = SD.cardType();

    if(cardType == CARD_NONE){
        sd_init_flag = false;
        return false;
    }

    if(cardType == CARD_MMC){
    } else if(cardType == CARD_SD){
    } else if(cardType == CARD_SDHC){
    } else {
    }

    uint64_t cardSize = SD.cardSize() / (1024 * 1024);
    sd_sum_Mbyte = (SD.totalBytes() / (1024 * 1024));

    // SD.usedBytes() uses FATFS f_getfree() - gets stats from filesystem metadata (no file scanning needed)
    sd_used_Mbyte = SD.usedBytes() / (1024 * 1024);


    sd_init_flag = true;
    return true;
}

void sd_unmount(void)
{
    if(sd_init_flag) {
        SD.end();
    }
    sd_init_flag = false;
    sd_sum_Mbyte = 0;
    sd_used_Mbyte = 0;
}