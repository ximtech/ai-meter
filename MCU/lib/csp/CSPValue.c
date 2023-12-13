#include "CSPValue.h"

#define CSP_CONCAT_STRINGS(dest, one, two) (strcat(strcpy(dest, (one)), (two)))

static CspObjectString *allocateStringObject(uint16_t length, bool isConstant);
static char *copyStringValue(const char *src, uint16_t length);
static bool doubleCspValVecCapacity(CspValVector *vector);
static bool adjustCspValVecCapacity(CspValVector *vector, uint8_t newCapacity);

static CspMapEntry *findCspEntry(CspMapEntry *entries, uint32_t capacity, const char *key);
static uint32_t nextPowerOfTwo(uint32_t capacity);
static uint32_t hashCspCode(const char *key);
static bool adjustCspHashMapCapacity(CspHashMap *hashMap, uint32_t capacity);


CspObjectString *newCspStringObject(const char *strValue, bool isConstant) {
    uint16_t length = strlen(strValue);
    CspObjectString *object = allocateStringObject(length, isConstant);
    if (object == NULL) return NULL;
    strncpy(object->chars, strValue, length);
    return object;
}

CspObjectString *newCspStringObjectConcat(const char *one, const char *two, uint16_t totalLength) {
    CspObjectString *object = allocateStringObject(totalLength, false);
    if (object == NULL) return NULL;
    CSP_CONCAT_STRINGS(object->chars, one, two);
    return object;
}

char *newCspVarObject(char *name) {
    return copyStringValue(name, strlen(name));
}

CspObjectArray *newCspArrayObject(uint32_t initCapacity, bool isConstant) {
    CspObjectArray *object = malloc(sizeof(struct CspObjectArray));
    if (object == NULL) return NULL;
    object->object.type = CSP_OBJ_ARRAY;
    object->vec = newCspValVec(initCapacity);
    if (object->vec == NULL) {
        free(object);
        return NULL;
    }

    if (!isConstant) {
        object->object.next = createdObjects;
        createdObjects = (CspObject *) object;
    }
    return object;
}

CspObjectMap *newCspMapObject(uint32_t initCapacity, bool isConstant) {
    CspObjectMap *object = malloc(sizeof(struct CspObjectMap));
    if (object == NULL) return NULL;
    object->object.type = CSP_OBJ_MAP;
    object->map = newCspHashMap(initCapacity);
    if (object->map == NULL) {
        free(object);
        return NULL;
    }

    if (!isConstant) {
        object->object.next = createdObjects;
        createdObjects = (CspObject *) object;
    }
    return object;
}

CspHashMap *newCspHashMap(uint32_t capacity) {
    CspHashMap *hashMapInstance = malloc(sizeof(struct CspHashMap));
    if (hashMapInstance == NULL) return NULL;

    hashMapInstance->size = 0;
    hashMapInstance->capacity = nextPowerOfTwo(capacity);
    hashMapInstance->deletedItemsCount = 0;
    hashMapInstance->entries = calloc(hashMapInstance->capacity, sizeof(struct CspMapEntry));
    if (hashMapInstance->entries == NULL) {
        free(hashMapInstance);
        return NULL;
    }
    return hashMapInstance;
}

