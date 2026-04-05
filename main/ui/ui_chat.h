#ifndef UI_CHAT_H
#define UI_CHAT_H

#include <stdint.h>
#include <esp_err.h>
#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t ui_chat_init(lv_display_t *disp);
void ui_chat_destroy(void);
void ui_chat_show(void);
void ui_chat_hide(void);
void ui_chat_update(void);
void ui_chat_scroll_down(void);

#ifdef __cplusplus
}
#endif

#endif
