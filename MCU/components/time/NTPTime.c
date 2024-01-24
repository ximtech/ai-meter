#include "NTPTime.h"

static const char *TAG = "SNTP";

TimeZone timeZone = UTC;
static TimeZoneRule TIMEZONE_RULES[TIMEZONE_RULES_LENGTH] = {0};

static bool isNtpTimeEnabled = true;
static bool isTimeWasNotSetAtBootPrintStartBlock = false;
static char buffer[128] = {0};

static void timeSyncNotificationCallback(struct timeval *tv);


bool setupNtpTime() {
    LOG_DEBUG(TAG, "Init NTP time start...");

    char *ntpEnabledStr = getPropertyOrDefault(&appConfig, "ntp.time.enable", "false");
    isNtpTimeEnabled = strcmp(ntpEnabledStr, "true") == 0;
    if (!isNtpTimeEnabled) {
        LOG_INFO(TAG, "NTP config disabled");
        return false;
    }

    bool isNtpServerProvided = propertiesHasKey(&appConfig, "ntp.time.server");
    if (!isNtpServerProvided) {
        LOG_INFO(TAG, "TimeServer config empty, disabling NTP");
        return false;
    }

    /* The RTC keeps the time after a restart (Except on Power On or Pin Reset) 
     * There should only be a minor correction through NTP */
    char *formattedTime = getCurrentTimeString(DATE_TIME_FORMAT_ZONE_STR);
    if (isProjectTimeSet()) {
        LOG_INFO(TAG, "Time is set: [%s]", formattedTime);
    }

    char *timeServer = getProperty(&appConfig, "ntp.time.server");
    LOG_INFO(TAG, "Configuring NTP Client...");
    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, timeServer);
    esp_sntp_init();

    sntp_set_time_sync_notification_cb(timeSyncNotificationCallback);

    uint8_t counter = 0;
    while (sntp_get_sync_status() != SNTP_SYNC_STATUS_COMPLETED && counter < 5) {   // wait some time for sync
        vTaskDelay(pdMS_TO_TICKS(5000));
        counter++;
    }

    if (!isProjectTimeSet()) {
        LOG_WARN(TAG, "The local time is unknown, starting with: [%s]", formattedTime);
        if (isNtpTimeEnabled) {
            LOG_INFO(TAG, "Once the NTP server provides a time, we will switch to that one");
            isTimeWasNotSetAtBootPrintStartBlock = true;
        }
    }
    return true;
}

bool isProjectTimeSet() {
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);
    // Is time set? If not, tm_year will be (1970 - 1900).
    return (timeinfo.tm_year < (2023 - 1900)) ? false : true;
}

bool isNtpTimeSyncEnabled() {
    return isNtpTimeEnabled;
}

void setupUserTimeZone() {
    if (propertiesHasKey(&appConfig, PROPERTY_APP_SYSTEM_TIMEZONE_KEY)) {
        char *timeZoneName = getPropertyOrDefault(&appConfig, PROPERTY_APP_SYSTEM_TIMEZONE_KEY, "UTC");
        LOG_INFO(TAG, "Time zone is set from [%s] config: [%s]", CONFIG_FILE, timeZoneName);
        timeZone = findTimeZoneInDb(timeZoneName);

    } else if (propertiesHasKey(&wlanConfig, PROPERTY_APP_SYSTEM_TIMEZONE_KEY)) {
        char *timeZoneName = getProperty(&wlanConfig, PROPERTY_APP_SYSTEM_TIMEZONE_KEY);
        LOG_INFO(TAG, "Time zone is set from [%s] config: [%s]", WLAN_CONFIG_FILE, timeZoneName);
        timeZone = findTimeZoneInDb(timeZoneName);

    } else {
        LOG_INFO(TAG, "User timezone has been set to default: [%s]", timeZone.id);
    }
}

TimeZone findTimeZoneInDb(const char *zoneId) {
    TimeZone resultTimeZone = UTC;
    ResultSet *rs = executeQuery(embeddedDb, "SELECT * FROM time_zone WHERE LOWER(zone_name) = LOWER(:zoneId)", SQL_PARAM_MAP("zoneId", (char *) zoneId));
    if (rs == NULL) {
        LOG_INFO(TAG, "Timezone: [%s] not found in DB returning default UTC", zoneId);
        return resultTimeZone;
    }

    nextResultSet(rs);  // take first one only
    resultTimeZone.id = strdup(rsGetString(rs, "zone_name"));
    resultTimeZone.utcOffset = rsGetInt(rs, "gmt_offset");

    sqlite3_finalize(rs->stmt);
    resultSetDelete(rs);
    return resultTimeZone;
}

char *getNtpServerName() {
    memset(buffer, 0, sizeof(buffer));
    if (esp_sntp_getservername(0) != NULL) {
        snprintf(buffer, sizeof(buffer), "%s", esp_sntp_getservername(0));
        return buffer;
    }

    ip_addr_t const *ip = esp_sntp_getserver(0);
    if (ipaddr_ntoa_r(ip, buffer, sizeof(buffer)) != NULL) {
        return buffer;
    }
    
    return strcpy(buffer, "");
}

