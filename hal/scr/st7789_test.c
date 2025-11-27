// ST7789 SPI Display - WORKING VERSION for BeagleY-AI
// Uses SPI0.0 + libgpiod v2
// Wiring: DC=gpiochip1/33, RST=gpiochip2/8

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/spi/spidev.h>
#include <sys/ioctl.h>
#include <gpiod.h>

#define WIDTH  240
#define HEIGHT 320

#define DC_CHIP   "/dev/gpiochip1"
#define DC_LINE   33
#define RST_CHIP  "/dev/gpiochip2"
#define RST_LINE  8

static int spi_fd;
static struct gpiod_line_request *dc_req;
static struct gpiod_line_request *rst_req;

// ================ SPI Write Helpers ==================

void spi_write_cmd(uint8_t cmd)
{
    gpiod_line_request_set_value(dc_req, DC_LINE, 0);
    struct spi_ioc_transfer tr = {
        .tx_buf = (uintptr_t)&cmd,
        .rx_buf = 0,
        .len = 1,
        .delay_usecs = 0,
        .speed_hz = 40000000,
        .bits_per_word = 8,
    };
    ioctl(spi_fd, SPI_IOC_MESSAGE(1), &tr);
}

void spi_write_data(const uint8_t *data, size_t len)
{
    gpiod_line_request_set_value(dc_req, DC_LINE, 1);
    while (len > 0) {
        size_t chunk = len > 4096 ? 4096 : len;
        struct spi_ioc_transfer tr = {
            .tx_buf = (uintptr_t)data,
            .rx_buf = 0,
            .len = chunk,
            .delay_usecs = 0,
            .speed_hz = 40000000,
            .bits_per_word = 8,
        };
        ioctl(spi_fd, SPI_IOC_MESSAGE(1), &tr);
        data += chunk;
        len -= chunk;
    }
}

// ================ ST7789 Commands ==================

void st_reset()
{
    gpiod_line_request_set_value(rst_req, RST_LINE, 0);
    usleep(50000);
    gpiod_line_request_set_value(rst_req, RST_LINE, 1);
    usleep(50000);
}

void st7789_init()
{
    st_reset();

    spi_write_cmd(0x3A);   // COLMOD - Pixel Format
    uint8_t pixfmt[1] = {0x55}; // RGB565 (16-bit color)
    spi_write_data(pixfmt, 1);

    spi_write_cmd(0x36);   // MADCTL - Memory Data Access Control
    uint8_t madctl[1] = {0x08}; // Portrait, BGR order
    spi_write_data(madctl, 1);

    spi_write_cmd(0x21);  // INVON - Display Inversion ON (CRITICAL!)

    spi_write_cmd(0x11); // SLPOUT - Sleep Out
    usleep(120000);

    spi_write_cmd(0x29); // DISPON - Display On
}

void set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
    spi_write_cmd(0x2A); // CASET - Column Address Set
    uint8_t col[4] = {x0>>8, x0&0xFF, x1>>8, x1&0xFF};
    spi_write_data(col, 4);

    spi_write_cmd(0x2B); // RASET - Row Address Set
    uint8_t row[4] = {y0>>8, y0&0xFF, y1>>8, y1&0xFF};
    spi_write_data(row, 4);

    spi_write_cmd(0x2C); // RAMWR - Memory Write
}

void fill_color(uint16_t color)
{
    uint32_t pixels = (uint32_t)WIDTH * HEIGHT;
    uint8_t hi = color >> 8;
    uint8_t lo = color & 0xFF;

    set_window(0, 0, WIDTH-1, HEIGHT-1);

    static uint8_t buf[4096];
    for (int i = 0; i < sizeof(buf); i += 2) {
        buf[i]   = hi;
        buf[i+1] = lo;
    }

    while (pixels > 0) {
        size_t chunk_px = (pixels > (sizeof(buf)/2)) ? (sizeof(buf)/2) : pixels;
        spi_write_data(buf, chunk_px*2);
        pixels -= chunk_px;
    }
}

// RGB to this display's weird color format
// Channel mapping: R->Green position, G->Blue position, B->Red position
// Then byte-swap the result
uint16_t rgb_to_color(uint8_t r, uint8_t g, uint8_t b)
{
    uint8_t r5 = r >> 3;  // 5 bits
    uint8_t g6 = g >> 2;  // 6 bits  
    uint8_t b5 = b >> 3;  // 5 bits
    
    // Rotate channels: R->G position, G->B position, B->R position
    uint16_t color = (r5 << 11) | (b5 << 5) | g6;
    
    // Byte swap
    return ((color & 0xFF) << 8) | (color >> 8);
}

void draw_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
    set_window(x, y, x+w-1, y+h-1);
    
    uint8_t hi = color >> 8;
    uint8_t lo = color & 0xFF;
    
    static uint8_t buf[4096];
    for (int i = 0; i < sizeof(buf); i += 2) {
        buf[i]   = hi;
        buf[i+1] = lo;
    }
    
    uint32_t pixels = (uint32_t)w * h;
    while (pixels > 0) {
        size_t chunk_px = (pixels > (sizeof(buf)/2)) ? (sizeof(buf)/2) : pixels;
        spi_write_data(buf, chunk_px*2);
        pixels -= chunk_px;
    }
}

