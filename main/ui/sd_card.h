#pragma once

#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_vfs_fat.h"
#include "dirent.h"
#include "sdmmc_cmd.h"
#include "driver/sdmmc_host.h"
#include "esp_log.h"
#include <errno.h>
#include "esp_flash.h"

#define MOUNT_POINT "/sdcard"

// SD Card Pins (1-wire mode)
#define SD_PIN_CLK   14
#define SD_PIN_CMD   17
#define SD_PIN_D0    16

esp_err_t sd_card_init(void);
uint32_t sd_get_card_size(void);
FILE* sd_open_file(const char *file_path);
int sd_list_files(const char* directory, const char* fileExtension);

extern uint32_t flash_size;
