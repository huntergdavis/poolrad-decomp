#ifndef POOLRAD_RECOVERED_OVERLAY_ECL_STORE_HOOK_H
#define POOLRAD_RECOVERED_OVERLAY_ECL_STORE_HOOK_H

/* Portable C for DOS v1.3 GAME.OVR unit 0x25 + 0x0A74 (file 0x07F47), the
 * class-1 ECL store hook: the routine the store-word core (+0x0C77, entry
 * 10) calls right after writing a word into the 0x6B00 window, with the
 * same (address, value) arguments (`push [bp+8]; push [bp+6]; push cs; call
 * +0x0A74`, so a far frame; the hook returns `retf 4`). It is a private
 * routine: no public entry, no fixup-confirmed far caller, one near caller.
 * Issue #90; evidence: docs/re/ECL_STORE_HOOK_0A74.md.
 * Classification: path-complete local-body model with two callback
 * boundaries (FUNCTION_LEDGER.md vocabulary), not a full translation: the
 * routine's own instruction paths are reproduced, the two untranslated
 * callees behind its four lcall sites are caller-supplied callbacks.
 *
 * What it does (all proven from the decode, 515 bytes, 163 instructions,
 * 45 branches, four lcall sites to two distinct targets): rel = address -
 * 0x6B00 selects one case; most cases copy the value into the active
 * character record at far pointer [0x5D92]; two set engine flag bytes; four
 * cases call routines that are not translated (+0x0C03 -> resident
 * 0x802:0x094A; +0x0C26, +0x0C49, +0x0C6C -> overlay unit 0x3bd entry 6) and
 * are modelled as caller-supplied callbacks. This file is standalone: it
 * does not include or modify ecl_helpers.h; the PM connects it to the
 * store-word core after review. Field meanings are not asserted. */

#include <stddef.h>
#include <stdint.h>

#define PR_U25_HOOK_RECORD_SIZE 285u    /* the verified character record frame */
#define PR_U25_HOOK_WINDOW_BASE 0x6B00u

/* Status of one hook call. */
typedef enum {
    PR_U25_HOOK_OK = 0,
    PR_U25_HOOK_ARGUMENT,        /* null flags, or a record case with a null record: the engine
                                    would write through the far pointer [0x5D92] unchecked */
    PR_U25_HOOK_CALLOUT          /* the case reached a call-out and no callback was supplied;
                                    nothing else is pending (each call-out is the case's last act) */
} pr_u25_hook_status;

/* Which case ran, for tests and tracing. */
typedef enum {
    PR_U25_HOOK_CASE_NONE = 0,   /* rel matched no case: no effect */
    PR_U25_HOOK_CASE_FLAG_84D3,  /* rel 0x000 */
    PR_U25_HOOK_CASE_BYTE_RANGE, /* rel 0x020..0x070: record[rel - 8] = low byte */
    PR_U25_HOOK_CASE_B8,         /* rel 0x0B8: record[0x84] = low byte of (value > 0xB2 ? value - 0x32 : value) */
    PR_U25_HOOK_CASE_WORD,       /* rel 0x0BB/0xBD/0xBF/0xC1/0xC3/0xF7: word store at 0x88/0x8A/0x8C/0x8E/0x90/0xB8 */
    PR_U25_HOOK_CASE_BYTE_BA,    /* rel 0x0F9: record[0xBA] = low byte */
    PR_U25_HOOK_CASE_100,        /* rel 0x100: record[0x10D]/[0x10C] and flag [0x84D2] */
    PR_U25_HOOK_CASE_10C,        /* rel 0x10C: record[0x10E]/[0x10F] from values 0, 0x80, 0x81 */
    PR_U25_HOOK_CASE_312,        /* rel 0x312: call-out resident 0x802:0x094A with the word
                                    0x0300 | low byte of value (AX still held rel when AL was loaded) */
    PR_U25_HOOK_CASE_322         /* rel 0x322/0x324/0x326: if value > 0x80, call-out unit 0x3bd
                                    entry 6 (value & 0x7F, selector 1/2/3) */
} pr_u25_hook_case;

/* Engine flag bytes the hook sets (never cleared here); the script-state
 * initializer clears both (FUNCTION_LEDGER.md, +0x01BC..+0x01DA). */
typedef struct {
    uint8_t b84d2;
    uint8_t b84d3;
} pr_u25_hook_flags;

/* Call-outs. Argument order is the Pascal push order of the original; each
 * argument is the exact 16-bit word pushed.
 *   res_802_094a: one word, 0x0300 | (value & 0xFF). At +0x0BFF the original
 *     loads only AL while AX still holds rel = 0x0312, so AH is 0x03.
 *   u3bd_entry6: two words. The value word is (value & 0x7F) with AH cleared
 *     by `and ax, 0x7F`; the selector word is 0x0001/0x0002/0x0003 (`mov al,
 *     n` after that `and`, so AH stays 0). Both fit a byte exactly. */
typedef void (*pr_u25_hook_res_802_094a_fn)(void *context, uint16_t pushed_word);
typedef void (*pr_u25_hook_u3bd_entry6_fn)(void *context, uint8_t value, uint8_t selector);

typedef struct {
    pr_u25_hook_res_802_094a_fn res_802_094a;   /* resident 0x802:0x094A, one word argument */
    pr_u25_hook_u3bd_entry6_fn u3bd_entry6;     /* overlay unit 0x3bd entry 6 (+0x0E08), `retf 4` */
    void *context;
} pr_u25_hook_callouts;

typedef struct {
    pr_u25_hook_case which;
    unsigned record_offset;      /* first record byte written (CASE_BYTE_RANGE, _B8, _WORD, _BYTE_BA, _100, _10C), else 0 */
    unsigned callout_made;       /* 1 when a callback was invoked */
    unsigned callout_value;      /* the value word passed: 0x0300 | low byte (CASE_312), value & 0x7F (CASE_322) */
    unsigned callout_selector;   /* 1, 2 or 3 for CASE_322 */
} pr_u25_hook_report;

/* The hook. `address` and `value` are the store-word arguments (the hook
 * recomputes rel = address - 0x6B00 itself, so any address is accepted;
 * the engine only calls it for class-1 addresses). `record` is the 285-byte
 * active character record, writable; it may be NULL when no record case is
 * hit. `callouts` may be NULL or have NULL members (CALLOUT status when
 * reached). `report` is optional. */
pr_u25_hook_status pr_u25_store_hook_0a74(uint16_t address, uint16_t value, uint8_t *record,
                                          pr_u25_hook_flags *flags,
                                          const pr_u25_hook_callouts *callouts,
                                          pr_u25_hook_report *report);

#endif
