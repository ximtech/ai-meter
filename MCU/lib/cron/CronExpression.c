#include "CronExpression.h"

#define CRON_SECONDS_LENGTH 8
#define CRON_MINUTES_LENGTH 8
#define CRON_HOURS_LENGTH 3
#define CRON_DAY_OF_MONTH_LENGTH 4
#define CRON_MONTHS_LENGTH 2
#define CRON_DAY_OF_WEEK_LENGTH 1
#define CRON_MAX_ATTEMPTS (366 * 5)

#define BIT_READ(value, bit) (((value) >> (bit)) & 0x01)
#define BIT_SET(value, bit) ((value) |= (1UL << (bit)))
#define BIT_CLEAR(value, bit) ((value) &= ~(1UL << (bit)))
#define BIT_WRITE(value, bit, bitValue) ((bitValue) ? BIT_SET((value), (bit)) : BIT_CLEAR((value), (bit)))

#define BITS_IN_BYTE 8
#define BIT_NOT_FOUND (-1)

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof(x)/sizeof((x)[0]))
#endif

#define ERROR_DATE_TIME (DateTime){.date = {0}, .time = {.hours = -1, .minutes = -1, .seconds = -1, .millis = -1}}

typedef enum CronField {
    CRON_SECOND = 0,
    CRON_MINUTE = 1,
    CRON_HOUR = 2,
    CRON_DAY_OF_MONTH = 3,
    CRON_MONTH = 4,
    CRON_DAY_OF_WEEK = 5,
} CronField;

typedef enum QuartzOption {
    // day of month "L"
    QUARTZ_LAST_DAY_OF_MONTH,
    QUARTZ_LAST_WEEKDAY_OF_MONTH,
    QUARTZ_LAST_DAY_OF_MONTH_COMPOSITE, // 1,2,3,L or 21-25,L
    QUARTZ_LAST_DAY_OFFSET,
    QUARTZ_NEAREST_WEEKDAY,
    // weekday "L"
    QUARTZ_LAST_GIVEN_WEEKDAY_OF_MONTH,
    // weekday "#"
    QUARTZ_DAY_OF_WEEK,
    QUARTZ_NUMBER_OF_WEEKDAYS
} QuartzOption;

typedef struct QuartzOptionHolder {
    bool isLastDayOptionSet;
    bool isLastWeekendOptionSet;
    bool isLastDayCompositeOptionSet;
    uint8_t lastDayOffset;
    uint8_t nearestWeekday;
    uint8_t lastWeekdayOfMonth;
    uint8_t numberOfWeekdays;
} QuartzOptionHolder;

static const char * const CRON_MACROS[] = {
        "@yearly",   "0 0 0 1 1 *",
        "@annually", "0 0 0 1 1 *",
        "@monthly",  "0 0 0 1 * *",
        "@weekly",   "0 0 0 * * 0",
        "@daily",    "0 0 0 * * *",
        "@midnight", "0 0 0 * * *",
        "@hourly",   "0 0 * * * *"
};

static const ValueRange WEEKDAY_RANGE = {.min = 0, .max = SUNDAY};

static const char *resolveCronMacros(const char *expression);

static CronStatus parseCronField(CronExpression *cron, CronField cronField, char *cronValue);
static CronStatus parseCronDayOfMonth(CronExpression *cron, char *cronValue);
static CronStatus parseCronWeekDay(CronExpression *cron, char *cronValue);
static CronStatus parseCronDate(CronExpression *cron, CronField cronField, uint8_t *bits, char *cronValue, const ValueRange *range);
static CronStatus setNumberHits(CronExpression *cron, CronField cronField, uint8_t *bits, char *cronValue, const ValueRange *range);
static ValueRange parseCronRange(CronExpression *cron, CronField cronField, char *cronValue, CronStatus *error, const ValueRange *range);
static void replaceMonthOrdinals(char *fieldBuffer);
static void replaceWeekDayOrdinals(char *fieldBuffer);

static void findNextDateTime(CronExpression *cron, DateTime *nextDateTime);
static void findNextMonth(CronExpression *cron, DateTime *nextDateTime);
static void findNextDayOfMonth(CronExpression *cron, DateTime *nextDateTime);
static void findNextHour(CronExpression *cron, DateTime *nextDateTime);
static void findNextMinutes(CronExpression *cron, DateTime *nextDateTime);
static void findNextSeconds(CronExpression *cron, DateTime *nextDateTime);
static void resetDateTimeSettings(CronExpression *cron, CronField fromField, DateTime *dateTime);
static Date getMonthLastWeekDayDate(Date *date);
static Date getLastDayOfMonthDate(Date *date);

static QuartzOptionHolder getQuartzOptions(CronExpression *cron, DateTime *nextDateTime);
static void adjustMonthLastDayOffset(CronExpression *cron, DateTime *nextDateTime, uint8_t lastDayOffset);
static void adjustDateToNearestWeekend(DateTime *nextDateTime, uint8_t nearestWeekday);
static void adjustLastWeekdayOfMonth(DateTime *nextDateTime, uint8_t lastWeekdayOfMonth);
static void adjustDateToWeekdayAndCount(DateTime *nextDateTime, DayOfWeek weekDay, uint8_t numberOfWeekdays);
static uint8_t getQuartzOption(CronExpression *cron, QuartzOption option);
static void setQuartzOption(CronExpression *cron, QuartzOption option, uint8_t value);

