#pragma once

#include <stdio.h>
#include <stdlib.h>
#include "Comparator.h"

#define HASH_MAP_EXPAND_FACTOR 2
#define HASH_MAP_BUFFER_EXPAND_FACTOR(CAPACITY) ((CAPACITY) * HASH_MAP_EXPAND_FACTOR)
#define HASH_MAP_ALIGN_CAPACITY(CAPACITY) (NEXT_POW_OF_2(HASH_MAP_BUFFER_EXPAND_FACTOR(CAPACITY)))

#define HASH_MAP_ENTRY_TYPEDEF(KEY_NAME, VALUE_NAME) KEY_NAME ## _ ## VALUE_NAME ## MapEntry
#define HASH_MAP_TYPEDEF(KEY_NAME, VALUE_NAME) KEY_NAME ## _ ## VALUE_NAME ## Map
#define HASH_MAP_ITERATOR_TYPEDEF(KEY_NAME, VALUE_NAME) KEY_NAME ## _ ## VALUE_NAME ## MapIterator

#define HASH_MAP_METHOD_NAME_2(PREFIX, KEY_NAME, VALUE_NAME, POSTFIX) PREFIX ## _ ## KEY_NAME ## _ ## VALUE_NAME ## POSTFIX
#define HASH_MAP_METHOD_NAME_1(KEY_NAME, VALUE_NAME, POSTFIX) KEY_NAME ## _ ## VALUE_NAME ## POSTFIX
#define HASH_MAP_METHOD_MACRO(_1, _2, _3, _4, FUN, ...) FUN
#define HASH_MAP_METHOD(...)                                     \
    HASH_MAP_METHOD_MACRO(__VA_ARGS__,                           \
                        HASH_MAP_METHOD_NAME_2,                  \
                        HASH_MAP_METHOD_NAME_1,                  \
                        ERROR)(__VA_ARGS__)                      \


#define CREATE_HASH_MAP_TYPE_NAME(KEY_TYPE, VALUE_TYPE, KEY_NAME, VALUE_NAME, COMPARE_FUN, HASH_FUN) \
typedef struct HASH_MAP_ENTRY_TYPEDEF(KEY_NAME, VALUE_NAME) { \
    KEY_TYPE key;               \
    VALUE_TYPE value;           \
    bool isDeleted;             \
    bool isEmptySlot;           \
} HASH_MAP_ENTRY_TYPEDEF(KEY_NAME, VALUE_NAME); \
\
typedef struct HASH_MAP_TYPEDEF(KEY_NAME, VALUE_NAME) { \
    HASH_MAP_ENTRY_TYPEDEF(KEY_NAME, VALUE_NAME) *entries; \
    uint32_t size;                  \
    uint32_t capacity;              \
    uint32_t deletedItemsCount;     \
} HASH_MAP_TYPEDEF(KEY_NAME, VALUE_NAME); \
\
typedef struct HASH_MAP_ITERATOR_TYPEDEF(KEY_NAME, VALUE_NAME) {    \
    KEY_TYPE key;                                                   \
    VALUE_TYPE value;                                               \
    HASH_MAP_TYPEDEF(KEY_NAME, VALUE_NAME) *map;                    \
    uint32_t index;                                                 \
} HASH_MAP_ITERATOR_TYPEDEF(KEY_NAME, VALUE_NAME);                  \
                                                                    \
