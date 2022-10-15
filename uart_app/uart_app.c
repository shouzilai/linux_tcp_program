#include "uart_app.h"

// 消息队列描述符
static int msg_queue_id;

void hup_success_handle_func(hup_protocol_type_p hup_frame)
{
    int ret = 0;
    if (NULL == hup_frame) {
        printf("commit message failure!!!\n");
        return ;
    }
    printf("hup_cmd: %d, hup_data_len: %d\n", hup_frame->cmd, hup_frame->data_len);
    printf("msg_queue_id:%d, hup_data:%s\n\n", msg_queue_id, hup_frame->data_addr);
    ret = msg_send(msg_queue_id, hup_frame->cmd, hup_frame->data_addr, hup_frame->data_len);// 直接向id发送指令和数据
    if (ret) {
        printf("[ERROR][UART]msg send\n");
    }
}

// 设备fd、波特率、数据大小、flow control、奇偶校验位、停止位
static int tty_setting(int fd, int bitrate, int datasize, int flow, int par, int stop)
{
    struct termios newtio;
    // struct termios
    // {
    // 	tcflag_t c_iflag;		/* input mode flags */
    // 	tcflag_t c_oflag;		/* output mode flags */
    // 	tcflag_t c_cflag;		/* control mode flags */
    // 	tcflag_t c_lflag;		/* local mode flags */
    // 	cc_t c_line;			/* line discipline */
    // 	cc_t c_cc[NCCS];		/* control characters */
    // 	speed_t c_ispeed;		/* input speed 波特率*/
    // 	speed_t c_ospeed;		/* output speed 波特率*/
    // #define _HAVE_STRUCT_TERMIOS_C_ISPEED 1
    // #define _HAVE_STRUCT_TERMIOS_C_OSPEED 1
    // };

    /* ignore modem control lines and enable receiver */
    memset(&newtio, 0, sizeof(newtio));
    newtio.c_cflag = newtio.c_cflag |= CLOCAL | CREAD;  // 忽略调制解调器线路状态 | 使用接收器
    if(flow == 1)
    {
        newtio.c_cflag = newtio.c_cflag |= CLOCAL | CREAD | CRTSCTS; // 忽略调制解调器线路状态 | 使用接收器 | 使用RTS/CTS流控制
    }

    newtio.c_cflag &= ~CSIZE; // 字符长度，取值范围为CS5、CS6、CS7或CS8

    /* set character size */
    switch (datasize) {
        case 8:
            newtio.c_cflag |= CS8;
            break;
        case 7:
            newtio.c_cflag |= CS7;
            break;
        case 6:
            newtio.c_cflag |= CS6;
            break;
        case 5:
            newtio.c_cflag |= CS5;
            break;
        default:
            newtio.c_cflag |= CS8;
            break;
    }

    /* set the parity */
    switch (par) {
        case 'o':
        case 'O':
            newtio.c_cflag |= PARENB;
            newtio.c_cflag |= PARODD;
            newtio.c_iflag |= (INPCK | ISTRIP);
            break;
        case 'e':
        case 'E':
            newtio.c_cflag |= PARENB;
            newtio.c_cflag &= ~PARODD;
            newtio.c_iflag |= (INPCK | ISTRIP);
            break;
        case 'n':
        case 'N':
            newtio.c_cflag &= ~PARENB;
            break;
        default:
            newtio.c_cflag &= ~PARENB;
            break;
    }

    /* set the stop bits */
    switch (stop) {
        case 1:
            newtio.c_cflag &= ~CSTOPB; // 不设置两个停止位
            break;
        case 2:
            newtio.c_cflag |= CSTOPB;  // 设置两个停止位
            break;
        default:
            newtio.c_cflag &= ~CSTOPB;
            break;
    }

    /* set output and input baud rate */
    switch (bitrate) {
        case 0:
            cfsetospeed(&newtio, B0);
            cfsetispeed(&newtio, B0);
            break;
        case 50:
            cfsetospeed(&newtio, B50);
            cfsetispeed(&newtio, B50);
            break;
        case 75:
            cfsetospeed(&newtio, B75);
            cfsetispeed(&newtio, B75);
            break;
        case 110:
            cfsetospeed(&newtio, B110);
            cfsetispeed(&newtio, B110);
            break;
        case 134:
            cfsetospeed(&newtio, B134);
            cfsetispeed(&newtio, B134);
            break;
        case 150:
            cfsetospeed(&newtio, B150);
            cfsetispeed(&newtio, B150);
            break;
        case 200:
            cfsetospeed(&newtio, B200);
            cfsetispeed(&newtio, B200);
            break;
        case 300:
            cfsetospeed(&newtio, B300);
            cfsetispeed(&newtio, B300);
            break;
        case 600:
            cfsetospeed(&newtio, B600);
            cfsetispeed(&newtio, B600);
            break;
        case 1200:
            cfsetospeed(&newtio, B1200);
            cfsetispeed(&newtio, B1200);
            break;
        case 1800:
            cfsetospeed(&newtio, B1800);
            cfsetispeed(&newtio, B1800);
            break;
        case 2400:
            cfsetospeed(&newtio, B2400);
            cfsetispeed(&newtio, B2400);
            break;
        case 4800:
            cfsetospeed(&newtio, B4800);
            cfsetispeed(&newtio, B4800);
            break;
        case 9600:
            cfsetospeed(&newtio, B9600);
            cfsetispeed(&newtio, B9600);
            break;
        case 19200:
            cfsetospeed(&newtio, B19200);
            cfsetispeed(&newtio, B19200);
            break;
        case 38400:
            cfsetospeed(&newtio, B38400);
            cfsetispeed(&newtio, B38400);
            break;
        case 57600:
            cfsetospeed(&newtio, B57600);
            cfsetispeed(&newtio, B57600);
            break;
        case 115200:
            cfsetospeed(&newtio, B115200);
            cfsetispeed(&newtio, B115200);
            break;
        case 230400:
            cfsetospeed(&newtio, B230400);
            cfsetispeed(&newtio, B230400);
            break;
        default:
            cfsetospeed(&newtio, B9600);
            cfsetispeed(&newtio, B9600);
            break;
    }

    /* set timeout in deciseconds for non-canonical read 设置非规范读取的超时（以分秒为单位）*/
    newtio.c_cc[VTIME] = 0;
    /* set minimum number of characters for non-canonical read */
    newtio.c_cc[VMIN] = 1;

    /* flushes data received but not read 刷新已接收但未读取的数据
    丢弃要写入引用的对象，但是尚未传输的数据，或者收到但是尚未读取的数据，取决于queue_selector 的值：
        TCIFLUSH：刷新收到的数据但是不读  
        TCOFLUSH ：刷新写入的数据但是不传送  
        TCIOFLUSH ：同时刷新收到的数据但是不读，并且刷新写入的数据但是不传送 
    */
    tcflush(fd, TCIFLUSH);
    /* 
       set the parameters associated with the terminal from
       the termios structure and the change occurs immediately 
       从中设置与终端相关的参数
       termios结构和变化立即发生
    */
    if((tcsetattr(fd, TCSANOW, &newtio))!=0) {
        dbg_perror("set_tty/tcsetattr");
        return -1;
    }
    /*
    原型：inttcsetattr(int fd,int actions,const struct    termios*termios_p);
    功能：设置与终端相关的参数 (除非需要底层支持却无法满足)，使用termios_p 引用的termios 结构。
    optional_actions（tcsetattr函数的第二个参数）指定了什么时候改变会起作用： 
        TCSANOW：改变立即发生  
        TCSADRAIN：改变在所有写入fd 的输出都被传输后生效。这个函数应当用于修改影响输出的参数时使用。(当前输出完成时将值改变)  
        TCSAFLUSH ：改变在所有写入fd 引用的对象的输出都被传输后生效，所有已接受但未读入的输入都在改变发生前丢弃(同TCSADRAIN，但会舍弃当前所有值)。 
    */

    //tty_mode(fd, mode);
    return 0;

}

