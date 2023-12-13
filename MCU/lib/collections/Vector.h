#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct Vector *Vector;
typedef void* VectorValueType; // Vector can keep any type, change for specific

Vector getVectorInstance(uint32_t capacity);

bool vectorAdd(Vector vector, VectorValueType item);
VectorValueType vectorGet(Vector vector, uint32_t index);
bool vectorPut(Vector vector, uint32_t index, VectorValueType item);

bool vectorAddAt(Vector vector, uint32_t index, VectorValueType item);
VectorValueType vectorRemoveAt(Vector vector, uint32_t index);

bool isVectorEmpty(Vector vector);
bool isVectorNotEmpty(Vector vector);
uint32_t getVectorSize(Vector vector);

void vectorClear(Vector vector);
void vectorDelete(Vector vector);

void initSingletonVector(Vector *vector, uint32_t capacity);
