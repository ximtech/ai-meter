#pragma once

#include <stdio.h>
#include <stdlib.h>
#include "Comparator.h"

#define HASH_SET_EXPAND_FACTOR 2
#define HASH_SET_BUFFER_EXPAND_FACTOR(CAPACITY) ((CAPACITY) * HASH_SET_EXPAND_FACTOR)
#define HASH_SET_ALIGN_CAPACITY(CAPACITY) (NEXT_POW_OF_2(HASH_SET_BUFFER_EXPAND_FACTOR(CAPACITY)))

#define HASH_SET_ENTRY_TYPEDEF(NAME) NAME ##HashSetEntry
#define HASH_SET_TYPEDEF(NAME) NAME ##HashSet
#define HASH_SET_ITERATOR_TYPEDEF(NAME) NAME ##SetIterator

#define HASH_SET_METHOD_NAME_2(PREFIX, NAME, POSTFIX) PREFIX ## NAME ## POSTFIX
#define HASH_SET_METHOD_NAME_1(NAME, POSTFIX) NAME ## POSTFIX
#define HASH_SET_METHOD_MACRO(_1, _2, _3, FUN, ...) FUN
#define HASH_SET_METHOD(...)                                     \
    HASH_SET_METHOD_MACRO(__VA_ARGS__,                           \
                        HASH_SET_METHOD_NAME_2,                  \
                        HASH_SET_METHOD_NAME_1,                  \
                        ERROR)(__VA_ARGS__)                      \


#define CREATE_HASH_SET_TYPE_NAME(TYPE, NAME, COMPARE_FUN, HASH_FUN) \
typedef struct HASH_SET_ENTRY_TYPEDEF(NAME) { \
    TYPE value;                 \
    bool isDeleted;             \
    bool isEmptySlot;           \
} HASH_SET_ENTRY_TYPEDEF(NAME); \
\
typedef struct HASH_SET_TYPEDEF(NAME) { \
    HASH_SET_ENTRY_TYPEDEF(NAME) *entries; \
    uint32_t size;                  \
    uint32_t capacity;              \
    uint32_t deletedItemsCount;     \
} HASH_SET_TYPEDEF(NAME);           \
\
typedef struct HASH_SET_ITERATOR_TYPEDEF(NAME) {    \
    TYPE value;                                     \
    HASH_SET_TYPEDEF(NAME) *set;                    \
    uint32_t index;                                 \
} HASH_SET_ITERATOR_TYPEDEF(NAME);                  \
                                                    \
static inline HASH_SET_TYPEDEF(NAME) * HASH_SET_METHOD(new, NAME, BufferSet)(HASH_SET_TYPEDEF(NAME) *set, HASH_SET_ENTRY_TYPEDEF(NAME) *entries, uint32_t capacity) { \
    if (set == NULL) return NULL;       \
    set->entries = entries;             \
    set->size = 0;                      \
    set->capacity = capacity;           \
    set->deletedItemsCount = 0;         \
    \
    for (uint32_t i = 0; i < capacity; i++) {   \
        HASH_SET_ENTRY_TYPEDEF(NAME) *entry = &entries[i]; \
        entry->value = (TYPE) {0};              \
        entry->isDeleted = false;               \
        entry->isEmptySlot = true;              \
    }                                           \
    return set;                                 \
}                                               \
                                                \
