#include <signal.h>
#include <time.h>
#include <sys/time.h>
#include "led_app.h"
#include "../common.h"

static int msg_queue_id;

// LED参数点式初始化
led_mng_t led_mng = {
    .led_green = {
        .dev_name = "/sys/class/leds/myc\\:green\\:user1/brightness\0",
        .sort = EM_HL_LED_GREEN,
        .mode = LED_MODE_LIGHT,
        .state = LED_STATE_ON,
        .speed = LED_SPEED_SLOW,
    },
    .led_red = {
        .dev_name = "/sys/class/leds/myd\\:green\\:user2/brightness\0", 
        .sort = EM_HL_LED_RED, 
        .mode = LED_MODE_LIGHT, 
        .state = LED_STATE_ON,
        .speed = LED_SPEED_SLOW, // 500
    }
};

void led_timer_timeout_handle_func(union sigval v)
// union sigval
// {
//   int sival_int;
//   void *sival_ptr;
// };
{
    int *state;
    switch (v.sival_int)
    {
    case EM_HL_LED_GREEN:
        state = &led_mng.led_green.state;
        *state =  (*state == LED_STATE_OFF)? LED_STATE_ON : LED_STATE_OFF;

    case EM_HL_LED_RED:
        state = &led_mng.led_red.state;
        *state =  (*state == LED_STATE_OFF)? LED_STATE_ON : LED_STATE_OFF;
        break;

    default:
        break;
    }
}

static void led_timer_perset()
{
    // 定时器配置
    memset(&led_mng.led_green.evp, 0, sizeof(led_mng.led_green.evp));
    memset(&led_mng.led_red.evp, 0, sizeof(led_mng.led_red.evp));

    led_mng.led_green.evp.sigev_value.sival_ptr = &led_mng.led_green.timer;
    led_mng.led_green.evp.sigev_value.sival_int = led_mng.led_green.sort;
    led_mng.led_green.evp.sigev_notify = SIGEV_THREAD; // 定时器到期时 将启动新的线程进行需要的处理  2	deliver via thread creation
    led_mng.led_green.evp.sigev_notify_function = led_timer_timeout_handle_func;

    led_mng.led_red.evp.sigev_value.sival_ptr = &led_mng.led_red.timer;
    led_mng.led_red.evp.sigev_value.sival_int = led_mng.led_red.sort;
    led_mng.led_red.evp.sigev_notify = SIGEV_THREAD;
    led_mng.led_red.evp.sigev_notify_function = led_timer_timeout_handle_func;

    return;
}

static void led_timer_postset()
{
    // 设置定时器间隔  一个红灯，一个绿灯，同样的参数设置
    led_mng.led_green.ts.it_interval.tv_sec = 0;
    led_mng.led_green.ts.it_interval.tv_nsec = 0;
    led_mng.led_green.ts.it_value.tv_sec = 0;
    led_mng.led_green.ts.it_value.tv_nsec = 0;

    led_mng.led_red.ts.it_interval.tv_sec = 0;
    led_mng.led_red.ts.it_interval.tv_nsec = 0;
    led_mng.led_red.ts.it_value.tv_sec = 0;
    led_mng.led_red.ts.it_value.tv_nsec = 0;

    return;
}

int app_led_init(led_p led, int msg_id)
{
    if (NULL == led)
    {
        perror("[ERROR]led ptr is NULL\n");
        return -1;
    }
    int ret = 0;
    msg_queue_id = msg_id; // 为静态变量消息队列id 赋值

    led_timer_perset();
    ret = timer_create(CLOCK_REALTIME, &led_mng.led_green.evp, &led_mng.led_green.timer); // 系统保存的时间，可用date命令显示
    if (ret) {
        perror("[LED][ERROR]timer create\n");
    }
    ret = timer_create(CLOCK_REALTIME, &led_mng.led_red.evp, &led_mng.led_red.timer);
    if (ret) {
        perror("[LED][ERROR]timer create\n");
    }
    led_timer_postset();

    led->led_mng = &led_mng; // 传出参数，将配置好的参数传回给函数参数
    led->led_count = LED_NUMBER_COUNT;
    led->handle_td = 0;      //
    printf("red_timer id is: %p, green_timer id is: %p\n", led->led_mng->led_red.timer, led->led_mng->led_green.timer);
    printf("led_red addr is %p, led_green addr is %p\n", &led->led_mng->led_red, &led->led_mng->led_green);
    printf("app_led_init\n");
    return 0;
}

int app_led_deinit(led_p led)
{
    if (NULL == led) {
        perror("[ERROR]led ptr is NULL\n");
        return -1;
    }

    int ret = 0;

    // 注销定时器
    ret = timer_delete(led_mng.led_green.timer);
    if (ret) {
        perror("[ERROR]timer detele\n");
    }

    ret = timer_delete(led_mng.led_red.timer);
    if (ret) {
        perror("[ERROR]timer detele\n");
    }

    return 0;
}

int app_led_start(led_p led)
{
    if (NULL == led) {
        perror("[ERROR]led ptr is NULL\n");
        return -1;
    }
    int ret = 0;
    led->handle_flag = 1;

    // 线程创建：处理LED灯状态
    ret = pthread_create(&led->handle_td, NULL, &app_led_thread_entry, (void *)led);
    if (ret) {
        printf("[ERROR][LED] create handle_thread\n");
        return -1;
    } else {
        printf("[OK][LED] create handle_thread success!\n");
    }
    printf("app_led_start\n");
    return 0;
}

