#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include <syslog.h>
#include <mosquitto.h>
#include "mqtt.h"

/* Automatically connect before publishing (if a connection is
 * needed at all), and try to handle any issues along the way. 
 * You should manually initialize before calling this function if your
 * program does not zero out memory.
 * @returns 0 on success, other values
 * on error. 
 */
static int mqtt_autoconnect(struct mqtt_st * mqtt);

int mqtt_initialize(struct mqtt_st * mqtt)
{
    int r;

    assert(mqtt != NULL);
    mqtt = memset(mqtt, 0, sizeof(*mqtt));

    int maj,min,rev;
    mosquitto_lib_version(&maj,&min,&rev);
    syslog(LOG_DEBUG,"Mosquitto version %d.%d.%d",maj,min,rev);

    r = mosquitto_lib_init();
    if(r != MOSQ_ERR_SUCCESS) {
        syslog(LOG_CRIT, "Failed to mosquitto_lib_init");
        return -EAGAIN;
    }

    mqtt->mosq = mosquitto_new(NULL, true, NULL);
    if(mqtt->mosq == NULL) {
        syslog(LOG_CRIT, "Failed to instantiate mosquitto client");
        return -errno;
    }

    FILE *machid = fopen(MQTT_MACHINEID_PATH, "r");
    if(!machid) goto generate_default_uuid;

    r = fgets(mqtt->uuid, sizeof(mqtt->uuid), machid);
    if(!r) goto generate_default_uuid;

    return 0;

    generate_default_uuid:
    strcpy(mqtt->uuid, "0");
    // TODO: generate random UUID on the fly using e.g. libuuid
    return 0;
}

int mqtt_finalize(struct mqtt_st * mqtt)
{
    int r;
    assert(mqtt != NULL);
    if(!mqtt->mosq) return -EINVAL;     // There is nothing to finalize

    // It's okay if mqtt_disconnect fails due to no connection
    mqtt_disconnect(mqtt);
    mosquitto_destroy(mqtt->mosq);

    r = mosquitto_lib_cleanup();
    if(r != MOSQ_ERR_SUCCESS) {
        syslog(LOG_CRIT, "Failed to mosquitto_lib_cleanup");
        return -EAGAIN;
    }

    return 0;
}

int mqtt_connect(struct mqtt_st * mqtt)
{
    int r;
    char * host = MQTT_BROKER_HOSTNAME;
    int  port = MQTT_BROKER_PORT;
    int  ka = MQTT_BROKER_KEEPALIVE_S;

    assert(mqtt != NULL);
    r = mosquitto_connect(mqtt->mosq, host, port, ka);
    if(r != MOSQ_ERR_SUCCESS) {
        syslog(LOG_WARNING, "Failed to connect to %s:%i",host,port);
        if(r == MOSQ_ERR_INVAL) {
            syslog(LOG_WARNING, "...because the parameters are invalid");
            return -EINVAL;
        }
        else {
            return -errno;
        }
    }
    mqtt->connected = true;
    return 0;
}

int mqtt_disconnect(struct mqtt_st * mqtt)
{
    int r;

    assert(mqtt != NULL);
    r = mosquitto_disconnect(mqtt->mosq);
    if(r != MOSQ_ERR_SUCCESS) {
        syslog(LOG_WARNING, "Failed to close connection");
        if(r == MOSQ_ERR_INVAL)
            syslog(LOG_WARNING, "...because the parameters are invalid");
        else if(r == MOSQ_ERR_NO_CONN)
            syslog(LOG_WARNING, "...because no connection was open");
        return -1;
    }
    mqtt->connected = false;
    return 0;
}

static int mqtt_autoconnect(struct mqtt_st *mqtt)
{
    int r = 0;

    if(!mqtt->mosq) {
        r = mqtt_initialize(mqtt);
        assert(r==0);
    }

    if(!mqtt->connected) {
        r = -1;
        for(int retry=0; retry<5; retry++) {
            r = mqtt_connect(mqtt);
            if(r) syslog(LOG_WARNING,"...%s",strerror(errno));
            else break;
        }
        assert(r==0);
    }
}

int mqtt_publish_panel_state(struct mqtt_st * mqtt)
{
    int r;
    r = mqtt_autoconnect(mqtt);
    assert(r == 0);
    // TODO: actually publish the panel state.
}

int mqtt_publish_unit_ping(struct mqtt_st * mqtt) 
{
    int r;
    r = mqtt_autoconnect(mqtt);
    assert(r == 0);
    r = mosquitto_publish(
        mqtt->mosq,
        NULL,
        MQTT_TOPIC,
        sizeof(mqtt->uuid),
        mqtt->uuid,
        1,
        true
    );
    syslog(LOG_ERR, "Couldn't ping: %s", mosquitto_strerror(r));
    return r;
}
