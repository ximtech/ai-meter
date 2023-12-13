#pragma once

#define SD_CARD_ROOT "/sdcard"

#define DEFAULT_LOG_LEVEL_STR "DEBUG"
#define DEFAULT_LOG_FILE_SIZE "1MB"
#define DEFAULT_LOG_FILE_BACKUPS "5"

#define DEFAULT_LOG_FILE_PATH SD_CARD_ROOT "/log"
#define DEFAULT_LOG_FILE_NAME "application.log"
#define LOGGER_MAX_SUBSCRIBERS 3
#define LOGGER_FILE_NAME_MAX_SIZE CONFIG_FATFS_MAX_LFN

#define PATH_MAX_LEN CONFIG_FATFS_MAX_LFN    //Max length a file path can have on storage
#define MAX_FILES_IN_DIR 256

#define CONFIG_FILE             SD_CARD_ROOT "/application.properties"
#define WLAN_CONFIG_FILE        SD_CARD_ROOT "/wlan.properties"
#define HTML_TEMPLATE_DIR       SD_CARD_ROOT "/html"
#define HTML_ASSETS_IMAGE_DIR   HTML_TEMPLATE_DIR "/assets/img"
#define CAMERA_IMAGE_DIR        SD_CARD_ROOT "/photo"
#define DATABASE_DIR            SD_CARD_ROOT "/db"
#define EMBEDDED_DATABASE_FILE  DATABASE_DIR "/embedded.db"

#define HIDE_PASSWORD   // hide password from wlan.properties in log

#define FLASH_GPIO          GPIO_NUM_4
// #define BLINK_GPIO          GPIO_NUM_33
#define BLINK_GPIO          GPIO_NUM_0
#define CONFIG_START_GPIO   GPIO_NUM_0

#define DEFAULT_TIME_ZONE "UTC"

#define MAX_FILE_SIZE "8 MB" // Max size of an individual file. Make sure this value is same as that set in upload_script.html and ota_page.html!
#define MAX_PHOTOS_IN_DIR 64
#define OLDEST_PHOTOS_TO_REMOVE_COUNT 16

// Soft AP default config
#define DEFAULT_ESP_WIFI_AP_SSID         "AI-Meter"
#define DEFAULT_ESP_WIFI_AP_PASSWORD      ""
#define DEFAULT_ESP_WIFI_AP_CHANNEL       "11"
#define DEFAULT_MAX_STA_CONNECTIONS       "1"

// CameraControl + CameraFlowTakeImage
#define CAMERA_MODEL_AI_THINKER
#define BOARD_ESP32CAM_AITHINKER

//CameraControl
#define USE_PWM_LED_FLASH
#define CALIBRATION_PHOTO_NAME "calibration_photo.jpeg"

// HTML Pages
#define CSP_WELCOME_PAGE_NAME "welcome.csp"
#define CSP_CONNECT_PAGE_NAME "connect.csp"
#define CSP_CALIBRATE_PAGE_NAME "calibrate.csp"
#define CSP_SCHEDULE_PAGE_NAME "schedule.csp"
#define CSP_MESSAGING_PAGE_NAME "messaging.csp"
#define CSP_SUMMARY_PAGE_NAME "summary.csp"
#define CSP_ADMIN_PAGE_NAME "admin.csp"
#define CSP_NOT_FOUND_PAGE_NAME "not_found.csp"
#define CSP_MESSAGE_CAPTION_NAME "message_caption.csp"

// Properties keys from application.properties
#define PROPERTY_IP_GEOLOCATION_ENABLED_KEY "ip.geolocation.enabled"
#define PROPERTY_IP_GEOLOCATION_URL_KEY "ip.geolocation.url"
#define PROPERTY_IP_GEOLOCATION_TIMEZONE_FORMAT_URL_KEY "ip.geolocation.timezone.format.url"
#define PROPERTY_IP_GEOLOCATION_API_KEY "ip.geolocation.api.key"

#define PROPERTY_TELEGRAM_API_BOT_NAME_KEY "telegram.api.bot.name"
#define PROPERTY_TELEGRAM_API_URL_KEY "telegram.api.bot.url"
#define PROPERTY_TELEGRAM_API_KEY "telegram.api.bot.api.key"

// Logger config keys
#define PROPERTY_LOG_LEVEL_KEY "logging.level"
#define PROPERTY_LOG_FILE_PATH_KEY "logging.file.path"
#define PROPERTY_LOG_FILE_NAME_KEY "logging.file.name"
#define PROPERTY_LOG_FILE_ENABLED_KEY "logging.file.enabled"
#define PROPERTY_LOG_CONSOLE_ENABLED_KEY "logging.console.enabled"

