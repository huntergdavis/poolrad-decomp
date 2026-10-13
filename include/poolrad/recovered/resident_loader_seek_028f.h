#ifndef POOLRAD_RECOVERED_RESIDENT_LOADER_SEEK_028F_H
#define POOLRAD_RECOVERED_RESIDENT_LOADER_SEEK_028F_H

#include "poolrad/recovered/resident_handler_prefix_02e6.h"

typedef struct {
    uint32_t linear;
    uint8_t old_byte, new_byte;
} pr_loader_seek_028f_write;

typedef enum {
    PR_LOADER_SEEK_028F_POST_SERVICE = 0,
    PR_LOADER_SEEK_028F_BLOCKED_DOS_SERVICE,
    PR_LOADER_SEEK_028F_BAD_EVENT,
    PR_LOADER_SEEK_028F_ARGUMENT
} pr_loader_seek_028f_status;

/* The host supplies the complete INT 21h result and ordered guest-RAM
 * writes. It may inspect but must not mutate ram. A zero return means the
 * event is ready; nonzero leaves the step blocked and unmodified. */
typedef int (*pr_loader_seek_028f_service)(
    void *context, const pr_handler_02e6_cpu *before,
    const uint8_t *ram, size_t ram_size,
    pr_handler_02e6_cpu *after,
    pr_loader_seek_028f_write *writes, size_t capacity,
    size_t *write_count);

/* Execute only 12C6:028F INT 21h, AX=4200h. A validated event advances
 * to the first post-service instruction +0291. Missing/blocked service and
 * invalid events leave the CPU and RAM untouched. The preceding +0268
 * stage supplies the seek request; later loader reads are separate. */
pr_loader_seek_028f_status pr_resident_loader_seek_028f(
    uint8_t *ram, size_t ram_size, pr_handler_02e6_cpu *cpu,
    pr_loader_seek_028f_service service, void *context,
    pr_loader_seek_028f_write *journal, size_t journal_capacity);

#endif