static int tty_read(int fd, char *frame)
{
    struct timeval timeout;
    fd_set fds;
    int ret;
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;

    FD_ZERO(&fds);
    FD_SET(fd, &fds);

    ret = select(fd+1, &fds, NULL, NULL, NULL); // 阻塞式的监听，文件的读、写事件
    if (ret < 0) {
        dbg_perror("select tty");
    }
    if (FD_ISSET(fd, &fds)) {
        ret = read(fd, frame, TTY_READ_BUFFER_SIZE); // 从frame中读取256个字节的数据，返回实际读到的字节数
    }

    return ret;
}

static int tty_write(int fd, char *frame, int len)
{
    int ret = -1;
    ret = write(fd, frame, len); // 向fd中写入len长度的数据，返回实际写入的字节数
    if (ret < 0) {
        dbg_perror("tty write!");
    }
    return ret;
}

int tty_init(tty_p tty_pointer, int msg_id)
{
    int fd, ret;
    msg_queue_id = msg_id;
    // 打开设备的结点文件
    fd = open(TTY_DEFAULT_DEVICE, O_RDWR); 
    if (fd < 0) {
        dbg_perror("open tty");
    }
    tty_pointer->tty_fd = fd;

    // 设置串口的参数配置
    ret = tty_setting(fd, TTY_DEFAULT_BAUDRATE, TTY_DEFAULT_DATA, TTY_DEFAULT_FLOW,
             TTY_DEFAULT_PAR, TTY_DEFAULT_STOP);
    if (ret < 0) {
        printf("[ERROR] uart argc setting failure!!!\n");
        return -1;
    }

    // 初始化两个fifo
    ret = buffer_fifo_init(&tty_pointer->r_fifo, BUFFER_R_FIFO);
    ret += buffer_fifo_init(&tty_pointer->w_fifo, BUFFER_W_FIFO);
    if (ret >0) {
        printf("[ERROR] uart fifo init failure!!!\n");
        return -1;
    }

    // 初始化hup
    hup_init(&tty_pointer->hup_frame, TTY_FRAME_PAYLOAD_SIZE, hup_success_handle_func);

    //  初始化串口数据的暂存区
    tty_pointer->recive_buff = (uint8_t*)malloc(sizeof(int)*TTY_RECIVE_BUFFER_SIZE);
    tty_pointer->send_buff = (int*)malloc(sizeof(int)*TTY_SEND_BUFFER_SIZE);
    if (tty_pointer->recive_buff == NULL | tty_pointer->send_buff == NULL) {
        printf("[ERROR] uart buffer allocated failure!!!\n");
        return -1;
    }

    printf("tty_init\n");
    return 0;
}