static inline HASH_MAP_TYPEDEF(KEY_NAME, VALUE_NAME) * HASH_MAP_METHOD(new, KEY_NAME, VALUE_NAME, BufferMap)(HASH_MAP_TYPEDEF(KEY_NAME, VALUE_NAME) *map, HASH_MAP_ENTRY_TYPEDEF(KEY_NAME, VALUE_NAME) *entries, uint32_t capacity) { \
    if (map == NULL) return NULL;       \
    map->entries = entries;             \
    map->size = 0;                      \
    map->capacity = capacity;           \
    map->deletedItemsCount = 0;         \
                                        \
    for (uint32_t i = 0; i < capacity; i++) {   \
        HASH_MAP_ENTRY_TYPEDEF(KEY_NAME, VALUE_NAME) *entry = &entries[i]; \
        entry->key = (KEY_TYPE) {0};                \
        entry->value = (VALUE_TYPE) {0};            \
        entry->isDeleted = false;                   \
        entry->isEmptySlot = true;                  \
    }                                               \
    return map;                                     \
}                                                   \
\
static inline HASH_MAP_ENTRY_TYPEDEF(KEY_NAME, VALUE_NAME) * HASH_MAP_METHOD(find, KEY_NAME, VALUE_NAME, MapEntry)(HASH_MAP_TYPEDEF(KEY_NAME, VALUE_NAME) *map, KEY_TYPE key) { \
    uint32_t hash = HASH_FUN(key);                              \
    uint32_t index = hash & (map->capacity - 1);                \
    HASH_MAP_ENTRY_TYPEDEF(KEY_NAME, VALUE_NAME) *tombstone = NULL;     \
                                                                \
    while (true) {                                              \
        HASH_MAP_ENTRY_TYPEDEF(KEY_NAME, VALUE_NAME) *entry = &map->entries[index]; \
        if (entry->isEmptySlot) {                               \
            if (!entry->isDeleted) {                            \
                return tombstone != NULL ? tombstone : entry;   \
            } else {                                            \
                if (tombstone == NULL) {                        \
                    tombstone = entry;                          \
                }                                               \
            }                                                   \
        } else if (COMPARE_FUN(key, entry->key) == 0) {         \
            return entry;                                       \
        }                                                       \
        index = (index + 1) & (map->capacity - 1);              \
    }                                                           \
}                                                               \
\
static inline bool HASH_MAP_METHOD(KEY_NAME, VALUE_NAME, MapAdd)(HASH_MAP_TYPEDEF(KEY_NAME, VALUE_NAME) *map, KEY_TYPE key, VALUE_TYPE value) {   \
    if (map != NULL && (map->size < (map->capacity / HASH_MAP_EXPAND_FACTOR))) {                           \
        HASH_MAP_ENTRY_TYPEDEF(KEY_NAME, VALUE_NAME) *entry = HASH_MAP_METHOD(find, KEY_NAME, VALUE_NAME, MapEntry)(map, key);                                                                                                 \
        bool alreadyExist = !entry->isEmptySlot; \
        if (!alreadyExist) {           \
            map->size++;                    \
        }                                   \
                                            \
        if (entry->isDeleted) {             \
            map->deletedItemsCount--;       \
        }                                   \
        entry->key = key;                   \
        entry->value = value;               \
        entry->isDeleted = false;           \
        entry->isEmptySlot = false;         \
        return !alreadyExist;               \
    }                                       \
    return false;                           \
}                                           \
\
static inline HASH_MAP_TYPEDEF(KEY_NAME, VALUE_NAME) * HASH_MAP_METHOD(new, KEY_NAME, VALUE_NAME, BufferMapOf)(HASH_MAP_TYPEDEF(KEY_NAME, VALUE_NAME) *map, HASH_MAP_ENTRY_TYPEDEF(KEY_NAME, VALUE_NAME) *entries, uint32_t capacity, uint32_t size) { \
    if (map == NULL || entries == NULL) return NULL;                                                \
    HASH_MAP_ENTRY_TYPEDEF(KEY_NAME, VALUE_NAME) tmpEntries[size];                                  \
    memcpy(tmpEntries, entries, sizeof(tmpEntries));                                                \
    map = HASH_MAP_METHOD(new, KEY_NAME, VALUE_NAME, BufferMap)(map, entries, capacity);            \
    for (uint32_t i = 0; i < size; i++) {                                                           \
        HASH_MAP_METHOD(KEY_NAME, VALUE_NAME, MapAdd)(map, tmpEntries[i].key, tmpEntries[i].value); \
    }                                                                                               \
    return map;                                                                                     \
}                                                                                                   \
\
static inline uint32_t HASH_MAP_METHOD(KEY_NAME, VALUE_NAME, MapSize)(HASH_MAP_TYPEDEF(KEY_NAME, VALUE_NAME) *map) {  \
    return map != NULL ? map->size : 0; \
}   \
\
static inline bool HASH_MAP_METHOD(is, KEY_NAME, VALUE_NAME, MapEmpty)(HASH_MAP_TYPEDEF(KEY_NAME, VALUE_NAME) *map) {  \
    return map != NULL ? map->size == 0 : true; \
}   \
\
static inline bool HASH_MAP_METHOD(is, KEY_NAME, VALUE_NAME, MapNotEmpty)(HASH_MAP_TYPEDEF(KEY_NAME, VALUE_NAME) *map) {   \
    return !HASH_MAP_METHOD(is, KEY_NAME, VALUE_NAME, MapEmpty)(map);        \
}   \
\
static inline bool HASH_MAP_METHOD(KEY_NAME, VALUE_NAME, MapContains)(HASH_MAP_TYPEDEF(KEY_NAME, VALUE_NAME) *map, KEY_TYPE key) {                                                                                                         \
    HASH_MAP_ENTRY_TYPEDEF(KEY_NAME, VALUE_NAME) *entry = HASH_MAP_METHOD(find, KEY_NAME, VALUE_NAME, MapEntry)(map, key); \
    return !entry->isEmptySlot;     \
}  \
\
static inline VALUE_TYPE HASH_MAP_METHOD(KEY_NAME, VALUE_NAME, MapGet)(HASH_MAP_TYPEDEF(KEY_NAME, VALUE_NAME) *map, KEY_TYPE key) {    \
    if (HASH_MAP_METHOD(is, KEY_NAME, VALUE_NAME, MapNotEmpty)(map)) {                                                          \
        HASH_MAP_ENTRY_TYPEDEF(KEY_NAME, VALUE_NAME) *entry = HASH_MAP_METHOD(find, KEY_NAME, VALUE_NAME, MapEntry)(map, key);  \
        return !entry->isEmptySlot ? entry->value : (VALUE_TYPE) {0};       \
    }                               \
    return (VALUE_TYPE) {0};        \
}                                   \
\
static inline VALUE_TYPE HASH_MAP_METHOD(KEY_NAME, VALUE_NAME, MapGetOrDefault)(HASH_MAP_TYPEDEF(KEY_NAME, VALUE_NAME) *map, KEY_TYPE key, VALUE_TYPE defaultValue) {                                                                 \
    if (HASH_MAP_METHOD(is, KEY_NAME, VALUE_NAME, MapNotEmpty)(map)) { \
        HASH_MAP_ENTRY_TYPEDEF(KEY_NAME, VALUE_NAME) *entry = HASH_MAP_METHOD(find, KEY_NAME, VALUE_NAME, MapEntry)(map, key); \
        return !entry->isEmptySlot ? entry->value : defaultValue;      \
    }                               \
    return (VALUE_TYPE) {0};        \
}                                   \
\
static inline VALUE_TYPE HASH_MAP_METHOD(KEY_NAME, VALUE_NAME, MapRemove)(HASH_MAP_TYPEDEF(KEY_NAME, VALUE_NAME) *map, KEY_TYPE key) { \
    if (HASH_MAP_METHOD(is, KEY_NAME, VALUE_NAME, MapNotEmpty)(map)) {                                                          \
        HASH_MAP_ENTRY_TYPEDEF(KEY_NAME, VALUE_NAME) *entry = HASH_MAP_METHOD(find, KEY_NAME, VALUE_NAME, MapEntry)(map, key);  \
        if (entry->isEmptySlot) return (VALUE_TYPE) {0};                 \
        VALUE_TYPE value = entry->value;    \
        entry->key = (KEY_TYPE) {0};        \
        entry->value = (VALUE_TYPE) {0};    \
        entry->isDeleted = true;            \
        entry->isEmptySlot = true;          \
        map->size--;                        \
        map->deletedItemsCount++;           \
        return value;                       \
    }                                       \
    return (VALUE_TYPE) {0};                \
}                                           \
\
static inline void HASH_MAP_METHOD(KEY_NAME, VALUE_NAME, MapAddAll)(HASH_MAP_TYPEDEF(KEY_NAME, VALUE_NAME) *fromMap, HASH_MAP_TYPEDEF(KEY_NAME, VALUE_NAME) *toMap) {  \
    for (uint32_t i = 0; i < fromMap->capacity; i++) {                                        \
        HASH_MAP_ENTRY_TYPEDEF(KEY_NAME, VALUE_NAME) *entry = &fromMap->entries[i];           \
        if (!entry->isEmptySlot) {                                                            \
            HASH_MAP_METHOD(KEY_NAME, VALUE_NAME, MapAdd)(toMap, entry->key, entry->value);   \
        }   \
    }       \
}           \
\
static inline void HASH_MAP_METHOD(KEY_NAME, VALUE_NAME, MapClear)(HASH_MAP_TYPEDEF(KEY_NAME, VALUE_NAME) *map) {    \
    if (map != NULL && map->entries != NULL) {          \
        for (uint32_t i = 0; i < map->capacity; i++) {  \
            map->entries[i].key = (KEY_TYPE) {0};       \
            map->entries[i].value = (VALUE_TYPE) {0};   \
            map->entries[i].isDeleted = false;          \
            map->entries[i].isEmptySlot = true;         \
        }                                               \
        map->size = 0;                                  \
        map->deletedItemsCount = 0;                     \
    }                                                   \
}                                                       \
\
static inline HASH_MAP_ITERATOR_TYPEDEF(KEY_NAME, VALUE_NAME) HASH_MAP_METHOD(KEY_NAME, VALUE_NAME, MapIter)(HASH_MAP_TYPEDEF(KEY_NAME, VALUE_NAME) *map) { \
    HASH_MAP_ITERATOR_TYPEDEF(KEY_NAME, VALUE_NAME) iterator = {.map = map, .index = 0};    \
    return iterator;    \
}                       \
\
static inline bool HASH_MAP_METHOD(KEY_NAME, VALUE_NAME, MapHasNext)(HASH_MAP_ITERATOR_TYPEDEF(KEY_NAME, VALUE_NAME) *iterator) { \
    if (iterator != NULL && iterator->map != NULL) {        \
        HASH_MAP_TYPEDEF(KEY_NAME, VALUE_NAME) *map = iterator->map;  \
        while (iterator->index < map->capacity) {           \
            uint32_t indexValue = iterator->index;          \
            iterator->index++;                              \
                                                            \
            HASH_MAP_ENTRY_TYPEDEF(KEY_NAME, VALUE_NAME) *pair = &map->entries[indexValue];\
            if (!pair->isEmptySlot) { \
                iterator->key = pair->key;                   \
                iterator->value = pair->value;               \
                return true;                                 \
            }                       \
        }                           \
        return false;               \
    }                               \
    return false;                   \
}\



