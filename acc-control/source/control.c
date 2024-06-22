#include <string.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>
#include <errno.h>
#include <syslog.h>
#include "accpanel.h"
#include "mqtt.h"
#include "machvis.h"
#include "control.h"

struct buttonclick_st {
    int power;
    int fan;
    int mode;
    int delay;
    int plus;
    int minus;
} clicks;

int control_buttonclick_snprint(char *str, size_t n, struct buttonclick_st * b);

int control_getclicks(
    struct buttonclick_st * clicks,
    struct panel_st * desired, 
    struct panel_st * actual);
int control_sendclicks(struct buttonclick_st * clicks);


int control_initialize(
    struct control_st * control, 
    struct mqtt_st * mqtt, 
    struct machvis_st * mv)
{
    int r;

    control->desiredpanel = malloc(sizeof(struct panel_st));
    if(!control->desiredpanel) return -errno;
    control->actualpanel = malloc(sizeof(struct panel_st));
    if(!control->actualpanel) return -errno;

    control->mqtt = mqtt;
    control->mv = mv;
    *control->desiredpanel = (struct panel_st)PANEL_INITIALIZER;
    *control->actualpanel = (struct panel_st)PANEL_INITIALIZER;
    
    machvis_machvispanel_set(mv, control->actualpanel);

    return 0;
}
int control_finalize(struct control_st * control)
{
    free(control->desiredpanel);
    free(control->actualpanel);
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
        if(r == -EAGAIN) {
            usleep(300000);    // don't DDOS the poor broker...
            continue;
        }
    } while(control->publish);
    return NULL;
}
void *control_listen(void * args)
{   
    int r;
    struct control_st * control = args;
    char * cmd = NULL;
    struct panel_st panel = PANEL_INITIALIZER;

    if(!control) return NULL;

    control->listen = true;
    do {
        pthread_testcancel();
        // do the listening
        // when a new publication is received, 
        // control->desiredpaneltouched = false
        
        cmd = mqtt_listen_command(control->mqtt);

        r = accpanel_parse(&panel, cmd);
        if(r) {
            syslog(LOG_NOTICE, "Failed to parse MQTT command");
            continue;
        }
        
        panel.consumed = false;
        accpanel_cpy(control->desiredpanel, &panel);
        printf("Received a command!\n");

    } while(control->listen);
    return NULL;
}

void *control_loop(void * args)
{
    int r;
    struct control_st * control = args;
    struct buttonclick_st clicks;

    if(!control) return NULL;

    control->loop = true;
    do {

        pthread_testcancel();

        pthread_mutex_lock(&control->desiredpanel->mutex);
        if(control->desiredpanel->consumed) {
            pthread_mutex_unlock(&control->desiredpanel->mutex);
            continue;
        }

        pthread_mutex_lock(&control->actualpanel->mutex);
        if(control->actualpanel->consumed) {
            pthread_mutex_unlock(&control->actualpanel->mutex);
            pthread_mutex_unlock(&control->desiredpanel->mutex);
            continue;
        }
        
        r = control_getclicks( 
            &clicks,
            control->desiredpanel, 
            control->actualpanel);
        
        if(r >= 0) {
            control->desiredpanel->consumed = true;
            control->actualpanel->consumed = true;
        }

        pthread_mutex_unlock(&control->actualpanel->mutex);
        pthread_mutex_unlock(&control->desiredpanel->mutex);

        if(r >= 0) {
            control_sendclicks(&clicks);
        }
        else if(r == -EAGAIN) {
            control_sendclicks(&clicks);    // turns on AC
            sleep(1);  // wait for the AC to turn on and image to be refreshed
        } 
        else syslog(LOG_NOTICE, "getclicks: %s", strerror(-r));

    } while(control->loop);

    return control;
}

int control_getclicks(
    struct buttonclick_st * clicks,
    struct panel_st * desired, 
    struct panel_st * actual)
{
    int r = 0;
    struct panel_st * diff;

    if(!desired || !actual) {
        errno = -EINVAL;
        return NULL;
    }

