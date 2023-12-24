#include "SoftAPServer.h"

#define DEFAULT_SCAN_LIST_SIZE 20
#define PIN_NUMBER_LOWER_LIMIT 1000
#define PIN_NUMBER_UPPER_LIMIT 9999

static const char *TAG = "SOFT_AP";

static CspTemplate *welcomePage;
static CspTemplate *connectPage;
static CspTemplate *calibratePage;
static CspTemplate *schedulePage;
static CspTemplate *messagingPage;
static CspTemplate *summaryPage;
static CspTemplate *adminPage;

static esp_err_t welcomePageHandler(httpd_req_t *request);
static esp_err_t connectPageHandler(httpd_req_t *request);
static esp_err_t calibratePageHandler(httpd_req_t *request);
static esp_err_t schedulePageHandler(httpd_req_t *request);
static esp_err_t messagingPageHandler(httpd_req_t *request);
static esp_err_t summaryPageHandler(httpd_req_t *request);
static esp_err_t adminPageHandler(httpd_req_t *request);

static esp_err_t saveNameAjaxHandler(httpd_req_t *request);
static esp_err_t saveWifiCredentialsAjaxHandler(httpd_req_t *request);
static esp_err_t imageCaptureAjaxHandler(httpd_req_t *request);
static esp_err_t cronAjaxHandler(httpd_req_t *request);
static esp_err_t telegramMessageAjaxHandler(httpd_req_t *request);
static esp_err_t timeZoneSearchAjaxHandler(httpd_req_t *request);
static esp_err_t timeZoneSaveAjaxHandler(httpd_req_t *request);
static esp_err_t summarySaveAjaxHandler(httpd_req_t *request);
static esp_err_t adminConfigPropertiesAjaxHandler(httpd_req_t *request);
static esp_err_t adminUpdatePropertyAjaxHandler(httpd_req_t *request);
static esp_err_t adminRemovePropertyAjaxHandler(httpd_req_t *request);
static esp_err_t adminGetLogContentsAjaxHandler(httpd_req_t *request);
static esp_err_t adminCleanLogFileAjaxHandler(httpd_req_t *request);
static esp_err_t adminDirectoryContentsAjaxHandler(httpd_req_t *request);
static esp_err_t deleteFileAjaxHandler(httpd_req_t *request);
static esp_err_t uploadFileAjaxHandler(httpd_req_t *request);
static esp_err_t restartEspAjaxHandler(httpd_req_t *request);

static CspObjectArray *mapApRecordsToList(wifi_ap_record_t *apRecords, uint16_t length);
static int apRecordComparator(const void *v1, const void *v2);
static Properties *getPropertiesByFileName(const char *configFileName);
static int propertyEntryKeyCompareFun(const void *one, const void *two);


httpd_handle_t startWebServerAP() {
    httpd_handle_t server = NULL;

    welcomePage = newCspTemplate(HTML_TEMPLATE_DIR "/" CSP_WELCOME_PAGE_NAME);
    logTemplate(welcomePage, CSP_WELCOME_PAGE_NAME);

    connectPage = newCspTemplate(HTML_TEMPLATE_DIR "/" CSP_CONNECT_PAGE_NAME);
    logTemplate(connectPage, CSP_CONNECT_PAGE_NAME);

    calibratePage = newCspTemplate(HTML_TEMPLATE_DIR "/" CSP_CALIBRATE_PAGE_NAME);
    logTemplate(calibratePage, CSP_CALIBRATE_PAGE_NAME);

    schedulePage = newCspTemplate(HTML_TEMPLATE_DIR "/" CSP_SCHEDULE_PAGE_NAME);
    logTemplate(schedulePage, CSP_SCHEDULE_PAGE_NAME);

    messagingPage = newCspTemplate(HTML_TEMPLATE_DIR "/" CSP_MESSAGING_PAGE_NAME);
    logTemplate(messagingPage, CSP_MESSAGING_PAGE_NAME);

    summaryPage = newCspTemplate(HTML_TEMPLATE_DIR "/" CSP_SUMMARY_PAGE_NAME);
    logTemplate(summaryPage, CSP_SUMMARY_PAGE_NAME);

    adminPage = newCspTemplate(HTML_TEMPLATE_DIR "/" CSP_ADMIN_PAGE_NAME);
    logTemplate(adminPage, CSP_ADMIN_PAGE_NAME);

    notFoundPage = newCspTemplate(HTML_TEMPLATE_DIR "/" CSP_NOT_FOUND_PAGE_NAME);
    logTemplate(notFoundPage, CSP_NOT_FOUND_PAGE_NAME);

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.max_uri_handlers = 32;
    config.stack_size = 8096;
    config.uri_match_fn = httpd_uri_match_wildcard;
    if (httpd_start(&server, &config) == ESP_OK) {
        LOG_INFO(TAG, "AP server started successfuly! Port: [%d]", config.server_port);
    }

    // Page handlers 
    httpd_uri_t welcomPageUri = {
            .uri = "/",
            .method = HTTP_GET,
            .handler = welcomePageHandler,
            .user_ctx = NULL};
    httpd_register_uri_handler(server, &welcomPageUri);

    httpd_uri_t connectPageUri = {
            .uri = "/connect",
            .method = HTTP_GET,
            .handler = connectPageHandler,
            .user_ctx = NULL};
    httpd_register_uri_handler(server, &connectPageUri);

    httpd_uri_t calibratePageUri = {
            .uri = "/calibrate",
            .method = HTTP_GET,
            .handler = calibratePageHandler,
            .user_ctx = NULL};
    httpd_register_uri_handler(server, &calibratePageUri);

    httpd_uri_t schedulePageUri = {
            .uri = "/schedule",
            .method = HTTP_GET,
            .handler = schedulePageHandler,
            .user_ctx = NULL};
    httpd_register_uri_handler(server, &schedulePageUri);

    httpd_uri_t messagingPageUri = {
            .uri = "/messaging",
            .method = HTTP_GET,
            .handler = messagingPageHandler,
            .user_ctx = NULL};
    httpd_register_uri_handler(server, &messagingPageUri);

    httpd_uri_t summaryPageUri = {
            .uri = "/summary",
            .method = HTTP_GET,
            .handler = summaryPageHandler,
            .user_ctx = NULL};
    httpd_register_uri_handler(server, &summaryPageUri);

    httpd_uri_t adminPageUri = {
            .uri = "/admin",
            .method = HTTP_GET,
            .handler = adminPageHandler,
            .user_ctx = NULL};
    httpd_register_uri_handler(server, &adminPageUri);

    // Ajax handlers
    httpd_uri_t saveMeterNameUri = {
            .uri = "/save/meter/name",
            .method = HTTP_POST,
            .handler = saveNameAjaxHandler,
            .user_ctx = NULL};
    httpd_register_uri_handler(server, &saveMeterNameUri);

    httpd_uri_t connectToWifiUri = {
            .uri = "/save/wifi",
            .method = HTTP_POST,
            .handler = saveWifiCredentialsAjaxHandler,
            .user_ctx = NULL};
    httpd_register_uri_handler(server, &connectToWifiUri);

    httpd_uri_t cameraCaptureUri = {
            .uri = "/camera/image",
            .method = HTTP_GET,
            .handler = imageCaptureAjaxHandler,
            .user_ctx = NULL};
    httpd_register_uri_handler(server, &cameraCaptureUri);

    httpd_uri_t cronSaveUri = {
            .uri = "/save/cron",
            .method = HTTP_POST,
            .handler = cronAjaxHandler,
            .user_ctx = NULL};
    httpd_register_uri_handler(server, &cronSaveUri);

    httpd_uri_t saveChatIdUri = {
            .uri = "/save/chat/id",
            .method = HTTP_POST,
            .handler = telegramMessageAjaxHandler,
            .user_ctx = NULL};
    httpd_register_uri_handler(server, &saveChatIdUri);

    httpd_uri_t searchTimeZoneUri = {
            .uri = "/find/time/zone",
            .method = HTTP_POST,
            .handler = timeZoneSearchAjaxHandler,
            .user_ctx = NULL};
    httpd_register_uri_handler(server, &searchTimeZoneUri);

    httpd_uri_t saveTimeZoneUri = {
            .uri = "/save/timezone",
            .method = HTTP_POST,
            .handler = timeZoneSaveAjaxHandler,
            .user_ctx = NULL};
    httpd_register_uri_handler(server, &saveTimeZoneUri);

    httpd_uri_t summarySaveUri = {
            .uri = "/summary/save",
            .method = HTTP_POST,
            .handler = summarySaveAjaxHandler,
            .user_ctx = NULL};
    httpd_register_uri_handler(server, &summarySaveUri);

    httpd_uri_t propertiesGetUri = {
            .uri = "/admin/meter/config/values",
            .method = HTTP_GET,
            .handler = adminConfigPropertiesAjaxHandler,
            .user_ctx = NULL};
    httpd_register_uri_handler(server, &propertiesGetUri);

    httpd_uri_t propertyUpdateUri = {
            .uri = "/admin/update/property",
            .method = HTTP_POST,
            .handler = adminUpdatePropertyAjaxHandler,
            .user_ctx = NULL};
    httpd_register_uri_handler(server, &propertyUpdateUri);

    httpd_uri_t propertyRemoveUri = {
            .uri = "/admin/remove/property",
            .method = HTTP_POST,
            .handler = adminRemovePropertyAjaxHandler,
            .user_ctx = NULL};
    httpd_register_uri_handler(server, &propertyRemoveUri);

    httpd_uri_t logContentsUri = {
            .uri = "/admin/meter/logs",
            .method = HTTP_GET,
            .handler = adminGetLogContentsAjaxHandler,
            .user_ctx = NULL};
    httpd_register_uri_handler(server, &logContentsUri);

    httpd_uri_t logFileDeleteUri = {
            .uri = "/admin/remove/log",
            .method = HTTP_POST,
            .handler = adminCleanLogFileAjaxHandler,
            .user_ctx = NULL};
    httpd_register_uri_handler(server, &logFileDeleteUri);

    httpd_uri_t dirContentUri = {
            .uri = "/admin/fs/dir/content",
            .method = HTTP_GET,
            .handler = adminDirectoryContentsAjaxHandler,
            .user_ctx = NULL};
    httpd_register_uri_handler(server, &dirContentUri);

    httpd_uri_t deleteFileUri = {
            .uri = "/admin/esp/delete/file",
            .method = HTTP_POST,
            .handler = deleteFileAjaxHandler,
            .user_ctx = NULL};
    httpd_register_uri_handler(server, &deleteFileUri);

    httpd_uri_t uploadFileUri = {
            .uri = "/admin/upload/file/*",
            .method = HTTP_POST,
            .handler = uploadFileAjaxHandler,
            .user_ctx = NULL};
    httpd_register_uri_handler(server, &uploadFileUri);

    httpd_uri_t restartEspUri = {
            .uri = "/admin/esp/restart",
            .method = HTTP_POST,
            .handler = restartEspAjaxHandler,
            .user_ctx = NULL};
    httpd_register_uri_handler(server, &restartEspUri);

    httpd_uri_t resourcesUri = {
            .uri = "/assets/*",
            .method = HTTP_GET,
            .handler = handleHtmlResources,
            .user_ctx = NULL};
    httpd_register_uri_handler(server, &resourcesUri);

    httpd_uri_t cameraResourcesUri = {
            .uri = "/photo/*",
            .method = HTTP_GET,
            .handler = handlePhotoResources,
            .user_ctx = NULL};
    httpd_register_uri_handler(server, &cameraResourcesUri);

    httpd_uri_t fileSystemResourcesUri = {
            .uri = "/file/*",
            .method = HTTP_GET,
            .handler = handleFileResources,
            .user_ctx = NULL};
    httpd_register_uri_handler(server, &fileSystemResourcesUri);

    httpd_uri_t faviconUri = {
            .uri = "/favicon.ico",
            .method = HTTP_GET,
            .handler = handleFavicon,
            .user_ctx = NULL};
    httpd_register_uri_handler(server, &faviconUri);

    httpd_register_err_handler(server, HTTPD_404_NOT_FOUND, handleErrorPage);
    return server;
}