#define CREATE_HASH_MAP_TYPE_1(KEY_TYPE, VALUE_TYPE) CREATE_HASH_MAP_TYPE_NAME(KEY_TYPE, VALUE_TYPE, KEY_TYPE, VALUE_TYPE, COMPARATOR_FOR_TYPE(KEY_TYPE), HASH_CODE_FOR_TYPE(KEY_TYPE))
#define CREATE_HASH_MAP_TYPE_2(KEY_TYPE, VALUE_TYPE, KEY_NAME) CREATE_HASH_MAP_TYPE_NAME(KEY_TYPE, VALUE_TYPE, KEY_NAME, VALUE_TYPE, COMPARATOR_FOR_TYPE(KEY_TYPE), HASH_CODE_FOR_TYPE(KEY_TYPE))
#define CREATE_HASH_MAP_TYPE_3(KEY_TYPE, VALUE_TYPE, KEY_NAME, VALUE_NAME) CREATE_HASH_MAP_TYPE_NAME(KEY_TYPE, VALUE_TYPE, KEY_NAME, VALUE_NAME, COMPARATOR_FOR_TYPE(KEY_TYPE), HASH_CODE_FOR_TYPE(KEY_TYPE))
#define CREATE_HASH_MAP_TYPE_4(KEY_TYPE, VALUE_TYPE, KEY_NAME, COMPARE_FUN, HASH_FUN) CREATE_HASH_MAP_TYPE_NAME(KEY_TYPE, VALUE_TYPE, KEY_NAME, VALUE_TYPE, COMPARE_FUN, HASH_FUN)
#define CREATE_HASH_MAP_TYPE_5(KEY_TYPE, VALUE_TYPE, KEY_NAME, VALUE_NAME, COMPARE_FUN, HASH_FUN) CREATE_HASH_MAP_TYPE_NAME(KEY_TYPE, VALUE_TYPE, KEY_NAME, VALUE_NAME, COMPARE_FUN, HASH_FUN)
#define CREATE_HASH_MAP_TYPE_MACRO(_1, _2, _3, _4, _5, _6, FUN, ...) FUN

