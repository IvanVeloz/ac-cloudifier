#include <stdlib.h>
#include <errno.h>
#include "mqtt.h"
#include "machvis.h"
#include "control.h"

int control_initialize(
    struct control_st * control, 
    struct mqtt_st * mqtt, 
    struct machvis_st * mv)
{
    int r;
    control->mqtt = mqtt;
    control->mv = mv;
    control->desiredpanel = NULL;
    control->actualpanel = NULL;
    return 0;
}
int control_finalize(struct control_st * control)
{
    mqtt_disconnect(control->mqtt);
    machvis_close(control->mv);
    return 0;
}

int control_publish_start(struct control_st * control)
{
    return pthread_create(
        control->control_publish_thread, NULL, control_publish, control);
}
void *control_publish(void *args)
{
    int r;
    struct control_st * control = args;
    if(!control) return NULL;

    control->publish = true;
    do {
        pthread_testcancel();
        r = mqtt_publish_panel_state(control->mqtt, control->mv);
        if(r == -EAGAIN) usleep(300000);    // don't DDOS the poor broker...
    } while(control->publish);
    return NULL;
}
void *control_listen(void * args)
{
    int r;
    struct control_st * control = args;
    if(!control) return NULL;

    control->listen = true;
    do {
        pthread_testcancel();
        // do the listening
    } while(control->listen);
    return NULL;
}

void *control_loop(void * args)
{
    int r;
    struct control_st * control = args;
    if(!control) return NULL;

    control->loop = true;
    do {
        pthread_testcancel();
        // do the control loop
    } while(control->loop);
    return NULL;
}
