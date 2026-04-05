#ifndef SCREEN_MANAGER_H
#define SCREEN_MANAGER_H

#include "lvgl.h"
#include "esp_err.h"
#include "stdbool.h"

#ifdef __cplusplus
extern "C" {
#endif

// Supported screen types
typedef enum {
    SCREEN_CHAT = 0,
    SCREEN_STATUS = 1,
    SCREEN_SKILLS = 2,
    SCREEN_MUSIC = 3,
    SCREEN_REMINDER = 4,
    SCREEN_COUNT // Sentinel value
} screen_id_t;

// Screen manager structure
typedef struct {
    lv_disp_t *disp;
    screen_id_t active_screen;
    bool initialized;
} screen_manager_t;

// API functions
/**
 * Initialize screen manager
 * @param disp Display handle
 * @return Pointer to manager or NULL on failure
 */
screen_manager_t* screen_manager_init(lv_disp_t *disp);

/**
 * Destroy and cleanup screen manager
 * @param mgr Screen manager pointer
 */
void screen_manager_destroy(screen_manager_t *mgr);

/**
 * Show a specific screen
 * @param mgr Screen manager instance
 * @param screen_id Screen to show
 * @return ESP_OK on success, ESP_FAIL on error
 */
esp_err_t screen_manager_show(screen_manager_t *mgr, screen_id_t screen_id);

/**
 * Hide all screens
 * @param mgr Screen manager instance
 */
void screen_manager_hide_all(screen_manager_t *mgr);

/**
 * Get current active screen
 * @param mgr Screen manager instance
 * @return Screen ID or SCREEN_COUNT if invalid
 */
screen_id_t screen_manager_get_active(screen_manager_t *mgr);

#ifdef __cplusplus
}
#endif

#endif // SCREEN_MANAGER_H
