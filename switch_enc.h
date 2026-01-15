#ifndef hydro_random_switch_enc_H
#define hydro_random_switch_enc_H

#include <switch.h>
#include <string.h>

/*
 * Only seed the DRBG.
 * hydro_random_buf() is implemented in random.h
 */

static int
hydro_random_init(void)
{
    /* Seed initial state using Horizon OS CSPRNG */
    randomGetBytes(hydro_random_context.state,
                   sizeof hydro_random_context.state);
    hydro_random_context.counter = 0;
    hydro_random_context.available = 0;
    return 0;
}

#endif
