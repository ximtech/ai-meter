#include "SqliteQuery.h"

#define DB_NAMED_PARAM_MAX_LENGTH 128
#define DB_NULL_STR_VALUE "NULL"

static char *allocateString(uint32_t capacity);
static void doubleStringCapacity(QueryString *str, uint32_t capacity);
static QueryString *newQueryStringFromBuffer(char *buffer, uint32_t size, uint32_t capacity);
static char *copyStringValue(QueryString *str, uint32_t size);
static uint32_t substringParamName(char *buffer, const char *origString);
char *intToString(int64_t value, char* result, int base);


QueryString *newQueryString() {
    return newQueryStringWithSize(DEFAULT_SQLITE_QUERY_STRING_SIZE);
}

QueryString *newQueryStringWithSize(uint32_t capacity) {
    if (capacity < 1) return NULL;
    QueryString *str = malloc(sizeof(struct QueryString));
    if (str == NULL) return NULL;

    str->value = allocateString(capacity);
    if (str->value == NULL) {
        free(str);
        return NULL;
    }

    str->capacity = capacity;
    str->size = 0;
    return str;
}

QueryString *queryStringAppend(QueryString *str, const char* value, uint32_t valueLength) {
    if (valueLength == 0) return str;
    uint32_t newLength = valueLength + str->size;
    if (newLength > str->capacity) {
        doubleStringCapacity(str, newLength);
    }

    memcpy(str->value + str->size, value, valueLength);
    str->size += valueLength;
    return str;
}

QueryString *queryStringAppendChar(QueryString *str, char charValue) {
    uint32_t newLength = str->size + 1;
    if (newLength > str->capacity) {
        doubleStringCapacity(str, newLength);
    }
    str->value[str->size] = charValue;
    str->size++;
    return str;
}

QueryString *queryStringOf(const char* format, ...) {
    int capacity = SQLITE_QUERY_FORMAT_STRING_SIZE;
    char *buffer = calloc(1, capacity);
    if (buffer == NULL) return NULL;

    va_list args;
    va_start(args, format);
    int formattedStrSize = vsnprintf(buffer, capacity, format, args);
    va_end(args);

    if (capacity > formattedStrSize) {
        QueryString *str = newQueryStringFromBuffer(buffer, formattedStrSize, capacity);
        if (str == NULL) {
            free(buffer);
            return NULL;
        }
        return str;
    }

    // formatted string is larger than expected
    free(buffer);
    buffer = calloc(1, formattedStrSize + 1);
    if (buffer == NULL) {
        return NULL;
    }

    va_start(args, format);
    formattedStrSize = vsnprintf(buffer, formattedStrSize + 1, format, args);
    va_end(args);

    QueryString *str = newQueryStringFromBuffer(buffer, formattedStrSize, formattedStrSize);
    if (buffer == NULL) {
        free(buffer);
        return NULL;
    }
    return str;
}

QueryString *namedQueryString(const char *sql, str_DbValueMap *queryParams) {
    uint32_t sqlLength = sql != NULL ? strlen(sql) : 0;
    if (sql == 0) return NULL;
    char buffer[DB_NAMED_PARAM_MAX_LENGTH];

    QueryString *query = newQueryStringWithSize((uint32_t) (sqlLength * 1.5));
    const char *sqlStr = sql;
    while (*sqlStr != '\0') {
        if (*sqlStr == ':') {
            sqlStr++;    // skip ':'
            uint32_t paramLength = substringParamName(buffer, sqlStr);
            DbValue dbValue = str_DbValueMapGetOrDefault(queryParams, buffer, DB_NULL_VALUE());

            switch (dbValue.type) {
                case DB_VALUE_NULL:
                    queryStringAppend(query, DB_NULL_STR_VALUE, sizeof(DB_NULL_STR_VALUE) - 1);
                    break;
                case DB_VALUE_TEXT:
                    queryStringAppendChar(query, '\'');
                    queryStringAppend(query, DB_VALUE_AS_STR(dbValue), strlen(DB_VALUE_AS_STR(dbValue)));
                    queryStringAppendChar(query, '\'');
                    break;
                case DB_VALUE_INT: {
                    char *intAsStr = intToString(DB_VALUE_AS_INT(dbValue), buffer, 10);
                    queryStringAppend(query, intAsStr, strlen(intAsStr));
                }
                    break;
                case DB_VALUE_REAL: {
                    uint32_t length = snprintf(buffer, DB_NAMED_PARAM_MAX_LENGTH, "%f", DB_VALUE_AS_DOUBLE(dbValue));
                    queryStringAppend(query, buffer, length);
                }
                    break;
                case DB_VALUE_BLOB:
                    break;
            }

            sqlStr += paramLength;
        }

        queryStringAppendChar(query, *sqlStr);
        sqlStr++;
    }

    return query;
}

const char *queryStringGetValue(QueryString *str) {
    return str->value;
}

void deleteQueryString(QueryString *str) {
    if (str != NULL) {
        free(str->value);
        str->value = NULL;
        free(str);
    }
}

static char *allocateString(uint32_t capacity) {
    char *data = malloc(capacity + 1);
    if (data == NULL) return NULL;
    memset(data, 0, capacity + 1);
    return data;
}

static void doubleStringCapacity(QueryString *str, uint32_t capacity) {
    uint32_t newCapacity = (str->capacity * 2) + 1;
    if (newCapacity < capacity) {
        newCapacity = capacity;
    }

    str->capacity = newCapacity;
    str->value = copyStringValue(str, newCapacity);
}

static QueryString *newQueryStringFromBuffer(char *buffer, uint32_t size, uint32_t capacity) {
    QueryString *str = malloc(sizeof(struct QueryString));
    if (str == NULL) return NULL;
    str->value = buffer;
    str->size = size;
    str->capacity = capacity;
    return str;
}

static char *copyStringValue(QueryString *str, uint32_t size) {
    char *data = allocateString(size);
    memcpy(data, str->value, str->size);
    free(str->value);
    return data;
}

static uint32_t substringParamName(char *buffer, const char *origString) {
    uint32_t i = 0;
    while (i < DB_NAMED_PARAM_MAX_LENGTH) {
        if (!isalnum((int) origString[i]) &&
            origString[i] != '_' &&
            origString[i] != '-') {
            break;
        }
        buffer[i] = origString[i];
        i++;
    }

    buffer[i] = '\0';
    return i;
}

char *intToString(int64_t value, char* result, int base) {
    if (base < 2 || base > 36) {    // check that the base if valid
        *result = '\0';
        return result;
    }

    char* ptr = result, *ptr1 = result;
    int64_t tmpValue;
    do {
        tmpValue = value;
        value /= base;
        *ptr++ = "zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnopqrstuvwxyz" [35 + (tmpValue - value * base)];
    } while ( value );

    // Apply negative sign
    if (tmpValue < 0) {
        *ptr++ = '-';
    }
    *ptr-- = '\0';

    while(ptr1 < ptr) {
        char tmpChar = *ptr;
        *ptr--= *ptr1;
        *ptr1++ = tmpChar;
    }
    return result;
}
