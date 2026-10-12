#ifndef POOLRAD_RECOVERED_OVERLAY_U9E_ENTRY8_ROLL_H
#define POOLRAD_RECOVERED_OVERLAY_U9E_ENTRY8_ROLL_H

#include <stdint.h>

/* GAME.OVR unit 0x9E entry 8, +0x0DE3..+0x0E2D. The two Pascal WORD
 * arguments are count then range; only their low bytes affect this routine.
 * rand_seed is the mutable DWORD at DOS DS:0x4428. See U9E_ENTRY8_ROLL.md. */
typedef enum {
    PR_U9E_ENTRY8_OK = 0,
    PR_U9E_ENTRY8_ARGUMENT,
    PR_U9E_ENTRY8_SEED_MISSING
} pr_u9e_entry8_status;

typedef struct {
    uint8_t count;
    uint8_t range;
    unsigned random_calls;
    uint32_t final_seed; /* valid when a seed pointer was supplied */
} pr_u9e_entry8_report;

/* al_out is required. A zero effective count returns AL=0 without reading a
 * seed. A positive count requires rand_seed before its first random call.
 * report may be NULL. On an error, al_out and rand_seed are unchanged. */
pr_u9e_entry8_status pr_u9e_entry8_roll(uint16_t count_word,
                                        uint16_t range_word,
                                        uint32_t *rand_seed,
                                        uint8_t *al_out,
                                        pr_u9e_entry8_report *report);

#endif
