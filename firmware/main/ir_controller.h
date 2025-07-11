#ifndef IR_CONTROLLER_H
#define IR_CONTROLLER_H

#include "esp_err.h"

// IR LED 핀 정의
#define IR_LED_PIN 2

// 에어컨 제어 명령
typedef enum {
    AIRCON_POWER_ON = 0,
    AIRCON_POWER_OFF,
    AIRCON_MODE_COOL,
    AIRCON_MODE_HEAT,
    AIRCON_MODE_FAN,
    AIRCON_TEMP_UP,
    AIRCON_TEMP_DOWN,
    AIRCON_FAN_SPEED_1,
    AIRCON_FAN_SPEED_2,
    AIRCON_FAN_SPEED_3
} aircon_command_t;

// IR 컨트롤러 함수들
esp_err_t ir_controller_init(void);
esp_err_t ir_controller_send_command(aircon_command_t command);
esp_err_t ir_controller_send_raw_code(uint32_t code);
esp_err_t ir_controller_learn_code(uint32_t* code);

#endif // IR_CONTROLLER_H 