esp_err_t handleFavicon(httpd_req_t *request) {
    return sendFile(request, HTML_ASSETS_IMAGE_DIR "/favicon.ico");
}

esp_err_t handleHtmlResources(httpd_req_t *request) {
    LOG_DEBUG(TAG, "URI: [%s]", request->uri);
    BufferString *filePath = NEW_STRING(PATH_MAX_LEN, HTML_TEMPLATE_DIR);
    concatChars(filePath, request->uri);

    int32_t paramIndex = lastIndexOfString(filePath, "?");
    if (paramIndex != -1) { // have additional param value, need to remove it
        filePath = SUBSTRING(PATH_MAX_LEN, filePath, 0, paramIndex);
    }
    return sendFile(request, filePath->value);
}

esp_err_t handlePhotoResources(httpd_req_t *request) {
    LOG_DEBUG(TAG, "URI: [%s]", request->uri);
    BufferString *filePath = NEW_STRING(PATH_MAX_LEN, SD_CARD_ROOT);
    concatChars(filePath, request->uri);

    int32_t paramIndex = lastIndexOfString(filePath, "?");
    if (paramIndex != -1) { // have additional param value, need to remove it
        filePath = SUBSTRING(PATH_MAX_LEN, filePath, 0, paramIndex);
    }
    return sendFile(request, filePath->value);
}

esp_err_t handleFileResources(httpd_req_t *request) {
    LOG_DEBUG(TAG, "URI: [%s]", request->uri);
    BufferString *filePath = SUBSTRING_AFTER(PATH_MAX_LEN, NEW_STRING(HTTPD_MAX_URI_LEN, request->uri), "/file");

    int32_t paramIndex = lastIndexOfString(filePath, "?");
    if (paramIndex != -1) { // have additional param value, need to remove it
        filePath = SUBSTRING(PATH_MAX_LEN, filePath, 0, paramIndex);
    }
    return sendFile(request, filePath->value);
}

bool isAppFullyConfigured(Properties *configProp) {
    bool isMeterNameSet = getProperty(configProp, PROPERTY_METER_NAME_KEY) != NULL;
    bool isTimeZoneSet = getProperty(configProp, PROPERTY_APP_SYSTEM_TIMEZONE_KEY) != NULL;
    bool isCameraCalibrated = getProperty(configProp, PROPERTY_FLASH_LIGHT_INTENSITY_KEY) != NULL && getProperty(configProp, PROPERTY_CALIBRATION_FOTO_KEY) != NULL;
    bool isSchedulerConfigured = getProperty(configProp, PROPERTY_SYSTEM_CRON_EXPR_KEY) != NULL;
    bool isSubscribedToBot = getProperty(configProp, PROPERTY_TELEGRAM_CHAT_ID_KEY) != NULL;

    bool isConfigButtonPressed = propertiesHasKey(configProp, PROPERTY_ENABLE_CONFIG_KEY);
    if (isConfigButtonPressed) {
        LOG_INFO(TAG, "Config button pressed. Enable Soft AP server");
    }

    return isMeterNameSet && isTimeZoneSet && isCameraCalibrated && isSchedulerConfigured && isSubscribedToBot && wifiHasLogAndPassToConnect(configProp) && !isConfigButtonPressed ;
}

