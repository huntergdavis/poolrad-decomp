#ifndef POOLRAD_RECOVERED_OVERLAY_U6F_22D5_TRANSITION_PROMPT_H
#define POOLRAD_RECOVERED_OVERLAY_U6F_22D5_TRANSITION_PROMPT_H

#include <stddef.h>
#include <stdint.h>

/* GAME.OVR unit 0x6F + 0x22D5: public entry 11 (run-time stub 0x7C:0x57),
 * executable [22D5,23E0), 267 bytes, 107 instructions, 11 call sites, one
 * far argument (a Pascal string, the next script file's name) consumed by
 * `retf 4`. Evidence and offsets: docs/re/U6F_22D5_TRANSITION_PROMPT.md. It
 * is the area-transition prompt the NEWECL handler (unit 0x1C + 0x0BD1,
 * site +0C47) calls before loading the next block; it is not the block
 * loader (that is unit 0x25 entry 4). Semantics:
 *   +22DC  name := Copy(arg, 1, 0x28) (0xAF8:0x0B25, limit 0x28);
 *   +22EF  resident 0x802:0x02EB probes that name; AL <> 0 returns at once
 *          (the file is present: nothing is shown, nothing changes);
 *   +2300  digit := Str(DS:[0x5376]) into a string[1] (0xAF8:0x1366 with
 *          width 0 and capacity 1: the first decimal digit of the byte);
 *   +231A  message := CS:0x22B7 (28 characters) + digit (0x0B0B, 0x0B98);
 *          menu := Copy(message, 1, 0x28) (0x0B25); title := CS:0x22D4 (the
 *          empty string, 0x0B0B);
 *   +2351  AL := unit 0x3B0 entry 2 (the menu key input, 0x3C5:0x2A) with
 *          (title, menu, row 0x0D, bytes 0x0A, 0x0F, 0, 0, var extended);
 *   +2372  an extended key, or a key other than 0x53, returns;
 *   +237E  x := x - CS-independent table DS:[0x2736 + facing]; a negative
 *          byte becomes 0x0F, a byte above 0x0F becomes 0; the same for y
 *          with DS:[0x273F + facing] (x, y, facing = DS:0x6AAD..0x6AAF);
 *   +23CC  resident 0x802:0x095D (DS:[0x5376] := DS:[0x5377]); near +1D58
 *          (entry 9, the save routine); resident 0x899:0x0000 (the exit
 *          routine); +23DA frame restore; retf 4.
 * The probe, the key input, the archive-number restore, the save and the
 * exit are boundaries supplied as callbacks; the string services and the
 * coordinate step are translated inline; the private CS literal is supplied
 * by the caller with its exact original length. There are no loops. */

typedef struct {
    uint8_t archive_number;      /* DS:0x5376, read for the digit */
    uint8_t x;                   /* DS:0x6AAD */
    uint8_t y;                   /* DS:0x6AAE */
    uint8_t facing;              /* DS:0x6AAF, the index into both tables */
    const uint8_t *dx_table;     /* DS:0x2736.. (facing + 1 bytes needed) */
    size_t dx_table_size;
    const uint8_t *dy_table;     /* DS:0x273F.. (facing + 1 bytes needed) */
    size_t dy_table_size;
} pr_u6f_22d5_state;

typedef struct {
    void *context;
    int (*select_file)(void *context, const uint8_t *pascal_name, size_t span,
                       uint8_t *al);                                          /* 0x802:0x02EB */
    int (*key_input)(void *context, const uint8_t *pascal_title, size_t title_span,
                     const uint8_t *pascal_menu, size_t menu_span,
                     uint8_t row, uint8_t arg_a, uint8_t arg_b, uint8_t arg_c, uint8_t arg_d,
                     uint8_t *extended, uint8_t *al);                         /* unit 0x3B0 entry 2 */
    int (*restore_archive_number)(void *context);                             /* 0x802:0x095D */
    int (*save)(void *context);                                               /* +1D58, entry 9 */
    int (*exit_game)(void *context);                                          /* 0x899:0x0000 */
} pr_u6f_22d5_hooks;

typedef struct {
    uint8_t selected;            /* the probe's AL was non-zero: the prompt was skipped */
    uint8_t prompted;            /* the key input was reached */
    uint8_t key;                 /* the key input's AL */
    uint8_t extended;            /* the key input's extended-key byte */
    uint8_t moved;               /* x and y were stepped */
    uint8_t restored;            /* 0x802:0x095D was reached */
    uint8_t saved;               /* +1D58 was reached */
    uint8_t exited;              /* 0x899:0x0000 was reached (and returned) */
    uint16_t next_offset;        /* source offset the model stopped at, or 0x23E0 after retf */
} pr_u6f_22d5_result;

typedef enum {
    PR_U6F_22D5_OK = 0,
    PR_U6F_22D5_ARGUMENT,        /* spans, literal length, table span at the facing */
    PR_U6F_22D5_NEED_HOOK,       /* a reached boundary has no callback; next_offset = the call instruction */
    PR_U6F_22D5_HOST_FAILURE     /* a callback returned non-zero; earlier effects kept */
} pr_u6f_22d5_status;

/* file_name: the far string argument (length byte first; the span must hold
 * length + 1 bytes). message: the 28-character CS:0x22B7 constant. state: the
 * DS bytes and tables, updated in place on the key-0x53 path. */
pr_u6f_22d5_status pr_u6f_22d5_transition_prompt(
    const uint8_t *file_name, size_t file_name_size,
    const uint8_t *message, size_t message_size,
    pr_u6f_22d5_state *state,
    const pr_u6f_22d5_hooks *hooks,
    pr_u6f_22d5_result *out);

#define PR_U6F_22D5_MESSAGE_LENGTH 28u
#define PR_U6F_22D5_NAME_LIMIT 0x28u
#define PR_U6F_22D5_ROW 0x0Du
#define PR_U6F_22D5_ARG_A 0x0Au
#define PR_U6F_22D5_ARG_B 0x0Fu
#define PR_U6F_22D5_SAVE_KEY 0x53u
#define PR_U6F_22D5_COORD_MAX 0x0Fu
#define PR_U6F_22D5_RETF_OFFSET 0x23DDu
#define PR_U6F_22D5_END_OFFSET 0x23E0u

#endif
