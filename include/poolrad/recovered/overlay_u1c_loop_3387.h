#ifndef POOLRAD_RECOVERED_OVERLAY_U1C_LOOP_3387_H
#define POOLRAD_RECOVERED_OVERLAY_U1C_LOOP_3387_H

/* Bounded C model of DOS v1.3 GAME.OVR unit 0x1c gameplay-loop slice
 * +0x3387..+0x3507 (file 0x40CD..0x424D, 385 bytes, 123 instructions of the
 * 528-byte routine +0x32F8..+0x3507, one `retf`; unit file offset 3,398).
 * It starts after the loop's initialisation (+0x32F8..+0x3386, not modelled)
 * and runs the input / key-dispatch / movement-bookkeeping loop until
 * [0x49FF] ends it. Evidence: docs/re/U1C_LOOP_3387.md. All jumps are
 * internal; the outer loop is `jmp +0x3395` (+0x34FC), the key loop `jmp
 * +0x33B9` (+0x3449).
 *
 * Original order (L-labels are the byte offsets):
 *   +0x3387  window-0 word 0x1E4 := map id [0x84DC]
 *   +0x3395  key := unit 0x68 entry 2 (lcall 0x72:0x2A); [0x4436]:[0x4438] := [0x5D92]:[0x5D94];
 *            word 0x1E4 := [0x84DC]
 *   +0x33B9  window-1 word 0x594 > 1 -> +0x33D2;  else upcase(key) <> 0x45 -> +0x344C
 *   +0x33D2  [0x49FF] <> 0 -> +0x344C
 *            upcase(key) = 0x45 -> near +0x2DD3, then +0x342C
 *            else [0x84DD] := word 0x594 and 1; word 0x594 := 1; unit 0x3b4 entry 1 (0x3CA:0x25);
 *                 near +0x3171([0x49E5]); [0x442F] <> 0 -> near +0x31B4; word 0x594 := [0x84DD]
 *   +0x342C  [0x49FF] = 0 -> key := entry 2 again, [0x4436]:[0x4438] := [0x5D92]:[0x5D94];  then +0x33B9
 *   +0x344C  [0x49FF] = 0 -> near +0x3171([0x49E3])
 *            [0x442F] <> 0 -> near +0x31B4, to +0x34F5;   [0x49FF] <> 0 -> +0x34F5
 *   +0x3473  words 0x1E0, 0x1E2 := sign-extended [0x6AAD], [0x6AAE]; unit 0x68 entry 1 (0x72:0x25);
 *            0x3CA:0x25; zero-extended words vs sign-extended bytes differ (x, then y) ->
 *            resident 0x8AE:0x00DC([0x2628]); [0x84E0] := 0; [0x84E1] := 1; near +0x3171([0x49E5]);
 *            [0x442F] <> 0 -> near +0x31B4
 *   +0x34F5  [0x49FF] = 0 -> +0x3395;  else [0x49FF] := 0 and return
 * The upcase is the runtime routine 0xAF8:0x182A, read from START.EXE:
 * 'a'..'z' lose 0x20. The position compare is the original's 32-bit one: a
 * stored word is zero-extended and the byte sign-extended, so a coordinate
 * byte of 0x80..0xFF always reads as changed.
 *
 * Classification: bounded model of the 385-byte slice. NOT strict full C:
 * every callee (unit 0x68 entries 2 and 1, near +0x2DD3/+0x3171/+0x31B4, unit
 * 0x3b4 entry 1, resident 0x8AE:0x00DC) is an explicit hook that may change
 * the state (e.g. entry 1 moves the party); a NULL reached hook returns
 * PR_U1C_HOOK at once (effects before it are retained). No saved party tuple
 * or branch is assumed. No game-function credit is claimed. */

#include <stddef.h>
#include <stdint.h>

#define PR_U1C_KEY_SEARCH 0x45u               /* cmp al, 0x45 after the upcase */
#define PR_U1C_WINDOW0_MIN_SIZE 0x1E6u        /* words 0x1E0, 0x1E2, 0x1E4 */
#define PR_U1C_WINDOW1_MIN_SIZE 0x596u        /* word 0x594 */
#define PR_U1C_STEP_CAP 100000u               /* host guard on loop iterations */

typedef struct pr_u1c_state pr_u1c_state;

typedef struct {
    uint8_t (*entry2_key)(void *ctx, pr_u1c_state *st);                   /* lcall 0x72:0x2A, AL */
    void (*entry1)(void *ctx, pr_u1c_state *st);                          /* lcall 0x72:0x25 */
    void (*near_2dd3)(void *ctx, pr_u1c_state *st);
    void (*script_3171)(void *ctx, pr_u1c_state *st, uint16_t entry);     /* near +0x3171, one word */
    void (*near_31b4)(void *ctx, pr_u1c_state *st);
    void (*view_3ca_25)(void *ctx, pr_u1c_state *st);                     /* lcall 0x3CA:0x25 */
    void (*resident_8ae_00dc)(void *ctx, pr_u1c_state *st, uint16_t word);
} pr_u1c_hooks;

struct pr_u1c_state {
    uint8_t ds_49ff;                  /* loop-exit flag */
    uint8_t ds_442f;
    uint8_t ds_84dc;                  /* map id */
    uint8_t ds_84dd;
    uint8_t ds_84e0, ds_84e1;
    uint16_t ds_49e3, ds_49e5;        /* script entry words */
    uint16_t ds_2628;
    uint16_t ds_5d92, ds_5d94;        /* active character far pointer words */
    uint16_t ds_4436, ds_4438;        /* copy of the above */
    uint8_t party_x, party_y;         /* [0x6AAD], [0x6AAE] */
    uint8_t *window0;                 /* far [0x49D2]: words 0x1E0, 0x1E2, 0x1E4 */
    size_t window0_size;
    uint8_t *window1;                 /* far [0x49D6]: word 0x594 */
    size_t window1_size;
    pr_u1c_hooks hooks;
    void *ctx;
};

typedef enum {
    PR_U1C_OK = 0,
    PR_U1C_ARGUMENT,   /* null state/window or a window too small. Nothing done. */
    PR_U1C_HOOK,       /* a reached hook is NULL; earlier effects are retained */
    PR_U1C_LIMIT       /* more than PR_U1C_STEP_CAP key-loop or outer-loop iterations; state partly updated */
} pr_u1c_status;

typedef enum {
    PR_U1C_CALL_ENTRY2 = 1, PR_U1C_CALL_ENTRY1, PR_U1C_CALL_2DD3, PR_U1C_CALL_3171_E5, PR_U1C_CALL_3171_E3,
    PR_U1C_CALL_31B4, PR_U1C_CALL_VIEW, PR_U1C_CALL_8AE
} pr_u1c_call;

typedef struct {
    unsigned iterations;      /* passes through +0x3395 */
    unsigned key_loops;       /* passes through +0x33B9 */
    unsigned movement_passes; /* times +0x3473 ran */
    unsigned position_changed;/* times 0x8AE:0x00DC was called */
    uint8_t last_key;
    unsigned calls_n;
    pr_u1c_call calls[256];   /* calls in order (capped; calls_n counts all) */
} pr_u1c_report;

/* The upcase 0xAF8:0x182A. */
uint8_t pr_u1c_upcase(uint8_t c);

/* Runs the slice from +0x3387 to the `retf`. `report` may be NULL. */
pr_u1c_status pr_u1c_loop_3387(pr_u1c_state *state, pr_u1c_report *report);

#endif