static esp_err_t welcomePageHandler(httpd_req_t *request) {
    LOG_DEBUG(TAG, "In %s handler", CSP_WELCOME_PAGE_NAME);
    char *fullMeterName = getPropertyOrDefault(&wlanConfig, PROPERTY_METER_NAME_KEY, "");
    if (isStringNotEmpty(fullMeterName)) { // Already have configuration, no need to go throw all flow. Move to last page where all settings can be reconfigured
        LOG_INFO(TAG, "Device have initial configuration. Redirecting to %s page", CSP_SUMMARY_PAGE_NAME);
        return summaryPageHandler(request);
    }

    BufferString *meterName = SUBSTRING_CSTR_AFTER(128, fullMeterName, METER_NAME_PREFIX);
    CspObjectMap *paramMap = newCspParamObjMap(8);
    cspAddStrToMap(paramMap, "meterName", meterName->value);
    return renderHtmlTemplate(request, welcomePage, paramMap);
}

static esp_err_t connectPageHandler(httpd_req_t *request) {
    LOG_DEBUG(TAG, "In %s handler", CSP_CONNECT_PAGE_NAME);
    char *meterName = getProperty(&wlanConfig, PROPERTY_METER_NAME_KEY);
    if (isStringEmpty(meterName)) {
        LOG_INFO(TAG, "Meter name not set. Redirecting to %s page", CSP_WELCOME_PAGE_NAME);
        return welcomePageHandler(request);
    }

    wifi_ap_record_t apRecords[DEFAULT_SCAN_LIST_SIZE] = {0};
    uint16_t apCount = scanWifiAccessPoints(apRecords, DEFAULT_SCAN_LIST_SIZE);

    CspObjectMap *paramMap = newCspParamObjMap(16);
    CspObjectArray *apRecordList = mapApRecordsToList(apRecords, apCount);
    cspAddVecToMap(apRecordList, paramMap, "apRecords");
    return renderHtmlTemplate(request, connectPage, paramMap);
}

static esp_err_t calibratePageHandler(httpd_req_t *request) {
    LOG_DEBUG(TAG, "In %s handler", CSP_CALIBRATE_PAGE_NAME);
    if (!propertiesHasKey(&wlanConfig, PROPERTY_FLASH_LIGHT_INTENSITY_KEY)) {
        putProperty(&wlanConfig, PROPERTY_FLASH_LIGHT_INTENSITY_KEY, UINT64_TO_STRING(LEDC_DEFAULT_DUTY)->value);
    }

    if (!propertiesHasKey(&wlanConfig, PROPERTY_CALIBRATION_FOTO_KEY)) {
        putProperty(&wlanConfig, PROPERTY_CALIBRATION_FOTO_KEY, CALIBRATION_PHOTO_NAME);
    }

    int64_t ledDuty;
    cStrToInt64(getProperty(&wlanConfig, PROPERTY_FLASH_LIGHT_INTENSITY_KEY), &ledDuty, 10);
    int ledRange = (int) ((LED_RANGE_LEVEL_COUNT * ledDuty) / LEDC_MAX_DUTY);
    LOG_INFO(TAG, "Flash led range level: [%d]. Duty: [%d]", ledRange, ledDuty);
    setLedIntensity((int32_t) ledDuty);

    initCamera();
    uint32_t size = cameraCaptureToFile(CALIBRATION_PHOTO_NAME);
    ASSERT_400(size > 0, "Error while taking meter photo")

    CspObjectMap *paramMap = newCspParamObjMap(8);
    cspAddStrToMap(paramMap, "calibrationPhotoUrl", "/photo/" CALIBRATION_PHOTO_NAME);
    cspAddIntToMap(paramMap, "rangeLevel", ledRange);
    return renderHtmlTemplate(request, calibratePage, paramMap);
}

static esp_err_t schedulePageHandler(httpd_req_t *request) {
    LOG_DEBUG(TAG, "In %s handler", CSP_SCHEDULE_PAGE_NAME);
    if (!isWifiHasConnection()) {
        LOG_INFO(TAG, "No Wifi connection redirecting to %s page", CSP_CONNECT_PAGE_NAME);
        return connectPageHandler(request);
    }

    CspObjectMap *paramMap = newCspParamObjMap(8);
    return renderHtmlTemplate(request, schedulePage, paramMap);
}

static esp_err_t messagingPageHandler(httpd_req_t *request) {
    LOG_DEBUG(TAG, "In %s handler", CSP_MESSAGING_PAGE_NAME);
    if (!isWifiHasConnection()) {
        LOG_INFO(TAG, "No Wifi connection redirecting to %s page", CSP_CONNECT_PAGE_NAME);
        return connectPageHandler(request);
    }

    // pin should be 4 digit length
    uint32_t fourDigitNumber = (esp_random() % (PIN_NUMBER_UPPER_LIMIT - PIN_NUMBER_LOWER_LIMIT + 1)) + PIN_NUMBER_LOWER_LIMIT;
    BufferString *fourDigitPinStr = UINT64_TO_STRING(fourDigitNumber);

    CspObjectMap *paramMap = newCspParamObjMap(8);
    cspAddStrToMap(paramMap, "botName", getProperty(&appConfig, PROPERTY_TELEGRAM_API_BOT_NAME_KEY));
    cspAddStrToMap(paramMap, "messageId", fourDigitPinStr->value);
    return renderHtmlTemplate(request, messagingPage, paramMap);
}

static esp_err_t summaryPageHandler(httpd_req_t *request) {
    LOG_DEBUG(TAG, "In %s handler", CSP_SUMMARY_PAGE_NAME);
    CspObjectMap *paramMap = newCspParamObjMap(16);

    char *fullMeterName = getPropertyOrDefault(&wlanConfig, PROPERTY_METER_NAME_KEY, "");
    BufferString *meterPostfixName = SUBSTRING_CSTR_AFTER(128, fullMeterName, METER_NAME_PREFIX);
    cspAddStrToMap(paramMap, "fullMeterName", fullMeterName);
    cspAddStrToMap(paramMap, "meterPostfixName", meterPostfixName->value);

    cspAddStrToMap(paramMap, "wifiApName", getPropertyOrDefault(&wlanConfig, PROPERTY_WIFI_SSID_KEY, ""));
    cspAddValToMap(paramMap, "wifiHaveConnection", CSP_BOOL_VALUE(isWifiHasConnection()));

    bool isTimeZoneSet = getProperty(&wlanConfig, PROPERTY_APP_SYSTEM_TIMEZONE_KEY) != NULL;
    cspAddStrToMap(paramMap, "timeZoneName", getPropertyOrDefault(&wlanConfig, PROPERTY_APP_SYSTEM_TIMEZONE_KEY, "UTC"));
    cspAddValToMap(paramMap, "isTimeZoneSet", CSP_BOOL_VALUE(isTimeZoneSet));

    bool isCameraCalibrated = getProperty(&wlanConfig, PROPERTY_FLASH_LIGHT_INTENSITY_KEY) != NULL && getProperty(&wlanConfig, PROPERTY_CALIBRATION_FOTO_KEY) != NULL;
    cspAddValToMap(paramMap, "isCameraCalibrated", CSP_BOOL_VALUE(isCameraCalibrated));

    bool isSchedulerConfigured = getProperty(&wlanConfig, PROPERTY_SYSTEM_CRON_EXPR_KEY) != NULL;
    cspAddValToMap(paramMap, "isSchedulerConfigured", CSP_BOOL_VALUE(isSchedulerConfigured));

    bool isSubscribedToBot = getProperty(&wlanConfig, PROPERTY_TELEGRAM_CHAT_ID_KEY) != NULL;
    cspAddValToMap(paramMap, "isSubscribedToBot", CSP_BOOL_VALUE(isSubscribedToBot));

    if (isSchedulerConfigured && isProjectTimeSet()) {
        parseCronExpression(&cron, getProperty(&wlanConfig, PROPERTY_SYSTEM_CRON_EXPR_KEY));
        ZonedDateTime currentDate = zonedDateTimeNow(&timeZone);
        ZonedDateTime nextDate = nextCronZonedDateTime(&cron, &currentDate);

        char nextDateStr[64] = {0};
        DateTimeFormatter formatter;
        parseDateTimePattern(&formatter, DATE_TIME_FORMAT_SHORT);
        formatZonedDateTime(&nextDate, nextDateStr, sizeof(nextDateStr), &formatter);
        LOG_INFO(TAG, "Next cron trigger date: %s", nextDateStr);
        cspAddStrToMap(paramMap, "nextCronDate", nextDateStr);
    }
    
    return renderHtmlTemplate(request, summaryPage, paramMap);
}

