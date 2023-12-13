#pragma once

#include <stdio.h>
#include <stdlib.h>
#include "Comparator.h"

#ifndef MIN
    #define MIN(x, y) (((x)<(y))?(x):(y))
#endif

#define HEAP_VECTOR_TYPEDEF(NAME) NAME ## HeapVec
#define HEAP_VECTOR_METHOD_NAME_2(PREFIX, NAME, POSTFIX) PREFIX ## NAME ## HeapVec ## POSTFIX
#define HEAP_VECTOR_METHOD_NAME_1(NAME, POSTFIX) NAME ## HeapVec ## POSTFIX

#define HEAP_VECTOR_METHOD_MACRO(_1, _2, _3, FUN, ...) FUN
#define HEAP_VECTOR_METHOD(...)                                     \
    HEAP_VECTOR_METHOD_MACRO(__VA_ARGS__,                           \
                        HEAP_VECTOR_METHOD_NAME_2,                  \
                        HEAP_VECTOR_METHOD_NAME_1,                  \
                        ERROR)(__VA_ARGS__)                    \


#define CREATE_HEAP_VECTOR_TYPE_NAME(TYPE, NAME, COMPARE_FUN) \
typedef struct HEAP_VECTOR_TYPEDEF(NAME) {  \
    TYPE *items;                       \
    uint32_t size;                     \
    uint32_t capacity;                 \
    uint32_t initialCapacity;          \
} HEAP_VECTOR_TYPEDEF(NAME);           \
\
static inline int NAME ##_compare(const void *a, const void *b) {      \
    TYPE valueA = *((TYPE *) a);                        \
    TYPE valueB = *((TYPE *) b);                        \
    return COMPARE_FUN(valueA, valueB);                 \
}                                                       \
\
static bool HEAP_VECTOR_METHOD(double, NAME, Capacity)(HEAP_VECTOR_TYPEDEF(NAME) *vector) {        \
    uint32_t newCapacity = vector->capacity * 2;                    \
    if (newCapacity < vector->capacity) return false;               \
    \
    TYPE *newItemArray = malloc(sizeof(TYPE) * newCapacity);  \
    if (newItemArray == NULL) return false;                         \
    \
    for (uint32_t i = 0; i < vector->size; i++) {                   \
        newItemArray[i] = vector->items[i];                         \
    }                                                               \
    free(vector->items);                \
    vector->items = newItemArray;       \
    vector->capacity = newCapacity;     \
    return true;                        \
}   \
\
static bool HEAP_VECTOR_METHOD(half, NAME, Capacity)(HEAP_VECTOR_TYPEDEF(NAME) *vector) {  \
    if (vector->capacity <= vector->initialCapacity) return false;  \
    uint32_t newCapacity = vector->capacity / 2;                    \
    TYPE *newItemArray = malloc(sizeof(TYPE) * newCapacity);  \
    if (newItemArray == NULL) return false;                         \
    \
    for (uint32_t i = 0; i < MIN(vector->size, newCapacity); i++) { \
        newItemArray[i] = vector->items[i];                         \
    }                                                               \
    free(vector->items);                            \
    vector->items = newItemArray;                   \
    vector->capacity = newCapacity;                 \
    vector->size = MIN(vector->size, newCapacity);  \
    return true;                                    \
}                                                   \
                                                    \
