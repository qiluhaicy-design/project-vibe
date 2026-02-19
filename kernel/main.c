#include <stdint.h>

#define UART_BASE 0x09000000
#define MAILBOX_BASE 0x3F00B880

#define MAILBOX_READ   ((volatile uint32_t*)(MAILBOX_BASE + 0x0))
#define MAILBOX_STATUS ((volatile uint32_t*)(MAILBOX_BASE + 0x18))
#define MAILBOX_WRITE  ((volatile uint32_t*)(MAILBOX_BASE + 0x20))

#define MAILBOX_EMPTY  0x40000000
#define MAILBOX_FULL   0x80000000

uint32_t fb_width = 800;
uint32_t fb_height = 600;
uint32_t fb_pitch;
uint32_t fb_size;
uint8_t* fb_addr;

struct Window {
    int x, y, w, h;
    char title[32];
};

struct Window windows[10];
int num_windows = 0;

int my_abs(int x) {
    return x < 0 ? -x : x;
}

void uart_init() {
    // Disable UART
    *(volatile uint32_t*)(UART_BASE + 0x30) = 0;
    // Clear pending interrupts
    *(volatile uint32_t*)(UART_BASE + 0x44) = 0x7FF;
    // Set baud rate to 115200 (assuming 48MHz clock)
    *(volatile uint32_t*)(UART_BASE + 0x24) = 1; // IBRD
    *(volatile uint32_t*)(UART_BASE + 0x28) = 40; // FBRD
    // Set word length to 8 bits, no parity, 1 stop bit
    *(volatile uint32_t*)(UART_BASE + 0x2C) = 0x70; // LCRH
    // Enable UART, TX, RX
    *(volatile uint32_t*)(UART_BASE + 0x30) = 0x301;
}

void uart_putc(char c) {
    while ((*(volatile uint32_t*)(UART_BASE + 0x18)) & 0x20);
    *(volatile uint32_t*)UART_BASE = c;
}

void uart_puts(const char* s) {
    while (*s) uart_putc(*s++);
}

uint32_t mailbox_read(uint8_t channel) {
    // Not used
    return 0;
}

void mailbox_write(uint8_t channel, uint32_t data) {
    // Not used
}

void init_framebuffer() {
    uart_puts("Init framebuffer start\n");
    // For QEMU virt, assume framebuffer at 0x40000000 + 0x200000
    fb_addr = (uint8_t*)(0x40000000 + 0x200000);
    fb_width = 800;
    fb_height = 600;
    fb_pitch = fb_width * 4;
    fb_size = fb_height * fb_pitch;
    uart_puts("FB width: ");
    uint32_t val1 = fb_width;
    for (int i = 28; i >= 0; i -= 4) {
        uint8_t digit = (val1 >> i) & 0xF;
        uart_putc(digit < 10 ? '0' + digit : 'A' + digit - 10);
    }
    uart_puts(", height: ");
    uint32_t val2 = fb_height;
    for (int i = 28; i >= 0; i -= 4) {
        uint8_t digit = (val2 >> i) & 0xF;
        uart_putc(digit < 10 ? '0' + digit : 'A' + digit - 10);
    }
    uart_puts(", pitch: ");
    uint32_t val3 = fb_pitch;
    for (int i = 28; i >= 0; i -= 4) {
        uint8_t digit = (val3 >> i) & 0xF;
        uart_putc(digit < 10 ? '0' + digit : 'A' + digit - 10);
    }
    uart_puts("\n");
    uart_puts("Framebuffer initialized\n");
}

void put_pixel(int x, int y, uint32_t color) {
    if (x < 0 || (uint32_t)x >= fb_width || y < 0 || (uint32_t)y >= fb_height) return;
    uint32_t* pixel = (uint32_t*)(fb_addr + y * fb_pitch + x * 4);
    *pixel = color;
    // Debug: count pixels, but too many, skip
}

