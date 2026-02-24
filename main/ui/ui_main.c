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

    lv_obj_t *cont = lv_obj_create(scr);
    lv_obj_set_size(cont, 360, 360);
    lv_obj_set_pos(cont, 0, 0);
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_bg_color(cont, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(cont, LV_OPA_COVER, 0);

    lv_obj_t *title = lv_label_create(cont);
    lv_label_set_text(title, "MimiClaw");
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 20);

    lv_obj_t *wifi_label = lv_label_create(cont);
    lv_label_set_text(wifi_label, "WiFi: Connecting...");
    lv_obj_set_style_text_color(wifi_label, lv_color_hex(0x888888), 0);
    lv_obj_align(wifi_label, LV_ALIGN_TOP_MID, 0, 60);

    lv_obj_t *msg_label = lv_label_create(cont);
    lv_label_set_text(msg_label, "Messages: 0");
    lv_obj_set_style_text_color(msg_label, lv_color_hex(0x888888), 0);
    lv_obj_align(msg_label, LV_ALIGN_TOP_MID, 0, 85);

    lv_obj_t *uptime_label = lv_label_create(cont);
    lv_label_set_text(uptime_label, "Uptime: 00:00:00");
    lv_obj_set_style_text_color(uptime_label, lv_color_hex(0x888888), 0);
    lv_obj_align(uptime_label, LV_ALIGN_TOP_MID, 0, 110);

    lv_obj_t *activity_label = lv_label_create(cont);
    lv_label_set_text(activity_label, "Activity:\n  Starting...");
    lv_obj_set_style_text_color(activity_label, lv_color_hex(0x888888), 0);
    lv_obj_set_width(activity_label, 300);
    lv_obj_align(activity_label, LV_ALIGN_TOP_MID, 0, 150);
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
