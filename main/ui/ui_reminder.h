#ifndef UI_REMINDER_H
#define UI_REMINDER_H

#include "lvgl.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t ui_reminder_init(lv_display_t *disp);
void ui_reminder_destroy(void);
void ui_reminder_show(void);
void ui_reminder_hide(void);

#ifdef __cplusplus
}
#endif

#endif
