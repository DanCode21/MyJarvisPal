// st7789_hal.c - HAL with proper text rendering
#include "st7789_hal.h"
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>
#include <string.h>
#include <gpiod.h>


// ----- Low-level driver declarations -----
extern int spi_fd;
extern struct gpiod_line_request *dc_req;
extern struct gpiod_line_request *rst_req;

extern int setup_gpio(void);
extern void st7789_init(void);
extern void fill_color(uint16_t color);
extern void draw_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
extern uint16_t rgb_to_color(uint8_t r, uint8_t g, uint8_t b);
extern void set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);
extern void spi_write_data(const uint8_t *data, size_t len);

// Simple 5x8 font
static const uint8_t font5x8[][5] = {
    {0x7E, 0x11, 0x11, 0x11, 0x7E}, // A
    {0x7F, 0x49, 0x49, 0x49, 0x36}, // B
    {0x3E, 0x41, 0x41, 0x41, 0x22}, // C
    {0x7F, 0x41, 0x41, 0x22, 0x1C}, // D
    {0x7F, 0x49, 0x49, 0x49, 0x41}, // E
    {0x7F, 0x09, 0x09, 0x09, 0x01}, // F
    {0x3E, 0x41, 0x49, 0x49, 0x7A}, // G
    {0x7F, 0x08, 0x08, 0x08, 0x7F}, // H
    {0x00, 0x41, 0x7F, 0x41, 0x00}, // I
    {0x20, 0x40, 0x41, 0x3F, 0x01}, // J
    {0x7F, 0x08, 0x14, 0x22, 0x41}, // K
    {0x7F, 0x40, 0x40, 0x40, 0x40}, // L
    {0x7F, 0x02, 0x0C, 0x02, 0x7F}, // M
    {0x7F, 0x04, 0x08, 0x10, 0x7F}, // N
    {0x3E, 0x41, 0x41, 0x41, 0x3E}, // O
    {0x7F, 0x09, 0x09, 0x09, 0x06}, // P
    {0x3E, 0x41, 0x51, 0x21, 0x5E}, // Q
    {0x7F, 0x09, 0x19, 0x29, 0x46}, // R
    {0x46, 0x49, 0x49, 0x49, 0x31}, // S
    {0x01, 0x01, 0x7F, 0x01, 0x01}, // T
    {0x3F, 0x40, 0x40, 0x40, 0x3F}, // U
    {0x1F, 0x20, 0x40, 0x20, 0x1F}, // V
    {0x3F, 0x40, 0x38, 0x40, 0x3F}, // W
    {0x63, 0x14, 0x08, 0x14, 0x63}, // X
    {0x07, 0x08, 0x70, 0x08, 0x07}, // Y
    {0x61, 0x51, 0x49, 0x45, 0x43}, // Z
    {0x00, 0x00, 0x00, 0x00, 0x00}, // Space (26)
    {0x7C, 0x12, 0x11, 0x12, 0x7C}, // a (27)
    {0x7F, 0x48, 0x44, 0x44, 0x38}, // b
    {0x38, 0x44, 0x44, 0x44, 0x20}, // c
    {0x38, 0x44, 0x44, 0x48, 0x7F}, // d
    {0x38, 0x54, 0x54, 0x54, 0x18}, // e
    {0x08, 0x7E, 0x09, 0x01, 0x02}, // f
    {0x0C, 0x52, 0x52, 0x52, 0x3E}, // g
    {0x7F, 0x08, 0x04, 0x04, 0x78}, // h
    {0x00, 0x44, 0x7D, 0x40, 0x00}, // i
    {0x20, 0x40, 0x44, 0x3D, 0x00}, // j
    {0x7F, 0x10, 0x28, 0x44, 0x00}, // k
    {0x00, 0x41, 0x7F, 0x40, 0x00}, // l
    {0x7C, 0x04, 0x18, 0x04, 0x78}, // m
    {0x7C, 0x08, 0x04, 0x04, 0x78}, // n
    {0x38, 0x44, 0x44, 0x44, 0x38}, // o
    {0x7C, 0x14, 0x14, 0x14, 0x08}, // p
    {0x08, 0x14, 0x14, 0x18, 0x7C}, // q
    {0x7C, 0x08, 0x04, 0x04, 0x08}, // r
    {0x48, 0x54, 0x54, 0x54, 0x20}, // s
    {0x04, 0x3F, 0x44, 0x40, 0x20}, // t
    {0x3C, 0x40, 0x40, 0x20, 0x7C}, // u
    {0x1C, 0x20, 0x40, 0x20, 0x1C}, // v
    {0x3C, 0x40, 0x30, 0x40, 0x3C}, // w
    {0x44, 0x28, 0x10, 0x28, 0x44}, // x
    {0x0C, 0x50, 0x50, 0x50, 0x3C}, // y
    {0x44, 0x64, 0x54, 0x4C, 0x44}, // z
    {0x3E, 0x51, 0x49, 0x45, 0x3E}, // 0 (53)
    {0x00, 0x42, 0x7F, 0x40, 0x00}, // 1
    {0x42, 0x61, 0x51, 0x49, 0x46}, // 2
    {0x21, 0x41, 0x45, 0x4B, 0x31}, // 3
    {0x18, 0x14, 0x12, 0x7F, 0x10}, // 4
    {0x27, 0x45, 0x45, 0x45, 0x39}, // 5
    {0x3C, 0x4A, 0x49, 0x49, 0x30}, // 6
    {0x01, 0x71, 0x09, 0x05, 0x03}, // 7
    {0x36, 0x49, 0x49, 0x49, 0x36}, // 8
    {0x06, 0x49, 0x49, 0x29, 0x1E}, // 9
    {0x00, 0x36, 0x36, 0x00, 0x00}, // : (63)
};

