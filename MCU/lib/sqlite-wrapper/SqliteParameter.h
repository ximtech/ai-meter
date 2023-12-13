#pragma once

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <inttypes.h>

#include "sqlite3.h"
#include "HashMap.h"
#include "Vector.h"
#include "BufferHashMap.h"


#define DB_INT_VALUE(value) ((DbValue) {.type = DB_VALUE_INT, .as.intValue = (value)})
#define DB_DOUBLE_VALUE(value) ((DbValue) {.type = DB_VALUE_REAL, .as.doubleValue = (value)})
#define DB_NULL_VALUE(value) ((DbValue) {.type = DB_VALUE_NULL})
#define DB_STR_VALUE(value) ((DbValue) {.type = DB_VALUE_TEXT, .as.strValue = (value)})

#define DB_VALUE_AS_INT(value) ((value).as.intValue)
#define DB_VALUE_AS_DOUBLE(value) ((value).as.doubleValue)
#define DB_VALUE_AS_STR(value) ((value).as.strValue)

typedef enum DbValueType {
    DB_VALUE_NULL = 1,    // Value is NULL (or a pointer)
    DB_VALUE_TEXT,        // Value is a string
    DB_VALUE_INT,         // Value is an integer
    DB_VALUE_REAL,        // Value is a float number
    DB_VALUE_BLOB         // Value is a blob
} DbValueType;

typedef struct DbValue {
    DbValueType type;
    union {
        int64_t intValue;
        double doubleValue;
        void *blobValue;
        char *strValue;
    } as;
} DbValue;


static inline DbValue intDbValue(int64_t value) {
    return DB_INT_VALUE(value);
}

static inline DbValue strDbValue(char *value) {
    return DB_STR_VALUE(value);
}

static inline DbValue nullDbValue(void *value) {
    return DB_NULL_VALUE();
}

static inline DbValue doubleDbValue(double value) {
    return DB_DOUBLE_VALUE(value);
}

#define DB_VALUE(X)              \
    _Generic((X),                \
        int: intDbValue,         \
        int64_t: intDbValue,     \
        default: nullDbValue,    \
        char*: strDbValue,       \
        float: doubleDbValue,    \
        double: doubleDbValue    \
    )(X)


typedef char* str;
CREATE_HASH_MAP_TYPE(str, DbValue); // creates: 'str_DbValueMap'


#define CREATE_SQL_PARAM_MAP_1(KEY_NAME, VALUE_NAME, K1) \
        NEW_HASH_MAP_OF(1, KEY_NAME, VALUE_NAME,     \
        MAP_ENTRY(K1, DB_VALUE(K1)))

#define CREATE_SQL_PARAM_MAP_2(KEY_NAME, VALUE_NAME, K1, V1) \
        NEW_HASH_MAP_OF(1, KEY_NAME, VALUE_NAME,         \
        MAP_ENTRY(K1, DB_VALUE(V1)))

#define CREATE_SQL_PARAM_MAP_3(KEY_NAME, VALUE_NAME, K1, V1, K2) \
        NEW_HASH_MAP_OF(2, KEY_NAME, VALUE_NAME,             \
        MAP_ENTRY(K1, DB_VALUE(V1)), MAP_ENTRY(K2, DB_VALUE(K2)))

#define CREATE_SQL_PARAM_MAP_4(KEY_NAME, VALUE_NAME, K1, V1, K2, V2) \
        NEW_HASH_MAP_OF(2, KEY_NAME, VALUE_NAME,                 \
        MAP_ENTRY(K1, DB_VALUE(V1)), MAP_ENTRY(K2, DB_VALUE(V2)))

#define CREATE_SQL_PARAM_MAP_5(KEY_NAME, VALUE_NAME, K1, V1, K2, V2, K3) \
        NEW_HASH_MAP_OF(3, KEY_NAME, VALUE_NAME,                     \
        MAP_ENTRY(K1, DB_VALUE(V1)), MAP_ENTRY(K2, DB_VALUE(V2)), MAP_ENTRY(K3, DB_VALUE(K3)))

#define CREATE_SQL_PARAM_MAP_6(KEY_NAME, VALUE_NAME, K1, V1, K2, V2, K3, V3) \
        NEW_HASH_MAP_OF(3, KEY_NAME, VALUE_NAME,                         \
        MAP_ENTRY(K1, DB_VALUE(V1)), MAP_ENTRY(K2, DB_VALUE(V2)), MAP_ENTRY(K3, DB_VALUE(V3)))

