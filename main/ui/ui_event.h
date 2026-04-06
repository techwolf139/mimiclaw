#pragma once

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct _lv_obj_t;
typedef struct _lv_obj_t lv_obj_t;

#define UI_EVENT_QUEUE_SIZE 10

typedef enum {
    UI_EVENT_STATUS_UPDATE,      // Text update for status label
    UI_EVENT_CHAT_MESSAGE,       // New chat message to display  
    UI_EVENT_ANIMATION,          // Screen animation trigger
    UI_EVENT_LABEL_SET_TEXT,     // General label text update
    UI_EVENT_SCREEN_CHANGE,      // Active screen change request
    UI_EVENT_TOUCH_FEEDBACK,     // Visual feedback for touch events
} ui_event_type_t;

typedef union {
    struct {
        char *text;
    } status_update;
    
    struct {
        char *role;
        char *content;
    } chat_message;
    
    struct {
        uint8_t animation_id;
        int32_t parameter;
    } animation;
    
    struct {
        lv_obj_t *label_obj;
        char *text;
    } label_set_text;
    
    struct {
        uint8_t screen_id;
    } screen_change;
    
    struct {
        int16_t x;
        int16_t y;
        bool pressed;
    } touch_feedback;
} ui_event_data_t;

typedef struct {
    ui_event_type_t type;
    ui_event_data_t data;
} ui_event_t;

typedef void (*ui_event_handler_t)(const ui_event_t *event, void *user_data);

/**
 * Initialize the UI event queue.
 * Must be called before any UI events are posted.
 */
esp_err_t ui_event_init(void);

/**
 * Post an event to the UI event queue.
 * The function will allocate copies of any string data provided.
 * Returns ESP_OK on success, ESP_ERR_NO_MEM or ESP_ERR_TIMEOUT on failure.
 */
esp_err_t ui_event_post(ui_event_type_t type, const ui_event_data_t *data);

/**
 * Process pending events from the UI event queue.
 * This should be called from the UI task's main loop.
 * Processes up to max_events events (0 = process all pending).
 * If handler is NULL, events are simply discarded.
 */
void ui_event_process(uint32_t max_events, ui_event_handler_t handler, void *user_data);

/**
 * Deinitialize the UI event queue and free any pending events.
 */
esp_err_t ui_event_deinit(void);

#ifdef __cplusplus
}
#endif