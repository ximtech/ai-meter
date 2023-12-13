#include "Vector.h"

#define MIN(x, y) (((x)<(y))?(x):(y))

static bool doubleVectorCapacity(Vector vector);
static bool halfVectorCapacity(Vector vector);

struct Vector {
    VectorValueType *itemArray;
    uint32_t initialCapacity;
    uint32_t capacity;
    uint32_t size;
};

Vector getVectorInstance(uint32_t capacity) {
    if (capacity < 1) return NULL;

    Vector vector = malloc(sizeof(struct Vector));
    if (vector == NULL) return NULL;
    vector->size = 0;
    vector->capacity = capacity;
    vector->initialCapacity = capacity;
    vector->itemArray = calloc(vector->capacity, sizeof(VectorValueType));

    if (vector->itemArray == NULL) {
        vectorDelete(vector);
        return NULL;
    }
    return vector;
}

bool vectorAdd(Vector vector, VectorValueType item) {
    if (vector != NULL) {
        if (vector->size >= vector->capacity) {
            if (!doubleVectorCapacity(vector)) return false;
        }
        vector->itemArray[vector->size++] = item;
        return true;
    }
    return false;
}

VectorValueType vectorGet(Vector vector, uint32_t index) {
    return (vector != NULL && index < vector->size) ? vector->itemArray[index] : (VectorValueType) NULL;
}

bool vectorPut(Vector vector, uint32_t index, VectorValueType item) {
    if (vector != NULL && index < vector->size) {
        vector->itemArray[index] = item;
        return true;
    }
    return false;
}

bool vectorAddAt(Vector vector, uint32_t index, VectorValueType item) {
    if (vector != NULL && index < vector->size) {
        if (vector->size >= vector->capacity) {
            if (!doubleVectorCapacity(vector)) return false;
        }
        for (uint32_t i = vector->size; i > index; i--) {
            vector->itemArray[i] = vector->itemArray[i - 1];
        }
        vector->itemArray[index] = item;
        vector->size++;
        return true;
    }
    return false;
}

VectorValueType vectorRemoveAt(Vector vector, uint32_t index) {
    if (vector != NULL && index < vector->size) {
        VectorValueType item = vector->itemArray[index];
        for (uint32_t i = index + 1; i < vector->size; i++) {
            vector->itemArray[i - 1] = vector->itemArray[i];
        }
        vector->size--;

        if ((vector->size * 4) < vector->capacity) {
            if (!halfVectorCapacity(vector)) return (VectorValueType) NULL;
        }
        return item;
    }
    return (VectorValueType) NULL;
}

bool isVectorEmpty(Vector vector) {
    return vector != NULL && vector->size == 0;
}

bool isVectorNotEmpty(Vector vector) {
    return !isVectorEmpty(vector);
}

uint32_t getVectorSize(Vector vector) {
    return vector != NULL ? vector->size : 0;
}

void vectorClear(Vector vector) {
    if (vector != NULL) {
        vector->size = 0;
        while (vector->capacity > vector->initialCapacity) {
            if (!halfVectorCapacity(vector)) break;
        }
    }
}

void vectorDelete(Vector vector) {
    if (vector != NULL) {
        free(vector->itemArray);
        free(vector);
    }
}

void initSingletonVector(Vector *vector, uint32_t capacity) {
    if (*vector == NULL) {
        *vector = getVectorInstance(capacity);
    }
}

static bool doubleVectorCapacity(Vector vector) {
    uint32_t newCapacity = vector->capacity * 2;
    if (newCapacity < vector->capacity) return false;   // overflow (capacity would be too big)

    VectorValueType *newItemArray = malloc(sizeof(VectorValueType) * newCapacity);
    if (newItemArray == NULL) return false;

    for (uint32_t i = 0; i < vector->size; i++) {
        newItemArray[i] = vector->itemArray[i];
    }
    free(vector->itemArray);
    vector->itemArray = newItemArray;
    vector->capacity = newCapacity;
    return true;
}

static bool halfVectorCapacity(Vector vector) {
    if (vector->capacity <= vector->initialCapacity) return false;
    uint32_t newCapacity = vector->capacity / 2;
    VectorValueType *newItemArray = malloc(sizeof(VectorValueType) * newCapacity);
    if (newItemArray == NULL) return false;

    for (uint32_t i = 0; i < MIN(vector->size, newCapacity); i++) {
        newItemArray[i] = vector->itemArray[i];
    }
    free(vector->itemArray);
    vector->itemArray = newItemArray;
    vector->capacity = newCapacity;
    vector->size = MIN(vector->size, newCapacity);
    return true;
}