static inline HEAP_VECTOR_TYPEDEF(NAME) * new ## NAME ## HeapVec(uint32_t capacity) { \
    if (capacity < 1) return NULL;                              \
    \
    HEAP_VECTOR_TYPEDEF(NAME) *vector = malloc(sizeof(struct HEAP_VECTOR_TYPEDEF(NAME)));   \
    if (vector == NULL) return NULL;                            \
    vector->size = 0;                                           \
    vector->capacity = capacity;                                \
    vector->initialCapacity = capacity;                         \
    vector->items = calloc(vector->capacity, sizeof(TYPE));  \
    \
    if (vector->items == NULL) {            \
        free(vector->items);                \
        free(vector);                       \
        return NULL;                        \
    }                                       \
    return vector;                          \
}                                           \
\
static inline HEAP_VECTOR_TYPEDEF(NAME) * new ## NAME ## HeapVecOf(const TYPE *buffer, uint32_t length) { \
    HEAP_VECTOR_TYPEDEF(NAME) *vector = new ## NAME ## HeapVec(length);       \
    if (vector == NULL) {                               \
        return vector;                                  \
    }                                                   \
    for (uint32_t i = 0; i < length; i++) {             \
        vector->items[i] = buffer[i];                   \
        vector->size++;                                 \
    }                                                   \
    return vector;                                      \
}                                                       \
\
static inline bool HEAP_VECTOR_METHOD(NAME, Add)(HEAP_VECTOR_TYPEDEF(NAME) *vector, TYPE item) { \
    if (vector != NULL) {                                       \
        if (vector->size >= vector->capacity) {                 \
            if (!HEAP_VECTOR_METHOD(double, NAME, Capacity)(vector)) {  \
                return false;                                           \
            }                                                           \
        }                                                               \
        vector->items[vector->size++] = item;                   \
        return true;                                            \
    }                                                           \
    return false;                                               \
}                                                               \
\
static inline TYPE HEAP_VECTOR_METHOD(NAME, Get)(HEAP_VECTOR_TYPEDEF(NAME) *vector, uint32_t index) {  \
    return (vector != NULL && index < vector->size) ? vector->items[index] : (TYPE) {0};    \
}                                                        \
\
static inline TYPE HEAP_VECTOR_METHOD(NAME, GetOrDefault)(HEAP_VECTOR_TYPEDEF(NAME) *vector, uint32_t index, TYPE defaultValue) {  \
    return (vector != NULL && index < vector->size) ? vector->items[index] : defaultValue;    \
}                                                        \
\
static inline bool HEAP_VECTOR_METHOD(NAME, Put)(HEAP_VECTOR_TYPEDEF(NAME) *vector, uint32_t index, TYPE item) {   \
    if (vector != NULL && index < vector->size) {   \
        vector->items[index] = item;                \
        return true;                                \
    }                                               \
    return false;                                   \
}                                      \
\
static inline bool HEAP_VECTOR_METHOD(NAME, AddAt)(HEAP_VECTOR_TYPEDEF(NAME) *vector, uint32_t index, TYPE item) { \
    if (vector != NULL && index < vector->capacity) {               \
        if (vector->size >= vector->capacity) {                     \
            if (!HEAP_VECTOR_METHOD(double, NAME, Capacity)(vector)) { \
                return false;                                       \
            }                                                       \
        }                                                           \
        for (uint32_t i = vector->size; i > index; i--) {           \
            vector->items[i] = vector->items[i - 1];                \
        }                                                           \
        vector->items[index] = item;                                \
        vector->size++;                                             \
        return true;                                                \
    }                                                               \
    return false;                                                   \
}                                      \
\
static inline TYPE HEAP_VECTOR_METHOD(NAME, RemoveAt)(HEAP_VECTOR_TYPEDEF(NAME) *vector, uint32_t index) {    \
    if (vector != NULL && index < vector->size) {                           \
        TYPE item = vector->items[index];                                   \
        for (uint32_t i = index + 1; i < vector->size; i++) {               \
            vector->items[i - 1] = vector->items[i];                        \
        }                                                                   \
        vector->size--;                                                     \
                                                                            \
        if ((vector->size * 4) < vector->capacity) {                        \
            if (!HEAP_VECTOR_METHOD(half, NAME, Capacity)(vector)) {        \
                return (TYPE) {0};                                          \
        }                                                                   \
        return item;                                                        \
        }                                                         \
    }\
    return (TYPE) {0};                                                      \
}                                      \
\
static inline bool HEAP_VECTOR_METHOD(is, NAME, Empty)(HEAP_VECTOR_TYPEDEF(NAME) *vector) {  \
    return (vector == NULL) || (vector->size == 0);     \
}                                      \
\
static inline bool HEAP_VECTOR_METHOD(is, NAME, NotEmpty)(HEAP_VECTOR_TYPEDEF(NAME) *vector) {   \
    return !HEAP_VECTOR_METHOD(is, NAME, Empty)(vector);                             \
}                                      \
\
static inline uint32_t HEAP_VECTOR_METHOD(NAME, Size)(HEAP_VECTOR_TYPEDEF(NAME) *vector) { \
    return vector != NULL ? vector->size : 0;                       \
}                                      \
\
static inline void HEAP_VECTOR_METHOD(NAME, Clear)(HEAP_VECTOR_TYPEDEF(NAME) *vector) {   \
    if (vector != NULL) {                                    \
      vector->size = 0;                                      \
      while (vector->capacity > vector->initialCapacity) {   \
            if (!HEAP_VECTOR_METHOD(half, NAME, Capacity)(vector)) { \
                break;                                               \
            }                                                        \
        }                                                            \
    }                                                                \
}\
\
static inline bool HEAP_VECTOR_METHOD(NAME, AddAll)(HEAP_VECTOR_TYPEDEF(NAME) *vecDest, HEAP_VECTOR_TYPEDEF(NAME) *vecSource) { \
    if (vecDest == NULL || vecSource == NULL) return false; \
    for (uint32_t i = 0; i < vecSource->size; i++) {        \
        TYPE item = HEAP_VECTOR_METHOD(NAME, Get)(vecSource, i);        \
        if (!HEAP_VECTOR_METHOD(NAME, Add)(vecDest, item)) {            \
            return false;                                   \
        }                                                   \
    }                                                       \
    return true;                                            \
}                                           \
\
static inline HEAP_VECTOR_TYPEDEF(NAME) * HEAP_VECTOR_METHOD(NAME, FromArray)(HEAP_VECTOR_TYPEDEF(NAME) *vector, TYPE array[], uint32_t length) {  \
    for (uint32_t i = 0; i < length; i++) {             \
        if (!HEAP_VECTOR_METHOD(NAME, Add)(vector, array[i])) {     \
            return vector;                              \
        }                                               \
    }                                                   \
    return vector;                                      \
}                                           \
\
static inline int32_t HEAP_VECTOR_METHOD(NAME, IndexOf)(HEAP_VECTOR_TYPEDEF(NAME) *vector, TYPE value) { \
    if (vector == NULL) return -1;                          \
    for (int32_t i = 0; i < HEAP_VECTOR_METHOD(NAME, Size)(vector); i++) {  \
        TYPE vectorValue = HEAP_VECTOR_METHOD(NAME, Get)(vector, i);         \
        if (COMPARE_FUN(vectorValue, value) == 0) {              \
            return i;                                       \
        }                                                   \
    }                                                       \
    return -1;                                              \
}                                                           \
\
static inline bool HEAP_VECTOR_METHOD(NAME, Contains)(HEAP_VECTOR_TYPEDEF(NAME) *vector, TYPE value) { \
    return HEAP_VECTOR_METHOD(NAME, IndexOf)(vector, value) >= 0; \
}                                                        \
                                                         \