static int char_to_index(char c) {
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a' + 27;
    if (c >= '0' && c <= '9') return c - '0' + 53;
    if (c == ' ') return 26;
    if (c == ':') return 63;
    return 26; // default space
}

// ===========================================================
// HAL INITIALIZATION
// ===========================================================

int st7789_hal_init(void)
{
    spi_fd = open("/dev/spidev0.0", O_RDWR);
    if (spi_fd < 0) {
        perror("open spidev");
        return -1;
    }

    uint8_t mode = 0;
    uint8_t bits = 8;
    uint32_t speed = 40000000;

    ioctl(spi_fd, SPI_IOC_WR_MODE, &mode);
    ioctl(spi_fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
    ioctl(spi_fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);

    if (setup_gpio() < 0)
        return -1;

    st7789_init();
    return 0;
}

void st7789_hal_cleanup(void)
{
    if (dc_req)
        gpiod_line_request_release(dc_req);
    if (rst_req)
        gpiod_line_request_release(rst_req);

    if (spi_fd >= 0)
        close(spi_fd);
}

// ===========================================================
// COLOR
// ===========================================================

uint16_t st7789_rgb(uint8_t r, uint8_t g, uint8_t b)
{
    return rgb_to_color(r, g, b);
}

// ===========================================================
// BASIC DRAW
// ===========================================================

void st7789_fill_screen(uint16_t color)
{
    fill_color(color);
}

void st7789_fill_rect(int x, int y, int w, int h, uint16_t color)
{
    draw_rect(x, y, w, h, color);
}

// ===========================================================
// TEXT RENDERING
// ===========================================================

void st7789_draw_char(int x, int y, char c, uint16_t fg, uint16_t bg, int scale)
{
    int idx = char_to_index(c);
    const uint8_t *glyph = font5x8[idx];
    
    if (scale == 1) {
        // Fast path for scale=1
        for (int col = 0; col < 5; col++) {
            for (int row = 0; row < 8; row++) {
                uint16_t color = (glyph[col] & (1 << row)) ? fg : bg;
                set_window(x + col, y + row, x + col, y + row);
                uint8_t px[2] = {color >> 8, color & 0xFF};
                spi_write_data(px, 2);
            }
        }
    } else {
        // Scaled rendering
        for (int col = 0; col < 5; col++) {
            for (int row = 0; row < 8; row++) {
                uint16_t color = (glyph[col] & (1 << row)) ? fg : bg;
                draw_rect(x + col * scale, y + row * scale, scale, scale, color);
            }
        }
    }
}

void st7789_draw_text(int x, int y, const char *txt, uint16_t fg, uint16_t bg, int scale)
{
    int cursor_x = x;
    while (*txt) {
        st7789_draw_char(cursor_x, y, *txt, fg, bg, scale);
        cursor_x += 6 * scale;
        txt++;
    }
}

// ===========================================================
// PROGRESS BAR
// ===========================================================

void st7789_draw_progress_bar(int x, int y, int w, int h,
                              float progress, uint16_t fg, uint16_t bg)
{
    if (progress < 0) progress = 0;
    if (progress > 1) progress = 1;

    int filled = (int)(w * progress);

    // Draw border
    draw_rect(x, y, w, 2, fg);           // top
    draw_rect(x, y+h-2, w, 2, fg);       // bottom
    draw_rect(x, y, 2, h, fg);           // left
    draw_rect(x+w-2, y, 2, h, fg);       // right

    // Fill interior
    draw_rect(x+2, y+2, w-4, h-4, bg);
    
    if (filled > 4) {
        draw_rect(x+2, y+2, filled-4, h-4, fg);
    }
}