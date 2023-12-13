#include "HashMap.h"

static MapEntry *findEntry(MapEntry *entries, uint32_t capacity, const char *key);
static uint32_t nextPowerOfTwo(uint32_t capacity);
static uint32_t hashCode(const char *key);
static bool adjustHashMapCapacity(HashMap hashMap, uint32_t capacity);


HashMap getHashMapInstance(uint32_t capacity) {
    HashMap hashMapInstance = malloc(sizeof(struct HashMap));
    if (hashMapInstance == NULL) return NULL;

    hashMapInstance->size = 0;
    hashMapInstance->capacity = nextPowerOfTwo(capacity);
    hashMapInstance->deletedItemsCount = 0;
    hashMapInstance->entries = calloc(hashMapInstance->capacity, sizeof(MapEntry));
    if (hashMapInstance->entries == NULL) {
        free(hashMapInstance);
        return NULL;
    }
    return hashMapInstance;
}

bool hashMapPut(HashMap hashMap, const char *key, MapValueType value) {
    if (hashMap != NULL && key != NULL) {
        if ((hashMap->size + hashMap->deletedItemsCount + 1) > (hashMap->capacity * HASH_MAP_LOAD_FACTOR)) {
            uint32_t newCapacity = (hashMap->capacity * 2);
            bool isMapCapacityChanged = adjustHashMapCapacity(hashMap, newCapacity);
            if (!isMapCapacityChanged) return false;
        }

        MapEntry *entry = findEntry(hashMap->entries, hashMap->capacity, key);
        bool isNewKey = entry->key == NULL;
        if (isNewKey) {
            hashMap->size++;
        }

        if (entry->isDeleted) {
            hashMap->deletedItemsCount--;
        }
        entry->key = (char *) key;
        entry->value = value;
        entry->isDeleted = false;
        return true;
    }
    return false;
}

MapValueType hashMapGet(HashMap hashMap, const char *key) {
    MapEntry *entry = hashMapGetEntry(hashMap, key);
    return entry != NULL ? entry->value : (MapValueType) NULL;
}

MapValueType hashMapGetOrDefault(HashMap hashMap, const char *key, MapValueType defaultValue) {
    MapValueType mapValue = hashMapGet(hashMap, key);
    return mapValue != (MapValueType) NULL ? mapValue : defaultValue;
}

MapEntry *hashMapGetEntry(HashMap hashMap, const char *key) {
    if (isHashMapNotEmpty(hashMap) && key != NULL) {
        MapEntry *entry = findEntry(hashMap->entries, hashMap->capacity, key);
        return entry->key != NULL ? entry : NULL;
    }
    return NULL;
}

MapValueType hashMapRemove(HashMap hashMap, const char *key) {
    if (isHashMapNotEmpty(hashMap) && key != NULL) {
        MapEntry *entry = findEntry(hashMap->entries, hashMap->capacity, key);
        return hashMapRemoveEntry(hashMap, entry);
    }
    return (MapValueType) NULL;
}

MapValueType hashMapRemoveEntry(HashMap hashMap, MapEntry *entry) {
    if (entry == NULL || entry->key == NULL) {
        return (MapValueType) NULL;
    }

    entry->key = NULL;
    entry->isDeleted = true; // Place a tombstone in the entry.
    hashMap->size--;
    hashMap->deletedItemsCount++;
    return entry->value;
}

void hashMapAddAll(HashMap from, HashMap to) {
    for (uint32_t i = 0; i < from->capacity; i++) {
        MapEntry *entry = &from->entries[i];
        if (entry->key != NULL) {
            hashMapPut(to, entry->key, entry->value);
        }
    }
}

void hashMapClear(HashMap hashMap) {
    if (hashMap != NULL) {
        for (uint32_t i = 0; i < hashMap->capacity; i++) {
            hashMap->entries[i].key = NULL;
            hashMap->entries[i].isDeleted = false;
        }
        hashMap->size = 0;
        hashMap->deletedItemsCount = 0;
    }
}

