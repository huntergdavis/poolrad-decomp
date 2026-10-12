#ifndef POOLRAD_REPLAY_SLOT_A_NATIVE_SAVE_H
#define POOLRAD_REPLAY_SLOT_A_NATIVE_SAVE_H

#include <stddef.h>
#include <stdint.h>

/* The ten destination widths in the DOS world-save transfer table. */
#define PR_SLOT_A_WINDOW0_SIZE 0x800u
#define PR_SLOT_A_WINDOW1_SIZE 0x800u
#define PR_SLOT_A_WINDOW2_SIZE 0x400u
#define PR_SLOT_A_SCRIPT_SIZE 0x1E00u
#define PR_SLOT_A_PARTY_SIZE 5u
#define PR_SLOT_A_NAMES_SIZE 0x148u

/* Numeric window-0 words displayed as minutes, tens of minutes, and hours
 * on the saved one-Up route. Each offset addresses a little-endian word. */
#define PR_SLOT_A_CLOCK_MINUTE_OFFSET 0x18Eu
#define PR_SLOT_A_CLOCK_TENS_OFFSET 0x190u
#define PR_SLOT_A_CLOCK_HOUR_OFFSET 0x192u

typedef struct {
    uint8_t initial_ecl_file;                 /* transfer 0: DS:5376 */
    uint8_t window0[PR_SLOT_A_WINDOW0_SIZE];  /* transfer 1: [DS:49D2] */
    uint8_t window1[PR_SLOT_A_WINDOW1_SIZE];  /* transfer 2: [DS:49D6] */
    uint8_t window2[PR_SLOT_A_WINDOW2_SIZE];  /* transfer 3: [DS:49DA] */
    uint8_t script[PR_SLOT_A_SCRIPT_SIZE];    /* transfer 4: [DS:49DE] */
    uint8_t party[PR_SLOT_A_PARTY_SIZE];      /* transfer 5: DS:6AAD..6AB1 */
    uint8_t post_load_selector;               /* transfer 6: DS:49FA */
    uint8_t mode;                             /* transfer 7: DS:49F3 */
    uint8_t character_count;                  /* transfer 8: local count */
    uint8_t name_slots[PR_SLOT_A_NAMES_SIZE]; /* transfer 9: eight slots */

    /* Known post-load lookup, kept separate from the ten raw transfers.
     * The DOS loader overwrites DS:5376 from window 1 +0x624. */
    uint8_t restored_ecl_file;
    uint8_t map_id;                           /* window 0 +0x1E4 */
} pr_slot_a_native_state;

typedef struct {
    uint8_t x;
    uint8_t y;
    uint8_t facing;
    uint16_t minute;
    uint16_t tens;
    uint16_t hour;
} pr_slot_a_native_snapshot;

typedef enum {
    PR_SLOT_A_NATIVE_OK = 0,
    PR_SLOT_A_NATIVE_ARGUMENT,
    PR_SLOT_A_NATIVE_BAD_FRAME,
    PR_SLOT_A_NATIVE_LAYOUT,
    /* One Up cannot yet enter the untranslated gameplay key dispatch
     * (unit 0x1c:+0x32F8 into unit 0x68 entry 2). No state was changed. */
    PR_SLOT_A_NATIVE_BLOCKED_KEY_DISPATCH
} pr_slot_a_native_status;

/* Deep-copy all ten DOS transfer spans from one complete world save. This
 * does not load character records, map data, or execute post-load calls.
 * Failure leaves out unchanged. */
pr_slot_a_native_status pr_slot_a_native_restore(const uint8_t *data,
                                                  size_t size,
                                                  pr_slot_a_native_state *out);

pr_slot_a_native_status pr_slot_a_native_read_snapshot(
    const pr_slot_a_native_state *state, pr_slot_a_native_snapshot *out);

/* Attempt one Up from the restored state. The current first missing effect
 * is the key dispatch; this returns its blocker without mutating state. */
pr_slot_a_native_status pr_slot_a_native_one_up(pr_slot_a_native_state *state);

const char *pr_slot_a_native_status_string(pr_slot_a_native_status status);

#endif
