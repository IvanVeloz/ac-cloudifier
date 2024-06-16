#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include "gpio.h"
#include "infrared.h"

#define LEDSLEEP    500000

int main() {

    int r = 0;
    struct GPIO gpio;
    struct infra_st infra;

    r = GPIO_initialize(&gpio);
    assert(r >= 0);
    r = infrared_initialize(&infra);
    assert(r == 0);

    /* Test the LEDs */
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

    /* Test the IR transmission with the flood light on */
    infrared_send(&infra, infra_delay);
    sleep(1);
    infrared_send(&infra, infra_delay);

    /* Turn off infrared flood light */
    GPIO_set_InfraLED(&gpio, ir_off);

    /* Finalize */
    infrared_finalize(&infra);
    GPIO_finalize(&gpio);

	return 0;
}

