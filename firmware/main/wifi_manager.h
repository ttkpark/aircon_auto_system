#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include "esp_err.h"

// WiFi 설정 구조체
typedef struct {
    char ssid[32];
    char password[64];
} wifi_config_t;

// WiFi 관리자 함수들
esp_err_t wifi_manager_init(void);
esp_err_t wifi_manager_connect(const char* ssid, const char* password);
esp_err_t wifi_manager_disconnect(void);
esp_err_t wifi_manager_get_status(wifi_config_t* config);
esp_err_t wifi_manager_save_config(const wifi_config_t* config);
esp_err_t wifi_manager_load_config(wifi_config_t* config);

#endif // WIFI_MANAGER_H 