#include "AppConfig.h"

#include "esp_chip_info.h"
#include "esp_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"
#include <inttypes.h>
#include <stdio.h>

#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"

#include "psa_crypto_rsa.h"
#include "esp_sleep.h"

#include "mbedtls/pk.h"
#include "mbedtls/error.h"

#include "SDCard.h"
#include "StatusLed.h"
#include "NTPTime.h"
#include "EspConfig.h"
#include "CameraControl.h"
#include "SoftAPServer.h"

/*
 * Two resistors divider of 100K and 10K
 *
 * Vbat     Vin      GND
 *  |        |        |
 *  +--[R1]--+--[R2]--+
 *
 *  R1 = 100kOhms
 *  R2 = 10kOhms
*/
#define R1 100
#define R2 10
#define VOLTAGE_OUT(Vin) (((Vin) * R2) / (R1 + R2))

// We can take the battery voltage of 4.1V as 100% and the voltage of 3.3V as 0%
#define BATTERY_VOLTAGE_MAX 4100
#define BATTERY_VOLTAGE_MIN 3300

#define BATTERY_DIVIDER_VOLTAGE_MAX (VOLTAGE_OUT(BATTERY_VOLTAGE_MAX))
#define BATTERY_DIVIDER_VOLTAGE_MIN (VOLTAGE_OUT(BATTERY_VOLTAGE_MIN))

static const char *TAG = "MAIN";

Properties appConfig;
Properties wlanConfig;

File imageDirRoot;
CronExpression cron;
sqlite3 *embeddedDb;

static int batteryPercentage;
static DateTimeFormatter photoFileDateTimeFormatter;

static int logOverrideFunction(const char *format, va_list argumentList);
static int getBatteryDividerVoltage();
static bool adcCalibrationInit(adc_unit_t unit, adc_channel_t channel, adc_atten_t atten, adc_cali_handle_t *outHandle);
static void adcCalibrationDeinit(adc_cali_handle_t handle);

static int calculateBatteryPercentage(int dividerVoltage);
static void executeCronJob();
static void cleanupPhotoDirectory();
static int fileDateCompareFunction (const void *one, const void *two);


