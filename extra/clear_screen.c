/* Simple demo file to experiment with the VGA controller on the DE1-SoC
 * Computer System by Altera.
 */

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/shm.h>
#include "address_map_arm_brl4.h"


int main()
{
    int fd;
    if ( (fd = open("/dev/mem", (O_RDWR|O_SYNC))) == -1 ) {
        printf("Error: failed to open \"/dev/mem\".\n");
        close(fd);
        return 1;
    }

    void *pixel_buffer_addr;
    if ( (pixel_buffer_addr = mmap(NULL, FPGA_ONCHIP_SPAN,
                                   (PROT_READ|PROT_WRITE),
                                   MAP_SHARED, fd, FPGA_ONCHIP_BASE))
            == MAP_FAILED )
    {
        printf("Error: failed to map the pixel buffer.\n");
        close(fd);
        return 1;
    }

    volatile unsigned int *pixel = (unsigned int *)pixel_buffer_addr;
    int count = 131072;
    while ( count > 0 ) {
        *pixel++ = 0x0000;
        --count;
    }

    return 0;
}