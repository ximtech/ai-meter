#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "CSPTokener.h"
#include "HeapVector.h"

#define IS_CSP_INT(value)        ((value).type == CSP_VAL_NUMBER_INT)
#define IS_CSP_FLOAT(value)      ((value).type == CSP_VAL_NUMBER_FLOAT)
#define IS_CSP_NULL(value)       ((value).type == CSP_VAL_NULL)
#define IS_CSP_BOOL_TRUE(value)  ((value).type == CSP_VAL_BOOL_TRUE)
#define IS_CSP_BOOL_FALSE(value) ((value).type == CSP_VAL_BOOL_FALSE)
#define IS_CSP_BOOL(value)       (IS_CSP_BOOL_TRUE(value) ||  IS_CSP_BOOL_FALSE(value))
#define IS_CSP_VARIABLE(value)   ((value).type == CSP_VAL_VARIABLE)
#define IS_CSP_OBJECT(value)     ((value).type == CSP_VAL_OBJECT)
#define IS_CSP_STRING(value)     isCspObjectType((value), CSP_OBJ_STRING)
#define IS_CSP_ARRAY(value)      isCspObjectType((value), CSP_OBJ_ARRAY)
#define IS_CSP_MAP(value)        isCspObjectType((value), CSP_OBJ_MAP)

#define AS_CSP_INT(value)      ((value).as.numInt)
#define AS_CSP_FLOAT(value)    ((value).as.numFloat)
#define AS_CSP_OBJECT(value)   ((value).as.object)
#define AS_CSP_VAR_NAME(value) ((value).as.varName)
#define AS_CSP_STRING(value)   ((CspObjectString *)AS_CSP_OBJECT(value))
#define AS_CSP_CSTRING(value)  (AS_CSP_STRING(value)->chars)
#define AS_CSP_ARRAY(value)    ((CspObjectArray *)AS_CSP_OBJECT(value))
#define AS_CSP_MAP(value)      ((CspObjectMap *)AS_CSP_OBJECT(value))

#define CSP_INT_VALUE(value)      ((CspValue) {.type = CSP_VAL_NUMBER_INT, .as.numInt = (value)})
#define CSP_FLOAT_VALUE(value)    ((CspValue) {.type = CSP_VAL_NUMBER_FLOAT, .as.numFloat = (CSP_FLOAT_TYPE)(value)})
#define CSP_NULL_VALUE()          ((CspValue) {.type = CSP_VAL_NULL})
#define CSP_BOOL_TRUE_VALUE()     ((CspValue) {.type = CSP_VAL_BOOL_TRUE})
#define CSP_BOOL_FALSE_VALUE()    ((CspValue) {.type = CSP_VAL_BOOL_FALSE})
#define CSP_BOOL_VALUE(boolVal)   ((CspValue) {.type = (boolVal) ? CSP_VAL_BOOL_TRUE : CSP_VAL_BOOL_FALSE})
#define CSP_VAR_VALUE(name)       ((CspValue) {.type = CSP_VAL_VARIABLE, .as.varName = newCspVarObject((name))})

#define CSP_OBJECT_TYPE(value)                  (AS_CSP_OBJECT(value)->type)
#define CSP_STR_VALUE(value)                   ((CspValue) {.type = CSP_VAL_OBJECT, .as.object = (CspObject *) newCspStringObject((value), false)})
#define CSP_CONST_STR_VALUE(value)             ((CspValue) {.type = CSP_VAL_OBJECT, .as.object = (CspObject *) newCspStringObject((value), true)})
#define CSP_STR_VALUE_CONCAT(first, second, length) ((CspValue) {.type = CSP_VAL_OBJECT, .as.object = (CspObject *) newCspStringObjectConcat(first, second, length)})
#define CSP_STR_CONCAT_OBJECTS(first, second)   CSP_STR_VALUE_CONCAT((first)->chars, (second)->chars, (first)->length + (second)->length)

