#include <string.h>
#include "ui_display.h"
#include "esp_log.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"

static const char *TAG = "ui_display";

static esp_lcd_panel_io_handle_t panel_io_handle;
static esp_lcd_panel_handle_t panel_handle;
static bool is_initialized = false;

#define LCD_HOST SPI2_HOST
#define LCD_PIN_MOSI  6
#define LCD_PIN_CLK   7
#define LCD_PIN_CS    4
#define LCD_PIN_DC    5
#define LCD_PIN_RST   9
#define LCD_PIN_BL    8

#define LCD_PIXEL_CLOCK_HZ    (40 * 1000 * 1000)
#define LCD_CMD_BITS         8
#define LCD_PARAM_BITS       8

esp_err_t ui_display_init(void) {
    if (is_initialized) {
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Initializing SPI LCD");

    gpio_config_t bk_gpio_config = {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = 1ULL << LCD_PIN_BL
    };
    ESP_ERROR_CHECK(gpio_config(&bk_gpio_config));
    gpio_set_level(LCD_PIN_BL, 1);

    spi_bus_config_t buscfg = {
        .sclk_io_num = LCD_PIN_CLK,
        .mosi_io_num = LCD_PIN_MOSI,
        .miso_io_num = -1,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = LCD_H_RES * LCD_DRAW_BUFF_HEIGHT * 2 + 10,
    };
    ESP_ERROR_CHECK(spi_bus_initialize(LCD_HOST, &buscfg, SPI_DMA_CH_AUTO));

    ESP_LOGI(TAG, "Install panel IO");
    esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num = LCD_PIN_DC,
        .cs_gpio_num = LCD_PIN_CS,
        .pclk_hz = LCD_PIXEL_CLOCK_HZ,
        .lcd_cmd_bits = LCD_CMD_BITS,
        .lcd_param_bits = LCD_PARAM_BITS,
        .spi_mode = 0,
        .trans_queue_depth = 10,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_HOST, &io_config, &panel_io_handle));

    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = LCD_PIN_RST,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
        .bits_per_pixel = 16,
    };

    ESP_LOGI(TAG, "Install ST7789 panel driver");
    ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(panel_io_handle, &panel_config, &panel_handle));

    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_mirror(panel_handle, true, true));

    is_initialized = true;
    ESP_LOGI(TAG, "LCD initialized successfully");

    return ESP_OK;
}

esp_err_t ui_display_deinit(void) {
    if (!is_initialized) {
        return ESP_OK;
    }

    esp_lcd_panel_del(panel_handle);
    spi_bus_free(LCD_HOST);

    is_initialized = false;
    return ESP_OK;
}

void ui_display_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map) {
    if (!is_initialized) return;

    int x1 = area->x1;
    int y1 = area->y1;
    int x2 = area->x2;
    int y2 = area->y2;

    uint16_t *buf = (uint16_t *)px_map;

    esp_lcd_panel_draw_bitmap(panel_handle, x1, y1, x2 + 1, y2 + 1, buf);

    lv_display_flush_ready(disp);
}
