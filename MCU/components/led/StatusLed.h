#pragma once

#include "../../src/AppConfig.h"

#include <sys/types.h>
#include <sys/stat.h>
#include "driver/gpio.h"
#include "rom/gpio.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "EspConfig.h"

extern TaskHandle_t xHandleTaskStatusLED;

typedef enum StatusLedSource {
	WLAN_CONNECTION_ERROR = 1,
    WLAN_INIT_ERROR = 2,
    SD_CARD_INIT_ERROR = 3,
	SD_CARD_CHECK_ERROR = 4,
    CONFIG_PROP_ERROR = 5,
    CAMERA_INIT_ERROR = 6,
    PSRAM_INIT_ERROR = 7,
    TIME_CHECK_ERROR = 8,
    AP_OR_OTA_ENABLED = 9
} StatusLedSource;

typedef struct StatusLEDData {
    uint32_t sourceBlinkCount;
    uint32_t codeBlinkCount;
    uint32_t blinkTimeMs;
    bool isInfiniteBlink;
    bool processingRequest;
} StatusLEDData;

void statusLed(StatusLedSource errorSource, uint8_t code, bool isInfiniteBlink);

void statusLedOff();
void statusLedOn();