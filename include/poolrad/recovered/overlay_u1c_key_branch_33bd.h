#ifndef POOLRAD_RECOVERED_OVERLAY_U1C_KEY_BRANCH_33BD_H
#define POOLRAD_RECOVERED_OVERLAY_U1C_KEY_BRANCH_33BD_H

#include <stddef.h>
#include <stdint.h>

/* Explicit DOSBox debugger word order. This narrow CPU gate does not
 * infer a complete natural +33BD register state from the saved RAM. */
typedef struct pr_rec_u1c_33bd_cpu {
    uint16_t cs, ip, ds, es, ss, sp, bp;
    uint16_t ax, bx, cx, dx, si, di, flags;
} pr_rec_u1c_33bd_cpu;

/* GAME.OVR U1C:+33BD CMP WORD ES:[DI+0594h],1; +33C3 JA +33D2.
 * Stop at +33C5 or +33D2 before either successor body. Return zero
 * on unsupported input with every CPU word and RAM byte unchanged. */
int pr_rec_u1c_33bd_branch(pr_rec_u1c_33bd_cpu *cpu,
                            const uint8_t *ram, size_t ram_size);

#endif