static void setBitsInRange(uint8_t *bits, const ValueRange *range);
static void clearBitsInRange(uint8_t *bits, const ValueRange *range);
static int32_t nextSetBit(uint8_t *bits, uint32_t fromIndex, uint32_t bitSetLength);
static void setBit(uint8_t *bits, uint32_t bitIndex);
static void clearBit(uint8_t *bits, uint32_t bitIndex);
static bool isBitSet(const uint8_t *bits, uint32_t bitIndex);


static inline bool isLongNumberValid(int64_t number, const char *valuePointer, const char *endPointer) {
    return ((valuePointer == endPointer) ||               // no digits found
            (errno == ERANGE && number == LLONG_MIN) ||   // underflow occurred
            (errno == ERANGE && number == LLONG_MAX)      // overflow occurred
            ? false : true);
}


CronStatus parseCronExpression(CronExpression *cron, const char *expression) {
    if (isStringBlank(expression)) {
        return CRON_ERROR_EMPTY_STRING;
    }
    memset(cron, 0, sizeof(struct CronExpression));
    expression = resolveCronMacros(expression);

    CronField cronField = CRON_SECOND;
    uint32_t length = strlen(expression);
    char cronValue[length];
    memset(cronValue, 0, length);

    while (isStringNotEmpty(expression) && cronField <= CRON_DAY_OF_WEEK) {
        const char *nextSpacePointer = strpbrk(expression, " ");
        if (nextSpacePointer != expression) { // if there are non-space characters
            uint32_t i = 0;
            while (!isspace((int) *expression) && *expression != '\0' && i < length) {
                cronValue[i] = *expression;
                expression++;
                i++;
            }

            trimString(cronValue);  // remove trailing whitespaces
            if (*cronValue == '?' && (cronField != CRON_DAY_OF_MONTH && cronField != CRON_DAY_OF_WEEK)){
                return CRON_ERROR_QUESTION_MARK_NOT_ALLOWED;
            }

            CronStatus status = parseCronField(cron, cronField, cronValue);
            if (status != CRON_OK) {
                return status;
            }
            memset(cronValue, 0, i); // clear array when job is done
        }

        // advance nextSpacePointer until we hit a non-space character
        while (nextSpacePointer != NULL && *nextSpacePointer != '\0' && *nextSpacePointer == ' ') {
            nextSpacePointer++;
        }
        expression = nextSpacePointer;
        cronField++;
    }

    if (cronField != (CRON_DAY_OF_WEEK + 1) || expression != NULL) {  // check that all fields has been provided
        return CRON_ERROR_INVALID_NUMBER_OF_FIELDS;
    }
    return CRON_OK;
}

DateTime nextCronDateTime(CronExpression *cron, DateTime *date) {
    if (!isDateTimeValid(date)) return ERROR_DATE_TIME;
    DateTime nextDateTime = *date;  // copy original date
    findNextDateTime(cron, &nextDateTime);

    if (isDateTimeEquals(date, &nextDateTime)) {
        // arrived at the original date - round up to the next whole second and try again...
        dateTimePlusSeconds(&nextDateTime, 1);
        findNextDateTime(cron, &nextDateTime);
    }
    return nextDateTime;
}

ZonedDateTime nextCronZonedDateTime(CronExpression *cron, ZonedDateTime *date) {
    DateTime nextDateTime = nextCronDateTime(cron, &date->dateTime);
    return zonedDateTimeOfDateTime(&nextDateTime, &date->zone);
}

static const char *resolveCronMacros(const char *expression) {
    for (int i = 0; i < ARRAY_SIZE(CRON_MACROS); i += 2) {
        if (strcasecmp(CRON_MACROS[i], expression) == 0) {
            return CRON_MACROS[i + 1];
        }
    }
    return expression;
}

static CronStatus parseCronField(CronExpression *cron, CronField cronField, char *cronValue) {
    switch (cronField) {
        case CRON_SECOND:
            return setNumberHits(cron, CRON_SECOND, cron->seconds, cronValue, &SECOND_OF_MINUTE_RANGE);
        case CRON_MINUTE:
            return setNumberHits(cron, CRON_MINUTE, cron->minutes, cronValue, &MINUTE_OF_HOUR_RANGE);
        case CRON_HOUR:
            return setNumberHits(cron, CRON_HOUR, cron->hours, cronValue, &HOUR_OF_DAY_RANGE);
        case CRON_DAY_OF_MONTH:
            return parseCronDayOfMonth(cron, cronValue);
        case CRON_MONTH:
            replaceMonthOrdinals(cronValue);
            return setNumberHits(cron, CRON_MONTH, cron->months, cronValue, &MONTH_OF_YEAR_RANGE);
        case CRON_DAY_OF_WEEK:
            replaceWeekDayOrdinals(cronValue);
            return parseCronWeekDay(cron, cronValue);
    }
    return CRON_ERROR_UNKNOWN_CRON_FIELD;
}

