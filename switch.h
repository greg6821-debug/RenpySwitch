#ifndef hydro_random_switch_H
#define hydro_random_switch_H

#include <switch.h>

static int
hydro_random_init(void)
{
    /* Seed DRBG state using Horizon OS CSPRNG */
    randomGetBytes(hydro_random_context.state, gimli_RATE);
    hydro_random_context.counter = 0;
    hydro_random_context.available = 0;
    return 0;
}

#endif