void app_main() {

    statusLedOff();

    LoggerEvent *consoleLogger = subscribeConsoleLogger(stringToLogLevel(DEFAULT_LOG_LEVEL_STR));
    if (consoleLogger == NULL) {
        return; // No way to continue without working logger!
    }

    vTaskDelay(pdMS_TO_TICKS(3000));
    int dividerVoltage = getBatteryDividerVoltage();

    LOG_INFO(TAG, "\n================ Start app_main =================");

    gpio_pulldown_en(FLASH_GPIO);   // set pull down to prevent flashlight flickering
    gpio_reset_pin(CONFIG_START_GPIO);

    // Init camera, try several times if no success from the first try
    esp_err_t cameraStatus;
    uint8_t cameraInitTry = 3;
    do {
        powerResetCamera();
        cameraStatus = initCamera();
        flashLightOff();
        cameraInitTry--;

        TickType_t xDelay = 2000;
        LOG_DEBUG(TAG, "After camera initialization: sleep for: %ldms", xDelay);
        vTaskDelay(pdMS_TO_TICKS(xDelay));
    } while (cameraInitTry > 0 && cameraStatus != ESP_OK);

    SDCardStatus cardStatus = initNvsSDCard();
    if (cardStatus != SD_CARD_OK) {
        LOG_FATAL(TAG, "SD card init failed!");
        statusLed(SD_CARD_INIT_ERROR, cardStatus, true);
        return;     // No way to continue without working SD card!
    }

    if (checkSDCardRW() != SD_CARD_OK) {
        LOG_FATAL(TAG, "SD card r/w check failed!");
        statusLed(SD_CARD_CHECK_ERROR, cardStatus, true);
        return;     // No way to continue without working SD card!
    }

    Properties *properties = loadProperties(&appConfig, CONFIG_FILE);
    if (properties->status != CONFIG_PROP_OK) {
        LOG_ERROR(TAG, "Filed to load application properties [%s]. Message: [%s]", CONFIG_FILE, propStatusToString(properties->status));
        statusLed(CONFIG_PROP_ERROR, properties->status, true);
        return; // mandatory application properties
    }

    Properties *wlanProperties = loadProperties(&wlanConfig, WLAN_CONFIG_FILE);
    if (wlanProperties->status != CONFIG_PROP_OK) {
        LOG_ERROR(TAG, "Filed to load wlan properties [%s]. Message: [%s]", WLAN_CONFIG_FILE, propStatusToString(wlanProperties->status));
        return; // mandatory wlan properties
    }

    bool isFileLogEnabled = isStringEquals(getPropertyOrDefault(&appConfig, PROPERTY_LOG_FILE_ENABLED_KEY, "false"), "true");
    if (isFileLogEnabled) {
        LOG_INFO(TAG, "File logging enabled. Setup logger from config");
        char *logFilePathStr = getPropertyOrDefault(&appConfig, PROPERTY_LOG_FILE_PATH_KEY, DEFAULT_LOG_FILE_PATH);
        File *logFilePath = NEW_FILE(logFilePathStr);
        File *logFile = FILE_OF(logFilePath, getPropertyOrDefault(&appConfig, PROPERTY_LOG_FILE_NAME_KEY, DEFAULT_LOG_FILE_NAME));
        LOG_INFO(TAG, "Log file path: [%s]", logFile->path);
        createFileDirs(logFile);
        
        char *logLevelStr = getPropertyOrDefault(&appConfig, PROPERTY_LOG_LEVEL_KEY, DEFAULT_LOG_LEVEL_STR);
        LogLevel level = stringToLogLevel(logLevelStr);
        if (level == LOG_LEVEL_UNKNOWN) {
            LOG_ERROR(TAG, "Invalid log level: [%s]. Set default log to: [%s]", logLevelStr, DEFAULT_LOG_LEVEL_STR);
            level = stringToLogLevel(DEFAULT_LOG_LEVEL_STR);
        }

        char *logFileSizeStr = getPropertyOrDefault(&appConfig, PROPERTY_LOG_FILE_MAX_SIZE_KEY, DEFAULT_LOG_FILE_SIZE);
        uint32_t logFileMaxSize = (uint32_t) displaySizeToBytes(logFileSizeStr);
        uint8_t maxBackupFiles = strtol(getPropertyOrDefault(&appConfig, PROPERTY_LOG_FILE_MAX_BACKUPS_KEY, DEFAULT_LOG_FILE_BACKUPS), NULL, 10);
        LoggerEvent *fileLogger = subscribeFileLogger(level, logFile->path, logFileMaxSize, maxBackupFiles);  // Register file logger
        if (!fileLogger->isSubscribed) {
            LOG_FATAL(TAG, "File logger init error: [%s]", fileLogger->buffer);
            statusLed(SD_CARD_CHECK_ERROR, SD_CARD_ERROR_CREATE_FILE, true);
            return;
        }
        esp_log_set_vprintf(logOverrideFunction);   // redirect all logs to custom logger

        bool isConsoleLogEnabled = isStringEquals(getPropertyOrDefault(&appConfig, PROPERTY_LOG_CONSOLE_ENABLED_KEY, "false"), "true");
        if (!isConsoleLogEnabled) {
            LOG_INFO(TAG, "At this point console log will be disabled");
            esp_log_level_set("*", ESP_LOG_NONE);
            loggerUnsubscribe(consoleLogger);     // Console logger doesn't need this anymore, all logs will be stored in a log file
        }
        LOG_INFO(TAG, "\nLog file directory: [%s]\n "
                    "File logger level: [%s]\n "
                    "File max size: [%s]\n "
                    "Backup file count: [%d]",
                    logFile->path, 
                    logLevelToString(level), 
                    logFileSizeStr, 
                    maxBackupFiles);
    }

    LOG_INFO(TAG, "Successfully loaded application properties: [%s]. Total values received: [%d]", CONFIG_FILE, propertiesSize(&appConfig));
    logSDCardInformation();
    
    LOG_INFO(TAG, "=================================================");
    LOG_INFO(TAG, "==================== Start ======================");
    LOG_INFO(TAG, "=================================================");

    LOG_INFO("Restart reason: [%s]", getResetReason());
    bool isButtonWakeup = esp_reset_reason() == ESP_RST_DEEPSLEEP && esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_EXT0;
    if (isButtonWakeup) {
        LOG_INFO(TAG, "Config GPIO Button was pressed. Enabling Soft AP configuration server");
        putProperty(&wlanConfig, PROPERTY_ENABLE_CONFIG_KEY, "true");
    }
    
    // Set CPU Frequency
    setupEspCpuFrequency();

    esp_err_t psRamStatus = initExternalPSRAM();
    if (psRamStatus != ESP_OK) {
        LOG_ERROR(TAG, "PSRAM init failed");
        statusLed(PSRAM_INIT_ERROR, 1, false);
        return;
    }

    if (cameraStatus != ESP_OK) {
        LOG_ERROR(TAG, "Camera init failed");
        statusLed(CAMERA_INIT_ERROR, 1, false);
        return;
    }

    if (!testCamera()) {    // Camera init OK --> continue to perform camera framebuffer check
        LOG_ERROR(TAG, "Camera framebuffer check failed");
        statusLed(CAMERA_INIT_ERROR, 2, false);
        return;
    }

    sensor_t *sensor = esp_camera_sensor_get();
    LOG_INFO(TAG, "Camera info: PID: 0x%02x, VER: 0x%02x, MIDL: 0x%02x, MIDH: 0x%02x", sensor->id.PID, sensor->id.VER, sensor->id.MIDH, sensor->id.MIDL);
    
    LOG_INFO(TAG,  "\n"
                     "  ___   _____  ___  ___       _                   \n"
                     " / _ \\ |_   _| |  \\/  |      | |                \n"
                     "/ /_\\ \\  | |   | .  . |  ___ | |_   ___  _ __   \n"
                     "|  _  |  | |   | |\\/| | / _ \\| __| / _ \\| '__| \n"
                     "| | | | _| |_  | |  | ||  __/| |_ |  __/| |       \n"
                     "\\_| |_/ \\___/  \\_|  |_/ \\___| \\__| \\___||_| \n"
                     "                                                  \n");

    // Print Device info
    esp_chip_info_t chipInfo;
    esp_chip_info(&chipInfo);
    LOG_INFO(TAG, "Device info: CPU cores: %d, Chip revision: %d", chipInfo.cores, chipInfo.revision);

    TickType_t xDelay = 2000;
    LOG_DEBUG(TAG, "main: sleep for: %d ms", xDelay);
    vTaskDelay(pdMS_TO_TICKS(xDelay));

    batteryPercentage = calculateBatteryPercentage(dividerVoltage);
    newFile(&imageDirRoot, CAMERA_IMAGE_DIR);
    MKDIR(imageDirRoot.path);

    initWifiMain();     // base wi-fi configuration
    initRestClient();   // base rest client

    embeddedDb = sqliteDbInit(EMBEDDED_DATABASE_FILE);
    if (embeddedDb == NULL) {
        LOG_ERROR(TAG, "Error open DB initialization: [%s]", EMBEDDED_DATABASE_FILE);
        return; // DB is mandatory to the application
    }
    LOG_INFO(TAG, "DB successfully initialized: [%s]", EMBEDDED_DATABASE_FILE);

    setupUserTimeZone();    // resolve timezone from config if present

    if (wifiHasLogAndPassToConnect(&wlanConfig)) {  // try to connect if credentials already provided
        LOG_DEBUG(TAG, "Wifi credentials provided, connecting to wifi");
        uint8_t i = 0;
        do {
            if (wifiInitSta(&wlanConfig) == ESP_OK) break;
            i++;
        } while (!isWifiHasConnection() && i < 3);

        if (isWifiHasConnection()) {
            setupNtpTime(); // init ntp time, we have connection already
            if (!isProjectTimeSet()) {
                esp_restart();  // time is not configured, retry after restart
            }

            loadTimezoneHistoricRules(&timeZone);   // at this point time should be setup, then load DST rules
            LOG_INFO(TAG, "Time now corrected to DST rules: [%s]", getCurrentTimeString(DATE_TIME_FORMAT_ZONE_STR));
        }
    }

    if (!isAppFullyConfigured(&wlanConfig) || !isWifiHasConnection()) { // Not configured or button pressed, then enable soft AP server
        LOG_INFO(TAG, "Starting access point for remote configuration");
        wifiInitSoftAp(&appConfig);
        startWebServerAP();
        statusLed(AP_OR_OTA_ENABLED, 2, true);
        while(true) { // wait until reboot
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }
    LOG_INFO(TAG, "Project fully configured, enabling worker mode");

    char *cronStr = getProperty(&wlanConfig, PROPERTY_SYSTEM_CRON_EXPR_KEY);
    CronStatus cronStatus = parseCronExpression(&cron, cronStr);
    if (cronStatus == CRON_OK) {
        LOG_INFO(TAG, "Cron initialized OK!");
    }

    executeCronJob();   // send a meter photo with data if it is time
    statusLedOff();
    cleanupPhotoDirectory();    // remove old photos
    configureButtonWakeup();    // if button pressed, then need to reconfigure app settings, wakeup from sleep and start soft AP server
    enterTimerDeepSleep(calculateSecondsToWaitFromNow());
}

void configureButtonWakeup() {
    gpio_config_t ioConfig;
    ioConfig.intr_type = GPIO_PIN_INTR_DISABLE; // Disable interrupt on GPIO
    ioConfig.pin_bit_mask = (1ULL << CONFIG_START_GPIO);
    ioConfig.mode = GPIO_MODE_INPUT_OUTPUT;
    ioConfig.pull_up_en = GPIO_PULLUP_DISABLE;
    ioConfig.pull_down_en = GPIO_PULLDOWN_ENABLE;
    gpio_config(&ioConfig);

    gpio_pulldown_en(CONFIG_START_GPIO);
    esp_sleep_enable_ext0_wakeup(CONFIG_START_GPIO, 1); // 1 for high-level wakeup
}

static int logOverrideFunction(const char *format, va_list argumentList) {
    static char buffer[LOGGER_BUFFER_SIZE] = {0};
    vsprintf(buffer, format, argumentList);
    LOG_TRACE(TAG, trimString(buffer));
    return vprintf(format, argumentList); // ALWAYS Write to stdout
}

static int getBatteryDividerVoltage() {
    LOG_DEBUG(TAG, "-------------ADC2 Init---------------");
    adc_oneshot_unit_handle_t adc2Handle;
    adc_oneshot_unit_init_cfg_t initConfig = {
            .unit_id = ADC_UNIT_2,
            .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&initConfig, &adc2Handle));

    LOG_DEBUG(TAG, "-------------ADC2 Calibration Init---------------");
    adc_cali_handle_t adc2CalibrationHandle = NULL;
    bool isDoCalibration = adcCalibrationInit(ADC_UNIT_2, BATTERY_ADC_CHANNEL, ADC_ATTEN_DB_11, &adc2CalibrationHandle);

    LOG_DEBUG(TAG, "-------------ADC2 Config---------------");
    adc_oneshot_chan_cfg_t config = {
            .bitwidth = ADC_BITWIDTH_DEFAULT,
            .atten = ADC_ATTEN_DB_11,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc2Handle, BATTERY_ADC_CHANNEL, &config));

    LOG_DEBUG(TAG, "-------------ADC2 Read---------------");
    int outRaw = 0;
    int voltage = 0;

    ESP_ERROR_CHECK(adc_oneshot_read(adc2Handle, BATTERY_ADC_CHANNEL, &outRaw));
    LOG_INFO(TAG, "ADC%d Channel[%d] Raw Data: %d", ADC_UNIT_2 + 1, BATTERY_ADC_CHANNEL, outRaw);
    if (isDoCalibration) {
        ESP_ERROR_CHECK(adc_cali_raw_to_voltage(adc2CalibrationHandle, outRaw, &voltage));
        LOG_INFO(TAG, "ADC%d Channel[%d] Calibration Voltage: %d mV", ADC_UNIT_2 + 1, BATTERY_ADC_CHANNEL, voltage);
    }

    //Tear Down
    ESP_ERROR_CHECK(adc_oneshot_del_unit(adc2Handle));
    if (isDoCalibration) {
        adcCalibrationDeinit(adc2CalibrationHandle);
    }
    gpio_reset_pin(BATTERY_GPIO_PIN);
    return voltage;
}

