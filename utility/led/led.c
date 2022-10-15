#include "led.h"

int led_set(int led_state)
{
	int ret = 0;
	char cmdline[128];

	sprintf(cmdline, "echo %d > /sys/class/leds/myc\\:green\\:user1/brightness", led_state);
	ret = system(cmdline);
	return  WEXITSTATUS(ret);
}

int led_trigger(int speed)
{
  static int led_state = 0;
  int ret = 0;
  usleep(speed*1000);
  ret = led_set(led_state++);
  led_state %= 2;
  return ret;
}


