#include <stdint.h>

#define UART_BASE 0x3F201000
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
    // Set GPIO 14 and 15 to alt0 for UART
    volatile uint32_t* gpfsel1 = (volatile uint32_t*)(0x3F200000 + 0x04);
    *gpfsel1 = (*gpfsel1 & ~(7 << 12)) | (4 << 12); // GPIO14 alt0
    *gpfsel1 = (*gpfsel1 & ~(7 << 15)) | (4 << 15); // GPIO15 alt0

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
    uint32_t data;
    do {
        while (*MAILBOX_STATUS & MAILBOX_EMPTY);
        data = *MAILBOX_READ;
    } while ((data & 0xF) != channel);
    return data >> 4;
}

void mailbox_write(uint8_t channel, uint32_t data) {
    while (*MAILBOX_STATUS & MAILBOX_FULL);
    *MAILBOX_WRITE = (data << 4) | channel;
}

void init_framebuffer() {
    uart_puts("Init framebuffer start\n");
    // First, get board revision
    uint32_t __attribute__((aligned(16))) mailbox_rev[8];
    mailbox_rev[0] = 7*4; // size
    mailbox_rev[1] = 0; // request
    mailbox_rev[2] = 0x10002; // get board revision
    mailbox_rev[3] = 4;
    mailbox_rev[4] = 0;
    mailbox_rev[5] = 0;
    mailbox_rev[6] = 0; // end
    mailbox_write(8, (uint32_t)mailbox_rev);
    uart_puts("Rev mailbox written\n");
    mailbox_read(8);
    uart_puts("Rev mailbox read\n");
    uart_puts("Rev response: ");
    uint32_t resp = mailbox_rev[5];
    for (int i = 28; i >= 0; i -= 4) {
        uint8_t digit = (resp >> i) & 0xF;
        uart_putc(digit < 10 ? '0' + digit : 'A' + digit - 10);
    }
    uart_puts("\n");

    uint32_t __attribute__((aligned(16))) mailbox[36];
    mailbox[0] = 35*4; // buffer size
    mailbox[1] = 0; // request
    mailbox[2] = 0x48003; // set physical width/height
    mailbox[3] = 8;
    mailbox[4] = 0;
    mailbox[5] = fb_width;
    mailbox[6] = fb_height;
    mailbox[7] = 0x48004; // set virtual width/height
    mailbox[8] = 8;
    mailbox[9] = 0;
    mailbox[10] = fb_width;
    mailbox[11] = fb_height;
    mailbox[12] = 0x48005; // set depth
    mailbox[13] = 4;
    mailbox[14] = 0;
    mailbox[15] = 32; // 32 bpp
    mailbox[16] = 0x40001; // allocate buffer
    mailbox[17] = 8;
    mailbox[18] = 0;
    mailbox[19] = 16; // alignment
    mailbox[20] = 0;
    mailbox[21] = 0x40008; // get pitch
    mailbox[22] = 4;
    mailbox[23] = 0;
    mailbox[24] = 0;
    mailbox[25] = 0; // end
    uart_puts("Mailbox buffer prepared\n");
    mailbox_write(8, (uint32_t)mailbox);
    uart_puts("Mailbox written\n");
    mailbox_read(8);
    uart_puts("Mailbox read\n");
    uart_puts("Mailbox response: ");
    resp = mailbox[1];
    for (int i = 28; i >= 0; i -= 4) {
        uint8_t digit = (resp >> i) & 0xF;
        uart_putc(digit < 10 ? '0' + digit : 'A' + digit - 10);
    }
    uart_puts("\n");
    if (mailbox[1] != 0x80000000) {
        uart_puts("Framebuffer init failed\n");
        return;
    }
    fb_addr = (uint8_t*)(mailbox[19] & 0x3FFFFFFF);
    fb_pitch = mailbox[23];
    fb_size = mailbox[19] & 0x3FFFFFFF ? mailbox[20] : 0;
    uart_puts("FB addr: ");
    // print hex fb_addr
    uint32_t addr = (uint32_t)fb_addr;
    for (int i = 28; i >= 0; i -= 4) {
        uint8_t digit = (addr >> i) & 0xF;
        uart_putc(digit < 10 ? '0' + digit : 'A' + digit - 10);
    }
    uart_puts("\n");
    uart_puts("Framebuffer initialized\n");
}

void put_pixel(int x, int y, uint32_t color) {
    if (x < 0 || (uint32_t)x >= fb_width || y < 0 || (uint32_t)y >= fb_height) return;
    uint32_t* pixel = (uint32_t*)(fb_addr + y * fb_pitch + x * 4);
    *pixel = color;
}

void draw_rect(int x, int y, int w, int h, uint32_t color) {
    for (int i = 0; i < h; i++) {
        for (int j = 0; j < w; j++) {
            put_pixel(x + j, y + i, color);
        }
    }
}

void draw_line(int x1, int y1, int x2, int y2, uint32_t color) {
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
    uart_init();
    uart_puts("Hello from microkernel!\n");
    init_framebuffer();
    // Create a window
    struct Window w1 = {50, 50, 200, 150, "Test"};
    draw_window(&w1);
    uart_puts("Window drawn\n");
    while(1);
}