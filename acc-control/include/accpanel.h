#ifndef _PANEL_H_
#define _PANEL_H_

#include <pthread.h>
#include <stdbool.h>

enum panel_fan {
    FAN_NONE = 0,
    FAN_AUTO = 1,
    FAN_HIGH = 2,
    FAN_MED  = 3,
    FAN_LOW  = 4,
    FAN_LASTELEMENT
};

enum panel_mode {
    MODE_NONE = 0,
    MODE_COOL = 1,
    MODE_FAN  = 2,
    MODE_ECO  = 3,
    MODE_LASTELEMENT
};

enum panel_delay {
    DELAY_NONE = 0,
    DELAY_ON   = 1,
    DELAY_OFF  = 2,
    DELAY_LASTELEMENT
};

enum panel_temperature {
    TEMPERATURE_MINIMUM = 64,
    TEMPERATURE_MAXIMUM = 86
};

struct panel_st {
    enum panel_fan fan;
    enum panel_mode mode;
    enum panel_delay delay;
    int temperature;
    bool filterbad;
    pthread_mutex_t mutex;
    bool consumed;
};

#define PANEL_INITIALIZER \
{ \
    .fan = FAN_NONE, \
    .mode = MODE_NONE, \
    .delay = DELAY_NONE, \
    .temperature = -1, \
    .filterbad = false, \
    .mutex = PTHREAD_MUTEX_INITIALIZER, \
    .consumed = true \
}

#define PANEL_TESTPANEL \
{ \
    .fan = FAN_MED, \
    .mode = MODE_COOL, \
    .delay = DELAY_NONE, \
    .temperature = 77, \
    .filterbad = false, \
    .mutex = PTHREAD_MUTEX_INITIALIZER, \
    .consumed = false, \
}

int accpanel_initialize(struct panel_st * panel, struct panel_st * template);
int accpanel_parse(struct panel_st * panel, const char * json);
struct panel_st * accpanel_sub(struct panel_st * a, struct panel_st * b);

/* Copy the contents of @param src to @param dest, respecting the mutex locks.
 * The `consumed` attribute is also copied. @returns 0 on success, negative 
 * errnos on failure.
 */
int accpanel_cpy(struct panel_st * dest, struct panel_st * src, bool lock);

#endif /* #ifndef _PANEL_H_ */
