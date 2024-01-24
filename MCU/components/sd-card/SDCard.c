#include "SDCard.h"

/* Source: https://git.kernel.org/pub/scm/utils/mmc/mmc-utils.git/tree/lsmmc.c */
/* SD Card Manufacturer Database */
typedef struct SDCardManufacturerDB {
    uint32_t id;
    const char *type;
    const char *manufacturer;
} SDCardManufacturerDB;

/* Source: https://git.kernel.org/pub/scm/utils/mmc/mmc-utils.git/tree/lsmmc.c */
/* SD Card Manufacturer Database */
const SDCardManufacturerDB SD_CARD_DATABASE[] = {
        {.id = 0x01, .type = "sd", .manufacturer = "Panasonic"},
        {.id = 0x02, .type = "sd", .manufacturer = "Toshiba/Kingston/Viking"},
        {.id = 0x03, .type = "sd", .manufacturer = "SanDisk"},
        {.id = 0x08, .type = "sd", .manufacturer = "Silicon Power"},
        {.id = 0x18, .type = "sd", .manufacturer = "Infineon"},
        {.id = 0x1b, .type = "sd", .manufacturer = "Transcend/Samsung"},
        {.id = 0x1c, .type = "sd", .manufacturer = "Transcend"},
        {.id = 0x1d, .type = "sd", .manufacturer = "Corsair/AData"},
        {.id = 0x1e, .type = "sd", .manufacturer = "Transcend"},
        {.id = 0x1f, .type = "sd", .manufacturer = "Kingston"},
        {.id = 0x27, .type = "sd", .manufacturer = "Delkin/Phison"},
        {.id = 0x28, .type = "sd", .manufacturer = "Lexar"},
        {.id = 0x30, .type = "sd", .manufacturer = "SanDisk"},
        {.id = 0x31, .type = "sd", .manufacturer = "Silicon Power"},
        {.id = 0x33, .type = "sd", .manufacturer = "STMicroelectronics"},
        {.id = 0x41, .type = "sd", .manufacturer = "Kingston"},
        {.id = 0x6f, .type = "sd", .manufacturer = "STMicroelectronics"},
        {.id = 0x74, .type = "sd", .manufacturer = "Transcend"},
        {.id = 0x76, .type = "sd", .manufacturer = "Patriot"},
        {.id = 0x82, .type = "sd", .manufacturer = "Gobe/Sony"},
        {.id = 0x89, .type = "sd", .manufacturer = "Unknown"}};

static const char *TAG = "SDCard";

static sdmmc_cid_t sdCardCid;
static sdmmc_csd_t sdCardCsd;

static const char *getSDCardManufacturerById(uint32_t id);

SDCardStatus initNvsSDCard() {
    esp_err_t status = nvs_flash_init();
    if (status == ESP_ERR_NVS_NO_FREE_PAGES) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        nvs_flash_init();
    }

    // Options for mounting the filesystem.
    // If format_if_mount_failed is set to true, SD card will be partitioned and
    // formatted in case when mounting fails.
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
            .format_if_mount_failed = false,
            .max_files = MAX_FILES_IN_DIR,
            .allocation_unit_size = 16 * ONE_KB};

    LOG_INFO(TAG, "Initializing SD card with root: [%s]", SD_CARD_ROOT);
    LOG_DEBUG(TAG, "Using SDMMC peripheral");

    // By default, SD card frequency is initialized to SDMMC_FREQ_DEFAULT (20MHz)
    // For setting a specific frequency, use host.max_freq_khz (range 400kHz - 40MHz for SDMMC)
    // Example: for fixed frequency of 10MHz, use host.max_freq_khz = 10000;
    sdmmc_host_t host = SDMMC_HOST_DEFAULT();

    // This initializes the slot without card detect (CD) and write protect (WP) signals.
    // Modify slot_config.gpio_cd and slot_config.gpio_wp if your board has these signals.
    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();

// To use 1-line SD mode, uncomment the following line:
#ifdef SD_CARD_USE_ONE_LINE_MODE
    slot_config.width = 1;
