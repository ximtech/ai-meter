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

httpd_handle_t startWebServerAP();

esp_err_t handleFavicon(httpd_req_t *request);
esp_err_t handleHtmlResources(httpd_req_t *request);
esp_err_t handlePhotoResources(httpd_req_t *request);
esp_err_t handleFileResources(httpd_req_t *request);
esp_err_t handleErrorPage(httpd_req_t *request, httpd_err_code_t error);

bool isAppFullyConfigured(Properties *configProp);
