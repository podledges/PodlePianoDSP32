/* Run the unchanged app loop with complete stdin lines and a captured LCD sink.
 * Does not emulate USB FIFO timing, DMA, panel initialization, or physical wiring.
 */
#include <setjmp.h>
#include "host_idf.h"
#include "../main/main.c"

static jmp_buf end_of_input;
static unsigned frames;
static const char *output_dir;
static const uint16_t backgrounds[] = {0x1a4f, 0xbb80, 0x1b27, 0x1a4f};

void vTaskDelay(int ticks)
{
    assert(ticks == 100);
    longjmp(end_of_input, 1);
}

int esp_lcd_panel_draw_bitmap(esp_lcd_panel_handle_t handle, int x0, int y0,
                             int x1, int y1, const void *pixels)
{
    (void)handle;
    assert(x0 == 0 && y0 == 0 && x1 == 170 && y1 == 320);
    assert(frames < 4);
    const unsigned char *bytes = pixels;
    assert(((unsigned)bytes[0] << 8 | bytes[1]) == backgrounds[frames]);
    char path[1024];
    assert(snprintf(path, sizeof(path), "%s/frame-%u.ppm", output_dir, frames) < (int)sizeof(path));
    FILE *file = fopen(path, "wb");
    assert(file);
    fprintf(file, "P6\n170 320\n255\n");
    unsigned foreground_pixels = 0;
    for (int i = 0; i < 170 * 320; ++i) {
        unsigned color = (unsigned)bytes[2 * i] << 8 | bytes[2 * i + 1];
        assert(color == backgrounds[frames] || color == 0xf7bf);
        foreground_pixels += color == 0xf7bf;
        fputc(((color >> 11) & 31) * 255 / 31, file);
        fputc(((color >> 5) & 63) * 255 / 63, file);
        fputc((color & 31) * 255 / 31, file);
    }
    assert(foreground_pixels > 2000);
    assert(fclose(file) == 0);
    printf("LCD frame %u: %s, 170x320\n", frames,
           frames == 1 ? "amber DECISION" : frames == 2 ? "green DONE" : "blue WORKING");
    ++frames;
    return ESP_OK;
}

int main(int argc, char **argv)
{
    assert(argc == 2);
    output_dir = argv[1];
    if (setjmp(end_of_input) == 0) {
        app_main();
    }
    assert(frames == 4);
    free(framebuffer);
    return 0;
}