bool cspMapPut(CspHashMap *hashMap, const char *key, CspValue value) {
    if (hashMap != NULL && key != NULL) {
        if ((hashMap->size + hashMap->deletedItemsCount + 1) > (hashMap->capacity * 0.75)) {
            uint32_t newCapacity = (hashMap->capacity * 2);
            bool isMapCapacityChanged = adjustCspHashMapCapacity(hashMap, newCapacity);
            if (!isMapCapacityChanged) return false;
        }

        CspMapEntry *entry = findCspEntry(hashMap->entries, hashMap->capacity, key);
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

CspValue cspMapGet(CspHashMap *hashMap, const char *key) {
    CspMapEntry *entry = getCspValueMapEntry(hashMap, key);
    return entry != NULL ? entry->value : CSP_NULL_VALUE();
}

CspValue cspMapRemove(CspHashMap *hashMap, const char *key) {
    if (getCspMapSize(hashMap) != 0 && key != NULL) {
        CspMapEntry *entry = findCspEntry(hashMap->entries, hashMap->capacity, key);
        return cspMapRemoveEntry(hashMap, entry);
    }
    return CSP_NULL_VALUE();
}

CspValue cspMapRemoveEntry(CspHashMap *hashMap, CspMapEntry *entry) {
    if (entry == NULL || entry->key == NULL) {
        return CSP_NULL_VALUE();
    }

    entry->key = NULL;
    entry->isDeleted = true; // Place a tombstone in the entry.
    hashMap->size--;
    hashMap->deletedItemsCount++;
    return entry->value;
}

CspMapEntry *getCspValueMapEntry(CspHashMap *hashMap, const char *key) {
    if (getCspMapSize(hashMap) != 0 && key != NULL) {
        CspMapEntry *entry = findCspEntry(hashMap->entries, hashMap->capacity, key);
        return entry->key != NULL ? entry : NULL;
    }
    return NULL;
}

uint32_t getCspMapSize(CspHashMap *hashMap) {
    return hashMap != NULL ? hashMap->size : 0;
}

void cspMapDelete(CspHashMap *hashMap) {
    if (hashMap != NULL) {
        for (uint32_t i = 0; i < hashMap->capacity; i++) {
            CspMapEntry entry = hashMap->entries[i];
            if (entry.key != NULL) {
                deleteCspValue(entry.value);
            }
        }
        free(hashMap->entries);
        free(hashMap);
    }
}

CspValVector *newCspValVec(uint8_t capacity) {
    if (capacity < 1) return NULL;
    CspValVector *vector = malloc(sizeof(struct CspValVector));
    if (vector == NULL)return NULL;
    vector->size = 0;
    vector->capacity = capacity;
    vector->items = calloc(vector->capacity, sizeof(CspValue));
    if (vector->items == NULL) {
        free(vector->items);
        free(vector);
        return NULL;
    }
    return vector;
}

bool cspValVecAdd(CspValVector *vector, CspValue item) {
    if (vector != NULL) {
        if (vector->size >= vector->capacity) {
            if (!doubleCspValVecCapacity(vector)) {
                return false;
            }
        }
        vector->items[vector->size++] = item;
        return true;
    }
    return false;
}

CspValue cspValVecGet(CspValVector *vector, uint8_t index) {
    return (vector != NULL && index < vector->size) ? vector->items[index] : CSP_NULL_VALUE();
}

bool isCspValVecEmpty(CspValVector *vector) {
    return (vector == NULL) || (vector->size == 0);
}

uint8_t cspValVecSize(CspValVector *vector) {
    return vector != NULL ? vector->size : 0;
}

bool cspValVecFitToSize(CspValVector *vector) {
    if (vector != NULL) {
        if (isCspValVecEmpty(vector)) {
            cspValVecDelete(vector);
            return true;
        }

        if (vector->capacity > vector->size) {
            return adjustCspValVecCapacity(vector, vector->size);
        }
    }
    return true;
}

void freeCspObjects() {
    CspObject *object = createdObjects;
    while (object != NULL) {
        CspObject *next = object->next;
        freeCspObject(object);
        object = next;
    }
    createdObjects = NULL;
}

void freeCspObject(CspObject *object) {
    switch (object->type) {
        case CSP_OBJ_STRING: {
            CspObjectString *strObj = (CspObjectString *) object;
            free(strObj);
            break;
        }
        case CSP_OBJ_ARRAY: {
            CspObjectArray *arrayObj = (CspObjectArray *) object;
            cspValVecDelete(arrayObj->vec);
            free(arrayObj);
            break;
        }
        case CSP_OBJ_MAP: {
            CspObjectMap *mapObj = (CspObjectMap *) object;
            cspMapDelete(mapObj->map);
            free(mapObj);
            break;
        }
    }
}

void cspValVecDelete(CspValVector *vector) {
    if (vector != NULL) {
        for (int i = 0; i < cspValVecSize(vector); i++) {
            CspValue value = cspValVecGet(vector, i);
            deleteCspValue(value);
        }
        free(vector->items);
        free(vector);
    }
}

void deleteCspValue(CspValue value) {
    if (IS_CSP_OBJECT(value)) {
        freeCspObject(AS_CSP_OBJECT(value));

    } else if (IS_CSP_VARIABLE(value)) {
        free(AS_CSP_VAR_NAME(value));
    }
}

static CspObjectString *allocateStringObject(uint16_t length, bool isConstant) {
    CspObjectString *object = malloc(sizeof(struct CspObjectString) + (length * sizeof(char) + 1));
    if (object == NULL) return NULL;
    object->object.type = CSP_OBJ_STRING;
    object->chars[length] = '\0';
    object->length = length;

    if (!isConstant) {
        object->object.next = createdObjects;
        createdObjects = (CspObject *) object;
    }
    return object;
}

static char *copyStringValue(const char *src, uint16_t length) {
    if (length == 0) return NULL;
    char *heapChars = malloc(sizeof(char) * length + 1);
    if (heapChars == NULL) return NULL;
    strncpy(heapChars, src, length);
    heapChars[length] = '\0';
    return heapChars;
}

static bool doubleCspValVecCapacity(CspValVector *vector) {
    uint8_t newCapacity = vector->capacity * 2;
    if (newCapacity >= UINT8_MAX) return false;
    return adjustCspValVecCapacity(vector, newCapacity);
}

static bool adjustCspValVecCapacity(CspValVector *vector, uint8_t newCapacity) {
    if (newCapacity < vector->size) return false;
    CspValue *newItemArray = malloc(sizeof(CspValue) * newCapacity);
    if (newItemArray == NULL) return false;

    for (uint32_t i = 0; i < vector->size; i++) {
        newItemArray[i] = vector->items[i];
    }
    free(vector->items);
    vector->items = newItemArray;
    vector->capacity = newCapacity;
    return true;
}

static CspMapEntry *findCspEntry(CspMapEntry *entries, uint32_t capacity, const char *key) {
    uint32_t hash = hashCspCode(key);
    uint32_t index = hash & (capacity - 1);
    CspMapEntry *tombstone = NULL;

    while (true) {
        CspMapEntry *entry = &entries[index];
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

static uint32_t hashCspCode(const char *key) {  // Returns a hashCode code for the provided string.
    uint32_t hash = 2166136261u;
    uint32_t keyLength = strlen(key);
    for (uint32_t i = 0; i < keyLength; i++) {
        hash ^= (uint8_t) key[i];
        hash *= 16777619;
    }
    return hash;
}

static bool adjustCspHashMapCapacity(CspHashMap *hashMap, uint32_t capacity) {
    CspMapEntry *newEntries = calloc(capacity, sizeof(struct CspMapEntry));
    if (newEntries == NULL) return false;

    hashMap->size = 0;  // Don’t copy the tombstones over. Recalculate the count since it may change during a resize
    for (uint32_t i = 0; i < hashMap->capacity; i++) {
        CspMapEntry *entry = &hashMap->entries[i];
        if (entry->key == NULL) continue;

        CspMapEntry *destination = findCspEntry(newEntries, capacity, entry->key);
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
