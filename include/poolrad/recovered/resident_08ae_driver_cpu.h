#ifndef POOLRAD_RECOVERED_RESIDENT_08AE_DRIVER_CPU_H
#define POOLRAD_RECOVERED_RESIDENT_08AE_DRIVER_CPU_H
#include <stddef.h>
#include <stdint.h>
#include "poolrad/recovered/dos_gettime_00a2_cpu.h"

#define PR_R08AE_DRIVER_RAM_SIZE (1u << 20)

/* Composition glue (no body of its own, no credit) for the 08AE sound-driver entries a game caller reaches:
 * 08AE:00DC (dispatch) and 08AE:017B (entry). Each entry model stops at its near callees; this runs the merged
 * member models in place of those stops (137C, 13B1, 1742, 17AE, 181A, 006C, and RES_08AE_0000 for 006C's time,
 * which composes Dos.GetTime and SYSTEM LongMul), resumes the entry at the return point, and repeats until the
 * entry's RETF. Every model reads its operands, relocated immediates and code from the same RAM, so the composition
 * adds no state of its own (docs/re/RES_0899_QUIT_0000_CPU.md).
 *
 * Declared environment, the union of the members' own declarations and no more:
 *   dos_vector: INT 21h AH=25h/35h for 137C and 13B1 (their contract: called after the hardware frame is pushed,
 *               performs the vector-table service, reports the returned flags; nonzero refuses);
 *   port_out / port_in: OUT 40h/43h/61h/C0h and IN 61h from 137C/13B1 (nonzero refuses);
 *   gettime: INT 21h AH=2Ch inside Dos.GetTime (its contract; must not write RAM);
 *   delay_steps: 006C's LIMIT (dispatch steps of its wait loop). */
typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_r08ae_driver_cpu;

typedef struct {
    void *context;
    int (*dos_vector)(void *context, uint8_t *ram, pr_r08ae_driver_cpu *cpu, uint16_t *flags_out);
    int (*port_out)(void *context, uint16_t port, uint8_t value);
    int (*port_in)(void *context, uint16_t port, uint8_t *value);
    pr_dos_gettime_service gettime;
    unsigned delay_steps;
} pr_r08ae_driver_env;

typedef enum {
    PR_R08AE_DRIVER_RETURN = 0,  /* the entry's RETF (00DC: RETF 2) executed */
    PR_R08AE_DRIVER_ARGUMENT,    /* null pointers, size, or an entry IP other than 00DC/017B; nothing changed */
    PR_R08AE_DRIVER_MEMBER       /* a member model stopped (diagnostic names it and its status); the CPU and RAM are the
                                    state that member left */
} pr_r08ae_driver_status;

typedef struct {
    uint16_t member;      /* offset of the member that stopped: 00DC/017B (the entry), 137C, 13B1, 1742, 17AE, 181A, 006C */
    int status;           /* that member's own status code */
    int time_status;      /* pr_r08ae_time_cpu_status of the last RES_08AE_0000 run inside 006C, -1 when none */
    unsigned calls;       /* member calls made */
} pr_r08ae_driver_diagnostic;

pr_r08ae_driver_status pr_r08ae_driver_call(uint8_t *ram, size_t ram_size, pr_r08ae_driver_cpu *cpu,
                                            const pr_r08ae_driver_env *env, pr_r08ae_driver_diagnostic *diagnostic);
#endif
