#include <string.h>
#include <stdlib.h>
#include "ui_main.h"
#include "ui_display.h"
#include "ui_touch.h"
#include "ui_state.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lvgl.h"

static const char *TAG = "ui_main";

#define UI_TASK_PRIORITY   4
#define UI_TASK_STACK_SIZE 4096
#define UI_TASK_CORE       1

static TaskHandle_t ui_task_handle;
static lv_display_t *disp;
static bool ui_started = false;

// UI Element references for updates
static lv_obj_t *wifi_label = NULL;
static lv_obj_t *msg_label = NULL;
static lv_obj_t *uptime_label = NULL;
static lv_obj_t *activity_label = NULL;
static lv_obj_t *status_arc = NULL;

static void lv_tick_task(void *arg) {
    lv_tick_inc(10);
}

static void ui_task(void *arg) {
    ESP_LOGI(TAG, "UI task started");

    while (1) {
        uint32_t task_delay = lv_timer_handler();
        if (task_delay == 0) {
            task_delay = 10;
        }
        vTaskDelay(pdMS_TO_TICKS(task_delay));
    }
}

static void create_main_screen(void) {
    lv_obj_t *scr = lv_screen_active();
    
    // Background
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);

    // Create main container
    lv_obj_t *cont = lv_obj_create(scr);
    lv_obj_set_size(cont, 360, 360);
    lv_obj_set_pos(cont, 0, 0);
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_opa(cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(cont, 0, 0);
    lv_obj_set_style_pad_all(cont, 0, 0);

    // Title
    lv_obj_t *title = lv_label_create(cont);
    lv_label_set_text(title, "MimiClaw");
    lv_obj_set_style_text_color(title, lv_color_hex(0x00D4FF), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);

    // Status Circle (circular progress indicator)
    lv_obj_t *circle_cont = lv_obj_create(cont);
    lv_obj_set_size(circle_cont, 200, 200);
    lv_obj_set_style_bg_opa(circle_cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(circle_cont, 0, 0);
    lv_obj_set_flex_flow(circle_cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(circle_cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    // Arc indicator
    status_arc = lv_arc_create(circle_cont);
    lv_obj_set_size(status_arc, 140, 140);
    lv_arc_set_rotation(status_arc, 270);
    lv_arc_set_bg_angles(status_arc, 0, 360);
    lv_arc_set_value(status_arc, 0);
    lv_obj_remove_flag(status_arc, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_arc_color(status_arc, lv_color_hex(0x333333), LV_PART_MAIN);
    lv_obj_set_style_arc_color(status_arc, lv_color_hex(0x00D4FF), LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(status_arc, 8, LV_PART_MAIN);
    lv_obj_set_style_arc_width(status_arc, 8, LV_PART_INDICATOR);

    // Center status text
    lv_obj_t *status_text = lv_label_create(circle_cont);
    lv_label_set_text(status_text, "Ready");
    lv_obj_set_style_text_color(status_text, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(status_text, &lv_font_montserrat_14, 0);

    // Status indicators container
    lv_obj_t *status_cont = lv_obj_create(cont);
    lv_obj_set_size(status_cont, 320, 80);
    lv_obj_set_style_bg_opa(status_cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(status_cont, 0, 0);
    lv_obj_set_flex_flow(status_cont, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(status_cont, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    // WiFi status
    lv_obj_t *wifi_box = lv_obj_create(status_cont);
    lv_obj_set_size(wifi_box, 90, 60);
    lv_obj_set_style_bg_opa(wifi_box, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(wifi_box, 0, 0);
    
    lv_obj_t *wifi_icon = lv_label_create(wifi_box);
    lv_label_set_text(wifi_icon, "W");
    lv_obj_set_style_text_font(wifi_icon, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(wifi_icon, lv_color_hex(0x00D4FF), 0);
    lv_obj_align(wifi_icon, LV_ALIGN_TOP_MID, 0, 0);
    
    wifi_label = lv_label_create(wifi_box);
    lv_label_set_text(wifi_label, "Connecting");
    lv_obj_set_style_text_color(wifi_label, lv_color_hex(0xFFAA00), 0);
    lv_obj_set_style_text_font(wifi_label, &lv_font_montserrat_14, 0);
    lv_obj_align(wifi_label, LV_ALIGN_BOTTOM_MID, 0, -2);

    // Messages status
    lv_obj_t *msg_box = lv_obj_create(status_cont);
    lv_obj_set_size(msg_box, 90, 60);
    lv_obj_set_style_bg_opa(msg_box, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(msg_box, 0, 0);
    
    lv_obj_t *msg_icon = lv_label_create(msg_box);
    lv_label_set_text(msg_icon, "M");
    lv_obj_set_style_text_font(msg_icon, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(msg_icon, lv_color_hex(0x00FF88), 0);
    lv_obj_align(msg_icon, LV_ALIGN_TOP_MID, 0, 0);
    
    msg_label = lv_label_create(msg_box);
    lv_label_set_text(msg_label, "0");
    lv_obj_set_style_text_color(msg_label, lv_color_hex(0x00FF88), 0);
    lv_obj_set_style_text_font(msg_label, &lv_font_montserrat_14, 0);
    lv_obj_align(msg_label, LV_ALIGN_BOTTOM_MID, 0, -2);

    // Uptime status
    lv_obj_t *time_box = lv_obj_create(status_cont);
    lv_obj_set_size(time_box, 90, 60);
    lv_obj_set_style_bg_opa(time_box, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(time_box, 0, 0);
    
    lv_obj_t *time_icon = lv_label_create(time_box);
    lv_label_set_text(time_icon, "T");
    lv_obj_set_style_text_font(time_icon, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(time_icon, lv_color_hex(0xAAAAAA), 0);
    lv_obj_align(time_icon, LV_ALIGN_TOP_MID, 0, 0);
    
    uptime_label = lv_label_create(time_box);
    lv_label_set_text(uptime_label, "00:00");
    lv_obj_set_style_text_color(uptime_label, lv_color_hex(0xAAAAAA), 0);
    lv_obj_set_style_text_font(uptime_label, &lv_font_montserrat_14, 0);
    lv_obj_align(uptime_label, LV_ALIGN_BOTTOM_MID, 0, -2);

    // Activity status
    lv_obj_t *act_box = lv_obj_create(status_cont);
    lv_obj_set_size(act_box, 90, 60);
    lv_obj_set_style_bg_opa(act_box, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(act_box, 0, 0);
    
    lv_obj_t *act_icon = lv_label_create(act_box);
    lv_label_set_text(act_icon, "A");
    lv_obj_set_style_text_font(act_icon, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(act_icon, lv_color_hex(0xFF66AA), 0);
    lv_obj_align(act_icon, LV_ALIGN_TOP_MID, 0, 0);
    
    activity_label = lv_label_create(act_box);
    lv_label_set_text(activity_label, "Idle");
    lv_obj_set_style_text_color(activity_label, lv_color_hex(0xFF66AA), 0);
    lv_obj_set_style_text_font(activity_label, &lv_font_montserrat_14, 0);
    lv_obj_align(activity_label, LV_ALIGN_BOTTOM_MID, 0, -2);
}

esp_err_t ui_init(void) {
    ESP_LOGI(TAG, "Initializing UI");

    ui_state_init();

    lv_init();

    ESP_ERROR_CHECK(ui_display_init());

    static lv_color_t *buf1 = NULL;
    static lv_color_t *buf2 = NULL;

    buf1 = heap_caps_malloc(LCD_H_RES * LCD_DRAW_BUFF_HEIGHT * sizeof(lv_color_t), MALLOC_CAP_SPIRAM);
    buf2 = heap_caps_malloc(LCD_H_RES * LCD_DRAW_BUFF_HEIGHT * sizeof(lv_color_t), MALLOC_CAP_SPIRAM);

    disp = lv_display_create(LCD_H_RES, LCD_V_RES);
    lv_display_set_buffers(disp, buf1, buf2, LCD_H_RES * LCD_DRAW_BUFF_HEIGHT * sizeof(lv_color_t), LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(disp, ui_display_flush);
    lv_display_set_rotation(disp, LV_DISPLAY_ROTATION_0);

    lv_timer_handler();

    ESP_LOGI(TAG, "Creating LVGL tick timer");

    esp_timer_create_args_t tick_args = {
        .callback = &lv_tick_task,
        .name = "lv_tick"
    };

    esp_timer_handle_t tick_timer;
    ESP_ERROR_CHECK(esp_timer_create(&tick_args, &tick_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(tick_timer, 10000));

    create_main_screen();

    ESP_LOGI(TAG, "UI initialized successfully");
    return ESP_OK;
}

esp_err_t ui_start(void) {
    if (ui_started) {
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Starting UI task");

    xTaskCreatePinnedToCore(
        ui_task,
        "ui_task",
        UI_TASK_STACK_SIZE,
        NULL,
        UI_TASK_PRIORITY,
        &ui_task_handle,
        UI_TASK_CORE
    );

    ui_started = true;
    return ESP_OK;
}
