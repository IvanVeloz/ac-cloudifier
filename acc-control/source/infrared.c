#include <string.h>
#include <syslog.h>
#include <lirc_client.h>
#include "infrared.h"

/* Default device for the infra_dev_st class. This matches what we need
 * for the project. These came from `GE-AHP05LZQ2.lircd.conf`.
 * The string order must match the order in `enum InfraCodes`.
 */
static struct infra_dev_st infra_default_dev = {
    .InfraRemote    = "GE-AHP05LZQ2",
    .InfraStrings   = {
        "KEY_POWER",
        "KEY_AIRCON_SPEED",
        "KEY_MODE",
        "KEY_UP",
        "KEY_DOWN",
        "KEY_AIRCON_DELAY",
        "KEY_AIRCON_ECO",
    }
};

int infrared_initialize(struct infra_st * infra)
{
    infra = memset(infra, 0, sizeof(*infra));
    infra->_fd = lirc_get_local_socket(NULL, 0);
    if(infra->_fd < 0) {
        syslog(LOG_ERR, "Failed to open LIRC interface.");
        return infra->_fd;
    }
    infra->dev = &infra_default_dev;   // in the future, this can be a parameter
    return 0;
}

int infrared_finalize(struct infra_st * infra)
{
    int r;
    r = close(infra->_fd);
    if(r) return r;

    infra->_fd = -1;
    infra->dev = NULL;
    return r;
}

int infrared_send(struct infra_st * infra, enum InfraCodes code)
{
    int r;
    infra->dev->code = code;
    r = lirc_send_one(  infra->_fd, 
                        infra->dev->InfraRemote,
                        infra->dev->InfraStrings[infra->dev->code]);
    return r;
}
