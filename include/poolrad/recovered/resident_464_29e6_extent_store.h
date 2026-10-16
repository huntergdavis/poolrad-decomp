#ifndef POOLRAD_RECOVERED_RESIDENT_464_29E6_EXTENT_STORE_H
#define POOLRAD_RECOVERED_RESIDENT_464_29E6_EXTENT_STORE_H

#include "poolrad/recovered/resident_464_2632_run014.h"

enum { PR_R464_29E6_RAM_BYTES = 0xA0000u };

typedef enum {
    PR_R464_29E6_OK = 0,
    PR_R464_29E6_INVALID_HOST = 1,
    PR_R464_29E6_UNSUPPORTED_PATH = 2
} pr_r464_29e6_status;

/* Supplied +29E6 frame only: execute through the one word store at +29EF,
 * stopping at +29F3. SHL's auxiliary flag is undefined by the 8086 ISA;
 * the returned flags retain the supplied AF as a host convention. */
pr_r464_29e6_status pr_r464_29e6_extent_store(
    uint8_t *ram, size_t ram_size, pr_r464_2632_cpu *cpu);

#endif
