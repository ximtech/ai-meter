#pragma once

#include <stdio.h>
#include <stdlib.h>
#include "Comparator.h"

#define VECTOR_DEQUE_ALIGN_CAPACITY(CAPACITY) (((CAPACITY) + 1) & ~1)

#define VECTOR_DEQUE_TYPEDEF(NAME) NAME ##VecDeq
#define VECTOR_DEQUE_METHOD_NAME_2(PREFIX, NAME, POSTFIX) PREFIX ## NAME ## VecDeq ## POSTFIX
#define VECTOR_DEQUE_METHOD_NAME_1(NAME, POSTFIX) NAME ## VecDeq ## POSTFIX

#define VECTOR_DEQUE_METHOD_MACRO(_1, _2, _3, FUN, ...) FUN
#define VECTOR_DEQUE_METHOD(...)                                     \
    VECTOR_DEQUE_METHOD_MACRO(__VA_ARGS__,                           \
                        VECTOR_DEQUE_METHOD_NAME_2,                  \
                        VECTOR_DEQUE_METHOD_NAME_1,                  \
                        ERROR)(__VA_ARGS__)                          \


#define CREATE_VECTOR_DEQUE_TYPE_NAME(TYPE, NAME, COMPARE_FUN) \
typedef struct VECTOR_DEQUE_TYPEDEF(NAME) {  \
    TYPE *items;                       \
    uint32_t head;                     \
    int32_t tail;                      \
    uint32_t size;                     \
    uint32_t capacity;                 \
} VECTOR_DEQUE_TYPEDEF(NAME);          \
\
static inline VECTOR_DEQUE_TYPEDEF(NAME) * new ## NAME ## BuffVecDeq(VECTOR_DEQUE_TYPEDEF(NAME) *vector, TYPE *buffer, uint32_t capacity) { \
    if (vector == NULL || capacity == 0) return NULL;       \
    vector->size = 0;                                       \
    vector->capacity = capacity;                            \
    vector->items = buffer;                                 \
    vector->head = capacity;                                \
    vector->tail = -1;                                      \
    return vector;                                          \
}                                      \
\
static inline bool VECTOR_DEQUE_METHOD(NAME, AddFirst)(VECTOR_DEQUE_TYPEDEF(NAME) *vector, TYPE item) { \
    if (vector != NULL && (vector->head - 1) != vector->tail) { \
        vector->items[--vector->head] = item;                   \
        vector->size++;                                         \
        return true;                                            \
    }                                                           \
    return false;                                               \
}                                      \
\
static inline bool VECTOR_DEQUE_METHOD(NAME, AddLast)(VECTOR_DEQUE_TYPEDEF(NAME) *vector, TYPE item) { \
    if (vector != NULL && vector->head != (vector->tail + 1)) { \
        vector->items[++vector->tail] = item;                   \
        vector->size++;                                         \
        return true;                                            \
    }                                                           \
    return false;                                               \
}                                                               \
\
static inline TYPE VECTOR_DEQUE_METHOD(NAME, GetFirst)(VECTOR_DEQUE_TYPEDEF(NAME) *vector) {  \
    return vector != NULL && vector->head < vector->capacity ? vector->items[vector->head] : (TYPE) {0};    \
} \
\
static inline TYPE VECTOR_DEQUE_METHOD(NAME, GetLast)(VECTOR_DEQUE_TYPEDEF(NAME) *vector) {  \
    return vector != NULL && vector->tail > -1 ? vector->items[vector->tail] : (TYPE) {0};    \
}                                                              \
\
static inline TYPE VECTOR_DEQUE_METHOD(NAME, RemoveFirst)(VECTOR_DEQUE_TYPEDEF(NAME) *vector) {    \
    if (vector != NULL && vector->head != vector->capacity) {               \
        TYPE item = vector->items[vector->head];                            \
        vector->items[vector->head] = (TYPE) {0};                           \
        vector->head++;                                                     \
        vector->size--;                                                     \
        return item;                                                        \
    }                                                                       \
    return (TYPE) {0};                                                      \
}                                      \
\
static inline TYPE VECTOR_DEQUE_METHOD(NAME, RemoveLast)(VECTOR_DEQUE_TYPEDEF(NAME) *vector) {    \
    if (vector != NULL && vector->tail >= 0) {                              \
        TYPE item = vector->items[vector->tail];                            \
        vector->items[vector->tail] = (TYPE) {0};                           \
        vector->tail--;                                                     \
        vector->size--;                                                     \
        return item;                                                        \
    }                                                                       \
    return (TYPE) {0};                                                      \
}                                                                           \
\
static inline bool VECTOR_DEQUE_METHOD(is, NAME, Empty)(VECTOR_DEQUE_TYPEDEF(NAME) *vector) {  \
    return (vector == NULL) || (vector->size == 0);     \
}                                      \
\
static inline bool VECTOR_DEQUE_METHOD(is, NAME, NotEmpty)(VECTOR_DEQUE_TYPEDEF(NAME) *vector) {   \
    return !VECTOR_DEQUE_METHOD(is, NAME, Empty)(vector);                             \
}                                      \
\
static inline uint32_t VECTOR_DEQUE_METHOD(NAME, Size)(VECTOR_DEQUE_TYPEDEF(NAME) *vector) { \
    return vector != NULL ? vector->size : 0;                       \
}                                      \
\
static inline void VECTOR_DEQUE_METHOD(NAME, Clear)(VECTOR_DEQUE_TYPEDEF(NAME) *vector) {   \
    if (vector != NULL) {                               \
    for (uint32_t i = vector->capacity - 1; i >= vector->head; i--) {   \
        vector->items[i] = (TYPE) {0};               \
    }                                               \
    for (int32_t i = 0; i <= vector->tail; i++) {  \
        vector->items[i] = (TYPE) {0};               \
    }                                               \
    vector->size = 0;                               \
    vector->head = vector->capacity;                \
    vector->tail = -1;                              \
    }                                               \
}                                                   \
\
static inline bool VECTOR_DEQUE_METHOD(NAME, Contains)(VECTOR_DEQUE_TYPEDEF(NAME) *vector, TYPE value) { \
    if (vector == NULL) return false;                          \
    for (uint32_t i = vector->capacity - 1; i >= vector->head; i--) {   \
        if (COMPARE_FUN(vector->items[i], value) == 0) {        \
            return true;                                        \
        }                                                       \
    }                                                           \
    for (int32_t i = 0; i <= vector->tail; i++) {              \
        if (COMPARE_FUN(vector->items[i], value) == 0) {        \
            return true;                                        \
        }                                                       \
    }                                                           \
    return false;                                               \
}                                                               \
\
static inline TYPE VECTOR_DEQUE_METHOD(NAME, RemoveFirstOccur)(VECTOR_DEQUE_TYPEDEF(NAME) *vector, TYPE value) {   \
    if (vector == NULL) return (TYPE) {0};                                       \
    for (uint32_t i = vector->capacity - 1; i >= vector->head; i--) {   \
        TYPE arrayValue = vector->items[i];                             \
        if (COMPARE_FUN(arrayValue, value) == 0) {                      \
            if (i == vector->head) {                                    \
                VECTOR_DEQUE_METHOD(NAME, RemoveFirst)(vector);         \
                return arrayValue;                                      \
            }                                                           \
                                                                        \
            for (uint32_t j = (i - 1); j >= vector->head; j--, i--) {   \
                vector->items[i] = vector->items[j];                    \
            }                                                           \
            vector->items[i] = (TYPE) {0};                              \
            vector->head++;                                             \
            vector->size--;                                             \
            return arrayValue;                                          \
        }                                                               \
    }                                                                   \
    return (TYPE) {0};                                                  \
}       \
\
static inline TYPE VECTOR_DEQUE_METHOD(NAME, RemoveLastOccur)(VECTOR_DEQUE_TYPEDEF(NAME) *vector, TYPE value) {    \
    if (vector == NULL) return (TYPE) {0};                              \
    for (uint32_t i = 0; i <= vector->tail; i++) {                      \
        TYPE arrayValue = vector->items[i];                             \
        if (COMPARE_FUN(arrayValue, value) == 0) {                      \
            if (i == vector->tail) {                                    \
                VECTOR_DEQUE_METHOD(NAME, RemoveLast)(vector);          \
                return arrayValue;                                      \
            }                                                           \
                                                                        \
            for (int32_t j = (i + 1); j <= vector->tail; j++, i++) {   \
                vector->items[i] = vector->items[j];                    \
            }                                                           \
            vector->items[i] = (TYPE) {0};                              \
            vector->tail--;                                             \
            vector->size--;                                             \
            return arrayValue;                                          \
        }                                                               \
    }                                                                   \
    return (TYPE) {0};                                                  \
}


