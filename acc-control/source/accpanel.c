#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "accpanel.h"

int accpanel_parse(struct panel_st * panel, const char * json)
{
    int r;
    struct panel_st * p = panel;
    int msdigit, lsdigit, fb;
    r = sscanf(json, 
        "{\"fan\": %i, \"mode\": %i, \"delay\": %i, \"msdigit\": %i, \"lsdigit\": %i, \"filterbad\": %i}", 
        (int*)&p->fan, (int*)&p->mode, (int*)&p->delay, &msdigit, &lsdigit, &fb);
    if(r != 6) {
        return -EINVAL;
    }

    p->temperature = (msdigit<0 || lsdigit<0)? (-1) : (10*msdigit + lsdigit);
    p->filterbad = (bool)fb;

    return 0; 
}

struct panel_st * accpanel_sub(struct panel_st * a, struct panel_st * b)
{
    if(!a || !b) {
        errno = EINVAL;
        return NULL;
    }
    struct panel_st * r = malloc(sizeof(struct panel_st));
    if(!r) return NULL;
    
    r->delay = a->delay - b->delay;
    r->fan = a->fan - b->fan;
    r->mode = a->mode - b->mode;
    r->temperature = a->temperature - b->temperature;
    r->filterbad = a->filterbad || b->filterbad;

    return r;
}

int accpanel_cpy(struct panel_st * dest, struct panel_st * src)
{
    // This function is meant to be generic, so we shouldn't lock both source
    // and destination at the same time. It's a risk of deadlock.
    
    struct panel_st temp;
    pthread_mutex_t tempmutex;

    pthread_mutex_lock(&src->mutex);
    memcpy(&temp, src, sizeof(struct panel_st));
    pthread_mutex_unlock(&src->mutex);

    pthread_mutex_lock(&dest->mutex);
    dest->fan = temp.fan;
    dest->mode = temp.mode;
    dest->delay = temp.delay;
    dest->temperature = temp.temperature;
    dest->filterbad = temp.filterbad;
    dest->consumed = temp.consumed;
    pthread_mutex_unlock(&dest->mutex);

}