static CronStatus parseCronDayOfMonth(CronExpression *cron, char *cronValue) {
    if (*cronValue == 'L') {
        cronValue++;    // skip "L"

        if (*cronValue == '\0') {
            setQuartzOption(cron, QUARTZ_LAST_DAY_OF_MONTH, true);
            return CRON_OK;

        } else if (*cronValue == 'W') {    // "LW"
            setQuartzOption(cron, QUARTZ_LAST_WEEKDAY_OF_MONTH, true);
            cronValue++;    // skip 'W'
            if (isStringEmpty(cronValue)) {
                return CRON_OK;
            }

        } else if (*cronValue == '-') { // "L-[0-30]"
            cronValue++;    // skip "-"
            char *endPointer = NULL;
            int64_t lastDayOffset = strtoll(cronValue, &endPointer, 10);    // extract offset
            if (!isLongNumberValid(lastDayOffset, cronValue, endPointer) || lastDayOffset > 30) {
                return CRON_ERROR_LAST_DAY_OFFSET_INVALID;
            }
            setQuartzOption(cron, QUARTZ_LAST_DAY_OFFSET, lastDayOffset);
            return CRON_OK;
        }
        return CRON_ERROR_UNRECOGNIZED_CHAR_NEAR_L;
    }

    if (strchr(cronValue, 'W') != NULL) {     // "[0-9]W"
        replaceString(cronValue, "W", "");    // remove "W"
        char *endPointer = NULL;
        int64_t dayOfMonth = strtoll(cronValue, &endPointer, 10);
        if (!isLongNumberValid(dayOfMonth, cronValue, endPointer) || !isValidValue(&DAY_OF_MONTH_RANGE, dayOfMonth)) {
            return CRON_ERROR_INVALID_NEAREST_WEEKDAY_VAlUE;
        }
        setQuartzOption(cron, QUARTZ_NEAREST_WEEKDAY, dayOfMonth);
        return CRON_OK;
    }

    return parseCronDate(cron, CRON_DAY_OF_MONTH, cron->daysOfMonth, cronValue, &DAY_OF_MONTH_RANGE);
}

static CronStatus parseCronWeekDay(CronExpression *cron, char *cronValue) {
    if (strchr(cronValue, 'L') != NULL) {
        replaceString(cronValue, "L", "");  // remove "L"

        // "[0-7]L"
        char *endPointer = NULL;
        int64_t dayOfWeek = strtoll(cronValue, &endPointer, 10);
        if (!isLongNumberValid(dayOfWeek, cronValue, endPointer)) {
            return CRON_ERROR_UNRECOGNIZED_CHAR_NEAR_L;
        }
        if (dayOfWeek == 0) {   // replace zero based week, to ordinary week
            dayOfWeek = SUNDAY;
        }
        if (!isValidValue(&WEEKDAY_RANGE, dayOfWeek)) {
            return CRON_ERROR_INVALID_DAY_OF_WEEK_VALUE_L;
        }
        setQuartzOption(cron, QUARTZ_LAST_GIVEN_WEEKDAY_OF_MONTH, dayOfWeek);
        return CRON_OK;
    }

    if (strchr(cronValue, '#') != NULL) {   // "[0-7]#[0-9]"
        if (!isdigit((int) *cronValue)) {
            return CRON_ERROR_INVALID_DAY_OF_WEEK_VALUE_HASH;
        }
        uint8_t dayOfWeek = *cronValue - '0';
        if (!isValidValue(&WEEKDAY_RANGE, dayOfWeek)) {
            return CRON_ERROR_INVALID_DAY_OF_WEEK_VALUE_HASH;
        }
        setQuartzOption(cron, QUARTZ_DAY_OF_WEEK, dayOfWeek);

        cronValue += 2; // skip dayOfWeek digit and "#"
        if (!isdigit((int) *cronValue)) {
            return CRON_ERROR_INVALID_NUMBER_OF_DAY_OF_MONTH_VALUE_HASH;
        }

        char *endPointer = NULL;
        int64_t dayOfWeekInMonth = strtoll(cronValue, &endPointer, 10);
        if (!isValidValue(&DAY_OF_MONTH_RANGE, dayOfWeekInMonth)) {
            return CRON_ERROR_INVALID_NUMBER_OF_DAY_OF_MONTH_VALUE_HASH;
        }
        setQuartzOption(cron, QUARTZ_NUMBER_OF_WEEKDAYS, dayOfWeekInMonth);
        return CRON_OK;
    }

    CronStatus status = parseCronDate(cron, CRON_DAY_OF_WEEK, cron->daysOfWeek, cronValue, &WEEKDAY_RANGE);
    if (isBitSet(cron->daysOfWeek, 0)) {
        setBit(cron->daysOfWeek, 7);   // cron supports 0 for Sunday. GlobalDateTime use 7, so convert week day
        clearBit(cron->daysOfWeek, 0);
    }
    return status;
}

