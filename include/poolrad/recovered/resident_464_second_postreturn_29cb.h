#ifndef POOLRAD_RECOVERED_RESIDENT_464_SECOND_POSTRETURN_29CB_H
#define POOLRAD_RECOVERED_RESIDENT_464_SECOND_POSTRETURN_29CB_H

#include "poolrad/recovered/resident_464_2632_run014.h"

enum { PR_R464_SECOND_POSTRETURN_RAM_BYTES = 0xA0000u };

typedef enum {
    PR_R464_SECOND_POSTRETURN_OK = 0,
    PR_R464_SECOND_POSTRETURN_INVALID_HOST = 1,
    PR_R464_SECOND_POSTRETURN_UNSUPPORTED_PATH = 2
} pr_r464_second_postreturn_status;

/* Supplied second Move return only. Execute loaded +29CB..+29D7 and stop
 * at +29AB before the counter increment. No Move body is replayed. */
pr_r464_second_postreturn_status pr_r464_second_postreturn_29cb(
    uint8_t *ram, size_t ram_size, pr_r464_2632_cpu *cpu);

#endif
