#ifndef hydro_random_switch_enc_H
#define hydro_random_switch_enc_H

#include <switch.h>
#include <string.h>
#include <stdint.h>

/*
 * Реализация randomGetBytes для Switch,
 * использует системный RNG из Horizon OS
 */
static void randomGetBytes(void *buf, size_t len) {
    uint8_t *p = (uint8_t *)buf;

    /* Используем svcGetSystemRandomSeed (доступно на всех версиях) */
    while (len > 0) {
        uint64_t seed;
        svcGetSystemRandomSeed(&seed);
        size_t copy = len < sizeof(seed) ? len : sizeof(seed);
        memcpy(p, &seed, copy);
        p += copy;
        len -= copy;
    }
}

/* Seed DRBG state from system RNG */
static int hydro_random_init(void)
{
    randomGetBytes(hydro_random_context.state,
                   sizeof hydro_random_context.state);
    hydro_random_context.counter = 0;
    hydro_random_context.available = 0;
    return 0;
}

#endif