void draw_rect(int x, int y, int w, int h, uint32_t color) {
    uart_puts("Drawing rect\n");
    for (int i = 0; i < h; i++) {
        for (int j = 0; j < w; j++) {
            put_pixel(x + j, y + i, color);
        }
    }
}

void draw_line(int x1, int y1, int x2, int y2, uint32_t color) {
    uart_puts("Drawing line\n");
    int dx = my_abs(x2 - x1);
    int dy = my_abs(y2 - y1);
    int sx = x1 < x2 ? 1 : -1;
    int sy = y1 < y2 ? 1 : -1;
    int err = dx - dy;
    while (1) {
        put_pixel(x1, y1, color);
        if (x1 == x2 && y1 == y2) break;
        int e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x1 += sx;
        }
        if (e2 < dx) {
            err += dx;
            y1 += sy;
        }
    }
}

void draw_window(struct Window* win) {
    uart_puts("Drawing window: x=");
    // print x
    uint32_t val = win->x;
    for (int i = 28; i >= 0; i -= 4) {
        uint8_t digit = (val >> i) & 0xF;
        uart_putc(digit < 10 ? '0' + digit : 'A' + digit - 10);
    }
    uart_puts(" y=");
    val = win->y;
    for (int i = 28; i >= 0; i -= 4) {
        uint8_t digit = (val >> i) & 0xF;
        uart_putc(digit < 10 ? '0' + digit : 'A' + digit - 10);
    }
    uart_puts(" w=");
    val = win->w;
    for (int i = 28; i >= 0; i -= 4) {
        uint8_t digit = (val >> i) & 0xF;
        uart_putc(digit < 10 ? '0' + digit : 'A' + digit - 10);
    }
    uart_puts(" h=");
    val = win->h;
    for (int i = 28; i >= 0; i -= 4) {
        uint8_t digit = (val >> i) & 0xF;
        uart_putc(digit < 10 ? '0' + digit : 'A' + digit - 10);
    }
    uart_puts("\n");

    // background
    draw_rect(win->x, win->y, win->w, win->h, 0xFFCCCCCC);
    // title bar
    draw_rect(win->x, win->y, win->w, 20, 0xFF0000FF);
    // close button
    draw_rect(win->x + win->w - 20, win->y, 20, 20, 0xFFFF0000);
    // border
    draw_line(win->x, win->y, win->x + win->w, win->y, 0xFF000000);
    draw_line(win->x, win->y, win->x, win->y + win->h, 0xFF000000);
    draw_line(win->x + win->w, win->y, win->x + win->w, win->y + win->h, 0xFF000000);
    draw_line(win->x, win->y + win->h, win->x + win->w, win->y + win->h, 0xFF000000);
}

void main() {
    uart_puts("Starting main\n");
    uart_init();
    uart_puts("UART initialized\n");
    uart_puts("Hello from microkernel!\n");
    uart_puts("Memory layout: kernel at 0x40000000, stack at 0x40000000 + 0x100000\n");
    uart_puts("Calling init_framebuffer\n");
    init_framebuffer();
    uart_puts("Graphics memory: fb_addr=");
    uint32_t addr = (uint32_t)fb_addr;
    for (int i = 28; i >= 0; i -= 4) {
        uint8_t digit = (addr >> i) & 0xF;
        uart_putc(digit < 10 ? '0' + digit : 'A' + digit - 10);
    }
    uart_puts(", size=");
    uint32_t val = fb_size;
    for (int i = 28; i >= 0; i -= 4) {
        uint8_t digit = (val >> i) & 0xF;
        uart_putc(digit < 10 ? '0' + digit : 'A' + digit - 10);
    }
    uart_puts("\n");
    uart_puts("Creating window\n");
    // Create a window
    struct Window w1 = {50, 50, 200, 150, "Test"};
    uart_puts("Calling draw_window\n");
    draw_window(&w1);
    uart_puts("Window drawn\n");
    while(1);
}