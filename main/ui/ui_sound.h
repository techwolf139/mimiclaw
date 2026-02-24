#ifndef UI_SOUND_H
#define UI_SOUND_H

#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t ui_sound_init(void);
esp_err_t ui_sound_beep(uint32_t freq_hz, uint32_t duration_ms);

#ifdef __cplusplus
}
#endif

#endif
