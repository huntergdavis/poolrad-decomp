#ifndef POOLRAD_RECOVERED_RESIDENT_LOADER_COMPOSED_028F_H
#define POOLRAD_RECOVERED_RESIDENT_LOADER_COMPOSED_028F_H

#include "poolrad/recovered/resident_loader_seek_028f.h"
#include "poolrad/recovered/resident_loader_dos_read_029f.h"

typedef struct {
    pr_loader_seek_028f_service seek;
    void *seek_context;
    pr_loader_seek_028f_write *seek_journal;
    size_t seek_capacity;
    pr_loader_dos_read_029f_service read;
    void *read_context;
    pr_loader_dos_read_029f_write *read_journal;
    size_t read_capacity;
} pr_loader_composed_028f_services;

typedef enum {
    PR_LOADER_COMPOSED_028F_AT_CALLER = 0,
    PR_LOADER_COMPOSED_028F_NEED_SEEK,
    PR_LOADER_COMPOSED_028F_NEED_FIRST_READ,
    PR_LOADER_COMPOSED_028F_NEED_SECOND_READ,
    PR_LOADER_COMPOSED_028F_BAD_SEEK_EVENT,
    PR_LOADER_COMPOSED_028F_BAD_READ_EVENT,
    PR_LOADER_COMPOSED_028F_ARGUMENT
} pr_loader_composed_028f_status;

/* Resume the loaded resident loader at +028F, +0291, +029F, +02A1,
 * or +02E1. The first seek/read require explicit host service events.
 * A missing event stops at its pending INT 21h with prior stages applied;
 * a later call may resume from that IP. The optional second read at +02C0
 * is a separate boundary. On the saved zero-count route, +02E1 POP DS
 * and +02E2 RETF 2 transfer to the far caller from the guest stack.
 * No DOS output, caller address, or file data is embedded here. */
pr_loader_composed_028f_status pr_resident_loader_composed_028f(
    uint8_t *ram, size_t ram_size, pr_handler_02e6_cpu *cpu,
    const pr_loader_composed_028f_services *services);

#endif