static esp_err_t adminPageHandler(httpd_req_t *request) {
    LOG_DEBUG(TAG, "In %s handler", CSP_ADMIN_PAGE_NAME);
    CspObjectMap *paramMap = newCspParamObjMap(8);

    File *configFile = NEW_FILE(CONFIG_FILE); 
    File *wlanConfigFile = NEW_FILE(WLAN_CONFIG_FILE);
    CspObjectArray *configs = newCspParamObjArray(4);
    cspAddStrToArray(configs, getFileName(configFile, EMPTY_STRING(64))->value);
    cspAddStrToArray(configs, getFileName(wlanConfigFile, EMPTY_STRING(64))->value);

    char *logFilePath = getPropertyOrDefault(&appConfig, PROPERTY_LOG_FILE_PATH_KEY, DEFAULT_LOG_FILE_PATH);
    uint8_t maxBackupFiles = strtol(getPropertyOrDefault(&appConfig, PROPERTY_LOG_FILE_MAX_BACKUPS_KEY, DEFAULT_LOG_FILE_BACKUPS), NULL, 10);

    File *logDir = NEW_FILE(logFilePath);
    File *fileBuffer = mallocPsramHeap(sizeof(struct File) * maxBackupFiles);
    fileVector *logVec = NEW_VECTOR_BUFF(File, file, fileBuffer, maxBackupFiles);
    LOG_INFO(TAG, "Searching in dir: [%s]", logFilePath);
    listFiles(logDir, logVec, false);
    LOG_INFO(TAG, "Found log files: [%d]", fileVecSize(logVec));

    CspObjectArray *logFiles = newCspParamObjArray(8);
    for (uint32_t i = 0; i < fileVecSize(logVec); i++) {
        File logFile = fileVecGet(logVec, i);
        BufferString *logFileName = trimAll(getFileName(&logFile, EMPTY_STRING(64)));
        if (isBuffStringNotEmpty(logFileName)) {
            LOG_INFO(TAG, "Log file: [%s]", logFileName->value);
            cspAddStrToArray(logFiles, logFileName->value);
        }
    }

    cspAddVecToMap(configs, paramMap, "configs");
    cspAddVecToMap(logFiles, paramMap, "logs");
    cspAddStrToMap(paramMap, "fsRootDirName", SUBSTRING_CSTR_AFTER(32, SD_CARD_ROOT, "/")->value);
    cspAddStrToMap(paramMap, "gitBranch", (char *) GIT_BRANCH);
    cspAddStrToMap(paramMap, "gitTag", (char *) GIT_TAG);
    cspAddStrToMap(paramMap, "gitRevision", (char *) GIT_REV);
    cspAddStrToMap(paramMap, "buildTime", (char *) BUILD_TIME);
    freePsramHeap(fileBuffer);
    return renderHtmlTemplate(request, adminPage, paramMap);
}

static esp_err_t saveNameAjaxHandler(httpd_req_t *request) {
    LOG_DEBUG(TAG, "In save name handler");
    JSONObject *rootObject = REQUEST_TO_JSON(request);
    ASSERT_400(rootObject != NULL, "Invalid json");

    char *meterName = trimString(getJsonObjectString(rootObject, "name"));
    ASSERT_JSON_VAL(rootObject, "name")
    BufferString *hostname = NEW_STRING(METER_NAME_MAX_LENGTH, meterName);
    replaceAllOccurrences(hostname, " ", "-");
    LOG_INFO(TAG, "Meter name: [%s]. Hostname: [%s]", meterName, hostname->value);

    putProperty(&wlanConfig, PROPERTY_METER_NAME_KEY, meterName);
    putProperty(&wlanConfig, PROPERTY_WIFI_HOSTNAME_KEY, hostname->value);
    deleteJSONObject(rootObject);
    httpd_resp_sendstr(request, "OK");
    return ESP_OK;
}

