#include "EspConfig.h"

#define MESSAGE_BUFFER_LENGTH 1024

static const char *TAG = "ESP_CONFIG";

BufferString espInfoStr = {0};


bool setupEspCpuFrequency() {
    char *cpuFrequency = getPropertyOrDefault(&appConfig, PROPERTY_SYSTEM_FREQUENCY_MHZ_KEY, "160");

    EspPowerManagementConfig powerManagement;
    if (esp_pm_get_configuration(&powerManagement) != ESP_OK) {
        LOG_ERROR(TAG, "Failed to read CPU Frequency!");
        return false;
    }

    if (strcmp(cpuFrequency, "160") == 0) {
        LOG_INFO(TAG, "%s Mhz is the default frequency, no need change");

    } else if (strcmp(cpuFrequency, "240") == 0) {
        powerManagement.maxFreqMhz = 240;
        powerManagement.minFreqMhz = powerManagement.maxFreqMhz;
        if (esp_pm_configure(&powerManagement) != ESP_OK) {
            LOG_ERROR(TAG, "Failed to set new CPU frequency!");
            return false;
        }

    } else {
        LOG_ERROR(TAG, "Unknown CPU frequency: %s! It must be 160 or 240!", cpuFrequency);
        return false;
    }

    if (esp_pm_get_configuration(&powerManagement) == ESP_OK) {
        LOG_INFO(TAG, "CPU frequency: %d Mhz", powerManagement.maxFreqMhz);
    }

    return true;
}

time_t getEspUpTime() {
    return (uint32_t) (esp_timer_get_time() / 1000 / 1000); // in seconds
}

char *getResetReason() {
	switch(esp_reset_reason()) {
		case ESP_RST_POWERON: return "Power-on event (or reset button)";    //!< Reset due to power-on event
		case ESP_RST_EXT: return "External pin";                  //!< Reset by external pin (not applicable for ESP32)
		case ESP_RST_SW: return "Via esp_restart";                //!< Software reset via esp_restart
		case ESP_RST_PANIC: return "Exception/panic";             //!< Software reset due to exception/panic
		case ESP_RST_INT_WDT: return "Interrupt watchdog";        //!< Reset (software or hardware) due to interrupt watchdog
		case ESP_RST_TASK_WDT: return "Task watchdog";            //!< Reset due to task watchdog
		case ESP_RST_WDT: return "Other watchdogs";               //!< Reset due to other watchdogs
		case ESP_RST_DEEPSLEEP: return "Exiting deep sleep mode";  //!< Reset after exiting deep sleep mode
		case ESP_RST_BROWNOUT: return "Brownout";                 //!< Brownout reset (software or hardware)
		case ESP_RST_SDIO: return "SDIO";                         //!< Reset over SDIO
		case ESP_RST_UNKNOWN:   //!< Reset reason can not be determined
		default: 
			return "Unknown";
	}
}

esp_err_t initExternalPSRAM() {
    esp_err_t psRamStatus = esp_psram_init();
    if (psRamStatus == ESP_FAIL || !esp_psram_is_initialized()) {
        LOG_ERROR(TAG, "PSRAM init failed [%d]! PSRAM not found or defective", psRamStatus);
        return psRamStatus;
    }

    // ESP_OK -> PSRAM init OK --> continue to check PSRAM size
    size_t psramSize = esp_psram_get_size();
    BufferString *valueSizeStr = byteCountToDisplaySize(psramSize, EMPTY_STRING(16));
    LOG_INFO(TAG, "PSRAM size: [%zu] bytes [%s]", psramSize, valueSizeStr->value);

    // Check PSRAM size
    if (psramSize < (4 * ONE_MB)) { // PSRAM is below 4 MBytes (32Mbit)
        LOG_ERROR(TAG, "PSRAM size >= 4MB (32Mbit) is mandatory to run this application. Actual size: [%s]", valueSizeStr->value);
        return ESP_FAIL;
    }

    // PSRAM size OK --> continue to check heap size
    size_t heapSize = getEspHeapSize();
    byteCountToDisplaySize(heapSize, clearString(valueSizeStr));
    LOG_INFO(TAG, "Total heap: [%s]", valueSizeStr->value);

    // Check heap memory
    if (heapSize < (4 * ONE_MB)) {  // Check available Heap memory for a bit less than 4 MB
        LOG_ERROR(TAG, "Total heap >= %lld byte is mandatory to run this application", (4 * ONE_MB));
        // return ESP_FAIL;
    }
    return ESP_OK;
}

BufferString *getEspHeapInfo() {
    static char messageBuffer[MESSAGE_BUFFER_LENGTH] = {0};
    newString(&espInfoStr, "", messageBuffer, MESSAGE_BUFFER_LENGTH);

	size_t aFreeHeapSize  = heap_caps_get_free_size(MALLOC_CAP_8BIT);
	size_t aFreeSPIHeapSize  = heap_caps_get_free_size(MALLOC_CAP_8BIT | MALLOC_CAP_SPIRAM);
	size_t aFreeInternalHeapSize  = heap_caps_get_free_size(MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL);
	size_t aHeapLargestFreeBlockSize = heap_caps_get_largest_free_block(MALLOC_CAP_8BIT | MALLOC_CAP_SPIRAM);
	size_t aHeapIntLargestFreeBlockSize = heap_caps_get_largest_free_block(MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL);
	size_t aMinFreeHeapSize =  heap_caps_get_minimum_free_size(MALLOC_CAP_8BIT | MALLOC_CAP_SPIRAM);
	size_t aMinFreeInternalHeapSize =  heap_caps_get_minimum_free_size(MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL);

    return stringFormat(&espInfoStr, "Heap Total: [%ld] | "
                                     "SPI Free: [%ld] | "
                                     "SPI Large Block: [%ld] | "
                                     "SPI Min Free: [%ld] | "
                                     "Int Free: [%ld] | "
                                     "Int Large Block: [%ld] | "
                                     "Int Min Free: [%ld]", 
                                    (long) aFreeHeapSize, 
                                    (long) aFreeSPIHeapSize, 
                                    (long) aHeapLargestFreeBlockSize, 
                                    (long) aMinFreeHeapSize, 
                                    (long) aFreeInternalHeapSize, 
                                    (long) aHeapIntLargestFreeBlockSize,
                                    (long) aMinFreeInternalHeapSize);
}


size_t getEspHeapSize() {
   return heap_caps_get_free_size(MALLOC_CAP_8BIT);
}


size_t getInternalEspHeapSize() {
	return heap_caps_get_free_size(MALLOC_CAP_8BIT| MALLOC_CAP_INTERNAL);
}