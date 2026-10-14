#ifndef POOLRAD_RECOVERED_OVERLAY_U77_SPELL_299C_H
#define POOLRAD_RECOVERED_OVERLAY_U77_SPELL_299C_H

#include <stdint.h>

/* External call sites in original execution order. The selected title call
 * has seven physical call sites but one runtime target (AF8:B25). */
typedef enum {
    PR_U77_299C_TITLE_0 = 0,
    PR_U77_299C_TITLE_1,
    PR_U77_299C_TITLE_2,
    PR_U77_299C_TITLE_3,
    PR_U77_299C_TITLE_4,
    PR_U77_299C_TITLE_5,
    PR_U77_299C_TITLE_6,
    PR_U77_299C_SELECT,
    PR_U77_299C_MESSAGE_1,
    PR_U77_299C_PREPARE,
    PR_U77_299C_ACTIVE_CALL,
    PR_U77_299C_MESSAGE_2,
    PR_U77_299C_APPEND,
    PR_U77_299C_DISPLAY,
    PR_U77_299C_FINAL
} pr_u77_299c_call;

/* The callback supplies the AL returned by SELECT and FINAL. Other return
 * values are unused by this body. It owns all external runtime effects. */
typedef uint8_t (*pr_u77_299c_external)(void *context,
                                         pr_u77_299c_call call);

typedef enum {
    PR_U77_299C_OK = 0,
    PR_U77_299C_ARGUMENT
} pr_u77_299c_status;

/* Models the in-body branch and call order of unit 0x77 entry 12. `slot`
 * stands for the byte at the first far pointer argument. `state_word`
 * stands for the signed word at the second far pointer. Callback-owned
 * runtime calls may mutate the pointed data before a later read. */
pr_u77_299c_status pr_u77_spell_299c(
    uint8_t mode, uint8_t *slot, const int16_t *state_word,
    pr_u77_299c_external external, void *context, uint8_t *result);

#endif
