#ifndef POOLRAD_RECOVERED_OVERLAY_U68_ROLL_GATE_0294_H
#define POOLRAD_RECOVERED_OVERLAY_U68_ROLL_GATE_0294_H

/* Bounded C model of DOS v1.3 GAME.OVR unit 0x68 internal routine
 * +0x0294..+0x04DE (file 0x1695F, exactly 587 bytes, 247 instructions,
 * `retf`, no arguments, result AL; no trailing data: the next routine is
 * +0x04DF). Near-called from entry 1 (+0x0D6D) for key 'B' at +0x0ECA and
 * +0x1009 (result stored to [bp-1]). Evidence: docs/re/U68_ROLL_GATE_0294.md.
 *
 * For each node of the far list at DS:[0x5D96] (next far pointer at +0x104),
 * until a node sets `found`, it calls the merged unit 0x3bd entry 3 with the
 * party x, y and facing bytes (the same arguments every iteration) and
 * branches on the node's byte +0x10 ("type") and, for type 0x12 only (the only
 * place byte +0x16 is read), byte +0x16:
 *   entry 3 result = 3:
 *     type 0x12: byte +0x16 in 0x5B..0x63 -> roll 1d6 == 1;  == 0x64 -> roll 1d6 <= 2;
 *                otherwise [0x6DDE] := 0 and no roll
 *     type 0x13, 0x14 -> 1d6 <= 3;  0x15, 0x16 -> 1d6 <= 4;  0x17 -> 1d6 <= 5;
 *     type 0x18 -> 1d8 <= 7;  0x19 -> found with no roll;  any other type -> [0x6DDE] := 0
 *   entry 3 result <> 3:
 *     type 3..7 -> 1d6 == 1;  8..0xF -> 1d6 <= 2;  0x10, 0x11 -> 1d6 <= 3;
 *     type 0x12: byte +0x16 in 0..0x32 -> 1d6 <= 3; 0x33..0x63 -> <= 4; == 0x64 -> <= 5;
 *                otherwise nothing;  any other type -> nothing
 * "Roll" is the far call 0xB0:0x48 (unit 0x9e entry 8) with the two byte
 * arguments (count 1, then 6 or 8), exactly as in the adjacent +0x04DF model;
 * the comparison is unsigned on AL ("== 1" is an equality: a roll of 0 does
 * not count; the "<= n" forms count 0). After the list, when found: the local
 * map updater +0x0109 (merged as overlay_u68_map_update.h) is called with
 * (x, y, facing) and then with (x + dx[facing], y + dy[facing], (facing + 4)
 * mod 8), the table bytes at DS:0x2736 + facing and DS:0x273F + facing sign
 * extended; the callee reads only the low bytes. The next pointer is
 * read once more after the hit, as in the original. Result AL = found.
 *
 * Classification: bounded model of the 587 bytes. NOT strict full C: the
 * roll (unit 0x9e entry 8, which has its own random-state semantics) and the
 * map updater are explicit hooks (the updater has its own merged model the
 * host can wire in); entry 3 is the merged translation; far pointers are
 * host views as in overlay_u68_movement_flag.h. No game-function credit is
 * claimed. */

#include <stddef.h>
#include <stdint.h>

#include "poolrad/recovered/overlay_u3bd_field.h"
#include "poolrad/recovered/overlay_u68_list_search.h"

#define PR_U68_ROLL_TYPE_OFFSET 0x10u         /* es:[di + 0x10] */
#define PR_U68_ROLL_VALUE_OFFSET 0x16u        /* es:[di + 0x16] */
#define PR_U68_ROLL_NEXT_OFFSET 0x104u        /* far next pointer at +0x104/+0x106 */
#define PR_U68_ROLL_SEGMENT_SIZE 65536u       /* a reached view must be a whole segment, as in +0x04DF's model */

/* 0xB0:0x48: AL of unit 0x9e entry 8, called with (count byte, range byte) pushed in that order;
 * the hook receives them in the adjacent model's order (range, then count). */
typedef uint8_t (*pr_u68_roll_gate_roll_hook)(void *ctx, uint8_t range_byte, uint8_t count_byte);
/* near +0x0109 with (x, y, selector), each the low byte of the pushed word. */
typedef void (*pr_u68_roll_gate_mark_hook)(void *ctx, uint8_t x, uint8_t y, uint8_t selector);

