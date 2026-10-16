#ifndef POOLRAD_RECOVERED_OVERLAY_U9E_ENTRY_1081_H
#define POOLRAD_RECOVERED_OVERLAY_U9E_ENTRY_1081_H

#include <stddef.h>
#include <stdint.h>

#define PR_U9E_1081_RAM_SIZE (1u << 20)

typedef struct {
    uint16_t ax, bx, cx, dx, si, di, bp, sp, cs, ip, ds, es, ss, flags;
} pr_u9e_1081_cpu;

typedef enum {
    PR_U9E_1081_CALL_LOOKUP = 0,
    PR_U9E_1081_CALL_COPY,
    PR_U9E_1081_CALL_EMIT,
    PR_U9E_1081_CALL_LOCAL_0026
} pr_u9e_1081_call_kind;

typedef enum {
    PR_U9E_1081_RETURNED = 0,
    PR_U9E_1081_ARGUMENT,
    PR_U9E_1081_CALLEE_REQUIRED,
    PR_U9E_1081_CALLEE_FAILED,
    PR_U9E_1081_TRANSFER
} pr_u9e_1081_status;

/* Called at the actual callee-entry CS:IP after U9E has pushed its return
 * frame. The callback owns the entire callee, including its return, argument
 * cleanup, CPU/RAM effects, and possible transfer. Return nonzero if it
 * supplied a terminal callee state; zero stops at that live state. */
typedef int (*pr_u9e_1081_call)(void *context, pr_u9e_1081_call_kind kind,
                                uint8_t *ram, size_t ram_size,
                                pr_u9e_1081_cpu *cpu);

typedef struct {
    uint16_t loaded_sidecar_cs; /* original calls name 00BA */
    uint16_t loaded_system_cs;  /* original call names 0AF8 */
    pr_u9e_1081_call call;
    void *context;
} pr_u9e_1081_io;

/* Complete original GAME.OVR U9E +1081..+10E3, entered after a far call
 * with three argument WORDs. Four conditional/ordered call sites are modeled
 * through the supplied full-state callback; the +0026 call is near after
 * PUSH CS. Original loaded code is assumed immutable. The host supplies
 * relocated callee segments; this API does not infer their live values. */
pr_u9e_1081_status pr_u9e_entry_1081(
    uint8_t *ram, size_t ram_size, pr_u9e_1081_cpu *cpu,
    const pr_u9e_1081_io *io, pr_u9e_1081_call_kind *stopped_at);

#endif