static CronStatus parseCronDate(CronExpression *cron, CronField cronField, uint8_t *bits, char *cronValue, const ValueRange *range) {
    if (isStringEquals(cronValue, "?")) {
        strcpy(cronValue, "*");
    }
    return setNumberHits(cron, cronField, bits, cronValue, range);
}

static CronStatus setNumberHits(CronExpression *cron, CronField cronField, uint8_t *bits, char *cronValue, const ValueRange *range) {
    CronStatus error = CRON_OK;
    char *nextPointer = NULL;
    char *cronFieldValue = splitStringReentrant(cronValue, ",", &nextPointer);

    while (cronFieldValue != NULL) {
        char *incrementer = strchr(cronFieldValue, '/');
        if (incrementer != NULL) {
            *incrementer = '\0';
            incrementer++;
            if (strchr(incrementer, '/') != NULL) {
                return CRON_ERROR_INCREMENTER_HAS_MORE_THAN_TWO_FIELDS;
            }

            bool haveNoMaxValue = strchr(cronFieldValue, '-') == NULL;
            ValueRange cronRange = parseCronRange(cron, cronField, cronFieldValue, &error, range);    // parse from value
            if (haveNoMaxValue) {
                cronRange.max = range->max;
            }

            char *endPointer = NULL;
            int64_t delta = strtoll(incrementer, &endPointer, 10);  // parse incrementer value
            if (!isLongNumberValid(delta, incrementer, endPointer) || delta <= 0) {
                return CRON_ERROR_INCREMENTER_INVALID_VALUE;
            }
            for (int64_t i = cronRange.min; i <= cronRange.max; i += delta) {
                setBit(bits, i);
            }

        } else {    // Not an incrementer so it must be a range (possibly empty)
            ValueRange cronRange = parseCronRange(cron, cronField, cronFieldValue, &error, range);
            if (error != CRON_OK) return error;
            setBitsInRange(bits, &cronRange);
        }

        cronFieldValue = splitStringReentrant(NULL, ",", &nextPointer);
    }
    return error;
}

static ValueRange parseCronRange(CronExpression *cron, CronField cronField, char *cronValue, CronStatus *error, const ValueRange *range) {
    if (strchr(cronValue, '*') != NULL) return *range;  // full range of bits should be set, from min to max

    ValueRange resultRange = {0};
    char *endPointer = NULL;
    char *secondNumberInRange = strchr(cronValue, '-');
    if (secondNumberInRange != NULL) {
        *secondNumberInRange = '\0';
        secondNumberInRange++;

        if (strchr(secondNumberInRange, '-') != NULL) {
            *error = CRON_ERROR_RANGE_HAS_MORE_THAN_TWO_FIELDS;
            return resultRange;
        }

        int64_t min = strtoll(cronValue, &endPointer, 10);
        if (!isLongNumberValid(min, cronValue, endPointer)) {
            *error = CRON_ERROR_RANGE_INVALID_MIN_VALUE;
            return resultRange;
        }

        if (cronField == CRON_DAY_OF_WEEK && min == 0) {
            min = MONDAY;
        }

        endPointer = NULL;
        int64_t max = strtoll(secondNumberInRange, &endPointer, 10);
        if (!isLongNumberValid(max, secondNumberInRange, endPointer)) {
            *error = CRON_ERROR_RANGE_INVALID_MAX_VALUE;
            return resultRange;
        }

        if (cronField == CRON_DAY_OF_WEEK && min == SUNDAY) {   // If used as a minimum in a range, Sunday means 0 (not 7)
            min = 0;
        }
        resultRange.min = min;
        resultRange.max = max;

    } else {    // parse as single number
        int64_t number = strtoll(cronValue, &endPointer, 10);
        if (!isLongNumberValid(number, cronValue, endPointer)) {
            if (*cronValue == 'L' && cronField == CRON_DAY_OF_MONTH) {
                setQuartzOption(cron, QUARTZ_LAST_DAY_OF_MONTH, true);
                setQuartzOption(cron, QUARTZ_LAST_DAY_OF_MONTH_COMPOSITE, true);
                return resultRange;
            }

            *error = CRON_ERROR_INVALID_SINGLE_VALUE;
            return resultRange;
        }

        if (cronField == CRON_DAY_OF_WEEK && number == 0) {
            number = SUNDAY;
        }
        resultRange.min = number;
        resultRange.max = number;
    }

    if (!isValidValue(range, resultRange.max)) {
        *error = CRON_ERROR_RANGE_EXCEEDS_MAX;
        return resultRange;
    }

    if (!isValidValue(range, resultRange.min)) {
        *error = CRON_ERROR_RANGE_EXCEEDS_MIN;
        return resultRange;
    }

    if (resultRange.min > resultRange.max) {
        *error = CRON_ERROR_RANGE_MIN_GREATER_THAN_MAX;
        return resultRange;
    }
    return resultRange;
}

static void replaceMonthOrdinals(char *fieldBuffer) {
    char monthNameUpper[4] = {0};
    char monthNumber[3] = {0};
    toUpperCaseString(fieldBuffer);
    for (Month i = JANUARY; i <= DECEMBER; i++) {
        const char *monthOrdinal = getMonthNameShort(i);
        strcpy(monthNameUpper, monthOrdinal);
        toUpperCaseString(monthNameUpper);
        sprintf(monthNumber, "%d", i);
        replaceString(fieldBuffer, monthNameUpper, monthNumber);
    }
}