static inline HASH_SET_ENTRY_TYPEDEF(NAME) * HASH_SET_METHOD(find, NAME, SetEntry)(HASH_SET_TYPEDEF(NAME) *set, TYPE value) { \
    uint32_t hash = HASH_FUN(value);                    \
    uint32_t index = hash & (set->capacity - 1);        \
    HASH_SET_ENTRY_TYPEDEF(NAME) *tombstone = NULL;     \
                                                        \
    while (true) {                                      \
        HASH_SET_ENTRY_TYPEDEF(NAME) *entry = &set->entries[index]; \
        if (entry->isEmptySlot) {                               \
            if (!entry->isDeleted) {                            \
                return tombstone != NULL ? tombstone : entry;   \
            } else {                                            \
                if (tombstone == NULL) {                        \
                    tombstone = entry;                          \
                }                                               \
            }                                                   \
        } else if (COMPARE_FUN(value, entry->value) == 0) {     \
            return entry;                                       \
        }                                                       \
        index = (index + 1) & (set->capacity - 1);              \
    }                                                           \
}                                                               \
\
static inline bool HASH_SET_METHOD(NAME, SetAdd)(HASH_SET_TYPEDEF(NAME) *set, TYPE value) {   \
    if (set != NULL && (set->size < (set->capacity / HASH_SET_EXPAND_FACTOR))) {                  \
        HASH_SET_ENTRY_TYPEDEF(NAME) *entry = HASH_SET_METHOD(find, NAME, SetEntry)(set, value);                                                               \
        bool alreadyExist = !entry->isEmptySlot;    \
        if (!alreadyExist) {                \
            set->size++;                    \
        }                                   \
                                            \
        if (entry->isDeleted) {             \
            set->deletedItemsCount--;       \
        }                                   \
        entry->value = value;               \
        entry->isDeleted = false;           \
        entry->isEmptySlot = false;         \
        return !alreadyExist;               \
    }                                       \
    return false;                           \
}                                           \
\
static inline HASH_SET_TYPEDEF(NAME) * HASH_SET_METHOD(new, NAME, BufferSetOf)(HASH_SET_TYPEDEF(NAME) *set, HASH_SET_ENTRY_TYPEDEF(NAME) *entries, uint32_t capacity, uint32_t size) { \
    if (set == NULL || entries == NULL) return NULL;    \
    HASH_SET_ENTRY_TYPEDEF(NAME) tmpEntries[size];      \
    memcpy(tmpEntries, entries, sizeof(tmpEntries));    \
    set = HASH_SET_METHOD(new, NAME, BufferSet)(set, entries, capacity); \
    for (uint32_t i = 0; i < size; i++) {               \
        HASH_SET_METHOD(NAME, SetAdd)(set, tmpEntries[i].value); \
    }                                                   \
    return set;                                         \
}                                                       \
\
static inline uint32_t HASH_SET_METHOD(NAME, SetSize)(HASH_SET_TYPEDEF(NAME) *set) {  \
    return set != NULL ? set->size : 0; \
}   \
\
static inline bool HASH_SET_METHOD(is, NAME, SetEmpty)(HASH_SET_TYPEDEF(NAME) *set) {  \
    return set != NULL ? set->size == 0 : true; \
}   \
\
static inline bool HASH_SET_METHOD(is, NAME, SetNotEmpty)(HASH_SET_TYPEDEF(NAME) *set) {   \
    return !HASH_SET_METHOD(is, NAME, SetEmpty)(set);        \
}   \
\
static inline bool HASH_SET_METHOD(NAME, SetContains)(HASH_SET_TYPEDEF(NAME) *set, TYPE value) {  \
    if (HASH_SET_METHOD(is, NAME, SetNotEmpty)(set)) {               \
        HASH_SET_ENTRY_TYPEDEF(NAME) *entry = HASH_SET_METHOD(find, NAME, SetEntry)(set, value); \
        return !entry->isEmptySlot;     \
    }               \
    return false;   \
}                   \
\
static inline bool HASH_SET_METHOD(NAME, SetRemove)(HASH_SET_TYPEDEF(NAME) *set, TYPE value) { \
    if (HASH_SET_METHOD(is, NAME, SetNotEmpty)(set)) {                                                          \
        HASH_SET_ENTRY_TYPEDEF(NAME) *entry = HASH_SET_METHOD(find, NAME, SetEntry)(set, value);  \
        if (entry->isEmptySlot) return false;   \
        entry->value = (TYPE) {0};              \
        entry->isDeleted = true;                \
        entry->isEmptySlot = true;              \
        set->size--;                            \
        set->deletedItemsCount++;               \
        return true;                            \
    }                                           \
    return false;                               \
}                                               \
\
static inline void HASH_SET_METHOD(NAME, SetAddAll)(HASH_SET_TYPEDEF(NAME) *fromSet, HASH_SET_TYPEDEF(NAME) *toSet) {  \
    for (uint32_t i = 0; i < fromSet->capacity; i++) {                        \
        HASH_SET_ENTRY_TYPEDEF(NAME) *entry = &fromSet->entries[i];           \
        if (!entry->isEmptySlot) {                                            \
            HASH_SET_METHOD(NAME, SetAdd)(toSet, entry->value);               \
        }                                                                     \
    }                                                                         \
}                                                                    \
\
static inline void HASH_SET_METHOD(NAME, SetClear)(HASH_SET_TYPEDEF(NAME) *set) {    \
    if (set != NULL && set->entries != NULL) {          \
        for (uint32_t i = 0; i < set->capacity; i++) {  \
            set->entries[i].value = (TYPE) {0};         \
            set->entries[i].isDeleted = false;          \
            set->entries[i].isEmptySlot = true;         \
        }                                               \
        set->size = 0;                                  \
        set->deletedItemsCount = 0;                     \
    }                                                   \
}                                                       \
\
static inline HASH_SET_ITERATOR_TYPEDEF(NAME) HASH_SET_METHOD(NAME, SetIter)(HASH_SET_TYPEDEF(NAME) *set) { \
    HASH_SET_ITERATOR_TYPEDEF(NAME) iterator = {.set = set, .index = 0};    \
    return iterator;    \
}                       \
\
static inline bool HASH_SET_METHOD(NAME, SetHasNext)(HASH_SET_ITERATOR_TYPEDEF(NAME) *iterator) { \
    if (iterator != NULL && iterator->set != NULL) {        \
        HASH_SET_TYPEDEF(NAME) *set = iterator->set;        \
        while (iterator->index < set->capacity) {           \
            uint32_t indexValue = iterator->index;          \
            iterator->index++;                              \
                                                            \
            HASH_SET_ENTRY_TYPEDEF(NAME) *pair = &set->entries[indexValue]; \
            if (!pair->isEmptySlot) {                       \
                iterator->value = pair->value;              \
                return true;                                \
            }                       \
        }                           \
        return false;               \
    }                               \
    return false;                   \
}                                                                    \
                                                                     \