#define CREATE_VECTOR_DEQ_TYPE_1(TYPE) CREATE_VECTOR_DEQUE_TYPE_NAME(TYPE, TYPE, COMPARATOR_FOR_TYPE(TYPE))
#define CREATE_VECTOR_DEQ_TYPE_2(TYPE, NAME) CREATE_VECTOR_DEQUE_TYPE_NAME(TYPE, NAME, COMPARATOR_FOR_TYPE(TYPE))
#define CREATE_VECTOR_DEQ_TYPE_3(TYPE, NAME, COMPARE_FUN) CREATE_VECTOR_DEQUE_TYPE_NAME(TYPE, NAME, COMPARE_FUN)
#define CREATE_VECTOR_DEQ_TYPE_MACRO(_1, _2, _3, FUN, ...) FUN
#define CREATE_VECTOR_DEQ_TYPE(...)                                     \
    CREATE_VECTOR_DEQ_TYPE_MACRO(__VA_ARGS__,                           \
                        CREATE_VECTOR_DEQ_TYPE_3,                       \
                        CREATE_VECTOR_DEQ_TYPE_2,                       \
                        CREATE_VECTOR_DEQ_TYPE_1,                       \
                        ERROR)(__VA_ARGS__)


#define NEW_VECTOR_DEQ_2(TYPE, NAME, CAPACITY) new ## NAME ## BuffVecDeq(&(VECTOR_DEQUE_TYPEDEF(NAME)){0}, \
                                                                            (TYPE [VECTOR_DEQUE_ALIGN_CAPACITY(CAPACITY)]){0}, \
                                                                            VECTOR_DEQUE_ALIGN_CAPACITY(CAPACITY))
