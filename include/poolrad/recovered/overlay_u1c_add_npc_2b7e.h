#ifndef POOLRAD_RECOVERED_OVERLAY_U1C_ADD_NPC_2B7E_H
#define POOLRAD_RECOVERED_OVERLAY_U1C_ADD_NPC_2B7E_H

/* Bounded model of DOS v1.3 GAME.OVR unit 0x1c +0x2B7E..+0x2C02 (file 0x38C4..0x3946, exactly 133 bytes,
 * `retf` last byte, six bytes of locals, no arguments, no result): the handler of ECL opcode 0x36 (ADD NPC,
 * reference name). Evidence: docs/re/U1C_ADD_NPC_2B7E.md.
 *
 * In the original's order (instruction offsets in the unit):
 *   loader(2)                                       0x2B:0x2A   (+0x2B84..+0x2B87)
 *   v1 := slot value(1), full AX word               0x2B:0x25   (+0x2B8C..+0x2B94)
 *   add_npc(v1)  (the word is AX, whose low byte is the NPC id)  0x7C:0x43   (+0x2B9A..+0x2B9B)
 *   v2 := slot value(2), full AX word               0x2B:0x25   (+0x2BA0..+0x2BA3)
 *   record[0x84] := ((low(v2) >> 1) | 0x80)         via far pointer [0x5D92]:[0x5D94] (+0x2BAB..+0x2BC4)
 *   record[0x10E] := (low(v1) == 0x18) ? 1 : 0       (+0x2BC9..+0x2BE2)
 *   resident 0xBA:0x0BB8 with the far pointer       (+0x2BE5..+0x2BED)
 *   resident 0xBA:0x0767 with the far pointer       (+0x2BF2..+0x2BFA)
 *   return
 *
 * Classification: bounded model over caller-supplied callbacks for the operand loader, the slot value, the
 * add-NPC routine and the two resident routines, plus a caller-supplied record buffer for the far record.
 * Not a strict translation. No game-function credit is claimed. The callees have their own evidence
 * documents; this file only states their calls and arguments. */

#include <stddef.h>
#include <stdint.h>

#include "poolrad/recovered/ecl_helpers.h"

#define PR_U1C_ADD_NPC_OPERANDS 2u
#define PR_U1C_ADD_NPC_RECORD_SIZE 0x11Du     /* the character record frame (docs/formats/DOS_CHARACTER.md) */
#define PR_U1C_ADD_NPC_OFF_CLASS 0x84u        /* record byte written with ((low(v2) >> 1) | 0x80) */
#define PR_U1C_ADD_NPC_OFF_FLAG 0x10Eu        /* record byte: 1 when the NPC id is 0x18, else 0 */
#define PR_U1C_ADD_NPC_FLAG_ID 0x18u

typedef enum {
    PR_U1C_ADD_NPC_OK = 0,
    PR_U1C_ADD_NPC_ARGUMENT,     /* null hook, null record, or a record smaller than the frame; nothing called */
    PR_U1C_ADD_NPC_CORE          /* a loader or slot status ended the handler (report->core_status) */
} pr_u1c_add_npc_status;

typedef struct {
    void *context;
    /* 0x2B:0x2A, loader(count): the operands are loaded into the ECL loader state. */
    pr_rec_ecl_loader_status (*load_operands)(void *context, uint8_t count);
    /* 0x2B:0x25, slot value(slot): the value (AX) is written to *value; the status is the window status. */
    pr_rec_ecl_window_status (*slot_value)(void *context, uint8_t slot, uint16_t *value);
    /* 0x7C:0x43, unit 0x6F entry 7 (pr_u6f_1262_add_npc): one word argument, its low byte is the NPC id. */
    void (*add_npc)(void *context, uint16_t word);
    /* 0xBA:0x0BB8 and 0xBA:0x0767: one far pointer argument each (segment pushed first, offset last). */
    void (*resident_0bb8)(void *context, uint16_t seg, uint16_t off);
    void (*resident_0767)(void *context, uint16_t seg, uint16_t off);
} pr_u1c_add_npc_hooks;

typedef struct {
    uint8_t *record;             /* the far record, supplied as a host buffer of at least record_size bytes */
    size_t record_size;          /* must be at least PR_U1C_ADD_NPC_RECORD_SIZE */
    uint16_t record_off;         /* the offset held at DS:0x5D92 when the handler is entered */
    uint16_t record_seg;         /* the segment held at DS:0x5D94 when the handler is entered */
} pr_u1c_add_npc_record;

typedef struct {
    pr_rec_ecl_handler_status core_status;   /* OK unless a loader or slot status ended the handler */
    pr_rec_ecl_window_status slot_status[2]; /* slot 1 and slot 2 status, OK when not reached */
    uint16_t slot1_word;         /* v1, the full AX word passed to add_npc */
    uint16_t slot2_word;         /* v2, the full AX word of the second slot */
    unsigned npc_called;         /* add_npc was called */
    unsigned residents_called;   /* both resident calls were made */
    uint8_t class_byte;          /* the value written at record[0x84] when the store was reached */
    uint8_t flag_byte;           /* the value written at record[0x10E] when the store was reached */
} pr_u1c_add_npc_report;

/* The handler on the caller's hooks and record. `report` may be NULL. A HOOK status from a slot is reported
 * as core_status HOOK after the handler has completed its writes and calls. */
pr_u1c_add_npc_status pr_u1c_op_add_npc(const pr_u1c_add_npc_hooks *hooks,
                                        const pr_u1c_add_npc_record *rec,
                                        pr_u1c_add_npc_report *report);

#endif