static inline bool HASH_SET_METHOD(NAME, SetContainsAll)(HASH_SET_TYPEDEF(NAME) *fromSet, HASH_SET_TYPEDEF(NAME) *compareSet) { \
    HASH_SET_ITERATOR_TYPEDEF(NAME) iterator = HASH_SET_METHOD(NAME, SetIter)(compareSet); \
    while (HASH_SET_METHOD(NAME, SetHasNext)(&iterator)) {                        \
        if (!HASH_SET_METHOD(NAME, SetContains)(fromSet, iterator.value)) {       \
            return false;                                                         \
        }                                                                         \
    }                                                                             \
    return true;                                                                  \
}\



#define CREATE_HASH_SET_TYPE_1(TYPE) CREATE_HASH_SET_TYPE_NAME(TYPE, TYPE, COMPARATOR_FOR_TYPE(TYPE), HASH_CODE_FOR_TYPE(TYPE))
#define CREATE_HASH_SET_TYPE_2(TYPE, NAME) CREATE_HASH_SET_TYPE_NAME(TYPE, NAME, COMPARATOR_FOR_TYPE(TYPE), HASH_CODE_FOR_TYPE(TYPE))
#define CREATE_HASH_SET_TYPE_3(TYPE, COMPARE_FUN, HASH_FUN) CREATE_HASH_SET_TYPE_NAME(TYPE, TYPE, COMPARE_FUN, HASH_FUN)
#define CREATE_HASH_SET_TYPE_4(TYPE, NAME, COMPARE_FUN, HASH_FUN) CREATE_HASH_SET_TYPE_NAME(TYPE, NAME, COMPARE_FUN, HASH_FUN)
#define CREATE_HASH_SET_TYPE_MACRO(_1, _2, _3, _4, FUN, ...) FUN

#define CREATE_HASH_SET_TYPE(...)                                     \
    CREATE_HASH_SET_TYPE_MACRO(__VA_ARGS__,                           \
                        CREATE_HASH_SET_TYPE_4,                       \
                        CREATE_HASH_SET_TYPE_3,                       \
                        CREATE_HASH_SET_TYPE_2,                       \
                        CREATE_HASH_SET_TYPE_1,                       \
                        ERROR)(__VA_ARGS__)


#define NEW_HASH_SET(NAME, CAPACITY) \
HASH_SET_METHOD(new, NAME, BufferSet)(&(HASH_SET_TYPEDEF(NAME)){0}, \
                                       (HASH_SET_ENTRY_TYPEDEF(NAME) [HASH_SET_ALIGN_CAPACITY(CAPACITY)]){0}, \
                                        HASH_SET_ALIGN_CAPACITY(CAPACITY))