#define NEW_VECTOR_DEQ_1(TYPE, CAPACITY) NEW_VECTOR_DEQ_2(TYPE, TYPE, CAPACITY)
#define NEW_VECTOR_DEQ_MACRO(_1, _2, _3, FUN, ...) FUN
#define NEW_VECTOR_DEQ(...)                                     \
    NEW_VECTOR_DEQ_MACRO(__VA_ARGS__,                           \
                        NEW_VECTOR_DEQ_2,                       \
                        NEW_VECTOR_DEQ_1,                       \
                        ERROR)(__VA_ARGS__)

#define NEW_VECTOR_DEQ_4(...)    NEW_VECTOR_DEQ(__VA_ARGS__, 4)
#define NEW_VECTOR_DEQ_8(...)    NEW_VECTOR_DEQ(__VA_ARGS__, 8)
#define NEW_VECTOR_DEQ_16(...)   NEW_VECTOR_DEQ(__VA_ARGS__, 16)
#define NEW_VECTOR_DEQ_32(...)   NEW_VECTOR_DEQ(__VA_ARGS__, 32)
#define NEW_VECTOR_DEQ_64(...)   NEW_VECTOR_DEQ(__VA_ARGS__, 64)
#define NEW_VECTOR_DEQ_128(...)  NEW_VECTOR_DEQ(__VA_ARGS__, 128)
#define NEW_VECTOR_DEQ_256(...)  NEW_VECTOR_DEQ(__VA_ARGS__, 256)
#define NEW_VECTOR_DEQ_512(...)  NEW_VECTOR_DEQ(__VA_ARGS__, 512)
#define NEW_VECTOR_DEQ_1024(...) NEW_VECTOR_DEQ(__VA_ARGS__, 1024)