typedef struct {
    pr_u68_roll_gate_roll_hook roll_9e_entry8;
    pr_u68_roll_gate_mark_hook mark_0109;
    void *ctx;
} pr_u68_roll_gate_hooks;

typedef struct {
    pr_u68_far head;                    /* DS:[0x5D96]:[0x5D98] */
    const pr_u68_segment_view *views;   /* 64 KiB ES views for reached nodes */
    size_t view_count;
    pr_u68_far *visited;                /* caller scratch for the host cycle guard */
    size_t visited_capacity;
    uint8_t party_x, party_y, facing;   /* DS:[0x6AAD], [0x6AAE], [0x6AAF] */
    uint8_t ds_6dde;                    /* DS:[0x6DDE], cleared to 0 on the "other type" branches */
    const uint8_t *direction_table;     /* DS bytes from 0x2736; dy at +9; reached only on a hit */
    size_t direction_table_size;
    pr_u3bd_field_state field;          /* merged entry 3's inputs */
    pr_u68_roll_gate_hooks hooks;
} pr_u68_roll_gate_state;

typedef enum {
    PR_U68_ROLL_OK = 0,
    PR_U68_ROLL_ARGUMENT,   /* null state/result, a missing or short view for a reached node, a link word at
                               0xFFFF, a short direction table when reached, or entry 3's null-buffer guard */
    PR_U68_ROLL_CYCLE,      /* host guard: a node reached twice */
    PR_U68_ROLL_CAPACITY,   /* host guard: more nodes than `visited_capacity` */
    PR_U68_ROLL_HOOK        /* a reached hook is NULL. Effects before it are retained. */
} pr_u68_roll_gate_status;

typedef struct {
    unsigned nodes;             /* nodes visited */
    unsigned rolls;
    unsigned cleared_6dde;      /* times [0x6DDE] := 0 ran */
    unsigned marks;             /* calls of the map updater (0 or 2) */
    uint8_t first_entry3;       /* entry 3's result at the first node */
} pr_u68_roll_gate_report;

/* The routine. On OK, *al_out = AL (0 or 1). `report` may be NULL. */
pr_u68_roll_gate_status pr_u68_roll_gate(pr_u68_roll_gate_state *state, uint8_t *al_out,
                                         pr_u68_roll_gate_report *report);


/* Raw-composition upgrade of the existing bounded model: exactly
 * [0294,04DF), 587 executable bytes / 247 instructions; no trailing data.
 * See docs/re/U68_ROLL_GATE_0294_CPU.md. */
#include "poolrad/recovered/overlay_u68_map_update_0109_cpu.h"
typedef pr_u68_0109_cpu pr_u68_roll_gate_cpu;
typedef enum {
    PR_U68_ROLL_CPU_OK = 0, PR_U68_ROLL_CPU_ARGUMENT,
    PR_U68_ROLL_CPU_CODE, PR_U68_ROLL_CPU_CALLEE,
    PR_U68_ROLL_CPU_TRANSFER, PR_U68_ROLL_CPU_LIMIT
} pr_u68_roll_gate_cpu_status;
typedef enum {
    PR_U68_ROLL_CPU_NONE = 0, PR_U68_ROLL_CPU_FIELD,
    PR_U68_ROLL_CPU_DICE, PR_U68_ROLL_CPU_MAP
} pr_u68_roll_gate_cpu_callee;
typedef struct {
    pr_u68_roll_gate_cpu_callee callee; /* last reached call boundary */
    int status; /* -1 until that callee runs */
} pr_u68_roll_gate_cpu_report;
/* Enter after far CALL at IP0294, SP0040..FFFC, with 1MiB A20-off ordinary
 * RAM and immutable code. Loaded overlay stubs are read from RAM.
 * List budget stops at target02AB; dice budget is per invocation and stops
 * in U9E at0E0E. Failed/stopped CPU and RAM are retained without unwind.
 * Dice LIMIT clears undefined XOR AF for the measured emulator profile;
 * the shared dice API is unchanged. No gameplay routing is changed. */
pr_u68_roll_gate_cpu_status pr_u68_roll_gate_0294_cpu(
    uint8_t *ram, size_t ram_size, pr_u68_roll_gate_cpu *cpu,
    unsigned max_loopbacks, unsigned max_system_calls,
    pr_u68_roll_gate_cpu_report *report);

#endif