int app_led_stop(led_p led)
{
    if (NULL == led) {
        perror("[ERROR]led ptr is null\n");
        return -1;
    }

    int ret = 0;
    led->handle_flag = 0;
    // ret = pthread_cancel(led->handle_thread_tid);
    // if(ret) {
    //     printf("[ERROR][LED] cancel handle_pthread\n");
    //     return -1;
    // }

    return 0;
}

int app_led_set(led_p led)
{
    if (NULL == led) {
        printf("[ERROR] led ptr is null\n");
        return -1;
    }

    int ret = 0;
    char cmdline[128];
    for (int i = 0; i < led->led_count; i++) {
        memset(cmdline, 0, sizeof(cmdline));
        led_num_em status = led->leds_sort[i];
        switch (status)
        {
        case EM_HL_LED_GREEN:
            sprintf(cmdline, "echo %d > %s", led->led_mng->led_green.state, led->led_mng->led_green.dev_name);
            break;

        case EM_HL_LED_RED:
            sprintf(cmdline, "echo %d > %s", led->led_mng->led_red.state, led->led_mng->led_red.dev_name);
            break;

        default:
            break;
        }
        ret = system(cmdline);
    }

    return WEXITSTATUS(ret);
}

static void led_set_mode(led_p led, led_item_conf_p ptr_led, int val)
{
    int state_val = val;
    int speed_val = val * LED_MODE_MERIT;
    switch (ptr_led->mode)
    {
    case LED_MODE_LIGHT:
        ptr_led->state = state_val;

        ptr_led->ts.it_interval.tv_sec = 0;
        ptr_led->ts.it_interval.tv_nsec = 0;
        ptr_led->ts.it_value.tv_sec = 0;
        ptr_led->ts.it_value.tv_nsec = 0;
        printf("light light light\n\n");
        break;

    case LED_MODE_TRIGGER:
        ptr_led->speed = speed_val;
        
        ptr_led->ts.it_interval.tv_sec = speed_val;
        ptr_led->ts.it_interval.tv_nsec = 0;
        ptr_led->ts.it_value.tv_sec = speed_val;
        ptr_led->ts.it_value.tv_nsec = 0;
        printf("trigger trigger trigger\n\n");
        break;

    default:
        break;
    }
}

static void led_set_num(led_p led, led_item_conf_p *ptr_led, led_num_em leds_sort)
{
    switch (leds_sort)
    {
    case EM_HL_LED_GREEN:
        *ptr_led = &(led->led_mng->led_green);
        break;
    case EM_HL_LED_RED:
        *ptr_led = &(led->led_mng->led_red);
        break;
    default:
        break;
    }
}

int app_led_conf(led_p led, led_num_em leds_sort[], led_cmd_mode_em led_mode[], int conf_val[], int ctrl_num)
{
    printf("led_cmd content led_sort[0]:%d, led_mode[0]:%d, led_val[0]:%d, led_num:%d\n",
            leds_sort[0], led_mode[0], conf_val[0], ctrl_num);
    if (NULL == led) {
        perror("[ERROR]led ptr is null\n");
        return -1;
    }
    int ret = 0;
    led_item_conf_p ptr_leds;
    
    for (int i = 0; i < ctrl_num; i++) {
        printf("stock ptr addr is %p, ctrl_num is %d\n", ptr_leds, ctrl_num);
        if (led_mode[i] == LED_MDDE_SPARE) {
            continue;
        }

        led_set_num(led, &ptr_leds, leds_sort[i]); // 赋值指针
        ptr_leds->mode = led_mode[i];
        led_set_mode(led, ptr_leds, conf_val[i]);  // 设置LED定时器参数
        printf("change ptr addr is %p\n", ptr_leds);

        // 开启定时器
        printf("the red_led timer id is:%p, the green_led timer id is:%p\n", led->led_mng->led_red.timer, led->led_mng->led_green.timer);
        printf("led timer id is: %p, interval is: %ld\n", ptr_leds->timer, ptr_leds->ts.it_interval.tv_nsec);
        ret = timer_settime(ptr_leds->timer, TIMER_ABSTIME, &ptr_leds->ts, NULL);
        if (ret) {
            perror("timer_settime");
        }
    }
}

int set_led_cmd(uint8_t cmd, led_cmd_p led_p)
{
    printf("set_led_cmd cmd is %d\n", cmd);
    switch (cmd)
    {
    case 0x01:
        printf("led cmd 1 success\n\n");
        led_p->led_num[0] = EM_HL_LED_GREEN;
        led_p->led_mode[0] = LED_MODE_LIGHT;
        led_p->ctrl_num = 1;
        led_p->val[0] = 0;
        return 1;
        break;
    
    case 0x02:
        printf("led cmd 2 success\n");
        led_p->led_num[0] = EM_HL_LED_GREEN;
        led_p->led_mode[0] = LED_MODE_LIGHT;
        led_p->ctrl_num = 1;
        led_p->val[0] = 1;
        return 1;
        break;

    case 0x03:
        led_p->led_num[0] = EM_HL_LED_GREEN;
        led_p->led_mode[0] = LED_MODE_TRIGGER;
        led_p->ctrl_num = 1;
        led_p->val[0] = 3;
        return 1;
        break;

    case 0x04:
        led_p->led_num[0] = EM_HL_LED_GREEN;
        led_p->led_mode[0] = LED_MODE_TRIGGER;
        led_p->ctrl_num = 1;
        led_p->val[0] = 1;
        return 1;
        break;

    default:
        return 0;
        break;
    }
}



void *app_led_thread_entry(void *argc)
{
	led_p led = (led_p)argc;

    while (led->handle_flag)
    {
        app_led_set(led);
        usleep(100);
    }
}
