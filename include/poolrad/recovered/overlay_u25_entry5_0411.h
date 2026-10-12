#ifndef POOLRAD_RECOVERED_OVERLAY_U25_ENTRY5_0411_H
#define POOLRAD_RECOVERED_OVERLAY_U25_ENTRY5_0411_H

/* Bounded model of DOS v1.3 GAME.OVR unit 0x25 +0x0411..+0x0481 (file 0x78E4..0x7954, exactly 113 bytes,
 * 36 instructions, `retf 0xE` last, four unused bytes of locals): public entry 5 (run-time stub 0x2B:0x39),
 * called by ECL LOAD MONSTER (unit 0x1c +0x04AA). Evidence: docs/re/U25_ENTRY5_0411.md.
 *
 * Arguments (Pascal, pushed left to right, `retf 0xE` = 14 bytes): [bp+0x12] word `id` (only its LOW byte is
 * read, `mov al,[bp+0x12]`; the high byte the caller pushed is discarded at entry), [bp+0xE] far pointer to a far-pointer
 * variable A, [bp+0xA] far pointer to a far-pointer variable B, [bp+6] far pointer to a far-pointer variable C.
 *
 * In the original's order:
 *   A := nil (both words zero)                                           (+0x0417..+0x0422)
 *   P := GetMem(0x11D)  (0xAF8:0x023F; DX:AX; runtime error 203 on failure)   (+0x0423..+0x042B)
 *   A := P                                                              (+0x042C..+0x0435)
 *   0x7C:0x3E (unit 0x6f entry 6, +0x0EC4) with (id word, far P: segment pushed first, then offset)   (+0x0436..+0x0444)
 *       the id word is `mov al,[bp+0x12]; push ax` and AX still holds the GetMem result's offset (`les di` and the
 *       two stores do not change AX): so the word is (P.offset & 0xFF00) | (input id & 0x00FF)
 *   B := the far pointer at Q + 0xC8 (offset word) and Q + 0xCA (segment word)   (+0x0449..+0x0463), Q = the far pointer CURRENTLY in A
 *       (`les di,[bp+0xE]; les di,es:[di]` re-reads variable A from memory)
 *   C := the far pointer at Q' + 0x7F (offset word) and Q' + 0x81 (segment word)   (+0x0466..+0x0478), Q' = A read AGAIN after B was written
 *       (both LES operations repeat; if B aliases A, Q' is the pointer just stored to B; the initializer may also have changed A)
 *   return
 *
 * Negative evidence from the bytes: no data-segment access, no other call, no FreeMem on any path, no check of the
 * GetMem result (a failure is inside GetMem), and **nothing writes the object's bytes**: B and C are whatever the
 * initializer left at +0xC8 and +0x7F. B and C are not touched when GetMem or the initializer fails.
 *
 * GetMem and the initializer are explicit fallible host hooks; the object's bytes live in the host heap, reached through
 * an explicit object table (far pointer -> view) because the model never invents contents. Words are little-endian and
 * every byte address wraps at 16 bits.
 *
 * Classification: bounded model (two hooks, the object view mapping and the heap are explicit boundaries). Not a strict
 * translation. No game-function credit is claimed. */

#include <stddef.h>
#include <stdint.h>

#define PR_U25_E5_OBJECT_SIZE 0x11Du      /* mov ax, 0x11D : GetMem size */
#define PR_U25_E5_OFF_B 0xC8u             /* offset word at +0xC8, segment word at +0xCA */
#define PR_U25_E5_OFF_C 0x7Fu             /* offset word at +0x7F, segment word at +0x81 */

typedef struct { uint16_t off, seg; } pr_far16;

typedef struct pr_u25_e5_env pr_u25_e5_env;

typedef struct {
    uint8_t *segment;                     /* host bytes of the object's ES segment, from offset 0 */
    size_t segment_size;                  /* at most 0x10000 */
    uint16_t segment_id;                  /* the far pointer's segment word */
    uint16_t offset;                      /* the far pointer's offset word */
} pr_u25_e5_object;

typedef struct {
    /* 0xAF8:0x023F GetMem(size): *result is the far pointer (DX:AX). Nonzero return stops with PR_U25_E5_CALLBACK. */
    int (*getmem_af8_023f)(void *ctx, pr_u25_e5_env *env, uint16_t size, pr_far16 *result);
    /* 0x7C:0x3E (unit 0x6f entry 6): (id word, far object). The id word is exactly the 16-bit word the wrapper pushes, (object offset & 0xFF00) | id low byte;
       whether the callee ignores the high byte is not decoded. Nonzero return stops with PR_U25_E5_CALLBACK. */
    int (*init_u6f_0ec4)(void *ctx, pr_u25_e5_env *env, uint16_t id_word, pr_far16 object);
} pr_u25_e5_hooks;

struct pr_u25_e5_env {
    pr_u25_e5_object *objects;            /* the host's table of objects the GetMem hook may return */
    size_t object_count;
    pr_u25_e5_hooks hooks;
    void *ctx;
};

typedef enum {
    PR_U25_E5_OK = 0,
    PR_U25_E5_ARGUMENT,   /* null env or variable pointer, or a null object table with a nonzero count: rejected before any effect */
    PR_U25_E5_HOOK,       /* a reached hook is NULL (report->failed_call); earlier effects stand */
    PR_U25_E5_CALLBACK,   /* a reached hook returned nonzero (report->callback_status); earlier effects stand */
    PR_U25_E5_UNMAPPED,   /* the pointer currently in A is not in the object table (checked when B is read and again when C is read; B may already be written) */
    PR_U25_E5_VIEW        /* the object's view is too small for a field read (report->view_field); B may already be written */
} pr_u25_e5_status;

typedef enum { PR_U25_E5_CALL_NONE = 0, PR_U25_E5_CALL_GETMEM, PR_U25_E5_CALL_INIT } pr_u25_e5_call;

typedef struct {
    pr_u25_e5_call failed_call;
    int callback_status;
    unsigned getmem_called, init_called;
    unsigned view_field;                  /* 0xC8 or 0x7F when VIEW */
} pr_u25_e5_report;

/* `id` is the pushed word's low byte (the caller's high byte is discarded); var_a/var_b/var_c are the three far-pointer variables. `report` may be NULL. */
pr_u25_e5_status pr_u25_entry5_0411(pr_u25_e5_env *env, uint8_t id, pr_far16 *var_a, pr_far16 *var_b, pr_far16 *var_c,
                                    pr_u25_e5_report *report);

#endif
