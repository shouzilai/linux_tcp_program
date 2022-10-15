#ifndef __UART_H__
#define __UART_H__

#include <termios.h>
#include <linux/ioctl.h>
#include <linux/serial.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <net/if.h>
#include <pthread.h>
#include <sys/msg.h>
#include <sys/ipc.h>
#include <sys/select.h>

#include "../common.h"
#include "../utility/hup/hup.h"
#include "../utility/msg/msg.h"
#include "../utility/fifo/ring_buffer_fifo.h"

#define TTY_DEFAULT_DEVICE      "/dev/ttyUSB0"
#define TTY_DEFAULT_BAUDRATE    9600
#define TTY_DEFAULT_DATA        8
#define TTY_DEFAULT_FLOW        0
#define TTY_DEFAULT_PAR         'n'
#define TTY_DEFAULT_STOP        1

#define TTY_READ_TIMEOUT_USEC	5000
#define TTY_READ_BUFFER_SIZE    256
#define TTY_RECIVE_BUFFER_SIZE	512
#define TTY_SEND_BUFFER_SIZE    512
#define TTY_FRAME_PAYLOAD_SIZE  128

typedef struct tty {
    int tty_fd;
    uint8_t* recive_buff;
    int* send_buff;

    pthread_t recive_td;
    pthread_t handle_td;
    pthread_t send_td;

    int recive_flag;
    int handle_flag;
    int send_flag;

    buffer_fifo_t r_fifo;
    buffer_fifo_t w_fifo;

    hup_protocol_type_t hup_frame;

} tty_t, *tty_p;

typedef struct tty_dev {
    char tty_dev[MAX_UART_NAME_LEN];    // 字符数组最大为256个字节
    int bitrate;                        // 比特率 operate mode. 0: RS232, 1: RS485 default mode: 0
    int datasize;                       // 数据大小
    char par;                           // 奇偶检验位
    int stop;                           // 停止位
    int flow;                           // flow control
    int rs485_mode;                     // 模式

} tty_dev_t, *tty_dev_p;

typedef struct uart_cmd {
    int ret_cmd;

} uart_cmd_t, *uart_cmd_p;


void* recive_thread_fun(void *argc);

void* handle_thread_fun(void *argc);

void* send_thread_fun(void *argc);


int	tty_init(tty_p tty_pointer, int msg_id);   // 初始化tty结构体

int tty_deinit(tty_p tty_pointer);

int tty_start(tty_p uart);

int tty_stop(tty_p uart);

int tty_send(tty_p uart, hup_protocol_type_em sort, int cmd, uint8_t* addr, int addr_len);

int set_uart_cmd(uint8_t cmd, uart_cmd_p urat_ptr);


#endif	 // __UART_H__

