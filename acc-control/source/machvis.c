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
    mv->machvispanel = NULL;                        // assigned externally
    if(!mv->machvistransmission) {
        syslog(LOG_ERR, "failed to initialize machvis: %s", strerror(errno));
    }
    return 0;
}

int machvis_finalize(struct machvis_st *mv)
{
    pthread_mutex_lock(&mv->machvismutex);
    free(mv->machvistransmission);
    //Mutex must be unlocked for destruction
    pthread_mutex_unlock(&mv->machvismutex);
    pthread_mutex_destroy(&mv->machvismutex);

    machvis_close(mv);
    pthread_mutex_destroy(&mv->socketmutex);

    return 0;
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
    r = close(mv->socketfd);
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
    char * buffer;
    
    r = machvis_open(mv);
    assert(r == 0);
  
    size_t n;
    mv->receive = true;
    do {
        pthread_testcancel();
        buffer = malloc(buffersize);
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

        pthread_mutex_lock(&mv->machvismutex);
        if(mv->machvispanel != NULL) {
            struct panel_st * p = mv->machvispanel; // fixes a concurrency bug
            pthread_mutex_unlock(&mv->machvismutex);
            r = machvis_parse(mv, p);
            if(r != 0 || r!= -EALREADY) syslog(LOG_DEBUG, "failed to parse!");
        }
        else {
            pthread_mutex_unlock(&mv->machvismutex);
        }

    } while(mv->receive);
    r = machvis_close(mv);
    return NULL;    
}

int machvis_parse(struct machvis_st *mv, struct panel_st *panel)
{
    int r;
    if(!mv || !panel) return -EINVAL;
    pthread_mutex_lock(&mv->machvismutex);
    if(mv->machvispanelparsed) {
        pthread_mutex_unlock(&mv->machvismutex);
        return -EALREADY;
    }

    struct panel_st * p = panel;
    pthread_mutex_lock(&p->mutex);
    r = accpanel_parse(p, mv->machvistransmission);
    if(r) goto ret;

    mv->machvispanelparsed = true;
    p->consumed = false;
    r = 0;

    ret:
    pthread_mutex_unlock(&p->mutex);
    pthread_mutex_unlock(&mv->machvismutex);
    return r; 
}

void machvis_machvispanel_set(struct machvis_st *mv, struct panel_st *panel) 
{
    pthread_mutex_lock(&mv->machvismutex);
    mv->machvispanel = panel;
    pthread_mutex_unlock(&mv->machvismutex);
}

struct panel_st * machvis_machvispanel_get(struct machvis_st *mv) 
{
    return mv->machvispanel;
}
