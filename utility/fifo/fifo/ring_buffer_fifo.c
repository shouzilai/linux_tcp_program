#include "ring_buffer_fifo.h"


static int _buffer_fifo_read_byte(buffer_fifo_p fifo)
{
    int ret = fifo->addr[fifo->ptr_read];
    fifo->free_lvl++;
    fifo->writen--;
    fifo->ptr_read = (fifo->ptr_read + 1) % fifo->size;
    return ret;
}

static int _buffer_fifo_write_byte(buffer_fifo_p fifo, int data)
{
    fifo->addr[fifo->ptr_write] = data;
    fifo->free_lvl--;
    fifo->writen++;
    fifo->ptr_write = (fifo->ptr_write + 1) % fifo->size;
    return 0;
}

int buffer_fifo_init(buffer_fifo_p fifo, uint16_t size)
{
    if (NULL == fifo || size <= 0) {
        printf("buffer fifo init arg illegal!!!\n");
        return -1;
    }

    fifo->addr = (uint8_t*)malloc(sizeof(char)*size);
    
    fifo->size = size;
    fifo->writen = 0;
    fifo->free_lvl = size;
    fifo->ptr_read = 0;
    fifo->ptr_write = 0;
    memset(fifo->addr, 0x0, sizeof(size));

    return 0;
}

int buffer_fifo_deinit(buffer_fifo_p fifo)
{
    if( fifo == NULL || fifo->addr == NULL) {
        printf("fifo or fifo_addr is NULL not need to free!!!\n");
        return -1;
    } else {
        free(fifo->addr);
        fifo->addr = NULL;
        return 0;
    }
}

int buffer_fifo_read(buffer_fifo_p fifo, int* read_buffer_addr, int read_buffer_length)
{
    if (NULL == fifo || NULL == read_buffer_addr || 0 == read_buffer_length) {
        return -1;
    } else if (fifo->writen < read_buffer_length) { 
        return -1;
    }
    for(int i = 0; i < read_buffer_length; i++) {
        read_buffer_addr[i] = _buffer_fifo_read_byte(fifo);
    }
    return 0;
}

int buffer_fifo_write(buffer_fifo_p fifo, int* write_buffer_addr, int write_buffer_length)
{
    if (NULL == fifo || NULL == write_buffer_addr || 0 == write_buffer_length) {
        return -1;
    }
    else if (fifo->free_lvl < write_buffer_length) {
        return -1;
    }
    for(int i = 0; i < write_buffer_length; i++) {
        _buffer_fifo_write_byte(fifo,write_buffer_addr[i]);
    }
    return 0;
}
