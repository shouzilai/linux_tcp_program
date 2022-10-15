#ifndef __TCP_SERVE_H__
#define __TCP_SERVE_H__

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

#define SERVER_PORT                 8887
#define TCP_MAX_CLIENT              10

#define CLIENT_CONNECT_ONLINE       1
#define CLIENT_CONNECT_OFFLINE      0

#define KEEP_ALIVE_INTERVAL         2
#define KEEP_ALIVE_MAX_TIME         10

#define SERVER_BUFFER_SIZE          2048
#define SERVER_RESERVE_FIFO_SIZE    512


typedef struct mng_clients_info {
    int client_fd;
    int is_online;
    uint8_t commit_flag;
    pthread_t commit_tid; 

    buffer_fifo_t s_fifo;

    struct sockaddr_in client_addr;

    int keep_last_time;                                 // 上一次服务器收到客户端心跳包时间

    hip_protocol_handle_t hip;
} mng_clients_info_t, *mng_clients_info_p;

typedef struct tcp_server {
    int listen_fd;                                      // int socket(int domain, int type, int protocol);
    int msg_queue_id;
    int add_position;

    uint8_t mng_flag;
    uint8_t keep_flag;
    pthread_t mng_tid;
    pthread_t keep_tid;              

    struct sockaddr_in serve_addr;                      // 存放绑定的服务器IP和端口

    mng_clients_info_t mng_client_table[TCP_MAX_CLIENT];

    int keep_max_time;                                  // 最大心跳间隔时间

    fd_set serve_fds;                                   // 存放客户端的文件描述符集合

    timer_t alive_timer;
    int alive_flag;

} tcp_server_t, *tcp_server_p;

typedef struct tcp_server_cmd {
    hip_protocol_cmd_em hip_cmd_em;
    int target_fd;
    
} tcp_server_cmd_t, *tcp_server_cmd_p;


void* mng_thread_fun(void *argc);           // 管理连接以及转存客户端发来的HIP数据包到sFIFO

void* keep_alive_thread_fun(void *argc);    // 管理用户表中客户端的心跳

void* handle_commit_thread_fun(void *argc); // 读取sFIFO，解析HIP，上报消息队列，APP分发数据给客户端



void hip_parse_s_success_handle_func(hip_protocol_type_p hip_frame);

int tcp_server_init(tcp_server_p server, int msg_id);

int tcp_server_deinit(tcp_server_p server);

int tcp_server_start(tcp_server_p server);

int tcp_server_stop(tcp_server_p server);

int tcp_server_send(tcp_server_p server, int sock_fd, hip_protocol_cmd_em cmd, uint8_t* buf, uint16_t buf_len);

int set_tcp_server_cmd(uint8_t cmd, uint8_t *data, tcp_server_cmd_p inet_p);


#endif // __TCP_SERVE_H__