    diff = malloc(sizeof(struct panel_st));
    if(!diff) return NULL;

    clicks = memset(clicks, 0, sizeof(*clicks));

    /* Handle bad requests. The AC unit would never be able to be set to these
     * combinations in real life. 
     */
    if( ((int)desired->mode == 0 && (int)desired->fan != 0) ||
        ((int)desired->mode != 0 && (int)desired->fan == 0) ||
        desired->temperature > TEMPERATURE_MAXIMUM  ||
        desired->temperature < TEMPERATURE_MINIMUM
    ) {
        r = -EBADR;     // bad request
        goto ret;
    }

    /* Handle machine vision errors. The AC unit would never present these 
     * combinations real life. 
     */
    if( ((int)desired->mode == 0 && (int)desired->fan != 0) ||
        ((int)desired->mode != 0 && (int)desired->fan == 0) ||
        desired->temperature > TEMPERATURE_MAXIMUM  ||
        desired->temperature < TEMPERATURE_MINIMUM
    ) {
        r = -EREMOTEIO; // remote I/O error
        goto ret;
    }

    /* Handle power-on and power-off edge cases */
    if( actual->mode == MODE_NONE && actual->fan == FAN_NONE &&
        desired->mode != MODE_NONE && desired->fan != FAN_NONE
    ) {
        // Was off, needs to be powered on
        clicks->power = 1;
        clicks->delay = 0;
        clicks->mode = 0;
        clicks->fan = 0;
        clicks->plus = 0;
        clicks->minus = 0;
        r = -EAGAIN;    // try again, now with the AC on
        goto ret;
    }
    else if(    actual->mode != MODE_NONE && actual->fan != FAN_NONE &&
                desired->mode == MODE_NONE && desired->fan == FAN_NONE
    ) {
        // Was on, needs to be powered off
        clicks->power = 1;
        clicks->delay = 0;
        clicks->mode = 0;
        clicks->fan = 0;
        clicks->plus = 0;
        clicks->minus = 0;
        r = 0;          // all other desires are ignored
        goto ret;           
    }
    else if(    actual->mode == MODE_NONE && actual->fan == FAN_NONE &&
                desired->mode == MODE_NONE && desired->fan == FAN_NONE
    ) {
        // was off, needs to stay powered off
        clicks->power = 0;
        clicks->delay = 0;
        clicks->mode = 0;
        clicks->fan = 0;
        clicks->plus = 0;
        clicks->minus = 0;
        r = 0;
        goto ret;
    }
    

    //AC was on, needs to stay on, but needs changes to settings
    diff = accpanel_sub(desired, actual);

    clicks->power = 0;
    clicks->delay = 0;
    clicks->mode = 
        ((int)diff->mode < 0)?    diff->mode+(MODE_LASTELEMENT-1) : diff->mode;
    clicks->fan = 
        ((int)diff->fan < 0)?       diff->fan+(FAN_LASTELEMENT-1) : diff->fan;
    clicks->plus = 
        ((int)diff->temperature > 0)?          diff->temperature  : 0;
    clicks->minus = 
        ((int)diff->temperature < 0)?      abs(diff->temperature) : 0;

    ret:
    free(diff);
    return r;

}

int control_sendclicks(struct buttonclick_st * clicks)
{
    int r;

    size_t dstrs = 200;
    char * dstr = malloc(dstrs);
    
    r = control_buttonclick_snprint(dstr, dstrs, clicks);
    if(r > 0) {
        printf("Sendclick: %s\n", dstr);
        syslog(LOG_INFO,"Sending IR button presses: %s",dstr);
    }
    free(dstr);

    // TODO: send LIRC clicks

    return 0;
}

int control_buttonclick_snprint(char *str, size_t n, struct buttonclick_st * b)
{
    return (snprintf(str, n, 
        "{power: %i, fan: %i, mode: %i, delay: %i, plus: %i, minus: %i}",
        b->power, b->fan, b->mode, b->delay, b->plus, b->minus));
}