static void replaceWeekDayOrdinals(char *fieldBuffer) {
    char weekNameUpper[4] = {0};
    char weekDayNumber[2] = {0};
    toUpperCaseString(fieldBuffer);
    for (DayOfWeek i = MONDAY; i <= SUNDAY; i++) {
        const char *weekOrdinal = getWeekDayNameShort(i);
        strcpy(weekNameUpper, weekOrdinal);
        toUpperCaseString(weekNameUpper);
        sprintf(weekDayNumber, "%d", i);
        replaceString(fieldBuffer, weekNameUpper, weekDayNumber);
    }
}

static void findNextDateTime(CronExpression *cron, DateTime *nextDateTime) {
    for (CronField field = CRON_SECOND; field <= CRON_DAY_OF_WEEK && isDateTimeValid(nextDateTime); field++) {
        switch (field) {    // No need to handle CRON_DAY_OF_WEEK separately, week settings is used in day of month calculation
            case CRON_MONTH:
                findNextMonth(cron, nextDateTime);
                break;
            case CRON_DAY_OF_MONTH:
                findNextDayOfMonth(cron, nextDateTime);
                break;
            case CRON_HOUR:
                findNextHour(cron, nextDateTime);
                break;
            case CRON_MINUTE:
                findNextMinutes(cron, nextDateTime);
                break;
            case CRON_SECOND:
                findNextSeconds(cron, nextDateTime);
                break;
            default:
                break;
        }
    }
}

static void findNextMonth(CronExpression *cron, DateTime *nextDateTime) {
    int32_t nextMonth = nextSetBit(cron->months, nextDateTime->date.month, CRON_MONTHS_LENGTH);
    if (nextMonth == BIT_NOT_FOUND) {
        nextMonth = nextSetBit(cron->months, 0, CRON_MONTHS_LENGTH);
        dateTimePlusYears(nextDateTime, 1);
    }

    if (nextMonth != nextDateTime->date.month) {
        resetDateTimeSettings(cron, CRON_DAY_OF_MONTH, nextDateTime);
        nextDateTime->date.month = nextMonth;
        int64_t maxYear = nextDateTime->date.year + CRON_MAX_ATTEMPTS;
        while (!isDateTimeValid(nextDateTime) && nextDateTime->date.year <= maxYear) { // check when non leap year set invalid day of february
            nextDateTime->date.year++;
        }

        if (nextDateTime->date.year >= maxYear) {
            *nextDateTime = ERROR_DATE_TIME;
            return;
        }

        nextDateTime->date.weekDay = getDayOfWeek(&nextDateTime->date); // recalculate weekday for new date
        findNextDateTime(cron, nextDateTime);
    }
}

static void findNextDayOfMonth(CronExpression *cron, DateTime *nextDateTime) {
    QuartzOptionHolder options = getQuartzOptions(cron, nextDateTime);
    bool isMonthQuartzOptionsSet = options.isLastDayOptionSet || options.isLastWeekendOptionSet || options.lastDayOffset != 0 || options.nearestWeekday != 0;
    bool isWeekdayQuartzOptionsSet = options.lastWeekdayOfMonth != 0 || options.numberOfWeekdays != 0;

    uint32_t count = 0;
    Month currentMonth = nextDateTime->date.month;
    while (count <= CRON_MAX_ATTEMPTS) {

        if (isMonthQuartzOptionsSet && (nextDateTime->date.month != currentMonth || count == 0)) {   // set month last day when month is changed or in first iteration, also clean previous month last day
            if (!options.isLastDayCompositeOptionSet) {
                clearBitsInRange(cron->daysOfMonth, &DAY_OF_MONTH_RANGE);   // clear previous day of month day of month set bits, do not clear for composite month field
            }

            if (options.isLastDayOptionSet) {   // "L"
                int8_t monthLastDay = lengthOfMonth(nextDateTime->date.month, isLeapYear(nextDateTime->date.year));
                setBit(cron->daysOfMonth, monthLastDay);

            } else if (options.isLastWeekendOptionSet) {    // "LW"
                Date currentMonthEndDate = getMonthLastWeekDayDate(&nextDateTime->date);
                if (!isDateEquals(&currentMonthEndDate, &nextDateTime->date)) {
                    resetDateTimeSettings(cron, CRON_HOUR, nextDateTime);
                }
                setBit(cron->daysOfMonth, currentMonthEndDate.day);

            } else if (options.lastDayOffset != 0) {  // "L-[1-30]"
                adjustMonthLastDayOffset(cron, nextDateTime, options.lastDayOffset);

            } else if (options.nearestWeekday != 0) { // "[0-9]W"
                DateTime copyDateTime = *nextDateTime;
                adjustDateToNearestWeekend(nextDateTime, options.nearestWeekday);
                if (!isDateTimeEquals(&copyDateTime, nextDateTime)) {
                    resetDateTimeSettings(cron, CRON_HOUR, nextDateTime);
                }
                setBit(cron->daysOfMonth, nextDateTime->date.day);
            }
            currentMonth = nextDateTime->date.month;
        }

        if (isWeekdayQuartzOptionsSet && (nextDateTime->date.month != currentMonth || count == 0)) {
            clearBitsInRange(cron->daysOfWeek, &WEEKDAY_RANGE);
            if (options.lastWeekdayOfMonth != 0) {   // "[0-7]L"
                DateTime copyDateTime = *nextDateTime;
                adjustLastWeekdayOfMonth(nextDateTime, options.lastWeekdayOfMonth);
                if (!isDateTimeEquals(&copyDateTime, nextDateTime)) {
                    resetDateTimeSettings(cron, CRON_HOUR, nextDateTime);
                }
                setBit(cron->daysOfWeek, nextDateTime->date.weekDay);

            } else if (options.numberOfWeekdays != 0) {   // "[0-7]#[0-9]"
                DateTime copyDateTime = *nextDateTime;
                DayOfWeek weekDay = getQuartzOption(cron, QUARTZ_DAY_OF_WEEK);
                adjustDateToWeekdayAndCount(nextDateTime, weekDay, options.numberOfWeekdays);
                if (!isDateTimeEquals(&copyDateTime, nextDateTime)) {
                    resetDateTimeSettings(cron, CRON_HOUR, nextDateTime);
                }
                setBit(cron->daysOfWeek, nextDateTime->date.weekDay);
            }
            currentMonth = nextDateTime->date.month;
        }

        if (isBitSet(cron->daysOfMonth, nextDateTime->date.day) &&
            isBitSet(cron->daysOfWeek, nextDateTime->date.weekDay)) {
            break;
        }

        if (count == 0) {   // if we are here, then day of month or week is not in bit set, so reset time once and proceed
            resetDateTimeSettings(cron, CRON_HOUR, nextDateTime);
        }

        dateTimePlusDays(nextDateTime, 1);
        count++;
    }

    if (count >= CRON_MAX_ATTEMPTS) {
        *nextDateTime = ERROR_DATE_TIME;
    }
}

