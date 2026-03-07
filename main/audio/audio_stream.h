#ifndef AUDIO_STREAM_H
#define AUDIO_STREAM_H

#include "esp_err.h"
#include <stdbool.h>

esp_err_t audio_stream_init(const char *funasr_host, int funasr_port);
esp_err_t audio_stream_start(void);
esp_err_t audio_stream_stop(void);
esp_err_t audio_stream_deinit(void);
bool audio_stream_is_running(void);

#endif
