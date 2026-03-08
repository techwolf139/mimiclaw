#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "esp_err.h"

#define SD_MOUNT_POINT "/sdcard"

#define SD_PIN_CLK   14
#define SD_PIN_CMD   17
#define SD_PIN_D0    16

typedef struct {
    bool enabled;
    bool record_on_start;
    uint32_t max_file_size_mb;
} sd_recording_config_t;

typedef struct {
    bool enabled;
    bool auto_archive;
    uint32_t archive_interval_hours;
} sd_archive_config_t;

typedef struct {
    bool enabled;
    bool scan_on_start;
} sd_skills_config_t;

typedef struct {
    bool enabled;
    bool auto_backup;
    uint32_t backup_interval_hours;
} sd_backup_config_t;

typedef struct {
    bool recording_enabled;
    bool archive_enabled;
    bool skills_enabled;
    bool backup_enabled;
    bool export_enabled;
    
    sd_recording_config_t recording;
    sd_archive_config_t archive;
    sd_skills_config_t skills;
    sd_backup_config_t backup;
} sd_storage_config_t;

extern sd_storage_config_t g_sd_config;

void sd_storage_set_defaults(void);
void sd_storage_load_config(void);
esp_err_t sd_storage_init(void);
uint32_t sd_get_card_size(void);
FILE* sd_open_file(const char *file_path);
int sd_list_files(const char *directory, const char *fileExtension);
bool sd_is_available(void);

esp_err_t sd_recording_start(const char *filename);
esp_err_t sd_recording_stop(void);
bool sd_recording_is_active(void);
void sd_recording_write(const uint8_t *data, size_t len);

esp_err_t sd_archive_session(const char *chat_id, const char *role, const char *content);

esp_err_t sd_load_skills_from_sd(void);

esp_err_t sd_backup_memory(void);
esp_err_t sd_export_all(const char *export_path);
