#ifndef __APP_H__
#define __APP_H__

#include <pthread.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#include "../common.h"
#include "../uart_app/uart_app.h"
#include "../led_app/led_app.h"
#include "../utility/msg/msg.h"
#include "../inet_app/tcp_client/tcp_client.h"
#include "../inet_app/tcp_server/tcp_server.h"


typedef struct cmd {
    led_cmd_t led_cmds;
    uart_cmd_t uart_cmds;
    tcp_client_cmd_t client_cmds;
    tcp_server_cmd_t server_cmds;

}cmd_t, *cmd_p;


typedef struct app {
    pthread_t app_td;
    int app_flag;

    int msg_id;
    msg_queue_t message;

    tty_t uart;
    led_t led;
    cmd_t cmds;
    tcp_server_t server;
    tcp_client_t client;

}app_t, *app_p;



void *app_process_thread_entry(void *argc);	// 接受消息队列的消息，调用处理函数

int app_start(app_p app);					// 设置app_flag为真，开启app线程

int app_stop(app_p app);					// 终止app_flag

void app_process(app_p app);				// 分别处理三种情况的状态机，控制led, uart, inet, stop

#endif // __APP_H__