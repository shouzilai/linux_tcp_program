#include "tcp_client.h"


static uint8_t addr_ip[16];
static tcp_client_p tcp_client;

void hip_parse_c_success_handle_func(hip_protocol_type_p hip_frame)
{
    int ret = 0;
    hip_protocol_type_p hip = hip_frame;
    uint8_t cmd = hip->command;
    uint8_t* data_addr = hip->payload;
    uint8_t data_len = hip->length - 16;

    printf("\n\n-----hip_client_success_handle_func-----\n");
    printf("CMD = %d\n", cmd);
    printf("LEN = %d\n", data_len);
    for (int i = 0; i < data_len; i++) {
        printf("%02x\t", data_addr[i]);
    }
    printf("\n----------------------------------------\n\n");

    if (hip->command == EM_HIP_CMD_KEEP_ALIVE) {
        uint32_t recv_time = 0;
        recv_time = time(NULL);
        tcp_client->keep_last_time = recv_time;
        printf("keep alive is update\n");
    } else {
        msg_send(tcp_client->msg_queue_id, cmd, (uint8_t*)data_addr, data_len); // 上报消息队列
    }
}

static uint8_t* translate_ip(struct sockaddr_in addr_in)
{
    memset(addr_ip, 0x0, sizeof(addr_ip));

    inet_ntop(AF_INET, &addr_in.sin_addr.s_addr, addr_ip, sizeof(addr_ip));

    return addr_ip;
}

static void show_inet_info(struct sockaddr_in* addr, char* source)
{
    char addr_ip[16];
    unsigned short addr_port;

    inet_ntop(AF_INET, &addr->sin_addr.s_addr, addr_ip, sizeof(addr_ip));
    addr_port = ntohs(addr->sin_port);

    printf("%s   addr IP is %s, port is %d\n", source, addr_ip, addr_port);
    return;
}

void handle_alive_fun_c(union sigval v)
{
    int flag;
    flag = (tcp_client->alive_flag == 0 ? 1 : 0);
    tcp_client->alive_flag = flag;
    printf("flag is %d\n", flag);
}

static int alive_timer_start_c(tcp_client_p tcp_c)
{
    int ret = -1;
    struct sigevent evp;
    struct itimerspec ts;
    
    memset(&evp, 0, sizeof(evp));

    evp.sigev_value.sival_ptr = &tcp_c->alive_timer;
    evp.sigev_notify = SIGEV_THREAD;
    evp.sigev_notify_function = handle_alive_fun_c;
    evp.sigev_value.sival_int = 3;   //作为handle()的参数
    ret = timer_create(CLOCK_REALTIME, &evp, &tcp_c->alive_timer);
    if(ret){
        perror("timer_create");
    }
   
    ts.it_interval.tv_sec = 2;
    ts.it_interval.tv_nsec = 0;
    ts.it_value.tv_sec = 2;
    ts.it_value.tv_nsec = 0;

    timer_settime(tcp_c->alive_timer, TIMER_ABSTIME, &ts, NULL);
    tcp_c->alive_flag = 0;

    return 0;
}

static int alive_timer_stop_c(tcp_client_p tcp_s)
{
    tcp_s->alive_flag = 0;
    timer_delete(tcp_s->alive_timer);

    return 0;
}


int tcp_client_init(tcp_client_p client, int msg_id)
{
    if (NULL == client) {
        printf("client init is NULL!!!\n");
    }
    int ret = -1, c_fd;
    client->msg_queue_id = msg_id;
    
    // 创建通信socket
    c_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (c_fd == -1) {
        perror("socket");
        return -1;
    } else {
        printf("socket commuicate fd success!!!   %d", c_fd);
        client->comm_fd = c_fd;
    }
    // 根据服务端暴露的IP和端口号连接
    struct sockaddr_in *ser_addr = &client->serve_addr;
    memset(ser_addr, 0x0, sizeof(struct sockaddr_in));
    ser_addr->sin_family = AF_INET;
    ser_addr->sin_port = htons(SERVER_PORT_CON);
    inet_pton(AF_INET, SERVE_IP_CON, &ser_addr->sin_addr.s_addr);

    // 申请FIFO
    hip_init(&client->hip, hip_parse_c_success_handle_func);
    ret = buffer_fifo_init(&client->c_fifo, COMM_FIFO_SIZE);
    if (ret < 0) {
        printf("[ERROR] client fifo init faliure!!!\n");
        return -1;
    }

    tcp_client = client;
    return 0;
}

