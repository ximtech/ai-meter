#pragma once

#include "../../src/AppConfig.h"

#include "esp_http_client.h"
#include "esp_tls.h"
#include "esp_random.h"

#define MAX_HTTP_OUTPUT_BUFFER 4 * ONE_KB

extern esp_http_client_handle_t restClient;
extern char *httpResponseBuffer;

void initRestClient();

esp_err_t sendFormDataInChunks(esp_http_client_handle_t client, const char *data, size_t dataLength);