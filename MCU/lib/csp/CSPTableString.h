#pragma once

#include <string.h>
#include <stdint.h>
#include <stdlib.h>

#ifndef CSP_STRING_MALLOC
#define CSP_STRING_MALLOC malloc
#endif

#ifndef CSP_STRING_REALLOC
#define CSP_STRING_REALLOC realloc
#endif

#ifndef CSP_STRING_FREE
#define CSP_STRING_FREE free
#endif

#ifndef TABLE_STR_CAPACITY_MULTIPLIER
#define TABLE_STR_CAPACITY_MULTIPLIER 1.5
#endif

typedef struct CspTableString {
    char *value;
    uint32_t length;
    uint32_t capacity;
} CspTableString;


CspTableString *newTableString(uint32_t initCapacity);

void tableStringAdd(CspTableString *str, const char *value, uint32_t length);
void tableStringAddChar(CspTableString *str, char charToAdd);

void deleteTableString(CspTableString *str);