#define CREATE_SQL_PARAM_MAP_7(KEY_NAME, VALUE_NAME, K1, V1, K2, V2, K3, V3, K4) \
        NEW_HASH_MAP_OF(4, KEY_NAME, VALUE_NAME,                             \
        MAP_ENTRY(K1, DB_VALUE(V1)), MAP_ENTRY(K2, DB_VALUE(V2)), MAP_ENTRY(K3, DB_VALUE(V3)), MAP_ENTRY(K4, DB_VALUE(K4)))

#define CREATE_SQL_PARAM_MAP_8(KEY_NAME, VALUE_NAME, K1, V1, K2, V2, K3, V3, K4, V4) \
        NEW_HASH_MAP_OF(4, KEY_NAME, VALUE_NAME,                                 \
        MAP_ENTRY(K1, DB_VALUE(V1)), MAP_ENTRY(K2, DB_VALUE(V2)), MAP_ENTRY(K3, DB_VALUE(V3)), MAP_ENTRY(K4, DB_VALUE(V4)))

#define CREATE_SQL_PARAM_MAP_9(KEY_NAME, VALUE_NAME, K1, V1, K2, V2, K3, V3, K4, V4, K5) \
        NEW_HASH_MAP_OF(5, KEY_NAME, VALUE_NAME,                                     \
        MAP_ENTRY(K1, DB_VALUE(V1)), MAP_ENTRY(K2, DB_VALUE(V2)), MAP_ENTRY(K3, DB_VALUE(V3)), MAP_ENTRY(K4, DB_VALUE(V4)), MAP_ENTRY(K5, DB_VALUE(K5)))

#define CREATE_SQL_PARAM_MAP_10(KEY_NAME, VALUE_NAME, K1, V1, K2, V2, K3, V3, K4, V4, K5, V5) \
        NEW_HASH_MAP_OF(5, KEY_NAME, VALUE_NAME,                                          \
        MAP_ENTRY(K1, DB_VALUE(V1)), MAP_ENTRY(K2, DB_VALUE(V2)), MAP_ENTRY(K3, DB_VALUE(V3)), MAP_ENTRY(K4, DB_VALUE(V4)), MAP_ENTRY(K5, DB_VALUE(V5)))

#define CREATE_SQL_PARAM_MAP_11(KEY_NAME, VALUE_NAME, K1, V1, K2, V2, K3, V3, K4, V4, K5, V5, K6) \
        NEW_HASH_MAP_OF(6, KEY_NAME, VALUE_NAME,                                              \
        MAP_ENTRY(K1, DB_VALUE(V1)), MAP_ENTRY(K2, DB_VALUE(V2)), MAP_ENTRY(K3, DB_VALUE(V3)), MAP_ENTRY(K4, DB_VALUE(V4)), MAP_ENTRY(K5, DB_VALUE(V5)), MAP_ENTRY(K6, DB_VALUE(K6)))

#define CREATE_SQL_PARAM_MAP_12(KEY_NAME, VALUE_NAME, K1, V1, K2, V2, K3, V3, K4, V4, K5, V5, K6, V6) \
        NEW_HASH_MAP_OF(6, KEY_NAME, VALUE_NAME,                                                                      \
        MAP_ENTRY(K1, DB_VALUE(V1)), MAP_ENTRY(K2, DB_VALUE(V2)), MAP_ENTRY(K3, DB_VALUE(V3)), MAP_ENTRY(K4, DB_VALUE(V4)), MAP_ENTRY(K5, DB_VALUE(V5)), MAP_ENTRY(K6, DB_VALUE(V6)))

#define CREATE_SQL_PARAM_MAP_13(KEY_NAME, VALUE_NAME, K1, V1, K2, V2, K3, V3, K4, V4, K5, V5, K6, V6, K7) \
        NEW_HASH_MAP_OF(7, KEY_NAME, VALUE_NAME,                                                            \
        MAP_ENTRY(K1, DB_VALUE(V1)), MAP_ENTRY(K2, DB_VALUE(V2)), MAP_ENTRY(K3, DB_VALUE(V3)), MAP_ENTRY(K4, DB_VALUE(V4)), MAP_ENTRY(K5, DB_VALUE(V5)), MAP_ENTRY(K6, DB_VALUE(V6)), MAP_ENTRY(K7, DB_VALUE(K7)))

