#ifndef __COMMON_H__

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <pthread.h>

#define DEBUG 1

#ifdef DEBUG
#define dbg_printf(fmt, args...) printf(fmt, ##args)
#define dbg_perror(msg) (perror(msg))
#else
#define dbg_printf(fmt, args...)
#define dbg_perror(msg)
#endif

#define MIN(x,y)  (((x)<(y))?(x):(y))
#define MAX(x,y)  (((x)>(y))?(x):(y))

#define EXIT_SUCCESS	0
#define EXIT_FAILURE	1


#define MAX_UART_NAME_LEN   255


#define CMD_CTRL_SYSTEM 0
#define CMD_CTRL_LED    1
#define CMD_CTRL_UART   2
#define CMD_CTRL_INET   3


#endif		// __COMMON_H__