static bool adcCalibrationInit(adc_unit_t unit, adc_channel_t channel, adc_atten_t atten, adc_cali_handle_t *outHandle) {
    adc_cali_handle_t handle = NULL;
    esp_err_t ret = ESP_FAIL;
    bool calibrated = false;

#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    if (!calibrated) {
        LOG_INFO(TAG, "Calibration scheme version is [%s]", "Curve Fitting");
        adc_cali_curve_fitting_config_t cali_config = {
            .unit_id = unit,
            .chan = channel,
            .atten = atten,
            .bitwidth = ADC_BITWIDTH_DEFAULT,
        };
        ret = adc_cali_create_scheme_curve_fitting(&cali_config, &handle);
        if (ret == ESP_OK) {
            calibrated = true;
        }
    }
#endif

#if ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
    if (!calibrated) {
        ESP_LOGI(TAG, "Calibration scheme version is [%s]", "Line Fitting");
        adc_cali_line_fitting_config_t cali_config = {
            .unit_id = unit,
            .atten = atten,
            .bitwidth = ADC_BITWIDTH_DEFAULT,
        };
        ret = adc_cali_create_scheme_line_fitting(&cali_config, &handle);
        if (ret == ESP_OK) {
            calibrated = true;
        }
    }
#endif

    *outHandle = handle;
    if (ret == ESP_OK) {
        LOG_INFO(TAG, "Calibration Success");
    } else if (ret == ESP_ERR_NOT_SUPPORTED || !calibrated) {
        LOG_WARN(TAG, "eFuse not burnt, skip software calibration");
    } else {
        LOG_ERROR(TAG, "Invalid ADC arg or no memory");
    }

    return calibrated;
}

