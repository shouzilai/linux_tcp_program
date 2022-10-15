#include "ring_buffer_fifo.h"


int buffer_fifo_init(buffer_fifo_p fifo_p, int size)
{
    if (fifo_p == NULL) {
        return -1;
    }

    fifo_p->read = 0;
    fifo_p->write = 0;
    fifo_p->size_mark = size - 1;
    fifo_p->buffer = (uint8_t*)malloc(sizeof(uint8_t)*size);

    return 0;
}

int buffer_fifo_deinit(buffer_fifo_p fifo_p)
{
    if (fifo_p == NULL) {
        return -1;
    }
    
    fifo_p->read = 0;
    fifo_p->write = 0;
    fifo_p->size_mark = 0;
    free(fifo_p->buffer);
    fifo_p->buffer = NULL;

    return 0;
}


static inline int ring_fifo_is_full(buffer_fifo_p fifo_p)
{
    int data_size = (fifo_p->write - fifo_p->read) & fifo_p->size_mark;
    return (data_size == fifo_p->size_mark); // 如果为满载返回，1
}

static inline int ring_fifo_is_empty(buffer_fifo_p fifo_p)
{
    return (fifo_p->write == fifo_p->read); // 如果为空返回，1
}

static inline int ring_fifo_data_number(buffer_fifo_p fifo_p)
{
    return (fifo_p->write - fifo_p->read) & fifo_p->size_mark;
}


static int ring_fifo_single_read(buffer_fifo_p fifo_p, uint8_t *buffer)
{
    if (ring_fifo_is_empty(fifo_p)) {
        return -1;
    }
    *buffer = fifo_p->buffer[fifo_p->read];
    fifo_p->read = (fifo_p->read + 1) % (fifo_p->size_mark + 1);
    return 0;
}

int buffer_fifo_read(buffer_fifo_p fifo_p, uint8_t *buffer, int size)
{
    if (fifo_p == NULL) {
        return -1;
    }
    if (ring_fifo_is_empty(fifo_p)) {
        return -1;
    }
    uint8_t *data = buffer;
    int count = 0;
    while (count < size && (ring_fifo_single_read(fifo_p, data) == 0)) {
        count++;
        data++;
    }
    return 0;
}

static int ring_fifo_single_write(buffer_fifo_p fifo_p, const uint8_t buffer)
{
    if (ring_fifo_is_full(fifo_p)) {
        return -1;
    }
    fifo_p->buffer[fifo_p->write] = buffer;
    fifo_p->write = (fifo_p->write + 1) % (fifo_p->size_mark + 1);
    return 0; 
}

int buffer_fifo_write(buffer_fifo_p fifo_p, uint8_t *buffer, int size)
{
    if (fifo_p == NULL) {
        return -1;
    }
    int write_size = (fifo_p->size_mark + 1) - ring_fifo_data_number(fifo_p);
    if (size > write_size) {
        return -1;
    }
    for (int i = 0; i < size && (ring_fifo_single_write(fifo_p, buffer[i]) == 0); i++) {
        
    }
    return 0;
}