#define NEW_HASH_SET_4(NAME)    NEW_HASH_SET(NAME, 4)
#define NEW_HASH_SET_8(NAME)    NEW_HASH_SET(NAME, 8)
#define NEW_HASH_SET_16(NAME)   NEW_HASH_SET(NAME, 16)
#define NEW_HASH_SET_32(NAME)   NEW_HASH_SET(NAME, 32)
#define NEW_HASH_SET_64(NAME)   NEW_HASH_SET(NAME, 64)
#define NEW_HASH_SET_128(NAME)  NEW_HASH_SET(NAME, 128)
#define NEW_HASH_SET_256(NAME)  NEW_HASH_SET(NAME, 256)
#define NEW_HASH_SET_512(NAME)  NEW_HASH_SET(NAME, 512)
#define NEW_HASH_SET_1024(NAME) NEW_HASH_SET(NAME, 1024)

#define SET_ENTRY(VALUE) {(VALUE), false, true}

#define NEW_HASH_SET_OF(CAPACITY, NAME, ...) \
HASH_SET_METHOD(new, NAME, BufferSetOf)(&(HASH_SET_TYPEDEF(NAME)){0}, \
                                         (HASH_SET_ENTRY_TYPEDEF(NAME) [HASH_SET_ALIGN_CAPACITY(CAPACITY)]){__VA_ARGS__}, \
                                          HASH_SET_ALIGN_CAPACITY(CAPACITY),      \
                                          VAR_ARGS_LENGTH(HASH_SET_ENTRY_TYPEDEF(NAME), __VA_ARGS__))

#define CREATE_HASH_SET_1(NAME, V1) \
        NEW_HASH_SET_OF(1, NAME, SET_ENTRY(V1))

#define CREATE_HASH_SET_2(NAME, V1, V2) \
        NEW_HASH_SET_OF(2, NAME, SET_ENTRY(V1), SET_ENTRY(V2))

#define CREATE_HASH_SET_3(NAME, V1, V2, V3) \
        NEW_HASH_SET_OF(3, NAME, SET_ENTRY(V1), SET_ENTRY(V2), SET_ENTRY(V3))

#define CREATE_HASH_SET_4(NAME, V1, V2, V3, V4) \
        NEW_HASH_SET_OF(4, NAME, SET_ENTRY(V1), SET_ENTRY(V2), SET_ENTRY(V3), SET_ENTRY(V4))

#define CREATE_HASH_SET_5(NAME, V1, V2, V3, V4, V5) \
        NEW_HASH_SET_OF(5, NAME, SET_ENTRY(V1), SET_ENTRY(V2), SET_ENTRY(V3), SET_ENTRY(V4), SET_ENTRY(V5))

#define CREATE_HASH_SET_6(NAME, V1, V2, V3, V4, V5, V6) \
        NEW_HASH_SET_OF(6, NAME, SET_ENTRY(V1), SET_ENTRY(V2), SET_ENTRY(V3), SET_ENTRY(V4), SET_ENTRY(V5), SET_ENTRY(V6))

#define CREATE_HASH_SET_7(NAME, V1, V2, V3, V4, V5, V6, V7) \
        NEW_HASH_SET_OF(7, NAME, SET_ENTRY(V1), SET_ENTRY(V2), SET_ENTRY(V3), SET_ENTRY(V4), SET_ENTRY(V5), SET_ENTRY(V6), SET_ENTRY(V7))

#define CREATE_HASH_SET_8(NAME, V1, V2, V3, V4, V5, V6, V7, V8) \
        NEW_HASH_SET_OF(8, NAME, SET_ENTRY(V1), SET_ENTRY(V2), SET_ENTRY(V3), SET_ENTRY(V4), SET_ENTRY(V5), SET_ENTRY(V6), SET_ENTRY(V7), SET_ENTRY(V8))

#define CREATE_HASH_SET_9(NAME, V1, V2, V3, V4, V5, V6, V7, V8, V9) \
        NEW_HASH_SET_OF(9, NAME, SET_ENTRY(V1), SET_ENTRY(V2), SET_ENTRY(V3), SET_ENTRY(V4), SET_ENTRY(V5), SET_ENTRY(V6), SET_ENTRY(V7), SET_ENTRY(V8), SET_ENTRY(V9))