#define CSP_ARRAY_VALUE(capacity)       ((CspValue) {.type = CSP_VAL_OBJECT, .as.object = (CspObject *) newCspArrayObject((capacity), false)})
#define CSP_CONST_ARRAY_VALUE(capacity) ((CspValue) {.type = CSP_VAL_OBJECT, .as.object = (CspObject *) newCspArrayObject((capacity), true)})
#define CSP_MAP_VALUE(capacity)         ((CspValue) {.type = CSP_VAL_OBJECT, .as.object = (CspObject *) newCspMapObject((capacity), false)})
#define CSP_CONST_MAP_VALUE(capacity)   ((CspValue) {.type = CSP_VAL_OBJECT, .as.object = (CspObject *) newCspMapObject((capacity), true)})


typedef enum CspValueType {
    CSP_VAL_NULL,             // NULL
    CSP_VAL_VARIABLE,         // var
    CSP_VAL_OBJECT,           // string, map, array
    CSP_VAL_NUMBER_INT,       // int value
    CSP_VAL_NUMBER_FLOAT,     // float value
    CSP_VAL_BOOL_TRUE,        // true
    CSP_VAL_BOOL_FALSE,       // false
} CspValueType;

typedef enum CspObjectType{
    CSP_OBJ_STRING,
    CSP_OBJ_ARRAY,
    CSP_OBJ_MAP,
} CspObjectType;

struct CspObject;
typedef struct CspValVector CspValVector;
typedef struct CspHashMap CspHashMap;

typedef struct CspObject {
    CspObjectType type;
    struct CspObject *next;
} CspObject;

typedef struct CspObjectString {
    CspObject object;
    uint16_t length;
    char chars[];
} CspObjectString;

typedef struct CspObjectArray {
    CspObject object;
    CspValVector *vec;
} CspObjectArray;

typedef struct CspObjectMap {
    CspObject object;
    CspHashMap *map;
} CspObjectMap;

typedef struct CspValue {
    CspValueType type;
    union {
        char *varName;
        CspObject *object;
        CSP_INT_TYPE numInt;
        CSP_FLOAT_TYPE numFloat;
    } as;
} CspValue;

struct CspValVector {
    CspValue *items;
    uint8_t size;
    uint8_t capacity;
};

typedef struct CspMapEntry {
    char *key;  // key is NULL if this slot empty
    CspValue value;
    bool isDeleted;
} CspMapEntry;

struct CspHashMap {
    uint16_t size;
    uint16_t capacity;
    uint16_t deletedItemsCount;
    CspMapEntry *entries;
};

static CspObject *createdObjects = NULL;

// Objects
CspObjectString *newCspStringObject(const char *strValue, bool isConstant);
CspObjectString *newCspStringObjectConcat(const char *one, const char *two, uint16_t totalLength);
CspObjectArray *newCspArrayObject(uint32_t initCapacity, bool isConstant);
CspObjectMap *newCspMapObject(uint32_t initCapacity, bool isConstant);
char *newCspVarObject(char *name);

// Map
CspHashMap *newCspHashMap(uint32_t capacity);
bool cspMapPut(CspHashMap *hashMap, const char *key, CspValue value);
CspValue cspMapGet(CspHashMap *hashMap, const char *key);
CspValue cspMapRemove(CspHashMap *hashMap, const char *key);
CspValue cspMapRemoveEntry(CspHashMap *hashMap, CspMapEntry *entry);
CspMapEntry *getCspValueMapEntry(CspHashMap *hashMap, const char *key);
uint32_t getCspMapSize(CspHashMap *hashMap);
void cspMapDelete(CspHashMap *hashMap);

// Vector
CspValVector *newCspValVec(uint8_t capacity);
bool cspValVecAdd(CspValVector *vector, CspValue item);
CspValue cspValVecGet(CspValVector *vector, uint8_t index);

bool isCspValVecEmpty(CspValVector *vector);
uint8_t cspValVecSize(CspValVector *vector);
bool cspValVecFitToSize(CspValVector *vector);

void freeCspObjects();
void freeCspObject(CspObject *object);
void cspValVecDelete(CspValVector *vector);
void deleteCspValue(CspValue value);

static inline bool isCspObjectType(CspValue value, CspObjectType type) {
    return IS_CSP_OBJECT(value) && AS_CSP_OBJECT(value)->type == type;
}