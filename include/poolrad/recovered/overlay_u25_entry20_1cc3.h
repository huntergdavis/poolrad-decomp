#ifndef POOLRAD_RECOVERED_OVERLAY_U25_ENTRY20_1CC3_H
#define POOLRAD_RECOVERED_OVERLAY_U25_ENTRY20_1CC3_H

#include <stddef.h>
#include <stdint.h>

#define PR_U25_1CC3_RAM_SIZE (1u << 20)

typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_u25_1cc3_cpu;

/* SYSTEM AF8 Real48 helpers: register-only, no stack arguments, plain
 * RETF (cleanup 0) when they return. */
typedef enum {
    PR_U25_1CC3_LONG_TO_REAL = 0, /* 119A: DX:AX -> DX:BX:AX, always returns */
    PR_U25_1CC3_REAL_MUL,         /* 1186: DX:BX:AX *= CX:SI:DI, RunError 205 */
    PR_U25_1CC3_REAL_TRUNC        /* 119E: CH=0, DX:BX:AX -> DX:AX, RunError 207 */
} pr_u25_1cc3_call;

/* Call-site hook with IP at the lcall. It applies the helper's register,
 * flag and RAM effects and returns 1 when the helper returns to the next
 * instruction, or 0 when it does not (a RunError exit, which never
 * returns, or a host failure). */
typedef int (*pr_u25_1cc3_external)(void *context, uint8_t *ram,
                                    size_t ram_size, pr_u25_1cc3_cpu *cpu,
                                    pr_u25_1cc3_call call);

typedef enum {
    PR_U25_1CC3_OK = 0,
    PR_U25_1CC3_ARGUMENT,
    /* A helper did not return; CPU and RAM are as the hook left them, IP at
     * that call site, and no later instruction has executed. */
    PR_U25_1CC3_NO_RETURN
} pr_u25_1cc3_status;

/* Entry at CS:1CC3 after the far CALL 2B:0084. SS:SP holds return IP/CS,
 * the far record pointer [BP+6] and the 6-byte Real48 factor [BP+0xA]. */
pr_u25_1cc3_status pr_u25_entry20_1cc3(
    uint8_t *ram, size_t ram_size, pr_u25_1cc3_cpu *cpu,
    pr_u25_1cc3_external external, void *context);

#endif
