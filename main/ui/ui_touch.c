#include <string.h>
#include "ui_touch.h"
#include "esp_log.h"
#include "driver/i2c.h"

static const char *TAG = "ui_touch";

#define I2C_HOST I2C_NUM_0
#define I2C_PIN_SDA 8
#define I2C_PIN_SCL 9

#define CST816T_REG_STATUS    0x01
#define CST816T_REG_X_POS_H   0x03
#define CST816T_REG_X_POS_L   0x04
#define CST816T_REG_Y_POS_H   0x05
#define CST816T_REG_Y_POS_L   0x06

static bool is_initialized = false;
static touch_callback_t user_callback = NULL;

esp_err_t ui_touch_init(void) {
    if (is_initialized) {
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Initializing CST816T touch controller");

    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_PIN_SDA,
        .scl_io_num = I2C_PIN_SCL,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 100000,
    };

    ESP_ERROR_CHECK(i2c_param_config(I2C_HOST, &conf));
    ESP_ERROR_CHECK(i2c_driver_install(I2C_HOST, conf.mode, 0, 0, 0));

    uint8_t check_addr = 0;
    i2c_master_read_from_device(I2C_HOST, CST816T_I2C_ADDR, &check_addr, 1, pdMS_TO_TICKS(100));
    ESP_LOGI(TAG, "CST816T touch controller initialized");

    is_initialized = true;
    return ESP_OK;
}

esp_err_t ui_touch_deinit(void) {
    if (!is_initialized) {
        return ESP_OK;
    }

    i2c_driver_delete(I2C_HOST);
    is_initialized = false;
    return ESP_OK;
}

bool ui_touch_read(touch_point_t *point) {
    if (!is_initialized || point == NULL) {
        return false;
    }

    uint8_t status = 0;
    i2c_master_read_from_device(I2C_HOST, CST816T_I2C_ADDR, &status, 1, pdMS_TO_TICKS(10));

    if ((status & 0x80) == 0) {
        point->pressed = false;
        return false;
    }

    uint8_t data[4] = {0};
    i2c_master_read_from_device(I2C_HOST, CST816T_I2C_ADDR, data, 4, pdMS_TO_TICKS(10));

    uint16_t x = ((data[0] & 0x0F) << 8) | data[1];
    uint16_t y = ((data[2] & 0x0F) << 8) | data[3];

    point->x = (int16_t)x;
    point->y = (int16_t)y;
    point->pressed = true;

    if (user_callback != NULL) {
        user_callback(point);
    }

    return true;
}

void ui_touch_set_callback(touch_callback_t callback) {
    user_callback = callback;
}
