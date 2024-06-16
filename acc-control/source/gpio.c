#include <string.h>
#include <syslog.h>
#ifndef _DESKTOP_BUILD_
#include <pigpiod_if2.h>
#else
#include <stdio.h>
#endif
#include "gpio.h"

int GPIO_initialize(struct GPIO *gpio)
{
    gpio = memset(gpio, 0, sizeof(*gpio));
    
    #ifndef _DESKTOP_BUILD_
    gpio->_fd = pigpio_start(NULL, NULL);
    #else
    gpio->_fd = 1;
    #endif

    if(gpio->_fd < 0) {
        syslog(LOG_ERR, "Failed to open pigpio interface.");
    }
    return gpio->_fd;
}

int GPIO_finalize(struct GPIO *gpio)
{
    #ifndef _DESKTOP_BUILD_
    pigpio_stop(gpio->_fd);
    #endif
    gpio = memset(gpio, -1, sizeof(*gpio));
    return 0;
}

int GPIO_set_StatusLED(struct GPIO *gpio, enum StatusLed color)
{
    int r = 0;

    #ifdef _DESKTOP_BUILD_
    printf("Status LED: %i\n", color);
    #else
    int gpioret[2] = {0,0};
    switch(color) {
        default:
        case stat_off:
            gpioret[0] = 
                gpio_write(gpio->_fd, _GPIO_H_LED_RED, _GPIO_H_LED_RED_OFF);
            gpioret[1] = 
                gpio_write(gpio->_fd, _GPIO_H_LED_BLUE, _GPIO_H_LED_BLUE_OFF);
            break;
        case stat_red:
            gpioret[0] = 
                gpio_write(gpio->_fd, _GPIO_H_LED_RED, _GPIO_H_LED_RED_ON);
            gpioret[1] = 
                gpio_write(gpio->_fd, _GPIO_H_LED_BLUE, _GPIO_H_LED_BLUE_OFF);
            break;
        case stat_blue:
            gpioret[0] = 
                gpio_write(gpio->_fd, _GPIO_H_LED_RED, _GPIO_H_LED_RED_OFF);
            gpioret[1] = 
                gpio_write(gpio->_fd, _GPIO_H_LED_BLUE, _GPIO_H_LED_BLUE_ON);
            break;
        case stat_purple:
            gpioret[0] = 
                gpio_write(gpio->_fd, _GPIO_H_LED_BLUE, _GPIO_H_LED_BLUE_ON);
            gpioret[1] = 
                gpio_write(gpio->_fd, _GPIO_H_LED_RED, _GPIO_H_LED_RED_ON);
            break;
    }
    for(uint8_t i=0; i < sizeof(gpioret)/sizeof(gpioret[0]); i++) {
        if(gpioret[i] != 0) {
            r = gpioret[i];
            syslog(LOG_ERR, "Got %i trying to gpio_write the status LED", r);
        }
        break;
    }

    #endif /* ifdef _DESKTOP_BUILD_ */
    return r;
}

int GPIO_get_StatusLED(struct GPIO *gpio)
{
    syslog(LOG_ERR, "GPIO_get_StatusLED not implemented");
    return -1;
}

int GPIO_set_InfraLED(struct GPIO *gpio, enum InfraLed state)
{
    int r = 0;
    int gpioret = 0;

    #ifdef _DESKTOP_BUILD_
    printf("Infrared LED: %i\n", state);
    
    #else
    switch(state) {
        default:
        case ir_off:
            gpioret = gpio_write(gpio->_fd, _GPIO_H_LED_IR, _GPIO_H_LED_IR_OFF);
            break;
        case ir_on:
            gpioret = gpio_write(gpio->_fd, _GPIO_H_LED_IR, _GPIO_H_LED_IR_ON);
            break;
    }
    if(gpioret !=0) {
        r = gpioret;
        syslog(LOG_ERR, "Got %i trying to gpio_write the infrared LED",r);
    }

    #endif /* ifdef _DESKTOP_BUILD_ */
    return r;
}

int GPIO_get_InfraLED(struct GPIO *gpio)
{
    syslog(LOG_ERR, "GPIO_get_InfraLED not implemented");
    return -1;
}

