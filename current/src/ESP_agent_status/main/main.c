#include <stdio.h>
#include <string.h>

#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_err.h"
#include "esp_heap_caps.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "status_model.h"

#define LCD_HOST SPI3_HOST
#define LCD_WIDTH 170
#define LCD_HEIGHT 320
#define LCD_X_GAP 35
#define LCD_PIXEL_CLOCK_HZ (20 * 1000 * 1000)

/* Waveshare ESP32-S3-LCD-1.9 demo values. See README before changing these. */
#define LCD_PIN_RST 9
#define LCD_PIN_SCLK 10
#define LCD_PIN_DC 11
#define LCD_PIN_CS 12
#define LCD_PIN_MOSI 13
#define LCD_PIN_BACKLIGHT 14

#define RGB565(r, g, b)                                                                 \
    ((uint16_t)((((uint16_t)(r) & 0xf8U) << 8) | (((uint16_t)(g) & 0xfcU) << 3) | \
                ((uint16_t)(b) >> 3)))

static const char *TAG = "agent_status";
static uint16_t *framebuffer;
static SemaphoreHandle_t transfer_done;
static esp_lcd_panel_handle_t panel;

/* Five columns per uppercase glyph, least-significant bit at the top. */
static const uint8_t font_5x7[26][5] = {
    {0x7e, 0x11, 0x11, 0x11, 0x7e}, {0x7f, 0x49, 0x49, 0x49, 0x36},
    {0x3e, 0x41, 0x41, 0x41, 0x22}, {0x7f, 0x41, 0x41, 0x22, 0x1c},
    {0x7f, 0x49, 0x49, 0x49, 0x41}, {0x7f, 0x09, 0x09, 0x09, 0x01},
    {0x3e, 0x41, 0x49, 0x49, 0x7a}, {0x7f, 0x08, 0x08, 0x08, 0x7f},
    {0x00, 0x41, 0x7f, 0x41, 0x00}, {0x20, 0x40, 0x41, 0x3f, 0x01},
    {0x7f, 0x08, 0x14, 0x22, 0x41}, {0x7f, 0x40, 0x40, 0x40, 0x40},
    {0x7f, 0x02, 0x0c, 0x02, 0x7f}, {0x7f, 0x04, 0x08, 0x10, 0x7f},
    {0x3e, 0x41, 0x41, 0x41, 0x3e}, {0x7f, 0x09, 0x09, 0x09, 0x06},
    {0x3e, 0x41, 0x51, 0x21, 0x5e}, {0x7f, 0x09, 0x19, 0x29, 0x46},
    {0x46, 0x49, 0x49, 0x49, 0x31}, {0x01, 0x01, 0x7f, 0x01, 0x01},
    {0x3f, 0x40, 0x40, 0x40, 0x3f}, {0x1f, 0x20, 0x40, 0x20, 0x1f},
    {0x3f, 0x40, 0x38, 0x40, 0x3f}, {0x63, 0x14, 0x08, 0x14, 0x63},
    {0x07, 0x08, 0x70, 0x08, 0x07}, {0x61, 0x51, 0x49, 0x45, 0x43},
};

static uint16_t panel_color(uint16_t color)
{
    return (uint16_t)((color << 8) | (color >> 8));
}

static void put_pixel(int x, int y, uint16_t color)
{
    if (x >= 0 && x < LCD_WIDTH && y >= 0 && y < LCD_HEIGHT) {
        framebuffer[y * LCD_WIDTH + x] = color;
    }
}

static void fill_rect(int x, int y, int width, int height, uint16_t color)
{
    for (int row = y; row < y + height; ++row) {
        for (int column = x; column < x + width; ++column) {
            put_pixel(column, row, color);
        }
    }
}

static void draw_line(int x0, int y0, int x1, int y1, int thickness, uint16_t color)
{
    int dx = x1 > x0 ? x1 - x0 : x0 - x1;
    int sx = x0 < x1 ? 1 : -1;
    int dy = -(y1 > y0 ? y1 - y0 : y0 - y1);
    int sy = y0 < y1 ? 1 : -1;
    int error = dx + dy;

    for (;;) {
        fill_rect(x0 - thickness / 2, y0 - thickness / 2, thickness, thickness, color);
        if (x0 == x1 && y0 == y1) {
            break;
        }
        const int twice_error = 2 * error;
        if (twice_error >= dy) {
            error += dy;
            x0 += sx;
        }
        if (twice_error <= dx) {
            error += dx;
            y0 += sy;
        }
    }
}

static void fill_circle(int center_x, int center_y, int radius, uint16_t color)
{
    for (int y = -radius; y <= radius; ++y) {
        for (int x = -radius; x <= radius; ++x) {
            if (x * x + y * y <= radius * radius) {
                put_pixel(center_x + x, center_y + y, color);
            }
        }
    }
}

static void draw_ring(int center_x, int center_y, int radius, int thickness, uint16_t color)
{
    const int inner = radius - thickness;
    for (int y = -radius; y <= radius; ++y) {
        for (int x = -radius; x <= radius; ++x) {
            const int distance = x * x + y * y;
            if (distance <= radius * radius && distance >= inner * inner) {
                put_pixel(center_x + x, center_y + y, color);
            }
        }
    }
}