#define CREATE_SQL_PARAM_MAP_14(KEY_NAME, VALUE_NAME, K1, V1, K2, V2, K3, V3, K4, V4, K5, V5, K6, V6, K7, V7) \
        NEW_HASH_MAP_OF(7, KEY_NAME, VALUE_NAME,                                                                                    \
        MAP_ENTRY(K1, DB_VALUE(V1)), MAP_ENTRY(K2, DB_VALUE(V2)), MAP_ENTRY(K3, DB_VALUE(V3)), MAP_ENTRY(K4, DB_VALUE(V4)), MAP_ENTRY(K5, DB_VALUE(V5)), MAP_ENTRY(K6, DB_VALUE(V6)), MAP_ENTRY(K7, DB_VALUE(V7)))

#define CREATE_SQL_PARAM_MAP_15(KEY_NAME, VALUE_NAME, K1, V1, K2, V2, K3, V3, K4, V4, K5, V5, K6, V6, K7, V7, K8) \
        NEW_HASH_MAP_OF(8, KEY_NAME, VALUE_NAME,                                                                                        \
        MAP_ENTRY(K1, DB_VALUE(V1)), MAP_ENTRY(K2, DB_VALUE(V2)), MAP_ENTRY(K3, DB_VALUE(V3)), MAP_ENTRY(K4, DB_VALUE(V4)), MAP_ENTRY(K5, DB_VALUE(V5)), MAP_ENTRY(K6, DB_VALUE(V6)), MAP_ENTRY(K7, DB_VALUE(V7)), MAP_ENTRY(K8, DB_VALUE(K8)))

#define CREATE_SQL_PARAM_MAP_16(KEY_NAME, VALUE_NAME, K1, V1, K2, V2, K3, V3, K4, V4, K5, V5, K6, V6, K7, V7, K8, V8) \
        NEW_HASH_MAP_OF(8, KEY_NAME, VALUE_NAME,                                                                      \
        MAP_ENTRY(K1, DB_VALUE(V1)), MAP_ENTRY(K2, DB_VALUE(V2)), MAP_ENTRY(K3, DB_VALUE(V3)), MAP_ENTRY(K4, DB_VALUE(V4)), MAP_ENTRY(K5, DB_VALUE(V5)), MAP_ENTRY(K6, DB_VALUE(V6)), MAP_ENTRY(K7, DB_VALUE(V7)), MAP_ENTRY(K8, DB_VALUE(V8)))

#define CREATE_SQL_PARAM_MAP_17(KEY_NAME, VALUE_NAME, K1, V1, K2, V2, K3, V3, K4, V4, K5, V5, K6, V6, K7, V7, K8, V8, K9) \
        NEW_HASH_MAP_OF(9, KEY_NAME, VALUE_NAME,                                                                          \
        MAP_ENTRY(K1, DB_VALUE(V1)), MAP_ENTRY(K2, DB_VALUE(V2)), MAP_ENTRY(K3, DB_VALUE(V3)), MAP_ENTRY(K4, DB_VALUE(V4)), MAP_ENTRY(K5, DB_VALUE(V5)), MAP_ENTRY(K6, DB_VALUE(V6)), MAP_ENTRY(K7, DB_VALUE(V7)), MAP_ENTRY(K8, DB_VALUE(V8)), MAP_ENTRY(K9, DB_VALUE(K9)))

#define CREATE_SQL_PARAM_MAP_18(KEY_NAME, VALUE_NAME, K1, V1, K2, V2, K3, V3, K4, V4, K5, V5, K6, V6, K7, V7, K8, V8, K9, V9) \
        NEW_HASH_MAP_OF(9, KEY_NAME, VALUE_NAME,                                                                              \
        MAP_ENTRY(K1, DB_VALUE(V1)), MAP_ENTRY(K2, DB_VALUE(V2)), MAP_ENTRY(K3, DB_VALUE(V3)), MAP_ENTRY(K4, DB_VALUE(V4)), MAP_ENTRY(K5, DB_VALUE(V5)), MAP_ENTRY(K6, DB_VALUE(V6)), MAP_ENTRY(K7, DB_VALUE(V7)), MAP_ENTRY(K8, DB_VALUE(V8)), MAP_ENTRY(K9, DB_VALUE(V9)))

