#ifndef _PANEL_H_
#define _PANEL_H_

#include <pthread.h>

enum panel_fan {
    FAN_NONE = 0,
    FAN_AUTO = 1,
    FAN_HIGH = 2,
    FAN_MED  = 3,
    FAN_LOW  = 4
};

enum panel_mode {
    MODE_NONE = 0,
    MODE_COOL = 1,
    MODE_FAN  = 2,
    MODE_ECO  = 3
};

enum panel_delay {
    DELAY_NONE = 0,
    DELAY_ON   = 1,
    DELAY_OFF  = 2
};

struct panel_st {
    enum panel_fan fan;
    enum panel_mode mode;
    enum panel_delay delay;
    int temperature;
    bool filterbad;
    pthread_mutex_t mutex;
};

#endif /* #ifndef _PANEL_H_ */
