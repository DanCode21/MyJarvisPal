#pragma once
#include <stdint.h>

#define DISPLAY_WIDTH  240
#define DISPLAY_HEIGHT 320

int st7789_hal_init(void);
void st7789_hal_cleanup(void);

uint16_t st7789_rgb(uint8_t r, uint8_t g, uint8_t b);

void st7789_fill_screen(uint16_t color);
void st7789_fill_rect(int x, int y, int w, int h, uint16_t color);

void st7789_draw_text(int x, int y, const char *txt,
                      uint16_t fg, uint16_t bg, int scale);

void st7789_draw_progress_bar(int x, int y, int w, int h,
                              float progress, uint16_t fg, uint16_t bg);