static esp_err_t saveWifiCredentialsAjaxHandler(httpd_req_t *request) {
    LOG_DEBUG(TAG, "In connect Ajax handler");
    JSONObject *rootObject = REQUEST_TO_JSON(request);
    ASSERT_400(rootObject != NULL, "Invalid json");

    char *ssid = getJsonObjectString(rootObject, "ssid");
    ASSERT_JSON_VAL(rootObject, "ssid");
    char *password = getJsonObjectOptString(rootObject, "password", "");   // pass can be empty for open AP
    
    // set optional parameters if present
    char *ip = getJsonObjectOptString(rootObject, "ip", "");
    char *gateway = getJsonObjectOptString(rootObject, "gateway", "");
    char *netmask = getJsonObjectOptString(rootObject, "netmask", "");
    char *dns = getJsonObjectOptString(rootObject, "netmask", "");

    putProperty(&wlanConfig, PROPERTY_WIFI_SSID_KEY, ssid);
    putProperty(&wlanConfig, PROPERTY_WIFI_PASSWORD_KEY, password);
    putProperty(&wlanConfig, PROPERTY_WIFI_IP_KEY, ip);
    putProperty(&wlanConfig, PROPERTY_WIFI_GATEWAY_KEY, gateway); 
    putProperty(&wlanConfig, PROPERTY_WIFI_NETMASK_KEY, netmask);
    putProperty(&wlanConfig, PROPERTY_WIFI_DNS_KEY, dns);

    if (!isWifiHasConnection()) {
        esp_err_t status = wifiInitSta(&wlanConfig);
        if (status != ESP_OK) {
            deleteJSONObject(rootObject);
            BufferString *message = STRING_FORMAT(128, "Connection to SSID: %s failed", ssid);
            ASSERT_400(false, message->value)
        }
    }
    vTaskDelay(1000 / portTICK_PERIOD_MS);

    char *geolocationEnabledStr = getPropertyOrDefault(&appConfig, PROPERTY_IP_GEOLOCATION_ENABLED_KEY, "false");
    bool isTimeZoneSet = getProperty(&wlanConfig, PROPERTY_APP_SYSTEM_TIMEZONE_KEY) != NULL; // can be already set in summary
    bool isGeolocationEnabledByParam = strcmp(geolocationEnabledStr, "true") == 0;

    if (isGeolocationEnabledByParam && !isTimeZoneSet) {
        // Call ip geolocation service to receive timezone
        BufferString *url = STRING_FORMAT_128(
            getProperty(&appConfig, PROPERTY_IP_GEOLOCATION_TIMEZONE_FORMAT_URL_KEY), 
            getProperty(&appConfig, PROPERTY_IP_GEOLOCATION_API_KEY));
        LOG_DEBUG(TAG, "Call geolocation to receive timezone: [%s]", url->value);
        esp_http_client_set_url(restClient, url->value);
        esp_http_client_set_method(restClient, HTTP_METHOD_GET);
        esp_http_client_set_header(restClient, "Content-Type", "application/json");
        esp_http_client_set_header(restClient, "Accept", "*/*");
        esp_http_client_set_header(restClient, "Cache-Control", "no-cache");
        esp_err_t status = esp_http_client_perform(restClient);

        if (status == ESP_OK) {
            uint32_t responseLength = strlen(httpResponseBuffer);
            LOG_INFO(TAG, "HTTP GET Status = %d, Content length = %d", esp_http_client_get_status_code(restClient), responseLength);

            if (responseLength > 0) {
                JSONTokener jsonTokener = getJSONTokener(httpResponseBuffer, responseLength);
                JSONObject responseJson = jsonObjectParse(&jsonTokener);
                JSONObject timezoneJson = getJSONObjectFromObject(&responseJson, "time_zone");

                char *timezoneName = getJsonObjectString(&timezoneJson, "name");
                char *currentTime = getJsonObjectString(&timezoneJson, "current_time");
                LOG_DEBUG(TAG, "Received timezone: [%s]. Time: [%s]", timezoneName, currentTime);

                DateTimeFormatter formatter;
                parseDateTimePattern(&formatter, "yyyy-MM-dd HH:mm:ss.SSSZ");
                ZonedDateTime zdateTime = parseToZonedDateTime(currentTime, &formatter);

                if (isDateTimeValid(&zdateTime.dateTime)) {
                    LOG_DEBUG(TAG, "Received time string successfully parsed...");
                    if (timeZone.id != NULL && strcmp(timeZone.id, UTC.id) != 0) {
                        free((char *) timeZone.id);
                    }

                    timeZone.id = strdup(timezoneName);
                    timeZone.utcOffset = zdateTime.zone.utcOffset;
                    timeZone.names = zdateTime.zone.names;
                    LOG_INFO(TAG, "Time zone updated. Zone id: [%s], Offset: %ds", timeZone.id, timeZone.utcOffset);

                } else {
                    LOG_ERROR(TAG, "Invalid date time string received: %s", currentTime);
                }
                putProperty(&wlanConfig, PROPERTY_APP_SYSTEM_TIMEZONE_KEY, timezoneName);
                deleteJSONObject(&responseJson);
            }
            
        } else {
            LOG_ERROR(TAG, "HTTP GET request failed: %s", esp_err_to_name(status));
        }

        esp_http_client_close(restClient);

    } else {
        char *message = !isGeolocationEnabledByParam ? "Disabled in param: " PROPERTY_IP_GEOLOCATION_ENABLED_KEY : "Time zone already set";
        LOG_INFO(TAG, "Geo IP service disabled: [%s]", message);
    }

    setupNtpTime(); // init ntp time, at this moment the internet should be accessible
    loadTimezoneHistoricRules(&timeZone);   // load DST if time zone set
    deleteJSONObject(rootObject);
    httpd_resp_sendstr(request, "Connection success");
    return ESP_OK;
}

static esp_err_t imageCaptureAjaxHandler(httpd_req_t *request) {
    LOG_DEBUG(TAG, "In image capture Ajax handler. URI: [%s]", request->uri);
    BufferString *uriStr = NEW_STRING(HTTPD_MAX_URI_LEN, request->uri);
    BufferString *rangeStr = SUBSTRING_AFTER(32, uriStr, "ledRange=");
    LOG_DEBUG(TAG, "Received param value: [%s]", rangeStr->value);

    int64_t ledLevel = 0;
    ASSERT_400(stringToI64(rangeStr, &ledLevel, 10) == STR_TO_I64_SUCCESS, "Invalid flash light range value");
    int32_t ledDuty = rangeLevelToLightDuty((uint8_t) ledLevel);
    setLedIntensity(ledDuty);

    uint32_t size = cameraCaptureToFile(CALIBRATION_PHOTO_NAME);
    ASSERT_400(size > 0, "Error while taking meter photo");

    putProperty(&wlanConfig, PROPERTY_FLASH_LIGHT_INTENSITY_KEY, UINT64_TO_STRING(ledDuty)->value);
    httpd_resp_sendstr(request, "/photo/" CALIBRATION_PHOTO_NAME);
    return ESP_OK;
}

static esp_err_t cronAjaxHandler(httpd_req_t *request) {
    LOG_DEBUG(TAG, "In cron save Ajax handler");
    JSONObject *rootObject = REQUEST_TO_JSON(request);
    ASSERT_400(rootObject != NULL, "Invalid json");

    char *cronStr = getJsonObjectString(rootObject, "cron");
    ASSERT_JSON_VAL(rootObject, "cron");
    LOG_DEBUG(TAG, "Received cron expression: [%s]", cronStr);

    CronStatus cronStatus = parseCronExpression(&cron, cronStr);
    if (cronStatus != CRON_OK) {
        deleteJSONObject(rootObject);
        ASSERT_400(false, "Invalid cron expression")
    }

    putProperty(&wlanConfig, PROPERTY_SYSTEM_CRON_EXPR_KEY, cronStr);
    deleteJSONObject(rootObject);
    httpd_resp_sendstr(request, "Cron saved");
    return ESP_OK;
}

static esp_err_t telegramMessageAjaxHandler(httpd_req_t *request) {
    LOG_DEBUG(TAG, "In telegram message Ajax handler");
    JSONObject *rootObject = REQUEST_TO_JSON(request);
    ASSERT_400(rootObject != NULL, "Invalid json");

    char *messageId = getJsonObjectString(rootObject, "message_id");
    ASSERT_JSON_VAL(rootObject, "message_id");
    LOG_DEBUG(TAG, "Received telegram bot message from request id: [%s]", messageId);

    // Call for telegram bot to receive last messages
    esp_err_t status = getLastTelegramMessage();
    if (status == ESP_OK) {
        uint32_t responseLength = strlen(httpResponseBuffer);
        LOG_INFO(TAG, "HTTP GET Status = %d, Content length = %lld", esp_http_client_get_status_code(restClient), responseLength);

        JSONTokener jsonTokener = getJSONTokener(httpResponseBuffer, responseLength);
        JSONObject responseJson = jsonObjectParse(&jsonTokener);
        if (!isJsonObjectOk(&responseJson)) {
            deleteJSONObject(&responseJson);
            ASSERT_500(false, "Invalid json received")
        }

        int chatId = -1;
        JSONArray resultJsonArray = getJSONArrayFromObject(&responseJson, "result");
        for (uint32_t i = 0; i < getJsonArrayLength(&resultJsonArray); i++) {
            JSONObject chatItemJsonObject = getJSONObjectFromArray(&resultJsonArray, i);
            JSONObject messageJsonObject = getJSONObjectFromObject(&chatItemJsonObject, "message");
            BufferString *messageText = NEW_STRING_16(getJsonObjectString(&messageJsonObject, "text"));
            
            if (isStrEndsWithIgnoreCase(messageText, messageId)) {
                JSONObject chatJsonObject = getJSONObjectFromObject(&messageJsonObject, "chat");
                chatId = getJsonObjectInt(&chatJsonObject, "id");
                LOG_INFO(TAG, "Telegram chat id found: %d", chatId);
                putProperty(&wlanConfig, PROPERTY_TELEGRAM_CHAT_ID_KEY, INT64_TO_STRING(chatId)->value);
            }
        }

        if (chatId == -1) {
            deleteJSONObject(&responseJson);
            ASSERT_400(false, "Chat id not found by provided message id")
        }

        char *meterName = getProperty(&wlanConfig, PROPERTY_METER_NAME_KEY);
        if (meterName == NULL) {
            deleteJSONObject(&responseJson);
            ASSERT_400(false, "Meter name is not provided. Send message aborted...")
        }

        BufferString *message = STRING_FORMAT_128("%s has been subscribed", meterName);
        status = sendTelegramMessage(message->value);
        if (status == ESP_OK) {
            LOG_INFO(TAG, "Subscription message sent");
        }

        deleteJSONObject(&responseJson);
        httpd_resp_sendstr(request, "Telegram chat id saved");
        return ESP_OK;
    }

    esp_http_client_close(restClient);
    httpd_resp_send_err(request, HTTPD_404_NOT_FOUND, "Failed to found chat id");
    return ESP_FAIL;
}

