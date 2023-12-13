#pragma once


#include "RestClient.h"


esp_err_t getLastTelegramMessage();

esp_err_t sendTelegramMessage(const char *message);

esp_err_t sendTelegramPhotoWithCaption(File *imageFile, const char *message, uint32_t messageLength);