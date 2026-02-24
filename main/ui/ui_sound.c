#include "ui_sound.h"
#include "esp_log.h"
#include "driver/ledc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "ui_sound";

#define BEEP_GPIO GPIO_NUM_47

static bool is_initialized = false;

esp_err_t ui_sound_init(void) {
    if (is_initialized) {
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Initializing beep output");

    ledc_timer_config_t timer_conf = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_num = LEDC_TIMER_0,
        .duty_resolution = LEDC_TIMER_8_BIT,
        .freq_hz = 1000,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    ESP_ERROR_CHECK(ledc_timer_config(&timer_conf));

    ledc_channel_config_t channel_conf = {
        .gpio_num = BEEP_GPIO,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_0,
        .timer_sel = LEDC_TIMER_0,
        .duty = 0,
        .hpoint = 0,
    };
    ESP_ERROR_CHECK(ledc_channel_config(&channel_conf));

    is_initialized = true;
    ESP_LOGI(TAG, "Beep initialized on GPIO%d", BEEP_GPIO);

    return ESP_OK;
}

esp_err_t ui_sound_beep(uint32_t freq_hz, uint32_t duration_ms) {
    if (!is_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    ESP_ERROR_CHECK(ledc_set_freq(LEDC_LOW_SPEED_MODE, LEDC_TIMER_0, freq_hz));
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 128));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0));

    vTaskDelay(pdMS_TO_TICKS(duration_ms));

    ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0));

    return ESP_OK;
}