static void findNextHour(CronExpression *cron, DateTime *nextDateTime) {
    int32_t nextHour = nextSetBit(cron->hours, nextDateTime->time.hours, CRON_HOURS_LENGTH);
    if (nextHour == BIT_NOT_FOUND) {
        nextHour = nextSetBit(cron->hours, 0, CRON_HOURS_LENGTH);
        dateTimePlusDays(nextDateTime, 1);
    }

    if (nextHour != nextDateTime->time.hours) {
        nextDateTime->time.hours = (int8_t) nextHour;
        resetDateTimeSettings(cron, CRON_MINUTE, nextDateTime);
    }
}

static void findNextMinutes(CronExpression *cron, DateTime *nextDateTime) {
    int32_t nextMinutes = nextSetBit(cron->minutes, nextDateTime->time.minutes, CRON_MINUTES_LENGTH);
    if (nextMinutes == BIT_NOT_FOUND) {
        nextMinutes = nextSetBit(cron->minutes, 0, CRON_MINUTES_LENGTH);
        dateTimePlusHours(nextDateTime, 1);
    }

    if (nextMinutes != nextDateTime->time.minutes) {
        nextDateTime->time.minutes = (int8_t) nextMinutes;
        resetDateTimeSettings(cron, CRON_SECOND, nextDateTime);
    }
}

static void findNextSeconds(CronExpression *cron, DateTime *nextDateTime) {
    int32_t nextSeconds = nextSetBit(cron->seconds, nextDateTime->time.seconds, CRON_SECONDS_LENGTH);
    if (nextSeconds == BIT_NOT_FOUND) {
        nextSeconds = nextSetBit(cron->seconds, 0, CRON_SECONDS_LENGTH);
        dateTimePlusMinutes(nextDateTime, 1);
    }

    if (nextSeconds != nextDateTime->time.seconds) {
        nextDateTime->time.seconds = (int8_t) nextSeconds;
    }
}

static void resetDateTimeSettings(CronExpression *cron, CronField fromField, DateTime *dateTime) {
    for (CronField field = fromField; field >= CRON_SECOND; field--) {
        switch (field) {
            case CRON_MONTH:
                dateTime->date.month = nextSetBit(cron->months, 0, CRON_MONTHS_LENGTH);
                dateTime->date.month = dateTime->date.month != 0 ? dateTime->date.month : JANUARY;
                break;
            case CRON_DAY_OF_MONTH:
            case CRON_DAY_OF_WEEK:
                dateTime->date.weekDay = nextSetBit(cron->daysOfWeek, 0, CRON_DAY_OF_WEEK_LENGTH);
                dateTime->date.day = (int8_t)nextSetBit(cron->daysOfMonth, 0, CRON_DAY_OF_MONTH_LENGTH);
                dateTime->date.day = dateTime->date.day != 0 ? dateTime->date.day : 1;
                break;
            case CRON_HOUR:
                dateTime->time.hours = (int8_t)nextSetBit(cron->hours, 0, CRON_HOURS_LENGTH);
                break;
            case CRON_MINUTE:
                dateTime->time.minutes = (int8_t)nextSetBit(cron->minutes, 0, CRON_MINUTES_LENGTH);
                break;
            case CRON_SECOND:
                dateTime->time.seconds = (int8_t)nextSetBit(cron->seconds, 0, CRON_SECONDS_LENGTH);
                break;
        }

        if (field == CRON_SECOND) {
            break;
        }
    }
}

