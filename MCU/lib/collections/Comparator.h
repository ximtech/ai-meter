#pragma once

#include <stdint.h>
#include <ctype.h>
#include <string.h>
#include <stdbool.h>

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
#endif

#define VAR_ARGS_LENGTH(TYPE, ...)  (sizeof((TYPE []){__VA_ARGS__}) / sizeof(TYPE))

#define NEXT_POW_OF_TWO_0(val) ((val) - 1)
#define NEXT_POW_OF_TWO_1(val) (NEXT_POW_OF_TWO_0(val) | NEXT_POW_OF_TWO_0(val) >> 1)
#define NEXT_POW_OF_TWO_2(val) (NEXT_POW_OF_TWO_1(val) | NEXT_POW_OF_TWO_1(val) >> 2)
#define NEXT_POW_OF_TWO_3(val) (NEXT_POW_OF_TWO_2(val) | NEXT_POW_OF_TWO_2(val) >> 4)
#define NEXT_POW_OF_TWO_4(val) (NEXT_POW_OF_TWO_3(val) | NEXT_POW_OF_TWO_3(val) >> 8)
#define NEXT_POW_OF_TWO_5(val) (NEXT_POW_OF_TWO_4(val) | NEXT_POW_OF_TWO_4(val) >> 16)

#define NEXT_POW_OF_2(val) (NEXT_POW_OF_TWO_5(val) + 1)

#define COMPARATOR_FOR_TYPE(TYPE) TYPE ## Comparator
#define HASH_CODE_FOR_TYPE(TYPE)  TYPE ## HashCode

#define LL_HASH_CODE(value) (int)((value) ^ ((value) >> 32))

#define CREATE_NUMBER_COMPARATOR(NAME, TYPE) \
static inline int NAME ##Comparator(TYPE one, TYPE two) { \
    return (((one) < (two)) ? (-1) : (((one) == (two)) ? 0 : 1));             \
} \

#define CREATE_NUMBER_HASH_CODE(NAME, TYPE) \
static inline uint32_t NAME ##HashCode(TYPE value) { \
    return (uint32_t) value;             \
} \

#define CREATE_CUSTOM_COMPARATOR(NAME, TYPE, PARAM_NAME_1, PARAM_NAME_2, EXPR) \
static inline int NAME ##Comparator(TYPE PARAM_NAME_1, TYPE PARAM_NAME_2) { \
    return (EXPR);             \
}

#define CREATE_CUSTOM_HASH_CODE(NAME, TYPE, PARAM_NAME, EXPR) \
static inline uint32_t NAME ##HashCode(TYPE PARAM_NAME) { \
    return (EXPR);             \
}

CREATE_NUMBER_COMPARATOR(int, int);                 // intComparator()
CREATE_NUMBER_COMPARATOR(long, long);               // longComparator()
CREATE_NUMBER_COMPARATOR(char, char);               // charComparator()
CREATE_NUMBER_COMPARATOR(int8_t, int8_t);           // int8_tComparator()
CREATE_NUMBER_COMPARATOR(uint8_t, uint8_t);         // uint8_tComparator()
CREATE_NUMBER_COMPARATOR(int16_t, int16_t);         // int16_tComparator()
CREATE_NUMBER_COMPARATOR(uint16_t, uint16_t);       // uint16_tComparator()
CREATE_NUMBER_COMPARATOR(int32_t, int32_t);         // int32_tComparator()
CREATE_NUMBER_COMPARATOR(uint32_t, uint32_t);       // uint32_tComparator()
CREATE_NUMBER_COMPARATOR(int64_t, int64_t);         // int64_tComparator()
CREATE_NUMBER_COMPARATOR(uint64_t, uint64_t);       // uint64_tComparator()

CREATE_NUMBER_HASH_CODE(int, int);                 // intHashCode()
CREATE_NUMBER_HASH_CODE(long, long);               // longHashCode()
CREATE_NUMBER_HASH_CODE(char, char);               // charHashCode()
CREATE_NUMBER_HASH_CODE(float, float);             // floatHashCode()
CREATE_NUMBER_HASH_CODE(int8_t, int8_t);           // int8_tHashCode()
CREATE_NUMBER_HASH_CODE(uint8_t, uint8_t);         // uint8_tHashCode()
CREATE_NUMBER_HASH_CODE(int16_t, int16_t);         // int16_tHashCode()
CREATE_NUMBER_HASH_CODE(uint16_t, uint16_t);       // uint16_tHashCode()
CREATE_NUMBER_HASH_CODE(int32_t, int32_t);         // int32_tHashCode()
CREATE_NUMBER_HASH_CODE(uint32_t, uint32_t);       // uint32_tHashCode()


static inline int charIgnoreCaseComparator(char one, char two) {
    return (int)((((char)tolower(one)) < ((char)tolower(two))) ? (-1) : ((((char)tolower(one)) == ((char)tolower(two))) ? 0 : 1));
}

static inline int strComparator(const char *one, const char *two) {
    return strcmp(one, two);
}

static inline uint32_t int64_tHashCode(int64_t value) {
    return LL_HASH_CODE(value);
}

static inline uint32_t uint64_tHashCode(uint64_t value) {
    return LL_HASH_CODE(value);
}

static inline uint32_t doubleHashCode(double value) {
    int64_t bits = (int64_t) value;
    return LL_HASH_CODE(bits);
}

uint32_t strHashCode(const char *key);

int doubleComparator(double one, double two);
int floatComparator(float one, float two);
int strNaturalSortComparator(const char *one, const char *two);