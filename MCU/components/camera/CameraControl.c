#include "CameraControl.h"

#ifndef MAX
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#endif

#ifndef MIN
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif

static const char *TAG = "CAM";

static int32_t ledIntensity = LEDC_DEFAULT_DUTY;
static bool isCameraInitSuccessfull = false;


static camera_config_t cameraConfig = {
    .pin_pwdn = CAM_PIN_PWDN,
    .pin_reset = CAM_PIN_RESET,
    .pin_xclk = CAM_PIN_XCLK,
    .pin_sccb_sda = CAM_PIN_SIOD,
    .pin_sccb_scl = CAM_PIN_SIOC,

    .pin_d7 = CAM_PIN_D7,
    .pin_d6 = CAM_PIN_D6,
    .pin_d5 = CAM_PIN_D5,
    .pin_d4 = CAM_PIN_D4,
    .pin_d3 = CAM_PIN_D3,
    .pin_d2 = CAM_PIN_D2,
    .pin_d1 = CAM_PIN_D1,
    .pin_d0 = CAM_PIN_D0,
    .pin_vsync = CAM_PIN_VSYNC,
    .pin_href = CAM_PIN_HREF,
    .pin_pclk = CAM_PIN_PCLK,

    //XCLK 20MHz or 10MHz for OV2640 double FPS (Experimental)
    .xclk_freq_hz = 10000000,             // Orginal value
    .ledc_timer = LEDC_TIMER_0,
    .ledc_channel = LEDC_CHANNEL_0,

    .pixel_format = PIXFORMAT_JPEG,     // YUV422,GRAYSCALE,RGB565,JPEG
    .frame_size = FRAME_SIZE,           // QQVGA-UXGA Do not use sizes above QVGA when not JPEG
    .jpeg_quality = 10,                 // 0-63 lower number means higher quality
    .fb_count = 2,                      // if more than one, i2s runs in continuous mode. Use only with JPEG
    .fb_location = CAMERA_FB_IN_PSRAM, // The location where the frame buffer will be allocated */
    .grab_mode = CAMERA_GRAB_LATEST,   // only from new esp32cam version
};


void powerResetCamera() {
    LOG_DEBUG(TAG, "Resetting camera by power down line");
    gpio_config_t conf;
    conf.intr_type = GPIO_INTR_DISABLE;
    conf.pin_bit_mask = 1LL << GPIO_NUM_32;
    conf.mode = GPIO_MODE_OUTPUT;
    conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    conf.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config(&conf);

    // carefull, logic is inverted compared to reset pin
    gpio_set_level(GPIO_NUM_32, 1);
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    gpio_set_level(GPIO_NUM_32, 0);
    vTaskDelay(1000 / portTICK_PERIOD_MS);
}

esp_err_t initCamera() {
    LOG_DEBUG(TAG, "Init Camera");

    esp_camera_deinit();
    esp_err_t cameraStatus = esp_camera_init(&cameraConfig);
    if (cameraStatus != ESP_OK) {
        LOG_ERROR(TAG, "Camera Init Failed");
        return cameraStatus;
    }

    isCameraInitSuccessfull = true;
    initLedControl();
    LOG_INFO(TAG, "Camera Init Success");
    return cameraStatus;
}

void initLedControl() {
#ifdef USE_PWM_LED_FLASH
    // Prepare and then apply the LEDC PWM timer configuration
    ledc_timer_config_t ledControlTimer = {0};

    ledControlTimer.speed_mode       = LEDC_MODE;
    ledControlTimer.timer_num        = LEDC_TIMER;
    ledControlTimer.duty_resolution  = LEDC_DUTY_RES;
    ledControlTimer.freq_hz          = LEDC_FREQUENCY;   // Set output frequency at 5 kHz
    ledControlTimer.clk_cfg          = LEDC_AUTO_CLK;

    ESP_ERROR_CHECK(ledc_timer_config(&ledControlTimer));

    // Prepare and then apply the LEDC PWM channel configuration
    ledc_channel_config_t ledControlChannel = {0};

    ledControlChannel.speed_mode     = LEDC_MODE;
    ledControlChannel.channel        = LEDC_CHANNEL;
    ledControlChannel.timer_sel      = LEDC_TIMER;
    ledControlChannel.intr_type      = LEDC_INTR_DISABLE;
    ledControlChannel.gpio_num       = LEDC_OUTPUT_IO;
    ledControlChannel.duty           = 0; // Set duty to 0%
    ledControlChannel.hpoint         = 0;

    ESP_ERROR_CHECK(ledc_channel_config(&ledControlChannel));
    LOG_DEBUG(TAG, "Flash led PWM init success");
#endif
}

