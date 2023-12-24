#include "PSRAM.h"

static const char* TAG = "PSRAM";


void *mallocPsramHeap(size_t size) {
	void *memPointer = heap_caps_malloc(size, MALLOC_CAP_SPIRAM);
    if (memPointer != NULL) {
	    LOG_DEBUG(TAG, "Allocated [%zu] bytes in PSRAM", size);
        return memPointer;
	}
    LOG_ERROR(TAG, "Failed to allocate [%zu] bytes in PSRAM", size);
	return memPointer;
}

void *callocPsramHeap(size_t n, size_t size) {
	void *memPointer = heap_caps_calloc(n, size, MALLOC_CAP_SPIRAM);
    if (memPointer != NULL) {
	    LOG_DEBUG(TAG, "Allocated [%zu] bytes in PSRAM", size);
        return memPointer;
	}
    LOG_ERROR(TAG, "Failed to allocate [%zu] bytes in PSRAM", size);
	return memPointer;
}

void *reallocPsRamHeap(char *ptr, size_t size) {
    void *memPointer = heap_caps_realloc(ptr, size, MALLOC_CAP_SPIRAM);
    if (memPointer != NULL) {
	    LOG_DEBUG(TAG, "Reallocated [%zu] bytes in PSRAM", size);
        return memPointer;
	}
    LOG_ERROR(TAG, "Failed to reallocated [%zu] bytes in PSRAM", size);
	return memPointer;

}

void freePsramHeap(void *memPointer) {
    LOG_DEBUG(TAG, "Freeing memory in PSRAM");
    heap_caps_free(memPointer);
}