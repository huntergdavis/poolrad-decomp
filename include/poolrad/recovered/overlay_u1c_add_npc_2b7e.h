#ifndef POOLRAD_RECOVERED_OVERLAY_U1C_ADD_NPC_2B7E_H
#define POOLRAD_RECOVERED_OVERLAY_U1C_ADD_NPC_2B7E_H

/* Bounded model of DOS v1.3 GAME.OVR unit 0x1c +0x2B7E..+0x2C02 (file 0x038C4..0x03948 inclusive, exactly 133 bytes,
 * `retf` last byte, six bytes of locals, no arguments, no result): the handler of ECL opcode 0x36 (ADD NPC, reference
 * name). Evidence: docs/re/U1C_ADD_NPC_2B7E.md.
 *
 * In the original's order (unit offsets; the far record pointer is read live at each use point, as in the code):
 *   loader(2)                                              0x2B:0x2A      (+2B84..+2B87)
 *   v1 := slot value(1), full AX word                      0x2B:0x25      (+2B8C..+2B94)
 *   add_npc(v1)  (AX word; its low byte is the NPC id)     0x7C:0x43      (+2B9A..+2B9B)
 *   v2 := slot value(2), full AX word                      0x2B:0x25      (+2BA0..+2BA3)
 *   use point CLASS:   record[0x84] := ((low(v2) >> 1) | 0x80)
 *                      through the active pointer read at +2BC0 (les di,[0x5D92])
 *   use point FLAG:    record[0x10E] := (low(v1) == 0x18) ? 1 : 0
 *                      through the active pointer read at +2BCF or +2BDB (les di,[0x5D92])
 *   use point RES_0BB8: resident 0xBA:0x0BB8 with the segment:offset pair read at +2BE5/+2BE9
 *   use point RES_0767: resident 0xBA:0x0767 with the segment:offset pair read at +2BF2/+2BF6
 *   return
 *
 * The active pointer is DS:0x5D92 (offset) and DS:0x5D94 (segment). A successful insertion by the add-NPC routine
 * (unit 0x6F entry 7, pr_u6f_1262_add_npc) can select a new active record, so the model never takes the pointer from
 * an entry snapshot: every use point calls read_active, then record_bytes for the resolved segment:offset. A use point
 * that cannot be resolved stops the handler before its store or call; the stores and calls already made are not undone.
 *
 * Classification: bounded model over caller-supplied hooks for the operand loader, the slot value, the add-NPC routine,
 * the live active pointer, the host record bytes, and the two resident routines. Translation only, modeled hooks, not a
 * strict translation. No parity, function-identity or game-function credit is claimed. */

#include <stddef.h>
#include <stdint.h>

#include "poolrad/recovered/ecl_helpers.h"

#define PR_U1C_ADD_NPC_OPERANDS 2u
#define PR_U1C_ADD_NPC_RECORD_SIZE 0x11Du     /* the character record frame (docs/formats/DOS_CHARACTER.md) */
#define PR_U1C_ADD_NPC_OFF_CLASS 0x84u        /* record byte written with ((low(v2) >> 1) | 0x80) */
#define PR_U1C_ADD_NPC_OFF_FLAG 0x10Eu        /* record byte: 1 when the NPC id is 0x18, else 0 */
#define PR_U1C_ADD_NPC_FLAG_ID 0x18u

/* The four use points, in the order the original reads the active pointer. */
#define PR_U1C_ADD_NPC_USE_CLASS 0u
#define PR_U1C_ADD_NPC_USE_FLAG 1u
#define PR_U1C_ADD_NPC_USE_RES_0BB8 2u
#define PR_U1C_ADD_NPC_USE_RES_0767 3u
#define PR_U1C_ADD_NPC_USES 4u

typedef enum {
    PR_U1C_ADD_NPC_OK = 0,
    PR_U1C_ADD_NPC_ARGUMENT,     /* a null hook; nothing called */
    PR_U1C_ADD_NPC_CORE          /* a loader or slot status, or an unresolved use point, ended the handler */
} pr_u1c_add_npc_status;

typedef struct {
    void *context;
    /* 0x2B:0x2A, loader(count): the operands are loaded into the ECL loader state. */
    pr_rec_ecl_loader_status (*load_operands)(void *context, uint8_t count);
    /* 0x2B:0x25, slot value(slot): the value (AX) is written to *value; the status is the window status. */
    pr_rec_ecl_window_status (*slot_value)(void *context, uint8_t slot, uint16_t *value);
    /* 0x7C:0x43, unit 0x6F entry 7 (pr_u6f_1262_add_npc): one word argument, its low byte is the NPC id.
     * It may change the active pointer (a successful insertion selects the new record). */
    void (*add_npc)(void *context, uint16_t word);
    /* Live read of the active far pointer: segment from DS:0x5D94, offset from DS:0x5D92, at the moment of the call.
     * Returns 0 when the pair was read, nonzero when the host cannot read it. Called once per use point. */
    int (*read_active)(void *context, uint16_t *segment, uint16_t *offset);
    /* Host bytes of the far record at segment:offset, at least `size` bytes. NULL when the host cannot resolve that
     * pointer to a record of that size. The model writes stores and passes the pair to residents only after this
     * returns non-NULL. */
    uint8_t *(*record_bytes)(void *context, uint16_t segment, uint16_t offset, size_t size);
    /* 0xBA:0x0BB8 and 0xBA:0x0767: one far pointer argument each (segment pushed first, offset last). */
    void (*resident_0bb8)(void *context, uint16_t segment, uint16_t offset);
    void (*resident_0767)(void *context, uint16_t segment, uint16_t offset);
} pr_u1c_add_npc_hooks;

typedef struct {
    pr_rec_ecl_handler_status core_status;   /* OK unless a loader, slot or use-point status ended the handler */
    pr_rec_ecl_window_status slot_status[2]; /* slot 1 and slot 2 status, OK when not reached */
    uint16_t slot1_word;         /* v1, the full AX word passed to add_npc */
    uint16_t slot2_word;         /* v2, the full AX word of the second slot */
    unsigned npc_called;         /* add_npc was called */
    unsigned residents_called;   /* number of resident calls made (0..2) */
    uint8_t class_byte;          /* the value written at record[0x84] when the class store was made */
    uint8_t flag_byte;           /* the value written at record[0x10E] when the flag store was made */
    unsigned class_written;      /* the class store was made */
    unsigned flag_written;       /* the flag store was made */
    unsigned use_resolved[PR_U1C_ADD_NPC_USES];   /* the use point read and resolved its record */
    uint16_t use_segment[PR_U1C_ADD_NPC_USES];    /* the segment read at each use point (when read) */
    uint16_t use_offset[PR_U1C_ADD_NPC_USES];     /* the offset read at each use point (when read) */
} pr_u1c_add_npc_report;

/* The handler on the caller's hooks. `report` may be NULL. A HOOK status from a slot is reported as core_status HOOK
 * after the handler has completed its writes and calls. */
pr_u1c_add_npc_status pr_u1c_op_add_npc(const pr_u1c_add_npc_hooks *hooks, pr_u1c_add_npc_report *report);

#endif