static inline void HEAP_VECTOR_METHOD(NAME, Reverse)(HEAP_VECTOR_TYPEDEF(NAME) *vector) {   \
    uint32_t i = 0;                                 \
    uint32_t j = vector->size - 1;                  \
    while (j > i) {                                 \
        TYPE tmp = vector->items[j];                \
        vector->items[j] = vector->items[i];        \
        vector->items[i] = tmp;                     \
        j--;                                        \
        i++;                                        \
    }                                               \
}                                                   \
\
static inline HEAP_VECTOR_TYPEDEF(NAME) * HEAP_VECTOR_METHOD(NAME, Sort)(HEAP_VECTOR_TYPEDEF(NAME) *vector) {   \
    qsort(vector->items, vector->size, sizeof(TYPE), NAME ##_compare);      \
    return vector;   \
}                                                        \
\
static inline bool HEAP_VECTOR_METHOD(is, NAME, Equals)(HEAP_VECTOR_TYPEDEF(NAME) *first, HEAP_VECTOR_TYPEDEF(NAME) *second) { \
    if (first == second) {              \
        return true;                    \
    }                                   \
    if (first == NULL || second == NULL || first->size != second->size) {   \
        return false;                   \
    }                                   \
\
    for (uint32_t i = 0, j = 0; i < first->size; i++, j++) {        \
        if (COMPARE_FUN(first->items[i], second->items[j]) != 0) {  \
        return false;                   \
        }                               \
    }                                   \
    return true;                        \
}                   \
\
static inline HEAP_VECTOR_TYPEDEF(NAME) * HEAP_VECTOR_METHOD(NAME, RemoveDup)(HEAP_VECTOR_TYPEDEF(NAME) *vector) {   \
    if (vector == NULL) return NULL;    \
    qsort(vector->items, vector->size, sizeof(TYPE), NAME ##_compare);   \
    uint32_t j = 0;                                      \
    for (uint32_t i = 0; i < vector->size; i++) {        \
        if (i == 0) {                                    \
            vector->items[j++] = vector->items[i];       \
            continue;                                    \
        }                                                \
                                                         \
        TYPE nextValue = vector->items[i];               \
        TYPE previousValue = vector->items[j - 1];       \
        if (COMPARE_FUN(nextValue, previousValue) != 0) {\
            vector->items[j++] = vector->items[i];       \
        }                                                \
    }                                                    \
    vector->size = j;                                    \
    return vector;                                       \
}                                                        \
\
static inline HEAP_VECTOR_TYPEDEF(NAME) * HEAP_VECTOR_METHOD(NAME, Union)(HEAP_VECTOR_TYPEDEF(NAME) *destVector, HEAP_VECTOR_TYPEDEF(NAME) *sourceVector) {  \
    if (destVector == NULL || sourceVector == NULL) return NULL;            \
    for (uint32_t i = 0; i < sourceVector->size; i++) {                     \
        HEAP_VECTOR_METHOD(NAME, Add)(destVector, HEAP_VECTOR_METHOD(NAME,Get)(sourceVector, i));  \
    }       \
    return HEAP_VECTOR_METHOD(NAME, RemoveDup)(destVector);                             \
}                                                        \
\
static inline HEAP_VECTOR_TYPEDEF(NAME) * HEAP_VECTOR_METHOD(NAME, Intersect)(HEAP_VECTOR_TYPEDEF(NAME) *destVector, HEAP_VECTOR_TYPEDEF(NAME) *sourceVector) {   \
    if (destVector == NULL || sourceVector == NULL) return NULL;                    \
    qsort(destVector->items, destVector->size, sizeof(TYPE), NAME ##_compare);      \
    qsort(sourceVector->items, sourceVector->size, sizeof(TYPE), NAME ##_compare);  \
    uint32_t index = 0;                                                             \
    for (uint32_t i = 0, j = 0; i < destVector->size && j < sourceVector->size;) {  \
        TYPE destValue = destVector->items[i];                          \
        TYPE sourceValue = sourceVector->items[j];                      \
                                                                        \
        if (COMPARE_FUN(destValue, sourceValue) < 0) {                  \
            i++;                                                        \
                                                                        \
        } else if (COMPARE_FUN(destValue, sourceValue) > 0) {           \
            j++;                                                        \
                                                                        \
        } else {                                                        \
            destVector->items[index++] = destValue;                     \
            i++;                                                        \
            j++;                                                        \
        }                                                               \
    }                                                                   \
    destVector->size = index;                                           \
    return HEAP_VECTOR_METHOD(NAME, RemoveDup)(destVector);                         \
}   \
\
static inline HEAP_VECTOR_TYPEDEF(NAME) * HEAP_VECTOR_METHOD(NAME, Subtract)(HEAP_VECTOR_TYPEDEF(NAME) *destVector, HEAP_VECTOR_TYPEDEF(NAME) *sourceVector) {   \
    if (destVector == NULL || sourceVector == NULL) return NULL;            \
    qsort(destVector->items, destVector->size, sizeof(TYPE), NAME ##_compare);      \
    qsort(sourceVector->items, sourceVector->size, sizeof(TYPE), NAME ##_compare);  \
    uint32_t i = 0;         \
    uint32_t j = 0;         \
                            \
    uint32_t index = 0;     \
    while (i < destVector->size && j < sourceVector->size) {    \
        TYPE destValue = destVector->items[i];                   \
        TYPE sourceValue = sourceVector->items[j];               \
                                                                \
        if (COMPARE_FUN(destValue, sourceValue) < 0) {          \
            destVector->items[index++] = destVector->items[i++];\
                                                                \
        } else if (COMPARE_FUN(destValue, sourceValue) > 0) {   \
            j++;                                         \
                            \
        } else {            \
            i++;            \
            j++;            \
        }                   \
    }                       \
                            \
    if (destVector->size > i) {                                     \
        while (i < destVector->size) {                              \
            destVector->items[index++] = destVector->items[i++];    \
        }                                                           \
    }                                                               \
    destVector->size = index;   \
    return destVector;          \
}               \
                                                         \
static inline HEAP_VECTOR_TYPEDEF(NAME) * HEAP_VECTOR_METHOD(NAME, Disjunction)(HEAP_VECTOR_TYPEDEF(NAME) *destVector, HEAP_VECTOR_TYPEDEF(NAME) *sourceVector) {    \
    if (HEAP_VECTOR_METHOD(NAME, AddAll)(destVector, sourceVector)) {                           \
        qsort(destVector->items, destVector->size, sizeof(TYPE), NAME ##_compare);   \
        uint32_t index = 0;                                                         \
        for (uint32_t i = 0, j = 1; j <= destVector->size; j++) {                   \
            if (i == destVector->size - 1) {                                        \
                destVector->items[index++] = destVector->items[i];                  \
                break;                                                              \
            }                                                                       \
                                                                                    \
            TYPE firstValue = destVector->items[i];                                 \
            TYPE nextValue = destVector->items[j];                                  \
            if (COMPARE_FUN(firstValue, nextValue) == 0) {                          \
                continue;                                                           \
            }                                                                       \
            if (i == (j - 1)) {                                                     \
                destVector->items[index++] = destVector->items[i];                  \
            }                                                                       \
            i = j;                                                                  \
        }                                                                           \
        destVector->size = index;                                                   \
        return destVector;                                                          \
    }                                                                               \
    return NULL;                                                                    \
}                                                                                   \
\
static inline void HEAP_VECTOR_METHOD(NAME, Delete)(HEAP_VECTOR_TYPEDEF(NAME) *vector) {      \
    if (vector != NULL) {               \
        free(vector->items);            \
        free(vector);                   \
    }                                   \
}                                                             \
\

#define CREATE_HEAP_VECTOR_TYPE_1(TYPE) CREATE_HEAP_VECTOR_TYPE_NAME(TYPE, TYPE, COMPARATOR_FOR_TYPE(TYPE))
#define CREATE_HEAP_VECTOR_TYPE_2(TYPE, NAME) CREATE_HEAP_VECTOR_TYPE_NAME(TYPE, NAME, COMPARATOR_FOR_TYPE(TYPE))
#define CREATE_HEAP_VECTOR_TYPE_3(TYPE, NAME, COMPARE_FUN) CREATE_HEAP_VECTOR_TYPE_NAME(TYPE, NAME, COMPARE_FUN)
#define CREATE_HEAP_VECTOR_TYPE_MACRO(_1, _2, _3, FUN, ...) FUN

#define CREATE_HEAP_VECTOR_TYPE(...)                                     \
    CREATE_HEAP_VECTOR_TYPE_MACRO(__VA_ARGS__,                           \
                        CREATE_HEAP_VECTOR_TYPE_3,                       \
                        CREATE_HEAP_VECTOR_TYPE_2,                       \
                        CREATE_HEAP_VECTOR_TYPE_1,                       \
                        ERROR)(__VA_ARGS__)


#define NEW_HEAP_VECTOR(NAME, CAPACITY) new ## NAME ## HeapVec(CAPACITY)

#define NEW_HEAP_VECTOR_4(NAME)    NEW_HEAP_VECTOR(NAME, 4)
#define NEW_HEAP_VECTOR_8(NAME)    NEW_HEAP_VECTOR(NAME, 8)
#define NEW_HEAP_VECTOR_16(NAME)   NEW_HEAP_VECTOR(NAME, 16)
#define NEW_HEAP_VECTOR_32(NAME)   NEW_HEAP_VECTOR(NAME, 32)
#define NEW_HEAP_VECTOR_64(NAME)   NEW_HEAP_VECTOR(NAME, 64)
#define NEW_HEAP_VECTOR_128(NAME)  NEW_HEAP_VECTOR(NAME, 128)
#define NEW_HEAP_VECTOR_256(NAME)  NEW_HEAP_VECTOR(NAME, 256)
#define NEW_HEAP_VECTOR_512(NAME)  NEW_HEAP_VECTOR(NAME, 512)
#define NEW_HEAP_VECTOR_1024(NAME) NEW_HEAP_VECTOR(NAME, 1024)


#define NEW_HEAP_VECTOR_OF(CAPACITY, TYPE, NAME, ...) new ## NAME ## HeapVecOf((TYPE [CAPACITY]){__VA_ARGS__}, CAPACITY)

#define CREATE_HEAP_VECTOR_1(TYPE, NAME, V1) \
        NEW_HEAP_VECTOR_OF(1, TYPE, NAME, V1)
#define CREATE_HEAP_VECTOR_2(TYPE, NAME, V1, V2) \
        NEW_HEAP_VECTOR_OF(2, TYPE, NAME, V1, V2)
#define CREATE_HEAP_VECTOR_3(TYPE, NAME, V1, V2, V3) \
        NEW_HEAP_VECTOR_OF(3, TYPE, NAME, V1, V2, V3)
#define CREATE_HEAP_VECTOR_4(TYPE, NAME, V1, V2, V3, V4) \
        NEW_HEAP_VECTOR_OF(4, TYPE, NAME, V1, V2, V3, V4)
#define CREATE_HEAP_VECTOR_5(TYPE, NAME, V1, V2, V3, V4, V5) \
        NEW_HEAP_VECTOR_OF(5, TYPE, NAME, V1, V2, V3, V4, V5)
#define CREATE_HEAP_VECTOR_6(TYPE, NAME, V1, V2, V3, V4, V5, V6) \
        NEW_HEAP_VECTOR_OF(6, TYPE, NAME, V1, V2, V3, V4, V5, V6)
#define CREATE_HEAP_VECTOR_7(TYPE, NAME, V1, V2, V3, V4, V5, V6, V7) \
        NEW_HEAP_VECTOR_OF(7, TYPE, NAME, V1, V2, V3, V4, V5, V6, V7)
#define CREATE_HEAP_VECTOR_8(TYPE, NAME, V1, V2, V3, V4, V5, V6, V7, V8) \
        NEW_HEAP_VECTOR_OF(8, TYPE, NAME, V1, V2, V3, V4, V5, V6, V7, V8)
#define CREATE_HEAP_VECTOR_9(TYPE, NAME, V1, V2, V3, V4, V5, V6, V7, V8, V9) \
        NEW_HEAP_VECTOR_OF(9, TYPE, NAME, V1, V2, V3, V4, V5, V6, V7, V8, V9)
#define CREATE_HEAP_VECTOR_10(TYPE, NAME, V1, V2, V3, V4, V5, V6, V7, V8, V9, V10) \
        NEW_HEAP_VECTOR_OF(10, TYPE, NAME, V1, V2, V3, V4, V5, V6, V7, V8, V9, V10)
#define CREATE_HEAP_VECTOR_11(TYPE, NAME, V1, V2, V3, V4, V5, V6, V7, V8, V9, V10, V11) \
        NEW_HEAP_VECTOR_OF(11, TYPE, NAME, V1, V2, V3, V4, V5, V6, V7, V8, V9, V10, V11)
#define CREATE_HEAP_VECTOR_12(TYPE, NAME, V1, V2, V3, V4, V5, V6, V7, V8, V9, V10, V11, V12) \
        NEW_HEAP_VECTOR_OF(12, TYPE, NAME, V1, V2, V3, V4, V5, V6, V7, V8, V9, V10, V11, V12)
#define CREATE_HEAP_VECTOR_13(TYPE, NAME, V1, V2, V3, V4, V5, V6, V7, V8, V9, V10, V11, V12, V13) \
        NEW_HEAP_VECTOR_OF(13, TYPE, NAME, V1, V2, V3, V4, V5, V6, V7, V8, V9, V10, V11, V12, V13)
#define CREATE_HEAP_VECTOR_14(TYPE, NAME, V1, V2, V3, V4, V5, V6, V7, V8, V9, V10, V11, V12, V13, V14) \
        NEW_HEAP_VECTOR_OF(14, TYPE, NAME, V1, V2, V3, V4, V5, V6, V7, V8, V9, V10, V11, V12, V13, V14)
#define CREATE_HEAP_VECTOR_15(TYPE, NAME, V1, V2, V3, V4, V5, V6, V7, V8, V9, V10, V11, V12, V13, V14, V15) \
        NEW_HEAP_VECTOR_OF(15, TYPE, NAME, V1, V2, V3, V4, V5, V6, V7, V8, V9, V10, V11, V12, V13, V14, V15)
#define CREATE_HEAP_VECTOR_16(TYPE, NAME, V1, V2, V3, V4, V5, V6, V7, V8, V9, V10, V11, V12, V13, V14, V15, V16) \
        NEW_HEAP_VECTOR_OF(16, TYPE, NAME, V1, V2, V3, V4, V5, V6, V7, V8, V9, V10, V11, V12, V13, V14, V15, V16)
#define CREATE_HEAP_VECTOR_17(TYPE, NAME, V1, V2, V3, V4, V5, V6, V7, V8, V9, V10, V11, V12, V13, V14, V15, V16, V17) \
        NEW_HEAP_VECTOR_OF(17, TYPE, NAME, V1, V2, V3, V4, V5, V6, V7, V8, V9, V10, V11, V12, V13, V14, V15, V16, V17)
#define CREATE_HEAP_VECTOR_18(TYPE, NAME, V1, V2, V3, V4, V5, V6, V7, V8, V9, V10, V11, V12, V13, V14, V15, V16, V17, V18) \
        NEW_HEAP_VECTOR_OF(18, TYPE, NAME, V1, V2, V3, V4, V5, V6, V7, V8, V9, V10, V11, V12, V13, V14, V15, V16, V17, V18)
#define CREATE_HEAP_VECTOR_19(TYPE, NAME, V1, V2, V3, V4, V5, V6, V7, V8, V9, V10, V11, V12, V13, V14, V15, V16, V17, V18, V19) \
        NEW_HEAP_VECTOR_OF(19, TYPE, NAME, V1, V2, V3, V4, V5, V6, V7, V8, V9, V10, V11, V12, V13, V14, V15, V16, V17, V18, V19)
#define CREATE_HEAP_VECTOR_20(TYPE, NAME, V1, V2, V3, V4, V5, V6, V7, V8, V9, V10, V11, V12, V13, V14, V15, V16, V17, V18, V19, V20) \
        NEW_HEAP_VECTOR_OF(20, TYPE, NAME, V1, V2, V3, V4, V5, V6, V7, V8, V9, V10, V11, V12, V13, V14, V15, V16, V17, V18, V19, V20)

#define GET_CREATE_HEAP_VECTOR_MACRO(TYPE, NAME, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, FUN, ...) FUN

#define HEAP_VECTOR_OF(TYPE, NAME, ...)                          \
    GET_CREATE_HEAP_VECTOR_MACRO(TYPE, NAME, __VA_ARGS__,        \
                        CREATE_HEAP_VECTOR_20,                   \
                        CREATE_HEAP_VECTOR_19,                   \
                        CREATE_HEAP_VECTOR_18,                   \
                        CREATE_HEAP_VECTOR_17,                   \
                        CREATE_HEAP_VECTOR_16,                   \
                        CREATE_HEAP_VECTOR_15,                   \
                        CREATE_HEAP_VECTOR_14,                   \
                        CREATE_HEAP_VECTOR_13,                   \
                        CREATE_HEAP_VECTOR_12,                   \
                        CREATE_HEAP_VECTOR_11,                   \
                        CREATE_HEAP_VECTOR_10,                   \
                        CREATE_HEAP_VECTOR_9,                   \
                        CREATE_HEAP_VECTOR_8,                   \
                        CREATE_HEAP_VECTOR_7,                   \
                        CREATE_HEAP_VECTOR_6,                   \
                        CREATE_HEAP_VECTOR_5,                   \
                        CREATE_HEAP_VECTOR_4,                   \
                        CREATE_HEAP_VECTOR_3,                   \
                        CREATE_HEAP_VECTOR_2,                   \
                        CREATE_HEAP_VECTOR_1,                   \
                        ERROR)(TYPE, NAME, __VA_ARGS__)

#define HEAP_VECTOR(TYPE, ...) HEAP_VECTOR_OF(TYPE, TYPE, __VA_ARGS__)