static esp_err_t timeZoneSearchAjaxHandler(httpd_req_t *request) {
    LOG_DEBUG(TAG, "In time zone search Ajax handler");
    JSONObject *rootObject = REQUEST_TO_JSON(request);
    ASSERT_400(rootObject != NULL, "Invalid json");

    char *zoneName = trimString(getJsonObjectString(rootObject, "zoneName"));
    ASSERT_JSON_VAL(rootObject, "zoneName");
    LOG_DEBUG(TAG, "Time zone to search: [%s]", zoneName);

    char *queryStr = "SELECT zone_name FROM time_zone WHERE lower(zone_name) LIKE lower(:time_zone) LIMIT :max_length";
    BufferString *zoneNamePattern = joinChars(EMPTY_STRING(32), "", 3, "%", zoneName, "%");
    str_DbValueMap *params = SQL_PARAM_MAP("time_zone", zoneNamePattern->value, "max_length", 10);
    ResultSet *rs = executeCallbackQuery(embeddedDb, queryStr, params);

    JSONTokener jsonTokener = createEmptyJSONTokener();
    JSONObject outputJson = createJsonObject(&jsonTokener);
    JSONArray zoneList = createJsonArray(&jsonTokener);

    uint32_t rows = 0;
    while (nextResultSet(rs)) {
        const char *zoneIdName = rsGetString(rs, "zone_name");
        jsonArrayPut(&zoneList, (char *) zoneIdName);
        rows++;
    }

    LOG_DEBUG(TAG, "Rows fetched: [%d]", rows);
    jsonObjectAddArray(&outputJson, "zones", &zoneList);
    static char buffer[512] = {0};
    memset(buffer, 0, sizeof(buffer));
    jsonObjectToString(&outputJson, buffer, ARRAY_SIZE(buffer));

    resultSetDelete(rs);
    deleteJSONObject(rootObject);
    deleteJSONObject(&outputJson);

    httpd_resp_set_hdr(request, "Content-Type", "application/json");
    httpd_resp_sendstr(request, buffer);
    return ESP_OK;
}

static esp_err_t timeZoneSaveAjaxHandler(httpd_req_t *request) {
    LOG_DEBUG(TAG, "In time zone save Ajax handler");
    JSONObject *rootObject = REQUEST_TO_JSON(request);
    ASSERT_400(rootObject != NULL, "Invalid json");

    char *zoneName = trimString(getJsonObjectString(rootObject, "timeZone"));
    ASSERT_JSON_VAL(rootObject, "timeZone");
    LOG_DEBUG(TAG, "Selected time zone: [%s]", zoneName);

    char *queryStr = "SELECT COUNT(zone_name) AS count FROM time_zone WHERE lower(zone_name) = lower(:time_zone)";
    ResultSet *rs = executeQuery(embeddedDb, queryStr, SQL_PARAM_MAP("time_zone", zoneName));
    nextResultSet(rs);
    bool isTimeZoneExist = rsGetInt(rs, "count") > 0;
    resultSetDelete(rs);

    ASSERT_404(isTimeZoneExist == true, "Not existing time zone");
    putProperty(&wlanConfig, PROPERTY_APP_SYSTEM_TIMEZONE_KEY, zoneName);

    deleteJSONObject(rootObject);
    httpd_resp_sendstr(request, "Time zone saved");
    return ESP_OK;
}

static esp_err_t summarySaveAjaxHandler(httpd_req_t *request) {
    LOG_DEBUG(TAG, "In summary save Ajax handler");
    bool isMeterNameSet = getProperty(&wlanConfig, PROPERTY_METER_NAME_KEY) != NULL;
    bool isTimeZoneSet = getProperty(&wlanConfig, PROPERTY_APP_SYSTEM_TIMEZONE_KEY) != NULL;
    bool isCameraCalibrated = getProperty(&wlanConfig, PROPERTY_FLASH_LIGHT_INTENSITY_KEY) != NULL && getProperty(&wlanConfig, PROPERTY_CALIBRATION_FOTO_KEY) != NULL;
    bool isSchedulerConfigured = getProperty(&wlanConfig, PROPERTY_SYSTEM_CRON_EXPR_KEY) != NULL;
    bool isSubscribedToBot = getProperty(&wlanConfig, PROPERTY_TELEGRAM_CHAT_ID_KEY) != NULL;

    ASSERT_400(isMeterNameSet == true, "Meter name is not set");
    ASSERT_400(isWifiHasConnection() == true, "Device is not connected to Wi-Fi");
    ASSERT_400(isTimeZoneSet == true, "Time zone is not set");
    ASSERT_400(isCameraCalibrated == true, "Camera is not calibrated");
    ASSERT_400(isSchedulerConfigured == true, "Scheduler is not set");
    ASSERT_400(isSubscribedToBot == true, "Telegram subscription is not configured");

    ZonedDateTime zdtNow = zonedDateTimeNow(&timeZone);
    char buffer[32];
    DateTimeFormatter formatter;
    parseDateTimePattern(&formatter, DATE_TIME_FORMAT_SHORT);
    formatDateTime(&zdtNow.dateTime, buffer, sizeof(buffer), &formatter);

    putProperty(&wlanConfig, PROPERTY_PREVIOUS_CRON_DATE_KEY, buffer);
    propertiesRemove(&wlanConfig, PROPERTY_ENABLE_CONFIG_KEY);  // remove button press config value if present
    storeProperties(&wlanConfig, WLAN_CONFIG_FILE);
    LOG_INFO(TAG, "All properties saved. Restarting...");

    destroyWifi();
    configureButtonWakeup();
    enterTimerDeepSleep(calculateSecondsToWaitFromNow());
    return ESP_OK;
}