bool testCamera() {
    camera_fb_t *frameBuffer = esp_camera_fb_get();
    bool success = frameBuffer != NULL;
    esp_camera_fb_return(frameBuffer);
    return success;
}

void flashLightOff() {
    #ifdef USE_PWM_LED_FLASH
        LOG_DEBUG(TAG, "Internal Flash-LED turn off PWM");
        ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, 0));
        ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, LEDC_CHANNEL));
    #else
        initFlashLightGpio();  
        gpio_set_level(FLASH_GPIO, 0);
    #endif
}

void flashLightOn() {
    #ifdef USE_PWM_LED_FLASH
        LOG_DEBUG(TAG, "Internal Flash-LED turn on with PWM %d", ledIntensity);
        ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, ledIntensity));
        ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, LEDC_CHANNEL));
    #else
        initFlashLightGpio();
        gpio_set_level(FLASH_GPIO, 1);
    #endif
}

uint16_t rangeLevelToLightDuty(uint8_t level) {
    static const uint16_t LEVEL_TO_DUTY_ARRAY[LED_RANGE_LEVEL_COUNT + 1] = {
            #define X(value) (LED_DUTY_PER_LEVEL * (value)),
                LED_RANGE_LEVELS
            #undef X
    };

    return level <= LED_RANGE_LEVEL_COUNT ? LEVEL_TO_DUTY_ARRAY[level] : LEDC_DEFAULT_DUTY;
}

void setLedIntensity(int32_t ledDuty) {
    ledIntensity = ledDuty;
}

bool isCameraInitialized() {
    return isCameraInitSuccessfull;
}

uint32_t cameraCaptureToFile(char *fileName) {
    LOG_DEBUG(TAG, "Camera image capture started");

    flashLightOn();
    vTaskDelay(400 / portTICK_PERIOD_MS);  // wait a little to get camera exposure settle to new light conditions

    camera_fb_t *cameraFrameBuffer = NULL;
    for (uint8_t i = 0; i < 3; i++) {   // After 3-5 frames green tint is basically gone. However, it's still greenish in bad conditions, like in a very dim light.
        cameraFrameBuffer = esp_camera_fb_get();
        esp_camera_fb_return(cameraFrameBuffer);
        cameraFrameBuffer = NULL;
    }

    cameraFrameBuffer = esp_camera_fb_get();
    if (cameraFrameBuffer == NULL) {
        LOG_ERROR(TAG, "Camera capture failed");
        flashLightOff();
        return 0;
    }

    flashLightOff();
     
    LOG_INFO(TAG, "Camera Frame Buffer: length = %llu", cameraFrameBuffer->len);
    File *imageFile = FILE_OF(&imageDirRoot, fileName);
    createFile(imageFile);

    uint32_t bytesWritten = writeCharsToFile(imageFile, (char *) cameraFrameBuffer->buf, cameraFrameBuffer->len, false);
    LOG_INFO(TAG, "Image bytes written to file: %llu", bytesWritten);

    //return the frame buffer to the driver for reuse
	esp_camera_fb_return(cameraFrameBuffer);
    return bytesWritten;
}

uint32_t cameraCaptureToResponse(httpd_req_t *request) {
    camera_fb_t *cameraFrameBuffer = esp_camera_fb_get();
    if (cameraFrameBuffer == NULL) {
        LOG_ERROR(TAG, "Camera capture failed");
        return 0;
    }

    size_t imageSize = cameraFrameBuffer->len;
    LOG_INFO(TAG, "Camera Frame Buffer to response: length = %llu", imageSize);

    httpd_resp_set_hdr(request, "Content-Type", "image/jpeg");
    httpd_resp_send(request, (char *) cameraFrameBuffer->buf, (ssize_t) imageSize);

    //return the frame buffer to the driver for reuse
	esp_camera_fb_return(cameraFrameBuffer);
    return imageSize;
}

void initFlashLightGpio() {
    // Init the GPIO
    gpio_pad_select_gpio(FLASH_GPIO);
    // Set the GPIO as a push/pull output 
    gpio_set_direction(FLASH_GPIO, GPIO_MODE_OUTPUT); 
}

