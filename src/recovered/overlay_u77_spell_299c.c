/* GAME.OVR unit 0x77:+299C..+2B0C; external effects supplied by caller. */
#include "poolrad/recovered/overlay_u77_spell_299c.h"

#include <stddef.h>

pr_u77_299c_status pr_u77_spell_299c(
    uint8_t mode, uint8_t *slot, const int16_t *state_word,
    pr_u77_299c_external external, void *context, uint8_t *result)
{
    uint8_t selected;
    if (slot == NULL || state_word == NULL || external == NULL || result == NULL)
        return PR_U77_299C_ARGUMENT;

    if (mode <= 6u)
        (void)external(context, (pr_u77_299c_call)(PR_U77_299C_TITLE_0 + mode));

    selected = external(context, PR_U77_299C_SELECT);
    *slot = selected;
    if (selected == 0u) {
        *result = 0u;
        return PR_U77_299C_OK;
    }

    if (*state_word < 0) {
        (void)external(context, PR_U77_299C_MESSAGE_1);
        (void)external(context, PR_U77_299C_PREPARE);
        (void)external(context, PR_U77_299C_ACTIVE_CALL);
        (void)external(context, PR_U77_299C_MESSAGE_2);
        (void)external(context, PR_U77_299C_APPEND);
        (void)external(context, PR_U77_299C_DISPLAY);
    }
    *result = external(context, PR_U77_299C_FINAL);
    return PR_U77_299C_OK;
}