static Date getMonthLastWeekDayDate(Date *date) {
    int8_t monthLastDay = lengthOfMonth(date->month, isLeapYear(date->year));
    Date monthLastWeekday = dateOf(date->year, date->month, monthLastDay);
    while (monthLastWeekday.weekDay > FRIDAY) {
        dateMinusDays(&monthLastWeekday, 1);
    }
    return monthLastWeekday;
}

static Date getLastDayOfMonthDate(Date *date) {
    int8_t monthLastDay = lengthOfMonth(date->month, isLeapYear(date->year));
    return dateOf(date->year, date->month, monthLastDay);
}

QuartzOptionHolder getQuartzOptions(CronExpression *cron, DateTime *nextDateTime) {
    QuartzOptionHolder options = {
            .isLastDayOptionSet = getQuartzOption(cron, QUARTZ_LAST_DAY_OF_MONTH),
            .isLastWeekendOptionSet = getQuartzOption(cron, QUARTZ_LAST_WEEKDAY_OF_MONTH),
            .isLastDayCompositeOptionSet = getQuartzOption(cron, QUARTZ_LAST_DAY_OF_MONTH_COMPOSITE),
            .lastDayOffset = getQuartzOption(cron, QUARTZ_LAST_DAY_OFFSET),
            .nearestWeekday = getQuartzOption(cron, QUARTZ_NEAREST_WEEKDAY),
            .lastWeekdayOfMonth = getQuartzOption(cron, QUARTZ_LAST_GIVEN_WEEKDAY_OF_MONTH),
            .numberOfWeekdays = getQuartzOption(cron, QUARTZ_NUMBER_OF_WEEKDAYS)};
    return options;
}

static void adjustMonthLastDayOffset(CronExpression *cron, DateTime *nextDateTime, uint8_t lastDayOffset) {
    Date currentMonthEndDate = getLastDayOfMonthDate(&nextDateTime->date);
    dateMinusDays(&currentMonthEndDate, lastDayOffset);
    setBit(cron->daysOfMonth, currentMonthEndDate.day);
}

static void adjustDateToNearestWeekend(DateTime *nextDateTime, uint8_t nearestWeekday) {
    int8_t monthLastDay = lengthOfMonth(nextDateTime->date.month, isLeapYear(nextDateTime->date.year));
    if (nearestWeekday > monthLastDay) {
        datePlusMonths(&nextDateTime->date, 1);
    }

    Date nearestWeekendDate = dateOf(nextDateTime->date.year, nextDateTime->date.month, nearestWeekday);
    if (isDateBefore(&nearestWeekendDate, &nextDateTime->date)) {
        datePlusMonths(&nearestWeekendDate, 1);
    }

    Month currentMonth = nearestWeekendDate.month;
    if (nearestWeekendDate.weekDay == SATURDAY) {
        if (nearestWeekendDate.day != 1) {
            dateMinusDays(&nearestWeekendDate, 1); // move to Friday
        } else {
            datePlusDays(&nearestWeekendDate, 2); // move to Monday
        }

    } else if (nearestWeekendDate.weekDay == SUNDAY) {
        datePlusDays(&nearestWeekendDate, 1);  // move to Monday
    }

    if (isDateBefore(&nearestWeekendDate, &nextDateTime->date)) {
        datePlusMonths(&nearestWeekendDate, 1);
        nextDateTime->date = nearestWeekendDate;
        adjustDateToNearestWeekend(nextDateTime, nearestWeekday);   // weekday calculated before current date, so move to next month and recalculate
        return;
    }

    if (nearestWeekendDate.month != currentMonth) {
        nextDateTime->date = nearestWeekendDate;
        adjustDateToNearestWeekend(nextDateTime, nearestWeekday);   // month is changed, adjust for new month
        return;
    }
    nextDateTime->date = nearestWeekendDate;
}

static void adjustLastWeekdayOfMonth(DateTime *nextDateTime, uint8_t lastWeekdayOfMonth) {
    Date lastWeekdayDate = getLastDayOfMonthDate(&nextDateTime->date);
    while (lastWeekdayDate.weekDay != lastWeekdayOfMonth) {
        dateMinusDays(&lastWeekdayDate, 1);
    }

    if (isDateBefore(&lastWeekdayDate, &nextDateTime->date)) {
        datePlusMonths(&lastWeekdayDate, 1);    // current month last weekday is before current date, so move to next month and recalculate
        nextDateTime->date = lastWeekdayDate;
        nextDateTime->date.day = 1;
        adjustLastWeekdayOfMonth(nextDateTime, lastWeekdayOfMonth);
        return;
    }
    nextDateTime->date = lastWeekdayDate;
}

