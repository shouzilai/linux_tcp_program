#include "tcp_server.h"


static uint8_t addr_ip[16];
tcp_server_p tcp_server; 

void hip_parse_s_success_handle_func(hip_protocol_type_p hip_frame)
{ 
    int      ret       = 0;
    hip_protocol_type_p hip = hip_frame;
    uint8_t  cmd       = hip->command;
    uint8_t* data_addr = hip->payload;
    uint8_t  data_len  = hip->length - 16;

    printf("\n\n-----hip_server_success_handle_func-----\n");
    printf("CMD = %d\n", cmd);
    printf("LEN = %d\n", data_len);
    for (int i = 0; i < data_len; i++) {
        printf("%02x\t", data_addr[i]);
    }
    printf("\n----------------------------------------\n\n");

    if (hip->command == EM_HIP_CMD_KEEP_ALIVE) {
        for (int i = 0; i < TCP_MAX_CLIENT; i++) {
            if (tcp_server->mng_client_table[i].is_online == CLIENT_CONNECT_OFFLINE) {
                continue;
            }
            uint32_t recv_time = 0;
            recv_time = time(NULL);
            ret = pthread_equal(tcp_server->mng_client_table[i].commit_tid, pthread_self());
            // printf("%d client tid is %d, tid self is %d, keep alive is %d, cur time is %d, tid time is %d\n", i, tcp_server->mng_client_table[i].commit_tid, pthread_self(), ret, recv_time, tcp_server->mng_client_table[i].keep_last_time);
            if (ret) {
                tcp_server->mng_client_table[i].keep_last_time = recv_time;
                printf("keep alive is update, position is %d\n", i);
                break;
            }
        }
    } else {
        msg_send(tcp_server->msg_queue_id, cmd, data_addr, data_len); // 上报消息队列
    }
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

void handle_alive_fun(union sigval v)
{
    int flag;
    flag = (tcp_server->alive_flag == 0 ? 1 : 0);
    tcp_server->alive_flag = flag;
    // printf("flag is %d\n", flag);
}

static uint8_t* translate_ip(struct sockaddr_in addr_in)
{
    memset(addr_ip, 0x0, sizeof(addr_ip));

    inet_ntop(AF_INET, &addr_in.sin_addr.s_addr, addr_ip, sizeof(addr_ip));

    return addr_ip;
}

static int client_table_init(mng_clients_info_p table)
{
    for (int i = 0; i < TCP_MAX_CLIENT; i++) {
        table[i].is_online = CLIENT_CONNECT_OFFLINE;
        table[i].client_fd = -1;
        table[i].commit_flag = 0; 
    }
    return 0;
}

static int alive_timer_start(tcp_server_p tcp_s)
{
    int ret = -1;
    struct sigevent evp;
    struct itimerspec ts;
    // timer_t timer = tcp_s->alive_timer;
    
    memset(&evp, 0, sizeof(evp));

    evp.sigev_value.sival_ptr = &tcp_s->alive_timer;
    evp.sigev_notify = SIGEV_THREAD;
    evp.sigev_notify_function = handle_alive_fun;
    evp.sigev_value.sival_int = 3;   //作为handle()的参数
    ret = timer_create(CLOCK_REALTIME, &evp, &tcp_s->alive_timer);
    if(ret){
        perror("timer_create");
    }
   
    ts.it_interval.tv_sec = 2;
    ts.it_interval.tv_nsec = 0;
    ts.it_value.tv_sec = 2;
    ts.it_value.tv_nsec = 0;

    timer_settime(tcp_s->alive_timer, TIMER_ABSTIME, &ts, NULL);
    tcp_s->alive_flag = 1;
    return 0;
}

static int alive_timer_stop(tcp_server_p tcp_s)
{
    tcp_s->alive_flag = 0;
    timer_delete(tcp_s->alive_timer);

    return 0;
}

int tcp_server_init(tcp_server_p server, int msg_id)
{
    int l_fd = -1, ret = -1;
    if (NULL == server) {
        printf("serve init is NULL!!!\n");
        return -1;
    }
    tcp_server = server;
    
    // 创建监听socket
    l_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (l_fd == -1) {
        perror("socket");
        return -1;
    } else {
        printf("socket listen fd success!!!   %d\n", l_fd);
        server->listen_fd = l_fd;
    }
    // 绑定IP和端口号
    struct sockaddr_in *ser_addr =  &server->serve_addr;
    memset(ser_addr, 0x0, sizeof(struct sockaddr_in));
    ser_addr->sin_family = AF_INET;
    ser_addr->sin_addr.s_addr = htonl(INADDR_ANY);
    ser_addr->sin_port = htons(SERVER_PORT);
    
    ret = bind(server->listen_fd, (struct sockaddr*)(ser_addr), sizeof(server->serve_addr));
    if (ret == -1) {
        perror("bind");
        return -1;
    } else {
        show_inet_info(ser_addr, "bind success");
    }
    // 赋值消息队列id
    server->msg_queue_id = msg_id;
    server->add_position = 0;
    server->keep_max_time = KEEP_ALIVE_MAX_TIME;

    // 设置监听socket属性
    ret = listen(server->listen_fd, TCP_MAX_CLIENT);
    if (ret == -1) {
        perror("listen");
        return -1;
    } else {
        printf("listen socket set success!!!\n");
    }
    client_table_init(server->mng_client_table);
   
    return 0;
}

int tcp_server_deinit(tcp_server_p serve)
{
    int ret = -1;
    mng_clients_info_p clients_table = serve->mng_client_table;

    ret = close(serve->listen_fd);
    if (ret == -1) {
        perror("close");
        return -1;
    }
    for (int i = 0; clients_table[i].is_online == CLIENT_CONNECT_ONLINE && i < TCP_MAX_CLIENT; i++) {
        close(clients_table[i].client_fd);
        buffer_fifo_deinit(&clients_table[i].s_fifo);
        hip_deinit(&clients_table[i].hip);
        clients_table[i].is_online = CLIENT_CONNECT_OFFLINE;
    }

    tcp_server = NULL;
    return 0;
}

int tcp_server_start(tcp_server_p serve)
{
    int ret = -1;
    if (NULL == serve) {
        printf("serve start is NULL!!!\n");
        return -1;
    }
    alive_timer_start(serve);

    ret = pthread_create(&serve->mng_tid, NULL, &mng_thread_fun, serve);
    if (ret != -1) {
        printf("[OK][SERVE] mng pthread create success!!!\n");
    } else {
        printf("[ERROR][SERVE] mng pthread create failure!!!\n");
        return -1;
    }

    ret = pthread_create(&serve->keep_tid, NULL, &keep_alive_thread_fun, serve);
    if (ret != -1) {
        printf("[OK][SERVE] keep pthread create success!!!\n");
    } else {
        printf("[ERROR][SERVE] keep pthread create failure!!!\n");
        return -1;
    }

    serve->mng_flag = 1;
    serve->keep_flag = 1;
    pthread_detach(serve->keep_tid);
    printf("tcp serve start\n");

    return 0;    
}

int tcp_server_stop(tcp_server_p serve)
{
    if (NULL == serve) {
        printf("serve stop is NULL!!!\n");
        return -1;
    }
    mng_clients_info_p clients_table = serve->mng_client_table;
    
    serve->mng_flag = 0;
    serve->keep_flag = 0;
    for (int i = 0; clients_table[i].is_online == CLIENT_CONNECT_ONLINE; i++) {
        clients_table[i].commit_flag = 0;
    }
    alive_timer_stop(serve);

    return 0;
}

int tcp_server_send(tcp_server_p server, int sock_fd, hip_protocol_cmd_em cmd, uint8_t* buf, uint16_t buf_len)
{
    if ((sock_fd <= 0) || (NULL == buf)) {
        printf("[ERROR] tcp serve send arg!!!\n");
        return -1;
    }

    printf("enter server inet send\n");
    int ret = 0;
    uint8_t num = 0;
    uint8_t frame_len = 0;
    uint8_t frame_buf[256] = { 0 };
    hip_protocol_handle_p hip = NULL;

    for (num = 0; num < TCP_MAX_CLIENT; num++) {
        if ((uint8_t)server->mng_client_table[num].client_fd == sock_fd) {
            hip = &server->mng_client_table[num].hip;
            break;
        }
    }
    if (num == TCP_MAX_CLIENT) {
        perror("[UNKNOW] tcp_serve_send client sock_fd is null\n");
        return -1;
    }

    frame_len = hip_pack(hip, cmd, frame_buf, 255, buf, buf_len);
    frame_buf[16] = (uint8_t)sock_fd;
    if (frame_len < 0) {
        printf("[ERROR] hip pack\n");
        return -1;
    }

    printf("\nrecv sock_fd = %d\n", sock_fd);
    printf("hip_frame_len = %d", frame_len);
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
    } else {
        perror("send");
    }
 
    return 0;
}

