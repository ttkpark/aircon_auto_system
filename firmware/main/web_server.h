#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include "esp_err.h"

// 웹 서버 함수들
esp_err_t web_server_start(void);
esp_err_t web_server_stop(void);
esp_err_t web_server_get_port(uint16_t* port);

#endif // WEB_SERVER_H 