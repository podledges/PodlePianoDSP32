/* Offline boundary doubles, not an ESP-IDF compatibility or hardware test. */
#ifndef HOST_IDF_H
#define HOST_IDF_H
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

typedef int BaseType_t;
typedef void *SemaphoreHandle_t;
typedef void *esp_lcd_panel_handle_t;
typedef void *esp_lcd_panel_io_handle_t;
typedef void *esp_lcd_spi_bus_handle_t;
typedef struct { int unused; } esp_lcd_panel_io_event_data_t;
typedef struct {
    int mosi_io_num, miso_io_num, sclk_io_num, quadwp_io_num, quadhd_io_num;
    size_t max_transfer_sz;
} spi_bus_config_t;
typedef struct {
    int dc_gpio_num, cs_gpio_num, pclk_hz, lcd_cmd_bits, lcd_param_bits;
    int spi_mode, trans_queue_depth;
    bool (*on_color_trans_done)(esp_lcd_panel_io_handle_t,
                               esp_lcd_panel_io_event_data_t *, void *);
    void *user_ctx;
} esp_lcd_panel_io_spi_config_t;
typedef struct { int reset_gpio_num, rgb_ele_order, bits_per_pixel; } esp_lcd_panel_dev_config_t;
typedef struct { uint64_t pin_bit_mask; int mode; } gpio_config_t;
#define SPI3_HOST 3
#define SPI_DMA_CH_AUTO 0
#define LCD_RGB_ELEMENT_ORDER_RGB 0
#define GPIO_MODE_OUTPUT 1
#define MALLOC_CAP_DMA 0
#define ESP_OK 0
#define ESP_ERR_NO_MEM 1
#define pdFALSE 0
#define pdTRUE 1
#define portMAX_DELAY (-1)
#define pdMS_TO_TICKS(ms) (ms)
#define ESP_ERROR_CHECK(result) assert((result) == ESP_OK)
#define ESP_LOGI(tag, format, ...) printf("%s: " format "\n", tag, __VA_ARGS__)
#define heap_caps_malloc(size, caps) malloc(size)
#define spi_bus_initialize(host, config, dma) ((void)(host), (void)(config), (void)(dma), 0)
#define esp_lcd_new_panel_io_spi(host, config, result) ((void)(host), (void)(config), *(result) = (void *)1, 0)
#define esp_lcd_new_panel_st7789(io, config, result) ((void)(io), (void)(config), *(result) = (void *)1, 0)
#define esp_lcd_panel_reset(panel) ((void)(panel), 0)
#define esp_lcd_panel_init(panel) ((void)(panel), 0)
#define esp_lcd_panel_set_gap(panel, x, y) ((void)(panel), (void)(x), (void)(y), 0)
#define esp_lcd_panel_disp_on_off(panel, on) ((void)(panel), (void)(on), 0)
#define gpio_config(config) ((void)(config), 0)
#define gpio_set_level(pin, level) ((void)(pin), (void)(level), 0)
#define xSemaphoreCreateBinary() ((void *)1)
#define xSemaphoreGiveFromISR(sem, woken) ((void)(sem), *(woken) = pdFALSE)
#define xSemaphoreTake(sem, timeout) ((void)(sem), (void)(timeout))
void vTaskDelay(int ticks);
int esp_lcd_panel_draw_bitmap(esp_lcd_panel_handle_t panel, int x0, int y0,
                             int x1, int y1, const void *pixels);
#endif
