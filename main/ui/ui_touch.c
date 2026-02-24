#include <string.h>
#include "ui_touch.h"
#include "esp_log.h"
#include "driver/i2c.h"

static const char *TAG = "ui_touch";

#define I2C_HOST         I2C_NUM_0
#define I2C_PIN_SDA     GPIO_NUM_11
#define I2C_PIN_SCL     GPIO_NUM_10

#define CST816_I2C_ADDR     0x15
#define CST816_REG_STATUS   0x01
#define CST816_REG_X_H      0x03
#define CST816_REG_X_L      0x04
#define CST816_REG_Y_H      0x05
#define CST816_REG_Y_L      0x06

static bool is_initialized = false;
static touch_callback_t user_callback = NULL;

static esp_err_t cst816_read_reg(uint8_t reg_addr, uint8_t *data, size_t len) {
    return i2c_master_write_read_device(
        I2C_HOST, 
        CST816_I2C_ADDR, 
        &reg_addr, 1, 
        data, len, 
        pdMS_TO_TICKS(100)
    );
}

esp_err_t ui_touch_init(void) {
    if (is_initialized) {
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Initializing CST816 touch controller");

    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_PIN_SDA,
        .scl_io_num = I2C_PIN_SCL,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 400000,
    };

    ESP_ERROR_CHECK(i2c_param_config(I2C_HOST, &conf));
    ESP_ERROR_CHECK(i2c_driver_install(I2C_HOST, conf.mode, 0, 0, 0));

    uint8_t chip_id = 0;
    cst816_read_reg(0xA7, &chip_id, 1);
    ESP_LOGI(TAG, "CST816 chip ID: 0x%02X", chip_id);

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
    cst816_read_reg(CST816_REG_STATUS, &status, 1);

    if ((status & 0x80) == 0) {
        point->pressed = false;
        return false;
    }

    uint8_t data[4] = {0};
    cst816_read_reg(CST816_REG_X_H, data, 4);

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
