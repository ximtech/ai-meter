#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#include "StringUtils.h"
#include "GlobalDateTime.h"

typedef enum CronStatus {
    CRON_OK,
    CRON_ERROR_EMPTY_STRING,
    CRON_ERROR_INVALID_NUMBER_OF_FIELDS,
    CRON_ERROR_INCREMENTER_HAS_MORE_THAN_TWO_FIELDS, // '/'
    CRON_ERROR_INCREMENTER_INVALID_VALUE,
    CRON_ERROR_RANGE_HAS_MORE_THAN_TWO_FIELDS,     // '-'
    CRON_ERROR_RANGE_INVALID_MIN_VALUE,
    CRON_ERROR_RANGE_INVALID_MAX_VALUE,
    CRON_ERROR_INVALID_SINGLE_VALUE,
    CRON_ERROR_RANGE_EXCEEDS_MIN,
    CRON_ERROR_RANGE_EXCEEDS_MAX,
    CRON_ERROR_RANGE_MIN_GREATER_THAN_MAX,
    CRON_ERROR_QUESTION_MARK_NOT_ALLOWED,                     // '?' - can only be specified for Day-of-Month or Day-of-Week
    CRON_ERROR_UNRECOGNIZED_CHAR_NEAR_L,
    CRON_ERROR_LAST_DAY_OFFSET_INVALID,                      // 'L' - offset from last day must be <= 30
    CRON_ERROR_INVALID_NEAREST_WEEKDAY_VAlUE,                // 'W' - value
    CRON_ERROR_INVALID_DAY_OF_WEEK_VALUE_L,                  // [0-7]L
    CRON_ERROR_INVALID_DAY_OF_WEEK_VALUE_HASH,               // Invalid first value "[0-7]#[0-9]"
    CRON_ERROR_INVALID_NUMBER_OF_DAY_OF_MONTH_VALUE_HASH,    // Invalid second value "[0-7]#[0-9]"
    CRON_ERROR_UNKNOWN_CRON_FIELD,
} CronStatus;

typedef struct CronExpression {
    uint8_t seconds[8];
    uint8_t minutes[8];
    uint8_t hours[3];
    uint8_t daysOfMonth[4];
    uint8_t months[2];
    uint8_t daysOfWeek[1];       //    0          1           2          3
    uint8_t quartzOptions[4];   // [flags], [dayOfMonth], [weekday], [weekdayNum]
} CronExpression;

/*
         ┌───────────── second (0-59)
         │ ┌───────────── minute (0 - 59)
         │ │ ┌───────────── hour (0 - 23)
         │ │ │ ┌───────────── day of the month (1 - 31)
         │ │ │ │ ┌───────────── month (1 - 12) (or JAN-DEC)
         │ │ │ │ │ ┌───────────── day of the week (0 - 7)
         │ │ │ │ │ │          (or MON-SUN -- 0 or 7 is Sunday)
         │ │ │ │ │ │
         * * * * * *
 */

CronStatus parseCronExpression(CronExpression *cron, const char *expression);

DateTime nextCronDateTime(CronExpression *cron, DateTime *date);
ZonedDateTime nextCronZonedDateTime(CronExpression *cron, ZonedDateTime *date);
