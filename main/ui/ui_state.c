#include <stdio.h>
#include <string.h>
#include "ui_state.h"

ui_state_t g_ui_state;

void ui_state_init(void) {
    memset(&g_ui_state, 0, sizeof(g_ui_state));
    g_ui_state.wifi_connected = false;
    g_ui_state.wifi_rssi = -100;
    g_ui_state.message_count = 0;
    g_ui_state.uptime_seconds = 0;
    g_ui_state.touch_enabled = true;
}

void ui_state_update_wifi(bool connected, int8_t rssi) {
    g_ui_state.wifi_connected = connected;
    g_ui_state.wifi_rssi = rssi;
}

void ui_state_update_message_count(uint32_t count) {
    g_ui_state.message_count = count;
}

void ui_state_update_uptime(uint32_t seconds) {
    g_ui_state.uptime_seconds = seconds;
}

void ui_state_update_conversation(const char *role, const char *content) {
    if (role == NULL || content == NULL) return;
    
    for (int i = 1; i >= 0; i--) {
        if (i > 0) {
            memcpy(g_ui_state.last_conversation[i], 
                   g_ui_state.last_conversation[i-1], 128);
        } else {
            snprintf(g_ui_state.last_conversation[0], 128, "[%s]: %.100s", 
                     role, content);
        }
    }
}

void ui_state_update_activity(const char *activity) {
    if (activity == NULL) return;
    
    for (int i = 4; i >= 0; i--) {
        if (i > 0) {
            memcpy(g_ui_state.activity_log[i], 
                   g_ui_state.activity_log[i-1], 64);
        } else {
            snprintf(g_ui_state.activity_log[0], 64, "%.60s", activity);
        }
    }
}

void ui_state_update_touch(int16_t x, int16_t y, bool pressed) {
    g_ui_state.touch_x = x;
    g_ui_state.touch_y = y;
    g_ui_state.touch_pressed = pressed;
}