uint32_t getHashMapSize(HashMap hashMap) {
    return hashMap != NULL ? hashMap->size : 0;
}

bool isHashMapEmpty(HashMap hashMap) {
    return hashMap != NULL ? hashMap->size == 0 : true;
}

bool isHashMapNotEmpty(HashMap hashMap) {
    return !isHashMapEmpty(hashMap);
}

bool isHashMapContainsKey(HashMap hashMap, const char *key) {
    if (key == NULL) return false;
    MapEntry *entry = findEntry(hashMap->entries, hashMap->capacity, key);
    return entry->key != NULL;
}

HashMapIterator getHashMapIterator(HashMap hashMap) {
    HashMapIterator iterator = {.hashMap = hashMap, .index = 0};
    return iterator;
}

bool hashMapHasNext(HashMapIterator *iterator) {
    if (iterator != NULL && iterator->hashMap != NULL) {
        HashMap hashMap = iterator->hashMap;
        while (iterator->index < hashMap->capacity) {
            uint32_t indexValue = iterator->index;
            iterator->index++;

            if (hashMap->entries[indexValue].key != NULL) { // Found next non-empty item, update iterator key and value.
                MapEntry pair = hashMap->entries[indexValue];
                iterator->key = pair.key;
                iterator->value = pair.value;
                return true;
            }
        }
        return false;
    }
    return false;
}

void hashMapDelete(HashMap hashMap) {
    if (hashMap != NULL) {
        free(hashMap->entries);
        free(hashMap);
    }
}

void initSingletonHashMap(HashMap *hashMap, uint32_t capacity) {
    if (*hashMap == NULL) {
        *hashMap = getHashMapInstance(capacity);
    }
}

static MapEntry *findEntry(MapEntry *entries, uint32_t capacity, const char *key) {
    uint32_t hash = hashCode(key);
    uint32_t index = hash & (capacity - 1);
    MapEntry *tombstone = NULL;

    while (true) {
        MapEntry *entry = &entries[index];
        if (entry->key == NULL) {
            if (!entry->isDeleted) { // Empty entry.
                return tombstone != NULL ? tombstone : entry;
            } else {
                if (tombstone == NULL) {
                    tombstone = entry;   // We found a tombstone.
                }
            }
        } else if (strcmp(key, entry->key) == 0) {   // If bucket has the same key, we’re done
            return entry;   // We found the key.
        }
        index = (index + 1) & (capacity - 1); // If we go past the end of the array, that second modulo operator wraps us back around to the beginning.
    }
}

static uint32_t nextPowerOfTwo(uint32_t capacity) {
    capacity--;
    uint32_t i = 0;
    for (; capacity > 0; capacity >>= 1) {
        i++;
    }
    return 1 << i;
}

static uint32_t hashCode(const char *key) {  // Returns a hashCode code for the provided string.
    uint32_t hash = 2166136261u;
    uint32_t keyLength = strlen(key);
    for (uint32_t i = 0; i < keyLength; i++) {
        hash ^= (uint8_t) key[i];
        hash *= 16777619;
    }
    return hash;
}

static bool adjustHashMapCapacity(HashMap hashMap, uint32_t capacity) {
    MapEntry *newEntries = calloc(capacity, sizeof(struct MapEntry));
    if (newEntries == NULL) return false;

    hashMap->size = 0;  // Don’t copy the tombstones over. Recalculate the count since it may change during a resize
    for (uint32_t i = 0; i < hashMap->capacity; i++) {
        MapEntry *entry = &hashMap->entries[i];
        if (entry->key == NULL) continue;

        MapEntry *destination = findEntry(newEntries, capacity, entry->key);
        destination->key = entry->key;
        destination->value = entry->value;
        hashMap->size++;
    }

    free(hashMap->entries);
    hashMap->entries = newEntries;
    hashMap->capacity = capacity;
    hashMap->deletedItemsCount = 0;
    return true;
}
