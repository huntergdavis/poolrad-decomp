#ifndef POOLRAD_RECOVERED_OVERLAY_U68_ENTRY2_UP_H
#define POOLRAD_RECOVERED_OVERLAY_U68_ENTRY2_UP_H

/* Bounded selector-1 extended-Up path through DOS v1.3 GAME.OVR entry 2
 * +0x0989..+0x0D4B. This represents selector [0x49FA]==1 and a prompt
 * event with extended!=0, key AL==0x48. Other entry-2 paths return
 * OUTSIDE_SLICE at their first branch, preserving prior effects.
 * Prompt preparation, key input, resident 0xBA:290B, and optional
 * 0x3F1:0137 are required host hooks when reached. No default key or
 * empty-hook success is supplied. See docs/re/U68_ENTRY2_UP_SLICE.md. */

#include <stdint.h>

#include "poolrad/recovered/overlay_u68_up_preflight.h"

#define PR_U68_E2_WORD_592 0x592u
#define PR_U68_E2_WINDOW1_MIN_SIZE 0x594u
#define PR_U68_E2_PROMPT_SOURCE_CS 0x97Au
#define PR_U68_E2_PROMPT_MESSAGE_DS 0x4B6u
#define PR_U68_E2_KEY_UP 0x48u

typedef struct pr_u68_e2_up_state pr_u68_e2_up_state;

typedef struct {
    uint8_t key_al;       /* AL returned by 0x3C5:0x2A */
    uint8_t extended;     /* byte written through SS:[BP-4] */
    uint8_t provided;     /* host must set nonzero for both values to be consumed */
} pr_u68_e2_key_event;

typedef struct {
    /* +0x09B2: runtime 0xAF8:0x0B0B copies a private CS literal into
     * local [BP-0x33]. The hook retains the private string; no text is
     * embedded in this public model. Zero means completed. */
    int (*prepare_prompt)(void *ctx, pr_u68_e2_up_state *state,
                          uint16_t source_cs_offset);
    /* +0x09D0: required input hook. It must provide both fields and set
     * event.provided; five raw low-byte controls are 0x0D,0x0A,0x0F,1,0. */
    int (*key_input)(void *ctx, pr_u68_e2_up_state *state,
                     uint16_t message_ds_offset, const uint8_t controls[5],
                     pr_u68_e2_key_event *event);
    int (*resident_290b)(void *ctx, pr_u68_e2_up_state *state); /* +0x0B9B */
    int (*exit_3f1_0137)(void *ctx, pr_u68_e2_up_state *state,
                          const uint8_t words_low[4]);       /* +0x0D35 */
    void *ctx;
} pr_u68_e2_up_hooks;

struct pr_u68_e2_up_state {
    uint8_t selector_49fa;             /* DS:[0x49FA], read after +0x592 clear */
    uint8_t previous_selector_49f9;    /* DS:[0x49F9], written at +0x0D1F */
    uint8_t flag_84e4;                 /* DS:[0x84E4], written only on exit hook arm */
    pr_u68_up_state preflight;         /* party/map/table/window inputs of merged +0x068D */
    const pr_u68_e2_up_hooks *hooks;
};

typedef enum {
    PR_U68_E2_UP_OK = 0,
    PR_U68_E2_UP_ARGUMENT,
    PR_U68_E2_UP_OUTSIDE_SLICE,
    PR_U68_E2_UP_HOOK,
    PR_U68_E2_UP_PREFLIGHT
} pr_u68_e2_up_status;

/* On OK, *al_out is the key supplied by key_input (0x48 on this slice).
 * On other statuses it is unchanged. stop_offset, when nonnull, identifies
 * the reached call/branch or the next offset after successful RETF. */
pr_u68_e2_up_status pr_u68_entry2_up_slice(pr_u68_e2_up_state *state,
                                            uint8_t *al_out,
                                            uint16_t *stop_offset);

#endif
