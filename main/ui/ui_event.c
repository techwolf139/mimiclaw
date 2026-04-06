#include "ui_event.h"
#include "esp_log.h"
#include <string.h>
#include <stdlib.h>

static const char *TAG = "ui_event";

static QueueHandle_t s_event_queue = NULL;

static void free_event_data(ui_event_type_t type, ui_event_data_t *data)
{
    switch (type) {
        case UI_EVENT_STATUS_UPDATE:
            if (data->status_update.text) {
                free(data->status_update.text);
                data->status_update.text = NULL;
            }
            break;
        case UI_EVENT_CHAT_MESSAGE:
            if (data->chat_message.role) {
                free(data->chat_message.role);
                data->chat_message.role = NULL;
            }
            if (data->chat_message.content) {
                free(data->chat_message.content);
                data->chat_message.content = NULL;
            }
            break;
        case UI_EVENT_LABEL_SET_TEXT:
            if (data->label_set_text.text) {
                free(data->label_set_text.text);
                data->label_set_text.text = NULL;
            }
            break;
        default:
            // Other event types have no heap data
            break;
    }
}

esp_err_t ui_event_init(void)
{
    if (s_event_queue != NULL) {
        ESP_LOGW(TAG, "UI event queue already initialized");
        return ESP_OK;
    }
    
    s_event_queue = xQueueCreate(UI_EVENT_QUEUE_SIZE, sizeof(ui_event_t));
    if (s_event_queue == NULL) {
        ESP_LOGE(TAG, "Failed to create UI event queue");
        return ESP_ERR_NO_MEM;
    }
    
    ESP_LOGI(TAG, "UI event queue initialized (size %d)", UI_EVENT_QUEUE_SIZE);
    return ESP_OK;
}

esp_err_t ui_event_post(ui_event_type_t type, const ui_event_data_t *data)
{
    if (s_event_queue == NULL) {
        ESP_LOGE(TAG, "UI event queue not initialized");
        return ESP_ERR_INVALID_STATE;
    }
    
    ui_event_t event = {
        .type = type,
        .data = *data
    };
    
    // Make copies of any heap-allocated strings
    switch (type) {
        case UI_EVENT_STATUS_UPDATE:
            if (data->status_update.text) {
                event.data.status_update.text = strdup(data->status_update.text);
                if (event.data.status_update.text == NULL) {
                    ESP_LOGE(TAG, "Failed to duplicate status text");
                    return ESP_ERR_NO_MEM;
                }
            } else {
                event.data.status_update.text = NULL;
            }
            break;
            
        case UI_EVENT_CHAT_MESSAGE:
            if (data->chat_message.role) {
                event.data.chat_message.role = strdup(data->chat_message.role);
                if (event.data.chat_message.role == NULL) {
                    ESP_LOGE(TAG, "Failed to duplicate role");
                    return ESP_ERR_NO_MEM;
                }
            } else {
                event.data.chat_message.role = NULL;
            }
            
            if (data->chat_message.content) {
                event.data.chat_message.content = strdup(data->chat_message.content);
                if (event.data.chat_message.content == NULL) {
                    free(event.data.chat_message.role);
                    ESP_LOGE(TAG, "Failed to duplicate content");
                    return ESP_ERR_NO_MEM;
                }
            } else {
                event.data.chat_message.content = NULL;
            }
            break;
            
        case UI_EVENT_LABEL_SET_TEXT:
            if (data->label_set_text.text) {
                event.data.label_set_text.text = strdup(data->label_set_text.text);
                if (event.data.label_set_text.text == NULL) {
                    ESP_LOGE(TAG, "Failed to duplicate label text");
                    return ESP_ERR_NO_MEM;
                }
            } else {
                event.data.label_set_text.text = NULL;
            }
            break;
            
        default:
            // No heap data to copy
            break;
    }
    
    // Send to queue with reasonable timeout
    TickType_t timeout = pdMS_TO_TICKS(100);
    if (xQueueSend(s_event_queue, &event, timeout) != pdTRUE) {
        // Failed to send, clean up any copied data
        free_event_data(type, &event.data);
        ESP_LOGW(TAG, "UI event queue full, dropping event type %d", type);
        return ESP_ERR_TIMEOUT;
    }
    
    return ESP_OK;
}

void ui_event_process(uint32_t max_events, ui_event_handler_t handler, void *user_data)
{
    if (s_event_queue == NULL) {
        return;
    }
    
    uint32_t processed = 0;
    ui_event_t event;
    
    while ((max_events == 0 || processed < max_events) &&
           xQueueReceive(s_event_queue, &event, 0) == pdTRUE) {
        
        if (handler != NULL) {
            handler(&event, user_data);
        }
        
        free_event_data(event.type, &event.data);
        
        processed++;
    }
    
    if (processed > 0) {
        ESP_LOGD(TAG, "Processed %u UI events", processed);
    }
}

esp_err_t ui_event_deinit(void)
{
    if (s_event_queue == NULL) {
        return ESP_OK;
    }
    
    // Process and discard any remaining events
    ui_event_process(0); // Process all pending
    
    vQueueDelete(s_event_queue);
    s_event_queue = NULL;
    
    ESP_LOGI(TAG, "UI event queue deinitialized");
    return ESP_OK;
}