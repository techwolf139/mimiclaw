#include "screen_manager.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "screen_mgr";

screen_manager_t* screen_manager_init(lv_disp_t *disp) {
    if (disp == NULL) {
        ESP_LOGE(TAG, "Invalid display handle");
        return NULL;
    }

    ESP_LOGI(TAG, "Initializing screen manager");

    screen_manager_t *mgr = calloc(1, sizeof(screen_manager_t));
    if (mgr == NULL) {
        ESP_LOGE(TAG, "Failed to allocate memory for screen manager");
        return NULL;
    }

    mgr->disp = disp;
    mgr->active_screen = SCREEN_COUNT;
    mgr->previous_screen = SCREEN_COUNT;
    mgr->transition_type = SCREEN_TRANSITION_FADE;
    mgr->initialized = true;
    mgr->animating = false;

    ESP_LOGI(TAG, "Screen manager initialized successfully");
    return mgr;
}

void screen_manager_destroy(screen_manager_t *mgr) {
    if (mgr == NULL) return;

    ESP_LOGI(TAG, "Destroying screen manager");
    mgr->disp = NULL;
    mgr->active_screen = SCREEN_COUNT;
    mgr->initialized = false;
    free(mgr);

    ESP_LOGI(TAG, "Screen manager destroyed");
}

esp_err_t screen_manager_set_transition(screen_manager_t *mgr,
                                        screen_transition_t transition,
                                        uint32_t duration_ms) {
    if (mgr == NULL || !mgr->initialized) {
        return ESP_FAIL;
    }

    if (transition >= SCREEN_TRANSITION_MAX) {
        return ESP_ERR_INVALID_ARG;
    }

    mgr->transition_type = transition;
    ESP_LOGI(TAG, "Set transition type: %d, duration: %dms", transition, duration_ms);
    return ESP_OK;
}

bool screen_manager_is_animating(screen_manager_t *mgr) {
    if (mgr == NULL) return false;
    return mgr->animating;
}

esp_err_t screen_manager_show(screen_manager_t *mgr, screen_id_t screen_id) {
    if (mgr == NULL || !mgr->initialized) {
        return ESP_FAIL;
    }

    if (screen_id < 0 || screen_id >= SCREEN_COUNT) {
        return ESP_ERR_INVALID_ARG;
    }

    mgr->previous_screen = mgr->active_screen;
    mgr->active_screen = screen_id;
    mgr->animating = true;
    
    ESP_LOGI(TAG, "Screen transition: %d -> %d (transition: %d)", 
             mgr->previous_screen, mgr->active_screen, mgr->transition_type);
    
    // For now, animation is immediate - will be enhanced later
    mgr->animating = false;
    
    return ESP_OK;
}

void screen_manager_hide_all(screen_manager_t *mgr) {
    if (mgr == NULL || !mgr->initialized) return;
    mgr->active_screen = SCREEN_COUNT;
}

screen_id_t screen_manager_get_active(screen_manager_t *mgr) {
    if (mgr == NULL) return SCREEN_COUNT;
    return mgr->active_screen;
}
