#ifndef __LED_APP_H__
#define __LED_APP_H__

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/time.h>
#include <signal.h>
#include <time.h>
#include "../utility/led/led.h"
#include "..//utility/hup/hup.h"
#include "../utility/fifo/ring_buffer_fifo.h"


typedef struct led_item_conf    // 具体 led灯 的属性结构体
{
    char *dev_name;             
    int sort;                   
    int mode;                   
    int state;                  
    uint16_t speed;             
    timer_t timer;              
    struct sigevent evp;        
    struct itimerspec ts;       
}led_item_conf_t, *led_item_conf_p;

typedef struct led_mng           // 管理开发板上 led灯 
{
    led_item_conf_t led_green;   
    led_item_conf_t led_red;
     
}led_mng_t, *led_mng_p;

typedef struct led {
    int handle_flag;                // 线程运行与否的标志
    int led_count;
    led_num_em leds_sort[LED_NUMBER_COUNT];
    pthread_t handle_td;            
    led_mng_p led_mng;

} led_t, *led_p;

typedef struct led_cmd {
    led_num_em led_num[LED_NUMBER_COUNT]; 
    led_cmd_mode_em led_mode[LED_NUMBER_COUNT];
    int ctrl_num;
    int val[LED_NUMBER_COUNT];

} led_cmd_t, *led_cmd_p;


void *app_led_thread_entry(void *argc);

int app_led_init(led_p led, int msg_id);         

int app_led_deinit(led_p led);                   

int app_led_start(led_p led);                    

int app_led_stop(led_p led);                     

int app_led_set(led_p led);   // 直接设置灯的亮灭情况

int app_led_conf(led_p led, led_num_em led_sort[], led_cmd_mode_em led_mode[], int conf_val[], int ctrl_num); // 启动定时器

int set_led_cmd(uint8_t cmd, led_cmd_p led_ptr);


#endif // __LED__APP_H__