static void adjustDateToWeekdayAndCount(DateTime *nextDateTime, DayOfWeek weekDay, uint8_t numberOfWeekdays) {
    Date monthStartDate = dateOf(nextDateTime->date.year, nextDateTime->date.month, 1);
    uint8_t weekDayCounter = 0;
    while (weekDayCounter <= DAY_OF_MONTH_RANGE.max) {
        if (monthStartDate.weekDay == weekDay) {
            weekDayCounter++;
            if (weekDayCounter == numberOfWeekdays) {
                break;
            }
        }
        datePlusDays(&monthStartDate, 1);
    }

    if (weekDayCounter >= DAY_OF_MONTH_RANGE.max) {
        return; // month overflow
    }

    if (isDateBefore(&monthStartDate, &nextDateTime->date)) {
        datePlusMonths(&monthStartDate, 1);
        monthStartDate.day = 1;
        nextDateTime->date = monthStartDate;
        adjustDateToWeekdayAndCount(nextDateTime, weekDay, numberOfWeekdays);
        return;
    }

    if (monthStartDate.month != nextDateTime->date.month) {
        nextDateTime->date = monthStartDate;
        adjustDateToWeekdayAndCount(nextDateTime, weekDay, numberOfWeekdays);
        return;
    }
    nextDateTime->date = monthStartDate;
}

static uint8_t getQuartzOption(CronExpression *cron, QuartzOption option) {
    switch (option) {
        case QUARTZ_LAST_DAY_OF_MONTH:
        case QUARTZ_LAST_WEEKDAY_OF_MONTH:
        case QUARTZ_LAST_DAY_OF_MONTH_COMPOSITE:
            return BIT_READ(cron->quartzOptions[0], option);
        case QUARTZ_LAST_DAY_OFFSET:
        case QUARTZ_NEAREST_WEEKDAY:
            return BIT_READ(cron->quartzOptions[0], option) ? cron->quartzOptions[1] : 0;
        case QUARTZ_LAST_GIVEN_WEEKDAY_OF_MONTH:
        case QUARTZ_DAY_OF_WEEK:
            return BIT_READ(cron->quartzOptions[0], option) ? cron->quartzOptions[2] : 0;
        case QUARTZ_NUMBER_OF_WEEKDAYS:
            return BIT_READ(cron->quartzOptions[0], option) ? cron->quartzOptions[3] : 0;
    }
    return 0;
}

static void setQuartzOption(CronExpression *cron, QuartzOption option, uint8_t value) {
    switch (option) {
        case QUARTZ_LAST_DAY_OF_MONTH:
        case QUARTZ_LAST_WEEKDAY_OF_MONTH:
        case QUARTZ_LAST_DAY_OF_MONTH_COMPOSITE:
            BIT_WRITE(cron->quartzOptions[0], option, value);
            break;
        case QUARTZ_LAST_DAY_OFFSET:
        case QUARTZ_NEAREST_WEEKDAY:
            BIT_WRITE(cron->quartzOptions[0], option, true);
            cron->quartzOptions[1] = value;
            break;
        case QUARTZ_LAST_GIVEN_WEEKDAY_OF_MONTH:
        case QUARTZ_DAY_OF_WEEK:
            BIT_WRITE(cron->quartzOptions[0], option, true);
            cron->quartzOptions[2] = value;
            break;
        case QUARTZ_NUMBER_OF_WEEKDAYS:
            BIT_WRITE(cron->quartzOptions[0], option, true);
            cron->quartzOptions[3] = value;
            break;
    }
}

static void setBitsInRange(uint8_t *bits, const ValueRange *range) {
    for (int64_t i = range->min; i <= range->max; i++) {
        setBit(bits, i);
    }
}

static void clearBitsInRange(uint8_t *bits, const ValueRange *range) {
    for (int64_t i = range->min; i <= range->max; i++) {
        clearBit(bits, i);
    }
}

static int32_t nextSetBit(uint8_t *bits, uint32_t fromIndex, uint32_t bitSetLength) {
    for (uint32_t i = fromIndex; i < bitSetLength * BITS_IN_BYTE; i++) {
        if (isBitSet(bits, i)) {
            return (int32_t) i;
        }
    }
    return BIT_NOT_FOUND;
}

static void setBit(uint8_t *bits, uint32_t bitIndex) {
    uint8_t byteIndex = bitIndex / BITS_IN_BYTE;
    uint8_t bitInByteIndex = bitIndex % BITS_IN_BYTE;
    BIT_SET(bits[byteIndex], bitInByteIndex);
}

static void clearBit(uint8_t *bits, uint32_t bitIndex) {
    uint8_t byteIndex = bitIndex / BITS_IN_BYTE;
    uint8_t bitInByteIndex = bitIndex % BITS_IN_BYTE;
    BIT_CLEAR(bits[byteIndex], bitInByteIndex);
}

static bool isBitSet(const uint8_t *bits, uint32_t bitIndex) {
    uint8_t byteIndex = bitIndex / BITS_IN_BYTE;
    uint8_t bitInByteIndex = bitIndex % BITS_IN_BYTE;
    return BIT_READ(bits[byteIndex], bitInByteIndex);
}
