#ifndef hydro_random_switch_enc_H
#define hydro_random_switch_enc_H

#include <switch.h>
#include <string.h>
#include <stdint.h>
#include <time.h>


static void randomGetBytes(void *buf, size_t len) {
    uint8_t *p = (uint8_t *)buf;
    uint64_t counter = (uint64_t)time(NULL); // простой seed
    for (size_t i = 0; i < len; ++i) {
        counter ^= (counter << 13);
        counter ^= (counter >> 7);
        counter ^= (counter << 17);
        p[i] = (uint8_t)counter;
    }
}

static int hydro_random_init(void)
{
    randomGetBytes(hydro_random_context.state,
                   sizeof hydro_random_context.state);
    hydro_random_context.counter = 0;
    hydro_random_context.available = 0;
    return 0;
}

#endif
