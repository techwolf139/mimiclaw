#include "sd_storage.h"
#include "mimi_config.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <time.h>
#include <sys/stat.h>
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "driver/sdmmc_host.h"
#include "esp_log.h"
#include "esp_err.h"
#include "cJSON.h"

static const char *TAG = "SD_STORAGE";

static sdmmc_card_t *sd_card = NULL;
static bool sd_initialized = false;

static FILE *rec_file = NULL;
static bool recording_active = false;
static uint32_t rec_bytes = 0;

sd_storage_config_t g_sd_config = {0};

static void mkdir_recursive(const char *path)
{
    char tmp[256];
    char *p = NULL;
    size_t len;
    
    snprintf(tmp, sizeof(tmp), "%s", path);
    len = strlen(tmp);
    
    if (tmp[len - 1] == '/') {
        tmp[len - 1] = 0;
    }
    
    for (p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = 0;
            mkdir(tmp, 0755);
            *p = '/';
        }
    }
    mkdir(tmp, 0755);
}

void sd_storage_set_defaults(void)
{
    g_sd_config.recording_enabled = false;
    g_sd_config.archive_enabled = false;
    g_sd_config.skills_enabled = false;
    g_sd_config.backup_enabled = false;
    g_sd_config.export_enabled = false;
    
    g_sd_config.recording.enabled = false;
    g_sd_config.recording.record_on_start = false;
    g_sd_config.recording.max_file_size_mb = 100;
    
    g_sd_config.archive.enabled = false;
    g_sd_config.archive.auto_archive = false;
    g_sd_config.archive.archive_interval_hours = 24;
    
    g_sd_config.skills.enabled = false;
    g_sd_config.skills.scan_on_start = false;
    
    g_sd_config.backup.enabled = false;
    g_sd_config.backup.auto_backup = false;
    g_sd_config.backup.backup_interval_hours = 24;
}

void sd_storage_load_config(void)
{
    const char *config_path = "/spiffs/config/sd_storage.json";
    FILE *f = fopen(config_path, "r");
    
    if (!f) {
        ESP_LOGI(TAG, "No config found, using defaults");
        return;
    }
    
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    char *content = malloc(fsize + 1);
    fread(content, 1, fsize, f);
    fclose(f);
    content[fsize] = 0;
    
    cJSON *root = cJSON_Parse(content);
    if (!root) {
        free(content);
        return;
    }
    
    cJSON *item;
    
    item = cJSON_GetObjectItem(root, "recording_enabled");
    if (item) g_sd_config.recording_enabled = item->valueint;
    
    item = cJSON_GetObjectItem(root, "archive_enabled");
    if (item) g_sd_config.archive_enabled = item->valueint;
    
    item = cJSON_GetObjectItem(root, "skills_enabled");
    if (item) g_sd_config.skills_enabled = item->valueint;
    
    item = cJSON_GetObjectItem(root, "backup_enabled");
    if (item) g_sd_config.backup_enabled = item->valueint;
    
    item = cJSON_GetObjectItem(root, "export_enabled");
    if (item) g_sd_config.export_enabled = item->valueint;
    
    item = cJSON_GetObjectItem(root, "recording");
    if (item) {
        cJSON *sub = cJSON_GetObjectItem(item, "enabled");
        if (sub) g_sd_config.recording.enabled = sub->valueint;
        sub = cJSON_GetObjectItem(item, "record_on_start");
        if (sub) g_sd_config.recording.record_on_start = sub->valueint;
    }
    
    item = cJSON_GetObjectItem(root, "archive");
    if (item) {
        cJSON *sub = cJSON_GetObjectItem(item, "enabled");
        if (sub) g_sd_config.archive.enabled = sub->valueint;
        sub = cJSON_GetObjectItem(item, "auto_archive");
        if (sub) g_sd_config.archive.auto_archive = sub->valueint;
    }
    
    item = cJSON_GetObjectItem(root, "skills");
    if (item) {
        cJSON *sub = cJSON_GetObjectItem(item, "enabled");
        if (sub) g_sd_config.skills.enabled = sub->valueint;
        sub = cJSON_GetObjectItem(item, "scan_on_start");
        if (sub) g_sd_config.skills.scan_on_start = sub->valueint;
    }
    
    item = cJSON_GetObjectItem(root, "backup");
    if (item) {
        cJSON *sub = cJSON_GetObjectItem(item, "enabled");
        if (sub) g_sd_config.backup.enabled = sub->valueint;
        sub = cJSON_GetObjectItem(item, "auto_backup");
        if (sub) g_sd_config.backup.auto_backup = sub->valueint;
    }
    
    cJSON_Delete(root);
    free(content);
    
    ESP_LOGI(TAG, "SD storage config loaded:");
    ESP_LOGI(TAG, "  Recording: %s", g_sd_config.recording_enabled ? "ON" : "OFF");
    ESP_LOGI(TAG, "  Archive: %s", g_sd_config.archive_enabled ? "ON" : "OFF");
    ESP_LOGI(TAG, "  Skills: %s", g_sd_config.skills_enabled ? "ON" : "OFF");
    ESP_LOGI(TAG, "  Backup: %s", g_sd_config.backup_enabled ? "ON" : "OFF");
    ESP_LOGI(TAG, "  Export: %s", g_sd_config.export_enabled ? "ON" : "OFF");
}

