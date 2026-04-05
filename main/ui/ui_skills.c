#include "ui_skills.h"
#include "esp_log.h"
#include "esp_err.h"

static const char *TAG = "ui_skills";
static lv_obj_t *s_screen = NULL;
static lv_obj_t *s_list_label = NULL;
static bool s_initialized = false;
static bool s_visible = false;

esp_err_t ui_skills_init(lv_display_t *disp) {
    if (s_initialized) return ESP_OK;
    
    if (disp == NULL) {
        ESP_LOGE(TAG, "NULL display handle");
        return ESP_ERR_INVALID_ARG;
    }
    
    lv_obj_t *scr = lv_screen_active();
    if (scr == NULL) {
        ESP_LOGE(TAG, "Failed to get active screen");
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "Initializing skills UI");
    
    s_screen = lv_obj_create(scr);
    if (s_screen == NULL) {
        ESP_LOGE(TAG, "Failed to create skills screen");
        return ESP_FAIL;
    }
    lv_obj_set_size(s_screen, 360, 360);
    lv_obj_set_pos(s_screen, 0, 0);
    lv_obj_set_style_bg_color(s_screen, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(s_screen, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(s_screen, 0, 0);
    lv_obj_set_style_pad_all(s_screen, 0, 0);
    lv_obj_set_style_radius(s_screen, 180, 0);
    
    lv_obj_t *title = lv_label_create(s_screen);
    if (title != NULL) {
        lv_label_set_text(title, "Skills");
        lv_obj_set_style_text_color(title, lv_color_hex(0xFF66AA), 0);
        lv_obj_set_style_text_font(title, &lv_font_montserrat_14, 0);
        lv_obj_set_pos(title, 140, 30);
    }
    
    s_list_label = lv_label_create(s_screen);
    if (s_list_label == NULL) {
        ESP_LOGE(TAG, "Failed to create skills list label");
        lv_obj_del(s_screen);
        s_screen = NULL;
        return ESP_FAIL;
    }
    lv_label_set_text(s_list_label, "No skills loaded");
    lv_obj_set_style_text_color(s_list_label, lv_color_hex(0x666666), 0);
    lv_obj_set_style_text_font(s_list_label, &lv_font_montserrat_14, 0);
    lv_obj_set_pos(s_list_label, 100, 150);
    
    lv_obj_add_flag(s_screen, LV_OBJ_FLAG_HIDDEN);
    s_initialized = true;
    ESP_LOGI(TAG, "Skills UI initialized");
    return ESP_OK;
}

void ui_skills_destroy(void) {
    if (!s_initialized) return;
    
    ESP_LOGI(TAG, "Destroying skills UI");
    
    if (s_screen != NULL) {
        lv_obj_del(s_screen);
        s_screen = NULL;
    }
    
    s_list_label = NULL;
    s_initialized = false;
    s_visible = false;
    
    ESP_LOGI(TAG, "Skills UI destroyed");
}

void ui_skills_show(void) {
    if (!s_initialized) return;
    lv_obj_clear_flag(s_screen, LV_OBJ_FLAG_HIDDEN);
    s_visible = true;
}

void ui_skills_hide(void) {
    if (!s_initialized) return;
    lv_obj_add_flag(s_screen, LV_OBJ_FLAG_HIDDEN);
    s_visible = false;
}
