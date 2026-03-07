#include "audio_stream.h"
#include "i2s_mic.h"
#include "ws_client.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdbool.h>

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
    ESP_LOGI(TAG, "[MIC] callback: conn=%d, handshake=%d, len=%d", 
        ws_connected, ws_handshake_done, len);
    
    if (!ws_connected || !ws_handshake_done) {
        ESP_LOGW(TAG, "[MIC] Skipping, not ready: conn=%d, handshake=%d", ws_connected, ws_handshake_done);
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
    } else {
        ESP_LOGI(TAG, "[MIC] Sent %d bytes to ASR", samples * 2);
    }
}

static void text_result_callback(const char *text)
{
    ESP_LOGI(TAG, "[ASR] Result: %s", text);
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