static void adcCalibrationDeinit(adc_cali_handle_t handle) {
#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    LOG_INFO(TAG, "Deregister [%s] calibration scheme", "Curve Fitting");
    ESP_ERROR_CHECK(adc_cali_delete_scheme_curve_fitting(handle));

#elif ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
    LOG_INFO(TAG, "Deregister [%s] calibration scheme", "Line Fitting");
    ESP_ERROR_CHECK(adc_cali_delete_scheme_line_fitting(handle));
#endif
}

static int calculateBatteryPercentage(int dividerVoltage) {
    int battPercentage = 100 * (dividerVoltage - BATTERY_DIVIDER_VOLTAGE_MIN) / (BATTERY_DIVIDER_VOLTAGE_MAX - BATTERY_DIVIDER_VOLTAGE_MIN);
    LOG_INFO(TAG, "Battery divider voltage: [%d], Percentage: [%d%%]", dividerVoltage, battPercentage);

    if (battPercentage < 0) {
        return 0;
    } else if (battPercentage > 100) {
        return 100;
    } else {
        return battPercentage;
    }
}

static void executeCronJob() {
    LOG_INFO(TAG, "Cron job started!");
    ZonedDateTime zdtNow = zonedDateTimeNow(&timeZone);
    LOG_INFO(TAG, "Cron now: %s", zonedDateTimeToStrByFormat(&zdtNow, DATE_TIME_FORMAT_SHORT));

    char *previousExecutionDateStr = getProperty(&wlanConfig, PROPERTY_PREVIOUS_CRON_DATE_KEY);
    DateTimeFormatter formatter;
    parseDateTimePattern(&formatter, DATE_TIME_FORMAT_SHORT);
    DateTime previousDateTime = parseToDateTime(previousExecutionDateStr, &formatter);
    ZonedDateTime previousZdt = zonedDateTimeOfDateTime(&previousDateTime, &timeZone);

    ZonedDateTime expectedExecution = nextCronZonedDateTime(&cron, &previousZdt);
    LOG_INFO(TAG, "Cron expected execution: %s", zonedDateTimeToStrByFormat(&expectedExecution, DATE_TIME_FORMAT_SHORT));
    ZonedDateTime *before = zonedDateTimeMinusMinutes(&Z_DATE_TIME_COPY(expectedExecution), CRON_JOB_ALLOWED_MINUTES_GAP + 1);
    ZonedDateTime *after = zonedDateTimePlusMinutes(&Z_DATE_TIME_COPY(expectedExecution), CRON_JOB_ALLOWED_MINUTES_GAP + 1);

    if (isDateTimeBetween(&zdtNow.dateTime, &before->dateTime, &after->dateTime)) {
        LOG_INFO(TAG, "Cron job approved. Starting task execution...");
        char *ledDutyStr = getProperty(&wlanConfig, PROPERTY_FLASH_LIGHT_INTENSITY_KEY);
        int64_t ledDuty;
        cStrToInt64(ledDutyStr, &ledDuty, 10);
        setLedIntensity((int32_t) ledDuty);

        BufferString *imageFileName = STRING_FORMAT_64("photo_%s.jpeg", zonedDateTimeToStrByFormat(&zdtNow, DATE_TIME_FILE_NAME_FORMAT));
        cameraCaptureToFile(imageFileName->value);
        File *imageFile = FILE_OF(&imageDirRoot, imageFileName->value);

        CspTemplate *messageTemplate = newCspTemplate(HTML_TEMPLATE_DIR "/" CSP_MESSAGE_CAPTION_NAME);
        logTemplate(messageTemplate, CSP_MESSAGE_CAPTION_NAME);

        CspObjectMap *params = newCspParamObjMap(8);
        cspAddStrToMap(params, "meterName", getProperty(&wlanConfig, PROPERTY_METER_NAME_KEY));
        cspAddStrToMap(params, "meterReadings", NULL);
        cspAddIntToMap(params, "battery", batteryPercentage);
        cspAddStrToMap(params, "date", zonedDateTimeToStrByFormat(&zdtNow, DATE_TIME_FORMAT_SHORT));

        CspRenderer *renderer = NEW_CSP_RENDERER(messageTemplate, params);
        CspTableString *captionMessage = renderCspTemplate(renderer);
        sendTelegramPhotoWithCaption(imageFile, captionMessage->value, captionMessage->length);

        deleteCspRenderer(renderer);
        deleteCspTemplate(messageTemplate);

        // End time should be over max gap, its guarantee that photo will not be sent twice or more when a device powered just in a time gap, or wakeup seconds calculated incorrectly
        ZonedDateTime *jobEndDateTime = zonedDateTimePlusMinutes(after, 1);
        putProperty(&wlanConfig, PROPERTY_PREVIOUS_CRON_DATE_KEY, zonedDateTimeToStrByFormat(jobEndDateTime, DATE_TIME_FORMAT_SHORT));
        storeProperties(&wlanConfig, WLAN_CONFIG_FILE);
        LOG_INFO(TAG, "Cron job finished");
        return;
    }

    putProperty(&wlanConfig, PROPERTY_PREVIOUS_CRON_DATE_KEY, zonedDateTimeToStrByFormat(&zdtNow, DATE_TIME_FORMAT_SHORT));
    storeProperties(&wlanConfig, WLAN_CONFIG_FILE);
    LOG_INFO(TAG, "Cron end");
}

