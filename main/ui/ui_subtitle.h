#ifndef UI_SUBTITLE_H
#define UI_SUBTITLE_H

#include <stdint.h>
#include <esp_err.h>
#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t ui_subtitle_init(lv_display_t *disp);
void ui_subtitle_show(const char *text);
void ui_subtitle_hide(void);
void ui_subtitle_update(void);

#ifdef __cplusplus
}
#endif

#endif
