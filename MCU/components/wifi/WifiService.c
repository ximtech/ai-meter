#include "WifiService.h"

/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries 
 */
#define WIFI_CONNECTED_BIT          BIT0
#define WIFI_FAIL_MAX_RETRIES_BIT   BIT1

static const char *TAG = "WIFI";

WlanConfig wlanInnerConfig = {.hostname = "watermeter"};
static esp_netif_t *accessPoint = NULL;
static esp_netif_t *staNetif = NULL;

// FreeRTOS event group to signal when we are connected
static EventGroupHandle_t wifiEventGroup;

static bool isWifiConnected = false;

static bool initWlanConfig(Properties *configProp);
static void wifiEventHandler(void* arg, esp_event_base_t eventBase, int32_t eventId, void* eventData);


void initWifiMain() {
    LOG_INFO(TAG, "in initWifiMain()");
	static bool isInitialized = false;
	if (isInitialized) {
        LOG_WARN(TAG, "Wifi already initialized");
        return;
    }

	ESP_ERROR_CHECK(esp_netif_init());
	wifiEventGroup = xEventGroupCreate();
	ESP_ERROR_CHECK(esp_event_loop_create_default());

	accessPoint = esp_netif_create_default_wifi_ap();
	assert(accessPoint);

	staNetif = esp_netif_create_default_wifi_sta();
	assert(staNetif);

	wifi_init_config_t wifiInitConfig = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK( esp_wifi_init(&wifiInitConfig) );

    ESP_ERROR_CHECK( esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_AP_STACONNECTED, &wifiEventHandler, NULL) );
    ESP_ERROR_CHECK( esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_AP_STADISCONNECTED, &wifiEventHandler, NULL) );
    ESP_ERROR_CHECK( esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &wifiEventHandler, NULL) );
    ESP_ERROR_CHECK( esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_CONNECTED, &wifiEventHandler, NULL) );
	ESP_ERROR_CHECK( esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifiEventHandler, NULL) );

	ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
	ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_APSTA) );
	ESP_ERROR_CHECK( esp_wifi_start() );

    LOG_INFO(TAG, "initWifiMain() - OK");
	isInitialized = true;
}

esp_err_t wifiInitSoftAp(Properties *configProp) {
    wifi_config_t wifiConfig = {0};

    char *ssidFromMeterName = getProperty(&wlanConfig, PROPERTY_WIFI_HOSTNAME_KEY);
    char *apSsid = ssidFromMeterName != NULL ? ssidFromMeterName : getPropertyOrDefault(configProp, "wifi.soft.access.point.ssid", DEFAULT_ESP_WIFI_AP_SSID);
    char *apPassword = getPropertyOrDefault(configProp, "wifi.soft.access.point.pass", DEFAULT_ESP_WIFI_AP_PASSWORD);
    char *apChannel = getPropertyOrDefault(configProp, "wifi.soft.access.point.channel", DEFAULT_ESP_WIFI_AP_CHANNEL);
    char *apMaxConnections = getPropertyOrDefault(configProp, "wifi.soft.access.point.max.connections", DEFAULT_MAX_STA_CONNECTIONS);

    strcpy((char *) wifiConfig.ap.ssid, apSsid);
    strcpy((char *) wifiConfig.ap.password, apPassword);
    wifiConfig.ap.channel = atoi(apChannel);
    wifiConfig.ap.max_connection = atoi(apMaxConnections);
    wifiConfig.ap.authmode = WIFI_AUTH_WPA_WPA2_PSK;

    if (strlen(apPassword) == 0) {
        LOG_DEBUG(TAG, "No password set for Soft AP, using open access");
        wifiConfig.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifiConfig));
    ESP_ERROR_CHECK(esp_wifi_start());

    LOG_INFO(TAG, "Started with SSID [%s], password: [%s], channel: [%s]. Connect to AP and open http://192.168.4.1", apSsid, apPassword, apChannel);
    return ESP_OK;
}

