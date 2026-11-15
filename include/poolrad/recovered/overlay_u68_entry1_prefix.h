#ifndef POOLRAD_RECOVERED_OVERLAY_U68_ENTRY1_PREFIX_H
#define POOLRAD_RECOVERED_OVERLAY_U68_ENTRY1_PREFIX_H
#include <stddef.h>
#include <stdint.h>

/* Pure supplied-state projection; not a CPU/RAM or native-call model.
 * If report overlaps input or its declared field buffer, return ARGUMENT
 * without writing report. Otherwise a non-null report is initialized. */
typedef struct {
    uint8_t selector, x, y, facing, map_id, frame_residue;
    uint16_t window_word;
    const uint8_t *field_buffer;
    size_t field_buffer_size;
} pr_u68_e1_prefix_input;
typedef enum {
    PR_U68_PREFIX_ARGUMENT, PR_U68_PREFIX_MISSING_INPUT,
    PR_U68_PREFIX_SELECTOR_PATH, PR_U68_PREFIX_WINDOW_CLEAR,
    PR_U68_PREFIX_MESSAGE_PATH, PR_U68_PREFIX_BEFORE_073B,
    PR_U68_PREFIX_BEFORE_8AE
} pr_u68_e1_prefix_stop;
typedef struct { pr_u68_e1_prefix_stop stop; uint8_t entry3_result; unsigned entry3_valid; } pr_u68_e1_prefix_report;
pr_u68_e1_prefix_stop pr_u68_e1_prefix(const pr_u68_e1_prefix_input *input,
                                       pr_u68_e1_prefix_report *report);
#endif
