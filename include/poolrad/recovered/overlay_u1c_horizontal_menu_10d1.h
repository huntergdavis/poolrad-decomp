#ifndef POOLRAD_RECOVERED_OVERLAY_U1C_HORIZONTAL_MENU_10D1_H
#define POOLRAD_RECOVERED_OVERLAY_U1C_HORIZONTAL_MENU_10D1_H

/* Bounded model of DOS v1.3 GAME.OVR unit 0x1c +0x10D1..+0x125C (file 0x1E17..0x1FA2, exactly 396 bytes,
 * 161 instructions, `retf` last byte, a 0x140-byte frame, no arguments, no result): the handler of ECL
 * opcode 0x2B (HORIZONTAL MENU, reference name; 2+n operands). The next routine is +0x125D. Evidence:
 * docs/re/U1C_HORIZONTAL_MENU_10D1.md.
 *
 * In the original's order (n = the byte below):
 *   [bp-5] := 0; text := '' (a 50-byte Pascal string local)             (+0x10D8..+0x10DC)
 *   loader(2) (0x2B:0x2A); dest := word(slot 1) (0x2B:0x4D)            (+0x10E0..+0x10F5)
 *   n := low byte of slot value(2) (0x2B:0x25)                         (+0x10F8..+0x1100)
 *   [0x49ED] := [0x49ED] - 1  (undoes the loader's PC increment)       (+0x1103)
 *   loader(n) (0x2B:0x2A): the n item operands, string items landing in the string buffers 1..  (+0x1107..+0x110B)
 *   n = 1: p3b := 1, p3d := 0x0F, p3e := 0x0F; if buffer 1 equals the code constant at cs:0x1085
 *          (0xAF8:0x0C10), buffer 1 := the constant at cs:0x10AA (0xAF8:0x0B25, max 0xFF)   (+0x111A..+0x114A)
 *   n <> 1: p3b := 0, p3d := 0x0F, p3e := 0x0A                          (+0x114C..+0x1154)
 *   p3c := 1 if [0x84E0] <> 0 and [0x84E1] <> 0, else 0                 (+0x1158..+0x116C)
 *   m := byte (n - 1); for i := 1 to m:                                 (+0x116F..+0x11D5)
 *       text := text + cs:0x10CD + buffer i + cs:0x10CF      (Pascal concatenation, stored with maximum 0x32)
 *   text := text + cs:0x10CD + buffer n   (stored with maximum 0x32)    (+0x11D7..+0x1210)
 *   al := 0x2B:0x61 (unit 0x25 entry 13) with (far var [bp-5], far text, 0x0D, p3e, p3d, p3b, p3c)  (+0x1215..+0x1232)
 *   store word 0x2B:0x52 (dest, zero-extended al)                       (+0x123A..+0x1243)
 *   0x709:0x0DD3 with the bytes (0, 0x18, 0x27, 0x18); return          (+0x1248..+0x1254)
 *
 * The string routines 0xAF8:0x0B0B (load), 0x0B98 (concatenate, length capped at 255), 0x0B25 (store with
 * maximum) and 0x0C10 (compare) are the standard Turbo Pascal Pascal-string runtime, decoded from the
 * START.EXE image and translated here (Pascal semantics, no hooks). The four code-segment constants at cs:0x1085,
 * 0x10AA, 0x10CD and 0x10CF are host data (Pascal strings, game text kept out of the repository; their lengths are
 * 36, 34, 1 and 1). The two remaining callees are unresolved explicit fallible host hooks: the menu routine
 * 0x2B:0x61 (unit 0x25 entry 13, +0x1684) and the display routine 0x709:0x0DD3. The
 * var byte [bp-5] is passed by reference to the menu routine (initialized to 0); the model returns what the hook leaves in it.
 *
 * A count n = 0 is a host bound (the original's byte loop would then run 255 iterations over string buffers beyond
 * the modelled ones); a count of 64 or more is the lane-A loader bound.
 *
 * Classification: bounded model composed of the lane-A loader, slot value, operand word and store word, the
 * translated Pascal string runtime, and two hooks. Not a strict translation. No game-function credit is claimed. */

