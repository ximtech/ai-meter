#pragma once

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#ifndef HASH_MAP_LOAD_FACTOR
#define HASH_MAP_LOAD_FACTOR 0.75
#endif

typedef struct HashMap *HashMap;
typedef void* MapValueType; // Map can keep any type, change for specific

typedef struct MapEntry {
    char *key;  // key is NULL if this slot empty
    MapValueType value;
    bool isDeleted;
} MapEntry;

struct HashMap {
    MapEntry *entries;
    uint32_t size;
    uint32_t capacity;
    uint32_t deletedItemsCount;
};

typedef struct HashMapIterator {
    const char *key;
    MapValueType value;
    HashMap hashMap;
    uint32_t index;
} HashMapIterator;

HashMap getHashMapInstance(uint32_t capacity);

bool hashMapPut(HashMap hashMap, const char *key, MapValueType value);
MapValueType hashMapGet(HashMap hashMap, const char *key);
MapValueType hashMapGetOrDefault(HashMap hashMap, const char *key, MapValueType defaultValue);
MapEntry *hashMapGetEntry(HashMap hashMap, const char *key);

MapValueType hashMapRemove(HashMap hashMap, const char *key);
MapValueType hashMapRemoveEntry(HashMap hashMap, MapEntry *entry);

void hashMapAddAll(HashMap from, HashMap to);
void hashMapClear(HashMap hashMap);

bool isHashMapEmpty(HashMap hashMap);
bool isHashMapNotEmpty(HashMap hashMap);
bool isHashMapContainsKey(HashMap hashMap, const char *key);
uint32_t getHashMapSize(HashMap hashMap);

HashMapIterator getHashMapIterator(HashMap hashMap);
bool hashMapHasNext(HashMapIterator *iterator);

void hashMapDelete(HashMap hashMap);

void initSingletonHashMap(HashMap *hashMap, uint32_t capacity);