static void cleanupPhotoDirectory() {
    File *fileBuffer = callocPsramHeap(MAX_FILES_IN_DIR + 1, sizeof(struct File));
    fileVector *photoVec = NEW_VECTOR_BUFF(File, file, fileBuffer, MAX_FILES_IN_DIR + 1);
    listFiles(&imageDirRoot, photoVec, false);

    LOG_INFO(TAG, "Total photos in dir: [%d]", fileVecSize(photoVec));
    if (fileVecSize(photoVec) >= MAX_FILES_IN_DIR) {
        LOG_INFO(TAG, "Collected photos more than: %d. Removing oldest %d photos", MAX_FILES_IN_DIR, OLDEST_PHOTOS_TO_REMOVE_COUNT);
        parseDateTimePattern(&photoFileDateTimeFormatter, DATE_TIME_FILE_NAME_FORMAT);
        qsort(photoVec->items, fileVecSize(photoVec), sizeof(File), fileDateCompareFunction);   // sort by photo date time from it name, first will be the oldest one

        for (int i = 0; i < OLDEST_PHOTOS_TO_REMOVE_COUNT; i++) { // remove oldest 20 photos
            File fileToRemove = fileVecGet(photoVec, i);
            remove(fileToRemove.path);    // calibration photo will be in the list bottom, so it will not be deleted
        }
    }

    freePsramHeap(fileBuffer);
}

static int fileDateCompareFunction (const void *one, const void *two) {
    BufferString *firstDateStr = SUBSTRING_CSTR_BETWEEN(32, ((File *) one)->path, "photo_", ".jpeg");   // extract date-time string
    BufferString *secondDateStr = SUBSTRING_CSTR_BETWEEN(32, ((File *) two)->path, "photo_", ".jpeg");
    DateTime firstDateTime = parseToDateTime(stringValue(firstDateStr), &photoFileDateTimeFormatter);
    DateTime secondDateTime = parseToDateTime(stringValue(secondDateStr), &photoFileDateTimeFormatter);
    return (int) dateTimeCompare(&firstDateTime, &secondDateTime);
}