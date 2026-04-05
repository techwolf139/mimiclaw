#include "ui_status.h"
#include "ui_state.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <inttypes.h>

static const char *TAG = "ui_status";
static lv_obj_t *s_screen = NULL;
static lv_obj_t *s_wifi_label = NULL;
static lv_obj_t *s_uptime_label = NULL;
static lv_obj_t *s_msgs_label = NULL;
static lv_obj_t *s_heap_label = NULL;
static bool s_initialized = false;
static bool s_visible = false;
static uint32_t s_boot_time_ms = 0;

esp_err_t ui_status_init(lv_display_t *disp) {
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
    
    ESP_LOGI(TAG, "Initializing status UI");
    
    s_screen = lv_obj_create(scr);
    if (s_screen == NULL) {
        ESP_LOGE(TAG, "Failed to create status screen");
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
        lv_label_set_text(title, "Status");
        lv_obj_set_style_text_color(title, lv_color_hex(0xFFAA00), 0);
        lv_obj_set_style_text_font(title, &lv_font_montserrat_14, 0);
        lv_obj_set_pos(title, 140, 25);
    }
    
    lv_obj_t *info_cont = lv_obj_create(s_screen);
    if (info_cont == NULL) {
        ESP_LOGE(TAG, "Failed to create info container");
        lv_obj_del(s_screen);
        s_screen = NULL;
        return ESP_FAIL;
    }
    lv_obj_set_size(info_cont, 320, 280);
    lv_obj_set_pos(info_cont, 20, 55);
    lv_obj_set_style_bg_opa(info_cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(info_cont, 0, 0);
    lv_obj_set_style_pad_all(info_cont, 10, 0);
    lv_obj_set_style_pad_row(info_cont, 12, 0);
    lv_obj_set_flex_flow(info_cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(info_cont, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    
    s_wifi_label = lv_label_create(info_cont);
    if (s_wifi_label != NULL) {
        lv_label_set_text(s_wifi_label, "WiFi: Initializing...");
        lv_obj_set_style_text_color(s_wifi_label, lv_color_hex(0x00D4FF), 0);
        lv_obj_set_style_text_font(s_wifi_label, &lv_font_montserrat_14, 0);
    }
    
    s_uptime_label = lv_label_create(info_cont);
    if (s_uptime_label != NULL) {
        lv_label_set_text(s_uptime_label, "Uptime: 00:00:00");
        lv_obj_set_style_text_color(s_uptime_label, lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_text_font(s_uptime_label, &lv_font_montserrat_14, 0);
    }
    
    s_msgs_label = lv_label_create(info_cont);
    if (s_msgs_label != NULL) {
        lv_label_set_text(s_msgs_label, "Messages: 0");
        lv_obj_set_style_text_color(s_msgs_label, lv_color_hex(0x00FF88), 0);
        lv_obj_set_style_text_font(s_msgs_label, &lv_font_montserrat_14, 0);
    }
    
    s_heap_label = lv_label_create(info_cont);
    if (s_heap_label != NULL) {
        lv_label_set_text(s_heap_label, "Heap: -- KB");
        lv_obj_set_style_text_color(s_heap_label, lv_color_hex(0xFF66AA), 0);
        lv_obj_set_style_text_font(s_heap_label, &lv_font_montserrat_14, 0);
    }
    
    s_boot_time_ms = (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS);
    
    lv_obj_add_flag(s_screen, LV_OBJ_FLAG_HIDDEN);
    s_initialized = true;
    ESP_LOGI(TAG, "Status UI initialized");
    return ESP_OK;
}

void ui_status_destroy(void) {
    if (!s_initialized) return;
    
    ESP_LOGI(TAG, "Destroying status UI");
    
    if (s_screen != NULL) {
        lv_obj_del(s_screen);
        s_screen = NULL;
    }
    
    s_wifi_label = NULL;
    s_uptime_label = NULL;
    s_msgs_label = NULL;
    s_heap_label = NULL;
    s_initialized = false;
    s_visible = false;
    
    ESP_LOGI(TAG, "Status UI destroyed");
}

void ui_status_update(void) {
    if (!s_initialized || !s_visible) return;
    
    if (s_wifi_label != NULL) {
        wifi_ap_record_t ap_info;
        esp_err_t err = esp_wifi_sta_get_ap_info(&ap_info);
        if (err == ESP_OK) {
            char wifi_text[64];
            snprintf(wifi_text, sizeof(wifi_text), "WiFi: %s (%d dBm)", (char *)ap_info.ssid, (int)ap_info.rssi);
            lv_label_set_text(s_wifi_label, wifi_text);
        } else {
            lv_label_set_text(s_wifi_label, "WiFi: Disconnected");
        }
    }
    
    if (s_uptime_label != NULL) {
        uint32_t now_ms = (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS);
        uint32_t uptime_s = (now_ms - s_boot_time_ms) / 1000;
        uint32_t hours = uptime_s / 3600;
        uint32_t minutes = (uptime_s % 3600) / 60;
        uint32_t seconds = uptime_s % 60;
        char uptime_text[30];
        snprintf(uptime_text, sizeof(uptime_text), "Uptime: %02" PRIu32 ":%02" PRIu32 ":%02" PRIu32, hours, minutes, seconds);
        lv_label_set_text(s_uptime_label, uptime_text);
    }
    
    if (s_msgs_label != NULL) {
        char msgs_text[30];
        snprintf(msgs_text, sizeof(msgs_text), "Messages: %d", g_ui_state.chat_msg_count);
        lv_label_set_text(s_msgs_label, msgs_text);
    }
    
    if (s_heap_label != NULL) {
        uint32_t free_heap = esp_get_free_heap_size() / 1024;
        char heap_text[30];
        snprintf(heap_text, sizeof(heap_text), "Free Heap: %" PRIu32 " KB", free_heap);
        lv_label_set_text(s_heap_label, heap_text);
    }
}

void ui_status_show(void) {
    if (!s_initialized) return;
    
    lv_obj_clear_flag(s_screen, LV_OBJ_FLAG_HIDDEN);
    s_visible = true;
    ui_status_update();
}

void ui_status_hide(void) {
    if (!s_initialized) return;
    
    lv_obj_add_flag(s_screen, LV_OBJ_FLAG_HIDDEN);
    s_visible = false;
}
