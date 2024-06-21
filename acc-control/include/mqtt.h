#ifndef _MQTT_H_
#define _MQTT_H_

#include <stdbool.h>
#include "accpanel.h"

#define MQTT_BROKER_HOSTNAME "mosquitto.int.ivanveloz.com"
#define MQTT_BROKER_PORT (1883)
#define MQTT_BROKER_KEEPALIVE_S (60)
#define MQTT_MACHINEID_PATH "/etc/machine-id"

#define MQTT_TOPIC "ac-cloudifier"
#define MQTT_QOS (0)

struct mqtt_st {
    struct mosquitto *mosq;     /* libmosquitto client instance */
    bool connected;
    bool publish;               /* Flag to control the mqtt_publish thread */
    char uuid[256];
    struct machvis_st *mv;      /* machvis instance to get transmissions from */
};

int mqtt_initialize(struct mqtt_st * mqtt, struct machvis_st * mv);
int mqtt_finalize(struct mqtt_st * mqtt);
int mqtt_connect(struct mqtt_st * mqtt);
int mqtt_disconnect(struct mqtt_st * mqtt);
void *mqtt_publish(void *args);



#endif /* #ifndef _MQTT_H_ */
