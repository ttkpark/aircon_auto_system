#include "web_server.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "cJSON.h"
#include "ir_controller.h"
#include "wifi_manager.h"

static const char *TAG = "WEB_SERVER";

static httpd_handle_t server = NULL;

// API 키 (실제 운영에서는 더 복잡한 인증 시스템 사용)
#define API_KEY "aircon_control_2024"

// CORS 헤더 추가
static void add_cors_headers(httpd_req_t *req)
{
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Headers", "Content-Type, Authorization");
}

// API 키 검증
static esp_err_t verify_api_key(httpd_req_t *req)
{
    char auth_header[256];
    int ret = httpd_req_get_hdr_value_str(req, "Authorization", auth_header, sizeof(auth_header));
    
    if (ret == ESP_OK) {
        if (strncmp(auth_header, "Bearer ", 7) == 0) {
            if (strcmp(auth_header + 7, API_KEY) == 0) {
                return ESP_OK;
            }
        }
    }
    
    return ESP_FAIL;
}

// 상태 확인 API
static esp_err_t status_get_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "상태 확인 요청");
    
    add_cors_headers(req);
    
    if (verify_api_key(req) != ESP_OK) {
        httpd_resp_set_status(req, "401 Unauthorized");
        httpd_resp_send(req, NULL, 0);
        return ESP_OK;
    }
    
    cJSON *response = cJSON_CreateObject();
    cJSON_AddStringToObject(response, "status", "online");
    cJSON_AddStringToObject(response, "device", "ESP32 Aircon Controller");
    cJSON_AddStringToObject(response, "version", "1.0.0");
    
    // WiFi 상태 추가
    wifi_config_t wifi_config;
    if (wifi_manager_get_status(&wifi_config) == ESP_OK) {
        cJSON_AddStringToObject(response, "wifi_ssid", wifi_config.ssid);
        cJSON_AddStringToObject(response, "wifi_status", "connected");
    } else {
        cJSON_AddStringToObject(response, "wifi_status", "disconnected");
    }
    
    char *response_str = cJSON_Print(response);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, response_str, strlen(response_str));
    
    free(response_str);
    cJSON_Delete(response);
    
    return ESP_OK;
}

// WiFi 상태 확인 API
static esp_err_t wifi_get_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "WiFi 상태 확인 요청");
    
    add_cors_headers(req);
    
    if (verify_api_key(req) != ESP_OK) {
        httpd_resp_set_status(req, "401 Unauthorized");
        httpd_resp_send(req, NULL, 0);
        return ESP_OK;
    }
    
    cJSON *response = cJSON_CreateObject();
    wifi_config_t wifi_config;
    
    if (wifi_manager_get_status(&wifi_config) == ESP_OK) {
        cJSON_AddStringToObject(response, "status", "connected");
        cJSON_AddStringToObject(response, "ssid", wifi_config.ssid);
    } else {
        cJSON_AddStringToObject(response, "status", "disconnected");
    }
    
    char *response_str = cJSON_Print(response);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, response_str, strlen(response_str));
    
    free(response_str);
    cJSON_Delete(response);
    
    return ESP_OK;
}

// 에어컨 전원 제어 API
static esp_err_t aircon_power_post_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "에어컨 전원 제어 요청");
    
    add_cors_headers(req);
    
    if (verify_api_key(req) != ESP_OK) {
        httpd_resp_set_status(req, "401 Unauthorized");
        httpd_resp_send(req, NULL, 0);
        return ESP_OK;
    }
    
    char content[100];
    int ret = httpd_req_recv(req, content, sizeof(content) - 1);
    if (ret <= 0) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
    content[ret] = '\0';
    
    cJSON *json = cJSON_Parse(content);
    if (!json) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
    
    cJSON *power = cJSON_GetObjectItem(json, "power");
    if (!power || !cJSON_IsString(power)) {
        cJSON_Delete(json);
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
    
    aircon_command_t command;
    if (strcmp(power->valuestring, "on") == 0) {
        command = AIRCON_POWER_ON;
    } else if (strcmp(power->valuestring, "off") == 0) {
        command = AIRCON_POWER_OFF;
    } else {
        cJSON_Delete(json);
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
    
    esp_err_t err = ir_controller_send_command(command);
    
    cJSON *response = cJSON_CreateObject();
    if (err == ESP_OK) {
        cJSON_AddStringToObject(response, "status", "success");
        cJSON_AddStringToObject(response, "message", "명령이 성공적으로 전송되었습니다");
    } else {
        cJSON_AddStringToObject(response, "status", "error");
        cJSON_AddStringToObject(response, "message", "명령 전송 실패");
    }
    
    char *response_str = cJSON_Print(response);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, response_str, strlen(response_str));
    
    free(response_str);
    cJSON_Delete(response);
    cJSON_Delete(json);
    
    return ESP_OK;
}