#endif

    // GPIOs 15, 2, 4, 12, 13 should have external 10k pull-ups.
    // Internal pull-ups are not sufficient. However, enabling internal pull-ups
    // does make a difference some boards, so we do that here.
    gpio_set_pull_mode(GPIO_NUM_15, GPIO_PULLUP_ONLY);// CMD, needed in 4- and 1- line modes
    gpio_set_pull_mode(GPIO_NUM_2, GPIO_PULLUP_ONLY); // D0, needed in 4- and 1-line modes
#ifndef SD_CARD_USE_ONE_LINE_MODE
    gpio_set_pull_mode(GPIO_NUM_4, GPIO_PULLUP_ONLY); // D1, needed in 4-line mode only
    gpio_set_pull_mode(GPIO_NUM_12, GPIO_PULLUP_ONLY);// D2, needed in 4-line mode only
#endif
    gpio_set_pull_mode(GPIO_NUM_13, GPIO_PULLUP_ONLY);// D3, needed in 4- and 1-line modes

    // Use settings defined above to initialize SD card and mount FAT filesystem.
    // Note: esp_vfs_fat_sdmmc_mount is an all-in-one convenience function.
    // Please check its source code and implement error recovery when developing
    // production applications.
    LOG_INFO(TAG, "Mounting filesystem");
    sdmmc_card_t *card;
    status = esp_vfs_fat_sdmmc_mount(SD_CARD_ROOT, &host, &slot_config, &mount_config, &card);

    if (status != ESP_OK) {
        if (status == ESP_FAIL) {
            LOG_ERROR(TAG, "Failed to mount FAT filesystem on SD card. Check SD card filesystem (only FAT supported) or try another card");
            return SD_CARD_FAILED_TO_MOUNT_FS;

        } else if (status == 263) {     // Error code: 0x107 --> usually: SD not found
            LOG_ERROR(TAG, "SD card init failed. Check if SD card is properly inserted into SD card slot or try another card");
            return SD_CARD_NOT_FOUND;

        } else {
            LOG_ERROR(TAG, "SD card init failed. Error code: [%d]", status);
            return SD_CARD_UNKNOWN_FAIL;
        }
    }
    LOG_INFO(TAG, "File system successfully mounted");

    // save card info
    sdCardCid = card->cid;
    sdCardCsd = card->csd;
    return SD_CARD_OK;
}

SDCardStatus checkSDCardRW() {
    LOG_INFO(TAG, "Basic R/W check started...");
    File *cardRoot = NEW_FILE(SD_CARD_ROOT);
    File *checkFile = FILE_OF(cardRoot, "/sdcheck.txt");
    LOG_DEBUG(TAG, "Check Path: [%s]", checkFile->path);

    if (!createFile(checkFile)) {
        LOG_ERROR(TAG, "Basic R/W check: (E%d) No able to create file to write", SD_CARD_ERROR_CREATE_FILE);
        return SD_CARD_ERROR_CREATE_FILE;
    }

    BufferString *testMessage = NEW_STRING_64("This message is used for a SD-Card basic check!");
    uint16_t crcMessage = generateCRC16(testMessage->value, testMessage->length);
    uint32_t writtenDataLength = writeStringToFile(checkFile, testMessage, false);
    if (writtenDataLength != testMessage->length) {
        LOG_ERROR(TAG, "Basic R/W check: (E%d) Not able to write file", SD_CARD_ERROR_WRIRE_TO_FILE);
        return SD_CARD_ERROR_WRIRE_TO_FILE;
    }

    clearString(testMessage);
    uint32_t readDataLen = readFileToString(checkFile, testMessage);
    if (readDataLen != writtenDataLength) {
        LOG_ERROR(TAG, "Basic R/W check: (E%d) Not able to open file to read back", SD_CARD_ERROR_READ_FROM_FILE);
        return SD_CARD_ERROR_READ_FROM_FILE;
    }

    uint16_t crcResultMessage = generateCRC16(testMessage->value, testMessage->length);
    if (crcResultMessage != crcMessage) {
        LOG_ERROR(TAG, "Basic R/W check: (E%d) Read back, but wrong CRC", SD_CARD_ERROR_CRC);
        return SD_CARD_ERROR_CRC;
    }

    if (remove(checkFile->path) != 0) {
        LOG_ERROR(TAG, "Basic R/W check: (E%d) Unable to delete the file", SD_CARD_ERROR_DELETE_FILE);
        return SD_CARD_ERROR_DELETE_FILE;
    }

    LOG_INFO(TAG, "Basic R/W check successful");
    return SD_CARD_OK;
}