#define CREATE_HASH_SET_10(NAME, V1, V2, V3, V4, V5, V6, V7, V8, V9, V10) \
        NEW_HASH_SET_OF(10, NAME, SET_ENTRY(V1), SET_ENTRY(V2), SET_ENTRY(V3), SET_ENTRY(V4), SET_ENTRY(V5), SET_ENTRY(V6), SET_ENTRY(V7), SET_ENTRY(V8), SET_ENTRY(V9), SET_ENTRY(V10))

#define CREATE_HASH_SET_11(NAME, V1, V2, V3, V4, V5, V6, V7, V8, V9, V10, V11) \
        NEW_HASH_SET_OF(11, NAME, SET_ENTRY(V1), SET_ENTRY(V2), SET_ENTRY(V3), SET_ENTRY(V4), SET_ENTRY(V5), SET_ENTRY(V6), SET_ENTRY(V7), SET_ENTRY(V8), SET_ENTRY(V9), SET_ENTRY(V10), SET_ENTRY(V11))

#define CREATE_HASH_SET_12(NAME, V1, V2, V3, V4, V5, V6, V7, V8, V9, V10, V11, V12) \
        NEW_HASH_SET_OF(12, NAME, SET_ENTRY(V1), SET_ENTRY(V2), SET_ENTRY(V3), SET_ENTRY(V4), SET_ENTRY(V5), SET_ENTRY(V6), SET_ENTRY(V7), SET_ENTRY(V8), SET_ENTRY(V9), SET_ENTRY(V10), SET_ENTRY(V11), SET_ENTRY(V12))

#define CREATE_HASH_SET_13(NAME, V1, V2, V3, V4, V5, V6, V7, V8, V9, V10, V11, V12, V13) \
        NEW_HASH_SET_OF(13, NAME, SET_ENTRY(V1), SET_ENTRY(V2), SET_ENTRY(V3), SET_ENTRY(V4), SET_ENTRY(V5), SET_ENTRY(V6), SET_ENTRY(V7), SET_ENTRY(V8), SET_ENTRY(V9), SET_ENTRY(V10), SET_ENTRY(V11), SET_ENTRY(V12), SET_ENTRY(V13))

#define CREATE_HASH_SET_14(NAME, V1, V2, V3, V4, V5, V6, V7, V8, V9, V10, V11, V12, V13, V14) \
        NEW_HASH_SET_OF(14, NAME, SET_ENTRY(V1), SET_ENTRY(V2), SET_ENTRY(V3), SET_ENTRY(V4), SET_ENTRY(V5), SET_ENTRY(V6), SET_ENTRY(V7), SET_ENTRY(V8), SET_ENTRY(V9), SET_ENTRY(V10), SET_ENTRY(V11), SET_ENTRY(V12), SET_ENTRY(V13), SET_ENTRY(V14))

#define CREATE_HASH_SET_15(NAME, V1, V2, V3, V4, V5, V6, V7, V8, V9, V10, V11, V12, V13, V14, V15) \
        NEW_HASH_SET_OF(15, NAME, SET_ENTRY(V1), SET_ENTRY(V2), SET_ENTRY(V3), SET_ENTRY(V4), SET_ENTRY(V5), SET_ENTRY(V6), SET_ENTRY(V7), SET_ENTRY(V8), SET_ENTRY(V9), SET_ENTRY(V10), SET_ENTRY(V11), SET_ENTRY(V12), SET_ENTRY(V13), SET_ENTRY(V14), SET_ENTRY(V15))

#define CREATE_HASH_SET_16(NAME, V1, V2, V3, V4, V5, V6, V7, V8, V9, V10, V11, V12, V13, V14, V15, V16) \
        NEW_HASH_SET_OF(16, NAME, SET_ENTRY(V1), SET_ENTRY(V2), SET_ENTRY(V3), SET_ENTRY(V4), SET_ENTRY(V5), SET_ENTRY(V6), SET_ENTRY(V7), SET_ENTRY(V8), SET_ENTRY(V9), SET_ENTRY(V10), SET_ENTRY(V11), SET_ENTRY(V12), SET_ENTRY(V13), SET_ENTRY(V14), SET_ENTRY(V15), SET_ENTRY(V16))