esp_err_t wifiInitSta(Properties *configProp) {
    if (!initWlanConfig(configProp)) {
        return ESP_ERR_WIFI_BASE;
    }

    bool isWlanHaveStaticParams = strlen(wlanInnerConfig.ipaddress) != 0 && strlen(wlanInnerConfig.gateway) != 0 && strlen(wlanInnerConfig.netmask) != 0;
    if (isWlanHaveStaticParams) {
        LOG_INFO(TAG, "Manual interface config -> IP: [%s], Gateway: [%s], Netmask: [%s]", wlanInnerConfig.ipaddress, wlanInnerConfig.gateway, wlanInnerConfig.netmask);
        esp_netif_dhcpc_stop(accessPoint);	// Stop DHCP service

        esp_netif_ip_info_t ipInfo;
        IPAddress ipAdress = ipAddressFromString(wlanInnerConfig.ipaddress);
        IP4_ADDR(&ipInfo.ip, ipAdress.octetsIPv4[0], ipAdress.octetsIPv4[1], ipAdress.octetsIPv4[2], ipAdress.octetsIPv4[3]);	// Set static IP address

        IPAddress gateway = ipAddressFromString(wlanInnerConfig.gateway);
        IP4_ADDR(&ipInfo.gw, gateway.octetsIPv4[0], gateway.octetsIPv4[1], gateway.octetsIPv4[2], gateway.octetsIPv4[3]);	// Set gateway

        IPAddress netmask = ipAddressFromString(wlanInnerConfig.netmask);
        IP4_ADDR(&ipInfo.netmask, netmask.octetsIPv4[0], netmask.octetsIPv4[1], netmask.octetsIPv4[2], netmask.octetsIPv4[3]);	// Set netmask

        esp_netif_set_ip_info(accessPoint, &ipInfo);	// Set static IP configuration

    } else {
        LOG_INFO(TAG, "Automatic interface config --> Use DHCP service");
    }

    if (isWlanHaveStaticParams) {
        if (strlen(wlanInnerConfig.dns) == 0) {
            LOG_INFO(TAG, "No DNS server, use gateway");
            strcpy(wlanInnerConfig.dns, wlanInnerConfig.netmask);

        } else {
            LOG_INFO(TAG, "Manual interface config -> DNS: [%s]", wlanInnerConfig.dns);
        }

        esp_netif_dns_info_t dnsInfo;
        ip4_addr_t ip;
        ip.addr = esp_ip4addr_aton(wlanInnerConfig.dns);
        ip4_addr_set_u32(&dnsInfo.ip.u_addr.ip4, ip.addr);

        esp_err_t result = esp_netif_set_dns_info(accessPoint, ESP_NETIF_DNS_MAIN, &dnsInfo);
		if (result != ESP_OK) {
			LOG_ERROR(TAG, "esp_netif_set_dns_info: Error: [%d]", result);
			return result;
		}
    }

    wifi_config_t wifiConfig = {0};
    wifiConfig.sta.scan_method = WIFI_ALL_CHANNEL_SCAN;		// Scan all channels instead of stopping after first match
	wifiConfig.sta.sort_method = WIFI_CONNECT_AP_BY_SIGNAL;	// Sort by signal strength and keep up to 4 best APs
	wifiConfig.sta.failure_retry_cnt = 2;
    
    strcpy((char*) wifiConfig.sta.ssid, wlanInnerConfig.ssid);
    strcpy((char*) wifiConfig.sta.password, wlanInnerConfig.password);

    ESP_ERROR_CHECK( esp_wifi_set_config(WIFI_IF_STA, &wifiConfig) );
    ESP_ERROR_CHECK( esp_wifi_connect() );
    LOG_INFO(TAG, "Init start successful. Trying to connect...");

    // Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum number of re-tries (WIFI_FAIL_MAX_RETRIES_BIT)
    EventBits_t eventBits = xEventGroupWaitBits(wifiEventGroup, WIFI_CONNECTED_BIT | WIFI_FAIL_MAX_RETRIES_BIT, pdTRUE, pdFALSE, portMAX_DELAY);

    // xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually happened.
    if (eventBits & WIFI_FAIL_MAX_RETRIES_BIT) {
        LOG_INFO(TAG, "Failed to connect by timeout: %s", wlanInnerConfig.ssid);
        return ESP_FAIL;
    }

    if (eventBits & WIFI_CONNECTED_BIT) {
        LOG_INFO(TAG, "Connection success: %s", wlanInnerConfig.ssid);

        if (strlen(wlanInnerConfig.hostname) != 0) {
            esp_err_t result = esp_netif_set_hostname(accessPoint, wlanInnerConfig.hostname);
            if (result != ESP_OK ) {
                LOG_ERROR(TAG, "Failed to set hostname! Error: [%d]", result);
                return result;
            }
		    LOG_INFO(TAG, "Set hostname to: [%s]", wlanInnerConfig.hostname);
        }
        return ESP_OK;
    }

    LOG_ERROR(TAG, "Unexpected wifi event");
    return ESP_FAIL;
}

uint16_t scanWifiAccessPoints(wifi_ap_record_t *apRecords, uint16_t length) {
    uint16_t number = length;
    uint16_t apCount = 0;

    esp_wifi_scan_start(NULL, true);
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&number, apRecords));
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&apCount));
    LOG_DEBUG(TAG, "Total APs scanned = %u", apCount);
    return MIN(apCount, number);
}

