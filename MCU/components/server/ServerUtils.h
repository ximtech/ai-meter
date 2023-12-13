#pragma once

#include "../../src/AppConfig.h"

#include "esp_wifi.h"
#include "rom/ets_sys.h"
#include "esp_http_server.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "nvs_flash.h"

#define NULL_VAL_ERROR_MESSAGE(name) "Mandatory field '" #name "' can't be NULL"

#define ASSERT(expr, msg) \
       if (!(expr)) {    \
           httpd_resp_send_err(request, HTTPD_400_BAD_REQUEST, msg); \
           LOG_ERROR(TAG, "%s", msg);   \
           return ESP_FAIL; \
       }                 \

#define ASSERT_JSON_VAL(jsonObj, name) \
        if (!isJsonObjectOk((jsonObj))) {    \
           httpd_resp_send_err(request, HTTPD_400_BAD_REQUEST, NULL_VAL_ERROR_MESSAGE(name)); \
           LOG_ERROR(TAG, "%s", NULL_VAL_ERROR_MESSAGE(name));   \
           deleteJSONObject((jsonObj));   \
           return ESP_FAIL; \
       }

#define ASSERT_NOT_NULL(value, name) \
        ASSERT((value) != NULL, NULL_VAL_ERROR_MESSAGE(name)) \

#define ASSERT_ESP_OK(value, msg) \
        ASSERT((value) == ESP_OK, msg) \

#define REQUEST_TO_JSON(request) requestBodyToJson(request, &(JSONObject){0})

extern CspTemplate *notFoundPage;

esp_err_t sendFile(httpd_req_t *request, const char *fileName);

esp_err_t getRequestBody(httpd_req_t *request, char *buffer, uint32_t length);

JSONObject *requestBodyToJson(httpd_req_t *request, JSONObject *resultObject);

void logTemplate(CspTemplate *templ, const char *name);

esp_err_t renderHtmlTemplate(httpd_req_t *request, CspTemplate *templ, CspObjectMap *paramMap);

esp_err_t handleErrorPage(httpd_req_t *request, httpd_err_code_t error);