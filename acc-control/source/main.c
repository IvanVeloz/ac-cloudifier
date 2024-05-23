#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include "gpio.h"

#define LEDSLEEP    500000

int main() {

    /* Test the LEDs */
    struct GPIO gpio;
    GPIO_initialize(&gpio);
    GPIO_set_InfraLED(&gpio, ir_on);
    usleep(LEDSLEEP);
    GPIO_set_StatusLED(&gpio, stat_red);
    usleep(LEDSLEEP);
    GPIO_set_StatusLED(&gpio, stat_blue);
    usleep(LEDSLEEP);
    GPIO_set_StatusLED(&gpio, stat_purple);
    usleep(LEDSLEEP);
    GPIO_set_StatusLED(&gpio, stat_off);
    usleep(LEDSLEEP);

    /* Test the IR transmission with the flood light on*/
    // TODO

    /* Turn off infrared flood light */
    GPIO_set_InfraLED(&gpio, stat_off);
    GPIO_finalize(&gpio);

	return 0;
}