int tty_deinit(tty_p tty_pointer)
{
    if(NULL == tty_pointer){
        return -1;
    }
    close(tty_pointer->tty_fd);

    buffer_fifo_deinit(&tty_pointer->r_fifo);
    buffer_fifo_deinit(&tty_pointer->w_fifo);

    hup_deinit(&tty_pointer->hup_frame);

    free(tty_pointer->recive_buff);
    free(tty_pointer->send_buff);

    return 0;
}

int tty_start(tty_p uart)
{
    int ret = -1;
    // pthread_t recive_td = -1, handle_td = -1, send_td = -1;
    
    ret = pthread_create(&uart->recive_td, NULL, &recive_thread_fun, uart);
    if (ret == -1) {
        printf("[ERROR][UART] recive thread create failure!!!\n");
    } else {
        printf("[OK][UART] recive thread create success!!!\n");
    }
    
    
    ret = pthread_create(&uart->handle_td, NULL, &handle_thread_fun, uart);
    if (ret == -1) {
        printf("[ERROR][UART] handle thread create failure!!!\n");
    } else {
        printf("[OK][UART] handle thread create success!!!\n");
    }
    
    
    ret = pthread_create(&uart->send_td, NULL, &send_thread_fun, uart);
    if (ret == -1) {
        printf("[ERROR][UART] send thread create failure!!!\n");
    } else {
        printf("[OK][UART] send thread create success!!!\n");
    }
    uart->recive_flag = 1;
    uart->handle_flag = 1;
    uart->send_flag = 1;
    printf("tty_start\n");
    return 0;
}