// ================ GPIO Setup ==================

int setup_gpio()
{
    struct gpiod_chip *chip_dc  = gpiod_chip_open(DC_CHIP);
    struct gpiod_chip *chip_rst = gpiod_chip_open(RST_CHIP);
    if (!chip_dc || !chip_rst) {
        perror("gpiod_chip_open");
        return -1;
    }

    struct gpiod_line_settings *settings = gpiod_line_settings_new();
    gpiod_line_settings_set_direction(settings, GPIOD_LINE_DIRECTION_OUTPUT);
    gpiod_line_settings_set_output_value(settings, GPIOD_LINE_VALUE_INACTIVE);

    unsigned int dc_offset = DC_LINE;
    struct gpiod_line_config *dc_config = gpiod_line_config_new();
    gpiod_line_config_add_line_settings(dc_config, &dc_offset, 1, settings);
    struct gpiod_request_config *dc_req_cfg = gpiod_request_config_new();
    gpiod_request_config_set_consumer(dc_req_cfg, "st7789_dc");
    dc_req = gpiod_chip_request_lines(chip_dc, dc_req_cfg, dc_config);

    gpiod_line_settings_set_output_value(settings, GPIOD_LINE_VALUE_ACTIVE);
    unsigned int rst_offset = RST_LINE;
    struct gpiod_line_config *rst_config = gpiod_line_config_new();
    gpiod_line_config_add_line_settings(rst_config, &rst_offset, 1, settings);
    struct gpiod_request_config *rst_req_cfg = gpiod_request_config_new();
    gpiod_request_config_set_consumer(rst_req_cfg, "st7789_rst");
    rst_req = gpiod_chip_request_lines(chip_rst, rst_req_cfg, rst_config);

    if (!dc_req || !rst_req) {
        perror("gpiod_chip_request_lines");
        return -1;
    }

    gpiod_request_config_free(dc_req_cfg);
    gpiod_request_config_free(rst_req_cfg);
    gpiod_line_config_free(dc_config);
    gpiod_line_config_free(rst_config);
    gpiod_line_settings_free(settings);
    gpiod_chip_close(chip_dc);
    gpiod_chip_close(chip_rst);

    return 0;
}

// ================ MAIN ==================

int main()
{
    printf("ST7789 Display Test\n");
    printf("===================\n\n");

    // ---------------- SPI setup ----------------
    printf("Opening SPI...\n");
    spi_fd = open("/dev/spidev0.0", O_RDWR);
    if (spi_fd < 0) {
        perror("open spidev");
        return 1;
    }

    uint8_t mode = 0;
    ioctl(spi_fd, SPI_IOC_WR_MODE, &mode);
    uint8_t bits = 8;
    ioctl(spi_fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
    uint32_t speed = 40000000;
    ioctl(spi_fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);

    // ---------------- GPIO setup ----------------
    printf("Setting up GPIO...\n");
    if (setup_gpio() < 0) {
        close(spi_fd);
        return 1;
    }

    // ---------------- Display Init ----------------
    printf("Initializing display...\n");
    st7789_init();

    // ---------------- Color Test ----------------
    printf("\nTesting colors:\n");
    
    printf("  RED\n");
    fill_color(rgb_to_color(255, 0, 0));
    sleep(1);
    
    printf("  GREEN\n");
    fill_color(rgb_to_color(0, 255, 0));
    sleep(1);
    
    printf("  BLUE\n");
    fill_color(rgb_to_color(0, 0, 255));
    sleep(1);
    
    printf("  WHITE\n");
    fill_color(rgb_to_color(255, 255, 255));
    sleep(1);
    
    printf("  BLACK\n");
    fill_color(rgb_to_color(0, 0, 0));
    sleep(1);

    // ---------------- Pattern Test ----------------
    printf("\nDrawing color bars...\n");
    fill_color(rgb_to_color(0, 0, 0)); // Clear to black
    
    uint16_t bar_height = HEIGHT / 6;
    draw_rect(0, 0*bar_height, WIDTH, bar_height, rgb_to_color(255, 0, 0));     // Red
    draw_rect(0, 1*bar_height, WIDTH, bar_height, rgb_to_color(0, 255, 0));     // Green
    draw_rect(0, 2*bar_height, WIDTH, bar_height, rgb_to_color(0, 0, 255));     // Blue
    draw_rect(0, 3*bar_height, WIDTH, bar_height, rgb_to_color(255, 255, 0));   // Yellow
    draw_rect(0, 4*bar_height, WIDTH, bar_height, rgb_to_color(255, 0, 255));   // Magenta
    draw_rect(0, 5*bar_height, WIDTH, bar_height, rgb_to_color(0, 255, 255));   // Cyan

    printf("\nDONE! Display should show 6 color bars.\n");

    // Cleanup
    gpiod_line_request_release(dc_req);
    gpiod_line_request_release(rst_req);
    close(spi_fd);

    return 0;
}