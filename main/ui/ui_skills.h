#ifndef UI_SKILLS_H
#define UI_SKILLS_H

#include "lvgl.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t ui_skills_init(lv_display_t *disp);
void ui_skills_destroy(void);
void ui_skills_show(void);
void ui_skills_hide(void);

#ifdef __cplusplus
}
#endif

#endif
