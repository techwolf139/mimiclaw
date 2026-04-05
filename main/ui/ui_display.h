#ifndef UI_DISPLAY_H
#define UI_DISPLAY_H

#include <stdint.h>
#include <esp_err.h>
#include "lvgl.h"
#include "esp_lcd_panel_ops.h"

#ifdef __cplusplus
extern "C" {
#endif

#define LCD_H_RES  360
#define LCD_V_RES  360
#define LCD_DRAW_BUFF_HEIGHT 36

esp_err_t ui_display_init(void);
esp_err_t ui_display_deinit(void);

void ui_display_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map);
esp_lcd_panel_handle_t ui_display_get_panel_handle(void);

#ifdef __cplusplus
}
#endif

#endif
