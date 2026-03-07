#include "i2s_mic.h"
#include "driver/i2s_std.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "i2s_mic";
static i2s_chan_handle_t rx_handle = NULL;
static i2s_mic_data_callback_t data_callback = NULL;
static TaskHandle_t reader_task_handle = NULL;
static bool is_running = false;

#define I2S_MIC_WS_PIN    GPIO_NUM_2
#define I2S_MIC_SCK_PIN   GPIO_NUM_15
#define I2S_MIC_SD_PIN    GPIO_NUM_39

static void reader_task(void *arg)
{
    size_t bytes_read = 0;
    uint8_t buffer[512];
    
    while (is_running) {
        esp_err_t ret = i2s_channel_read(rx_handle, buffer, sizeof(buffer), &bytes_read, pdMS_TO_TICKS(100));
        if (ret == ESP_OK && bytes_read > 0 && data_callback) {
            data_callback(buffer, bytes_read);
        }
    }
    
    vTaskDelete(NULL);
}

esp_err_t i2s_mic_init(void)
{
    ESP_LOGI(TAG, "Initializing I2S microphone");
    
    i2s_chan_config_t chan_cfg = {
        .id = I2S_NUM_0,
        .role = I2S_ROLE_MASTER,
        .dma_desc_num = 6,
        .dma_frame_num = 256,
        .auto_clear_after_cb = true,
        .auto_clear_before_cb = true,
    };
    ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, NULL, &rx_handle));
    
    i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(I2S_MIC_SAMPLE_RATE),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_32BIT, I2S_SLOT_MODE_MONO),
        .gpio_cfg = {
            .ws = I2S_MIC_WS_PIN,
            .bclk = I2S_MIC_SCK_PIN,
            .dout = I2S_GPIO_UNUSED,
            .din = I2S_MIC_SD_PIN,
            .invert_flags = {
                .bclk_inv = false,
                .ws_inv = false,
            },
        },
    };
    
    std_cfg.slot_cfg.slot_mask = I2S_STD_SLOT_RIGHT;
    
    ESP_ERROR_CHECK(i2s_channel_init_std_mode(rx_handle, &std_cfg));
    
    ESP_LOGI(TAG, "I2S microphone initialized");
    return ESP_OK;
}

esp_err_t i2s_mic_start(void)
{
    if (is_running) {
        return ESP_OK;
    }
    
    ESP_ERROR_CHECK(i2s_channel_enable(rx_handle));
    is_running = true;
    
    BaseType_t ret = xTaskCreatePinnedToCore(
        reader_task,
        "i2s_reader",
        4096,
        NULL,
        10,
        &reader_task_handle,
        0
    );
    
    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create reader task");
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "I2S microphone started");
    return ESP_OK;
}

esp_err_t i2s_mic_stop(void)
{
    if (!is_running) {
        return ESP_OK;
    }
    
    is_running = false;
    
    if (reader_task_handle) {
        vTaskDelay(pdMS_TO_TICKS(100));
        reader_task_handle = NULL;
    }
    
    ESP_ERROR_CHECK(i2s_channel_disable(rx_handle));
    
    ESP_LOGI(TAG, "I2S microphone stopped");
    return ESP_OK;
}

esp_err_t i2s_mic_deinit(void)
{
    if (is_running) {
        i2s_mic_stop();
    }
    
    if (rx_handle) {
        ESP_ERROR_CHECK(i2s_del_channel(rx_handle));
        rx_handle = NULL;
    }
    
    ESP_LOGI(TAG, "I2S microphone deinitialized");
    return ESP_OK;
}

esp_err_t i2s_mic_register_callback(i2s_mic_data_callback_t callback)
{
    data_callback = callback;
    return ESP_OK;
}
