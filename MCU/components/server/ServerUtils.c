#include "ServerUtils.h"

static const char *TAG = "SERVER";

static char scratch[SERVER_FILE_SCRATCH_BUFFER_SIZE];

CspTemplate *notFoundPage;

static esp_err_t setContentTypeByFileExtension(httpd_req_t *request, const char *fileName);


esp_err_t sendFile(httpd_req_t *request, const char *fileName) {
    File *file = NEW_FILE(fileName);
    if (!isFileExists(file)) {
        LOG_ERROR(TAG, "Failed to read file: %s", fileName);
        handleErrorPage(request, HTTPD_404_NOT_FOUND);
        return ESP_FAIL;
    }

    LOG_DEBUG(TAG, "Sending file: %s", fileName);
    // For all files from asset directory tell the webbrowser to cache them for all time
    if (strstr(fileName, "assets")) {
        httpd_resp_set_hdr(request, "Cache-Control", "max-age=31536000");
    }

    setContentTypeByFileExtension(request, fileName);

    // Retrieve the pointer to scratch buffer for temporary storage
    FILE *fd = fopen(file->path, "r");
    if (fd == NULL) {
        LOG_ERROR(TAG, "File not found: [%s]", fileName);
        return ESP_FAIL;
    }

    char *chunk = scratch;
    size_t chunksize;
    do {
        // Read file in chunks into the scratch buffer
        chunksize = fread(chunk, 1, SERVER_FILE_SCRATCH_BUFFER_SIZE, fd);

        /* Send the buffer contents as HTTP response chunk */
        if (httpd_resp_send_chunk(request, chunk, (ssize_t) chunksize) != ESP_OK) {
            fclose(fd);
            LOG_ERROR(TAG, "File sending failed!");
            // Abort sending file
            httpd_resp_sendstr_chunk(request, NULL);
            // Respond with 500 Internal Server Error
            httpd_resp_send_err(request, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to send file");
            return ESP_FAIL;
        }

    // Keep looping till the whole file is sent
    } while (chunksize != 0);

    // Respond with an empty chunk to signal HTTP response completion
    httpd_resp_send_chunk(request, NULL, 0);

    // Close file after sending complete
    fclose(fd);
    LOG_DEBUG(TAG, "File sending complete");
    return ESP_OK;
}

esp_err_t getRequestBody(httpd_req_t *request, char *buffer, uint32_t length) {
    size_t remainingLength = request->content_len;

    while (remainingLength > 0) {
        // Read the data for the request
        int receivedBytes = httpd_req_recv(request, buffer, MIN(remainingLength, length));
        if (receivedBytes <= 0) {
            if (receivedBytes == HTTPD_SOCK_ERR_TIMEOUT) {
                continue;   // Retry receiving if timeout occurred
            }
            return ESP_FAIL;
        }
        remainingLength -= receivedBytes;
    }
    return ESP_OK;
}

JSONObject *requestBodyToJson(httpd_req_t *request, JSONObject *resultObject) {
    esp_err_t status = getRequestBody(request, scratch, sizeof(scratch));
    if (status != ESP_OK) {
        return NULL;
    }

    JSONTokener jsonTokener = getJSONTokener(scratch, strlen(scratch));
    JSONObject rootObject = jsonObjectParse(&jsonTokener);
    if (!isJsonObjectOk(&rootObject)) {
        BufferString *message = STRING_FORMAT_64("JSON is not valid. Error code: [%d]", rootObject.jsonTokener->jsonStatus);
        LOG_ERROR(TAG, "%s", message->value);
        return NULL;
    }

    *resultObject = rootObject;
    return resultObject;
}

void logTemplate(CspTemplate *templ, const char *name) {
    LogLevel level = isCspTemplateOk(templ) ? LOG_LEVEL_INFO : LOG_LEVEL_ERROR;
    logMessage(TAG, level, "CSP template [%s]: %s", name, isCspTemplateOk(templ) ? "OK" : cspTemplateErrorMessage(templ));
}

esp_err_t renderHtmlTemplate(httpd_req_t *request, CspTemplate *templ, CspObjectMap *paramMap) {
    CspRenderer *renderer = NEW_CSP_RENDERER(templ, paramMap);
    if (renderer == NULL) {
        LOG_ERROR(TAG, "%s", cspTemplateErrorMessage(templ));
        return ESP_FAIL;
    }
    CspTableString *str = renderCspTemplate(renderer);
    httpd_resp_send(request, str->value, (int) str->length);
    httpd_resp_set_hdr(request, "Access-Control-Allow-Origin", "*");
    deleteCspRenderer(renderer);
    // deleteCspParams(paramMap);
    return ESP_OK;
}

esp_err_t handleErrorPage(httpd_req_t *request, httpd_err_code_t error) {
    if (error == HTTPD_404_NOT_FOUND) {
        LOG_DEBUG(TAG, "In 404 handler");
        renderHtmlTemplate(request, notFoundPage, NULL);
        return ESP_OK;
    }
    return ESP_ERR_NOT_FOUND;
}

static esp_err_t setContentTypeByFileExtension(httpd_req_t *request, const char *fileName) {
    BufferString *fileStr = NEW_STRING(PATH_MAX_LEN, fileName);
    if (isStrEndsWith(fileStr, ".pdf")) {
        return httpd_resp_set_type(request, "application/pdf");

    } else if (isStrEndsWith(fileStr, ".html")) {
        return httpd_resp_set_type(request, "text/html");

    } else if (isStrEndsWith(fileStr, ".jpeg") || isStrEndsWith(fileStr, ".jpg")) {
        return httpd_resp_set_type(request, "image/jpeg");

    } else if (isStrEndsWith(fileStr, ".ico")) {
        return httpd_resp_set_type(request, "image/x-icon");

    } else if (isStrEndsWith(fileStr, ".js")) {
        return httpd_resp_set_type(request, "text/javascript");

    } else if (isStrEndsWith(fileStr, ".css")) {
        return httpd_resp_set_type(request, "text/css");

    } else if (isStrEndsWith(fileStr, ".woff")) {
        return httpd_resp_set_type(request, "font/woff");

    } else if (isStrEndsWith(fileStr, ".woff2")) {
        return httpd_resp_set_type(request, "font/woff2");

    } else {
        // This is a limited set only. For any other type always set as plain text
        return httpd_resp_set_type(request, "text/plain");
    }
}