uint32_t getSDCardPartitionSize() {// Get volume information and free clusters of drive 0
    FATFS *fileSystem;
    uint32_t freeClusters;
    f_getfree("0:", (DWORD *) &freeClusters, &fileSystem);
    return ((fileSystem->n_fatent - 2) * fileSystem->csize) / 1024 / (1024 / sdCardCsd.sector_size);//corrected by SD Card sector size (usually 512 bytes) and convert to MB
}

uint32_t getSDCardFreePartitionSpace() {// Get volume information and free clusters of drive 0
    FATFS *fileSystem;
    uint32_t freeClusters;
    f_getfree("0:", (DWORD *) &freeClusters, &fileSystem);
    return (freeClusters * fileSystem->csize) / 1024 / (1024 / sdCardCsd.sector_size);//corrected by SD Card sector size (usually 512 bytes) and convert to MB
}

uint32_t getSDCardPartitionAllocationSize() {// Get volume information and free clusters of drive 0
    FATFS *fileSystem;
    uint32_t freeClusters;
    f_getfree("0:", (DWORD *) &freeClusters, &fileSystem);
    return fileSystem->ssize;
}

void getSDCardManufacturer(BufferString *str) {
    const char *cardManufacturer = getSDCardManufacturerById(sdCardCid.mfg_id);
    stringFormat(str, "%s (ID: %d)", cardManufacturer, sdCardCid.mfg_id);
}

void getSDCardName(BufferString *str) {
    concatChars(str, sdCardCid.name);
}

uint32_t getSDCardCapacity() {
    return sdCardCsd.capacity / (1024 / sdCardCsd.sector_size) / 1024;// total sectors * sector size  --> Byte to MB (1024*1024)
}

uint32_t getSDCardSectorSize() {
    return sdCardCsd.sector_size;
}

void logSDCardInformation() {
    uint32_t cardPartitionSize = getSDCardPartitionSize();
    uint32_t cardFreePartitionSpace = getSDCardFreePartitionSpace();
    uint32_t cardAllocationSize = getSDCardPartitionAllocationSize();
    uint32_t cardCapacity = getSDCardCapacity();
    uint32_t cardSectorSize = getSDCardSectorSize();

    BufferString *manufacturer = EMPTY_STRING(32);
    getSDCardManufacturer(manufacturer);
    BufferString *cardName = EMPTY_STRING(8);
    getSDCardName(cardName);

    LOG_INFO(TAG, "\n[%d] MB total drive space\n "
                  "[%d] MB free drive space\n "
                  "SD Card Partition Allocation Size: [%d] bytes\n "
                  "SD Card Manufacturer: [%s]\n "
                  "SD Card Name: [%s]\n SD"
                  "Card Capacity: [%d]\n "
                  "SD Card Sector Size: [%d] bytes",
             cardPartitionSize,
             cardFreePartitionSpace,
             cardAllocationSize,
             manufacturer->value,
             cardName->value,
             cardCapacity,
             cardSectorSize);
}

static const char *getSDCardManufacturerById(uint32_t id) {// Parse SD Card Manufacturer Database
    for (int i = 0; i < ARRAY_SIZE(SD_CARD_DATABASE); i++) {
        const SDCardManufacturerDB *cardData = &SD_CARD_DATABASE[i];
        if (cardData->id == id) {
            return cardData->manufacturer;
        }
    }
    return "ID unknown (not in DB)";
}