#define CREATE_HASH_MAP_TYPE(...)                                     \
    CREATE_HASH_MAP_TYPE_MACRO(__VA_ARGS__,                           \
                        CREATE_HASH_MAP_TYPE_5,                       \
                        CREATE_HASH_MAP_TYPE_4,                       \
                        CREATE_HASH_MAP_TYPE_3,                       \
                        CREATE_HASH_MAP_TYPE_2,                       \
                        CREATE_HASH_MAP_TYPE_1,                       \
                        ERROR)(__VA_ARGS__)


#define NEW_HASH_MAP_3(KEY_TYPE, VALUE_TYPE, KEY_NAME, VALUE_NAME, CAPACITY) \
HASH_MAP_METHOD(new, KEY_NAME, VALUE_NAME, BufferMap)(&(HASH_MAP_TYPEDEF(KEY_NAME, VALUE_NAME)){0}, \
                                                       (HASH_MAP_ENTRY_TYPEDEF(KEY_NAME, VALUE_NAME) [HASH_MAP_ALIGN_CAPACITY(CAPACITY)]){0}, \
                                                        HASH_MAP_ALIGN_CAPACITY(CAPACITY))
#define NEW_HASH_MAP_2(KEY_TYPE, VALUE_TYPE, KEY_NAME, CAPACITY) NEW_HASH_MAP_3(KEY_TYPE, VALUE_TYPE, KEY_NAME, VALUE_TYPE, CAPACITY)
#define NEW_HASH_MAP_1(KEY_TYPE, VALUE_TYPE, CAPACITY) NEW_HASH_MAP_3(KEY_TYPE, VALUE_TYPE, KEY_TYPE, VALUE_TYPE, CAPACITY)
#define NEW_HASH_MAP_MACRO(_1, _2, _3, _4, _5, FUN, ...) FUN

