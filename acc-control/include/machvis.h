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

    volatile bool receive;   /* Controls the machvis_receive thread */

    char * machvistransmission;
    size_t machvistransmissionsize;
    bool machvispanelparsed;
    bool machvispanelpublished;
    struct panel_st * machvispanel;
    pthread_mutex_t machvismutex;
};

int machvis_initialize(struct machvis_st * mv);
int machvis_finalize(struct machvis_st *mv);
int machvis_open(struct machvis_st * mv);
int machvis_close(struct machvis_st *mv);
void *machvis_receive(void *args);
int machvis_parse(struct machvis_st *mv, struct panel_st *panel);
void machvis_machvispanel_set(struct machvis_st *mv, struct panel_st *panel);
struct panel_st * machvis_machvispanel_get(struct machvis_st *mv);

#endif /* #ifndef _MACHVIS_H_ */
