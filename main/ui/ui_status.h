#ifndef UI_STATUS_H
#define UI_STATUS_H

#include "lvgl.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t ui_status_init(lv_display_t *disp);
void ui_status_destroy(void);
void ui_status_show(void);
void ui_status_hide(void);
void ui_status_update(void);

#ifdef __cplusplus
}
#endif

#endif