esp_err_t sd_storage_init(void)
{
    esp_err_t ret;
    
    sd_storage_set_defaults();
    
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 10,
        .allocation_unit_size = 16 * 1024
    };
    
    const char mount_point[] = SD_MOUNT_POINT;
    ESP_LOGI(TAG, "Initializing SD card");
    
    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    host.max_freq_khz = 20000;
    
    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
    slot_config.width = 1;
    slot_config.clk = SD_PIN_CLK;
    slot_config.cmd = SD_PIN_CMD;
    slot_config.d0 = SD_PIN_D0;
    slot_config.d1 = -1;
    slot_config.d2 = -1;
    slot_config.d3 = -1;
    slot_config.flags |= SDMMC_SLOT_FLAG_INTERNAL_PULLUP;
    
    ret = esp_vfs_fat_sdmmc_mount(mount_point, &host, &slot_config, &mount_config, &sd_card);
    
    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount filesystem");
        } else {
            ESP_LOGE(TAG, "Failed to initialize card (%s)", esp_err_to_name(ret));
        }
        sd_initialized = false;
        return ret;
    }
    
    sdmmc_card_print_info(stdout, sd_card);
    ESP_LOGI(TAG, "SD card mounted at %s", mount_point);
    
    if (esp_flash_get_physical_size(NULL, &flash_size) == ESP_OK) {
        flash_size = flash_size / (1024 * 1024);
        ESP_LOGI(TAG, "Flash size: %lu MB", flash_size);
    }
    
    sd_initialized = true;
    
    sd_storage_load_config();
    
    return ESP_OK;
}

bool sd_is_available(void)
{
    return sd_initialized;
}

uint32_t sd_get_card_size(void)
{
    if (!sd_card) {
        return 0;
    }
    return ((uint64_t)sd_card->csd.capacity) * sd_card->csd.sector_size / (1024 * 1024);
}

FILE* sd_open_file(const char *file_path)
{
    FILE *fp = fopen(file_path, "rb");
    if (fp == NULL) {
        ESP_LOGE(TAG, "Failed to open: %s", file_path);
    }
    return fp;
}

int sd_list_files(const char *directory, const char *fileExtension)
{
    DIR *dir = opendir(directory);
    if (!dir) {
        return -1;
    }
    
    int count = 0;
    struct dirent *entry;
    
    while ((entry = readdir(dir))) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        const char *dot = strrchr(entry->d_name, '.');
        if (dot && strcasecmp(dot, fileExtension) == 0) {
            ESP_LOGI(TAG, "Found: %s/%s", directory, entry->d_name);
            count++;
        }
    }
    
    closedir(dir);
    return count;
}