#define NEW_HASH_MAP(...)                                     \
    NEW_HASH_MAP_MACRO(__VA_ARGS__,                           \
                        NEW_HASH_MAP_3,                       \
                        NEW_HASH_MAP_2,                       \
                        NEW_HASH_MAP_1,                       \
                        ERROR)(__VA_ARGS__)

#define NEW_HASH_MAP_4(...)    NEW_HASH_MAP(__VA_ARGS__, 4)
#define NEW_HASH_MAP_8(...)    NEW_HASH_MAP(__VA_ARGS__, 8)
#define NEW_HASH_MAP_16(...)   NEW_HASH_MAP(__VA_ARGS__, 16)
#define NEW_HASH_MAP_32(...)   NEW_HASH_MAP(__VA_ARGS__, 32)
#define NEW_HASH_MAP_64(...)   NEW_HASH_MAP(__VA_ARGS__, 64)
#define NEW_HASH_MAP_128(...)  NEW_HASH_MAP(__VA_ARGS__, 128)
#define NEW_HASH_MAP_256(...)  NEW_HASH_MAP(__VA_ARGS__, 256)
#define NEW_HASH_MAP_512(...)  NEW_HASH_MAP(__VA_ARGS__, 512)
#define NEW_HASH_MAP_1024(...) NEW_HASH_MAP(__VA_ARGS__, 1024)


#define MAP_ENTRY(KEY, VALUE) {(KEY), (VALUE), false, true}

#define NEW_HASH_MAP_OF(CAPACITY, KEY_NAME, VALUE_NAME, ...) \
HASH_MAP_METHOD(new, KEY_NAME, VALUE_NAME, BufferMapOf)(&(HASH_MAP_TYPEDEF(KEY_NAME, VALUE_NAME)){0}, \
                                                         (HASH_MAP_ENTRY_TYPEDEF(KEY_NAME, VALUE_NAME) [HASH_MAP_ALIGN_CAPACITY(CAPACITY)]){__VA_ARGS__}, \
                                                          HASH_MAP_ALIGN_CAPACITY(CAPACITY),      \
                                                          VAR_ARGS_LENGTH(HASH_MAP_ENTRY_TYPEDEF(KEY_NAME, VALUE_NAME), __VA_ARGS__))

#define CREATE_HASH_MAP_1(KEY_NAME, VALUE_NAME, K1) \
        NEW_HASH_MAP_OF(1, KEY_NAME, VALUE_NAME, MAP_ENTRY(K1, K1))

#define CREATE_HASH_MAP_2(KEY_NAME, VALUE_NAME, K1, V1) \
        NEW_HASH_MAP_OF(1, KEY_NAME, VALUE_NAME, MAP_ENTRY(K1, V1))

#define CREATE_HASH_MAP_3(KEY_NAME, VALUE_NAME, K1, V1, K2) \
        NEW_HASH_MAP_OF(2, KEY_NAME, VALUE_NAME, MAP_ENTRY(K1, V1), MAP_ENTRY(K2, K2))

#define CREATE_HASH_MAP_4(KEY_NAME, VALUE_NAME, K1, V1, K2, V2) \
        NEW_HASH_MAP_OF(2, KEY_NAME, VALUE_NAME, MAP_ENTRY(K1, V1), MAP_ENTRY(K2, V2))

