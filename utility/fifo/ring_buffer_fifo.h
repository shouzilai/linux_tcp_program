#ifndef __RING_BUFFER_FIFO_H__
#define __RING_BUFFER_FIFO_H__


#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#define BUFFER_R_FIFO       512
#define BUFFER_W_FIFO       128

typedef struct buffer_fifo {
    int size_mark;
    int read;
    int write;
    uint8_t *buffer;

} buffer_fifo_t, *buffer_fifo_p;


int buffer_fifo_init(buffer_fifo_p fifo_p, int size);

int buffer_fifo_deinit(buffer_fifo_p fifo_p);

int buffer_fifo_read(buffer_fifo_p fifo_p, uint8_t *buffer, int size);

int buffer_fifo_write(buffer_fifo_p fifo_p, uint8_t *buffer, int size);


#endif // __RING_BUFFER_FIFO_H__