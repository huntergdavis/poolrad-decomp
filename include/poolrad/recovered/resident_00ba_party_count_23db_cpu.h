#ifndef POOLRAD_RECOVERED_RESIDENT_00BA_PARTY_COUNT_23DB_CPU_H
#define POOLRAD_RECOVERED_RESIDENT_00BA_PARTY_COUNT_23DB_CPU_H
#include "poolrad/recovered/resident_00ba_link_check_2e03_cpu.h"

typedef pr_res_2e03_cpu pr_res_23db_cpu;
typedef enum {
    PR_RES_23DB_RETURNED = 0,
    PR_RES_23DB_ARGUMENT,   /* null RAM/CPU/budget, RAM not exactly 1 MiB, IP not 23DBh or entry SP outside [0006h,FFFCh]; nothing changed */
    PR_RES_23DB_LIMIT,      /* the host budget ran out at the next unexecuted instruction (a cyclic party list never ends, as in DOS) */
    PR_RES_23DB_TRANSFER    /* RETF popped a return address other than the entry frame's; the CPU is left at the popped CS:IP */
} pr_res_23db_status;

/* START.EXE resident segment 00BA [23DB,2433): the party count, 88 bytes / 29 instructions. Enter after the far call at IP 23DB with SS:[SP] return
 * IP and [SP+2] return CS; no arguments (RETF). PUSH BP; MOV BP,SP; SUB SP,4. The bytes DS:[6814] and DS:[6815] := 0. The local far pointer [BP-4]
 * := the party head DS:[5D96]/[5D98]; while it is not null (OR of its words): when the character's byte +10Dh is non-zero, INC BYTE DS:[6814h + CBW
 * of its byte +10Eh] (a signed index that may leave the initialised pair); then the pointer := the character's next link +104h/+106h. MOV SP,BP;
 * POP BP; RETF. The flags follow the pinned DOSBox-X stored rules (OR clears CF/OF/AF; INC keeps CF). Every operand is reread from RAM in source
 * order; LES reads its segment word at (offset+2) mod 64K. All accesses are flat RAM. Each instruction consumes one unit of *budget; LIMIT leaves
 * the next unexecuted instruction in cpu->ip. Exactly 1 MiB of A20-off RAM; entry SP in [0006h,FFFCh]. Code is immutable. */
pr_res_23db_status pr_res_00ba_party_count_23db_cpu(uint8_t *ram, size_t ram_size, pr_res_23db_cpu *cpu, size_t *budget);
#endif