static void draw_text(const char *text, int y, int scale, uint16_t color)
{
    const int width = (int)strlen(text) * 6 * scale - scale;
    int x = (LCD_WIDTH - width) / 2;

    for (; *text; ++text, x += 6 * scale) {
        if (*text < 'A' || *text > 'Z') {
            continue;
        }
        const uint8_t *glyph = font_5x7[*text - 'A'];
        for (int column = 0; column < 5; ++column) {
            for (int row = 0; row < 7; ++row) {
                if (glyph[column] & (1U << row)) {
                    fill_rect(x + column * scale, y + row * scale, scale, scale, color);
                }
            }
        }
    }
}

static bool notify_transfer_done(esp_lcd_panel_io_handle_t io,
                                 esp_lcd_panel_io_event_data_t *event_data, void *user_ctx)
{
    (void)io;
    (void)event_data;
    BaseType_t task_woken = pdFALSE;
    xSemaphoreGiveFromISR((SemaphoreHandle_t)user_ctx, &task_woken);
    return task_woken == pdTRUE;
}

static void render_status(agent_status_t status)
{
    uint16_t background;
    const uint16_t foreground = panel_color(RGB565(245, 247, 250));

    switch (status) {
    case AGENT_STATUS_DECISION:
        background = panel_color(RGB565(190, 112, 0));
        break;
    case AGENT_STATUS_FINISHED:
        background = panel_color(RGB565(25, 103, 57));
        break;
    case AGENT_STATUS_WORKING:
    default:
        background = panel_color(RGB565(24, 73, 126));
        break;
    }

    for (int i = 0; i < LCD_WIDTH * LCD_HEIGHT; ++i) {
        framebuffer[i] = background;
    }
    draw_text("PODLESP", 38, 2, foreground);
    draw_ring(LCD_WIDTH / 2, 145, 48, 5, foreground);

    if (status == AGENT_STATUS_DECISION) {
        draw_line(85, 119, 85, 157, 8, foreground);
        fill_circle(85, 173, 5, foreground);
    } else if (status == AGENT_STATUS_FINISHED) {
        draw_line(57, 146, 77, 166, 8, foreground);
        draw_line(77, 166, 113, 126, 8, foreground);
    } else {
        fill_circle(62, 145, 7, foreground);
        fill_circle(85, 145, 7, foreground);
        fill_circle(108, 145, 7, foreground);
    }

    draw_text(status_name(status), 225, 3, foreground);
    ESP_ERROR_CHECK(esp_lcd_panel_draw_bitmap(panel, 0, 0, LCD_WIDTH, LCD_HEIGHT, framebuffer));
    xSemaphoreTake(transfer_done, portMAX_DELAY);
}

static void init_display(void)
{
    transfer_done = xSemaphoreCreateBinary();
    ESP_ERROR_CHECK(transfer_done ? ESP_OK : ESP_ERR_NO_MEM);

    const spi_bus_config_t bus_config = {
        .mosi_io_num = LCD_PIN_MOSI,
        .miso_io_num = -1,
        .sclk_io_num = LCD_PIN_SCLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = LCD_WIDTH * LCD_HEIGHT * sizeof(uint16_t),
    };
    ESP_ERROR_CHECK(spi_bus_initialize(LCD_HOST, &bus_config, SPI_DMA_CH_AUTO));

    const esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num = LCD_PIN_DC,
        .cs_gpio_num = LCD_PIN_CS,
        .pclk_hz = LCD_PIXEL_CLOCK_HZ,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
        .spi_mode = 0,
        .trans_queue_depth = 1,
        .on_color_trans_done = notify_transfer_done,
        .user_ctx = transfer_done,
    };
    esp_lcd_panel_io_handle_t io;
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_HOST, &io_config, &io));

    const esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = LCD_PIN_RST,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
        .bits_per_pixel = 16,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(io, &panel_config, &panel));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel));
    ESP_ERROR_CHECK(esp_lcd_panel_set_gap(panel, LCD_X_GAP, 0));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel, true));

    const gpio_config_t backlight = {
        .pin_bit_mask = 1ULL << LCD_PIN_BACKLIGHT,
        .mode = GPIO_MODE_OUTPUT,
    };
    ESP_ERROR_CHECK(gpio_config(&backlight));
    ESP_ERROR_CHECK(gpio_set_level(LCD_PIN_BACKLIGHT, 0));

    framebuffer = heap_caps_malloc(LCD_WIDTH * LCD_HEIGHT * sizeof(uint16_t), MALLOC_CAP_DMA);
    ESP_ERROR_CHECK(framebuffer ? ESP_OK : ESP_ERR_NO_MEM);
}

void app_main(void)
{
    init_display();
    agent_status_t status = AGENT_STATUS_WORKING;
    render_status(status);

    setvbuf(stdin, NULL, _IONBF, 0);
    setvbuf(stdout, NULL, _IONBF, 0);
    printf("Agent status ready. Use STATUS working|decision|finished.\n");

    char line[64];
    for (;;) {
        if (!fgets(line, sizeof(line), stdin)) {
            clearerr(stdin);
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }

        if (!strchr(line, '\n')) {
            int character;
            while ((character = getchar()) != '\n' && character != EOF) {
            }
            printf("Command too long. Use STATUS working|decision|finished.\n");
            continue;
        }

        agent_status_t next;
        if (!status_parse_command(line, &next)) {
            printf("Invalid command. Use STATUS working|decision|finished.\n");
            continue;
        }
        if (next != status) {
            status = next;
            render_status(status);
            ESP_LOGI(TAG, "status=%s", status_name(status));
        }
    }
}
