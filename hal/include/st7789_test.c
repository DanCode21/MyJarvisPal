// st7789_hal.h - Hardware Abstraction Layer for ST7789 Display
// BeagleY-AI with libgpiod v2

#ifndef ST7789_HAL_H
#define ST7789_HAL_H

#include <stdint.h>

#define DISPLAY_WIDTH  240
#define DISPLAY_HEIGHT 320

// Display initialization and cleanup
int st7789_hal_init(void);
void st7789_hal_cleanup(void);

// Color conversion
uint16_t st7789_rgb(uint8_t r, uint8_t g, uint8_t b);

// Drawing primitives
void st7789_fill_screen(uint16_t color);
void st7789_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
void st7789_draw_pixel(uint16_t x, uint16_t y, uint16_t color);

// Text rendering
void st7789_draw_char(uint16_t x, uint16_t y, char c, uint16_t fg, uint16_t bg, uint8_t scale);
void st7789_draw_text(uint16_t x, uint16_t y, const char *text, uint16_t fg, uint16_t bg, uint8_t scale);

// UI elements
void st7789_draw_progress_bar(uint16_t x, uint16_t y, uint16_t w, uint16_t h, 
                               float progress, uint16_t fg, uint16_t bg);

// Common colors
#define COLOR_BLACK   st7789_rgb(0, 0, 0)
#define COLOR_WHITE   st7789_rgb(255, 255, 255)
#define COLOR_RED     st7789_rgb(255, 0, 0)
#define COLOR_GREEN   st7789_rgb(0, 255, 0)
#define COLOR_BLUE    st7789_rgb(0, 0, 255)
#define COLOR_YELLOW  st7789_rgb(255, 255, 0)
#define COLOR_CYAN    st7789_rgb(0, 255, 255)
#define COLOR_MAGENTA st7789_rgb(255, 0, 255)

#endif // ST7789_HAL_H