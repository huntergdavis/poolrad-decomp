#ifndef POOLRAD_RECOVERED_OVERLAY_U25_MOVE_19DB_CPU_H
#define POOLRAD_RECOVERED_OVERLAY_U25_MOVE_19DB_CPU_H
#include "poolrad/recovered/memory_bus.h"

typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_u25_19db_cpu;

typedef enum {
    PR_U25_19DB_RETURNED = 0,
    PR_U25_19DB_ARGUMENT,   /* null bus/RAM/CPU/budget, RAM not exactly 1 MiB, IP not 19DBh, entry SP outside [0030h,FFFCh]; nothing changed */
    PR_U25_19DB_LIMIT,      /* the host budget ran out at the next unexecuted instruction of this body (cpu->ip) */
    PR_U25_19DB_STUB,       /* a far call's overlay-stub slot is not a loaded JMP FAR to the expected 3BD entry (e.g. still INT 3Fh): CPU at the
                               LCALL, before its frame, so the host can let the overlay manager load unit 3BDh and resume */
    PR_U25_19DB_TRANSFER,   /* a callee's RETF or this body's RETF popped a return address other than its own frame's; CPU at the popped CS:IP */
    PR_U25_19DB_CALLEE      /* a 3BD callee rejected its entry (its ARGUMENT); CPU at the callee's entry */
} pr_u25_19db_status;

/* GAME.OVR unit 0x25 public entry 19 [19DB,1A65): the party step, 138 bytes / 50 instructions, RETF, no arguments. Enter after the far call at IP
 * 19DB: SS:[SP] return IP, [SP+2] return CS. PUSH BP; MOV BP,SP. On the facing byte DS:[6AAF] (MOV AL then CMP AL,0/2/4/6): 0 decrements the y byte
 * DS:[6AAE] (a signed JLE against 0 wraps it to 0Fh), 2 increments x DS:[6AAD] (a signed JGE against 0Fh wraps it to 0), 4 increments y, 6
 * decrements x; any other facing moves nothing. Then PUSH x, PUSH y (MOV AL,[..]; PUSH AX: AH is whatever AX held), LCALL stub:0034 (unit 3BDh
 * entry 4, +057A, the cell lookup, RETF 4) and DS:[6AB1] := AL; PUSH x, PUSH y, PUSH facing, LCALL stub:002A (entry 2, +047B, the nibble lookup,
 * RETF 6) and DS:[6AB0] := AL. POP BP; RETF. Each LCALL's segment word is the loaded stub segment (relocated); the stub slot must hold the overlay
 * manager's JMP FAR to unit 3BDh's entry, which the model follows; the callees are the raw models pr_u3bd_cell_057a_cpu (its map byte through the
 * bus) and pr_u3bd_nibble_047b_fullstate (flat). This body's DS globals and the stack are flat. Declared exclusion: +047B reads the map
 * behind DS:[6A5C] FLAT, so a map inside A0000..AFFFF is read through RAM by +047B (only +057A's read goes through the bus and the EGA). The flags follow the pinned DOSBox-X stored rules
 * (CMP: the six arithmetic flags; INC/DEC keep CF); at RETURNED they are the +047B callee's. Each instruction of THIS body consumes one unit of
 * *budget (the stub's JMP FAR and the callees do not; they are bounded). Exactly 1 MiB of A20-off RAM; entry SP in [0030h,FFFCh] so the callees'
 * frames stay inside the stack segment. Code is immutable. */
pr_u25_19db_status pr_u25_move_19db_cpu(pr_bus *bus, size_t ram_size, pr_u25_19db_cpu *cpu, size_t *budget);
#endif
