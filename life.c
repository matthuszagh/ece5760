/* An implementation of Conway's Game of Life.
 *
 * Rules can be found here:
 * http://mathworld.wolfram.com/GameofLife.html
 * 
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
#define OFF_WHITE   (unsigned short)0xEFFF
#define BLUE        (unsigned short)0x000F
#define BLACK       (unsigned short)0x0000


// Forward Declarations
void draw_pixel(volatile unsigned char *,
		unsigned char, unsigned int, unsigned int);

void draw_pi(unsigned char *,
	     volatile unsigned char *, unsigned int, unsigned int);

void draw_gun(unsigned char *,
	      volatile unsigned char *, unsigned int, unsigned int);

inline unsigned int index(unsigned int, unsigned int);


int main()
{
    // Create a file descriptor for "/dev/mem"
    int fd_mem;
    if ((fd_mem = open("/dev/mem", (O_RDWR|O_SYNC))) == -1) {
        printf("Error: failed to open \"/dev/mem\".\n");
        close(fd_mem);
        return 1;
    }

    // Create a non-blocking file descriptor for "/dev/input/mice"
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

    // Map the FPGA switches
    void *switch_addr;
    if ((switch_addr = mmap(NULL, HW_REGS_SPAN, (PROT_READ|PROT_WRITE),
                            MAP_SHARED, fd_mem, HW_REGS_BASE))
            == MAP_FAILED)
    {
        printf("Error: failed to map FPGA switches.\n");
        close(fd_mem);
        return 1;
    }

    volatile unsigned char *switches = (unsigned char *)(switch_addr+SW_BASE);

    volatile unsigned char *pixel = (unsigned char *)pixel_buffer_addr;
    volatile unsigned char *pixel_end = (unsigned char *)
					(pixel_buffer_addr + FPGA_ONCHIP_SPAN);

    // Read data from the mouse
    int bytes;
    int left, middle, right;
    unsigned int x, y = 0;
    unsigned char drawn_arr[PIXEL_COLS*PIXEL_ROWS];
    signed char data[3];

    while (1) {
        bytes = read(fd_mouse, data, sizeof(data));

        if (bytes > 0) {
            left = data[0] & 0x1;   // first bit
            right = data[0] & 0x2; // second bit
            middle = data[0] & 0x4;  // third bit
	    
	    if (drawn_arr[index(x, y)] == 0) draw_pixel(pixel, BLACK, x, y);

            x += data[1];
            if (x < 0) x = 0;
            else if (x > PIXEL_COLS) x = PIXEL_COLS;
            y -= data[2];
            if (y < 0) y = 0;
            else if (y > PIXEL_ROWS) y = PIXEL_ROWS;

	    if (left && drawn_arr[index(x, y)] == 0 && *(switches) != 2) {
		drawn_arr[index(x, y)] = 1;
	    }
	    else if (left && *(switches) != 2) drawn_arr[index(x, y)] = 0;
	    else if (left && *(switches) == 2) draw_pi(drawn_arr, pixel, x, y);

	    if (right) draw_gun(drawn_arr, pixel, x, y); 

 	    draw_pixel(pixel, OFF_WHITE, x, y);
        }
    }

    return 0;
}

inline unsigned int index(unsigned int x, unsigned int y)
{ return x + (PIXEL_COLS*(y-1)); }

inline unsigned int max(unsigned int x1, unsigned int x2)
{ return x1 > x2 ? x1 : x2; }

inline unsigned int min(unsigned int x1, unsigned int x2)
{ return x1 < x2 ? x1 : x2; }

void draw_pixel(volatile unsigned char *addr_base, unsigned char color,
		unsigned int x, unsigned int y)
{
    if (x > PIXEL_COLS || y > PIXEL_ROWS) return;
    *(addr_base + x + (y<<10)) = color;
}

void draw_pi(unsigned char *drawn_arr, volatile unsigned char *pixel,
	     unsigned int x, unsigned int y)
{
    unsigned int cpx1 = x;
    unsigned int cpy1 = y;
    unsigned int cpx2 = x+1;
    unsigned int cpy2 = y+1;
    unsigned int cpx3 = x+2;
    unsigned int cpy3 = y+2;

    if (cpx3 > PIXEL_COLS || cpy3 > PIXEL_ROWS) return;    

    draw_pixel(pixel, OFF_WHITE, cpx1, cpy1);
    drawn_arr[index(cpx1, cpy1)] = 1;

    draw_pixel(pixel, OFF_WHITE, cpx2, cpy1);
    drawn_arr[index(cpx2, cpy1)] = 1;

    draw_pixel(pixel, OFF_WHITE, cpx3, cpy1);
    drawn_arr[index(cpx3, cpy1)] = 1;

    draw_pixel(pixel, OFF_WHITE, cpx1, cpy2);
    drawn_arr[index(cpx1, cpy2)] = 1;

    draw_pixel(pixel, OFF_WHITE, cpx1, cpy3);
    drawn_arr[index(cpx1, cpy3)] = 1;

    draw_pixel(pixel, OFF_WHITE, cpx3, cpy2);
    drawn_arr[index(cpx3, cpy2)] = 1;

    draw_pixel(pixel, OFF_WHITE, cpx3, cpy3);
    drawn_arr[index(cpx3, cpy3)] = 1;
}

void draw_gun(unsigned char *drawn_arr, volatile unsigned char *pixel,
	      unsigned int x, unsigned int y)
{
    unsigned int right_bound = x+35;
    unsigned int bottom_bound = y+8;

    if (right_bound > PIXEL_COLS || bottom_bound > PIXEL_ROWS) return;

    // row 1
    draw_pixel(pixel, OFF_WHITE, x+24, y);
    drawn_arr[index(x+24, y)] = 1;

    // row 2
    draw_pixel(pixel, OFF_WHITE, x+22, y+1);
    drawn_arr[index(x+22, y+1)] = 1;

    draw_pixel(pixel, OFF_WHITE, x+24, y+1);
    drawn_arr[index(x+24, y+1)] = 1;

    // row 3
    draw_pixel(pixel, OFF_WHITE, x+12, y+2);
    drawn_arr[index(x+12, y+2)] = 1;

    draw_pixel(pixel, OFF_WHITE, x+13, y+2);
    drawn_arr[index(x+13, y+2)] = 1;

    draw_pixel(pixel, OFF_WHITE, x+20, y+2);
    drawn_arr[index(x+20, y+2)] = 1;

    draw_pixel(pixel, OFF_WHITE, x+21, y+2);
    drawn_arr[index(x+21, y+2)] = 1;

    draw_pixel(pixel, OFF_WHITE, x+34, y+2);
    drawn_arr[index(x+34, y+2)] = 1;

    draw_pixel(pixel, OFF_WHITE, x+35, y+2);
    drawn_arr[index(x+35, y+2)] = 1;

    // row 4
    draw_pixel(pixel, OFF_WHITE, x+11, y+3);
    drawn_arr[index(x+11, y+3)] = 1;

    draw_pixel(pixel, OFF_WHITE, x+15, y+3);
    drawn_arr[index(x+15, y+3)] = 1;

    draw_pixel(pixel, OFF_WHITE, x+20, y+3);
    drawn_arr[index(x+20, y+3)] = 1;

    draw_pixel(pixel, OFF_WHITE, x+21, y+3);
    drawn_arr[index(x+21, y+3)] = 1;

    draw_pixel(pixel, OFF_WHITE, x+34, y+3);
    drawn_arr[index(x+34, y+3)] = 1;

    draw_pixel(pixel, OFF_WHITE, x+35, y+3);
    drawn_arr[index(x+35, y+3)] = 1;

    // row 5
    draw_pixel(pixel, OFF_WHITE, x, y+4);
    drawn_arr[index(x, y+4)] = 1;

    draw_pixel(pixel, OFF_WHITE, x+1, y+4);
    drawn_arr[index(x+1, y+4)] = 1;

    draw_pixel(pixel, OFF_WHITE, x+10, y+4);
    drawn_arr[index(x+10, y+4)] = 1;

    draw_pixel(pixel, OFF_WHITE, x+16, y+4);
    drawn_arr[index(x+16, y+4)] = 1;

    draw_pixel(pixel, OFF_WHITE, x+20, y+4);
    drawn_arr[index(x+20, y+4)] = 1;

    draw_pixel(pixel, OFF_WHITE, x+21, y+4);
    drawn_arr[index(x+21, y+4)] = 1;

    // row 6
    draw_pixel(pixel, OFF_WHITE, x, y+5);
    drawn_arr[index(x, y+5)] = 1;

    draw_pixel(pixel, OFF_WHITE, x+1, y+5);
    drawn_arr[index(x+1, y+5)] = 1;

    draw_pixel(pixel, OFF_WHITE, x+10, y+5);
    drawn_arr[index(x+10, y+5)] = 1;

    draw_pixel(pixel, OFF_WHITE, x+14, y+5);
    drawn_arr[index(x+14, y+5)] = 1;

    draw_pixel(pixel, OFF_WHITE, x+16, y+5);
    drawn_arr[index(x+16, y+5)] = 1;

    draw_pixel(pixel, OFF_WHITE, x+17, y+5);
    drawn_arr[index(x+17, y+5)] = 1;

    draw_pixel(pixel, OFF_WHITE, x+22, y+5);
    drawn_arr[index(x+22, y+5)] = 1;

    draw_pixel(pixel, OFF_WHITE, x+24, y+5);
    drawn_arr[index(x+24, y+5)] = 1;

    // row 7
    draw_pixel(pixel, OFF_WHITE, x+10, y+6);
    drawn_arr[index(x+10, y+6)] = 1;

    draw_pixel(pixel, OFF_WHITE, x+16, y+6);
    drawn_arr[index(x+16, y+6)] = 1;

    draw_pixel(pixel, OFF_WHITE, x+24, y+6);
    drawn_arr[index(x+24, y+6)] = 1;

    // row 8
    draw_pixel(pixel, OFF_WHITE, x+11, y+7);
    drawn_arr[index(x+11, y+7)] = 1;

    draw_pixel(pixel, OFF_WHITE, x+15, y+7);
    drawn_arr[index(x+15, y+7)] = 1;

    // row 9
    draw_pixel(pixel, OFF_WHITE, x+12, y+8);
    drawn_arr[index(x+12, y+8)] = 1;

    draw_pixel(pixel, OFF_WHITE, x+13, y+8);
    drawn_arr[index(x+13, y+8)] = 1;
}
