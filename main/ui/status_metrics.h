#ifndef STATUS_METRICS_H
#define STATUS_METRICS_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

// Metric categories
typedef enum {
    METRIC_NETWORK = 0,
    METRIC_SYSTEM,
    METRIC_PERFORMANCE,
    METRIC_APPLICATION,
    METRIC_MAX_CATEGORIES
} metric_category_t;

// Network metrics
typedef struct {
    bool connected;
    int8_t rssi;
    char ssid[32];
    uint32_t bytes_rx;
    uint32_t bytes_tx;
    uint32_t connection_time_s;
    uint8_t retry_count;
} network_metrics_t;

// System metrics
typedef struct {
    uint32_t uptime_s;
    uint32_t free_heap_kb;
    uint32_t min_free_heap_kb;
    uint32_t psram_free_kb;
    uint32_t psram_total_kb;
    float temperature_c;  // CPU temperature if available
    uint32_t reboot_count;
} system_metrics_t;

// Performance metrics
typedef struct {
    uint32_t ui_fps;
    uint32_t lvgl_tick_time_ms;
    uint32_t message_process_time_avg_ms;
    uint32_t llm_response_time_avg_ms;
    uint32_t task_counts[10];  // FreeRTOS task counts by priority
} performance_metrics_t;

// Application metrics
typedef struct {
    uint32_t chat_messages_total;
    uint32_t chat_messages_today;
    uint32_t memory_entries;
    uint32_t cron_jobs_active;
    uint32_t heartbeat_checks;
    uint32_t tool_calls;
    uint32_t web_searches;
} application_metrics_t;

// Complete metrics structure
typedef struct {
    network_metrics_t network;
    system_metrics_t system;
    performance_metrics_t performance;
    application_metrics_t application;
    uint32_t last_update_tick;
    bool initialized;
} status_metrics_t;

/**
 * Initialize metrics collection system
 * @return ESP_OK on success
 */
esp_err_t status_metrics_init(void);

/**
 * Get current metrics snapshot
 * @param metrics Output metrics structure
 * @return ESP_OK on success
 */
esp_err_t status_metrics_get(status_metrics_t *metrics);

/**
 * Update specific metric category
 * @param category Metric category to update
 * @return ESP_OK on success
 */
esp_err_t status_metrics_update(metric_category_t category);

/**
 * Get metric history for trending (last N samples)
 * @param category Metric category
 * @param history_buffer Output buffer
 * @param max_samples Maximum samples to return
 * @param value_type 0=current value, 1=delta, 2=min, 3=max
 * @return Number of samples returned
 */
int status_metrics_get_history(metric_category_t category,
                              uint32_t *history_buffer,
                              int max_samples,
                              int value_type);

/**
 * Reset all metrics counters
 */
void status_metrics_reset(void);

/**
 * Format metric for display
 * @param category Metric category
 * @param metric_name Specific metric name
 * @param buffer Output buffer
 * @param buffer_size Buffer size
 * @return true if formatted successfully
 */
bool status_metrics_format(metric_category_t category,
                          const char *metric_name,
                          char *buffer,
                          size_t buffer_size);

#ifdef __cplusplus
}
#endif

#endif // STATUS_METRICS_H