#include "RestClient.h"

typedef struct HttpHandlerData {
    int outputLength;       // Stores number of bytes read
} HttpHandlerData;

static const char *TAG = "REST";

static void initHttpResponseBuffer();
static esp_err_t httpEventHandler(esp_http_client_event_t *event);
static esp_err_t onDataHandler(esp_http_client_event_t *event, HttpHandlerData *httpData);
static void onFinishHandler(esp_http_client_event_t *event, HttpHandlerData *httpData);
static void onDisconnecthHandler(esp_http_client_event_t *event, HttpHandlerData *httpData);

esp_http_client_handle_t restClient;
char *httpResponseBuffer = NULL;

void initRestClient() {
    initHttpResponseBuffer();

    esp_http_client_config_t config = {
            .host = "https://api.ipgeolocation.io", 
            .path = "/",
            .transport_type = HTTP_TRANSPORT_OVER_SSL,
            .event_handler = httpEventHandler,
            .user_data = httpResponseBuffer,        // Pass address of local buffer to get response
            .timeout_ms = 10000,
        };

    restClient = esp_http_client_init(&config);
    if (restClient == NULL) {
        LOG_ERROR(TAG, "Error init rest client");
        return;
    }
    LOG_INFO(TAG, "Rest client successfully initialized");
}

esp_err_t sendFormDataInChunks(esp_http_client_handle_t client, const char *data, size_t dataLength) {
    // Send the file data in chunks
    size_t chunkSize = 16 * ONE_KB;
    for (size_t offset = 0; offset < dataLength; offset += chunkSize) {
        size_t remaining = dataLength - offset;
        size_t sendSize = (remaining < chunkSize) ? remaining : chunkSize;

        int writtenDataLength = esp_http_client_write(client, data + offset, sendSize);
        if (writtenDataLength < 0) {
            LOG_ERROR(TAG, "Failed to send form-data chunk");
            return ESP_FAIL;
        }
    }

    LOG_INFO(TAG, "Form data chunks has been successfully send");
    return ESP_OK;
}

static void initHttpResponseBuffer() {
    if (httpResponseBuffer == NULL) {
        LOG_DEBUG(TAG, "Allocating http response buffer. Size: [%d]", MAX_HTTP_OUTPUT_BUFFER);
        httpResponseBuffer = malloc(sizeof(char) * MAX_HTTP_OUTPUT_BUFFER + 1);
        if (httpResponseBuffer == NULL) {
            LOG_ERROR(TAG, "Memory allocation fail for http response buffer");
            return;
        }
    }
}

static esp_err_t httpEventHandler(esp_http_client_event_t *event) {
    static HttpHandlerData httpData = {0};
    esp_err_t status = ESP_OK;

    switch(event->event_id) {
        case HTTP_EVENT_ERROR:
            LOG_DEBUG(TAG, "HTTP_EVENT_ERROR");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            LOG_DEBUG(TAG, "HTTP_EVENT_ON_CONNECTED");
            break;
        case HTTP_EVENT_HEADER_SENT:
            LOG_DEBUG(TAG, "HTTP_EVENT_HEADER_SENT");
            break;
        case HTTP_EVENT_ON_HEADER:
            LOG_DEBUG(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", event->header_key, event->header_value);
            break;
        case HTTP_EVENT_ON_DATA:
            LOG_DEBUG(TAG, "HTTP_EVENT_ON_DATA");
            status = onDataHandler(event, &httpData);
            break;
        case HTTP_EVENT_ON_FINISH:
            LOG_DEBUG(TAG, "HTTP_EVENT_ON_FINISH");
            onFinishHandler(event, &httpData);
            break;
        case HTTP_EVENT_DISCONNECTED:
            LOG_INFO(TAG, "HTTP_EVENT_DISCONNECTED");
            onDisconnecthHandler(event, &httpData);
            break;
        case HTTP_EVENT_REDIRECT:
            LOG_DEBUG(TAG, "HTTP_EVENT_REDIRECT");
            esp_http_client_set_header(event->client, "From", "user@example.com");
            esp_http_client_set_header(event->client, "Accept", "text/html");
            esp_http_client_set_redirection(event->client);
            break;
    }
    return status;
}

static esp_err_t onDataHandler(esp_http_client_event_t *event, HttpHandlerData *httpData) {
    // Clean the buffer in case of a new request
    if (httpData->outputLength == 0 && event->user_data) {
        memset(event->user_data, 0, MAX_HTTP_OUTPUT_BUFFER);    // we are just starting to copy the output data into the use
    }

    if (!esp_http_client_is_chunked_response(event->client)) {
        // If user_data buffer is configured, copy the response into the buffer
        int32_t responseLength = 0;
        if (event->user_data != NULL) {
            // The last byte in event->user_data is kept for the NULL character in case of out-of-bound access.
            responseLength = MIN(event->data_len, (MAX_HTTP_OUTPUT_BUFFER - httpData->outputLength));
            if (responseLength > 0) {
                memcpy((event->user_data + httpData->outputLength), event->data, responseLength);
            }
            httpData->outputLength += responseLength;
            return ESP_OK;
        }
        
        LOG_ERROR(TAG, "Response buffer is NULL");
        return ESP_FAIL;
    } 

    if (esp_http_client_is_chunked_response(event->client)) {
        bool hasSpaceInBuffer = (httpData->outputLength + event->data_len) < MAX_HTTP_OUTPUT_BUFFER;
        if (hasSpaceInBuffer && event->user_data != NULL) {
            memcpy(event->user_data + httpData->outputLength, event->data, event->data_len);
            httpData->outputLength += event->data_len;
            return ESP_OK;
        }

        LOG_ERROR(TAG, "Response buffer overflow!");
        return ESP_ERR_NO_MEM;
    }

    return ESP_OK;
}

static void onFinishHandler(esp_http_client_event_t *event, HttpHandlerData *httpData) {
    httpData->outputLength = 0;
}

static void onDisconnecthHandler(esp_http_client_event_t *event, HttpHandlerData *httpData) {
    int mbedtlsError = 0;
    esp_err_t error = esp_tls_get_and_clear_last_error((esp_tls_error_handle_t) event->data, &mbedtlsError, NULL);
    if (error != ESP_OK) {
        LOG_INFO(TAG, "Last esp error code: 0x%x", error);
        LOG_INFO(TAG, "Last mbedtls failure: 0x%x", mbedtlsError);
    }

    if (event->data && strlen(event->data)) {
        LOG_INFO(TAG, "Status code: %d", atoi(event->data));
    }

    httpData->outputLength = 0;
}

