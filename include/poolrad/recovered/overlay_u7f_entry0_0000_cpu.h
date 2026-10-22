#ifndef POOLRAD_RECOVERED_OVERLAY_U7F_ENTRY0_0000_CPU_H
#define POOLRAD_RECOVERED_OVERLAY_U7F_ENTRY0_0000_CPU_H
#include "poolrad/recovered/resident_00ba_character_0bb8_cpu.h"

typedef enum {
    PR_U7F_0000_RETURNED = 0,
    PR_U7F_0000_ARGUMENT,   /* null/size/IP/SP outside the contract; nothing changed */
    PR_U7F_0000_CODE,       /* the far call at 000C is not `9A AB 13 seg`; IP at the call, before its return frame */
    PR_U7F_0000_LIMIT,      /* the host budget ran out at the next unexecuted instruction */
    PR_U7F_0000_TRANSFER,   /* the resident callee returned somewhere other than 0011 in this body's segment */
    PR_U7F_0000_CALLEE      /* the resident model stopped otherwise; diagnostic.callee_status says how */
} pr_u7f_0000_status;

typedef struct { int callee_status; uint16_t site; } pr_u7f_0000_diagnostic;

/* GAME.OVR unit 0x7F [0000,0020): 32 bytes / 12 instructions. Enter after the far CALL at IP 0000. Frame: SP IP, SP+2 CS, SP+4 the
 * pointer's offset word, SP+6 its segment word; RETF 4. The body hands the two words to the resident far call 00BA:13AB and returns
 * AX := result + 05DCh with the flags of that ADD.
 * The body dereferences no pointer itself: the record pointer it was given is passed on, and 13AB reads through it as flat RAM
 * (see the resident contract; the A0000..AFFFF aperture is outside it). The resident segment is the relocated word at CS:000F,
 * read live on every call. Exactly 1 MiB of A20-off RAM; entry SP in [0040h,FFF0h]. Each instruction of this body and of the
 * resident closure consumes one unit of *budget; LIMIT leaves the next unexecuted instruction in cpu->cs:ip. CPU, budget and
 * diagnostic storage are distinct from RAM. Code is immutable and the resident closure keeps its own contract. */
pr_u7f_0000_status pr_u7f_entry0_0000_cpu(uint8_t *ram, size_t ram_size, pr_res00ba_cpu *cpu, size_t *budget, pr_u7f_0000_diagnostic *diagnostic);
#endif
