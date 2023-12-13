#include "StatusLed.h"

#define INIT_LED_DATA(errorSource, code, isInfiniteBlink) (StatusLEDData) {errorSource, code, 250, isInfiniteBlink, false}
#define REPEAT_TIMES 2

static const char *TASK_NAME = "statusLedTask";
static const char *TAG = "STATUSLED";

TaskHandle_t xHandleTaskStatusLED = NULL;
StatusLEDData statusLedData = INIT_LED_DATA(1, 1, false);

static void initLedBlinkGpio();
static void ledBlink(uint32_t statusModeCounter, uint32_t delayMs);


void statusLedTask(void *pvParameter) {
    LOG_DEBUG(TAG, "[%s] - create", TASK_NAME);

    while(statusLedData.processingRequest) {
        LOG_DEBUG(TAG, "[%s] - start", TASK_NAME);

        initLedBlinkGpio();
        gpio_set_level(BLINK_GPIO, 1);  // LED off

        for (uint8_t i = 0; i < REPEAT_TIMES;) {
            if (!statusLedData.isInfiniteBlink) {
                i++;
            }

            ledBlink(statusLedData.sourceBlinkCount, statusLedData.blinkTimeMs);
            vTaskDelay(500 / portTICK_PERIOD_MS);	// Delay between module code and error code

            ledBlink(statusLedData.codeBlinkCount, statusLedData.blinkTimeMs);
			vTaskDelay(1500 / portTICK_PERIOD_MS);	// Delay to signal new round
        }

        statusLedData.processingRequest = false;
        LOG_DEBUG(TAG, "[%s] - done/wait", TASK_NAME);
        vTaskDelay(10000 / portTICK_PERIOD_MS);	// Wait for an upcoming request otherwise continue and delete task to save memory
    }

    LOG_DEBUG(TAG, "[%s] - delete", TASK_NAME);
    xHandleTaskStatusLED = NULL;
    vTaskDelete(NULL); // Delete this task due to no request
}

void statusLed(StatusLedSource errorSource, uint8_t code, bool isInfiniteBlink) {
    LOG_DEBUG(TAG, "[%s] - start. Params source: [%d], code: [%d]", TASK_NAME, errorSource, code);

    switch (errorSource) {
        case WLAN_CONNECTION_ERROR:
        case WLAN_INIT_ERROR:
        case SD_CARD_INIT_ERROR:
        case SD_CARD_CHECK_ERROR:
        case CONFIG_PROP_ERROR:
        case CAMERA_INIT_ERROR:
        case PSRAM_INIT_ERROR:
        case TIME_CHECK_ERROR:
        case AP_OR_OTA_ENABLED:
            statusLedData = INIT_LED_DATA(errorSource, code, isInfiniteBlink);
            break;
        default:
            statusLedData = INIT_LED_DATA(1, 1, false);
            break;
    }

    if (xHandleTaskStatusLED != NULL && !statusLedData.processingRequest) {
        statusLedData.processingRequest = true;
        BaseType_t xReturned = xTaskAbortDelay(xHandleTaskStatusLED);	// Reuse still running status LED task

    } else if (xHandleTaskStatusLED == NULL) {
        statusLedData.processingRequest = true;
        BaseType_t xReturned = xTaskCreate(&statusLedTask, TASK_NAME, 2048, NULL, tskIDLE_PRIORITY + 1, &xHandleTaskStatusLED);
        if (xReturned != pdPASS) {
            xHandleTaskStatusLED = NULL;
            LOG_ERROR(TAG, "[%s] failed to create", TASK_NAME);
            LOG_DEBUG("HEAP", getEspHeapInfo()->value);
            return;
        }
        LOG_DEBUG(TAG, "[%s] - task created", TASK_NAME);

    } else {
        LOG_DEBUG(TAG, "[%s] still processing, request skipped", TASK_NAME);	// Requests with high frequency could be skipped, but LED is only helpful for static states 
    }

    LOG_DEBUG(TAG, "[%s] - done", TASK_NAME);
}

void statusLedOff() {
    if (xHandleTaskStatusLED != NULL) {
        vTaskDelete(xHandleTaskStatusLED); // Delete task for statusLedTask to force stop of blinking
        xHandleTaskStatusLED = NULL;
    }
    initLedBlinkGpio();
    gpio_set_level(BLINK_GPIO, 0);// LED off
}

void statusLedOn() {
    initLedBlinkGpio();
    gpio_set_level(BLINK_GPIO, 1);// LED on

}

static void initLedBlinkGpio() {
    gpio_pad_select_gpio(BLINK_GPIO); // Init the GPIO
	gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT); // Set the GPIO as a push/pull output
}

static void ledBlink(uint32_t statusModeCounter, uint32_t delayMs) {
    for (uint32_t j = 0; j < statusModeCounter; j++) {
        gpio_set_level(BLINK_GPIO, 1);      
        vTaskDelay(delayMs / portTICK_PERIOD_MS);
        gpio_set_level(BLINK_GPIO, 0);
        vTaskDelay(delayMs / portTICK_PERIOD_MS);
    }
}