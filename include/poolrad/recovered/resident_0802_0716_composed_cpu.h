#ifndef POOLRAD_RECOVERED_RESIDENT_0802_0716_COMPOSED_CPU_H
#define POOLRAD_RECOVERED_RESIDENT_0802_0716_COMPOSED_CPU_H
#include <stddef.h>
#include <stdint.h>
#include "poolrad/recovered/resident_0802_0716_cpu.h"
#include "poolrad/recovered/resident_0802_02eb_cpu.h"
#include "poolrad/recovered/resident_0802_0193_cpu.h"

#define PR_R0802_0716C_RAM_SIZE (1u << 20)

/* The t30 cluster composed end to end: START.EXE resident 0802:0716 (pr_r0802_0716) with each of its seven near-call stops
 * replaced by the real leaf model, and its QUIT by the credited 0899:0000 (docs/re/RES_0802_0716_COMPOSED.md, VP i20311).
 *
 *   OPEN   @0775 -> 05CD (pr_r0802_05cd; its PROMPT inside 025D -> 0193, resumed at 02ADh), back at 0778h
 *   PROMPT @084A -> 0193 (pr_r0802_0193), back at 084Dh
 *   CLOSE  @08AD/@091D -> 06E7 (pr_r0802_06e7), back at 08B0h / 0920h
 *   ALLOC  @08BC -> 0020 (pr_r0802_0020), back at 08BFh
 *   DECODE @0935 -> 050F (pr_r0802_050f), back at 0938h
 *   FREE   @0941 -> 0000 (pr_r0802_0000), back at 0944h
 *   QUIT   @084D -> 0899:0000 (pr_r0899_quit_0000_cpu, credited) -> Halt
 *
 * No named stop remains: every stop is an architectural end state (RETURNED; EXIT at Halt 0AF8:00E9; RUNERROR at 0AF8:00E2;
 * DIVIDE at BlockRead's DIV; BREAK from ReadKey; STALLED, 050F's proven fixed point) or a declared limit or service refusal.
 * Each leaf must come back to the instruction after its call in 0716's segment (else CALLEE). The CPU is passed between the
 * models unchanged; RAM is the one 1 MiB image they all share. */
typedef pr_r0802_0716_cpu pr_r0802_0716c_cpu;

typedef struct {
    pr_r0802_0716_env loader;     /* BlockRead / FilePos / Seek services and GetMem's walk cap, as for 0716 */
    pr_r0802_02eb_env file;       /* INT 21h 3Dh/3Eh, as for 05CD, 025D, 02EB and 06E7 */
    pr_r0802_0193_env prompt;     /* the display (INT 10h) and ReadKey environments; prompt.key.quit also serves QUIT */
    unsigned heap_steps;          /* the GetMem / FreeMem free-list walk cap for 0020 and 0000 */
    uint32_t decode_arrivals;     /* 050F's max_arrivals */
} pr_r0802_0716c_env;

typedef enum {
    PR_R0802_0716C_RETURNED = 0,  /* 0716's RETF 12h executed */
    PR_R0802_0716C_ARGUMENT,      /* rejected before any change */
    PR_R0802_0716C_EXIT,          /* Halt reached (0716's QUIT, or QUIT inside ReadKey): the process exits */
    PR_R0802_0716C_RUNERROR,      /* RunError entered at 0AF8:00E2 */
    PR_R0802_0716C_DIVIDE,        /* BlockRead's DIV by recsize 0 */
    PR_R0802_0716C_BREAK,         /* ReadKey's Ctrl-Break path (EXIT class) */
    PR_R0802_0716C_STALLED,       /* 050F's proven fixed point: the original never returns (a HANG) */
    PR_R0802_0716C_LIMIT,         /* a declared cap: 050F's BUDGET, a heap walk, ReadKey's drain */
    PR_R0802_0716C_SERVICE,       /* a declared service declined or broke its continuation */
    PR_R0802_0716C_CALLEE         /* a model stopped otherwise, or a leaf returned elsewhere (see the diagnostic) */
} pr_r0802_0716c_status;

typedef struct {
    uint16_t seam;                /* the 0716 call site of the leaf that stopped (0775, 084A, 08AD, 08BC, 091D, 0935, 0941,
                                     084D for QUIT), 0 when 0716 itself stopped */
    uint16_t nested;              /* inside 05CD: 02AA when the 0193 prompt called from 025D stopped, else 0 */
    int leaf_status;              /* the stopping model's own status, -1 when none */
    unsigned seams_composed;      /* leaf calls run to their return, all seams together */
    unsigned prompts;             /* 0193 runs (from 025D and from 0716) */
    uint32_t decode_arrivals;     /* the last 050F run's loop-head arrivals */
} pr_r0802_0716c_diagnostic;

pr_r0802_0716c_status pr_r0802_0716_composed(uint8_t *ram, size_t ram_size, pr_r0802_0716c_cpu *cpu,
                                             const pr_r0802_0716c_env *env, pr_r0802_0716c_diagnostic *diagnostic);
#endif
