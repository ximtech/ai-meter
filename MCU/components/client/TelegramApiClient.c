#include "TelegramApiClient.h"

static const char *TAG = "TELEGRAM_API";


esp_err_t getLastTelegramMessage() {
    char *telegramBotUrl = getProperty(&appConfig, PROPERTY_TELEGRAM_API_URL_KEY);
    char *telegramBotApiKey = getProperty(&appConfig, PROPERTY_TELEGRAM_API_KEY);
    BufferString *url = joinChars(EMPTY_STRING(128), "", 3, telegramBotUrl, telegramBotApiKey, "/getUpdates");

    LOG_DEBUG(TAG, "Call telegram bot. URL: [%s]", url->value);
    esp_http_client_set_url(restClient, url->value);
    esp_http_client_set_method(restClient, HTTP_METHOD_GET);
    esp_http_client_set_header(restClient, "Accept", "*/*");
    esp_http_client_set_header(restClient, "Host", "api.telegram.org");
    esp_http_client_set_header(restClient, "Cache-Control", "no-cache");
    esp_http_client_set_header(restClient, "Content-Type", "application/x-www-form-urlencoded");

    const char *postData = "limit=1&offset=-1";
    esp_http_client_set_post_field(restClient, postData, (int) strlen(postData));
    esp_err_t status = esp_http_client_perform(restClient);
    if (status == ESP_OK) {
        LOG_INFO(TAG, "Telegram message updates successfully fetched");
    } else {
        LOG_ERROR(TAG, "Error receive telegram updates: [%s]", esp_err_to_name(status));
    }

    esp_http_client_close(restClient);
    return status;
}


esp_err_t sendTelegramMessage(const char *message) {
    char *telegramBotUrl = getProperty(&appConfig, PROPERTY_TELEGRAM_API_URL_KEY);
    char *telegramBotApiKey = getProperty(&appConfig, PROPERTY_TELEGRAM_API_KEY);
    char *telegramChatId = getProperty(&wlanConfig, PROPERTY_TELEGRAM_CHAT_ID_KEY);

    BufferString *url = joinChars(EMPTY_STRING(128), "", 3, telegramBotUrl, telegramBotApiKey, "/sendMessage");
    LOG_INFO(TAG, "Sending message to telegram bot. URL: [%s]", url->value);
    esp_http_client_set_url(restClient, url->value);
    esp_http_client_set_method(restClient, HTTP_METHOD_POST);
    esp_http_client_set_header(restClient, "Accept", "*/*");
    esp_http_client_set_header(restClient, "Host", "api.telegram.org");
    esp_http_client_set_header(restClient, "Cache-Control", "no-cache");
    esp_http_client_set_header(restClient, "Content-Type", "application/x-www-form-urlencoded");

    LOG_INFO(TAG, "Message: [%s]", message);
    BufferString *bodyMessage = STRING_FORMAT_128("chat_id=%s&text=%s", telegramChatId, message);
    esp_http_client_set_post_field(restClient, bodyMessage->value, (int) bodyMessage->length);
    esp_err_t status = esp_http_client_perform(restClient);
    if (status == ESP_OK) {
        LOG_INFO(TAG, "Telegram message successfully sended");
    } else {
        LOG_ERROR(TAG, "Error sending telegram message: [%s]", esp_err_to_name(status));
    }

    esp_http_client_close(restClient);
    return status;
}

