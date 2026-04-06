#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "ui_main.h"
#include "ui_display.h"
#include "screen_manager.h"
#include "ui_state.h"
#include "ui_sound.h"
#include "ui_event.h"
#include "ui_chat.h"
#include "ui_subtitle.h"
#include "config_screen.h"
#include "ui_status.h"
#include "ui_skills.h"
#include "ui_music.h"
#include "ui_reminder.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_lcd_panel_ops.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lvgl.h"

static const char *TAG = "ui_main";

#define UI_TASK_PRIORITY   4
#define UI_TASK_STACK_SIZE 8192
#define UI_TASK_CORE       1

static void ui_event_handler(const ui_event_t *event, void *user_data);

static TaskHandle_t ui_task_handle;
static lv_display_t *disp;
static bool ui_started = false;

static screen_manager_t *s_mgr = NULL;
static lv_obj_t *main_screen_cont = NULL;
static lv_obj_t *status_text = NULL;
static lv_obj_t *time_label = NULL;
static lv_obj_t *date_label = NULL;

static void ui_task(void *arg)
{
    ESP_LOGI(TAG, "UI task started");

    uint32_t status_tick = 0;
    uint32_t time_tick = 0;

    while (1) {
        uint32_t task_delay = lv_timer_handler();
        if (task_delay == 0) {
            task_delay = 10;
        }
        
        ui_subtitle_update();

        status_tick += task_delay;
        if (status_tick >= 5000) {
            status_tick = 0;
            if (s_mgr && screen_manager_get_active(s_mgr) == SCREEN_STATUS) {
                ui_status_update();
            }
        }

        time_tick += task_delay;
        if (time_tick >= 1000) {
            time_tick = 0;
            if (time_label != NULL) {
                time_t now;
                struct tm timeinfo;
                time(&now);
                localtime_r(&now, &timeinfo);
                char time_str[16];
                strftime(time_str, sizeof(time_str), "%H:%M", &timeinfo);
                lv_label_set_text(time_label, time_str);
                if (date_label) {
                    char date_str[16];
                    strftime(date_str, sizeof(date_str), "%m/%d", &timeinfo);
                    lv_label_set_text(date_label, date_str);
                }
            }
        }

        ui_event_process(0, ui_event_handler, NULL);
        
        vTaskDelay(pdMS_TO_TICKS(task_delay));
    }
}

static void nav_btn_clicked(lv_event_t *e) {
    screen_id_t screen_id = (screen_id_t)(uintptr_t)lv_event_get_user_data(e);
    ESP_LOGI(TAG, "Navigation: screen %d", screen_id);
    
    if (s_mgr) {
        if (screen_manager_get_active(s_mgr) == screen_id) {
            screen_manager_hide_all(s_mgr);
            if (main_screen_cont) lv_obj_clear_flag(main_screen_cont, LV_OBJ_FLAG_HIDDEN);
            ui_chat_hide();
            ui_status_hide();
            ui_skills_hide();
            ui_music_hide();
            ui_reminder_hide();
            if (status_text) lv_label_set_text(status_text, "Ready");
        } else {
            if (main_screen_cont) lv_obj_add_flag(main_screen_cont, LV_OBJ_FLAG_HIDDEN);
            ui_chat_hide();
            ui_status_hide();
            ui_skills_hide();
            ui_music_hide();
            ui_reminder_hide();
            screen_manager_show(s_mgr, screen_id);
            
            switch(screen_id) {
                case SCREEN_CHAT:
                    ui_chat_show();
                    if (status_text) lv_label_set_text(status_text, "Chat");
                    break;
                case SCREEN_STATUS:
                    ui_status_show();
                    break;
                case SCREEN_SKILLS:
                    ui_skills_show();
                    break;
                case SCREEN_MUSIC:
                    ui_music_show();
                    break;
                case SCREEN_REMINDER:
                    ui_reminder_show();
                    break;
                default:
                    break;
            }
        }
    }
}