// 에어컨 온도 제어 API
static esp_err_t aircon_temp_post_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "에어컨 온도 제어 요청");
    
    add_cors_headers(req);
    
    if (verify_api_key(req) != ESP_OK) {
        httpd_resp_set_status(req, "401 Unauthorized");
        httpd_resp_send(req, NULL, 0);
        return ESP_OK;
    }
    
    char content[100];
    int ret = httpd_req_recv(req, content, sizeof(content) - 1);
    if (ret <= 0) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
    content[ret] = '\0';
    
    cJSON *json = cJSON_Parse(content);
    if (!json) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
    
    cJSON *action = cJSON_GetObjectItem(json, "action");
    if (!action || !cJSON_IsString(action)) {
        cJSON_Delete(json);
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
    
    aircon_command_t command;
    if (strcmp(action->valuestring, "up") == 0) {
        command = AIRCON_TEMP_UP;
    } else if (strcmp(action->valuestring, "down") == 0) {
        command = AIRCON_TEMP_DOWN;
    } else {
        cJSON_Delete(json);
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
    
    esp_err_t err = ir_controller_send_command(command);
    
    cJSON *response = cJSON_CreateObject();
    if (err == ESP_OK) {
        cJSON_AddStringToObject(response, "status", "success");
        cJSON_AddStringToObject(response, "message", "온도 조정 명령이 전송되었습니다");
    } else {
        cJSON_AddStringToObject(response, "status", "error");
        cJSON_AddStringToObject(response, "message", "명령 전송 실패");
    }
    
    char *response_str = cJSON_Print(response);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, response_str, strlen(response_str));
    
    free(response_str);
    cJSON_Delete(response);
    cJSON_Delete(json);
    
    return ESP_OK;
}

// 에어컨 모드 제어 API
static esp_err_t aircon_mode_post_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "에어컨 모드 제어 요청");
    
    add_cors_headers(req);
    
    if (verify_api_key(req) != ESP_OK) {
        httpd_resp_set_status(req, "401 Unauthorized");
        httpd_resp_send(req, NULL, 0);
        return ESP_OK;
    }
    
    char content[100];
    int ret = httpd_req_recv(req, content, sizeof(content) - 1);
    if (ret <= 0) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
    content[ret] = '\0';
    
    cJSON *json = cJSON_Parse(content);
    if (!json) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
    
    cJSON *mode = cJSON_GetObjectItem(json, "mode");
    if (!mode || !cJSON_IsString(mode)) {
        cJSON_Delete(json);
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
    
    aircon_command_t command;
    if (strcmp(mode->valuestring, "cool") == 0) {
        command = AIRCON_MODE_COOL;
    } else if (strcmp(mode->valuestring, "heat") == 0) {
        command = AIRCON_MODE_HEAT;
    } else if (strcmp(mode->valuestring, "fan") == 0) {
        command = AIRCON_MODE_FAN;
    } else {
        cJSON_Delete(json);
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
    
    esp_err_t err = ir_controller_send_command(command);
    
    cJSON *response = cJSON_CreateObject();
    if (err == ESP_OK) {
        cJSON_AddStringToObject(response, "status", "success");
        cJSON_AddStringToObject(response, "message", "모드 변경 명령이 전송되었습니다");
    } else {
        cJSON_AddStringToObject(response, "status", "error");
        cJSON_AddStringToObject(response, "message", "명령 전송 실패");
    }
    
    char *response_str = cJSON_Print(response);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, response_str, strlen(response_str));
    
    free(response_str);
    cJSON_Delete(response);
    cJSON_Delete(json);
    
    return ESP_OK;
}

