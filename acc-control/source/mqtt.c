#include <string.h>
#include <syslog.h>
#include <stdio.h>
#include <mosquitto.h>
#include "mqtt.h"

int mqtt_initialize(struct mqtt_st * mqtt)
{
    int r;
    mqtt = memset(mqtt, 0, sizeof(*mqtt));
    r = mosquitto_lib_init();
    if(r != MOSQ_ERR_SUCCESS)
        return r;
    return 0;
}

int mqtt_finalize(struct mqtt_st * mqtt)
{
    int r;
    r = mosquitto_lib_cleanup();
    if(r != MOSQ_ERR_SUCCESS)
        return r;
    return 0;
}
