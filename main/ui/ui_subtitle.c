#include "ui_subtitle.h"
#include "esp_log.h"
#include "esp_timer.h"
#include <string.h>

static const char *TAG = "ui_subtitle";

static lv_obj_t *subtitle_label = NULL;
static lv_obj_t *subtitle_bg = NULL;
static bool s_initialized = false;
static int64_t hide_start_time = 0;
static bool s_visible = false;

#define SUBTITLE_DURATION_MS 5000

esp_err_t ui_subtitle_init(lv_display_t *disp)
{
    if (s_initialized) {
        return ESP_OK;
    }
    
    ESP_LOGI(TAG, "Initializing subtitle UI");
    
    lv_obj_t *scr = lv_screen_active();
    
    subtitle_bg = lv_obj_create(scr);
    lv_obj_set_size(subtitle_bg, 320, 40);
    lv_obj_set_pos(subtitle_bg, 20, 280);
    lv_obj_set_style_bg_color(subtitle_bg, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(subtitle_bg, LV_OPA_70, 0);
    lv_obj_set_style_border_width(subtitle_bg, 0, 0);
    lv_obj_set_style_radius(subtitle_bg, 20, 0);
    
    subtitle_label = lv_label_create(subtitle_bg);
    lv_label_set_text(subtitle_label, "");
    lv_obj_set_width(subtitle_label, 300);
    lv_obj_align(subtitle_label, LV_ALIGN_CENTER, 0, 0);
    lv_label_set_long_mode(subtitle_label, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_style_text_color(subtitle_label, lv_color_hex(0x00FF88), 0);
    lv_obj_set_style_text_font(subtitle_label, &lv_font_montserrat_14, 0);
    
    lv_obj_add_flag(subtitle_bg, LV_OBJ_FLAG_HIDDEN);
    
    s_initialized = true;
    ESP_LOGI(TAG, "Subtitle UI initialized");
    
    return ESP_OK;
}

void ui_subtitle_show(const char *text)
{
    if (!s_initialized || text == NULL) {
        return;
    }
    
    lv_label_set_text(subtitle_label, text);
    lv_obj_remove_flag(subtitle_bg, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_style_opa(subtitle_bg, LV_OPA_80, 0);
    
    s_visible = true;
    hide_start_time = esp_timer_get_time() / 1000;
}

void ui_subtitle_hide(void)
{
    if (!s_initialized) {
        return;
    }
    
    lv_obj_add_flag(subtitle_bg, LV_OBJ_FLAG_HIDDEN);
    s_visible = false;
}

void ui_subtitle_update(void)
{
    if (!s_initialized || !s_visible) {
        return;
    }
    
    int64_t now = esp_timer_get_time() / 1000;
    if (now - hide_start_time > SUBTITLE_DURATION_MS) {
        ui_subtitle_hide();
    }
}
