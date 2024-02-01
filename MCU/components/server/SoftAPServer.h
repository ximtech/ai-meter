#pragma once

#include "../../src/AppConfig.h"

#include "esp_wifi.h"
#include "rom/ets_sys.h"
#include "esp_http_server.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
// #include "mbedtls/aes.h"

#include "ServerUtils.h"
#include "WifiService.h"
#include "CameraControl.h"
#include "TelegramApiClient.h"
#include "NTPTime.h"
#include "Encryptor.h"

httpd_handle_t startWebServerAP();

esp_err_t handleFavicon(httpd_req_t *request);
esp_err_t handleHtmlResources(httpd_req_t *request);
esp_err_t handlePhotoResources(httpd_req_t *request);
esp_err_t handleFileResources(httpd_req_t *request);

bool isAppFullyConfigured(Properties *configProp);