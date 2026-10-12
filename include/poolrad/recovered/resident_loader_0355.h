#ifndef POOLRAD_RECOVERED_RESIDENT_LOADER_0355_H
#define POOLRAD_RECOVERED_RESIDENT_LOADER_0355_H

#include "poolrad/recovered/resident_handler_prefix_02e6.h"

typedef enum {
    PR_LOADER_0355_AT_CALL_0538 = 0,
    PR_LOADER_0355_ARGUMENT
} pr_loader_0355_status;

/* Execute +0355..+0362 after +0553 returns. Stop before the reached
 * +0363 CALL +0538. The caller supplies loaded CPU and 1 MiB RAM. */
pr_loader_0355_status pr_resident_loader_0355(
    uint8_t *ram, size_t ram_size, pr_handler_02e6_cpu *cpu);

#endif