esp_err_t sendTelegramPhotoWithCaption(File *imageFile, const char *message, uint32_t messageLength) {
    LOG_INFO(TAG, "Start to send image [%s] to Telegram bot with message: [%s]", imageFile->path, message);
	char *telegramBotUrl = getProperty(&appConfig, PROPERTY_TELEGRAM_API_URL_KEY);
    char *telegramBotApiKey = getProperty(&appConfig, PROPERTY_TELEGRAM_API_KEY);
    char *telegramChatId = getProperty(&wlanConfig, PROPERTY_TELEGRAM_CHAT_ID_KEY);
    BufferString *url = joinChars(EMPTY_STRING(256), "", 6, telegramBotUrl, telegramBotApiKey, "/sendPhoto", "?", "chat_id=", telegramChatId);

    BufferString *imageName = getFileName(imageFile, EMPTY_STRING(32));
    BufferString *parseModeValue = NEW_STRING(16, "html");

    BufferString *boundary = STRING_FORMAT_64("--------------------------%U32%U32", esp_random(), esp_random());
    BufferString *imageFileHeader = STRING_FORMAT_256(
            "--%S\r\n"
            "Content-Disposition: form-data; name=\"photo\"; filename=\"%S\"\r\n"
            "Content-Type: image/jpeg\r\n\r\n", boundary, imageName);
    BufferString *captionHeader = STRING_FORMAT_256(
            "\r\n--%S\r\n"
            "Content-Disposition: form-data; name=\"caption\"\r\n\r\n", boundary);
    BufferString *parseModeHeader = STRING_FORMAT_256(
            "\r\n--%S\r\n"
            "Content-Disposition: form-data; name=\"parse_mode\"\r\n\r\n", boundary);
    BufferString *boundaryEnd = STRING_FORMAT_64("\r\n--%S--\r\n", boundary);

    size_t imageSize = getFileSize(imageFile);
    size_t totalSize = imageFileHeader->length + imageSize + captionHeader->length + messageLength + parseModeHeader->length + parseModeValue->length + boundaryEnd->length;
    
    LOG_INFO(TAG, "Image: [%s]. Size: [%zu], Total payload size: [%zu]", imageName->value, imageSize, totalSize);
    char *imageBuffer = callocPsramHeap(imageSize + 1, sizeof(char));
    if (imageBuffer == NULL) {
        LOG_ERROR(TAG, "PSRAM memory allocation fails for image: [%s]", imageFile->path);
        return ESP_FAIL;
    }

    FILE *infile = fopen(imageFile->path, "rb");
    fseek(infile, 0L, SEEK_SET);    // set file position indicator to the beginning of the file
    fread(imageBuffer, sizeof(char), imageSize, infile);   // copy all content into the buffer
    fclose(infile);

    LOG_DEBUG(TAG, "Request URL: [%s]", url->value);
    esp_http_client_set_url(restClient, url->value);
    esp_http_client_set_method(restClient, HTTP_METHOD_POST);
    esp_http_client_set_header(restClient, "Accept", "*/*");
    esp_http_client_set_header(restClient, "Cache-Control", "no-cache");
    esp_http_client_set_header(restClient, "Connection", "keep-alive");
    esp_http_client_set_header(restClient, "Content-Type", STRING_FORMAT_128("multipart/form-data; boundary=%S", boundary)->value);

    esp_err_t status = esp_http_client_open(restClient, (int) totalSize);
    if (status == ESP_OK) {
        LOG_DEBUG(TAG, "Form-data header length written: %d", esp_http_client_write(restClient, imageFileHeader->value, imageFileHeader->length));
        LOG_DEBUG(TAG, "Image data length written: %d", esp_http_client_write(restClient, imageBuffer, imageSize));

        LOG_DEBUG(TAG, "Form-data caption header length written: %d", esp_http_client_write(restClient, captionHeader->value, captionHeader->length));
        LOG_DEBUG(TAG, "Caption message length written: %d", esp_http_client_write(restClient, message, messageLength));

        LOG_DEBUG(TAG, "Form-data parse mode header length written: %d", esp_http_client_write(restClient, parseModeHeader->value, parseModeHeader->length));
        LOG_DEBUG(TAG, "Parse mode value length written: %d", esp_http_client_write(restClient, parseModeValue->value, parseModeValue->length));

        LOG_DEBUG(TAG, "Form-data boundary end length written: %d", esp_http_client_write(restClient, boundaryEnd->value, boundaryEnd->length));

        status = esp_http_client_perform(restClient);
        if (status == ESP_OK) {
            LOG_INFO(TAG, "Image: [%s] successfully sended", imageName->value);
        } else {
            LOG_ERROR(TAG, "Failed to send image: [%s]. Status code: [%s]", imageName->value, esp_err_to_name(status));
        }
        
    } else {
        LOG_ERROR(TAG, "Failed to establish connection: Status code: [%s]", esp_err_to_name(status));
    }
    
    esp_http_client_close(restClient);
    freePsramHeap(imageBuffer);
    return status;
}