static esp_err_t adminConfigPropertiesAjaxHandler(httpd_req_t *request) {
    LOG_DEBUG(TAG, "In admin properties Ajax handler");
    BufferString *uriStr = NEW_STRING(HTTPD_MAX_URI_LEN, request->uri);
    BufferString *configFileName = SUBSTRING_AFTER(64, uriStr, "configFileName=");
    ASSERT_400(isBuffStringNotEmpty(configFileName) == true, "Empty config file name");
    LOG_DEBUG(TAG, "Config file name: [%s]", configFileName->value);

    Properties *configProp = getPropertiesByFileName(configFileName->value);
    if (configProp == NULL) {
        BufferString *message = STRING_FORMAT_64("Unknown config file [%S]", configFileName);
        ASSERT_404(false, message->value)
    }
    Properties *configPropCopy = LOAD_PROPERTIES(configProp->file.path);    // create the copy of original properties
    qsort(configPropCopy->map->entries, configPropCopy->map->capacity, sizeof(MapEntry), propertyEntryKeyCompareFun);   // sort by key name prefix

    // Map properties to Json
    JSONTokener jsonTokener = createEmptyJSONTokener();
    JSONObject rootObject = createJsonObject(&jsonTokener);
    JSONArray pairsArray = createJsonArray(&jsonTokener);
    HashMapIterator iterator = getHashMapIterator(configPropCopy->map);
    while (hashMapHasNext(&iterator)) {
        JSONObject keyValuePairObject = createJsonObject(&jsonTokener);
        jsonObjectPut(&keyValuePairObject, "key", (char *) iterator.key);
        jsonObjectPut(&keyValuePairObject, "value", (char *) iterator.value);
        jsonArrayAddObject(&pairsArray, &keyValuePairObject);
    }

    jsonObjectAddArray(&rootObject, "pairs", &pairsArray);
    char buffer[2048] = {0};
    jsonObjectToString(&rootObject, buffer, ARRAY_SIZE(buffer));

    deleteJSONObject(&rootObject);
    deleteConfigProperties(configPropCopy);
    httpd_resp_set_hdr(request, "Content-Type", "application/json");
    httpd_resp_sendstr(request, buffer);
    return ESP_OK;
}

static esp_err_t adminUpdatePropertyAjaxHandler(httpd_req_t *request) {
    LOG_DEBUG(TAG, "In admin update property Ajax handler");
    JSONObject *rootObject = REQUEST_TO_JSON(request);
    ASSERT_400(rootObject != NULL, "Invalid json");

    char *configFileName = trimString(getJsonObjectString(rootObject, "propertyFileName"));
    ASSERT_JSON_VAL(rootObject, "propertyFileName");
    LOG_INFO(TAG, "Config file name: [%s]", configFileName);

    char *propertyKey = trimString(getJsonObjectString(rootObject, "key"));
    ASSERT_JSON_VAL(rootObject, "key");
    LOG_INFO(TAG, "Config key: [%s]", propertyKey);

    char *propertyValue = trimString(getJsonObjectString(rootObject, "value")); // no need to validate, can be empty
    LOG_INFO(TAG, "Config new value: [%s]", propertyValue);

    Properties *configProp = getPropertiesByFileName(configFileName);
    if (configProp == NULL) {
        BufferString *message = STRING_FORMAT_64("Unknown config file: [%S]", configFileName);
        deleteJSONObject(rootObject);
        ASSERT_404(false, message->value)
    }

    putProperty(configProp, propertyKey, propertyValue);
    deleteJSONObject(rootObject);
    httpd_resp_sendstr(request, "Key updated");
    return ESP_OK;
}

static esp_err_t adminRemovePropertyAjaxHandler(httpd_req_t *request) {
    LOG_DEBUG(TAG, "In admin remove property Ajax handler");
    JSONObject *rootObject = REQUEST_TO_JSON(request);
    ASSERT_400(rootObject != NULL, "Invalid json");

    char *configFileName = trimString(getJsonObjectString(rootObject, "propertyFileName"));
    ASSERT_JSON_VAL(rootObject, "propertyFileName");
    LOG_INFO(TAG, "Config file name: [%s]", configFileName);

    char *propertyKey = trimString(getJsonObjectString(rootObject, "key"));
    ASSERT_JSON_VAL(rootObject, "key");
    LOG_INFO(TAG, "Config key to remove: [%s]", propertyKey);

    Properties *configProp = getPropertiesByFileName(configFileName);
    if (configProp == NULL) {
        BufferString *message = STRING_FORMAT_64("Unknown config file: [%S]", configFileName);
        LOG_ERROR(TAG, "%s", message->value);
        httpd_resp_send_err(request, HTTPD_404_NOT_FOUND, message->value);
        deleteJSONObject(rootObject);
        return ESP_FAIL;
    }

    propertiesRemove(configProp, propertyKey);
    deleteJSONObject(rootObject);
    httpd_resp_sendstr(request, "Key removed");
    return ESP_OK;
}

static esp_err_t adminGetLogContentsAjaxHandler(httpd_req_t *request) {
    LOG_DEBUG(TAG, "In admin log content Ajax handler");
    BufferString *uriStr = NEW_STRING(HTTPD_MAX_URI_LEN, request->uri);
    BufferString *logFileName = SUBSTRING_AFTER(64, uriStr, "logFileName=");
    ASSERT_400(isBuffStringNotEmpty(logFileName) == true, "Empty log file name");
    LOG_DEBUG(TAG, "Log file name: [%s]", logFileName->value);

    char *logFilePath = getPropertyOrDefault(&appConfig, PROPERTY_LOG_FILE_PATH_KEY, DEFAULT_LOG_FILE_PATH);
    uint8_t maxBackupFiles = strtol(getPropertyOrDefault(&appConfig, PROPERTY_LOG_FILE_MAX_BACKUPS_KEY, DEFAULT_LOG_FILE_BACKUPS), NULL, 10);

    File *fileBuffer = malloc(sizeof(struct File) * maxBackupFiles);
    fileVector *logVec = NEW_VECTOR_BUFF(File, file, fileBuffer, maxBackupFiles);
    LOG_INFO(TAG, "Searching in: [%s]", logFilePath);
    File *logDir = NEW_FILE(logFilePath);
    listFiles(logDir, logVec, false);
    LOG_INFO(TAG, "Found log files: [%d]", fileVecSize(logVec));

    for (uint32_t i = 0; i < fileVecSize(logVec); i++) {
        File logFile = fileVecGet(logVec, i);
        BufferString *logName = getFileName(&logFile, EMPTY_STRING(64));
        if (isBuffStrEquals(logFileName, logName)) {
            free(fileBuffer);
            return sendFile(request, logFile.path);
        }
    }

    BufferString *message = STRING_FORMAT_128("Log file not found: [%S]", logFileName);
    LOG_ERROR(TAG, "%s", message->value);
    httpd_resp_send_err(request, HTTPD_404_NOT_FOUND, message->value);
    free(fileBuffer);
    return ESP_FAIL;
}

static esp_err_t adminCleanLogFileAjaxHandler(httpd_req_t *request) {
    LOG_DEBUG(TAG, "In admin delete log file Ajax handler");
    JSONObject *rootObject = REQUEST_TO_JSON(request);
    ASSERT_400(rootObject != NULL, "Invalid json");

    char *logFileName = trimString(getJsonObjectString(rootObject, "logFileName"));
    ASSERT_JSON_VAL(rootObject, "logFileName");
    LOG_INFO(TAG, "Log file name to delete: [%s]", logFileName);

    char *logFilePath = getPropertyOrDefault(&appConfig, PROPERTY_LOG_FILE_PATH_KEY, DEFAULT_LOG_FILE_PATH);
    File *logDir = NEW_FILE(logFilePath);
    File *logFile = FILE_OF(logDir, logFileName);
    fclose(logFile->file);
    writeCharsToFile(logFile, "Empty", 5, false);

    LOG_INFO(TAG, "Log file cleared: [%s]", logFile->path);
    httpd_resp_sendstr(request, "Log file cleared");
    return ESP_OK;
}

