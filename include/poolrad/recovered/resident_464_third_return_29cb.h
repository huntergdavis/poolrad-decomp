#ifndef POOLRAD_RECOVERED_RESIDENT_464_THIRD_RETURN_29CB_H
#define POOLRAD_RECOVERED_RESIDENT_464_THIRD_RETURN_29CB_H

#include "poolrad/recovered/resident_464_2632_run014.h"

enum { PR_R464_THIRD_RETURN_RAM_BYTES = 0xA0000u };

typedef enum {
    PR_R464_THIRD_RETURN_OK = 0,
    PR_R464_THIRD_RETURN_INVALID_HOST = 1,
    PR_R464_THIRD_RETURN_UNSUPPORTED_PATH = 2
} pr_r464_third_return_status;

/* Audit a supplied third Move return at +29CB. No Move or caller replay. */
pr_r464_third_return_status pr_r464_third_return_29cb(
    const uint8_t *ram, size_t ram_size, const pr_r464_2632_cpu *cpu);

#endif
