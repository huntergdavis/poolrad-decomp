#ifndef POOLRAD_RECOVERED_RESIDENT_LOADER_READ_BRANCH_02A1_H
#define POOLRAD_RECOVERED_RESIDENT_LOADER_READ_BRANCH_02A1_H

#include "poolrad/recovered/resident_handler_prefix_02e6.h"

typedef enum {
    PR_LOADER_READ_BRANCH_02A1_AT_EPILOGUE = 0,
    PR_LOADER_READ_BRANCH_02A1_AT_SECOND_DOS_READ,
    PR_LOADER_READ_BRANCH_02A1_ARGUMENT
} pr_loader_read_branch_02a1_status;

/* Execute from loaded 12C6:02A1, immediately after the first DOS read.
 * Stop before +02E1 POP DS or +02C0 INT 21h. The latter DOS service,
 * relocation loop, and return are outside this bounded stage. RAM is read
 * only; its size must be the one-megabyte real-mode address space. */
pr_loader_read_branch_02a1_status pr_resident_loader_read_branch_02a1(
    const uint8_t *ram, size_t ram_size, pr_handler_02e6_cpu *cpu);

#endif