int tcp_client_deinit(tcp_client_p client)
{
    if (NULL == client) {
        printf("client deinit is NULL!!!\n");
        return -1;
    }
    close(client->comm_fd);
    buffer_fifo_deinit(&client->c_fifo);
    hip_deinit(&client->hip);

    tcp_client = NULL;
    return 0;
}

int tcp_client_start(tcp_client_p client)
{
    int ret = -1;
    if (NULL == client) {
        printf("client start is NULL!!!\n");
        return -1;
    }
    alive_timer_start_c(client);

    ret = pthread_create(&client->connect_tid, NULL, &recive_serve_thread_fun, client);
    if (ret != -1) {
        printf("[OK][CLIENT] connect pthread create success!!!\n");
    } else {
        printf("[ERROR][CLIENT] connect pthread create failure!!!\n");
        return -1;
    }

    ret = pthread_create(&client->keep_tid, NULL, &keep_c_alive_thread_fun, client);
    if (ret != -1) {
        printf("[OK][CLIENT] keep pthread create success!!!\n");
    } else {
        printf("[ERROR][CLIENT] kepp pthread create failure!!!\n");
        return -1;
    }

    ret = pthread_create(&client->commit_tid, NULL, &handle_c_commit_thread_fun, client);
    if (ret != -1) {
        printf("[OK][CLIENT] commit pthread create success!!!\n");
    } else {
        printf("[ERROR][CLIENT] commit pthread create failure!!!\n");
        return -1;
    }

    client->connect_flag = 1;
    client->keep_flag = 1;
    client->commit_flag = 1;
    printf("tcp client start......\n");
    return 0; 
}

int tcp_client_stop(tcp_client_p client)
{
    if (NULL == client) {
        printf("client stop is NULL!!!\n");
        return -1;
    }

    alive_timer_stop_c(client);
    client->connect_flag = 0;
    client->keep_flag = 0;
    client->commit_flag = 0;

    return 0;
}

int tcp_client_send(tcp_client_p client, int sock_fd, hip_protocol_cmd_em cmd, uint8_t* buf, uint16_t buf_len)
{
    printf("sock fd is %d, buf is %hhn\n", sock_fd, buf);
    if ((sock_fd <= 0) || (NULL == buf)) {
        printf("[ERROR] tcp client send arg!!!\n");
        return -1;
    }
    if (cmd == EM_HIP_CMD_WAIT) {
        printf("client request hip has been acked!!!  cmd is %d\n", cmd);
        return 0;
    }
    printf("enter client inet send\n");
    int ret = 0;
    uint8_t num = 0;
    uint8_t frame_len = 0;
    uint8_t frame_buf[256] = { 0 };
    hip_protocol_handle_p hip = NULL;

    hip = &client->hip;

    frame_len = hip_pack(hip, cmd, frame_buf, 255, buf, buf_len); // 打包需要发送的数据，命令，数据长度
    frame_buf[16] = (uint8_t)sock_fd;
    if (frame_len < 0) {
        printf("[ERROR] hip pack\n");
        return -1;
    }

    printf("\nrecv sock_fd = %d\n", sock_fd);
    printf("hip_frame_len = %d, cmd is %d", frame_len, cmd);
    for (int i = 0; i < frame_len; i++) {
        if (i % 4 == 0) {
            printf("\n");
        }
        printf("%02x\t", frame_buf[i]);
    }
    printf("\n");

    ret = send(sock_fd, frame_buf, frame_len, 0);
    if (ret == -1) {
        perror("send");
        return -1;
    }

    return 0;
}

