#ifndef __RING_BUFFER_FIFO_H__
#define __RING_BUFFER_FIFO_H__

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUFFER_R_FIFO       512
#define BUFFER_W_FIFO       128

typedef struct buffer_fifo{
    uint8_t *addr;
    uint16_t size;
    uint16_t writen;
    uint16_t free_lvl;
    uint16_t ptr_read;
    uint16_t ptr_write;
}buffer_fifo_t, *buffer_fifo_p;

int buffer_fifo_init(buffer_fifo_p fifo, uint16_t size);

int buffer_fifo_deinit(buffer_fifo_p fifo);

int buffer_fifo_read(buffer_fifo_p fifo, int* read_buffer_addr, int read_buffer_length);

int buffer_fifo_write(buffer_fifo_p fifo, int* write_buffer_addr, int write_buffer_length);

#endif // __RING_BUFFER_FIFO_H__
