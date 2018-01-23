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
    volatile unsigned char *pixel_end = (unsigned char *)(pixel_buffer_addr +
                                                          FPGA_ONCHIP_SPAN);

    // Read data from the mouse
    int bytes;
    int left, middle, right;
    int x, y = 0;
    unsigned char drawn_arr[PIXEL_COLS*PIXEL_ROWS];
    signed char data[3];

    while (1) {
        bytes = read(fd_mouse, data, sizeof(data));

        if (bytes > 0) {
            left = data[0] & 0x1;   // first bit
            middle = data[0] & 0x2; // second bit
            right = data[0] & 0x4;  // third bit

	    if (drawn_arr[x*y] == 0) draw_rect(pixel, BLACK, x, y, x, y);

            x += data[1];
            if (x < 0) x = 0;
            else if (x > PIXEL_COLS) x = PIXEL_COLS;
            y -= data[2];
            if (y < 0) y = 0;
            else if (y > PIXEL_ROWS) y = PIXEL_ROWS;

	    if (left && drawn_arr[x*y] == 0 && *(switches) != 2) {
		drawn_arr[x*y] = 1;
	    }
	    else if (left && *(switches) != 2) drawn_arr[x*y] = 0;
	    else if (left && *(switches) == 2) {
		int cpx2 = x;
		int cpy2 = y;
		int cpx1 = cpx2-1;
		int cpy1 = cpy2-1;
		int cpx3 = cpx2+1;
		int cpy3 = cpy2+1;

		// None of the PI pixels go past a boundary
		if (!(cpx1 < 0 || cpy1 < 0 ||
		      cpx3 > PIXEL_COLS || cpy3 > PIXEL_ROWS)) {
		    draw_rect(pixel, OFF_WHITE, cpx3, cpy3, cpx3, cpy3);
		    drawn_arr[cpx3 * cpy3] = 1;
		    draw_rect(pixel, OFF_WHITE, cpx3, cpy2, cpx3, cpy2);
		    drawn_arr[cpx3 * cpy2] = 1;
		    draw_rect(pixel, OFF_WHITE, cpx3, cpy1, cpx3, cpy1);
		    drawn_arr[cpx3 * cpy1] = 1;
		    draw_rect(pixel, OFF_WHITE, cpx2, cpy1, cpx2, cpy1);
		    drawn_arr[cpx2 * cpy1] = 1;
 		    draw_rect(pixel, OFF_WHITE, cpx1, cpy1, cpx1, cpy1);
		    drawn_arr[cpx1 * cpy1] = 1;
		    draw_rect(pixel, OFF_WHITE, cpx1, cpy2, cpx1, cpy2);
		    drawn_arr[cpx1 * cpy2] = 1;
		    draw_rect(pixel, OFF_WHITE, cpx1, cpy3, cpx1, cpy3);
		    drawn_arr[cpx1 * cpy3] = 1;
		}
		else if (cpx1 < 0) {
		    if (cpy1 < 0) {  // The left and top edges pass the boundary
			draw_rect(pixel, OFF_WHITE, cpx3, cpy1, cpx3, cpy1);	
			drawn_arr[cpx3 * cpy1] = 1;
			draw_rect(pixel, OFF_WHITE, cpx3, cpy2, cpx3, cpy2);
			drawn_arr[cpx3 * cpy2] = 1;
		    }
		    else {	// Only the left edge passes the boundary
			draw_rect(pixel, OFF_WHITE, cpx2, cpy1, cpx2, cpy1);
			drawn_arr[cpx2 * cpy1] = 1;
			draw_rect(pixel, OFF_WHITE, cpx3, cpy1, cpx3, cpy1);
			drawn_arr[cpx3 * cpy1] = 1;
			draw_rect(pixel, OFF_WHITE, cpx3, cpy2, cpx3, cpy2);
			drawn_arr[cpx3 * cpy2] = 1;
			draw_rect(pixel, OFF_WHITE, cpx3, cpy3, cpx3, cpy3);
			drawn_arr[cpx3 * cpy3] = 1;
		    }
		}
		else if (cpy1 < 0) {  // Only the top edge passes the boundary
		    draw_rect(pixel, OFF_WHITE, cpx1, cpy2, cpx1, cpy2);
		    drawn_arr[cpx1 * cpy2] = 1;
		    draw_rect(pixel, OFF_WHITE, cpx1, cpy3, cpx1, cpy3);
		    drawn_arr[cpx1 * cpy3] = 1;
		    draw_rect(pixel, OFF_WHITE, cpx3, cpy3, cpx3, cpy3);
		    drawn_arr[cpx3 * cpy3] = 1;
		    draw_rect(pixel, OFF_WHITE, cpx3, cpy2, cpx3, cpy2);
		    drawn_arr[cpx3 * cpy2] = 1;
		}
		else if (cpx3 > PIXEL_COLS) {
		    if (cpy3 > PIXEL_ROWS) {  // The right and bottom edges pass the boundary
			draw_rect(pixel, OFF_WHITE, cpx1, cpy2, cpx1, cpy2);
			drawn_arr[cpx1 * cpy2] = 1;
			draw_rect(pixel, OFF_WHITE, cpx1, cpy1, cpx1, cpy1);	
			drawn_arr[cpx1 * cpy1] = 1;
			draw_rect(pixel, OFF_WHITE, cpx2, cpy1, cpx2, cpy1);
			drawn_arr[cpx2 * cpy1] = 1;
		    }
		    else {  // Only the right edge passes the boundary
			draw_rect(pixel, OFF_WHITE, cpx1, cpy3, cpx1, cpy3);
			drawn_arr[cpx1 * cpy3] = 1;
			draw_rect(pixel, OFF_WHITE, cpx1, cpy2, cpx1, cpy2);
			drawn_arr[cpx1 * cpy2] = 1;
			draw_rect(pixel, OFF_WHITE, cpx1, cpy1, cpx1, cpy1);
			drawn_arr[cpx1 * cpy1] = 1;
			draw_rect(pixel, OFF_WHITE, cpx2, cpy1, cpx2, cpy1);
			drawn_arr[cpx2 * cpy1] = 1;
		    }
		}
		else if (cpy3 > PIXEL_ROWS) { // Only the bottom edge passes the boundary
		    draw_rect(pixel, OFF_WHITE, cpx3, cpy2, cpx3, cpy2);
		    drawn_arr[cpx3 * cpy2] = 1;
		    draw_rect(pixel, OFF_WHITE, cpx3, cpy1, cpx3, cpy1);
		    drawn_arr[cpx3 * cpy1] = 1;
		    draw_rect(pixel, OFF_WHITE, cpx2, cpy1, cpx2, cpy1);
		    drawn_arr[cpx2 * cpy1] = 1;
		    draw_rect(pixel, OFF_WHITE, cpx1, cpy1, cpx1, cpy1);
		    drawn_arr[cpx1 * cpy1] = 1;
		    draw_rect(pixel, OFF_WHITE, cpx1, cpy2, cpx1, cpy1);
		    drawn_arr[cpx1 * cpy2] = 1;
		}
	    }
	    
 	    draw_rect(pixel, OFF_WHITE, x, y, x, y);
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

    while (y <= y2) {
        while (x <= x2) {
            *(addr_base + x + (y<<10)) = color;
            ++x;
        }
        x = x1;
        ++y;
    }
}