#define CREATE_HASH_MAP_5(KEY_NAME, VALUE_NAME, K1, V1, K2, V2, K3) \
        NEW_HASH_MAP_OF(3, KEY_NAME, VALUE_NAME, MAP_ENTRY(K1, V1), MAP_ENTRY(K2, V2), MAP_ENTRY(K3, K3))

#define CREATE_HASH_MAP_6(KEY_NAME, VALUE_NAME, K1, V1, K2, V2, K3, V3) \
        NEW_HASH_MAP_OF(3, KEY_NAME, VALUE_NAME, MAP_ENTRY(K1, V1), MAP_ENTRY(K2, V2), MAP_ENTRY(K3, V3))

#define CREATE_HASH_MAP_7(KEY_NAME, VALUE_NAME, K1, V1, K2, V2, K3, V3, K4) \
        NEW_HASH_MAP_OF(4, KEY_NAME, VALUE_NAME, MAP_ENTRY(K1, V1), MAP_ENTRY(K2, V2), MAP_ENTRY(K3, V3), MAP_ENTRY(K4, K4))

#define CREATE_HASH_MAP_8(KEY_NAME, VALUE_NAME, K1, V1, K2, V2, K3, V3, K4, V4) \
        NEW_HASH_MAP_OF(4, KEY_NAME, VALUE_NAME, MAP_ENTRY(K1, V1), MAP_ENTRY(K2, V2), MAP_ENTRY(K3, V3), MAP_ENTRY(K4, V4))

#define CREATE_HASH_MAP_9(KEY_NAME, VALUE_NAME, K1, V1, K2, V2, K3, V3, K4, V4, K5) \
        NEW_HASH_MAP_OF(5, KEY_NAME, VALUE_NAME, MAP_ENTRY(K1, V1), MAP_ENTRY(K2, V2), MAP_ENTRY(K3, V3), MAP_ENTRY(K4, V4), MAP_ENTRY(K5, K5))

#define CREATE_HASH_MAP_10(KEY_NAME, VALUE_NAME, K1, V1, K2, V2, K3, V3, K4, V4, K5, V5) \
        NEW_HASH_MAP_OF(5, KEY_NAME, VALUE_NAME, MAP_ENTRY(K1, V1), MAP_ENTRY(K2, V2), MAP_ENTRY(K3, V3), MAP_ENTRY(K4, V4), MAP_ENTRY(K5, V5))

#define CREATE_HASH_MAP_11(KEY_NAME, VALUE_NAME, K1, V1, K2, V2, K3, V3, K4, V4, K5, V5, K6) \
        NEW_HASH_MAP_OF(6, KEY_NAME, VALUE_NAME, MAP_ENTRY(K1, V1), MAP_ENTRY(K2, V2), MAP_ENTRY(K3, V3), MAP_ENTRY(K4, V4), MAP_ENTRY(K5, V5), MAP_ENTRY(K6, K6))

#define CREATE_HASH_MAP_12(KEY_NAME, VALUE_NAME, K1, V1, K2, V2, K3, V3, K4, V4, K5, V5, K6, V6) \
        NEW_HASH_MAP_OF(6, KEY_NAME, VALUE_NAME, MAP_ENTRY(K1, V1), MAP_ENTRY(K2, V2), MAP_ENTRY(K3, V3), MAP_ENTRY(K4, V4), MAP_ENTRY(K5, V5), MAP_ENTRY(K6, V6))

#define CREATE_HASH_MAP_13(KEY_NAME, VALUE_NAME, K1, V1, K2, V2, K3, V3, K4, V4, K5, V5, K6, V6, K7) \
        NEW_HASH_MAP_OF(7, KEY_NAME, VALUE_NAME, MAP_ENTRY(K1, V1), MAP_ENTRY(K2, V2), MAP_ENTRY(K3, V3), MAP_ENTRY(K4, V4), MAP_ENTRY(K5, V5), MAP_ENTRY(K6, V6), MAP_ENTRY(K7, K7))

#define CREATE_HASH_MAP_14(KEY_NAME, VALUE_NAME, K1, V1, K2, V2, K3, V3, K4, V4, K5, V5, K6, V6, K7, V7) \
        NEW_HASH_MAP_OF(7, KEY_NAME, VALUE_NAME, MAP_ENTRY(K1, V1), MAP_ENTRY(K2, V2), MAP_ENTRY(K3, V3), MAP_ENTRY(K4, V4), MAP_ENTRY(K5, V5), MAP_ENTRY(K6, V6), MAP_ENTRY(K7, V7))