// Properties keys from wlan.properties
#define METER_NAME_PREFIX "AI-Meter-"
#define METER_NAME_MAX_LENGTH (62 + sizeof(METER_NAME_PREFIX) - 1)
#define PROPERTY_METER_NAME_KEY "meter.ai.name"
#define PROPERTY_APP_SYSTEM_TIMEZONE_KEY "system.time.zone"
#define PROPERTY_SYSTEM_CRON_EXPR_KEY "system.cron.expr"
#define PROPERTY_ENABLE_CONFIG_KEY "enable.soft.ap.config"
#define PROPERTY_PREVIOUS_CRON_DATE_KEY "system.cron.previous.date" // Format: DATE_TIME_FORMAT_SHORT

#define PROPERTY_WIFI_SSID_KEY "wifi.ssid"
#define PROPERTY_WIFI_PASSWORD_KEY "wifi.password"
#define PROPERTY_WIFI_HOSTNAME_KEY "wifi.hostname"
#define PROPERTY_WIFI_IP_KEY "wifi.ip"
#define PROPERTY_WIFI_GATEWAY_KEY "wifi.gateway"
#define PROPERTY_WIFI_NETMASK_KEY "wifi.netmask"
#define PROPERTY_WIFI_DNS_KEY "wifi.dns"

#define PROPERTY_CALIBRATION_FOTO_KEY "calibration.photo.name"
#define PROPERTY_FLASH_LIGHT_INTENSITY_KEY "flash.light.intensity.level"
#define PROPERTY_TELEGRAM_CHAT_ID_KEY "telegram.chat.id"

#include "FileUtils.h"
#include "Logger.h"
#include "BufferString.h"
#include "GlobalDateTime.h"
#include "CronExpression.h"
#include "SqliteWrapper.h"
#include "BufferVector.h"
#include "Properties.h"
#include "IPAddress.h"
#include "PSRAM.h"
#include "JSON.h"

#include "CSPRenderer.h"
#include "version.h"

extern Properties appConfig;
extern Properties wlanConfig;

extern File imageDirRoot;
extern TimeZone timeZone;
extern CronExpression cron;
extern sqlite3 *embeddedDb;

#define SERVER_FILE_SCRATCH_BUFFER_SIZE (32 * ONE_KB)
#define DATE_TIME_FORMAT_ZONE_STR "yyyy-MM-dd HH:mm:ss ZZZZZ"
#define DATE_TIME_FORMAT_WITHOUT_ZONE_STR "yyyy-MM-dd HH:mm:ss"
#define DATE_TIME_FORMAT_SHORT "yyyy.MM.dd HH:mm"
#define DATE_TIME_FILE_NAME_FORMAT "yyyy_MM_dd_HH_mm"
#define CRON_JOB_ALLOWED_MINUTES_GAP 5

// SD Card config
#define SD_CARD_USE_ONE_LINE_MODE

// Camera config
#define CAMERA_CONFIG_FRAMESIZE_UXGA

//******* camera model 
#if defined(CAMERA_MODEL_WROVER_KIT)
    #define PWDN_GPIO_NUM    -1
    #define RESET_GPIO_NUM   -1
    #define XCLK_GPIO_NUM    21
    #define SIOD_GPIO_NUM    26
    #define SIOC_GPIO_NUM    27

    #define Y9_GPIO_NUM      35
    #define Y8_GPIO_NUM      34
    #define Y7_GPIO_NUM      39
    #define Y6_GPIO_NUM      36
    #define Y5_GPIO_NUM      19
    #define Y4_GPIO_NUM      18
    #define Y3_GPIO_NUM       5
    #define Y2_GPIO_NUM       4
    #define VSYNC_GPIO_NUM   25
    #define HREF_GPIO_NUM    23
    #define PCLK_GPIO_NUM    22

#elif defined(CAMERA_MODEL_M5STACK_PSRAM)
    #define PWDN_GPIO_NUM     -1
    #define RESET_GPIO_NUM    15
    #define XCLK_GPIO_NUM     27
    #define SIOD_GPIO_NUM     25
    #define SIOC_GPIO_NUM     23

    #define Y9_GPIO_NUM       19
    #define Y8_GPIO_NUM       36
    #define Y7_GPIO_NUM       18
    #define Y6_GPIO_NUM       39
    #define Y5_GPIO_NUM        5
    #define Y4_GPIO_NUM       34
    #define Y3_GPIO_NUM       35
    #define Y2_GPIO_NUM       32
    #define VSYNC_GPIO_NUM    22
    #define HREF_GPIO_NUM     26
    #define PCLK_GPIO_NUM     21

