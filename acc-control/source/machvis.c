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
    pthread_mutex_init(&mv->socketmutex,NULL);
    pthread_mutex_init(&mv->machvismutex,NULL);
    mv->machvistransmissionsize = 128;
    mv->machvistransmission = malloc(mv->machvistransmissionsize);
    if(!mv->machvistransmission) {
        syslog(LOG_ERR, "failed to initialize machvis: %s", strerror(errno));
    }
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
    pthread_mutex_unlock(&mv->socketmutex);
    syslog(LOG_ERR, "failed to bind machvis socket: %s", strerror(errno));
    return -1;

}

int machvis_close(struct machvis_st *mv)
{
    int r = 0;
    pthread_mutex_lock(&mv->socketmutex);
    r = close(&mv->socketfd);
    pthread_mutex_unlock(&mv->socketmutex);
    if(r) {
        syslog(LOG_ERR, "failed to close machvis socket: %s", strerror(errno));
        return -1;
    }
    mv->socketopen = false;
    return 0;
}

void *machvis_receive(void *args)
{
    int r = 0;
    struct machvis_st * mv = (struct machvis_st *)args;
    const int buffersize = 1024;
    char * buffer;

    r = machvis_open(mv);
    assert(r == 0);

    buffer = malloc(buffersize);
    
    size_t n;
    while(true) {

        pthread_mutex_lock(&mv->socketmutex);
        n = recvfrom(mv->socketfd, buffer, buffersize, 0, (struct sockaddr*)NULL, NULL);
        pthread_mutex_unlock(&mv->socketmutex);
        
        if(n<=0) continue;
        printf("Got %lu bytes:\t",n);
        printf("%s\n", buffer);

        pthread_mutex_lock(&mv->machvismutex);
        strncpy(mv->machvistransmission, buffer, mv->machvistransmissionsize);
        mv->machvispanelparsed = false;
        mv->machvispanelpublished = false;
        pthread_mutex_unlock(&mv->machvismutex);
    }

    free(buffer);
}