esp_err_t sd_recording_start(const char *filename)
{
    if (!sd_initialized || !g_sd_config.recording_enabled) {
        return ESP_ERR_INVALID_STATE;
    }
    
    if (recording_active) {
        sd_recording_stop();
    }
    
    char path[128];
    snprintf(path, sizeof(path), "/sdcard/recordings/%s", filename);
    
    mkdir_recursive("/sdcard/recordings");
    
    rec_file = fopen(path, "wb");
    if (!rec_file) {
        ESP_LOGE(TAG, "Failed to create recording: %s", path);
        return ESP_FAIL;
    }
    
    recording_active = true;
    rec_bytes = 0;
    
    ESP_LOGI(TAG, "Recording started: %s", path);
    
    return ESP_OK;
}

esp_err_t sd_recording_stop(void)
{
    if (!recording_active || !rec_file) {
        return ESP_ERR_INVALID_STATE;
    }
    
    fclose(rec_file);
    rec_file = NULL;
    recording_active = false;
    
    ESP_LOGI(TAG, "Recording stopped, bytes: %lu", rec_bytes);
    
    return ESP_OK;
}

bool sd_recording_is_active(void)
{
    return recording_active;
}

void sd_recording_write(const uint8_t *data, size_t len)
{
    if (recording_active && rec_file) {
        fwrite(data, 1, len, rec_file);
        rec_bytes += len;
        
        if (g_sd_config.recording.max_file_size_mb > 0 && 
            rec_bytes >= g_sd_config.recording.max_file_size_mb * 1024 * 1024) {
            sd_recording_stop();
            ESP_LOGI(TAG, "Recording auto-stopped: max size reached");
        }
    }
}

esp_err_t sd_archive_session(const char *chat_id, const char *role, const char *content)
{
    if (!sd_initialized || !g_sd_config.archive_enabled || !g_sd_config.archive.enabled) {
        return ESP_ERR_INVALID_STATE;
    }
    
    time_t now = time(NULL);
    struct tm tm;
    localtime_r(&now, &tm);
    
    char month_dir[32];
    strftime(month_dir, sizeof(month_dir), "/sdcard/archives/%Y-%m", &tm);
    
    mkdir_recursive(month_dir);
    
    char filepath[128];
    snprintf(filepath, sizeof(filepath), "%s/tg_%s.jsonl", month_dir, chat_id);
    
    FILE *f = fopen(filepath, "a");
    if (!f) {
        ESP_LOGE(TAG, "Failed to open archive: %s", filepath);
        return ESP_FAIL;
    }
    
    cJSON *obj = cJSON_CreateObject();
    cJSON_AddStringToObject(obj, "role", role);
    cJSON_AddStringToObject(obj, "content", content);
    cJSON_AddNumberToObject(obj, "ts", (double)now);
    
    char *line = cJSON_PrintUnformatted(obj);
    cJSON_Delete(obj);
    
    if (line) {
        fprintf(f, "%s\n", line);
        free(line);
    }
    
    fclose(f);
    
    return ESP_OK;
}

