#ifndef POOLRAD_RECOVERED_OVERLAY_U5E_ENTRY2_0123_H
#define POOLRAD_RECOVERED_OVERLAY_U5E_ENTRY2_0123_H

#include <stddef.h>
#include <stdint.h>

#define PR_U5E_0123_RAM_SIZE (1u << 20)

typedef struct {
    uint16_t ax, bx, cx, dx, si, di, bp, sp, cs, ip, ds, es, ss, flags;
} pr_u5e_0123_cpu;

typedef enum {
    PR_U5E_0123_RETURNED = 0,
    PR_U5E_0123_ARGUMENT,
    PR_U5E_0123_CALLEE_REQUIRED,
    PR_U5E_0123_CALLEE_FAILED,
    PR_U5E_0123_TRANSFER
} pr_u5e_0123_status;

/* Called with the real far-call return frame at the loaded 00B0:002F.
 * The host owns all callee effects, return cleanup, and possible transfer. */
typedef int (*pr_u5e_0123_call)(void *context, uint8_t *ram,
                                size_t ram_size, pr_u5e_0123_cpu *cpu);

typedef struct {
    uint16_t loaded_callee_cs; /* original call operand 00B0 */
    pr_u5e_0123_call call;
    void *context;
} pr_u5e_0123_io;

/* GAME.OVR U5E entry 2, complete +0123..+0190 local body. The caller
 * supplies a far character pointer and enters after a far call. Source
 * assumes the loaded U5E body is immutable; callee effects are external. */
pr_u5e_0123_status pr_u5e_entry2_0123(
    uint8_t *ram, size_t ram_size, pr_u5e_0123_cpu *cpu,
    const pr_u5e_0123_io *io);

#endif
