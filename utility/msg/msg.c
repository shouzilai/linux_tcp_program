#include "msg.h"


// 根据key,得到一个消息队列，msgget(key, IPC_CREAT | 0777)，返回消息队列id
int msg_init(int *msg_id)
{
    int ret = 0;

    key_t key = ftok("./", 120);          // convert  a pathname and a project identifier to a System V IPC key

    if (-1 == key) {
        printf("[ERROR]ftok\n");
    }                                    // key 可以任意指定，也可以使用ftok函数获取键值

    ret = msgget(key, IPC_CREAT | 0777); // 根据key获取一个信号队列，不同的进程调用此函数，只要用相同的key值就能得到同一个消息队列的标识符。
    if (ret == -1) {
        printf("[ERROR]msgget\n");
        return ret;
    }
    *msg_id = ret;                       // msg_id 是一个传出参数
    printf("msg_init\n");
    return 0;
}

// 根据msg_id,控制消息队列的属性，删除其，msgctl(msg_id, IPC_RMID, NULL)
int msg_deinit(int msg_id)
{
    int ret = 0;
    ret = msgctl(msg_id, IPC_RMID, NULL);// 控制消息队列， 消息队列id、IPC_CREAT	IPC_EXCL	IPC_NOWAIT、 NULL
    if(ret < 0) {
        printf("[ERROR]msgctrl\n"); 
        return -1;
    }

    return 0;
}

// 根据msg_id,向消息队列发送解析后的数据，msgsnd(msg_id, &send_msg, data_len, 0)
int msg_send(int msg_id, int cmd, uint8_t *data_addr, int data_len)
{
    int ret = 0;
    msg_queue_t send_msg;
    memset(send_msg.data, 0x0, sizeof(send_msg.data));
    /*
    typedef struct msg_queue
    {
        uint8_t cmd;
        uint8_t data[256];
    }msg_queue_t, *msg_queue_p;
    */
    if (NULL == data_addr) {
        printf("[ERROR]data_addr is null\n");
        return -1;
    }
    for(int i = 0 ; i < data_len ; i++) {
        send_msg.data[i] = data_addr[i];
    }
    send_msg.cmd = cmd;
    printf("send_msg_data:%s, send_msg_cmd:%d\n", send_msg.data, send_msg.cmd);

    ret = msgsnd(msg_id, &send_msg, data_len, 0);          // 使用消息队列发送临时结构体中的数据，借由 msg_id
    if(ret < 0) {
        printf("[ERROR]msgsnd\n");
        return -1;
    }
    printf("commit message success!!!\n");
    return 0;
}

// 根据msg_id,接受消息队列数据到结构体指针，msgrcv(msg_id, recv_msg, sizeof(recv_msg), 0, 0)
int msg_recive(int msg_id, msg_queue_p recv_msg)
{
    if (NULL == recv_msg) {
        printf("[ERROR]ptr is null\n");
        return -1;
    }

    int ret = 0;
    printf("msg_recive blocking......\n");
    memset(recv_msg->data, 0, sizeof(recv_msg->data));
    ret = msgrcv(msg_id, recv_msg, sizeof(recv_msg->data), 0, 0);// 使用消息队列接受结构体指针的数据，借由 msg_id
    if (ret < 0) {
        perror("msgrcv");
        printf("[ERROR]msgrcv\n");
        return -1;
    }

    printf("msg_recive success!!!\n\n");
    return 0;
}