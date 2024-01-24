#pragma once

#include "../../src/AppConfig.h"

#include <sys/time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_attr.h"
#include "esp_sleep.h"
#include "esp_sntp.h"
#include "driver/gpio.h"
#include <driver/rtc_io.h>

#include "WifiService.h"

#define TIMEZONE_RULES_LENGTH (8 + 1)   // The last one rule should be empty to recognize array end value 

bool setupNtpTime();

bool isProjectTimeSet();

void setupUserTimeZone();
TimeZone findTimeZoneInDb(const char *zoneId);

char *getNtpServerName();
char *getCurrentTimeString(const char *format);

void loadTimezoneHistoricRules(TimeZone *userTimeZone);
int64_t calculateSecondsToWaitFromNow();
void enterTimerDeepSleep(int64_t sleepSeconds);

char *zonedDateTimeToStrByFormat(ZonedDateTime *zdt, const char *format);