int set_tcp_server_cmd(uint8_t cmd, uint8_t *data, tcp_server_cmd_p inet_p)
{
    printf("start set cmd is %d\n", cmd);
    switch(cmd)
    {
        case EM_HIP_CMD_DETECT_REQUEST:
            printf("client match success, will send response cmd %d\n\n", EM_HIP_CMD_DETECT_REQUEST);
            inet_p->hip_cmd_em = EM_HIP_CMD_DETECT_REQUEST;
            inet_p->target_fd = data[0];
            break;

        case EM_HIP_CMD_USER_LOGIN:
            printf("client login success, will send response cmd %d\n\n", EM_HIP_CMD_USER_LOGIN);
            inet_p->hip_cmd_em = EM_HIP_CMD_USER_LOGIN;
            inet_p->target_fd = data[0];
            break;

        case EM_HIP_CMD_KEEP_ALIVE:
            printf("client keep success, will send response cmd %d\n\n", EM_HIP_CMD_KEEP_ALIVE);
            inet_p->hip_cmd_em = EM_HIP_CMD_KEEP_ALIVE;
            inet_p->target_fd = data[0];
            break;

        case EM_HIP_CMD_UART_PASS_THROUGH:
            printf("client match success, will send response cmd %d\n\n", EM_HIP_CMD_UART_PASS_THROUGH);
            inet_p->hip_cmd_em = EM_HIP_CMD_UART_PASS_THROUGH;
            inet_p->target_fd = data[0];
            break;

        default:
            break;
    }
    return 0;
}



