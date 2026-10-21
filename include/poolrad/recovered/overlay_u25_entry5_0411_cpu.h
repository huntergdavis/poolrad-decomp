#ifndef POOLRAD_RECOVERED_OVERLAY_U25_ENTRY5_0411_CPU_H
#define POOLRAD_RECOVERED_OVERLAY_U25_ENTRY5_0411_CPU_H
#include <stddef.h>
#include <stdint.h>
#include "poolrad/recovered/overlay_u6f_entry6_0ec4_cpu.h"

/* Raw-CPU, full-state model of unit 0x25 [0411,0482) (113 bytes, 36 instructions, RETF 0Eh), the LOAD MONSTER wrapper
 * (docs/re/OVL_0025_0411_CPU.md). Entered by a far call: [SP] IP, [SP+2] CS, then the arguments (low address first):
 * [SP+4] far address A6 of an output far pointer, [SP+8] far address AA of an output far pointer, [SP+0Ch] far address AE of the
 * record variable, [SP+10h] the block id (a word slot, read as a byte). RETF 0Eh.
 *
 *   0411 PUSH BP; MOV BP,SP; SUB SP,4
 *   0417 AE^ := nil                                                       (LES DI,[BP+0Eh]; XOR AX,AX; two stores)
 *   0423 GetMem(11Dh) -> AE^                                              LCALL 0AF8:023F, back at 042Ch
 *   0436 push id (AL := [BP+12h], AH as it stands), push AE^'s segment and offset
 *   0444 LCALL 007C:003E                                                  unit 0x6F stub entry 6 = +0EC4, back at 0449h
 *   0449 AA^ := R+C8/CA;  A6^ := R+7F/81                                  (R := AE^^)
 *   047C MOV SP,BP; POP BP; RETF 0Eh
 *
 * Composition: GetMem 023F through its merged model (segment word read live from CS:042A), and the 0EC4 body through
 * pr_u6f_entry6_0ec4_cpu. The roll stub's rule is 0EC4's: the segment word at CS:0447 is a relocation read live, the stub entry
 * there must be a far JMP (EAh) to offset 0EC4h whose live segment is the loaded unit 0x6F; the INT 3Fh overlay-manager dispatch
 * is NOT executed. A call whose opcode or offset is not the original, or a stub that is not that JMP, is CODE with IP at the call
 * and before its return frame. Each callee must return to the instruction after its call (else TRANSFER).
 *
 * End states: RETURNED; every 0EC4 end state propagates unchanged (EXIT, RUNERROR, DIVIDE, BREAK, STALLED, LIMIT, SERVICE, CALLEE),
 * and GetMem's RUNERROR (203, heap exhausted: AE^ is nil at that stop), LIMIT and TRANSFER. ARGUMENT before any change; entry SP in
 * [0A04h,FFEEh] (0EC4's bound 09F4h plus the 10h bytes this wrapper has pushed when it calls it; the argument words stay below the
 * top). Exactly 1 MiB of A20-off RAM; the wrapper's own accesses through the caller's pointers (the three output variables, the record pointer AE^ and the record's fields) go through the bus, so they
 * may alias each other, the heap and even the EGA aperture; stack, code and the callees' flat-RAM windows are as in the callees' contracts. */
typedef pr_u6f_0ec4_cpu pr_u25_0411_cpu;

typedef enum {
    PR_U25_0411_RETURNED = 0, PR_U25_0411_ARGUMENT, PR_U25_0411_CODE, PR_U25_0411_EXIT, PR_U25_0411_RUNERROR,
    PR_U25_0411_DIVIDE, PR_U25_0411_BREAK, PR_U25_0411_STALLED, PR_U25_0411_LIMIT, PR_U25_0411_SERVICE,
    PR_U25_0411_CALLEE, PR_U25_0411_TRANSFER
} pr_u25_0411_status;

typedef enum { PR_U25_0411_NONE = -1, PR_U25_0411_GETMEM = 0, PR_U25_0411_ENTRY6 = 1 } pr_u25_0411_callee;

typedef struct {
    uint16_t site;                   /* IP of the call this stop concerns (0 when none) */
    int callee;                      /* pr_u25_0411_callee (-1 when none) */
    int callee_status;               /* that model's own status (-1 when none) */
    unsigned calls;                  /* far calls run to their return */
    pr_u6f_0ec4_diagnostic inner;    /* the 0EC4 call's diagnostic */
} pr_u25_0411_diagnostic;

pr_u25_0411_status pr_u25_entry5_0411_cpu(pr_bus *bus, size_t ram_size, pr_u25_0411_cpu *cpu, const pr_u6f_0ec4_env *env,
                                          pr_u25_0411_diagnostic *diagnostic);
#endif
