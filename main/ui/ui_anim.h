#ifndef UI_ANIM_H
#define UI_ANIM_H

#include "lvgl.h"
#include "screen_manager.h"
#include "esp_err.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize animation system
 */
esp_err_t ui_anim_init(void);

/**
 * Execute screen transition animation
 * @param from_screen Screen to hide (or SCREEN_COUNT for none)
 * @param to_screen Screen to show (or SCREEN_COUNT for hide all)
 * @param transition Animation type
 * @param duration_ms Animation duration
 * @param user_data Callback user data
 * @return ESP_OK on success, ESP_FAIL on error
 */
esp_err_t ui_anim_do_transition(screen_id_t from_screen,
                                screen_id_t to_screen,
                                screen_transition_t transition,
                                uint32_t duration_ms,
                                void *user_data);

/**
 * Check if any animation is currently running
 * @return true if animation is in progress
 */
bool ui_anim_is_busy(void);

/**
 * Cancel any running animation
 */
void ui_anim_cancel(void);

#ifdef __cplusplus
}
#endif

#endif // UI_ANIM_H