#include <string.h>
#include "ui_display.h"
#include "esp_log.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_st77916.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"

static const char *TAG = "ui_display";

static esp_lcd_panel_io_handle_t panel_io_handle;
static esp_lcd_panel_handle_t panel_handle;
static bool is_initialized = false;

// Waveshare ESP32-S3-Touch-LCD-1.85C pin definitions
// https://www.waveshare.com/wiki/ESP32-S3-Touch-LCD-1.85C
#define LCD_HOST             SPI2_HOST

// QSPI interface pins
#define LCD_PIN_SCK         GPIO_NUM_40   // LCD_SCK
#define LCD_PIN_DATA0       GPIO_NUM_46   // LCD_SDA0
#define LCD_PIN_DATA1       GPIO_NUM_45   // LCD_SDA1
#define LCD_PIN_DATA2       GPIO_NUM_42   // LCD_SDA2
#define LCD_PIN_DATA3       GPIO_NUM_41   // LCD_SDA3
#define LCD_PIN_CS          GPIO_NUM_21   // LCD_CS
#define LCD_PIN_TE          GPIO_NUM_18   // LCD_TE (tearing effect, optional)

// Reset via GPIO expander (EXIO2) - use -1 for software reset only
#define LCD_PIN_RST         (-1)

// Backlight control
#define LCD_PIN_BL          GPIO_NUM_5    // LCD_BL

#define LCD_H_RES           360
#define LCD_V_RES           360
#define LCD_PIXEL_CLOCK_HZ  (40 * 1000 * 1000)
#define LCD_BIT_PER_PIXEL   16

esp_err_t ui_display_init(void) {
    if (is_initialized) {
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Initializing QSPI LCD ST77916");

    // Configure backlight GPIO
    gpio_config_t bk_gpio_config = {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = 1ULL << LCD_PIN_BL
    };
    ESP_ERROR_CHECK(gpio_config(&bk_gpio_config));
    gpio_set_level(LCD_PIN_BL, 1);

    // Initialize QSPI bus
    ESP_LOGI(TAG, "Initialize QSPI bus");
    const spi_bus_config_t buscfg = ST77916_PANEL_BUS_QSPI_CONFIG(
        LCD_PIN_SCK,
        LCD_PIN_DATA0,
        LCD_PIN_DATA1,
        LCD_PIN_DATA2,
        LCD_PIN_DATA3,
        LCD_H_RES * 80 * sizeof(uint16_t)
    );
    ESP_ERROR_CHECK(spi_bus_initialize(LCD_HOST, &buscfg, SPI_DMA_CH_AUTO));

    // Install panel IO
    ESP_LOGI(TAG, "Install panel IO");
    esp_lcd_panel_io_handle_t io_handle = NULL;
    const esp_lcd_panel_io_spi_config_t io_config = ST77916_PANEL_IO_QSPI_CONFIG(
        LCD_PIN_CS,
        NULL,
        NULL
    );
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_HOST, &io_config, &io_handle));
    panel_io_handle = io_handle;

    // Install ST77916 panel driver
    ESP_LOGI(TAG, "Install ST77916 panel driver");
    const st77916_vendor_config_t vendor_config = {
        .flags = {
            .use_qspi_interface = 1,
        },
    };
    const esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = LCD_PIN_RST,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
        .bits_per_pixel = LCD_BIT_PER_PIXEL,
        .vendor_config = &vendor_config,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_st77916(io_handle, &panel_config, &panel_handle));

    // Reset and initialize the panel
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
    
    // Turn on display
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));

    // Mirror the display (based on board orientation)
    ESP_ERROR_CHECK(esp_lcd_panel_mirror(panel_handle, true, true));

    is_initialized = true;
    ESP_LOGI(TAG, "LCD ST77916 initialized successfully");

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
    if (!is_initialized) {
        lv_display_flush_ready(disp);
        return;
    }

    int x1 = area->x1;
    int y1 = area->y1;
    int x2 = area->x2;
    int y2 = area->y2;

    uint16_t *buf = (uint16_t *)px_map;

    esp_err_t ret = esp_lcd_panel_draw_bitmap(panel_handle, x1, y1, x2 + 1, y2 + 1, buf);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "LCD draw bitmap failed: %d", ret);
    }

    lv_display_flush_ready(disp);
}
