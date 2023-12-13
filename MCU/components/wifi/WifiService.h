#pragma once

#include "../../src/AppConfig.h"

#include "esp_wifi.h"
#include "lwip/ip4_addr.h"
#include "esp_netif.h"
#include "rom/ets_sys.h"
#include "freertos/event_groups.h"

#include "StatusLed.h"

#define RSSI_NOT_CONNECTED -127

typedef struct WlanConfig {
    char ssid[32];
    char password[32];
    char hostname[32];    // Default: watermeter
    char ipaddress[16];
    char gateway[32];
    char netmask[16];
    char dns[16];
} WlanConfig;

extern WlanConfig wlanInnerConfig;

void initWifiMain();
esp_err_t wifiInitSta(Properties *configProp);
esp_err_t wifiInitSoftAp(Properties *configProp);

uint16_t scanWifiAccessPoints(wifi_ap_record_t *apRecords, uint16_t length);
char *wifiAccessPointAuthModeToStr(wifi_auth_mode_t authMode);

bool wifiHasLogAndPassToConnect(Properties *configProp);

int32_t getWifiRssi();  // signal strength

bool isWifiHasConnection();

void destroyWifi();
void resetWifiSettings();