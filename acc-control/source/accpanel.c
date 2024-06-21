#include <stdbool.h>
#include <errno.h>
#include "accpanel.h"

struct panel_st * accpanel_parse(struct panel_st * panel, const char * json)
{
    int r;
    struct panel_st * p = panel;
    int msdigit, lsdigit, fb;
    r = sscanf(json, 
        "{\"fan\": %i, \"mode\": %i, \"delay\": %i, \"msdigit\": %i, \"lsdigit\": %i, \"filterbad\": %i}", 
        &p->fan, &p->mode, &p->delay, &msdigit, &lsdigit, &fb);
    if(r != 6) {
        return -EINVAL;
    }

    p->temperature = (msdigit<0 || lsdigit<0)? (-1) : (10*msdigit + lsdigit);
    p->filterbad = (bool)fb;

    return 0; 
}
