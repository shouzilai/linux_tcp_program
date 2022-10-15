#ifndef TCP_CLIENT_H__
#define TCP_CLIENT_H__

#include <unistd.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <signal.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <errno.h>

#include "../../common.h"
#include "../../utility/fifo/ring_buffer_fifo.h"
#include "../../utility/hip/hip.h"
#include "../../utility/msg/msg.h"


#define SERVE_IP_CON            "192.168.41.62"
#define SERVER_PORT_CON         8887

#define KEEP_ALIVE_INTERVAL_C     2
#define KEEP_ALIVE_MAX_TIME_C     10

#define CLIENT_BUFFER_SIZE      2048
#define COMM_FIFO_SIZE          1024
#define RECV_APP_FIFO_SIZE      1024

typedef struct tcp_client {
    int msg_queue_id;
    int comm_fd;

    uint8_t connect_flag;
    uint8_t keep_flag;
    uint8_t commit_flag;

    pthread_t connect_tid;
    pthread_t keep_tid;
    pthread_t commit_tid;

    buffer_fifo_t c_fifo;

    struct sockaddr_in serve_addr;              // 存放绑定的服务器IP和端口

    uint32_t keep_last_time;                    // 上一次心跳时间

    fd_set client_fds;                          // 存放服务端的文件描述符集合

    hip_protocol_handle_t hip;

    timer_t alive_timer;
    int alive_flag;

} tcp_client_t, *tcp_client_p;

typedef struct tcp_client_cmd {
    hip_protocol_cmd_em hip_cmd_em;
    int target_fd;
    
} tcp_client_cmd_t, *tcp_client_cmd_p;


void* recive_serve_thread_fun(void *argc);      // 监听服务端的数据动向，转存到cFIFO

void* keep_c_alive_thread_fun(void *argc);        // 管理与服务端的心跳

void* handle_c_commit_thread_fun(void *argc);     // 取出cFIFO的负载数据随后直接上报消息队列



void hip_parse_c_success_handle_func(hip_protocol_type_p hip_frame);

int tcp_client_init(tcp_client_p client, int msg_id);

int tcp_client_deinit(tcp_client_p client);

int tcp_client_start(tcp_client_p client);

int tcp_client_stop(tcp_client_p client);

int tcp_client_send(tcp_client_p client, int sock_fd, hip_protocol_cmd_em cmd, uint8_t* buf, uint16_t buf_len);

int set_tcp_client_cmd(uint8_t cmd, uint8_t *data, tcp_client_cmd_p inet_p);


#endif // TCP_CLIENT_H__