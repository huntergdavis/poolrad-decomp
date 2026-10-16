#ifndef POOLRAD_RECOVERED_OVERLAY_U25_TILDE_CAPS_156D_H
#define POOLRAD_RECOVERED_OVERLAY_U25_TILDE_CAPS_156D_H

#include <stddef.h>
#include <stdint.h>

#define PR_U25_156D_RAM_SIZE (1u << 20)

typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_u25_156d_cpu;

/* SYSTEM AF8 runtime callees and their confirmed cleanups. */
typedef enum {
    PR_U25_156D_IN_SET = 0, /* 0DC0 set membership, ZF result, RETF 6 */
    PR_U25_156D_DELETE,     /* 0CBC Delete(s, index, count), RETF 8 */
    PR_U25_156D_UPCASE,     /* 182A UpCase(ch) in AL, RETF 2 */
    PR_U25_156D_STRLOAD,    /* 0B0B load, RETF 4 keeps destination */
    PR_U25_156D_CHARSTR,    /* 0C3B char to string, RETF 2 */
    PR_U25_156D_CONCAT,     /* 0B98 concat, RETF 4 keeps destination */
    PR_U25_156D_STRSTORE    /* 0B25 store with cap, RETF 0xA */
} pr_u25_156d_call;

/* Call-site hook with IP at the lcall and arguments on SS:SP. The hook
 * supplies all runtime-routine RAM/CPU effects (including ZF for the set
 * test); this wrapper performs the confirmed argument cleanup. */
typedef int (*pr_u25_156d_external)(void *context, uint8_t *ram,
                                    size_t ram_size, pr_u25_156d_cpu *cpu,
                                    pr_u25_156d_call call);

typedef enum {
    PR_U25_156D_OK = 0,
    PR_U25_156D_ARGUMENT,
    PR_U25_156D_CALL_ABORT
} pr_u25_156d_status;

/* Entry at CS:156D after the caller's PUSH CS; CALL. SS:SP holds return
 * IP/CS, the far destination string and the far source string, which is
 * modified in place. The 32-byte set constant is read from CS:154D in the
 * caller-owned RAM by the set-test callee. */
pr_u25_156d_status pr_u25_tilde_caps_156d(
    uint8_t *ram, size_t ram_size, pr_u25_156d_cpu *cpu,
    pr_u25_156d_external external, void *context);

#endif