#elif defined(CAMERA_MODEL_AI_THINKER)
    #define PWDN_GPIO_NUM     GPIO_NUM_32
    #define RESET_GPIO_NUM    (-1)
    #define XCLK_GPIO_NUM      GPIO_NUM_0
    #define SIOD_GPIO_NUM     GPIO_NUM_26
    #define SIOC_GPIO_NUM     GPIO_NUM_27

    #define Y9_GPIO_NUM       GPIO_NUM_35
    #define Y8_GPIO_NUM       GPIO_NUM_34
    #define Y7_GPIO_NUM       GPIO_NUM_39
    #define Y6_GPIO_NUM       GPIO_NUM_36
    #define Y5_GPIO_NUM       GPIO_NUM_21
    #define Y4_GPIO_NUM       GPIO_NUM_19
    #define Y3_GPIO_NUM       GPIO_NUM_18
    #define Y2_GPIO_NUM        GPIO_NUM_5
    #define VSYNC_GPIO_NUM    GPIO_NUM_25
    #define HREF_GPIO_NUM     GPIO_NUM_23
    #define PCLK_GPIO_NUM     GPIO_NUM_22

#else
    #error "Camera model not selected"
#endif  //camera model

// ******* Board type   
#ifdef BOARD_WROVER_KIT // WROVER-KIT PIN Map

    #define CAM_PIN_PWDN -1  //power down is not used
    #define CAM_PIN_RESET -1 //software reset will be performed
    #define CAM_PIN_XCLK 21
    #define CAM_PIN_SIOD 26
    #define CAM_PIN_SIOC 27

    #define CAM_PIN_D7 35
    #define CAM_PIN_D6 34
    #define CAM_PIN_D5 39
    #define CAM_PIN_D4 36
    #define CAM_PIN_D3 19
    #define CAM_PIN_D2 18
    #define CAM_PIN_D1 5
    #define CAM_PIN_D0 4
    #define CAM_PIN_VSYNC 25
    #define CAM_PIN_HREF 23
    #define CAM_PIN_PCLK 22

#endif //// WROVER-KIT PIN Map

    
#ifdef BOARD_ESP32CAM_AITHINKER // ESP32Cam (AiThinker) PIN Map

    #define CAM_PIN_PWDN 32
    #define CAM_PIN_RESET (-1) //software reset will be performed
    #define CAM_PIN_XCLK 0
    #define CAM_PIN_SIOD 26
    #define CAM_PIN_SIOC 27

    #define CAM_PIN_D7 35
    #define CAM_PIN_D6 34
    #define CAM_PIN_D5 39
    #define CAM_PIN_D4 36
    #define CAM_PIN_D3 21
    #define CAM_PIN_D2 19
    #define CAM_PIN_D1 18
    #define CAM_PIN_D0 5
    #define CAM_PIN_VSYNC 25
    #define CAM_PIN_HREF 23
    #define CAM_PIN_PCLK 22

#endif // ESP32Cam (AiThinker) PIN Map

// ******* LED definition
//// PWM for Flash-LED
#define LEDC_TIMER              LEDC_TIMER_1 // LEDC_TIMER_0
#define LEDC_MODE               LEDC_LOW_SPEED_MODE
#define LEDC_OUTPUT_IO          FLASH_GPIO // Define the output GPIO
#define LEDC_CHANNEL            LEDC_CHANNEL_1
#define LEDC_DUTY_RES           LEDC_TIMER_13_BIT // Set duty resolution to 13 bits

#define LEDC_MAX_DUTY           (8191 / 10)  // Max duty 8191, then divide by 10 for lowest intensity
#define LEDC_DEFAULT_DUTY       (LEDC_MAX_DUTY / 2) // Set duty to 50%. If max: ((2 ** 13) - 1) * 50% = 4095
#define LEDC_FREQUENCY          (5000) // Frequency in Hertz. Set frequency at 5 kHz
#define LED_RANGE_LEVEL_COUNT   (24)
#define LED_DUTY_PER_LEVEL      (LEDC_MAX_DUTY / LED_RANGE_LEVEL_COUNT)
#define LED_RANGE_LEVELS \
    X(0) X(1) X(2) X(3) X(4) X(5) X(6) X(7) X(8) X(9) X(10) X(11) X(12) \
    X(13) X(14) X(15) X(16) X(17) X(18) X(19) X(20) X(21) X(22) X(23) X(24)

#define BATTERY_ADC_CHANNEL ADC_CHANNEL_6      // GPIO14 -> HS2_CLK, will be used one on app startup
#define BATTERY_GPIO_PIN    GPIO_NUM_14

extern void configureButtonWakeup();