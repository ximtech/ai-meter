#pragma once

#include "../../src/AppConfig.h"

#include "esp_heap_caps.h"


void *mallocPsramHeap(size_t size);

void *callocPsramHeap(size_t n, size_t size);

void *reallocPsRamHeap(char *ptr, size_t size);

void freePsramHeap(void *memPointer);
