#pragma once

#include "../../src/AppConfig.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"

#include "esp_camera.h"
#include "driver/ledc.h"
#include "hal/ledc_types.h"
#include "esp_http_server.h"

#include "StatusLed.h"

#define CAMERA_INIT_TRY_COUNT 3

#if defined(CAMERA_CONFIG_FRAMESIZE_VGA)
    static const framesize_t FRAME_SIZE = FRAMESIZE_VGA;
	#define FRAMESIZE_STRING "640x480"
#elif defined(CAMERA_CONFIG_FRAMESIZE_SVGA)
    static const framesize_t FRAME_SIZE = FRAMESIZE_SVGA;
	#define FRAMESIZE_STRING "800x600"
#elif defined(CAMERA_CONFIG_FRAMESIZE_XGA)
    static const framesize_t FRAME_SIZE = FRAMESIZE_XGA;
	#define FRAMESIZE_STRING "1024x768"
#elif defined(CAMERA_CONFIG_FRAMESIZE_HD)
    static const framesize_t FRAME_SIZE = FRAMESIZE_HD;
	#define FRAMESIZE_STRING "1280x720"
#elif defined(CAMERA_CONFIG_FRAMESIZE_SXGA)
    static const framesize_t FRAME_SIZE = FRAMESIZE_SXGA;
	#define FRAMESIZE_STRING "1280x1024"
#elif defined(CAMERA_CONFIG_FRAMESIZE_UXGA)
    static const framesize_t FRAME_SIZE = FRAMESIZE_UXGA;
	#define FRAMESIZE_STRING "1600x1200"
#endif


void powerResetCamera();
esp_err_t initCamera();
esp_err_t initCameraWithRetry(uint8_t cameraInitTry);

void initLedControl();
bool testCamera();

void flashLightOff();
void flashLightOn();
uint16_t rangeLevelToLightDuty(uint8_t level);
void setLedIntensity(int32_t ledDuty);
void initFlashLightGpio();

bool isCameraInitialized();

uint32_t cameraCaptureToFile(char *fileName);
uint32_t cameraCaptureToResponse(httpd_req_t *request);