#ifndef UI_STATE_H
#define UI_STATE_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// UI状态结构
typedef struct {
    // WiFi状态
    bool wifi_connected;
    int8_t wifi_rssi;
    
    // 消息计数
    uint32_t message_count;
    
    // 运行时间 (秒)
    uint32_t uptime_seconds;
    
    // 最后对话预览 (最多2条)
    char last_conversation[2][128];
    
    // 活动时间线 (最多5条)
    char activity_log[5][64];
    
    // 触摸状态
    bool touch_enabled;
    int16_t touch_x;
    int16_t touch_y;
    bool touch_pressed;
} ui_state_t;

// 全局UI状态
extern ui_state_t g_ui_state;

// 初始化UI状态
void ui_state_init(void);

// 更新WiFi状态
void ui_state_update_wifi(bool connected, int8_t rssi);

// 更新消息计数
void ui_state_update_message_count(uint32_t count);

// 更新运行时间
void ui_state_update_uptime(uint32_t seconds);

// 更新对话预览
void ui_state_update_conversation(const char *role, const char *content);

// 更新活动时间线
void ui_state_update_activity(const char *activity);

// 更新触摸状态
void ui_state_update_touch(int16_t x, int16_t y, bool pressed);

#ifdef __cplusplus
}
#endif

#endif // UI_STATE_H