static int is_addr_equal(struct sockaddr_in *a_addr, struct sockaddr_in *b_addr)
{
    char ip_a[16], ip_b[16];

    if (a_addr->sin_port != b_addr->sin_port) {
        return -1;   
    }
    inet_ntop(AF_INET, &a_addr->sin_addr.s_addr, ip_a, sizeof(ip_a));
    inet_ntop(AF_INET, &b_addr->sin_addr.s_addr, ip_b, sizeof(ip_b));
    for (int i = 0; i < sizeof(ip_a); i++) {
        if (ip_a[i] != ip_b[i]) {
            return -1;
        }
    }
    return 0;
}

static int distinguish_client(tcp_server_p tcp_s, struct sockaddr_in *t_addr)
{
    mng_clients_info_p table = tcp_s->mng_client_table;

    for (int i = 0; i < TCP_MAX_CLIENT && table[i].is_online == CLIENT_CONNECT_ONLINE; i++) {
        if (is_addr_equal(&table[i].client_addr, t_addr) == 0) {
            return 0;
        } // 出现相同的客户端连接
    }

    return -1;
}

static int client_info_init(tcp_server_p tcp_s, struct sockaddr_in *t_addr, int c_fd)
{
    printf("start applying for a new client socket is %d\n", c_fd);
    int a_pos = tcp_s->add_position, ret = -1, level = 0;
    mng_clients_info_p table = tcp_s->mng_client_table;

    printf("table pos is %d, state is %d\n", a_pos, table[a_pos].is_online);
    while (level++ < TCP_MAX_CLIENT) {
        if (table[a_pos].is_online == CLIENT_CONNECT_ONLINE) {
            a_pos = (a_pos + 1) % TCP_MAX_CLIENT;
        } else {
            break;
        }
    }
    if (level >= TCP_MAX_CLIENT) {
        printf("clients info table is full!!!\n");
        return -1;
    }
    printf("comfirm position is %d\n", a_pos);
    table[a_pos].keep_last_time = time(NULL);
    tcp_s->add_position = a_pos;

    table[a_pos].client_fd = c_fd;
    table[a_pos].is_online = CLIENT_CONNECT_ONLINE;
    table[a_pos].commit_flag = 1;
    table[a_pos].client_addr = *t_addr;

    hip_init(&tcp_s->mng_client_table[a_pos].hip, hip_parse_s_success_handle_func);
    buffer_fifo_init(&table[a_pos].s_fifo, SERVER_RESERVE_FIFO_SIZE);

    ret = pthread_create(&table[a_pos].commit_tid, NULL, &handle_commit_thread_fun, tcp_s);
    if (ret != -1) {
        printf("[OK][SERVE] commit pthread create success!!!\n");
    } else {
        printf("[ERROR][SERVE] commit pthread create failure!!!\n");
        return -1; 
    }

    pthread_detach(table[a_pos].commit_tid);
    return 0;
}