int set_tcp_client_cmd(uint8_t cmd, uint8_t *data, tcp_client_cmd_p inet_p)
{
    switch(cmd)
    {
        case EM_HIP_CMD_DETECT_REQUEST:
            inet_p->hip_cmd_em = EM_HIP_CMD_USER_LOGIN;
            inet_p->target_fd = data[0];
            printf("server match success, will send request cmd %d\n\n", EM_HIP_CMD_DETECT_REQUEST);
            break;

        case EM_HIP_CMD_USER_LOGIN:
            inet_p->hip_cmd_em = EM_HIP_CMD_WAIT;
            inet_p->target_fd = data[0];
            printf("server match success, will send request cmd %d\n\n", EM_HIP_CMD_USER_LOGIN);
            break;

        case EM_HIP_CMD_KEEP_ALIVE:
            inet_p->hip_cmd_em = EM_HIP_CMD_WAIT;
            inet_p->target_fd = data[0];
            printf("server match success, will send response cmd %d\n\n", EM_HIP_CMD_KEEP_ALIVE);
            break;

        case EM_HIP_CMD_UART_PASS_THROUGH:
            inet_p->hip_cmd_em = EM_HIP_CMD_UART_PASS_THROUGH;
            inet_p->target_fd = data[0];
            printf("server match success, will send request cmd %d\n\n", EM_HIP_CMD_UART_PASS_THROUGH);
            break;

        default:
            break;
    }
    return 0;
}



static int recive_data_server(int fd, fd_set *fds, char* buff)
{
    int ret = -1, len = -1;

    if (FD_ISSET(fd, fds)) {
        ret = recv(fd, buff, sizeof(CLIENT_BUFFER_SIZE), 0);
        if (ret == -1) {
            perror("recv");
            return -1;
        } else if (ret == 0) {
            return 0;
        } else {
            len = ret;
            return len;
        }
    } else {
        return 0;
    }
}

static int write_data_c_fifo(buffer_fifo_p fifo_p, char* buff, int len)
{
    for (int i = 0; i < len; i++) {
        buffer_fifo_write(fifo_p, buff++, 1);
    }
    return 0;
}

static int retry_connect_server(tcp_client_p tcp_c)
{
    int ret = -1;
    static int retry_count = 0;
    if (retry_count > 2) {
        exit(-1);
    }
    if (FD_ISSET(tcp_c->comm_fd, &tcp_c->client_fds)) {
        FD_CLR(tcp_c->comm_fd, &tcp_c->client_fds);
    }
    close(tcp_c->comm_fd);

    ret = socket(AF_INET, SOCK_STREAM, 0);
    FD_SET(tcp_c->comm_fd, &tcp_c->client_fds);
    if (ret == -1) {
        perror("retry socket");
        return -1;
    } else {
        tcp_c->comm_fd = ret;
        tcp_c->keep_last_time = time(NULL);
        printf(" retry socket success\n");
    }
    printf("try again comm fd is %d\n", tcp_c->comm_fd);

    ret = connect(tcp_c->comm_fd, (struct sockaddr*)&tcp_c->serve_addr, sizeof(tcp_c->serve_addr));
    if (ret == -1) {
        perror("connect");
        return -1;
    } else {
        show_inet_info(&tcp_c->serve_addr, "retry connect server");
    }
    tcp_client_send(tcp_c, tcp_c->comm_fd, EM_HIP_CMD_DETECT_REQUEST, NULL, 0);     // 用户配对请求消息包

    retry_count++;
    return 0;
}

static int check_readable(tcp_client_p tcp_c, fd_set *fds, char* buff)
{
    int ret = select(tcp_c->comm_fd + 1, fds, NULL, NULL, NULL);
    if (ret == -1) {
        perror("select");
        return -1;
    } else if (ret > 0) {
        // 服务端socket有读事件
        int len = recive_data_server(tcp_c->comm_fd, fds, buff);
        if (len == 0) {
            // 服务端断开连接，进入断线重连
            // retry_connect_server(tcp_c);
            return 0;
        } else if (len == -1) {
            // recv出错
            printf("recv error\n");
            return -1;
        } else {
            // 正常读写
            return len;
        }
    } else {
        return 0;
    }
}

