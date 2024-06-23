#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include <syslog.h>
#include <mosquitto.h>
#include "mqtt.h"
#include "machvis.h"

/* Automatically connect before publishing (if a connection is
 * needed at all), and try to handle any issues along the way. 
 * You should manually initialize before calling this function if your
 * program does not zero out memory.
 * @returns 0 on success, other values
 * on error. 
 */
static int mqtt_autoconnect(struct mqtt_st * mqtt);

int mqtt_initialize(struct mqtt_st * mqtt, struct machvis_st * mv)
{
    int r;
    bool machineid = false;

    assert(mqtt != NULL);
    mqtt = memset(mqtt, 0, sizeof(*mqtt));

    mqtt->mv = mv;

    int maj,min,rev;
    mosquitto_lib_version(&maj,&min,&rev);
    syslog(LOG_DEBUG,"Mosquitto version %d.%d.%d",maj,min,rev);

    FILE *machid = fopen(MQTT_MACHINEID_PATH, "r");
    if(!machid) 
        r = 0;
    else 
        r = fgets(mqtt->uuid, sizeof(mqtt->uuid), machid);
    if(r) {
        machineid = true;
        for(size_t i=0; i<sizeof(mqtt->uuid); i++) {
            if(mqtt->uuid[i] == '\n') mqtt->uuid[i] = '\0';
        }
    }
    
    r = mosquitto_lib_init();
    if(r != MOSQ_ERR_SUCCESS) {
        syslog(LOG_CRIT, "Failed to mosquitto_lib_init");
        return -EAGAIN;
    }
    const char * uuid = (machineid)? mqtt->uuid:NULL;
    mqtt->mosq = mosquitto_new(uuid, true, NULL);
    if(mqtt->mosq == NULL) {
        syslog(LOG_CRIT, "Failed to instantiate mosquitto client");
        return -errno;
    }

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

    if(!mqtt->mosq) return -1;

    if(!mqtt->connected) {
        for(int retry=0; retry<5; retry++) {
            r = mqtt_connect(mqtt);
            if(r) syslog(LOG_WARNING,"...%s",strerror(errno));
            else break;
        }
    }
    return r;
}

void *mqtt_publish(void *args)
{
    int r = 0;
    struct mqtt_st * mqtt = (struct mqtt_st *)args;

    for(int i=0; i<5; i++) {
        r = mqtt_autoconnect(mqtt);
        if(r != 0) syslog(LOG_ERR, "Can't open MQTT: ", mosquitto_strerror(r));
        else break;
        sleep(1);
    }
    if(r!=0) return NULL;

    mqtt->publish = true;
    do {
        pthread_testcancel();
        pthread_mutex_lock(&mqtt->mv->machvismutex);
        if(mqtt->mv->machvispanelpublished){
            pthread_mutex_unlock(&mqtt->mv->machvismutex);
            continue;
        }
        r = mosquitto_publish(
            mqtt->mosq,
            NULL,
            MQTT_TOPIC,
            mqtt->mv->machvistransmissionsize,
            mqtt->mv->machvistransmission,
            0,
            true
        );
        printf("Published: %s\n", mqtt->mv->machvistransmission);
        if(r) {
            pthread_mutex_unlock(&mqtt->mv->machvismutex);
            syslog(LOG_ERR, "Couldn't publish: %s", mosquitto_strerror(r));
            usleep(300000);
            continue;
        }
        mqtt->mv->machvispanelpublished = true;
        pthread_mutex_unlock(&mqtt->mv->machvismutex);
    } while(mqtt->publish);
}

int mqtt_publish_panel_state(struct mqtt_st * mqtt, struct machvis_st * mv)
{
    int r;
    if(!mqtt || !mv) return -EINVAL;
    for(int i=0; i<5; i++) {
        r = mqtt_autoconnect(mqtt);
        if(r != 0) syslog(LOG_ERR, "Can't open MQTT: ", mosquitto_strerror(r));
        else break;
        sleep(1);
    }
    if(r!=0) return r;

    pthread_mutex_lock(&mv->machvismutex);
    if(mv->machvispanelpublished){
        pthread_mutex_unlock(&mv->machvismutex);
        return -EALREADY;
    }
    r = mosquitto_publish(
        mqtt->mosq,
        NULL,
        MQTT_TOPIC,
        mv->machvistransmissionsize,
        mv->machvistransmission,
        0,
        true
    );
    if(r) {
        pthread_mutex_unlock(&mv->machvismutex);
        syslog(LOG_ERR, "Couldn't publish: %s", mosquitto_strerror(r));
        return -EAGAIN;
    }
    printf("Published: %s\n", mv->machvistransmission);
    mv->machvispanelpublished = true;
    pthread_mutex_unlock(&mv->machvismutex);
    return 0;
}

int mqtt_publish_unit_ping(struct mqtt_st * mqtt) 
{
    int r;
    r = mqtt_autoconnect(mqtt);
    if(r != 0) syslog(LOG_ERR, "Couldn't open MQTT: ", mosquitto_strerror(r));
    r = mosquitto_publish(
        mqtt->mosq,
        NULL,
        MQTT_TOPIC,
        sizeof(mqtt->uuid),
        mqtt->uuid,
        1,
        true
    );
    if(r) syslog(LOG_ERR, "Couldn't ping: %s", mosquitto_strerror(r));
    return r;
}

char * mqtt_listen_command(struct mqtt_st *mqtt)
{
    int r;

    size_t msgsn = 1;
    struct mosquitto_message ** msgs = 
        calloc(msgsn, sizeof(struct mosquitto_message));
    
    r = mosquitto_subscribe_simple (
        msgs,
        msgsn,
        false,
        MQTT_LISTEN_TOPIC,
        MQTT_LISTEN_QOS,
        MQTT_BROKER_HOSTNAME,
        MQTT_BROKER_PORT,
        NULL,                   // todo: use previously obtained machineid
        MQTT_BROKER_KEEPALIVE_S,
        true,
        NULL,
        NULL,
        NULL,
        NULL
    );

    if(r) return NULL;
    if(!msgs) return NULL;
    
    char * cmd = malloc(msgs[0]->payloadlen);
    memcpy(cmd, msgs[0]->payload, msgs[0]->payloadlen);

    for(size_t i = 0; i < msgsn; i++) {
        mosquitto_message_free(msgs[i]);
    }

    return cmd;
}
