#pragma once

#include "../../src/AppConfig.h"

#include "esp_pm.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "driver/sdmmc_host.h"
#include "nvs.h"
#include "nvs_flash.h"

typedef enum SDCardStatus {
    SD_CARD_OK = 0,
    SD_CARD_FAILED_TO_MOUNT_FS = 1,
    SD_CARD_UNKNOWN_FAIL = 2,
    SD_CARD_NOT_FOUND = 3,
    SD_CARD_ERROR_CREATE_FILE = 4,
    SD_CARD_ERROR_WRIRE_TO_FILE = 5,
    SD_CARD_ERROR_READ_FROM_FILE = 6,
    SD_CARD_ERROR_CRC = 7,
    SD_CARD_ERROR_DELETE_FILE = 8
} SDCardStatus;


SDCardStatus initNvsSDCard();
SDCardStatus checkSDCardRW();

uint32_t getSDCardPartitionSize();
uint32_t getSDCardFreePartitionSpace();
uint32_t getSDCardPartitionAllocationSize();
void getSDCardManufacturer(BufferString *str);
void getSDCardName(BufferString *str);
uint32_t getSDCardCapacity();
uint32_t getSDCardSectorSize();

void logSDCardInformation();