esp_err_t sd_load_skills_from_sd(void)
{
    if (!sd_initialized || !g_sd_config.skills_enabled || !g_sd_config.skills.enabled) {
        return ESP_ERR_INVALID_STATE;
    }
    
    const char *skill_dir = "/sdcard/skills";
    DIR *dir = opendir(skill_dir);
    
    if (!dir) {
        ESP_LOGW(TAG, "No skills directory on SD");
        return ESP_ERR_NOT_FOUND;
    }
    
    ESP_LOGI(TAG, "Loading skills from SD card");
    
    struct dirent *entry;
    while ((entry = readdir(dir))) {
        if (strstr(entry->d_name, ".md") == NULL) {
            continue;
        }
        
        char filepath[256];
        snprintf(filepath, sizeof(filepath), "%s/%s", skill_dir, entry->d_name);
        
        FILE *f = fopen(filepath, "r");
        if (!f) {
            continue;
        }
        
        fseek(f, 0, SEEK_END);
        long size = ftell(f);
        fseek(f, 0, SEEK_SET);
        
        char *content = malloc(size + 1);
        fread(content, 1, size, f);
        fclose(f);
        content[size] = 0;
        
        ESP_LOGI(TAG, "Loaded skill from SD: %s (%ld bytes)", entry->d_name, size);
        
        free(content);
    }
    
    closedir(dir);
    
    return ESP_OK;
}

static esp_err_t copy_file(const char *src, const char *dst)
{
    FILE *in = fopen(src, "rb");
    if (!in) {
        return ESP_FAIL;
    }
    
    mkdir_recursive("/sdcard/backups/temp");
    
    char dst_path[256];
    snprintf(dst_path, sizeof(dst_path), "/sdcard/backups/temp/%s", dst);
    
    char *last_slash = strrchr(dst_path, '/');
    if (last_slash) {
        *last_slash = 0;
        mkdir_recursive(dst_path);
        *last_slash = '/';
    }
    
    FILE *out = fopen(dst_path, "wb");
    if (!out) {
        fclose(in);
        return ESP_FAIL;
    }
    
    char buf[1024];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), in)) > 0) {
        fwrite(buf, 1, n, out);
    }
    
    fclose(in);
    fclose(out);
    
    return ESP_OK;
}

esp_err_t sd_backup_memory(void)
{
    if (!sd_initialized || !g_sd_config.backup_enabled || !g_sd_config.backup.enabled) {
        return ESP_ERR_INVALID_STATE;
    }
    
    time_t now = time(NULL);
    struct tm tm;
    localtime_r(&now, &tm);
    
    char backup_dir[32];
    strftime(backup_dir, sizeof(backup_dir), "/sdcard/backups/%Y-%m-%d", &tm);
    
    mkdir_recursive(backup_dir);
    
    ESP_LOGI(TAG, "Backing up memory to %s", backup_dir);
    
    copy_file("memory/MEMORY.md", "MEMORY.md");
    copy_file("config/SOUL.md", "SOUL.md");
    copy_file("config/USER.md", "USER.md");
    copy_file("cron.json", "cron.json");
    
    ESP_LOGI(TAG, "Memory backup complete");
    
    return ESP_OK;
}

esp_err_t sd_export_all(const char *export_path)
{
    if (!sd_initialized || !g_sd_config.export_enabled) {
        return ESP_ERR_INVALID_STATE;
    }
    
    time_t now = time(NULL);
    struct tm tm;
    localtime_r(&now, &tm);
    
    char export_dir[64];
    strftime(export_dir, sizeof(export_dir), export_path, &tm);
    
    mkdir_recursive(export_dir);
    
    ESP_LOGI(TAG, "Exporting all data to %s", export_dir);
    
    copy_file("memory/MEMORY.md", "memory/MEMORY.md");
    copy_file("config/SOUL.md", "config/SOUL.md");
    copy_file("config/USER.md", "config/USER.md");
    copy_file("sessions", "sessions");
    copy_file("skills", "skills");
    copy_file("cron.json", "cron.json");
    
    FILE *meta = fopen("metadata.json", "w");
    if (meta) {
        fprintf(meta, "{\n");
        fprintf(meta, "  \"export_time\": %lu,\n", (unsigned long)now);
        fprintf(meta, "  \"version\": \"1.0\"\n");
        fprintf(meta, "}\n");
        fclose(meta);
    }
    
    ESP_LOGI(TAG, "Export complete");
    
    return ESP_OK;
}
