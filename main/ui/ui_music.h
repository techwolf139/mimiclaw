#ifndef UI_MUSIC_H
#define UI_MUSIC_H

#include "lvgl.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t ui_music_init(lv_display_t *disp);
void ui_music_destroy(void);
void ui_music_show(void);
void ui_music_hide(void);

#ifdef __cplusplus
}
#endif

#endif
