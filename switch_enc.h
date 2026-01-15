#ifndef hydro_random_switch_H
#define hydro_random_switch_H

#include <switch.h>
#include <stddef.h>

static int
hydro_random_init(void)
{
    /* libnx RNG doesn't require explicit init */
    return 0;
}

static void
hydro_random_buf(void *buf, size_t size)
{
    randomGetBytes(buf, size);
}

#endif
