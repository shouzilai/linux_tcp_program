#include "app.h"

// 设置app_flag为真，开启app线程
int app_start(app_p app)
{
    if(NULL == app) {
        perror("[ERROR]app ptr is NULL\n");
        return -1;
    }
    int ret = 0;
    
    memset(app->message.data, 0, sizeof(app->message.data));

    ret = pthread_create(&app->app_td, NULL, &app_process_thread_entry, app);
    if(ret < 0){
        printf("[ERROR] app thread create failure!!!\n");
    } else{
        printf("[OK] app thread create success!!!\n");
    }
    app->app_flag = 1;
    printf("app_start\n");
    return 0;
}

// 终止app_flag
int app_stop(app_p app)
{
    if(NULL == app) {
        perror("[ERROR]app ptr is NULL\n");
        return -1;
    }
    
    app->app_flag = 0;
    return 0;
}

static int parse_cmd(uint8_t cmd, uint8_t *data, cmd_p cmd_ptr)
{
    printf("parse cmd is %d\n", cmd);
    if (cmd == 0x00) {
        return 0;

    } else if (cmd > 0x00 && cmd <= 0x04) {
        printf("led cmd implement......%x\n", cmd);
        set_led_cmd(cmd, &cmd_ptr->led_cmds);
        return 1;

    } else if (cmd > 0x04 && cmd <= 0x8) {
        printf("inet cmd implement......%x\n", cmd);
#if INET_TCP_SERVER
        set_tcp_server_cmd(cmd, data, &cmd_ptr->server_cmds);
#else
        set_tcp_client_cmd(cmd, data, &cmd_ptr->client_cmds);
#endif
        return 3;

    } else {
        printf("uart cmd implement......%x\n", cmd);
        set_uart_cmd(cmd, &cmd_ptr->uart_cmds);
        return 2;
    }
}

static int state_led(app_p app, uint8_t cmd, uint8_t* data, int data_len)
{
    printf("start deal led......\n");
    led_cmd_p leds_cmd = &(app->cmds.led_cmds);
    app_led_conf(&app->led, leds_cmd->led_num, leds_cmd->led_mode, leds_cmd->val, leds_cmd->ctrl_num);	// 发送命令，依据参数，设置led，开启定时器

    tty_send(&app->uart, EM_HUP_TYPE_ACK, cmd, data, data_len);
    return 0;
}

static int state_uart(app_p app, uint8_t cmd, uint8_t* data, int data_len)
{
    tty_send(&app->uart, EM_HUP_TYPE_ACK, cmd, data, data_len);		// 发送命令，让数据写入，send_fifo中，长度为 data_len
    printf("< OK > -------- [UART]\n");
#if INET_TCP_SERVER
    
#else
    if (app->client.comm_fd != -1) {
        tcp_client_send(&app->client, app->client.comm_fd, EM_HIP_CMD_UART_PASS_THROUGH, data, data_len);
    }
#endif    
    return 0;
}

static int state_inet(app_p app, uint8_t cmd, uint8_t* data, int data_len)
{
#if INET_TCP_SERVER
    tcp_server_cmd_p server_cmd = &app->cmds.server_cmds;
    tcp_server_send(&app->server, server_cmd->target_fd, server_cmd->hip_cmd_em, data, data_len);
#else
    tcp_client_cmd_p client_cmd = &app->cmds.client_cmds;
    tcp_client_send(&app->client, client_cmd->target_fd, client_cmd->hip_cmd_em, data, data_len);
    if (client_cmd->hip_cmd_em == EM_HIP_CMD_UART_PASS_THROUGH) {
        tty_send(&app->uart, EM_HUP_TYPE_ACK, cmd, data, data_len);
    }
#endif
    printf("< OK > -------- [INET]\n");
    return 0;
}

static int state_system(app_p app)
{
#if INET_TCP_SERVER
    tcp_server_stop(&app->server);
#else
    tcp_client_stop(&app->client);
    tty_stop(&app->uart);
    app_led_stop(&app->led);
#endif
    app_stop(app);

    printf("\n\n>>>>>>>>>>>>> quit >>>>>>>>>>>>>>>\n\n");
    return 0;
}

void app_process(app_p app)
{
    printf("app_process running......\n");
    int ret = 0;
    if(NULL == app){
        printf("app is NULL!!!\n");
    }
    uint8_t cmd = app->message.cmd;
    uint8_t *data = app->message.data;
    uint8_t data_len = strlen(data) + 1;
    int state = parse_cmd(cmd, data, &app->cmds);

    printf("start states check!!! datalen is %d\n", data_len);
    switch (state)
    {
    case CMD_CTRL_LED:
        state_led(app, cmd, data, data_len);
        break;

    case CMD_CTRL_UART:
        state_uart(app, cmd, data, data_len);
        break;

    case CMD_CTRL_INET:
        state_inet(app, cmd, data, data_len);
        break;

    case CMD_CTRL_SYSTEM:
        state_system(app); // 终止app线程以及其他一切线程的运行
        break;

    default:
        break;
    }
}



// 接受消息队列的消息，调用处理函数
void *app_process_thread_entry(void *argc)
{
    app_p app = (app_p)argc;
    if (NULL == app) {
        printf("app_thread app is NULL!!!|n");
    }

    while(app->app_flag) {
        printf("app_thread running......1\n");
        msg_recive(app->msg_id, &app->message);// 接受消息队列的信息
        printf("app_process start......\n");
        app_process(app);					   // 着手处理消息队列里的消息
        usleep(100);
    }
}