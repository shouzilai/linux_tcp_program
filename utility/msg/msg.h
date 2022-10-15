#ifndef __MSG_H__
#define __MSG_H__

#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/msg.h>
#include <sys/ipc.h>

#include "../../common.h"

typedef struct msg_queue {
    int cmd;
    uint8_t data[256];
} msg_queue_t, *msg_queue_p;

// 根据key,得到一个消息队列，msgget(key, IPC_CREAT | 0777)，返回消息队列id
int msg_init(int *msg_id);

// 根据msg_id,控制消息队列的属性，删除其，msgctl(msg_id, IPC_RMID, NULL)
int msg_deinit(int msg_id);

// 根据msg_id,向消息队列发送解析后的数据，msgsnd(msg_id, &send_msg, data_len, 0)
int msg_send(int msg_id, int cmd, uint8_t *data_addr, int data_len);

// 根据msg_id,接受消息队列数据到结构体指针，msgrcv(msg_id, recv_msg, sizeof(recv_msg), 0, 0)
int msg_recive(int msg_id, msg_queue_p recv_msg);







#endif // __MSG_H__