#include <stdint.h>

#include "poolrad/recovered/ecl_helpers.h"

#define PR_U1C_HMENU_TEXT_MAX 0x32u          /* mov ax, 0x32 : StrStore maximum for the text local */
#define PR_U1C_HMENU_BUF_MAX 0xFFu           /* mov ax, 0xFF : StrStore maximum for string buffer 1 */
#define PR_U1C_HMENU_KIND 0x0Du              /* mov al, 0x0D pushed to the menu routine */

typedef struct pr_u1c_hmenu_env pr_u1c_hmenu_env;

typedef struct {
    /* 0x2B:0x61: (far var byte, far Pascal string, 0x0D, p3e, p3d, p3b, p3c) -> AL. `var5` is [bp-5] (0 on entry); the
       hook may change it. `text` is a length byte followed by that many bytes. Nonzero return stops the handler with
       PR_U1C_HMENU_CALLBACK. */
    int (*menu_25_0061)(void *ctx, pr_u1c_hmenu_env *env, uint8_t *var5, const uint8_t *text, uint8_t kind,
                        uint8_t p3e, uint8_t p3d, uint8_t p3b, uint8_t p3c, uint8_t *al);
    /* 0x709:0x0DD3 with the four byte words (0, 0x18, 0x27, 0x18) in push order. */
    int (*region_709_0dd3)(void *ctx, pr_u1c_hmenu_env *env, const uint8_t args[4]);
} pr_u1c_hmenu_hooks;

struct pr_u1c_hmenu_env {
    const uint8_t *str_1085, *str_10aa, *str_10cd, *str_10cf;   /* Pascal strings: code constants (host data) */
    uint8_t b84e0, b84e1;                                        /* DS bytes [0x84E0], [0x84E1] */
    pr_u1c_hmenu_hooks hooks;
    void *ctx;
};

typedef enum {
    PR_U1C_HMENU_OK = 0,
    PR_U1C_HMENU_ARGUMENT,   /* null state or env */
    PR_U1C_HMENU_CORE,       /* a lane-A core status ended the handler (report->core_status). A HOOK from the slot value(2) read stops
                                before any step that uses n (the PC decrement, loader(n), the hooks); a HOOK from the store word is
                                reported after the display hook, the handler having completed */
    PR_U1C_HMENU_COUNT,      /* n = 0: not modelled (host bound) */
    PR_U1C_HMENU_STRING,     /* a reached code constant is NULL */
    PR_U1C_HMENU_HOOK,       /* a reached hook is NULL; earlier effects retained */
    PR_U1C_HMENU_CALLBACK    /* a reached hook returned nonzero (report->callback_status); earlier effects retained */
} pr_u1c_hmenu_status;

typedef struct {
    uint8_t n;                       /* low byte of slot value(2) */
    uint16_t dest;                   /* word(slot 1) */
    uint8_t p3b, p3c, p3d, p3e;      /* the menu parameters */
    uint8_t buffer1_replaced;        /* n = 1 and buffer 1 equalled the constant at cs:0x1085 */
    uint8_t text[1 + PR_U1C_HMENU_TEXT_MAX];   /* the composed Pascal string, length first */
    uint8_t var5;                    /* [bp-5] after the menu hook */
    uint8_t result;                  /* AL from the menu hook */
    unsigned menu_called, stored, region_called;
    int callback_status;
    pr_rec_ecl_handler_status core_status;
    pr_rec_ecl_window_status slot_status, store_status;
} pr_u1c_hmenu_report;

/* The handler on lane A's loader state. `report` may be NULL. */
pr_u1c_hmenu_status pr_u1c_op_horizontal_menu(pr_rec_ecl_loader_state *state, pr_u1c_hmenu_env *env,
                                              pr_u1c_hmenu_report *report);

#endif
