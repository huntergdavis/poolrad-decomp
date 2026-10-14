#ifndef POOLRAD_RECOVERED_RESIDENT_0709_DELAY_WRAPPER_0DBE_H
#define POOLRAD_RECOVERED_RESIDENT_0709_DELAY_WRAPPER_0DBE_H

#include <stddef.h>
#include <stdint.h>

/* DOS v1.3 START resident 0709:+0DBE..+0DD3, loaded at 0F2D:+0DBE.
 * This is the whole far routine including its A40 far-call frame. */
typedef struct {
    uint16_t ax, bx, cx, dx, si, di, bp, sp;
    uint16_t cs, ip, ds, es, ss, flags;
} pr_r0709_delay_cpu;

typedef enum {
    PR_R0709_DELAY_OK = 0,
    PR_R0709_DELAY_ARGUMENT,
    PR_R0709_DELAY_EXTERNAL,
    PR_R0709_DELAY_CALLBACK,
    PR_R0709_DELAY_CALLEE_FRAME
} pr_r0709_delay_status;

typedef struct {
    uint16_t argument;
    uint16_t callee_frame_sp;
    unsigned callback_invoked;
    int callback_status;
} pr_r0709_delay_report;

/* Callback is entered at A40:+029C after the wrapper has pushed its word
 * argument and far return address. Its SP must be unchanged on success: the
 * wrapper performs the callee's RETF 2, then its own POP BP and RETF. The
 * callback may alter any other CPU register, flags, and RAM as A40 would.
 * A nonzero return stops at that reached boundary without stack cleanup. */
typedef int (*pr_r0709_delay_a40_call)(void *ctx, uint8_t *ram,
                                        size_t ram_size,
                                        pr_r0709_delay_cpu *cpu,
                                        uint16_t argument);

/* Exactly one MiB of guest RAM is required for 20-bit real-mode addressing.
 * a40_cs is the relocated far-call target segment (0x1264 in the pinned
 * slot-A load). The caller's far return frame must already be at SS:SP.
 * The memory effects of all pushes remain in RAM after the returns.
 * MUL's CF/OF are cleared because byte*225 fits; its other flag results are
 * architecturally undefined and are left as supplied until the callback. */
pr_r0709_delay_status pr_r0709_delay_wrapper_0dbe(
    uint8_t *ram, size_t ram_size, pr_r0709_delay_cpu *cpu,
    uint16_t a40_cs, pr_r0709_delay_a40_call call_a40, void *ctx,
    pr_r0709_delay_report *report);

#endif
