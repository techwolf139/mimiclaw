#include "sd_card.h"

static const char *TAG = "SD_CARD";
static sdmmc_card_t *sd_card = NULL;
uint32_t flash_size = 0;

esp_err_t sd_card_init(void)
{
    esp_err_t ret;

    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = true,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024
    };

    const char mount_point[] = MOUNT_POINT;
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

    ESP_LOGI(TAG, "Mounting SD card");
    ret = esp_vfs_fat_sdmmc_mount(mount_point, &host, &slot_config, &mount_config, &sd_card);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount filesystem");
        } else {
            ESP_LOGE(TAG, "Failed to initialize the card (%s)", esp_err_to_name(ret));
        }
        return ret;
    }

    sdmmc_card_print_info(stdout, sd_card);
    ESP_LOGI(TAG, "SD card mounted at %s", mount_point);

    if (esp_flash_get_physical_size(NULL, &flash_size) == ESP_OK) {
        flash_size = flash_size / (1024 * 1024);
        ESP_LOGI(TAG, "Flash size: %lu MB", flash_size);
    }

    return ESP_OK;
}

uint32_t sd_get_card_size(void)
{
    if (sd_card == NULL) {
        return 0;
    }
    return ((uint64_t)sd_card->csd.capacity) * sd_card->csd.sector_size / (1024 * 1024);
}

FILE* sd_open_file(const char *file_path)
{
    FILE *fp = fopen(file_path, "rb");
    if (fp == NULL) {
        ESP_LOGE(TAG, "Failed to open file: %s", file_path);
    } else {
        ESP_LOGI(TAG, "Opened file: %s", file_path);
    }
    return fp;
}

int sd_list_files(const char *directory, const char *fileExtension)
{
    DIR *dir = opendir(directory);
    if (dir == NULL) {
        ESP_LOGE(TAG, "Directory not found: %s", directory);
        return -1;
    }

    int file_count = 0;
    struct dirent *entry;

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        const char *dot = strrchr(entry->d_name, '.');
        if (dot != NULL && strcasecmp(dot, fileExtension) == 0) {
            ESP_LOGI(TAG, "Found: %s/%s", directory, entry->d_name);
            file_count++;
        }
    }

    closedir(dir);
    return file_count;
}
