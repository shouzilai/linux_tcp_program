#ifndef __LED_H__
#define __LED_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define LED_STATE_ON        1
#define LED_STATE_OFF       0

#define LED_SPEED_FAST      500
#define LED_SPEED_SLOW      1000

#define LED_MODE_MERIT      1
#define LED_NUMBER_COUNT    2

typedef enum {
    EM_HL_LED_GREEN = 0x00,
    EM_HL_LED_RED,
} led_num_em;

typedef enum {
    LED_MDDE_SPARE = 0x0,
    LED_MODE_LIGHT, 
    LED_MODE_TRIGGER, 
} led_cmd_mode_em;


int led_set(int led_state);

int led_trigger(int speed);

#endif // __LED_H__