// WiFi 설정 API
static esp_err_t config_wifi_post_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "WiFi 설정 요청");
    
    add_cors_headers(req);
    
    if (verify_api_key(req) != ESP_OK) {
        httpd_resp_set_status(req, "401 Unauthorized");
        httpd_resp_send(req, NULL, 0);
        return ESP_OK;
    }
    
    char content[200];
    int ret = httpd_req_recv(req, content, sizeof(content) - 1);
    if (ret <= 0) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
    content[ret] = '\0';
    
    cJSON *json = cJSON_Parse(content);
    if (!json) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
    
    cJSON *ssid = cJSON_GetObjectItem(json, "ssid");
    cJSON *password = cJSON_GetObjectItem(json, "password");
    
    if (!ssid || !password || !cJSON_IsString(ssid) || !cJSON_IsString(password)) {
        cJSON_Delete(json);
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
    
    wifi_config_t config;
    strncpy(config.ssid, ssid->valuestring, sizeof(config.ssid) - 1);
    strncpy(config.password, password->valuestring, sizeof(config.password) - 1);
    
    esp_err_t err = wifi_manager_save_config(&config);
    if (err == ESP_OK) {
        err = wifi_manager_connect(config.ssid, config.password);
    }
    
    cJSON *response = cJSON_CreateObject();
    if (err == ESP_OK) {
        cJSON_AddStringToObject(response, "status", "success");
        cJSON_AddStringToObject(response, "message", "WiFi 설정이 저장되었습니다");
    } else {
        cJSON_AddStringToObject(response, "status", "error");
        cJSON_AddStringToObject(response, "message", "WiFi 설정 실패");
    }
    
    char *response_str = cJSON_Print(response);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, response_str, strlen(response_str));
    
    free(response_str);
    cJSON_Delete(response);
    cJSON_Delete(json);
    
    return ESP_OK;
}

// 설정 조회 API
static esp_err_t config_get_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "설정 조회 요청");
    
    add_cors_headers(req);
    
    if (verify_api_key(req) != ESP_OK) {
        httpd_resp_set_status(req, "401 Unauthorized");
        httpd_resp_send(req, NULL, 0);
        return ESP_OK;
    }
    
    cJSON *response = cJSON_CreateObject();
    
    // WiFi 설정 로드
    wifi_config_t wifi_config;
    if (wifi_manager_load_config(&wifi_config) == ESP_OK) {
        cJSON_AddStringToObject(response, "wifi_ssid", wifi_config.ssid);
        cJSON_AddStringToObject(response, "wifi_password", "***");  // 보안상 비밀번호는 숨김
    }
    
    char *response_str = cJSON_Print(response);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, response_str, strlen(response_str));
    
    free(response_str);
    cJSON_Delete(response);
    
    return ESP_OK;
}

// URL 핸들러 등록
static const httpd_uri_t uri_handlers[] = {
    {
        .uri = "/api/status",
        .method = HTTP_GET,
        .handler = status_get_handler,
        .user_ctx = NULL
    },
    {
        .uri = "/api/wifi",
        .method = HTTP_GET,
        .handler = wifi_get_handler,
        .user_ctx = NULL
    },
    {
        .uri = "/api/aircon/power",
        .method = HTTP_POST,
        .handler = aircon_power_post_handler,
        .user_ctx = NULL
    },
    {
        .uri = "/api/aircon/temp",
        .method = HTTP_POST,
        .handler = aircon_temp_post_handler,
        .user_ctx = NULL
    },
    {
        .uri = "/api/aircon/mode",
        .method = HTTP_POST,
        .handler = aircon_mode_post_handler,
        .user_ctx = NULL
    },
    {
        .uri = "/api/config/wifi",
        .method = HTTP_POST,
        .handler = config_wifi_post_handler,
        .user_ctx = NULL
    },
    {
        .uri = "/api/config",
        .method = HTTP_GET,
        .handler = config_get_handler,
        .user_ctx = NULL
    }
};

esp_err_t web_server_start(void)
{
    ESP_LOGI(TAG, "웹 서버 시작");
    
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.port = 80;
    config.max_uri_handlers = 20;
    
    esp_err_t ret = httpd_start(&server, &config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "웹 서버 시작 실패: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // URL 핸들러 등록
    for (int i = 0; i < sizeof(uri_handlers) / sizeof(uri_handlers[0]); i++) {
        ret = httpd_register_uri_handler(server, &uri_handlers[i]);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "URL 핸들러 등록 실패: %s", esp_err_to_name(ret));
            return ret;
        }
    }
    
    ESP_LOGI(TAG, "웹 서버 시작 완료 (포트: %d)", config.port);
    return ESP_OK;
}

esp_err_t web_server_stop(void)
{
    if (server) {
        return httpd_stop(server);
    }
    return ESP_OK;
}

esp_err_t web_server_get_port(uint16_t* port)
{
    if (!port) {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (server) {
        *port = 80;  // 기본 포트
        return ESP_OK;
    }
    
    return ESP_FAIL;
} 