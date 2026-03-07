#ifndef I2S_MIC_H
#define I2S_MIC_H

#include "esp_err.h"

#define I2S_MIC_SAMPLE_RATE    16000
#define I2S_MIC_CHANNEL_NUM    1
#define I2S_MIC_BIT_WIDTH     32

typedef void (*i2s_mic_data_callback_t)(const uint8_t *data, size_t len);

esp_err_t i2s_mic_init(void);
esp_err_t i2s_mic_start(void);
esp_err_t i2s_mic_stop(void);
esp_err_t i2s_mic_deinit(void);
esp_err_t i2s_mic_register_callback(i2s_mic_data_callback_t callback);

#endif