static int client_info_deinit(const int pos, tcp_server_p tcp_s)
{
    int level = 0;
    mng_clients_info_p table = tcp_s->mng_client_table;
    printf("delete postion is %d, fd is %d\n", pos, table[pos].client_fd);
    if (tcp_s->add_position == pos && pos > 0) {
        tcp_s->add_position--;
    } else {
        tcp_s->add_position = pos;
    }

    close(table[pos].client_fd);
    table[pos].is_online = CLIENT_CONNECT_OFFLINE;
    table[pos].commit_flag = 0;
    memset(&table[pos].client_addr, 0x0, sizeof(struct sockaddr_in));

    if ((NULL == &table[pos].s_fifo) || (NULL == &table[pos].hip)) {
        printf("NULL for fifo or hip\n");
    }
    buffer_fifo_deinit(&table[pos].s_fifo);
    hip_deinit(&table[pos].hip);

    table[pos].keep_last_time = 0;
    printf("client info deinit success!!! pos is :%d\n", pos);

    return 0;
}

static int accept_client(tcp_server_p tcp_s, fd_set *fds, struct sockaddr_in *t_addr)
{
    int flags = -1, c_fd = -1, len = sizeof(struct sockaddr_in);
    if (FD_ISSET(tcp_s->listen_fd, fds)) {
        flags = fcntl(tcp_s->listen_fd, F_GETFL, 0);
        fcntl(tcp_s->listen_fd, F_SETFL, flags | O_NONBLOCK);
        printf("try to accept client\n");
        c_fd = accept(tcp_s->listen_fd, (struct sockaddr*)t_addr, &len);
        if (c_fd == -1) {
            perror("accept");
            return -1;
        } else if (c_fd > 0) {
            printf("connect client socket is %d\n", c_fd);
            return c_fd;
        } else {
            return 0;
        }
    }
}

static int recv_data_client(int fd, fd_set* fds, char* buff)
{
    int len = -1;

    if (FD_ISSET(fd, fds)) {
        memset(buff, 0x0, sizeof(SERVER_BUFFER_SIZE));
        len = recv(fd, buff, sizeof(SERVER_BUFFER_SIZE), MSG_DONTWAIT);
        if (len < 0) {
            if (errno == EWOULDBLOCK) {

            } else {
                perror("recv");
            }
        } else if (len == 0) {
            return 0;           // 客户断开连接，删除表，移除监控的文件描述集FD_CRL(fd, *fd_set);
        } else {
            return len;         // 读取到数据，返回读取到的实际数据长度
        }
    } else {
        return -1;
    }
}

static int write_data_c_fifo(buffer_fifo_p fifo_p, char* buff, int len)
{
    for (int i = 0; i < len; i++) {
        buffer_fifo_write(fifo_p, buff++, 1);
    }
    
    return 0;
}

static int delete_client(int fd, tcp_server_p tcp_s)
{
    for (int i = 0; i < TCP_MAX_CLIENT; i++) {
        int temp_fd = tcp_s->mng_client_table[i].client_fd;
        if (temp_fd == fd) {
            // 回收客户端的资源
            printf("delete client socket %d\n", fd);
            client_info_deinit(i, tcp_s);
            break;
        } else {
            continue;
        }
    }
}

static int recive_data_clients(tcp_server_p tcp_s, fd_set *fds, int *max_fd)
{
    int ret = -1, fd = -1;
    char buffer[SERVER_BUFFER_SIZE];
    for (int i = 0; i < TCP_MAX_CLIENT; i++) {
        printf("%d ", tcp_s->mng_client_table[i].is_online);
    }

    for (int i = 0; i < TCP_MAX_CLIENT; i++) {
        fd = tcp_s->mng_client_table[i].client_fd;
        if (tcp_s->mng_client_table[i].is_online == CLIENT_CONNECT_OFFLINE) {
            continue;
        }
        ret = recv_data_client(fd, fds, buffer);
        if (ret > 0) {
            // 读取到实际数据长度，写入各自的fifo
            write_data_c_fifo(&tcp_s->mng_client_table[i].s_fifo, buffer, ret);
        } else if (ret == 0) {
            // 删除表, 读取socket的结果为 0
            FD_CLR(fd, &tcp_s->serve_fds);
            printf("check fd:%d, is set %d\n", fd, FD_ISSET(fd, &tcp_s->serve_fds));
            if (*max_fd <= fd) {
                *max_fd = *max_fd - 1;
            }
            delete_client(fd, tcp_s);
        } else {
            // 无数据可读
            continue;
        }
    }
    return 0;
}

