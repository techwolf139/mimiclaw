#include <string.h>
#include "ui_chat.h"
#include "ui_state.h"
#include "esp_log.h"

static const char *TAG = "ui_chat";


static lv_obj_t *s_scroll_cont;
static lv_obj_t *s_msg_container;
static bool s_initialized = false;
static bool s_visible = false;

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
    
    if (disp == NULL) {
        ESP_LOGE(TAG, "NULL display handle");
        return ESP_ERR_INVALID_ARG;
    }
    
    
    
    ESP_LOGI(TAG, "Initializing chat UI");
    
    lv_obj_t *scr = lv_screen_active();
    if (scr == NULL) {
        ESP_LOGE(TAG, "Failed to get active screen");
        return ESP_FAIL;
    }
    
    s_scroll_cont = lv_obj_create(scr);
    if (s_scroll_cont == NULL) {
        ESP_LOGE(TAG, "Failed to create scroll container");
        return ESP_FAIL;
    }
    lv_obj_set_size(s_scroll_cont, 360, 360);
    lv_obj_set_pos(s_scroll_cont, 0, 0);
    lv_obj_set_style_bg_color(s_scroll_cont, lv_color_hex(COLOR_BG), 0);
    lv_obj_set_style_bg_opa(s_scroll_cont, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(s_scroll_cont, 0, 0);
    lv_obj_set_style_pad_all(s_scroll_cont, 0, 0);
    lv_obj_set_style_radius(s_scroll_cont, 180, 0);
    
    lv_obj_set_scroll_dir(s_scroll_cont, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(s_scroll_cont, LV_SCROLLBAR_MODE_OFF);
    
    s_msg_container = lv_obj_create(s_scroll_cont);
    if (s_msg_container == NULL) {
        ESP_LOGE(TAG, "Failed to create message container");
        lv_obj_del(s_scroll_cont);
        s_scroll_cont = NULL;
        return ESP_FAIL;
    }
    lv_obj_set_size(s_msg_container, 340, LV_SIZE_CONTENT);
    lv_obj_set_pos(s_msg_container, 10, 10);
    lv_obj_set_style_bg_opa(s_msg_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(s_msg_container, 0, 0);
    lv_obj_set_style_pad_all(s_msg_container, 8, 0);
    lv_obj_set_style_pad_row(s_msg_container, 8, 0);
    lv_obj_set_flex_flow(s_msg_container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(s_msg_container, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    
    lv_obj_add_flag(s_scroll_cont, LV_OBJ_FLAG_HIDDEN);
    
    s_initialized = true;
    ESP_LOGI(TAG, "Chat UI initialized");
    
    return ESP_OK;
}

void ui_chat_destroy(void) {
    if (!s_initialized) return;
    
    ESP_LOGI(TAG, "Destroying chat UI");
    
    if (s_msg_container != NULL) {
        lv_obj_del(s_msg_container);
        s_msg_container = NULL;
    }
    
    if (s_scroll_cont != NULL) {
        lv_obj_del(s_scroll_cont);
        s_scroll_cont = NULL;
    }
    
    // Display handle not stored - using lv_screen_active()
    s_initialized = false;
    s_visible = false;
    
    ESP_LOGI(TAG, "Chat UI destroyed");
}

void ui_chat_update(void) {
    if (!s_initialized || !s_visible) return;
    
    lv_obj_clean(s_msg_container);
    
    for (int i = 0; i < g_ui_state.chat_msg_count; i++) {
        ui_chat_msg_t *msg = &g_ui_state.chat_messages[i];
        
        lv_obj_t *bubble = lv_obj_create(s_msg_container);
        if (bubble == NULL) continue;
        
        lv_obj_set_width(bubble, 320);
        lv_obj_set_style_bg_color(bubble, lv_color_hex(
            strcmp(msg->role, "user") == 0 ? COLOR_USER_BG : COLOR_ASSIST_BG), 0);
        lv_obj_set_style_bg_opa(bubble, LV_OPA_COVER, 0);
        lv_obj_set_style_radius(bubble, 12, 0);
        lv_obj_set_style_border_width(bubble, 1, 0);
        lv_obj_set_style_border_color(bubble, lv_color_hex(COLOR_BUBBLE_BORDER), 0);
        lv_obj_set_style_pad_all(bubble, 12, 0);
        lv_obj_set_style_pad_top(bubble, 14, 0);
        
        lv_obj_t *role_label = lv_label_create(bubble);
        if (role_label != NULL) {
            char role_text[32];
            snprintf(role_text, sizeof(role_text), "%s:", strcmp(msg->role, "user") == 0 ? "You" : "AI");
            lv_label_set_text(role_label, role_text);
            lv_obj_set_style_text_color(role_label, lv_color_hex(
                strcmp(msg->role, "user") == 0 ? COLOR_USER_TEXT : 0x00FF88), 0);
            lv_obj_set_style_text_font(role_label, &lv_font_montserrat_14, 0);
            lv_obj_set_style_pad_bottom(role_label, 4, 0);
        }
        
        lv_obj_t *content_label = lv_label_create(bubble);
        if (content_label != NULL) {
            lv_label_set_text(content_label, msg->content);
            lv_obj_set_style_text_color(content_label, lv_color_hex(COLOR_ASSIST_TEXT), 0);
            lv_obj_set_style_text_font(content_label, &lv_font_montserrat_14, 0);
            lv_obj_set_width(content_label, 296);
            lv_label_set_long_mode(content_label, LV_LABEL_LONG_WRAP);
            lv_obj_set_style_text_line_space(content_label, 4, 0);
        }
    }
    
    lv_obj_scroll_to_y(s_scroll_cont, LV_COORD_MAX, LV_ANIM_ON);
}

void ui_chat_show(void) {
    if (!s_initialized) return;
    
    lv_obj_clear_flag(s_scroll_cont, LV_OBJ_FLAG_HIDDEN);
    s_visible = true;
    ui_chat_update();
}

void ui_chat_hide(void) {
    if (!s_initialized) return;
    
    lv_obj_add_flag(s_scroll_cont, LV_OBJ_FLAG_HIDDEN);
    s_visible = false;
}

void ui_chat_scroll_down(void) {
    if (!s_initialized) return;
    lv_obj_scroll_to_y(s_scroll_cont, LV_COORD_MAX, LV_ANIM_ON);
}
