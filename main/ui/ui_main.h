#ifndef UI_MAIN_H
#define UI_MAIN_H

#include <stdint.h>
#include <esp_err.h>

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t ui_init(void);
esp_err_t ui_start(void);
void ui_main_set_status(const char *text);

#ifdef __cplusplus
}
#endif

#endif