static uint8_t* translate_device_id(uint64_t decv_id)
{
    static uint8_t decive_id[8];
    for (int i = 0; i < 8; i++) {
        decive_id[i] = decv_id >> 8 * i;
    }

    return decive_id;
}

// 监听服务端的数据动向，转存到cFIFO
void* recive_serve_thread_fun(void *argc)
{
    int ret = -1, len = -1;
    char buffer[CLIENT_BUFFER_SIZE];
    tcp_client_t *tcp_c = (tcp_client_p)argc;
    fd_set tmp_set;
    
    FD_ZERO(&tcp_c->client_fds);
    FD_SET(tcp_c->comm_fd, &tcp_c->client_fds);

    ret = connect(tcp_c->comm_fd, (struct sockaddr*)&tcp_c->serve_addr, sizeof(tcp_c->serve_addr));
    if (ret == -1) {
        perror("connect");
        exit(-1);
    } else {
        tcp_c->keep_last_time = time(NULL);
        show_inet_info(&tcp_c->serve_addr, "connect server");
    }
    uint8_t *addr = translate_ip(tcp_c->serve_addr);
    tcp_client_send(tcp_c, tcp_c->comm_fd, EM_HIP_CMD_DETECT_REQUEST, addr, sizeof(addr_ip));     // 用户配对请求消息包

    while (tcp_c->connect_flag) {
        tmp_set = tcp_c->client_fds;

        ret = check_readable(tcp_c, &tmp_set, buffer);
        if (ret == -1) {
            exit(-1);
        } else {
            usleep(10000);
        }
        len = ret;
        write_data_c_fifo(&tcp_c->c_fifo, buffer, len);
    }
}

static int check_alive(tcp_client_p tcp_c)
{
    int count = 0, time_result = 0;
    uint8_t *addr = translate_ip(tcp_c->serve_addr);
    uint32_t cur_time = time(NULL);

    time_result = cur_time - tcp_c->keep_last_time >= KEEP_ALIVE_MAX_TIME_C;
    if (time_result && tcp_c->alive_flag) {
        // 断线重连，发送 登录请求包 
        printf("time gab is %d - %d , keep_max_time is %d\n", cur_time, tcp_c->keep_last_time, KEEP_ALIVE_MAX_TIME_C);
        retry_connect_server(tcp_c);
        // tcp_client_send(tcp_c, tcp_c->comm_fd, EM_HIP_CMD_USER_LOGIN, addr, sizeof(addr_ip));
        return 0;
    } else {
        // 根据心跳间隔，发送 正常心跳包
        if (tcp_c->alive_flag) {
            tcp_client_send(tcp_c, tcp_c->comm_fd, EM_HIP_CMD_KEEP_ALIVE, addr, sizeof(addr_ip));
            tcp_c->alive_flag = 0;
            return 0;
        } else {
            usleep(10000);
            return 0;
        }
    }
}

// 管理与服务端的心跳
void* keep_c_alive_thread_fun(void *argc)
{
    int ret = -1;
    tcp_client_t *tcp_c = (tcp_client_p)argc;

    while (tcp_c->keep_flag) {
        ret = check_alive(tcp_c);
        if (ret == -1) {
            exit(-1);
        }
    }
}

static int handle_hip_parse(tcp_client_p tcp_c, uint8_t* buff)
{
    if (buffer_fifo_read(&tcp_c->c_fifo, buff, 1) == 0) {
        printf("read fifo data is %x\n", *buff);
        if (hip_parse(&tcp_c->hip, *buff) == 1) {
            // 解析成功，上报消息队列
            return 0;
        } else {
            // 尚未成功
            usleep(100);
            return -1;
        }
    } else {
        usleep(100);
        return -1;
    }
}

// 取出cFIFO的负载数据随后直接上报消息队列
void* handle_c_commit_thread_fun(void *argc)
{
    int ret = -1;
    uint8_t *temp = (uint8_t*)malloc(sizeof(uint8_t));
    tcp_client_t *tcp_c = (tcp_client_p)argc;

    while (tcp_c->connect_flag) {
        memset(temp, 0x0, 1);
        handle_hip_parse(tcp_c, temp);
    }
    free(temp);
}