static void create_main_screen(void) {
    lv_obj_t *scr = lv_screen_active();
    if (scr == NULL) return;
    
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x0a0a0a), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);

    main_screen_cont = lv_obj_create(scr);
    if (main_screen_cont == NULL) return;
    lv_obj_set_size(main_screen_cont, 360, 360);
    lv_obj_set_pos(main_screen_cont, 0, 0);
    lv_obj_set_style_bg_opa(main_screen_cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(main_screen_cont, 0, 0);
    lv_obj_set_style_pad_all(main_screen_cont, 0, 0);
    lv_obj_set_style_radius(main_screen_cont, 180, 0);

    status_text = lv_label_create(main_screen_cont);
    if (status_text) {
        lv_label_set_text(status_text, "Ready");
        lv_obj_set_style_text_color(status_text, lv_color_hex(0xFF6666), 0);
        lv_obj_set_style_text_font(status_text, &lv_font_montserrat_14, 0);
        lv_obj_align(status_text, LV_ALIGN_CENTER, 0, -10);
    }

    time_label = lv_label_create(main_screen_cont);
    if (time_label) {
        lv_label_set_text(time_label, "00:00");
        lv_obj_set_style_text_color(time_label, lv_color_hex(0xFF8888), 0);
        lv_obj_set_style_text_font(time_label, &lv_font_montserrat_14, 0);
        lv_obj_align(time_label, LV_ALIGN_CENTER, 0, -45);
    }

    date_label = lv_label_create(main_screen_cont);
    if (date_label) {
        lv_label_set_text(date_label, "01/01");
        lv_obj_set_style_text_color(date_label, lv_color_hex(0x888888), 0);
        lv_obj_set_style_text_font(date_label, &lv_font_montserrat_14, 0);
        lv_obj_align(date_label, LV_ALIGN_CENTER, 0, -25);
    }

    const char *nav_labels[] = {"Chat", "Status", "Skills", "Music", "Reminder"};
    const uint32_t nav_colors[] = {0x00D4FF, 0xFFAA00, 0xFF66AA, 0x9966FF, 0xFF6666};
    const screen_id_t nav_screens[] = {SCREEN_CHAT, SCREEN_STATUS, SCREEN_SKILLS, SCREEN_MUSIC, SCREEN_REMINDER};
    
    int btn_width = 56;
    int btn_height = 32;
    int spacing = 8;
    int total_width = 5 * btn_width + 4 * spacing;
    int start_x = (360 - total_width) / 2;
    int btn_y = 290;
    
    for (int i = 0; i < 5; i++) {
        lv_obj_t *btn = lv_button_create(main_screen_cont);
        if (btn == NULL) continue;
        lv_obj_set_size(btn, btn_width, btn_height);
        lv_obj_set_pos(btn, start_x + i * (btn_width + spacing), btn_y);
        lv_obj_set_style_bg_color(btn, lv_color_hex(0x1a1a1a), 0);
        lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
        lv_obj_set_style_radius(btn, 16, 0);
        lv_obj_set_style_border_width(btn, 1, 0);
        lv_obj_set_style_border_color(btn, lv_color_hex(nav_colors[i]), 0);
        
        lv_obj_t *label = lv_label_create(btn);
        if (label) {
            lv_label_set_text(label, nav_labels[i]);
            lv_obj_set_style_text_color(label, lv_color_hex(nav_colors[i]), 0);
            lv_obj_set_style_text_font(label, &lv_font_montserrat_14, 0);
            lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
        }
        
        lv_obj_add_event_cb(btn, nav_btn_clicked, LV_EVENT_CLICKED, (void *)(uintptr_t)nav_screens[i]);
    }
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
    
    if (buf1 == NULL || buf2 == NULL) {
        ESP_LOGE(TAG, "Failed to allocate LVGL buffers");
        return ESP_ERR_NO_MEM;
    }

    disp = lv_display_create(LCD_H_RES, LCD_V_RES);
    if (disp == NULL) {
        ESP_LOGE(TAG, "Failed to create LVGL display");
        return ESP_FAIL;
    }
    
    lv_display_set_buffers(disp, buf1, buf2, LCD_H_RES * LCD_DRAW_BUFF_HEIGHT * sizeof(lv_color_t), LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(disp, ui_display_flush);
    lv_display_set_rotation(disp, LV_DISPLAY_ROTATION_270);
    lv_display_set_user_data(disp, ui_display_get_panel_handle());

    ui_sound_init();

    ESP_ERROR_CHECK(ui_chat_init(disp));
    ESP_ERROR_CHECK(ui_status_init(disp));
    ESP_ERROR_CHECK(ui_skills_init(disp));
    ESP_ERROR_CHECK(ui_music_init(disp));
    ESP_ERROR_CHECK(ui_reminder_init(disp));
    ESP_ERROR_CHECK(ui_subtitle_init(disp));

    lv_timer_handler();

    s_mgr = screen_manager_init(disp);
    if (s_mgr == NULL) {
        ESP_LOGE(TAG, "Failed to create screen manager");
        ui_chat_destroy();
        ui_status_destroy();
        ui_skills_destroy();
        ui_music_destroy();
        ui_reminder_destroy();
        return ESP_FAIL;
    }

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

void ui_main_set_status(const char *text)
{
    if (text == NULL) return;
    
    ui_event_data_t event_data = {0};
    event_data.status_update.text = (char *)text;
    
    esp_err_t ret = ui_event_post(UI_EVENT_STATUS_UPDATE, &event_data);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to post status event: %d", ret);
    }
}

static void ui_event_handler(const ui_event_t *event, void *user_data)
{
    if (event == NULL) return;
    
    switch (event->type) {
        case UI_EVENT_STATUS_UPDATE:
            if (status_text != NULL && event->data.status_update.text != NULL) {
                lv_label_set_text(status_text, event->data.status_update.text);
            }
            break;
            
        case UI_EVENT_LABEL_SET_TEXT:
            if (event->data.label_set_text.label_obj != NULL && 
                event->data.label_set_text.text != NULL) {
                lv_label_set_text(event->data.label_set_text.label_obj, 
                                 event->data.label_set_text.text);
            }
            break;
            
        default:
            break;
    }
}