#define CREATE_HASH_MAP_15(KEY_NAME, VALUE_NAME, K1, V1, K2, V2, K3, V3, K4, V4, K5, V5, K6, V6, K7, V7, K8) \
        NEW_HASH_MAP_OF(8, KEY_NAME, VALUE_NAME, MAP_ENTRY(K1, V1), MAP_ENTRY(K2, V2), MAP_ENTRY(K3, V3), MAP_ENTRY(K4, V4), MAP_ENTRY(K5, V5), MAP_ENTRY(K6, V6), MAP_ENTRY(K7, V7), MAP_ENTRY(K8, K8))

#define CREATE_HASH_MAP_16(KEY_NAME, VALUE_NAME, K1, V1, K2, V2, K3, V3, K4, V4, K5, V5, K6, V6, K7, V7, K8, V8) \
        NEW_HASH_MAP_OF(8, KEY_NAME, VALUE_NAME, MAP_ENTRY(K1, V1), MAP_ENTRY(K2, V2), MAP_ENTRY(K3, V3), MAP_ENTRY(K4, V4), MAP_ENTRY(K5, V5), MAP_ENTRY(K6, V6), MAP_ENTRY(K7, V7), MAP_ENTRY(K8, V8))

#define CREATE_HASH_MAP_17(KEY_NAME, VALUE_NAME, K1, V1, K2, V2, K3, V3, K4, V4, K5, V5, K6, V6, K7, V7, K8, V8, K9) \
        NEW_HASH_MAP_OF(9, KEY_NAME, VALUE_NAME, MAP_ENTRY(K1, V1), MAP_ENTRY(K2, V2), MAP_ENTRY(K3, V3), MAP_ENTRY(K4, V4), MAP_ENTRY(K5, V5), MAP_ENTRY(K6, V6), MAP_ENTRY(K7, V7), MAP_ENTRY(K8, V8), MAP_ENTRY(K9, K9))

#define CREATE_HASH_MAP_18(KEY_NAME, VALUE_NAME, K1, V1, K2, V2, K3, V3, K4, V4, K5, V5, K6, V6, K7, V7, K8, V8, K9, V9) \
        NEW_HASH_MAP_OF(9, KEY_NAME, VALUE_NAME, MAP_ENTRY(K1, V1), MAP_ENTRY(K2, V2), MAP_ENTRY(K3, V3), MAP_ENTRY(K4, V4), MAP_ENTRY(K5, V5), MAP_ENTRY(K6, V6), MAP_ENTRY(K7, V7), MAP_ENTRY(K8, V8), MAP_ENTRY(K9, V9))

#define CREATE_HASH_MAP_19(KEY_NAME, VALUE_NAME, K1, V1, K2, V2, K3, V3, K4, V4, K5, V5, K6, V6, K7, V7, K8, V8, K9, V9, K10) \
        NEW_HASH_MAP_OF(10, KEY_NAME, VALUE_NAME, MAP_ENTRY(K1, V1), MAP_ENTRY(K2, V2), MAP_ENTRY(K3, V3), MAP_ENTRY(K4, V4), MAP_ENTRY(K5, V5), MAP_ENTRY(K6, V6), MAP_ENTRY(K7, V7), MAP_ENTRY(K8, V8), MAP_ENTRY(K9, V9), MAP_ENTRY(K10, K10))

#define CREATE_HASH_MAP_20(KEY_NAME, VALUE_NAME, K1, V1, K2, V2, K3, V3, K4, V4, K5, V5, K6, V6, K7, V7, K8, V8, K9, V9, K10, V10) \
        NEW_HASH_MAP_OF(10, KEY_NAME, VALUE_NAME, MAP_ENTRY(K1, V1), MAP_ENTRY(K2, V2), MAP_ENTRY(K3, V3), MAP_ENTRY(K4, V4), MAP_ENTRY(K5, V5), MAP_ENTRY(K6, V6), MAP_ENTRY(K7, V7), MAP_ENTRY(K8, V8), MAP_ENTRY(K9, V9), MAP_ENTRY(K10, V10))

#define CREATE_HASH_MAP_21(KEY_NAME, VALUE_NAME, K1, V1, K2, V2, K3, V3, K4, V4, K5, V5, K6, V6, K7, V7, K8, V8, K9, V9, K10, V10, K11) \
        NEW_HASH_MAP_OF(11, KEY_NAME, VALUE_NAME, MAP_ENTRY(K1, V1), MAP_ENTRY(K2, V2), MAP_ENTRY(K3, V3), MAP_ENTRY(K4, V4), MAP_ENTRY(K5, V5), MAP_ENTRY(K6, V6), MAP_ENTRY(K7, V7), MAP_ENTRY(K8, V8), MAP_ENTRY(K9, V9), MAP_ENTRY(K10, V10), MAP_ENTRY(K11, K11))

