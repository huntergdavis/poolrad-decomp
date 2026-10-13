#ifndef POOLRAD_RECOVERED_RESIDENT_LOADER_DOS_READ_029F_H
#define POOLRAD_RECOVERED_RESIDENT_LOADER_DOS_READ_029F_H

#include "poolrad/recovered/resident_handler_prefix_02e6.h"

#define PR_LOADER_DOS_READ_029F_MAX_WRITES 4096u

typedef struct {
    uint32_t linear;
    uint8_t old_byte, new_byte;
} pr_loader_dos_read_029f_write;

typedef enum {
    PR_LOADER_DOS_READ_029F_POST_SERVICE = 0,
    PR_LOADER_DOS_READ_029F_BLOCKED_DOS_SERVICE,
    PR_LOADER_DOS_READ_029F_BAD_EVENT,
    PR_LOADER_DOS_READ_029F_ARGUMENT
} pr_loader_dos_read_029f_status;

/* The host supplies the complete INT 21h result and ordered guest-RAM
 * writes. It may inspect but must not mutate ram. Zero means an event is
 * ready; nonzero leaves the service blocked and state unmodified. */
typedef int (*pr_loader_dos_read_029f_service)(
    void *context, const pr_handler_02e6_cpu *before,
    const uint8_t *ram, size_t ram_size,
    pr_handler_02e6_cpu *after,
    pr_loader_dos_read_029f_write *writes, size_t capacity,
    size_t *write_count);

/* Execute only loaded 12C6:029F INT 21h, AH=3Fh. A validated event
 * advances to +02A1, before the following JB. The preceding +0291
 * stage supplies DS:DX and CX; later loader branches remain separate. */
pr_loader_dos_read_029f_status pr_resident_loader_dos_read_029f(
    uint8_t *ram, size_t ram_size, pr_handler_02e6_cpu *cpu,
    pr_loader_dos_read_029f_service service, void *context,
    pr_loader_dos_read_029f_write *journal, size_t journal_capacity);

#endif