// 管理连接以及转存客户端发来的HIP数据包到sFIFO
void* mng_thread_fun(void *argc)
{
    tcp_server_p tcp_s = (tcp_server_p)argc;
    if (NULL == tcp_s) {
        printf("tcp serve mng thread is NULL!!!\n");
    }
    int ret = -1, c_fd = -1, len = -1;
    fd_set temp_fds;
    struct sockaddr_in temp_addr;
    struct timeval time_out;
    time_out.tv_sec = 0;
    time_out.tv_usec = 0;

    printf("mng thread running......\n");
    FD_ZERO(&tcp_s->serve_fds);
    FD_SET(tcp_s->listen_fd, &tcp_s->serve_fds);

    int max_fd = tcp_s->listen_fd;
    while (tcp_s->mng_flag) {
        temp_fds = tcp_s->serve_fds;
        ret = select(max_fd + 1, &temp_fds, NULL, NULL, &time_out);
        if (ret == -1) {
            perror("select");
            exit(-1);
        } else {
            c_fd = accept_client(tcp_s, &temp_fds, &temp_addr);
            if (distinguish_client(tcp_s, &temp_addr) != 0 && c_fd > 0) {
                // 判定为 新客户端，分配资源
                client_info_init(tcp_s, &temp_addr, c_fd);
                FD_SET(c_fd, &tcp_s->serve_fds);
            } else {
                // 判定为 老客户端，不再分配资源
                c_fd = max_fd;
            }
            max_fd = c_fd > max_fd? c_fd: max_fd;
        }
        recive_data_clients(tcp_s, &temp_fds, &max_fd);
        printf("max_fd is %d\n", max_fd);
        usleep(100000);
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

static int check_alive(tcp_server_p tcp_s)
{
    uint8_t *addr;
    int time_result = 0, count = 0;
    int cur_time;
    mng_clients_info_p table = tcp_s->mng_client_table;

    for (int i = 0; i < TCP_MAX_CLIENT & tcp_s->alive_flag; i++) {
        if (table[i].is_online == CLIENT_CONNECT_OFFLINE) {
            continue;
        }
        cur_time = time(NULL);
        time_result = (cur_time - table[i].keep_last_time) >= tcp_s->keep_max_time;
        if (time_result) {
            // 客户端下线，回收客户端在表中的资源
            printf("client offline socket %d\n", table[i].client_fd);
            printf("time gab is %d - %d , keep_max_time is %d\n", cur_time, table[i].keep_last_time, tcp_s->keep_max_time);
            delete_client(table[i].client_fd, tcp_s);
            FD_CLR(table[i].client_fd, &tcp_s->serve_fds);

        } else {
            // 根据心跳间隔，发送正常心跳包
            translate_ip(table[i].client_addr);
            tcp_server_send(tcp_s, table[i].client_fd, EM_HIP_CMD_KEEP_ALIVE, addr_ip, sizeof(addr_ip));
            count++;
        }
    }
    if (count > 0) {
        tcp_s->alive_flag = 0;
    }
    return 0;
}

// 管理用户表中客户端的心跳
void* keep_alive_thread_fun(void *argc)
{
    if (argc == NULL) {
        printf("keep alive is NULL!!!\n");
    }
    int ret = -1;
    tcp_server_t *tcp_s = (tcp_server_p)argc;
    while (tcp_s->keep_flag) {
        check_alive(tcp_s);
    }
}

static int handle_hip_parse(int pos, tcp_server_p tcp_s, uint8_t* buff)
{
    if (buffer_fifo_read(&tcp_s->mng_client_table[pos].s_fifo, buff, 1) == 0) {
        if (hip_parse(&tcp_s->mng_client_table[pos].hip, *buff) == 1) {
            // 解析成功，上报消息队列
            return 0;
        } else {
            // 尚未成功
            return -1;
        }
    }
}

// 读取sFIFO，解析HIP，上报消息队列，APP分发数据给客户端
void* handle_commit_thread_fun(void *argc)
{
    int ret = -1;
    uint8_t temp;
    tcp_server_t *tcp_s = (tcp_server_p)argc;

    printf("start handle client data %d\n", tcp_s->mng_client_table[0].client_fd);
    int exec_pos = tcp_s->add_position;
    while (tcp_s->mng_client_table[exec_pos].commit_flag) {
        temp = 0x0;
        ret = handle_hip_parse(exec_pos, tcp_s, &temp);
        if (ret == 0) {
            usleep(100);
        }
    }
}
