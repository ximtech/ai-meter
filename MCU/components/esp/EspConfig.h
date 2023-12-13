#pragma once

#include "../../src/AppConfig.h"

#include "esp_heap_caps.h"
#include "esp_pm.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "esp_psram.h"
#include "esp32/himem.h"

/**
 * Power management config for ESP32
 *
 * Pass a pointer to this structure as an argument to espPmConfigure function.
 */
typedef struct EspPowerManagementConfig {
    int32_t maxFreqMhz;               // Maximum CPU frequency, in MHz
    int32_t minFreqMhz;               // Minimum CPU frequency to use when no locks are taken, in MHz
    bool lightSleepEnable;        // Enter light sleep when no locks are taken
} EspPowerManagementConfig;


bool setupEspCpuFrequency();
esp_err_t initExternalPSRAM();

time_t getEspUpTime();
char *getResetReason();

BufferString *getEspHeapInfo();
size_t getEspHeapSize();
size_t getInternalEspHeapSize();