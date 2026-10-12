#ifndef POOLRAD_RECOVERED_OVERLAY_U6F_POSTINSERT_02AE_H
#define POOLRAD_RECOVERED_OVERLAY_U6F_POSTINSERT_02AE_H

#include <stddef.h>
#include <stdint.h>

/* Unit 0x6F:+0x02AE has no arguments. The live Pascal string represents
 * DS:0x537A, and the literals stand in for private CS data. Callbacks
 * replace reached DOS and resident effects; they may change the live string
 * before a retry. A callback returns zero on completion. */
typedef struct {
    void *context;
    int (*make_directory)(void *context, const uint8_t *asciiz,
                          size_t characters);
    int (*io_result)(void *context, uint16_t *result);
    int (*display_and_input)(void *context, const uint8_t *pascal_message,
                             size_t span, uint8_t color);
} pr_u6f_postinsert_02ae_hooks;

typedef struct {
    size_t attempts;       /* reached make-directory calls, including failure */
    size_t displays;       /* reached resident display/input calls */
    uint16_t last_io_result;
    uint16_t next_offset;  /* next source instruction, or terminal RETF +0x032E */
    uint8_t has_io_result;
} pr_u6f_postinsert_02ae_result;

typedef enum {
    PR_U6F_POSTINSERT_02AE_OK = 0,
    PR_U6F_POSTINSERT_02AE_ARGUMENT,
    PR_U6F_POSTINSERT_02AE_BOUNDS,
    PR_U6F_POSTINSERT_02AE_NEED_MKDIR,
    PR_U6F_POSTINSERT_02AE_NEED_IO_RESULT,
    PR_U6F_POSTINSERT_02AE_NEED_MESSAGE,
    PR_U6F_POSTINSERT_02AE_NEED_DISPLAY,
    PR_U6F_POSTINSERT_02AE_HOST_FAILURE,
    PR_U6F_POSTINSERT_02AE_RETRY_LIMIT
} pr_u6f_postinsert_02ae_status;

/* max_attempts is a host guard, checked before the next mkdir. Error
 * literals and display callback are only required after IOResult != 0/5.
 * The returned state retains every earlier completed callback effect. */
pr_u6f_postinsert_02ae_status pr_u6f_postinsert_02ae(
    uint8_t *live_pascal, size_t live_size,
    const uint8_t *message_prefix, size_t prefix_size,
    const uint8_t *message_suffix, size_t suffix_size,
    const pr_u6f_postinsert_02ae_hooks *hooks,
    size_t max_attempts, pr_u6f_postinsert_02ae_result *out);

#endif
