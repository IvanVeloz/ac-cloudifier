#ifndef _MQTT_H_
#define _MQTT_H_

#include <stdbool.h>

#define MQTT_BROKER_HOSTNAME "test.mosquitto.org"
#define MQTT_BROKER_PORT (1883)
#define MQTT_BROKER_KEEPALIVE_S (60)
#define MQTT_MACHINEID_PATH "/etc/machine-id"

#define MQTT_TOPIC "ac-cloudifier"
#define MQTT_QOS "0"

struct mqtt_st {
    struct mosquitto *mosq;     /* libmosquitto client instance */
    bool connected;
    char uuid[256];
};

int mqtt_initialize(struct mqtt_st * mqtt);
int mqtt_finalize(struct mqtt_st * mqtt);
int mqtt_connect(struct mqtt_st * mqtt);
int mqtt_disconnect(struct mqtt_st * mqtt);


#endif /* #ifndef _MQTT_H_ */
