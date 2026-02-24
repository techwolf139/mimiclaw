#ifndef UI_TOUCH_H
#define UI_TOUCH_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CST816T_I2C_ADDR     0x15

typedef struct {
    int16_t x;
    int16_t y;
    bool pressed;
} touch_point_t;

typedef void (*touch_callback_t)(touch_point_t *point);

esp_err_t ui_touch_init(void);
esp_err_t ui_touch_deinit(void);
bool ui_touch_read(touch_point_t *point);

void ui_touch_set_callback(touch_callback_t callback);

lv_indev_t *ui_touch_register_lvgl_indev(lv_display_t *disp);

#ifdef __cplusplus
}
#endif

#endif