const char *getNtpStatusText(sntp_sync_status_t status) {
    switch (status) {
        case SNTP_SYNC_STATUS_COMPLETED: return "Synchronized";
        case SNTP_SYNC_STATUS_IN_PROGRESS: return "In Progress";
        case SNTP_SYNC_STATUS_RESET: return "Reset";
        default: return "Unknown";
    }
}

char *getCurrentTimeString(const char *format) {
    DateTimeFormatter formatter;
    parseDateTimePattern(&formatter, format);
    ZonedDateTime zDateTime = zonedDateTimeNow(&timeZone);
    formatZonedDateTime(&zDateTime, buffer, sizeof(buffer), &formatter);
    return buffer;
}

void loadTimezoneHistoricRules(TimeZone *userTimeZone) {
    if (isTimeZoneEquals(userTimeZone, &UTC) || isTimeZoneEquals(userTimeZone, &GMT)) {
        LOG_INFO(TAG, "Timezone: %s don't have historic rules... Returning", userTimeZone->id);
        return;
    }

    ZonedDateTime zDateTime = zonedDateTimeNow(userTimeZone);
    zonedDateTimeMinusYears(&zDateTime, 1);   // move back for one year for a transition gap
    int64_t fromEpoch = dateTimeToEpochSecond(&zDateTime.dateTime, zDateTime.offset);

    LOG_INFO(TAG, "Fetching timezone: %s, DST historic rules", userTimeZone->id);
    char *queryStr = "SELECT * FROM time_zone_rules "
                     "WHERE zone_name = :time_zone "
                     "AND time_start > :epoch "
                     "LIMIT :max_length";
    str_DbValueMap *params = SQL_PARAM_MAP("time_zone", (char *) userTimeZone->id, "epoch", fromEpoch, "max_length", TIMEZONE_RULES_LENGTH - 1);
    ResultSet *rs = executeQuery(embeddedDb, queryStr, params);

    uint32_t rows = 0;
    while (nextResultSet(rs) && rows < TIMEZONE_RULES_LENGTH - 1) {
        TimeZoneRule *zoneRule = &TIMEZONE_RULES[rows];
        zoneRule->transition = rsGetI64(rs, "time_start");
        zoneRule->gmtOffset = rsGetInt(rs, "gmt_offset");
        zoneRule->isDaylightTime = (rsGetInt(rs, "dst") == 1);
        rows++;
    }

    resultSetDelete(rs);
    LOG_INFO(TAG, "Total time zone rules rows fetched: [%d]", rows);
    userTimeZone->rules = TIMEZONE_RULES;
}

int64_t calculateSecondsToWaitFromNow() {
    ZonedDateTime zdtNow = zonedDateTimeNow(&timeZone);
    zonedDateTimePlusSeconds(&zdtNow, 1);
    ZonedDateTime nextExecutionZdt = nextCronZonedDateTime(&cron, &zdtNow);
    int64_t currentEpoch = dateTimeToEpochSecond(&zdtNow.dateTime, 0);
    int64_t nextEpoch = dateTimeToEpochSecond(&nextExecutionZdt.dateTime, 0);
    int64_t secondsToSleep = nextEpoch - currentEpoch;
    LOG_INFO(TAG, "Calculated time to sleep: %llds", secondsToSleep);
    return secondsToSleep;
}

void enterTimerDeepSleep(int64_t sleepSeconds) {
    LOG_INFO(TAG, "Enabling timer wakeup, %llds", sleepSeconds);
    esp_sleep_enable_timer_wakeup(sleepSeconds * MICROS_PER_SECOND);
    destroyWifi();              // disconnect from Wi-Fi
    LOG_INFO(TAG, "Entering deep sleep");
    esp_deep_sleep_start();
}

char *zonedDateTimeToStrByFormat(ZonedDateTime *zdt, const char *format) {
    memset(buffer, 0, sizeof(buffer));
    DateTimeFormatter formatter;
    parseDateTimePattern(&formatter, format);
    formatZonedDateTime(zdt, buffer, sizeof(buffer), &formatter);
    return buffer;
}

ZonedDateTime zonedDateTimeNow(const TimeZone *zone) {
    time_t now;
    struct tm timeinfo;
    setenv("TZ", "UTC0", 1);    // All system logs and data should be in the UTC time zone. User-specific zone will be handled separately
    tzset();
    time(&now);
    localtime_r(&now, &timeinfo);
    ZonedDateTime zDateTime = zonedDateTimeOf(timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday, timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec, 0, &UTC);
    return zonedDateTimeWithSameInstant(&zDateTime, zone);
}

static void timeSyncNotificationCallback(struct timeval *tv) {
    if (isTimeWasNotSetAtBootPrintStartBlock) {
        LOG_INFO(TAG, "=================================================");
        LOG_INFO(TAG, "==================== Start ======================");
        LOG_INFO(TAG, "==== Log time before time sync -> 1970-01-01 ====");
        isTimeWasNotSetAtBootPrintStartBlock = false;
    }

    LOG_INFO(TAG, "Time is synced with NTP Server: [%s]", getNtpServerName());
    LOG_INFO(TAG, "Current time: [%s]", getCurrentTimeString(DATE_TIME_FORMAT_ZONE_STR));
}