char *wifiAccessPointAuthModeToStr(wifi_auth_mode_t authMode) {
    switch (authMode) {
        case WIFI_AUTH_OPEN:
            return "WIFI_AUTH_OPEN";
        case WIFI_AUTH_OWE:
            return "WIFI_AUTH_OWE";
        case WIFI_AUTH_WEP:
            return "WIFI_AUTH_WEP";
        case WIFI_AUTH_WPA_PSK:
            return "WIFI_AUTH_WPA_PSK";
        case WIFI_AUTH_WPA2_PSK:
            return "WIFI_AUTH_WPA2_PSK";
        case WIFI_AUTH_WPA_WPA2_PSK:
            return "WIFI_AUTH_WPA_WPA2_PSK";
        case WIFI_AUTH_WPA2_ENTERPRISE:
            return "WIFI_AUTH_WPA2_ENTERPRISE";
        case WIFI_AUTH_WPA3_PSK:
            return "WIFI_AUTH_WPA3_PSK";
        case WIFI_AUTH_WPA2_WPA3_PSK:
            return "WIFI_AUTH_WPA2_WPA3_PSK";
        default:
            return "WIFI_AUTH_UNKNOWN";
    }
}

bool wifiHasLogAndPassToConnect(Properties *configProp) {
    return getProperty(configProp, PROPERTY_WIFI_SSID_KEY) != NULL && getProperty(configProp, PROPERTY_WIFI_PASSWORD_KEY) != NULL;
}

int32_t getWifiRssi() {
	wifi_ap_record_t accessPointRecord;
    return esp_wifi_sta_get_ap_info(&accessPointRecord) == ESP_OK ? accessPointRecord.rssi : RSSI_NOT_CONNECTED;
}

bool isWifiHasConnection() { 
    return isWifiConnected;
}

void destroyWifi()  {	
	esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, wifiEventHandler);
    esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, wifiEventHandler);
	esp_wifi_disconnect();
	esp_wifi_stop();
	esp_wifi_deinit();
}

void resetWifiSettings() {
   wifi_config_t currentConfig;
   esp_wifi_get_config((wifi_interface_t)ESP_IF_WIFI_STA, &currentConfig);
   memset(currentConfig.sta.ssid, 0, sizeof(currentConfig.sta.ssid));
   memset(currentConfig.sta.password, 0, sizeof(currentConfig.sta.password));
   esp_wifi_set_config((wifi_interface_t)ESP_IF_WIFI_STA, &currentConfig);
}

static bool initWlanConfig(Properties *configProp) {
    if (!wifiHasLogAndPassToConnect(configProp)) {
        LOG_ERROR(TAG, "SSID/Password is empty. Device init aborted");
        return false;
    }

    char *meterName = getProperty(configProp, PROPERTY_METER_NAME_KEY);
    if (isCstrBlank(meterName)) {
        LOG_ERROR(TAG, "Meter name is empty. Device init aborted");
        return false;
    }

    char *ssid = getProperty(configProp, PROPERTY_WIFI_SSID_KEY);
    char *password = getPropertyOrDefault(configProp, PROPERTY_WIFI_PASSWORD_KEY, "");
    strncpy(wlanInnerConfig.ssid, ssid, sizeof(wlanInnerConfig.ssid));
    strncpy(wlanInnerConfig.password, password, sizeof(wlanInnerConfig.password));

    LOG_INFO(TAG, "Wifi SSID: [%s]", wlanInnerConfig.ssid);
    #ifndef HIDE_PASSWORD
    LOG_INFO(TAG, "Wifi Password: [%s]", wlanInnerConfig.password);
    #else
    LOG_INFO(TAG,  "Wifi Password: [XXXXXXXX]");
    #endif

    char *hostname = getProperty(configProp, PROPERTY_WIFI_HOSTNAME_KEY);
    BufferString *host = toLowerCase(NEW_STRING(METER_NAME_MAX_LENGTH, hostname));
    strncpy(wlanInnerConfig.hostname, host->value, host->length);
    LOG_INFO(TAG, "Wifi Hostname: [%s]", wlanInnerConfig.hostname);

    // set optional parameters if present
    char *ip = getProperty(configProp, PROPERTY_WIFI_IP_KEY);
    if (isCstrNotEmpty(ip)) {
        strncpy(wlanInnerConfig.ipaddress, ip, sizeof(wlanInnerConfig.ipaddress));
        LOG_INFO(TAG, "Wifi IP-Address: [%s]", wlanInnerConfig.ipaddress);
    }

    char *gateway = getProperty(configProp, PROPERTY_WIFI_GATEWAY_KEY);
    if (isCstrNotEmpty(gateway)) {
        strncpy(wlanInnerConfig.gateway, gateway, sizeof(wlanInnerConfig.gateway));
        LOG_INFO(TAG, "Wifi Gateway: [%s]", wlanInnerConfig.gateway);
    }

    char *netmask = getProperty(configProp, PROPERTY_WIFI_NETMASK_KEY);
    if (isCstrNotEmpty(netmask)) {
        strncpy(wlanInnerConfig.netmask, netmask, sizeof(wlanInnerConfig.netmask));
        LOG_INFO(TAG, "Wifi Netmask: [%s]", wlanInnerConfig.netmask);
    }

    char *dns = getProperty(configProp, PROPERTY_WIFI_DNS_KEY);
    if (isCstrNotEmpty(dns)) {
        strncpy(wlanInnerConfig.dns, dns, sizeof(wlanInnerConfig.dns));
        LOG_INFO(TAG, "Wifi DNS: [%s]", wlanInnerConfig.dns);
    }
    
    LOG_DEBUG(TAG, "Configuration set success");
    return true;
}