int tty_stop(tty_p uart)
{
    if (NULL == uart) {
        perror("[ERROR] app ptr is NULL\n");
        return -1;
    }
    uart->recive_flag = 0;
    uart->handle_flag = 0;
    uart->send_flag = 0;

    return 0;
}

int tty_send(tty_p uart, hup_protocol_type_em sort, int cmd, uint8_t* addr, int addr_len)
{
    
    if (NULL == uart) {
        printf("tty send uart is NULL!!!\n");
        return -1;
    }
    if (addr_len < 0 || NULL == addr) {
        printf("tty send addr_len or addr illegal!!!\n");
        return -1;
    }
    uint8_t frame[TTY_SEND_BUFFER_SIZE];
    int frame_len = -1;

    hup_protocol_type_p hup = hup_pack(sort, cmd, addr_len, addr);
    frame_len = hup->data_len + 5;
    memcpy(frame, &hup ,frame_len);

    buffer_fifo_write(&uart->w_fifo, frame, frame_len);

    return 0;
}

int set_uart_cmd(uint8_t cmd, uart_cmd_p uart_ptr)
{
    printf("set uart cmd success!!!\n");
}



void* recive_thread_fun(void *argc)
{
    int ret = 0;
    tty_p tty_de = (tty_p)argc;
    
    while(tty_de->recive_flag)
    {
        memset(tty_de->recive_buff, 0, 512);
        // printf("tty_read blocking......\n");
        ret = tty_read(tty_de->tty_fd, (char*)tty_de->recive_buff);
        printf("read uart data number is %d \n",ret);
        // ret = read(tty_de->fd, frame, 128);
        if (ret > 0) {
            // printf("write r_fifo data is %s \n", (char*)tty_de->recive_buff);
            buffer_fifo_write(&tty_de->r_fifo, tty_de->recive_buff, ret);
            printf("[OK] recive uart and write data r_fifo success !!!\n\n");
            
        } else {
            printf("[ERROR] recive uart and write data r_fifo failure !!!\n\n");
        }
    }
}

void* handle_thread_fun(void *argc)
{
    int  ret = -1;
    int parse_ret = -1;
    tty_p tty_de = (tty_p)argc;
    uint8_t temp;

    while(tty_de->handle_flag)
    {
        ret = buffer_fifo_read(&tty_de->r_fifo, &temp, 1);

        if (ret == 0) {
            printf("get data from r_fifo , gonna to handle......\n");
            parse_ret = hup_parse(&tty_de->hup_frame, temp); // AADD01044f50454e66
            
            // if (parse_ret == 10) {
            //     printf("hup_pack start......\n");
            //     hup_protocol_type_p hup = hup_pack(EM_HUP_TYPE_ACK, &tty_de->hup_frame);
            // 
            //     if (NULL == hup) {
            //         printf("hup_pack failure!!!\n\n");
            //     }
            //     buffer_fifo_write(&tty_de->w_fifo, (int*)hup, hup->data_len + 5);
            //     printf("[OK] parse data and write w_fifo success !!!\n\n");
            // }

        } else {
            usleep(100);
            // printf("[ERROR] parse data and write w_fifo failure !!!\n\n");
        }
    }
}

void* send_thread_fun(void *argc)
{
    uint8_t data;
    int ret;
    tty_p tty_de = (tty_p)argc;
    while(tty_de->send_flag)
    {
        ret = buffer_fifo_read(&tty_de->w_fifo, &data, 1);
        
        if (ret == 0) {
            printf("read w_fifo data, try to send hup to uart\n");
            tty_write(tty_de->tty_fd, (char*)&data, sizeof(int));
            printf("[OK] read w_fifo and send data to uart success !!!\n\n\n");
        } else {
                usleep(100);
                // printf("[ERROR] read w_fifo and send null data to uart failure !!!\n\n\n");
        }
        // printf("w_fifo data is NULL! \n");
    }
}

