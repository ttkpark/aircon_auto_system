#include "ir_controller.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"

static const char *TAG = "IR_CONTROLLER";

// NEC 프로토콜 타이밍 (마이크로초)
#define NEC_HDR_MARK    9000
#define NEC_HDR_SPACE   4500
#define NEC_BIT_MARK    560
#define NEC_ONE_SPACE   1690
#define NEC_ZERO_SPACE  560
#define NEC_TRAILER     560

// 에어컨 IR 코드 (예시 - 실제 에어컨에 맞게 수정 필요)
static const uint32_t aircon_codes[] = {
    0x20DF10EF,  // 전원 ON
    0x20DF10EF,  // 전원 OFF (같은 코드)
    0x20DF08F7,  // 냉방 모드
    0x20DF0CF3,  // 난방 모드
    0x20DF0EF1,  // 송풍 모드
    0x20DF40BF,  // 온도 UP
    0x20DFC03F,  // 온도 DOWN
    0x20DF8877,  // 팬 속도 1
    0x20DF48B7,  // 팬 속도 2
    0x20DFC837   // 팬 속도 3
};

esp_err_t ir_controller_init(void)
{
    ESP_LOGI(TAG, "IR 컨트롤러 초기화");
    
    // GPIO 설정
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << IR_LED_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    
    esp_err_t err = gpio_config(&io_conf);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "GPIO 설정 실패: %s", esp_err_to_name(err));
        return err;
    }
    
    // IR LED 초기 상태 (OFF)
    gpio_set_level(IR_LED_PIN, 0);
    
    ESP_LOGI(TAG, "IR 컨트롤러 초기화 완료");
    return ESP_OK;
}

// NEC 프로토콜로 IR 신호 전송
static void send_nec_code(uint32_t code)
{
    ESP_LOGI(TAG, "IR 코드 전송: 0x%08X", code);
    
    // 헤더 전송
    gpio_set_level(IR_LED_PIN, 1);
    esp_timer_delay_us(NEC_HDR_MARK);
    gpio_set_level(IR_LED_PIN, 0);
    esp_timer_delay_us(NEC_HDR_SPACE);
    
    // 32비트 데이터 전송
    for (int i = 31; i >= 0; i--) {
        gpio_set_level(IR_LED_PIN, 1);
        esp_timer_delay_us(NEC_BIT_MARK);
        gpio_set_level(IR_LED_PIN, 0);
        
        if (code & (1 << i)) {
            esp_timer_delay_us(NEC_ONE_SPACE);
        } else {
            esp_timer_delay_us(NEC_ZERO_SPACE);
        }
    }
    
    // 트레일러 전송
    gpio_set_level(IR_LED_PIN, 1);
    esp_timer_delay_us(NEC_TRAILER);
    gpio_set_level(IR_LED_PIN, 0);
}

esp_err_t ir_controller_send_command(aircon_command_t command)
{
    if (command >= sizeof(aircon_codes) / sizeof(aircon_codes[0])) {
        ESP_LOGE(TAG, "잘못된 명령: %d", command);
        return ESP_ERR_INVALID_ARG;
    }
    
    ESP_LOGI(TAG, "에어컨 명령 전송: %d", command);
    
    // IR 코드 전송 (3번 반복하여 신뢰성 향상)
    for (int i = 0; i < 3; i++) {
        send_nec_code(aircon_codes[command]);
        vTaskDelay(pdMS_TO_TICKS(100));  // 100ms 대기
    }
    
    return ESP_OK;
}

esp_err_t ir_controller_send_raw_code(uint32_t code)
{
    ESP_LOGI(TAG, "Raw IR 코드 전송: 0x%08X", code);
    send_nec_code(code);
    return ESP_OK;
}

esp_err_t ir_controller_learn_code(uint32_t* code)
{
    if (!code) {
        return ESP_ERR_INVALID_ARG;
    }
    
    ESP_LOGI(TAG, "IR 코드 학습 모드 시작");
    
    // TODO: IR 수신기 구현
    // 현재는 더미 코드 반환
    *code = 0x20DF10EF;
    
    ESP_LOGI(TAG, "학습된 코드: 0x%08X", *code);
    return ESP_OK;
} 