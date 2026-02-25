#include <string.h>
#include "ui_chat.h"
#include "ui_state.h"
#include "esp_log.h"

static const char *TAG = "ui_chat";

static lv_display_t *s_disp;
static lv_obj_t *s_scroll_cont;
static lv_obj_t *s_msg_container;
static bool s_initialized = false;
static bool s_visible = false;

// Colors - Dark theme optimized for round LCD
#define COLOR_BG             0x000000
#define COLOR_USER_BG        0x1A1A2E
#define COLOR_ASSIST_BG      0x0F3460
#define COLOR_USER_TEXT      0x00D4FF
#define COLOR_ASSIST_TEXT    0xFFFFFF
#define COLOR_TIMESTAMP      0x666666
#define COLOR_BUBBLE_BORDER  0x333333

esp_err_t ui_chat_init(lv_display_t *disp) {
    if (s_initialized) {
        return ESP_OK;
    }
    
    s_disp = disp;
    
    ESP_LOGI(TAG, "Initializing chat UI");
    
    // Get active screen
    lv_obj_t *scr = lv_screen_active();
    
    // Create scrollable container (full screen)
    s_scroll_cont = lv_obj_create(scr);
    lv_obj_set_size(s_scroll_cont, 360, 360);
    lv_obj_set_pos(s_scroll_cont, 0, 0);
    lv_obj_set_style_bg_color(s_scroll_cont, lv_color_hex(COLOR_BG), 0);
    lv_obj_set_style_bg_opa(s_scroll_cont, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(s_scroll_cont, 0, 0);
    lv_obj_set_style_pad_all(s_scroll_cont, 0, 0);
    
    // Enable scrolling
    lv_obj_set_scroll_dir(s_scroll_cont, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(s_scroll_cont, LV_SCROLLBAR_MODE_OFF);
    
    // Message container (vertical flex)
    s_msg_container = lv_obj_create(s_scroll_cont);
    lv_obj_set_size(s_msg_container, 360, 360);
    lv_obj_set_pos(s_msg_container, 0, 0);
    lv_obj_set_style_bg_opa(s_msg_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(s_msg_container, 0, 0);
    lv_obj_set_style_pad_all(s_msg_container, 8, 0);
    lv_obj_set_style_pad_row(s_msg_container, 8, 0);  // Add spacing between messages
    lv_obj_set_flex_flow(s_msg_container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(s_msg_container, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    
    // Hide initially
    lv_obj_add_flag(s_scroll_cont, LV_OBJ_FLAG_HIDDEN);
    
    s_initialized = true;
    ESP_LOGI(TAG, "Chat UI initialized");
    
    return ESP_OK;
}

void ui_chat_update(void) {
    if (!s_initialized || !s_visible) return;
    
    // Clear existing messages
    lv_obj_clean(s_msg_container);
    
    // Add messages from state
    for (int i = 0; i < g_ui_state.chat_msg_count; i++) {
        ui_chat_msg_t *msg = &g_ui_state.chat_messages[i];
        
        // Message bubble
        lv_obj_t *bubble = lv_obj_create(s_msg_container);
        lv_obj_set_width(bubble, 340);
        lv_obj_set_style_bg_color(bubble, lv_color_hex(
            strcmp(msg->role, "user") == 0 ? COLOR_USER_BG : COLOR_ASSIST_BG), 0);
        lv_obj_set_style_bg_opa(bubble, LV_OPA_COVER, 0);
        lv_obj_set_style_radius(bubble, 12, 0);
        lv_obj_set_style_border_width(bubble, 1, 0);
        lv_obj_set_style_border_color(bubble, lv_color_hex(COLOR_BUBBLE_BORDER), 0);
        lv_obj_set_style_pad_all(bubble, 12, 0);
        lv_obj_set_style_pad_top(bubble, 14, 0);  // More padding at top for readability
        
        // Role label - make it more prominent
        lv_obj_t *role_label = lv_label_create(bubble);
        char role_text[32];
        snprintf(role_text, sizeof(role_text), "%s:", strcmp(msg->role, "user") == 0 ? "You" : "AI");
        lv_label_set_text(role_label, role_text);
        lv_obj_set_style_text_color(role_label, lv_color_hex(
            strcmp(msg->role, "user") == 0 ? COLOR_USER_TEXT : 0x00FF88), 0);
        lv_obj_set_style_text_font(role_label, &lv_font_montserrat_14, 0);
        lv_obj_set_style_pad_bottom(role_label, 4, 0);
        
        // Content label - improved readability
        lv_obj_t *content_label = lv_label_create(bubble);
        lv_label_set_text(content_label, msg->content);
        lv_obj_set_style_text_color(content_label, lv_color_hex(COLOR_ASSIST_TEXT), 0);
        lv_obj_set_style_text_font(content_label, &lv_font_montserrat_14, 0);
        lv_obj_set_width(content_label, 316);
        lv_label_set_long_mode(content_label, LV_LABEL_LONG_WRAP);
        
        // Improve line spacing
        lv_obj_set_style_text_line_space(content_label, 4, 0);
    }
    
    // Scroll to bottom
    lv_obj_scroll_to_y(s_scroll_cont, LV_COORD_MAX, true);
}

void ui_chat_show(bool show) {
    if (!s_initialized) return;
    
    s_visible = show;
    if (show) {
        lv_obj_remove_flag(s_scroll_cont, LV_OBJ_FLAG_HIDDEN);
        ui_chat_update();
    } else {
        lv_obj_add_flag(s_scroll_cont, LV_OBJ_FLAG_HIDDEN);
    }
}

void ui_chat_scroll_down(void) {
    if (!s_initialized) return;
    lv_obj_scroll_to_y(s_scroll_cont, LV_COORD_MAX, true);
}