static esp_err_t adminDirectoryContentsAjaxHandler(httpd_req_t *request) {
    LOG_DEBUG(TAG, "In admin directory content Ajax handler");
    BufferString *uriStr = NEW_STRING(PATH_MAX_LEN, request->uri);
    BufferString *dirPathStr = SUBSTRING_AFTER(PATH_MAX_LEN, uriStr, "dirPath=");
    ASSERT_400(isBuffStringNotEmpty(dirPathStr), "Empty directory path");
    LOG_DEBUG(TAG, "Request content for directory: [%s]", dirPathStr->value);

    File *dir = NEW_FILE(dirPathStr->value);
    ASSERT_400(isDirectory(dir), "Only existing directory is allowed")

    LOG_INFO(TAG, "Searching for content in directory: [%s]", dir->path);
    File *fileBuffer = calloc(MAX_FILES_IN_DIR + 1, sizeof(struct File));
    fileVector *contentVec = NEW_VECTOR_BUFF(File, file, fileBuffer, MAX_FILES_IN_DIR + 1);
    listFilesAndDirs(dir, contentVec, false);
    LOG_INFO(TAG, "Total files and dirs fetched: [%d]", fileVecSize(contentVec));

    // Map files and directories to Json
    JSONTokener jsonTokener = createEmptyJSONTokener();
    JSONObject rootObject = createJsonObject(&jsonTokener);
    JSONArray contentArray = createJsonArray(&jsonTokener);
    for (uint32_t i = 0; i < fileVecSize(contentVec); i++) {
        File *contentItem = &contentVec->items[i];
        JSONObject itemObject = createJsonObject(&jsonTokener);
        jsonObjectPut(&itemObject, "type", isDirectory(contentItem) ? "dir" : "file");
        jsonObjectPut(&itemObject, "path", contentItem->path);
        jsonArrayAddObject(&contentArray, &itemObject);
    }

    jsonObjectAddArray(&rootObject, "content", &contentArray);
    size_t bufferSize = (MAX_FILES_IN_DIR * PATH_MAX_LEN) + 4096;
    char *buffer = callocPsramHeap(bufferSize, sizeof(char));
    jsonObjectToString(&rootObject, buffer, bufferSize);

    httpd_resp_set_hdr(request, "Content-Type", "application/json");
    httpd_resp_sendstr(request, buffer);
    deleteJSONObject(&rootObject);
    free(fileBuffer);
    freePsramHeap(buffer);
    return ESP_OK;
}

static esp_err_t uploadFileAjaxHandler(httpd_req_t *request) {
    LOG_DEBUG(TAG, "In admin upload file Ajax handler");
    BufferString *filePathStr = SUBSTRING_CSTR_AFTER(PATH_MAX_LEN, (char *) request->uri, "/admin/upload/file");
    ASSERT_400(isBuffStringNotEmpty(filePathStr), "Empty file save to path")
    LOG_INFO(TAG, "File to save: [%s]", filePathStr->value);

    File *uploadFile = NEW_FILE(filePathStr->value);
    if (isFileExists(uploadFile)) {
        LOG_INFO(TAG, "File [%s] already exist. Removing first", uploadFile->path);
        remove(uploadFile->path);
    }

    createFileDirs(uploadFile); // create user defined dirs if present
    createFile(uploadFile);     // create file itself
    uploadFile->file = fopen(uploadFile->path, "w");
    ASSERT_500(uploadFile->file != NULL, "Failed to create file");
    LOG_INFO(TAG, "Receiving file: %s...", uploadFile->path);

    char *chunk = getScratchBufferPointer();
    size_t remainingLength = request->content_len;
    LOG_INFO(TAG, "File remaining length: %zu", remainingLength);

    while (remainingLength > 0) {
        LOG_INFO(TAG, "Remaining length: %zu", remainingLength);
        int receivedLength = httpd_req_recv(request, chunk, MIN(remainingLength, SERVER_FILE_SCRATCH_BUFFER_SIZE));
        if (receivedLength <= 0) {
            if (receivedLength == HTTPD_SOCK_ERR_TIMEOUT) {
                continue;
            }

            fclose(uploadFile->file);
            remove(uploadFile->path);
            ASSERT_500(false, "File reception failed!")
        }

        size_t bytesWritten = fwrite(chunk, 1, receivedLength, uploadFile->file);
        if (bytesWritten != receivedLength) {
            fclose(uploadFile->file);
            remove(uploadFile->path);
            ASSERT_500(false, "Failed to write file to storage")
        }

        remainingLength -= receivedLength;
    }

    fclose(uploadFile->file);
    httpd_resp_send_chunk(request, NULL, 0);

    LOG_INFO(TAG, "File reception complete");
    return ESP_OK;
}

static esp_err_t deleteFileAjaxHandler(httpd_req_t *request) {
    LOG_DEBUG(TAG, "In admin delete file Ajax handler");
    BufferString *uriStr = NEW_STRING(HTTPD_MAX_URI_LEN, request->uri);
    BufferString *filePathStr = SUBSTRING_AFTER(PATH_MAX_LEN, uriStr, "filePath=");
    ASSERT_400(isBuffStringNotEmpty(filePathStr) == true, "Empty file path")
    LOG_DEBUG(TAG, "Request to delete file: [%s]", filePathStr->value);

    File *fileToDelete = NEW_FILE(filePathStr->value);
    ASSERT_400(isFileExists(fileToDelete), "Not existing file")
    remove(fileToDelete->path);
    httpd_resp_sendstr(request, "File deleted");
    return ESP_OK;
}

static esp_err_t restartEspAjaxHandler(httpd_req_t *request) {
    storeProperties(&wlanConfig, WLAN_CONFIG_FILE);
    storeProperties(&appConfig, CONFIG_FILE);
    LOG_INFO(TAG, "All properties saved. Restarting...");
    esp_restart();
}

static CspObjectArray *mapApRecordsToList(wifi_ap_record_t *apRecords, uint16_t length) {
    CspObjectArray *apList = newCspParamObjArray(length);
    qsort(apRecords, length, sizeof(wifi_ap_record_t), apRecordComparator);

    for (uint16_t i = 0; i < length; i++) {
       wifi_ap_record_t apRecord = apRecords[i];
       CspObjectMap *apObject = newCspParamObjMap(8);
       cspAddStrToMap(apObject, "ssid", (char *) apRecord.ssid);
       cspAddStrToMap(apObject, "authMode", wifiAccessPointAuthModeToStr(apRecord.authmode));
       cspAddIntToMap(apObject, "signalStrength", apRecord.rssi);
       cspAddMapToArray(apObject, apList);
       LOG_DEBUG(TAG, "AP Record: SSID: %s, Auth: %s, RSSI: %d", (char *) apRecord.ssid, wifiAccessPointAuthModeToStr(apRecord.authmode), apRecord.rssi);
    }
    return apList;
}

static int apRecordComparator(const void *v1, const void *v2) { // compare access points by it signal strength
    const wifi_ap_record_t *p1 = (wifi_ap_record_t *)v1;
    const wifi_ap_record_t *p2 = (wifi_ap_record_t *)v2;
    if (p1->rssi < p2->rssi) {
        return 1;

    } else if (p1->rssi > p2->rssi) {
        return -1;
        
    } else {
        return 0;
    }
}

static Properties *getPropertiesByFileName(const char *configFileName) {
    File *configFile = NEW_FILE(CONFIG_FILE); 
    File *wlanConfigFile = NEW_FILE(WLAN_CONFIG_FILE);
    if (isStringEquals(configFileName, getFileName(configFile, EMPTY_STRING(64))->value)) {
        return &appConfig;

    } else if (isStringEquals(configFileName, getFileName(wlanConfigFile, EMPTY_STRING(64))->value)) {
        return &wlanConfig;

    }
    return NULL;
}

static int propertyEntryKeyCompareFun(const void *one, const void *two) {
    BufferString *firstKey = SUBSTRING_CSTR_BEFORE(PATH_MAX_LEN, ((MapEntry *) one)->key, ".");
    BufferString *secondKey = SUBSTRING_CSTR_BEFORE(PATH_MAX_LEN, ((MapEntry *) two)->key, ".");
    return strcmp(stringValue(firstKey), stringValue(secondKey));
}
