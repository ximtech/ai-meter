#pragma once

#include "FileUtils.h"
#include "HashMap.h"

#ifndef PROPERTIES_INITIAL_CAPACITY
    #define PROPERTIES_INITIAL_CAPACITY 8
#endif

typedef enum PropertiesStatus {
    CONFIG_PROP_OK,
    CONFIG_PROP_ERROR_INVALID_FILE_TYPE,
    CONFIG_PROP_ERROR_FILE_NOT_FOUND,
    CONFIG_PROP_ERROR_CREATE_FILE,
    CONFIG_PROP_ERROR_OPEN_FILE,
    CONFIG_PROP_ERROR_MEMORY_ALLOC,
    CONFIG_PROP_ERROR_READ_FILE,
    CONFIG_PROP_ERROR_MEMORY_ALLOC_KEY,
    CONFIG_PROP_ERROR_MEMORY_ALLOC_VALUE,
} PropertiesStatus;

typedef struct Properties {
    HashMap map;
    PropertiesStatus status;
} Properties;


#define INIT_PROPERTIES() initProperties(&(Properties){0})
#define LOAD_PROPERTIES(fileName) loadProperties(&(Properties){0}, fileName)
#define LOAD_PROPERTIES_BUFF(dataBuffer) loadPropertiesBuffer(&(Properties){0}, dataBuffer)


Properties *loadProperties(Properties *properties, const char *fileName);
Properties *loadPropertiesBuffer(Properties *properties, char *dataBuffer);

Properties *initProperties(Properties *properties);
void storeProperties(Properties *properties, const char *fileName);

const char *propStatusToString(PropertiesStatus status);
void deleteConfigProperties(Properties *properties);

bool putProperty(Properties *properties, char *key, char *value);
void propertiesRemove(Properties *properties, char *key);
void propertiesPutAll(Properties *from, Properties *to);

static inline char *getProperty(Properties *properties, char *key) {
    return (char *) hashMapGet(properties->map, key);
}

static inline char *getPropertyOrDefault(Properties *properties, char *key, char *defaultValue) {
    return (char *) hashMapGetOrDefault(properties->map, key, defaultValue);
}

static inline uint32_t propertiesSize(Properties *properties) {
    return getHashMapSize(properties->map);
}

static inline bool isEmptyProperties(Properties *properties) {
    return isHashMapEmpty(properties->map);
}

static inline bool propertiesHasKey(Properties *properties, char *key) {
    return isHashMapContainsKey(properties->map, key);
}

void propertiesToString(Properties *properties, char *buffer, int32_t length);




