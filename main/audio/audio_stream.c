#include "audio_stream.h"
#include "i2s_mic.h"
#include "ws_client.h"
#include "ui/ui_state.h"
#include "ui/ui_chat.h"
#include "ui/ui_main.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdbool.h>
#include <string.h>

static const char *TAG = "audio_stream";
static bool stream_running = false;
static bool ws_connected = false;
static bool ws_handshake_done = false;

static void connect_callback(bool connected);
static void handshake_callback(bool done);
static void mic_data_callback(const uint8_t *data, size_t len);
static void text_result_callback(const char *text);

static void connect_callback(bool connected)
{
    ESP_LOGI(TAG, "[ASR] %s", connected ? "Connected to server" : "Disconnected from server");
    ws_connected = connected;
    if (!connected) {
        ws_handshake_done = false;
    }
}

static void handshake_callback(bool done)
{
    ESP_LOGI(TAG, "[ASR] Handshake %s", done ? "done" : "reset");
    ws_handshake_done = done;
}

static void mic_data_callback(const uint8_t *data, size_t len)
{
    if (!ws_connected || !ws_handshake_done) {
        return;
    }
    
    // Convert 32-bit PCM to 16-bit PCM (max 256 samples from 512 bytes)
    int16_t pcm16[256];
    uint32_t *src = (uint32_t *)data;
    size_t samples = len / 4;
    if (samples > 256) samples = 256;
    
    for (size_t i = 0; i < samples; i++) {
        pcm16[i] = (int16_t)(src[i] >> 16);
    }
    
    esp_err_t ret = ws_client_send_audio((const uint8_t *)pcm16, samples * 2);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "[MIC] Send failed: %d", ret);
    }
}

static void text_result_callback(const char *text)
{
    char *json_str = strdup(text);
    if (json_str == NULL) return;
    
    // Check if this is offline mode result
    char *mode_start = strstr(json_str, "\"mode\":\"");
    bool is_offline = false;
    if (mode_start) {
        mode_start += 7;
        if (strncmp(mode_start, "offline", 7) == 0) {
            is_offline = true;
        }
    }
    
    if (!is_offline) {
        free(json_str);
        return;
    }
    
    // Extract text field
    char *text_start = strstr(json_str, "\"text\":\"");
    if (text_start) {
        text_start += 8;
        char *text_end = strstr(text_start, "\"");
        if (text_end) {
            *text_end = '\0';
            ESP_LOGI(TAG, "[ASR] Recognized: %s", text_start);
            
            ui_state_add_chat_message("assistant", text_start);
            g_ui_state.chat_active = true;
            ui_chat_show();
            ui_chat_scroll_down();
            ui_main_set_status(text_start);
        }
    }
    
    free(json_str);
}

esp_err_t audio_stream_init(const char *funasr_host, int funasr_port)
{
    ESP_LOGI(TAG, "Initializing audio stream to %s:%d", funasr_host, funasr_port);
    
    ESP_ERROR_CHECK(i2s_mic_init());
    ESP_ERROR_CHECK(i2s_mic_register_callback(mic_data_callback));
    
    ESP_ERROR_CHECK(ws_client_init(funasr_host, funasr_port));
    ESP_ERROR_CHECK(ws_client_register_text_callback(text_result_callback));
    ESP_ERROR_CHECK(ws_client_register_connect_callback(connect_callback));
    ESP_ERROR_CHECK(ws_client_register_handshake_callback(handshake_callback));
    
    return ESP_OK;
}

esp_err_t audio_stream_start(void)
{
    if (stream_running) {
        return ESP_OK;
    }
    
    ws_connected = false;
    ws_handshake_done = false;
    
    ESP_ERROR_CHECK(ws_client_connect());
    
    int timeout = 5000;
    int waited = 0;
    while ((!ws_connected || !ws_handshake_done) && waited < timeout) {
        vTaskDelay(pdMS_TO_TICKS(100));
        waited += 100;
    }
    
    if (!ws_connected || !ws_handshake_done) {
        ESP_LOGE(TAG, "[ASR] Connection timeout, connected=%d, handshake=%d", ws_connected, ws_handshake_done);
        return ESP_ERR_TIMEOUT;
    }
    
    ESP_LOGI(TAG, "[ASR] Waiting for connection to stabilize...");
    vTaskDelay(pdMS_TO_TICKS(500));
    
    ESP_ERROR_CHECK(i2s_mic_start());
    
    stream_running = true;
    ESP_LOGI(TAG, "Audio stream started");
    return ESP_OK;
}

esp_err_t audio_stream_stop(void)
{
    if (!stream_running) {
        return ESP_OK;
    }
    
    i2s_mic_stop();
    ws_client_disconnect();
    
    stream_running = false;
    ESP_LOGI(TAG, "Audio stream stopped");
    return ESP_OK;
}

esp_err_t audio_stream_deinit(void)
{
    if (stream_running) {
        audio_stream_stop();
    }
    
    i2s_mic_deinit();
    return ESP_OK;
}

bool audio_stream_is_running(void)
{
    return stream_running;
}
