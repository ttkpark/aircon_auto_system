#include "wifi_manager.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_netif.h"

static const char *TAG = "WIFI_MANAGER";

// NVS 네임스페이스
#define WIFI_CONFIG_NAMESPACE "wifi_config"
#define WIFI_SSID_KEY "ssid"
#define WIFI_PASSWORD_KEY "password"

esp_err_t wifi_manager_init(void)
{
    ESP_LOGI(TAG, "WiFi 관리자 초기화");
    return ESP_OK;
}

esp_err_t wifi_manager_connect(const char* ssid, const char* password)
{
    if (!ssid || !password) {
        return ESP_ERR_INVALID_ARG;
    }

    wifi_config_t wifi_config = {0};
    strncpy((char*)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid) - 1);
    strncpy((char*)wifi_config.sta.password, password, sizeof(wifi_config.sta.password) - 1);

    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_connect());

    ESP_LOGI(TAG, "WiFi 연결 시도: %s", ssid);
    return ESP_OK;
}

esp_err_t wifi_manager_disconnect(void)
{
    ESP_ERROR_CHECK(esp_wifi_disconnect());
    ESP_LOGI(TAG, "WiFi 연결 해제");
    return ESP_OK;
}

esp_err_t wifi_manager_get_status(wifi_config_t* config)
{
    if (!config) {
        return ESP_ERR_INVALID_ARG;
    }

    wifi_ap_record_t ap_info;
    if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) {
        strncpy(config->ssid, (char*)ap_info.ssid, sizeof(config->ssid) - 1);
        config->ssid[sizeof(config->ssid) - 1] = '\0';
        ESP_LOGI(TAG, "현재 연결된 WiFi: %s", config->ssid);
    } else {
        ESP_LOGW(TAG, "WiFi 연결 상태 확인 실패");
        config->ssid[0] = '\0';
    }

    return ESP_OK;
}

esp_err_t wifi_manager_save_config(const wifi_config_t* config)
{
    if (!config) {
        return ESP_ERR_INVALID_ARG;
    }

    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(WIFI_CONFIG_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "NVS 열기 실패: %s", esp_err_to_name(err));
        return err;
    }

    err = nvs_set_str(nvs_handle, WIFI_SSID_KEY, config->ssid);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "SSID 저장 실패: %s", esp_err_to_name(err));
        nvs_close(nvs_handle);
        return err;
    }

    err = nvs_set_str(nvs_handle, WIFI_PASSWORD_KEY, config->password);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Password 저장 실패: %s", esp_err_to_name(err));
        nvs_close(nvs_handle);
        return err;
    }

    err = nvs_commit(nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "NVS 커밋 실패: %s", esp_err_to_name(err));
    }

    nvs_close(nvs_handle);
    ESP_LOGI(TAG, "WiFi 설정 저장 완료");
    return err;
}

esp_err_t wifi_manager_load_config(wifi_config_t* config)
{
    if (!config) {
        return ESP_ERR_INVALID_ARG;
    }

    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(WIFI_CONFIG_NAMESPACE, NVS_READONLY, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "NVS 열기 실패: %s", esp_err_to_name(err));
        return err;
    }

    size_t ssid_len = sizeof(config->ssid);
    err = nvs_get_str(nvs_handle, WIFI_SSID_KEY, config->ssid, &ssid_len);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "SSID 로드 실패: %s", esp_err_to_name(err));
        nvs_close(nvs_handle);
        return err;
    }

    size_t password_len = sizeof(config->password);
    err = nvs_get_str(nvs_handle, WIFI_PASSWORD_KEY, config->password, &password_len);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Password 로드 실패: %s", esp_err_to_name(err));
        nvs_close(nvs_handle);
        return err;
    }

    nvs_close(nvs_handle);
    ESP_LOGI(TAG, "WiFi 설정 로드 완료: %s", config->ssid);
    return ESP_OK;
} 