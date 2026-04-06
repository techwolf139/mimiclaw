#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "ui_state.h"
#include "esp_log.h"

static const char *TAG = "ui_state";

ui_state_t g_ui_state;
SemaphoreHandle_t g_ui_mutex = NULL;

#define UI_STATE_LOCK() do { \
    if (g_ui_mutex != NULL) { \
        xSemaphoreTakeRecursive(g_ui_mutex, portMAX_DELAY); \
    } \
} while (0)

#define UI_STATE_UNLOCK() do { \
    if (g_ui_mutex != NULL) { \
        xSemaphoreGiveRecursive(g_ui_mutex); \
    } \
} while (0)

void ui_state_init(void) {
    memset(&g_ui_state, 0, sizeof(g_ui_state));
    g_ui_state.wifi_connected = false;
    g_ui_state.wifi_rssi = -100;
    g_ui_state.message_count = 0;
    g_ui_state.uptime_seconds = 0;
    g_ui_state.touch_enabled = true;
    
    if (g_ui_mutex == NULL) {
        g_ui_mutex = xSemaphoreCreateRecursiveMutex();
        if (g_ui_mutex == NULL) {
            ESP_LOGE(TAG, "Failed to create UI mutex");
        } else {
            ESP_LOGI(TAG, "UI mutex created");
        }
    }
}

void ui_state_update_wifi(bool connected, int8_t rssi) {
    UI_STATE_LOCK();
    g_ui_state.wifi_connected = connected;
    g_ui_state.wifi_rssi = rssi;
    UI_STATE_UNLOCK();
}

void ui_state_update_message_count(uint32_t count) {
    UI_STATE_LOCK();
    g_ui_state.message_count = count;
    UI_STATE_UNLOCK();
}

void ui_state_update_uptime(uint32_t seconds) {
    UI_STATE_LOCK();
    g_ui_state.uptime_seconds = seconds;
    UI_STATE_UNLOCK();
}

void ui_state_update_conversation(const char *role, const char *content) {
    if (role == NULL || content == NULL) return;
    
    UI_STATE_LOCK();
    for (int i = 1; i >= 0; i--) {
        if (i > 0) {
            memcpy(g_ui_state.last_conversation[i], 
                   g_ui_state.last_conversation[i-1], 128);
        } else {
            snprintf(g_ui_state.last_conversation[0], 128, "[%s]: %.100s", 
                     role, content);
        }
    }
    UI_STATE_UNLOCK();
}

void ui_state_update_activity(const char *activity) {
    if (activity == NULL) return;
    
    UI_STATE_LOCK();
    for (int i = 4; i >= 0; i--) {
        if (i > 0) {
            memcpy(g_ui_state.activity_log[i], 
                   g_ui_state.activity_log[i-1], 64);
        } else {
            snprintf(g_ui_state.activity_log[0], 64, "%.60s", activity);
        }
    }
    UI_STATE_UNLOCK();
}

void ui_state_update_touch(int16_t x, int16_t y, bool pressed) {
    UI_STATE_LOCK();
    g_ui_state.touch_x = x;
    g_ui_state.touch_y = y;
    g_ui_state.touch_pressed = pressed;
    UI_STATE_UNLOCK();
}

void ui_state_add_chat_message(const char *role, const char *content) {
    if (!role || !content) return;
    
    UI_STATE_LOCK();
    
    // Use circular buffer index
    int idx = g_ui_state.chat_msg_count % UI_CHAT_MAX_MESSAGES;
    
    ui_chat_msg_t *msg = &g_ui_state.chat_messages[idx];
    snprintf(msg->role, sizeof(msg->role), "%s", role);
    snprintf(msg->content, sizeof(msg->content), "%.255s", content);
    msg->timestamp = g_ui_state.uptime_seconds;
    
    if (g_ui_state.chat_msg_count < UI_CHAT_MAX_MESSAGES) {
        g_ui_state.chat_msg_count++;
    }
    
    UI_STATE_UNLOCK();
}

void ui_state_clear_chat(void) {
    UI_STATE_LOCK();
    g_ui_state.chat_msg_count = 0;
    g_ui_state.chat_scroll_pos = 0;
    g_ui_state.chat_active = false;
    g_ui_state.receiving_response = false;
    UI_STATE_UNLOCK();
}