#define CREATE_HASH_MAP_22(KEY_NAME, VALUE_NAME, K1, V1, K2, V2, K3, V3, K4, V4, K5, V5, K6, V6, K7, V7, K8, V8, K9, V9, K10, V10, K11, V11) \
        NEW_HASH_MAP_OF(11, KEY_NAME, VALUE_NAME, MAP_ENTRY(K1, V1), MAP_ENTRY(K2, V2), MAP_ENTRY(K3, V3), MAP_ENTRY(K4, V4), MAP_ENTRY(K5, V5), MAP_ENTRY(K6, V6), MAP_ENTRY(K7, V7), MAP_ENTRY(K8, V8), MAP_ENTRY(K9, V9), MAP_ENTRY(K10, V10), MAP_ENTRY(K11, V11))

#define CREATE_HASH_MAP_23(KEY_NAME, VALUE_NAME, K1, V1, K2, V2, K3, V3, K4, V4, K5, V5, K6, V6, K7, V7, K8, V8, K9, V9, K10, V10, K11, V11, K12) \
        NEW_HASH_MAP_OF(12, KEY_NAME, VALUE_NAME, MAP_ENTRY(K1, V1), MAP_ENTRY(K2, V2), MAP_ENTRY(K3, V3), MAP_ENTRY(K4, V4), MAP_ENTRY(K5, V5), MAP_ENTRY(K6, V6), MAP_ENTRY(K7, V7), MAP_ENTRY(K8, V8), MAP_ENTRY(K9, V9), MAP_ENTRY(K10, V10), MAP_ENTRY(K11, V11), MAP_ENTRY(K12, K12))

#define CREATE_HASH_MAP_24(KEY_NAME, VALUE_NAME, K1, V1, K2, V2, K3, V3, K4, V4, K5, V5, K6, V6, K7, V7, K8, V8, K9, V9, K10, V10, K11, V11, K12, V12) \
        NEW_HASH_MAP_OF(12, KEY_NAME, VALUE_NAME, MAP_ENTRY(K1, V1), MAP_ENTRY(K2, V2), MAP_ENTRY(K3, V3), MAP_ENTRY(K4, V4), MAP_ENTRY(K5, V5), MAP_ENTRY(K6, V6), MAP_ENTRY(K7, V7), MAP_ENTRY(K8, V8), MAP_ENTRY(K9, V9), MAP_ENTRY(K10, V10), MAP_ENTRY(K11, V11), MAP_ENTRY(K12, V12))

#define GET_CREATE_HASH_MAP_MACRO(KEY_NAME, VALUE_NAME, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, FUN, ...) FUN

#define HASH_MAP_OF(KEY_NAME, VALUE_NAME, ...)                          \
    GET_CREATE_HASH_MAP_MACRO(KEY_NAME, VALUE_NAME, __VA_ARGS__,        \
                        CREATE_HASH_MAP_24,                   \
                        CREATE_HASH_MAP_23,                   \
                        CREATE_HASH_MAP_22,                   \
                        CREATE_HASH_MAP_21,                   \
                        CREATE_HASH_MAP_20,                   \
                        CREATE_HASH_MAP_19,                   \
                        CREATE_HASH_MAP_18,                   \
                        CREATE_HASH_MAP_17,                   \
                        CREATE_HASH_MAP_16,                   \
                        CREATE_HASH_MAP_15,                   \
                        CREATE_HASH_MAP_14,                   \
                        CREATE_HASH_MAP_13,                   \
                        CREATE_HASH_MAP_12,                   \
                        CREATE_HASH_MAP_11,                   \
                        CREATE_HASH_MAP_10,                   \
                        CREATE_HASH_MAP_9,                   \
                        CREATE_HASH_MAP_8,                   \
                        CREATE_HASH_MAP_7,                   \
                        CREATE_HASH_MAP_6,                   \
                        CREATE_HASH_MAP_5,                   \
                        CREATE_HASH_MAP_4,                   \
                        CREATE_HASH_MAP_3,                   \
                        CREATE_HASH_MAP_2,                   \
                        CREATE_HASH_MAP_1,                   \
                        ERROR)(KEY_NAME, VALUE_NAME, __VA_ARGS__)
