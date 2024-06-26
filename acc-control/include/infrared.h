#ifndef _INFRARED_H_
#define _INFRARED_H_

#ifdef HAVE_CONFIG_H
# include <config.h>	// this is LIRC code
#endif
#include "gpio.h"

enum InfraCodes {
        infra_power     =   0, 
        infra_speed     =   1, 
        infra_mode      =   2, 
        infra_plus      =   3, 
        infra_minus     =   4,
        infra_delay     =   5,
        infra_eco       =   6
};

struct infra_dev_st {
    enum InfraCodes code;
    char * InfraRemote;
    char * InfraStrings[];
};

/* Holds variables for the infra object. 
 */
struct infra_st {
    int _fd;
    struct infra_dev_st * dev;
    struct GPIO * gpio;
};

int infrared_initialize(struct infra_st * infra, struct GPIO * gpio);
int infrared_finalize(struct infra_st * infra);
/* This is a blocking call*/
int infrared_send(struct infra_st * infra, enum InfraCodes code);

#endif
