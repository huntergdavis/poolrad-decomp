#ifndef POOLRAD_RECOVERED_OVERLAY_U9E_ENTRY2_H
#define POOLRAD_RECOVERED_OVERLAY_U9E_ENTRY2_H

/* Portable C model of DOS v1.3 GAME.OVR unit 0x9e public entry 2 (+0x0026,
 * file 0x2AF10, run-time stub 0xB0:0x002A) and its near callee +0x0000
 * (entry 1, the 38-byte dispatcher). Entry 2 is exactly 282 bytes
 * +0x0026..+0x013F, 94 instructions, `retf 0xA`; the 32 bytes
 * +0x0140..+0x015F up to the next prologue are a 256-bit set constant used
 * by the routine at +0x0160, not part of this routine. Evidence:
 * docs/re/U9E_ENTRY2.md.
 *
 * Pascal frame, arguments pushed left to right: member far pointer
 * ([bp+0xE]:[bp+0xC]), type byte ([bp+0xA]), item far pointer
 * ([bp+8]:[bp+6]). It removes one item record from the member's item list:
 *   item null -> item = first record in the member's list with byte +0 == type
 *                (none: return, nothing done)                         (+0x002C..+0x007E)
 *   item.flag4 (+4) nonzero -> near call +0x0000 (type, member, item, 1) (+0x0083..+0x00A1)
 *   unlink: head == item -> member +0x7F := item +5; otherwise the
 *           record whose next is item gets item's next                (+0x00A4..+0x0127)
 *   FreeMem(item, 9)   (0xAF8:0x0254, argument shape)                 (+0x012B..+0x0135)
 * Entry 1 (+0x0000) is `lcall [0x6828 + 4 * type]` with (member, item,
 * mode byte). It is the call overlay_u7d_zero.c hooks.
 *
 * Classification: bounded model of the 282 + 38 bytes. The handlers behind
 * the DS far-pointer table at 0x6828 and the runtime FreeMem are host
 * boundaries (hooks); the table's live contents are not established. Not
 * claimed as strict full C; no game-function credit is claimed. */

#include <stddef.h>
#include <stdint.h>

#include "poolrad/recovered/overlay_u7d_zero.h"   /* pr_u7d0_member / pr_u7d0_item (+0x7F, +0x104; +0, +1, +4, +5) */

#define PR_U9E_HANDLER_COUNT 256u          /* di = type byte * 4 into DS:0x6828 */
#define PR_U9E_HANDLER_TABLE_DS 0x6828u
#define PR_U9E_ITEM_SIZE 9u                /* +0x0131: mov ax, 9 -> FreeMem(item, 9) */
#define PR_U9E_REMOVE_MODE 1u              /* +0x009D: mov al, 1, the byte entry 2 passes to the handler */

/* The far call through DS:0x6828[type]; the callee takes (member, item, mode byte as a word). */
typedef void (*pr_u9e_handler)(void *ctx, pr_u7d0_member *member, pr_u7d0_item *item, uint8_t mode);
/* 0xAF8:0x0254 with (item, 9). Called after the unlink; the host may free the record. */
typedef void (*pr_u9e_release_hook)(void *ctx, pr_u7d0_item *item, uint16_t size);

typedef struct {
    pr_u9e_handler handlers[PR_U9E_HANDLER_COUNT];   /* NULL = a null far pointer: the model skips it */
    pr_u9e_release_hook release;                     /* may be NULL */
    void *ctx;
} pr_u9e_state;

typedef enum {
    PR_U9E_OK = 0,
    PR_U9E_ARGUMENT,       /* null state or member. Nothing done. */
    PR_U9E_UNLINK_FAILED   /* the record is not in the member's list when the unlink runs: the original
                              stores through a null far pointer (es:[0:5..8]) and then frees the record;
                              the model stops there (handler already ran when flag4 was set; no release) */
} pr_u9e_status;

typedef struct {
    unsigned searched;             /* item was null: the list was searched by type */
    unsigned found;                /* a record was selected (searched or given) */
    unsigned handler_called;       /* flag4 nonzero */
    unsigned handler_was_null;     /* flag4 nonzero but handlers[type] is NULL */
    unsigned was_head;             /* unlinked as the head (member +0x7F rewritten) */
    unsigned released;             /* FreeMem(item, 9) reached */
} pr_u9e_report;

/* Entry 1 (+0x0000): `lcall [DS:0x6828 + 4 * type]` with (member, item, mode). */
void pr_u9e_dispatch(const pr_u9e_state *state, uint8_t type, pr_u7d0_member *member, pr_u7d0_item *item,
                     uint8_t mode);

/* Entry 2. `item` = the far pointer ([bp+8]:[bp+6]), NULL to search by `type`. */
pr_u9e_status pr_u9e_entry2(pr_u9e_state *state, pr_u7d0_member *member, uint8_t type, pr_u7d0_item *item,
                            pr_u9e_report *report);

/* Adapter with the pr_u7d0_remove_hook signature (`ctx` is a pr_u9e_state *), so overlay_u7d_zero.c's
 * unit 0x9e call can be wired to this model: passes the item through (never NULL there). */
void pr_u9e_as_u7d0_remove_hook(void *ctx, pr_u7d0_member *member, uint8_t type, pr_u7d0_item *item);

#endif
