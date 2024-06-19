#ifndef _MACHVIS_H_
#define _MACHVIS_H_

#include <pthread.h>
#include <sys/socket.h>
#include "accpanel.h"

#define MACHVIS_SOCKET_FAM  (AF_INET)
#define MACHVIS_SOCKET_PATH "127.0.0.1"
#define MACHVIS_SOCKET_PORT (64000)

struct machvis_st {
    int socketfd;
    bool socketopen;
    pthread_mutex_t socketmutex;
    char * machvistransmission;
    size_t machvistransmissionsize;
    struct panel_st machvispanel;
    bool machvispanelparsed;
    bool machvispanelpublished;
    pthread_mutex_t machvismutex;
};

int machvis_initialize(struct machvis_st * mv);
int machvis_open(struct machvis_st * mv);
void *machvis_receive(void *args);


#endif /* #ifndef _MACHVIS_H_ */
