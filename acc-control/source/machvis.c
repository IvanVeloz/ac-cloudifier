#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <errno.h>
#include <syslog.h>
#include <arpa/inet.h> 
#include <sys/socket.h>
#include <netinet/in.h> 
#include <unistd.h> 
#include <pthread.h>
#include "accpanel.h"
#include "machvis.h"


int machvis_initialize(struct machvis_st * mv)
{
    mv = memset(mv, 0, sizeof(*mv));
    mv->machvispanelparsed = true;
    mv->machvispanelpublished = true;
    pthread_mutex_init(&mv->socketmutex,NULL);
    pthread_mutex_init(&mv->machvismutex,NULL);
    mv->machvistransmissionsize = 128;
    mv->machvistransmission = NULL;
    mv->machvispanel = NULL;
    if(!mv->machvistransmission || !mv->machvispanel) {
        syslog(LOG_ERR, "failed to initialize machvis: %s", strerror(errno));
    }
    return 0;
}

int machvis_finalize(struct machvis_st *mv)
{
    pthread_mutex_lock(&mv->machvismutex);
    free(mv->machvistransmission);
    free(mv->machvispanel);
    //Mutex must be unlocked for destruction
    pthread_mutex_unlock(&mv->machvismutex);
    pthread_mutex_destroy(&mv->machvismutex);

    machvis_close(mv);
    pthread_mutex_destroy(&mv->socketmutex);
}

int machvis_open(struct machvis_st * mv)
{
    
    int r = 0;
    if(mv->socketopen) return 0;

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_addr.s_addr = inet_addr(MACHVIS_SOCKET_PATH); 
    addr.sin_port = htons(MACHVIS_SOCKET_PORT); 
    addr.sin_family = MACHVIS_SOCKET_FAM;

    pthread_mutex_lock(&mv->socketmutex);

    mv->socketfd = socket(MACHVIS_SOCKET_FAM, SOCK_DGRAM, 0);
    if(mv->socketfd == -1) goto fail;

    r = bind(mv->socketfd, (struct sockaddr *)&addr, sizeof(addr));
    if(r < 0) goto fail;

    mv->socketopen = true;
    pthread_mutex_unlock(&mv->socketmutex);
    return 0;

    fail:
    mv->socketopen = false;
    pthread_mutex_unlock(&mv->socketmutex);
    syslog(LOG_ERR, "failed to bind machvis socket: %s", strerror(errno));
    return -1;

}

int machvis_close(struct machvis_st *mv)
{
    int r = 0;
    pthread_mutex_lock(&mv->socketmutex);
    r = close(&mv->socketfd);
    mv->socketopen = (r)? true : false;
    pthread_mutex_unlock(&mv->socketmutex);
    if(r) {
        syslog(LOG_ERR, "failed to close machvis socket: %s", strerror(errno));
        return -1;
    }
    return 0;
}

void *machvis_receive(void *args)
{
    int r = 0;
    struct machvis_st * mv = (struct machvis_st *)args;
    const size_t buffersize = 1024;
    char * buffer = malloc(buffersize);
    
    r = machvis_open(mv);
    assert(r == 0);
  
    size_t n;
    mv->receive = true;
    do {
        pthread_testcancel();
        pthread_mutex_lock(&mv->socketmutex);
        n = recvfrom(mv->socketfd, buffer, buffersize, 0, 
            (struct sockaddr*)NULL, NULL);
        pthread_mutex_unlock(&mv->socketmutex);
        
        if(n<=0) continue;
        syslog(LOG_DEBUG,"Got %lu bytes:\t",n);
        syslog(LOG_DEBUG,"%s\n", buffer);

        pthread_mutex_lock(&mv->machvismutex);
        free(mv->machvistransmission);
        mv->machvistransmissionsize = buffersize;
        mv->machvistransmission = buffer;
        mv->machvispanelparsed = false;
        mv->machvispanelpublished = false;
        pthread_mutex_unlock(&mv->machvismutex);

        buffer = malloc(buffersize);
        machvis_parse(mv);

    } while(mv->receive);
    r = machvis_close(mv);
    return NULL;    
}

int machvis_parse(struct machvis_st *mv)
{
    int r;
    pthread_mutex_lock(&mv->machvismutex);
    if(mv->machvispanelparsed) {
        pthread_mutex_unlock(&mv->machvismutex);
        return -EALREADY;
    }

    char * json = mv->machvistransmission;
    struct panel_st * p = malloc(sizeof(struct panel_st));
    int msdigit, lsdigit, fb;
    r = sscanf(json, 
        "{\"fan\": %i, \"mode\": %i, \"delay\": %i, \"msdigit\": %i, \"lsdigit\": %i, \"filterbad\": %i}", 
        &p->fan, &p->mode, &p->delay, &msdigit, &lsdigit, &fb);
    if(r != 6) {
        pthread_mutex_unlock(&mv->machvismutex);
        return -EINVAL;
    }

    p->temperature = (msdigit<0 || lsdigit<0)? (-1) : (10*msdigit + lsdigit);
    p->filterbad = (bool)fb;
    free(mv->machvispanel);
    mv->machvispanel = p;
    mv->machvispanelparsed = true;

    pthread_mutex_unlock(&mv->machvismutex);
    return 0; 
}