#define CREATE_SQL_PARAM_MAP_19(KEY_NAME, VALUE_NAME, K1, V1, K2, V2, K3, V3, K4, V4, K5, V5, K6, V6, K7, V7, K8, V8, K9, V9, K10) \
        NEW_HASH_MAP_OF(10, KEY_NAME, VALUE_NAME,                                                                                  \
        MAP_ENTRY(K1, DB_VALUE(V1)), MAP_ENTRY(K2, DB_VALUE(V2)), MAP_ENTRY(K3, DB_VALUE(V3)), MAP_ENTRY(K4, DB_VALUE(V4)), MAP_ENTRY(K5, DB_VALUE(V5)), MAP_ENTRY(K6, DB_VALUE(V6)), MAP_ENTRY(K7, DB_VALUE(V7)), MAP_ENTRY(K8, DB_VALUE(V8)), MAP_ENTRY(K9, DB_VALUE(V9)), MAP_ENTRY(K10, DB_VALUE(K10)))

#define CREATE_SQL_PARAM_MAP_20(KEY_NAME, VALUE_NAME, K1, V1, K2, V2, K3, V3, K4, V4, K5, V5, K6, V6, K7, V7, K8, V8, K9, V9, K10, V10) \
        NEW_HASH_MAP_OF(10, KEY_NAME, VALUE_NAME,                                                                                       \
        MAP_ENTRY(K1, DB_VALUE(V1)), MAP_ENTRY(K2, DB_VALUE(V2)), MAP_ENTRY(K3, DB_VALUE(V3)), MAP_ENTRY(K4, DB_VALUE(V4)), MAP_ENTRY(K5, DB_VALUE(V5)), MAP_ENTRY(K6, DB_VALUE(V6)), MAP_ENTRY(K7, DB_VALUE(V7)), MAP_ENTRY(K8, DB_VALUE(V8)), MAP_ENTRY(K9, DB_VALUE(V9)), MAP_ENTRY(K10, DB_VALUE(V10)))

#define CREATE_SQL_PARAM_MAP_21(KEY_NAME, VALUE_NAME, K1, V1, K2, V2, K3, V3, K4, V4, K5, V5, K6, V6, K7, V7, K8, V8, K9, V9, K10, V10, K11) \
        NEW_HASH_MAP_OF(11, KEY_NAME, VALUE_NAME,                                                                                            \
        MAP_ENTRY(K1, DB_VALUE(V1)), MAP_ENTRY(K2, DB_VALUE(V2)), MAP_ENTRY(K3, DB_VALUE(V3)), MAP_ENTRY(K4, DB_VALUE(V4)), MAP_ENTRY(K5, DB_VALUE(V5)), MAP_ENTRY(K6, DB_VALUE(V6)), MAP_ENTRY(K7, DB_VALUE(V7)), MAP_ENTRY(K8, DB_VALUE(V8)), MAP_ENTRY(K9, DB_VALUE(V9)), MAP_ENTRY(K10, DB_VALUE(V10)), MAP_ENTRY(K11, DB_VALUE(K11)))

#define CREATE_SQL_PARAM_MAP_22(KEY_NAME, VALUE_NAME, K1, V1, K2, V2, K3, V3, K4, V4, K5, V5, K6, V6, K7, V7, K8, V8, K9, V9, K10, V10, K11, V11) \
        NEW_HASH_MAP_OF(11, KEY_NAME, VALUE_NAME,                                                                                                 \
        MAP_ENTRY(K1, DB_VALUE(V1)), MAP_ENTRY(K2, DB_VALUE(V2)), MAP_ENTRY(K3, DB_VALUE(V3)), MAP_ENTRY(K4, DB_VALUE(V4)), MAP_ENTRY(K5, DB_VALUE(V5)), MAP_ENTRY(K6, DB_VALUE(V6)), MAP_ENTRY(K7, DB_VALUE(V7)), MAP_ENTRY(K8, DB_VALUE(V8)), MAP_ENTRY(K9, DB_VALUE(V9)), MAP_ENTRY(K10, DB_VALUE(V10)), MAP_ENTRY(K11, DB_VALUE(V11)))

