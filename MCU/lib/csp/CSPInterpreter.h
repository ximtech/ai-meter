#pragma once

#include <math.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "CSPCompiler.h"
#include "CSPTableString.h"

#ifndef CSP_STACK_MAX_VALUES
#define CSP_STACK_MAX_VALUES 256
#endif

#ifndef CSP_PARAMETER_STRING_LENGTH
#define CSP_PARAMETER_STRING_LENGTH 256
#endif


void interpretCspChunk(CspReport *report, CspChunk* chunk, CspTableString *resultStr, CspObjectMap *paramMap);
bool isTruthyCspExp(CspReport *report, CspChunk* chunk, CspObjectMap *paramMap);
CspValue evaluateToCspValue(CspReport *report, CspChunk* chunk, CspObjectMap *paramMap);


static inline CspObjectMap *newCspParamObjMap(uint32_t initCapacity) {
    return newCspMapObject(initCapacity, true);
}

static inline bool cspAddIntToMap(CspObjectMap *mapObj, const char *key, CSP_INT_TYPE value) {
    return mapObj != NULL ? cspMapPut(mapObj->map, key, CSP_INT_VALUE(value)) : false;
}

static inline bool cspAddFloatToMap(CspObjectMap *mapObj, const char *key, CSP_FLOAT_TYPE value) {
    return mapObj != NULL ? cspMapPut(mapObj->map, key, CSP_FLOAT_VALUE(value)) : false;
}

static inline bool cspAddStrToMap(CspObjectMap *mapObj, const char *key, char *value) {
    return mapObj != NULL ? cspMapPut(mapObj->map, key, value != NULL ? CSP_STR_VALUE(value) : CSP_NULL_VALUE()) : false;
}

static inline bool cspAddValToMap(CspObjectMap *mapObj, const char *key, CspValue value) {
    return mapObj != NULL ? cspMapPut(mapObj->map, key, value) : false;
}

static inline bool cspAddMapToMap(CspObjectMap *mapObj, CspObjectMap *toMapObj, const char *key) {
    CspValue mapValue = {.type = CSP_VAL_OBJECT, .as.object = (CspObject *) mapObj};
    return mapObj != NULL && toMapObj != NULL ? cspMapPut(toMapObj->map, key, mapValue) : false;
}

static inline bool cspAddVecToMap(CspObjectArray *arrayObj, CspObjectMap *toMapObj, const char *key) {
    CspValue arrayValue = {.type = CSP_VAL_OBJECT, .as.object = (CspObject *) arrayObj};
    return arrayObj != NULL && toMapObj != NULL ? cspMapPut(toMapObj->map, key, arrayValue) : false;
}


static inline CspObjectArray *newCspParamObjArray(uint32_t initCapacity) {
    return newCspArrayObject(initCapacity, true);
}

static inline bool cspAddIntToArray(CspObjectArray *arrayObj, CSP_INT_TYPE value) {
    return arrayObj != NULL ? cspValVecAdd(arrayObj->vec, CSP_INT_VALUE(value)) : false;
}

static inline bool cspAddFloatToArray(CspObjectArray *arrayObj, CSP_FLOAT_TYPE value) {
    return arrayObj != NULL ? cspValVecAdd(arrayObj->vec, CSP_FLOAT_VALUE(value)) : false;
}

static inline bool cspAddStrToArray(CspObjectArray *arrayObj, const char *value) {
    return arrayObj != NULL ? cspValVecAdd(arrayObj->vec, value != NULL ? CSP_STR_VALUE(value) : CSP_NULL_VALUE()) : false;
}

static inline bool cspAddValToArray(CspObjectArray *arrayObj, CspValue value) {
    return arrayObj != NULL ? cspValVecAdd(arrayObj->vec, value) : false;
}

static inline bool cspAddMapToArray(CspObjectMap *mapObj, CspObjectArray *toArrayObj) {
    CspValue mapValue = {.type = CSP_VAL_OBJECT, .as.object = (CspObject *) mapObj};
    return mapObj != NULL && toArrayObj != NULL ? cspValVecAdd(toArrayObj->vec, mapValue) : false;
}

static inline bool cspAddArrayToArray(CspObjectArray *arrayObj, CspObjectArray *toArrayObj) {
    CspValue arrayValue = {.type = CSP_VAL_OBJECT, .as.object = (CspObject *) arrayObj};
    return arrayObj != NULL && toArrayObj != NULL ? cspValVecAdd(toArrayObj->vec, arrayValue) : false;
}

static inline void deleteCspParams(CspObjectMap *paramMap) {
    if (paramMap != NULL ) cspMapDelete(paramMap->map);
}
