#include "ws_client.h"
#include "esp_websocket_client.h"
#include "esp_log.h"
#include "string.h"

static const char *TAG = "ws_client";
static esp_websocket_client_handle_t client = NULL;
static ws_client_text_callback_t text_cb = NULL;
static ws_client_connect_callback_t on_connect_cb = NULL;
static ws_client_handshake_callback_t handshake_cb = NULL;
static bool is_connected = false;
static bool handshake_done = false;
static char ws_uri[64];

static esp_err_t send_asr_config(void)
{
    const char *json_config = 
        "{\"chunk_size\":[5,10,5],\"chunk_interval\":10,\"wav_name\":\"mimiclaw\","
        "\"wav_format\":\"pcm\",\"audio_fs\":16000,\"itn\":true,\"svs_itn\":true,\"is_speaking\":true}";
    
    ESP_LOGI(TAG, "[ASR] Sending config: %s", json_config);
    
    int ret = esp_websocket_client_send_text(client, json_config, strlen(json_config), portMAX_DELAY);
    if (ret < 0) {
        ESP_LOGE(TAG, "[ASR] Failed to send config: %d", ret);
        return ESP_FAIL;
    }
    
    handshake_done = true;
    if (handshake_cb) {
        handshake_cb(true);
    }
    return ESP_OK;
}

static esp_err_t send_asr_end(void)
{
    const char *json_end = "{\"is_speaking\":false}";
    
    ESP_LOGI(TAG, "[ASR] Sending end signal");
    
    int ret = esp_websocket_client_send_text(client, json_end, strlen(json_end), portMAX_DELAY);
    if (ret < 0) {
        ESP_LOGE(TAG, "[ASR] Failed to send end: %d", ret);
        return ESP_FAIL;
    }
    
    handshake_done = false;
    return ESP_OK;
}

static void ws_event_handler(void *handler_args, esp_event_base_t base, int32_t id, void *event_data)
{
    esp_websocket_event_data_t *data = (esp_websocket_event_data_t *)event_data;
    switch (id) {
    case WEBSOCKET_EVENT_CONNECTED:
        ESP_LOGI(TAG, "[ASR] WebSocket connected to %s", ws_uri);
        is_connected = true;
        handshake_done = false;
        send_asr_config();
        if (on_connect_cb) {
            on_connect_cb(true);
        }
        break;
    case WEBSOCKET_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "[ASR] WebSocket disconnected, data_len=%d", data->data_len);
        if (data->data_len > 0 && data->data_ptr) {
            ESP_LOGE(TAG, "[ASR] Disconnect msg: %.*s", (int)data->data_len, (char *)data->data_ptr);
        }
        is_connected = false;
        handshake_done = false;
        if (on_connect_cb) {
            on_connect_cb(false);
        }
        break;
    case WEBSOCKET_EVENT_DATA:
        ESP_LOGI(TAG, "[ASR] Received data, len=%d", data->data_len);
        if (data->data_len > 0 && data->data_ptr) {
            ESP_LOGI(TAG, "[ASR] Result: %.*s", 
                data->data_len > 256 ? 256 : (int)data->data_len, 
                (char *)data->data_ptr);
            if (text_cb) {
                text_cb((const char *)data->data_ptr);
            }
        }
        break;
    case WEBSOCKET_EVENT_ERROR:
        ESP_LOGE(TAG, "[ASR] WebSocket error");
        if (data && data->data_len > 0) {
            ESP_LOGE(TAG, "[ASR] Error data: %.*s", (int)data->data_len, (char *)data->data_ptr);
        }
        break;
    default:
        ESP_LOGD(TAG, "[ASR] Event id: %ld", id);
        break;
    }
}

esp_err_t ws_client_init(const char *host, int port)
{
    snprintf(ws_uri, sizeof(ws_uri), "ws://%s:%d", host, port);
    ESP_LOGI(TAG, "[ASR] WebSocket URI: %s", ws_uri);

    esp_websocket_client_config_t config = {
        .uri = ws_uri,
        .reconnect_timeout_ms = 30000,
        .network_timeout_ms = 30000,
        .pingpong_timeout_sec = 60,
        .disable_pingpong_discon = true,
    };

    client = esp_websocket_client_init(&config);
    if (client == NULL) {
        ESP_LOGE(TAG, "[ASR] Failed to init WebSocket client");
        return ESP_FAIL;
    }

    esp_websocket_register_events(client, WEBSOCKET_EVENT_ANY, ws_event_handler, NULL);
    return ESP_OK;
}

esp_err_t ws_client_connect(void)
{
    if (client == NULL) {
        ESP_LOGE(TAG, "[ASR] WebSocket client not initialized");
        return ESP_FAIL;
    }

    handshake_done = false;
    esp_err_t ret = esp_websocket_client_start(client);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "[ASR] Failed to start WebSocket client: %d", ret);
        return ret;
    }

    return ESP_OK;
}

esp_err_t ws_client_disconnect(void)
{
    if (client == NULL) {
        return ESP_OK;
    }

    if (is_connected && handshake_done) {
        send_asr_end();
    }

    return esp_websocket_client_stop(client);
}

esp_err_t ws_client_send_audio(const uint8_t *data, size_t len)
{
    if (client == NULL || !is_connected || !handshake_done) {
        ESP_LOGE(TAG, "[ASR] Send failed: not ready, conn=%d, handshake=%d", is_connected, handshake_done);
        return ESP_ERR_INVALID_STATE;
    }

    int ret = esp_websocket_client_send_bin(client, (const char *)data, len, pdMS_TO_TICKS(1000));
    if (ret < 0) {
        ESP_LOGE(TAG, "[ASR] Send bin failed: %d", ret);
        return ret;
    }
    
    return ESP_OK;
}

esp_err_t ws_client_register_text_callback(ws_client_text_callback_t callback)
{
    text_cb = callback;
    return ESP_OK;
}

esp_err_t ws_client_register_connect_callback(ws_client_connect_callback_t callback)
{
    on_connect_cb = callback;
    return ESP_OK;
}

esp_err_t ws_client_register_handshake_callback(ws_client_handshake_callback_t callback)
{
    handshake_cb = callback;
    return ESP_OK;
}

bool ws_client_is_connected(void)
{
    return is_connected;
}

bool ws_client_is_handshake_done(void)
{
    return handshake_done;
}