#define CREATE_SQL_PARAM_MAP_23(KEY_NAME, VALUE_NAME, K1, V1, K2, V2, K3, V3, K4, V4, K5, V5, K6, V6, K7, V7, K8, V8, K9, V9, K10, V10, K11, V11, K12) \
        NEW_HASH_MAP_OF(12, KEY_NAME, VALUE_NAME,                                                                                                      \
        MAP_ENTRY(K1, DB_VALUE(V1)), MAP_ENTRY(K2, DB_VALUE(V2)), MAP_ENTRY(K3, DB_VALUE(V3)), MAP_ENTRY(K4, DB_VALUE(V4)), MAP_ENTRY(K5, DB_VALUE(V5)), MAP_ENTRY(K6, DB_VALUE(V6)), MAP_ENTRY(K7, DB_VALUE(V7)), MAP_ENTRY(K8, DB_VALUE(V8)), MAP_ENTRY(K9, DB_VALUE(V9)), MAP_ENTRY(K10, DB_VALUE(V10)), MAP_ENTRY(K11, DB_VALUE(V11)), MAP_ENTRY(K12, DB_VALUE(K12)))

#define CREATE_SQL_PARAM_MAP_24(KEY_NAME, VALUE_NAME, K1, V1, K2, V2, K3, V3, K4, V4, K5, V5, K6, V6, K7, V7, K8, V8, K9, V9, K10, V10, K11, V11, K12, V12) \
        NEW_HASH_MAP_OF(12, KEY_NAME, VALUE_NAME,                                                                                                           \
        MAP_ENTRY(K1, DB_VALUE(V1)), MAP_ENTRY(K2, DB_VALUE(V2)), MAP_ENTRY(K3, DB_VALUE(V3)), MAP_ENTRY(K4, DB_VALUE(V4)), MAP_ENTRY(K5, DB_VALUE(V5)), MAP_ENTRY(K6, DB_VALUE(V6)), MAP_ENTRY(K7, DB_VALUE(V7)), MAP_ENTRY(K8, DB_VALUE(V8)), MAP_ENTRY(K9, DB_VALUE(V9)), MAP_ENTRY(K10, DB_VALUE(V10)), MAP_ENTRY(K11, DB_VALUE(V11)), MAP_ENTRY(K12, DB_VALUE(V12)))

#define CREATE_SQL_PARAM_MAP_MACRO(KEY_NAME, VALUE_NAME, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, FUN, ...) FUN


#define SQL_PARAM_MAP(...)                          \
    CREATE_SQL_PARAM_MAP_MACRO(str, DbValue, __VA_ARGS__,        \
                        CREATE_SQL_PARAM_MAP_24,                   \
                        CREATE_SQL_PARAM_MAP_23,                   \
                        CREATE_SQL_PARAM_MAP_22,                   \
                        CREATE_SQL_PARAM_MAP_21,                   \
                        CREATE_SQL_PARAM_MAP_20,                   \
                        CREATE_SQL_PARAM_MAP_19,                   \
                        CREATE_SQL_PARAM_MAP_18,                   \
                        CREATE_SQL_PARAM_MAP_17,                   \
                        CREATE_SQL_PARAM_MAP_16,                   \
                        CREATE_SQL_PARAM_MAP_15,                   \
                        CREATE_SQL_PARAM_MAP_14,                   \
                        CREATE_SQL_PARAM_MAP_13,                   \
                        CREATE_SQL_PARAM_MAP_12,                   \
                        CREATE_SQL_PARAM_MAP_11,                   \
                        CREATE_SQL_PARAM_MAP_10,                   \
                        CREATE_SQL_PARAM_MAP_9,                   \
                        CREATE_SQL_PARAM_MAP_8,                   \
                        CREATE_SQL_PARAM_MAP_7,                   \
                        CREATE_SQL_PARAM_MAP_6,                   \
                        CREATE_SQL_PARAM_MAP_5,                   \
                        CREATE_SQL_PARAM_MAP_4,                   \
                        CREATE_SQL_PARAM_MAP_3,                   \
                        CREATE_SQL_PARAM_MAP_2,                   \
                        CREATE_SQL_PARAM_MAP_1,                   \
                        ERROR)(str, DbValue, __VA_ARGS__)

#define NEW_SQL_PARAM_MAP(CAPACITY)  NEW_HASH_MAP_1(str, DbValue, CAPACITY)