#define CREATE_HASH_SET_17(NAME, V1, V2, V3, V4, V5, V6, V7, V8, V9, V10, V11, V12, V13, V14, V15, V16, V17) \
        NEW_HASH_SET_OF(17, NAME, SET_ENTRY(V1), SET_ENTRY(V2), SET_ENTRY(V3), SET_ENTRY(V4), SET_ENTRY(V5), SET_ENTRY(V6), SET_ENTRY(V7), SET_ENTRY(V8), SET_ENTRY(V9), SET_ENTRY(V10), SET_ENTRY(V11), SET_ENTRY(V12), SET_ENTRY(V13), SET_ENTRY(V14), SET_ENTRY(V15), SET_ENTRY(V16), SET_ENTRY(V17))

#define CREATE_HASH_SET_18(NAME, V1, V2, V3, V4, V5, V6, V7, V8, V9, V10, V11, V12, V13, V14, V15, V16, V17, V18) \
        NEW_HASH_SET_OF(18, NAME, SET_ENTRY(V1), SET_ENTRY(V2), SET_ENTRY(V3), SET_ENTRY(V4), SET_ENTRY(V5), SET_ENTRY(V6), SET_ENTRY(V7), SET_ENTRY(V8), SET_ENTRY(V9), SET_ENTRY(V10), SET_ENTRY(V11), SET_ENTRY(V12), SET_ENTRY(V13), SET_ENTRY(V14), SET_ENTRY(V15), SET_ENTRY(V16), SET_ENTRY(V17), SET_ENTRY(V18))

#define CREATE_HASH_SET_19(NAME, V1, V2, V3, V4, V5, V6, V7, V8, V9, V10, V11, V12, V13, V14, V15, V16, V17, V18, V19) \
        NEW_HASH_SET_OF(19, NAME, SET_ENTRY(V1), SET_ENTRY(V2), SET_ENTRY(V3), SET_ENTRY(V4), SET_ENTRY(V5), SET_ENTRY(V6), SET_ENTRY(V7), SET_ENTRY(V8), SET_ENTRY(V9), SET_ENTRY(V10), SET_ENTRY(V11), SET_ENTRY(V12), SET_ENTRY(V13), SET_ENTRY(V14), SET_ENTRY(V15), SET_ENTRY(V16), SET_ENTRY(V17), SET_ENTRY(V18), SET_ENTRY(V19))

#define CREATE_HASH_SET_20(NAME, V1, V2, V3, V4, V5, V6, V7, V8, V9, V10, V11, V12, V13, V14, V15, V16, V17, V18, V19, V20) \
        NEW_HASH_SET_OF(20, NAME, SET_ENTRY(V1), SET_ENTRY(V2), SET_ENTRY(V3), SET_ENTRY(V4), SET_ENTRY(V5), SET_ENTRY(V6), SET_ENTRY(V7), SET_ENTRY(V8), SET_ENTRY(V9), SET_ENTRY(V10), SET_ENTRY(V11), SET_ENTRY(V12), SET_ENTRY(V13), SET_ENTRY(V14), SET_ENTRY(V15), SET_ENTRY(V16), SET_ENTRY(V17), SET_ENTRY(V18), SET_ENTRY(V19), SET_ENTRY(V20))

#define GET_CREATE_HASH_SET_MACRO(NAME, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, FUN, ...) FUN

#define HASH_SET_OF(NAME, ...)                                \
    GET_CREATE_HASH_SET_MACRO(NAME, __VA_ARGS__,              \
                        CREATE_HASH_SET_20,                   \
                        CREATE_HASH_SET_19,                   \
                        CREATE_HASH_SET_18,                   \
                        CREATE_HASH_SET_17,                   \
                        CREATE_HASH_SET_16,                   \
                        CREATE_HASH_SET_15,                   \
                        CREATE_HASH_SET_14,                   \
                        CREATE_HASH_SET_13,                   \
                        CREATE_HASH_SET_12,                   \
                        CREATE_HASH_SET_11,                   \
                        CREATE_HASH_SET_10,                   \
                        CREATE_HASH_SET_9,                    \
                        CREATE_HASH_SET_8,                    \
                        CREATE_HASH_SET_7,                    \
                        CREATE_HASH_SET_6,                    \
                        CREATE_HASH_SET_5,                    \
                        CREATE_HASH_SET_4,                    \
                        CREATE_HASH_SET_3,                    \
                        CREATE_HASH_SET_2,                    \
                        CREATE_HASH_SET_1,                    \
                        ERROR)(NAME, __VA_ARGS__)

