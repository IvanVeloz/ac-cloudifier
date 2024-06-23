#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>
#include "gpio.h"
#include "infrared.h"
#include "mqtt.h"
#include "machvis.h"
#include "control.h"

#define LEDSLEEP    500000

struct mainthreads_st {
    pthread_t machvis;
    pthread_t control_publish;
    pthread_t control_listen;
    pthread_t control_loop;
};

int main() {

    int r = 0;
    struct GPIO gpio;
    struct infra_st infra;
    struct mqtt_st mqtt;
    struct machvis_st mv;
    struct mainthreads_st mt;
    struct control_st control;

    r = GPIO_initialize(&gpio);
    assert(r >= 0);

    r = infrared_initialize(&infra);
    assert(r == 0);

    r = machvis_initialize(&mv);
    assert(r == 0);
    pthread_create(&mt.machvis, NULL, machvis_receive, &mv);

    r = mqtt_initialize(&mqtt, &mv);
    assert(r == 0);
    r = mqtt_publish_unit_ping(&mqtt);
    assert(r == 0);

    r = control_initialize(&control, &mqtt, &infra, &mv);
    assert(r == 0);
    pthread_create(&mt.control_publish, NULL, control_publish, &control);
    pthread_create(&mt.control_listen, NULL, control_listen, &control);
    pthread_create(&mt.control_loop, NULL, control_loop, &control);

    // This pause could be a loop that watches over the threads instead
    pause();
    mqtt.publish = false;
    mv.receive = false;
    control.listen = false;
    control.publish = false;

    pthread_join(mt.control_loop, NULL);
    pthread_join(mt.control_listen, NULL);
    pthread_join(mt.control_publish, NULL);
    pthread_join(mt.machvis, NULL);

    r = control_finalize(&control);
    assert (r == 0);

    r = mqtt_disconnect(&mqtt);
    assert(r == 0);
    r = mqtt_finalize(&mqtt);
    assert(r == 0);
    
    r = machvis_close(&mv);
    assert(r == 0 || errno == EBADF);
    r = machvis_finalize(&mv);
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

