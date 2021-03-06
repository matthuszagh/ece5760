/* Simple demo file to experiment with the VGA controller on the DE1-SoC
 * Computer System by Altera.
 */

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/shm.h>
#include "address_map_arm_brl4.h"

#define PIXEL_COLS  (unsigned int)640
#define PIXEL_ROWS  (unsigned int)480

#define WHITE       (unsigned short)0xFFFF
#define BLUE        (unsigned short)0x000F
#define BLACK       (unsigned short)0x0000


// Forward Declarations
void draw_rect(volatile unsigned char *, unsigned short,
               unsigned int, unsigned int,
               unsigned int, unsigned int);

int main()
{
    // Create a file descriptor for "/dev/mem"
    int fd_mem;
    if ((fd_mem = open("/dev/mem", (O_RDWR|O_SYNC))) == -1) {
        printf("Error: failed to open \"/dev/mem\".\n");
        close(fd_mem);
        return 1;
    }

    // Create a file descriptor for "/dev/input/mice"
    int fd_mouse;
    if ((fd_mouse = open("/dev/input/mice", (O_RDWR|O_SYNC))) == -1) {
        printf("Error: failed to open \"/dev/input/mice\".\n");
        close(fd_mouse);
        return 1;
    }
    int flags = fcntl(fd_mouse, F_GETFL, 0);
    fcntl(fd_mouse, F_SETFL, flags | O_NONBLOCK);

    // Map the pixel buffer to a virtual address
    void *pixel_buffer_addr;
    if ((pixel_buffer_addr = mmap(NULL, FPGA_ONCHIP_SPAN,
                                  (PROT_READ|PROT_WRITE),
                                  MAP_SHARED, fd_mem, FPGA_ONCHIP_BASE))
            == MAP_FAILED)
    {
        printf("Error: failed to map the pixel buffer.\n");
        close(fd_mem);
        return 1;
    }

    volatile unsigned char *pixel = (unsigned char *)pixel_buffer_addr;
    volatile unsigned char *pixel_end = (unsigned char *)(pixel_buffer_addr +
                                                          FPGA_ONCHIP_SPAN);

    // Read data from the mouse
    int bytes;
    int left, middle, right;
    int x, y = 0;
    signed char data[3];

    while (1) {
        bytes = read(fd_mouse, data, sizeof(data));

        if (bytes > 0) {
            left = data[0] & 0x1;   // first bit
            middle = data[0] & 0x2; // second bit
            right = data[0] & 0x4;  // third bit

            draw_rect(pixel, BLACK, x-2, y-2, x+2, y+2);

            x += data[1];
            if (x < 0) x = 0;
            else if (x > PIXEL_COLS) x = PIXEL_COLS;
            y -= data[2];
            if (y < 0) y = 0;
            else if (y > PIXEL_ROWS) y = PIXEL_ROWS;

            draw_rect(pixel, WHITE, x-2, y-2, x+2, y+2);
            printf("x: %d\ty: %d\n", x, y);
        }
    }

    return 0;
}

inline unsigned int max(unsigned int x1, unsigned int x2)
{ return x1 > x2 ? x1 : x2; }

inline unsigned int min(unsigned int x1, unsigned int x2)
{ return x1 < x2 ? x1 : x2; }

void draw_rect(volatile unsigned char *addr_base, unsigned short color,
               unsigned int x1, unsigned int y1,
               unsigned int x2, unsigned int y2)
{
    // bounds checking
    x1 = max(x1, (unsigned int)0);
    y1 = max(y1, (unsigned int)0);
    x2 = min(x2, PIXEL_COLS);
    y2 = min(y2, PIXEL_ROWS);

    unsigned int x = x1;
    unsigned int y = y1;

    while (y < y2) {
        while (x < x2) {
            *(addr_base + x + (y<<10)) = color;
            ++x;
        }
        x = x1;
        ++y;
    }
}