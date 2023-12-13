#pragma once

#include <stdio.h>
#include <stdlib.h>
#include "Comparator.h"

#define VECTOR_TYPEDEF(NAME) NAME ##Vector
#define VECTOR_METHOD_NAME_2(PREFIX, NAME, POSTFIX) PREFIX ## NAME ## Vec ## POSTFIX
#define VECTOR_METHOD_NAME_1(NAME, POSTFIX) NAME ## Vec ## POSTFIX

#define VECTOR_METHOD_MACRO(_1, _2, _3, FUN, ...) FUN
#define VECTOR_METHOD(...)                                     \
    VECTOR_METHOD_MACRO(__VA_ARGS__,                           \
                        VECTOR_METHOD_NAME_2,                  \
                        VECTOR_METHOD_NAME_1,                  \
                        ERROR)(__VA_ARGS__)                    \


#define CREATE_VECTOR_TYPE_NAME(TYPE, NAME, COMPARE_FUN) \
typedef struct VECTOR_TYPEDEF(NAME) {  \
    TYPE *items;                       \
    uint32_t size;                     \
    uint32_t capacity;                 \
} VECTOR_TYPEDEF(NAME);                \
\
static inline int NAME ##_compare(const void *a, const void *b) {      \
    TYPE valueA = *((TYPE *) a);                        \
    TYPE valueB = *((TYPE *) b);                        \
    return COMPARE_FUN(valueA, valueB);                 \
}                                     \
\
static inline VECTOR_TYPEDEF(NAME) * new ## NAME ## BuffVector(VECTOR_TYPEDEF(NAME) *vector, TYPE *buffer, uint32_t capacity) { \
    if (vector == NULL || capacity == 0) return NULL;       \
    vector->size = 0;                                       \
    vector->capacity = capacity;                            \
    vector->items = buffer;                                 \
    return vector;                                          \
}                                      \
\
static inline VECTOR_TYPEDEF(NAME) *new ## NAME ## BuffVectorOf(VECTOR_TYPEDEF(NAME) *vector, TYPE *buffer, uint32_t capacity, uint32_t size) { \
    vector = new ## NAME ## BuffVector(vector, buffer, capacity); \
    if (vector == NULL) {                                         \
        return vector;                                            \
    }                                                             \
    vector->size = size;                                          \
    return vector;                                                \
}                                      \
\
static inline bool VECTOR_METHOD(NAME, Add)(VECTOR_TYPEDEF(NAME) *vector, TYPE item) { \
    if (vector != NULL && vector->size < vector->capacity) {    \
        vector->items[vector->size++] = item;                   \
        return true;                                            \
    }                                                           \
    return false;                                               \
}                                      \
\
static inline TYPE VECTOR_METHOD(NAME, Get)(VECTOR_TYPEDEF(NAME) *vector, uint32_t index) {  \
    return (vector != NULL && index < vector->size) ? vector->items[index] : (TYPE) {0};    \
}                                                        \
\
static inline TYPE VECTOR_METHOD(NAME, GetOrDefault)(VECTOR_TYPEDEF(NAME) *vector, uint32_t index, TYPE defaultValue) {  \
    return (vector != NULL && index < vector->size) ? vector->items[index] : defaultValue;    \
}                                                        \
\
static inline bool VECTOR_METHOD(NAME, Put)(VECTOR_TYPEDEF(NAME) *vector, uint32_t index, TYPE item) {   \
    if (vector != NULL && index < vector->size) {   \
        vector->items[index] = item;                \
        return true;                                \
    }                                               \
    return false;                                   \
}                                      \
\
static inline bool VECTOR_METHOD(NAME, AddAt)(VECTOR_TYPEDEF(NAME) *vector, uint32_t index, TYPE item) { \
    if (vector != NULL && index < vector->capacity) {               \
        if ((vector->size + 1) > vector->capacity) {                \
            return false;                                           \
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
static inline TYPE VECTOR_METHOD(NAME, RemoveAt)(VECTOR_TYPEDEF(NAME) *vector, uint32_t index) {    \
    if (vector != NULL && index < vector->capacity) {                       \
        TYPE item = vector->items[index];                                   \
        for (uint32_t i = index + 1; i < vector->size; i++) {               \
            vector->items[i - 1] = vector->items[i];                        \
        }                                                                   \
        vector->size--;                                                     \
        return item;                                                        \
    }                                                                       \
    return (TYPE) {0};                                          \
}                                      \
\
static inline bool VECTOR_METHOD(is, NAME, Empty)(VECTOR_TYPEDEF(NAME) *vector) {  \
    return (vector == NULL) || (vector->size == 0);     \
}                                      \
\
static inline bool VECTOR_METHOD(is, NAME, NotEmpty)(VECTOR_TYPEDEF(NAME) *vector) {   \
    return !VECTOR_METHOD(is, NAME, Empty)(vector);                             \
}                                      \
\
static inline uint32_t VECTOR_METHOD(NAME, Size)(VECTOR_TYPEDEF(NAME) *vector) { \
    return vector != NULL ? vector->size : 0;                       \
}                                      \
\
static inline void VECTOR_METHOD(NAME, Clear)(VECTOR_TYPEDEF(NAME) *vector) {   \
    if (vector != NULL) {                               \
        for (uint32_t i = 0; i < vector->size; i++) {   \
            vector->items[i] = (TYPE) {0};              \
        }                                               \
        vector->size = 0;                               \
    }                                                   \
}                                      \
\
static inline bool VECTOR_METHOD(NAME, AddAll)(VECTOR_TYPEDEF(NAME) *vecDest, VECTOR_TYPEDEF(NAME) *vecSource) { \
    if (vecDest == NULL || vecSource == NULL) return false; \
    for (uint32_t i = 0; i < vecSource->size; i++) {        \
        TYPE item = VECTOR_METHOD(NAME, Get)(vecSource, i);        \
        if (!VECTOR_METHOD(NAME, Add)(vecDest, item)) {            \
            return false;                                   \
        }                                                   \
    }                                                       \
    return true;                                            \
}                                           \
\
static inline VECTOR_TYPEDEF(NAME) * VECTOR_METHOD(NAME, FromArray)(VECTOR_TYPEDEF(NAME) *vector, TYPE array[], uint32_t length) {  \
    for (uint32_t i = 0; i < length; i++) {             \
        if (!VECTOR_METHOD(NAME, Add)(vector, array[i])) {     \
            return vector;                              \
        }                                               \
    }                                                   \
    return vector;                                      \
}                                           \
\
static inline int32_t VECTOR_METHOD(NAME, IndexOf)(VECTOR_TYPEDEF(NAME) *vector, TYPE value) { \
    if (vector == NULL) return -1;                          \
    for (int32_t i = 0; i < VECTOR_METHOD(NAME, Size)(vector); i++) {  \
        TYPE vectorValue = VECTOR_METHOD(NAME, Get)(vector, i);         \
        if (COMPARE_FUN(vectorValue, value) == 0) {              \
            return i;                                       \
        }                                                   \
    }                                                       \
    return -1;                                              \
}                                           \
\
static inline bool VECTOR_METHOD(NAME, Contains)(VECTOR_TYPEDEF(NAME) *vector, TYPE value) { \
    return VECTOR_METHOD(NAME, IndexOf)(vector, value) >= 0; \
}                                                        \
                                                         \
static inline void VECTOR_METHOD(NAME, Reverse)(VECTOR_TYPEDEF(NAME) *vector) {   \
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
static inline VECTOR_TYPEDEF(NAME) * VECTOR_METHOD(NAME, Sort)(VECTOR_TYPEDEF(NAME) *vector) {   \
    qsort(vector->items, vector->size, sizeof(TYPE), NAME ##_compare);      \
    return vector;   \
}                                                        \
\
static inline bool VECTOR_METHOD(is, NAME, Equals)(VECTOR_TYPEDEF(NAME) *first, VECTOR_TYPEDEF(NAME) *second) { \
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
static inline VECTOR_TYPEDEF(NAME) * VECTOR_METHOD(NAME, RemoveDup)(VECTOR_TYPEDEF(NAME) *vector) {   \
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
static inline VECTOR_TYPEDEF(NAME) * VECTOR_METHOD(NAME, Union)(VECTOR_TYPEDEF(NAME) *destVector, VECTOR_TYPEDEF(NAME) *sourceVector) {  \
    if (destVector == NULL || sourceVector == NULL) return NULL;            \
    for (uint32_t i = 0; i < sourceVector->size; i++) {                     \
        VECTOR_METHOD(NAME, Add)(destVector, VECTOR_METHOD(NAME,Get)(sourceVector, i));  \
    }       \
    return VECTOR_METHOD(NAME, RemoveDup)(destVector);                             \
}                                                        \
\
static inline VECTOR_TYPEDEF(NAME) * VECTOR_METHOD(NAME, Intersect)(VECTOR_TYPEDEF(NAME) *destVector, VECTOR_TYPEDEF(NAME) *sourceVector) {   \
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
    return VECTOR_METHOD(NAME, RemoveDup)(destVector);                         \
}   \
\
static inline VECTOR_TYPEDEF(NAME) * VECTOR_METHOD(NAME, Subtract)(VECTOR_TYPEDEF(NAME) *destVector, VECTOR_TYPEDEF(NAME) *sourceVector) {   \
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
static inline VECTOR_TYPEDEF(NAME) * VECTOR_METHOD(NAME, Disjunction)(VECTOR_TYPEDEF(NAME) *destVector, VECTOR_TYPEDEF(NAME) *sourceVector) {    \
    if (VECTOR_METHOD(NAME, AddAll)(destVector, sourceVector)) {                           \
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
}                                                        \
\


#define CREATE_VECTOR_TYPE_1(TYPE) CREATE_VECTOR_TYPE_NAME(TYPE, TYPE, COMPARATOR_FOR_TYPE(TYPE))
#define CREATE_VECTOR_TYPE_2(TYPE, NAME) CREATE_VECTOR_TYPE_NAME(TYPE, NAME, COMPARATOR_FOR_TYPE(TYPE))
#define CREATE_VECTOR_TYPE_3(TYPE, NAME, COMPARE_FUN) CREATE_VECTOR_TYPE_NAME(TYPE, NAME, COMPARE_FUN)
#define CREATE_VECTOR_TYPE_MACRO(_1, _2, _3, FUN, ...) FUN

#define CREATE_VECTOR_TYPE(...)                                     \
    CREATE_VECTOR_TYPE_MACRO(__VA_ARGS__,                           \
                        CREATE_VECTOR_TYPE_3,                       \
                        CREATE_VECTOR_TYPE_2,                       \
                        CREATE_VECTOR_TYPE_1,                       \
                        ERROR)(__VA_ARGS__)


#define NEW_VECTOR_2(TYPE, NAME, CAPACITY) new ## NAME ## BuffVector(&(VECTOR_TYPEDEF(NAME)){0}, (TYPE [CAPACITY]){0}, CAPACITY)
#define NEW_VECTOR_1(TYPE, CAPACITY) NEW_VECTOR_2(TYPE, TYPE, CAPACITY)
#define NEW_VECTOR_MACRO(_1, _2, _3, FUN, ...) FUN

#define NEW_VECTOR(...)                                     \
    NEW_VECTOR_MACRO(__VA_ARGS__,                           \
                        NEW_VECTOR_2,                       \
                        NEW_VECTOR_1,                       \
                        ERROR)(__VA_ARGS__)

#define NEW_VECTOR_4(...)    NEW_VECTOR(__VA_ARGS__, 4)
#define NEW_VECTOR_8(...)    NEW_VECTOR(__VA_ARGS__, 8)
#define NEW_VECTOR_16(...)   NEW_VECTOR(__VA_ARGS__, 16)
#define NEW_VECTOR_32(...)   NEW_VECTOR(__VA_ARGS__, 32)
#define NEW_VECTOR_64(...)   NEW_VECTOR(__VA_ARGS__, 64)
#define NEW_VECTOR_128(...)  NEW_VECTOR(__VA_ARGS__, 128)
#define NEW_VECTOR_256(...)  NEW_VECTOR(__VA_ARGS__, 256)
#define NEW_VECTOR_512(...)  NEW_VECTOR(__VA_ARGS__, 512)
#define NEW_VECTOR_1024(...) NEW_VECTOR(__VA_ARGS__, 1024)

#define NEW_VECTOR_BUFF(TYPE, NAME, BUFFER, SIZE) new ## NAME ## BuffVector(&(VECTOR_TYPEDEF(NAME)){0}, BUFFER, SIZE)

#define NEW_VECTOR_OF(CAPACITY, TYPE, NAME, ...) new ## NAME ## BuffVectorOf(&(VECTOR_TYPEDEF(NAME)){0}, (TYPE [CAPACITY]){__VA_ARGS__}, CAPACITY, VAR_ARGS_LENGTH(TYPE, __VA_ARGS__))

#define CREATE_VECTOR_1(TYPE, NAME, V1) \
        NEW_VECTOR_OF(1, TYPE, NAME, V1)
#define CREATE_VECTOR_2(TYPE, NAME, V1, V2) \
        NEW_VECTOR_OF(2, TYPE, NAME, V1, V2)
#define CREATE_VECTOR_3(TYPE, NAME, V1, V2, V3) \
        NEW_VECTOR_OF(3, TYPE, NAME, V1, V2, V3)
#define CREATE_VECTOR_4(TYPE, NAME, V1, V2, V3, V4) \
        NEW_VECTOR_OF(4, TYPE, NAME, V1, V2, V3, V4)
#define CREATE_VECTOR_5(TYPE, NAME, V1, V2, V3, V4, V5) \
        NEW_VECTOR_OF(5, TYPE, NAME, V1, V2, V3, V4, V5)
#define CREATE_VECTOR_6(TYPE, NAME, V1, V2, V3, V4, V5, V6) \
        NEW_VECTOR_OF(6, TYPE, NAME, V1, V2, V3, V4, V5, V6)
#define CREATE_VECTOR_7(TYPE, NAME, V1, V2, V3, V4, V5, V6, V7) \
        NEW_VECTOR_OF(7, TYPE, NAME, V1, V2, V3, V4, V5, V6, V7)
#define CREATE_VECTOR_8(TYPE, NAME, V1, V2, V3, V4, V5, V6, V7, V8) \
        NEW_VECTOR_OF(8, TYPE, NAME, V1, V2, V3, V4, V5, V6, V7, V8)
#define CREATE_VECTOR_9(TYPE, NAME, V1, V2, V3, V4, V5, V6, V7, V8, V9) \
        NEW_VECTOR_OF(9, TYPE, NAME, V1, V2, V3, V4, V5, V6, V7, V8, V9)
#define CREATE_VECTOR_10(TYPE, NAME, V1, V2, V3, V4, V5, V6, V7, V8, V9, V10) \
        NEW_VECTOR_OF(10, TYPE, NAME, V1, V2, V3, V4, V5, V6, V7, V8, V9, V10)
#define CREATE_VECTOR_11(TYPE, NAME, V1, V2, V3, V4, V5, V6, V7, V8, V9, V10, V11) \
        NEW_VECTOR_OF(11, TYPE, NAME, V1, V2, V3, V4, V5, V6, V7, V8, V9, V10, V11)
#define CREATE_VECTOR_12(TYPE, NAME, V1, V2, V3, V4, V5, V6, V7, V8, V9, V10, V11, V12) \
        NEW_VECTOR_OF(12, TYPE, NAME, V1, V2, V3, V4, V5, V6, V7, V8, V9, V10, V11, V12)
#define CREATE_VECTOR_13(TYPE, NAME, V1, V2, V3, V4, V5, V6, V7, V8, V9, V10, V11, V12, V13) \
        NEW_VECTOR_OF(13, TYPE, NAME, V1, V2, V3, V4, V5, V6, V7, V8, V9, V10, V11, V12, V13)
#define CREATE_VECTOR_14(TYPE, NAME, V1, V2, V3, V4, V5, V6, V7, V8, V9, V10, V11, V12, V13, V14) \
        NEW_VECTOR_OF(14, TYPE, NAME, V1, V2, V3, V4, V5, V6, V7, V8, V9, V10, V11, V12, V13, V14)
#define CREATE_VECTOR_15(TYPE, NAME, V1, V2, V3, V4, V5, V6, V7, V8, V9, V10, V11, V12, V13, V14, V15) \
        NEW_VECTOR_OF(15, TYPE, NAME, V1, V2, V3, V4, V5, V6, V7, V8, V9, V10, V11, V12, V13, V14, V15)
#define CREATE_VECTOR_16(TYPE, NAME, V1, V2, V3, V4, V5, V6, V7, V8, V9, V10, V11, V12, V13, V14, V15, V16) \
        NEW_VECTOR_OF(16, TYPE, NAME, V1, V2, V3, V4, V5, V6, V7, V8, V9, V10, V11, V12, V13, V14, V15, V16)
#define CREATE_VECTOR_17(TYPE, NAME, V1, V2, V3, V4, V5, V6, V7, V8, V9, V10, V11, V12, V13, V14, V15, V16, V17) \
        NEW_VECTOR_OF(17, TYPE, NAME, V1, V2, V3, V4, V5, V6, V7, V8, V9, V10, V11, V12, V13, V14, V15, V16, V17)
#define CREATE_VECTOR_18(TYPE, NAME, V1, V2, V3, V4, V5, V6, V7, V8, V9, V10, V11, V12, V13, V14, V15, V16, V17, V18) \
        NEW_VECTOR_OF(18, TYPE, NAME, V1, V2, V3, V4, V5, V6, V7, V8, V9, V10, V11, V12, V13, V14, V15, V16, V17, V18)
#define CREATE_VECTOR_19(TYPE, NAME, V1, V2, V3, V4, V5, V6, V7, V8, V9, V10, V11, V12, V13, V14, V15, V16, V17, V18, V19) \
        NEW_VECTOR_OF(19, TYPE, NAME, V1, V2, V3, V4, V5, V6, V7, V8, V9, V10, V11, V12, V13, V14, V15, V16, V17, V18, V19)
#define CREATE_VECTOR_20(TYPE, NAME, V1, V2, V3, V4, V5, V6, V7, V8, V9, V10, V11, V12, V13, V14, V15, V16, V17, V18, V19, V20) \
        NEW_VECTOR_OF(20, TYPE, NAME, V1, V2, V3, V4, V5, V6, V7, V8, V9, V10, V11, V12, V13, V14, V15, V16, V17, V18, V19, V20)

#define GET_CREATE_VECTOR_MACRO(TYPE, NAME, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, FUN, ...) FUN

#define VECTOR_OF(TYPE, NAME, ...)                          \
    GET_CREATE_VECTOR_MACRO(TYPE, NAME, __VA_ARGS__,        \
                        CREATE_VECTOR_20,                   \
                        CREATE_VECTOR_19,                   \
                        CREATE_VECTOR_18,                   \
                        CREATE_VECTOR_17,                   \
                        CREATE_VECTOR_16,                   \
                        CREATE_VECTOR_15,                   \
                        CREATE_VECTOR_14,                   \
                        CREATE_VECTOR_13,                   \
                        CREATE_VECTOR_12,                   \
                        CREATE_VECTOR_11,                   \
                        CREATE_VECTOR_10,                   \
                        CREATE_VECTOR_9,                   \
                        CREATE_VECTOR_8,                   \
                        CREATE_VECTOR_7,                   \
                        CREATE_VECTOR_6,                   \
                        CREATE_VECTOR_5,                   \
                        CREATE_VECTOR_4,                   \
                        CREATE_VECTOR_3,                   \
                        CREATE_VECTOR_2,                   \
                        CREATE_VECTOR_1,                   \
                        ERROR)(TYPE, NAME, __VA_ARGS__)

#define VECTOR(TYPE, ...) VECTOR_OF(TYPE, TYPE, __VA_ARGS__)