static void wifiEventHandler(void* arg, esp_event_base_t eventBase, int32_t eventId, void* eventData) {
    if (eventBase == WIFI_EVENT && eventId == WIFI_EVENT_STA_START) {
        isWifiConnected = false;
        esp_wifi_connect();

    } else if (eventId == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t *event = (wifi_event_ap_staconnected_t *) eventData;
        LOG_INFO(TAG, "Station " MACSTR " join, AID=%d", MAC2STR(event->mac), event->aid);

    } else if (eventId == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_staconnected_t *event = (wifi_event_ap_staconnected_t *) eventData;
        LOG_INFO(TAG, "Station " MACSTR " leave, AID=%d", MAC2STR(event->mac), event->aid);

    } else if (eventBase == WIFI_EVENT && eventId == WIFI_EVENT_STA_DISCONNECTED) {

        /* Disconnect reason: https://github.com/espressif/esp-idf/blob/d825753387c1a64463779bbd2369e177e5d59a79/components/esp_wifi/include/esp_wifi_types.h */
		wifi_event_sta_disconnected_t *disconnected = (wifi_event_sta_disconnected_t *)eventData;
		if (disconnected->reason == WIFI_REASON_ROAMING) {
			LOG_WARN(TAG, "Disconnected: [%d], Roaming 802.11kv", disconnected->reason);
			// --> no reconnect neccessary, it should automatically reconnect to new AP

		} else {
			isWifiConnected = false;
			if (disconnected->reason == WIFI_REASON_NO_AP_FOUND) {
				LOG_WARN(TAG, "Disconnected: [%d], No AP", disconnected->reason);

			} else if (disconnected->reason == WIFI_REASON_AUTH_EXPIRE ||
					 disconnected->reason == WIFI_REASON_AUTH_FAIL || 
					 disconnected->reason == WIFI_REASON_NOT_AUTHED ||
					 disconnected->reason == WIFI_REASON_4WAY_HANDSHAKE_TIMEOUT || 
					 disconnected->reason == WIFI_REASON_HANDSHAKE_TIMEOUT) {
				LOG_WARN(TAG, "Disconnected: [%d], Auth fail", disconnected->reason);

			} else if (disconnected->reason == WIFI_REASON_BEACON_TIMEOUT) {
				LOG_WARN(TAG, "Disconnected: [%d], Timeout", disconnected->reason);

			} else {
				LOG_WARN(TAG, "Disconnected: [%d], Unknown", disconnected->reason);
			}
		}

        xEventGroupSetBits(wifiEventGroup, WIFI_FAIL_MAX_RETRIES_BIT);
		LOG_ERROR(TAG, "Disconnected, multiple reconnect attempts failed: [%d], still retrying...", disconnected->reason);

    } else if (eventBase == WIFI_EVENT && eventId == WIFI_EVENT_STA_CONNECTED) {
        LOG_INFO(TAG, "Connected to : %s, RSSI: %d", wlanInnerConfig.ssid, getWifiRssi());

    } else if (eventBase == IP_EVENT && eventId == IP_EVENT_STA_GOT_IP) {
        isWifiConnected = true;
        xEventGroupSetBits(wifiEventGroup, WIFI_CONNECTED_BIT);

        ip_event_got_ip_t* event = (ip_event_got_ip_t*) eventData;
        strncpy(wlanInnerConfig.ipaddress, ip4addr_ntoa((const ip4_addr_t *)&event->ip_info.ip), sizeof(wlanInnerConfig.ipaddress));
        LOG_INFO("Assigned IP: %s", wlanInnerConfig.ipaddress);
    }
}