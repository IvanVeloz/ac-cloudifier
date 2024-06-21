#ifndef _CONTROL_H_
#define _CONTROL_H_

#include <stdbool.h>
#include <pthread.h>
#include "mqtt.h"
#include "accpanel.h"

struct control_st {
    struct mqtt_st * mqtt;
    struct machvis_st * mv;
    struct panel_st * desiredpanel;
    struct panel_st * actualpanel;
    bool publish;
    bool listen;
    bool loop;
    pthread_t control_publish_thread;
    pthread_t control_listen_thread;
    pthread_t control_loop_thread;
};

int control_initialize(
    struct control_st * control, 
    struct mqtt_st * mqtt, 
    struct machvis_st * mv);
int control_finalize(struct control_st * control);
int control_publish_start(struct control_st * control);
int control_listen_start(struct control_st * control);
void *control_publish(void *args);
void *control_listen(void *args);
void *control_loop(void *args);

#endif /* #ifndef _CONTROL_H_ */