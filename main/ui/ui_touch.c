#include "driver/i2c.h"
#include <string.h>
#include <stdio.h>
#include "ui_touch.h"
#include "lvgl.h"
#include "esp_log.h"
#include "esp_lcd_touch.h"
#include "esp_lcd_touch_cst816s.h"

static const char *TAG = "ui_touch";

#define I2C_HOST         I2C_NUM_0
#define TOUCH_INT_PIN    GPIO_NUM_4

static bool is_initialized = false;
static esp_lcd_touch_handle_t tp = NULL;
static lv_indev_t *indev = NULL;

static void touch_read_cb(lv_indev_t *indev, lv_indev_data_t *data);

esp_err_t ui_touch_init(void) {
    if (is_initialized) {
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Initializing CST816 touch controller");

    // Configure I2C0 pins for touch (GPIO10/11)
    i2c_config_t i2c_conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = GPIO_NUM_11,
        .scl_io_num = GPIO_NUM_10,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 100000,
    };
    
    // Try to install I2C driver first
    esp_err_t ret = i2c_driver_install(I2C_HOST, I2C_MODE_MASTER, 0, 0, 0);
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        ESP_LOGW(TAG, "I2C driver install failed: %d", ret);
    }
    
    // Configure pins - this will re-configure the bus
    ret = i2c_param_config(I2C_HOST, &i2c_conf);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "I2C param config failed: %d", ret);
    }
    
    ESP_LOGI(TAG, "I2C configured: SDA=GPIO11, SCL=GPIO10");

    // Create panel IO handle for I2C
    esp_lcd_panel_io_handle_t io_handle = NULL;
    esp_lcd_panel_io_i2c_config_t io_config = {
        .dev_addr = 0x15,
        .control_phase_bytes = 1,
        .dc_bit_offset = 0,
        .lcd_cmd_bits = 8,
        .flags = {
            .disable_control_phase = 1,
        }
    };
    ret = esp_lcd_new_panel_io_i2c(I2C_HOST, &io_config, &io_handle);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to create panel IO: %d", ret);
        return ESP_OK;
    }

    // Touch configuration - use NC for RST since we can't control it directly
    // The touch controller has internal pull-up and doesn't need external reset
    esp_lcd_touch_config_t tp_cfg = {
        .x_max = 360,
        .y_max = 360,
        .rst_gpio_num = GPIO_NUM_NC,  // Not connected, use internal
        .int_gpio_num = GPIO_NUM_4,
        .levels = {
            .reset = 0,
            .interrupt = 0,
        },
        .flags = {
            .swap_xy = 0,
            .mirror_x = 0,
            .mirror_y = 0,
        },
    };

    ret = esp_lcd_touch_new_i2c_cst816s(io_handle, &tp_cfg, &tp);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to create touch: %d", ret);
        return ESP_OK;
    }

    is_initialized = true;
    ESP_LOGI(TAG, "Touch controller initialized");

    return ESP_OK;
}

esp_err_t ui_touch_deinit(void) {
    if (!is_initialized) {
        return ESP_OK;
    }

    if (tp) {
        esp_lcd_touch_del(tp);
        tp = NULL;
    }

    is_initialized = false;
    return ESP_OK;
}

lv_indev_t *ui_touch_register_lvgl_indev(lv_display_t *disp) {
    if (!is_initialized || indev != NULL) {
        return indev;
    }

    indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, touch_read_cb);
    lv_indev_set_display(indev, disp);
    lv_indev_set_user_data(indev, tp);

    ESP_LOGI(TAG, "LVGL touch input device registered");

    return indev;
}

static void touch_read_cb(lv_indev_t *indev, lv_indev_data_t *data) {
    if (!tp) {
        data->state = LV_INDEV_STATE_RELEASED;
        return;
    }

    // Read touch data
    esp_lcd_touch_read_data(tp);

    // Get touch point
    esp_lcd_touch_point_data_t touch_data[1];
    uint8_t count = 0;

    bool pressed = esp_lcd_touch_get_data(tp, touch_data, &count, 1);

    if (pressed && count > 0) {
        data->point.x = touch_data[0].x;
        data->point.y = touch_data[0].y;
        data->state = LV_INDEV_STATE_PRESSED;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}
