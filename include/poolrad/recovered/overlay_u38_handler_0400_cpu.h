#ifndef POOLRAD_RECOVERED_OVERLAY_U38_HANDLER_0400_CPU_H
#define POOLRAD_RECOVERED_OVERLAY_U38_HANDLER_0400_CPU_H
#include <stddef.h>
#include <stdint.h>
#include "poolrad/recovered/resident_00ba_party_count_23db_cpu.h"

/* Raw-CPU, full-state model of GAME.OVR unit 38h +[0400,04BA) (186 bytes, 59 instructions, RETF 0Ah), the handler that
 * the 009E dispatcher reaches through slot 11 (stub 0041:0057). Arguments: [BP+6] a byte selector, [BP+8] a far pointer A,
 * [BP+0Ch] a far pointer B.
 *   [BP+6] non-zero: B+10Eh := (A+3 AND 40h) >> 6; when B+84h = B3h, B+84h := 0. Return.
 *   [BP+6] zero, A+3 bit 5 set: DS:6825 := 64h. Return.
 *   [BP+6] zero, A+3 bit 5 clear: A+3 := low byte of (A+3 + 20h + (CBW(B+10Eh) << 6)); B+10Eh := A+3 >> 7; B+10Fh := 1;
 *     when B+84h <= 7Fh (unsigned), B+84h := B3h; the far pointer at B+108h is followed and its words +0Ah/+0Ch := 0;
 *     then 00BA:23DB (the party count) through its merged model; then DS:6825 := 64h.
 * Every far pointer is reread with LES at each use, in source order. Entered after a far call: [SP] IP, [SP+2] CS, then
 * the five argument words; RETF 0Ah. CS is the unit's load segment; the far-call segment word CS:04AF is a loader
 * fixup, read live. Flat 1 MiB A20-off RAM: offsets wrap at 16 bits, physical addresses at 1 MiB, word operands are
 * physically contiguous. See docs/re/OVL_0038_0400_CPU.md for the contract and evidence. */
typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_u38_0400_cpu;

typedef enum {
    PR_U38_0400_OK = 0,         /* RETF 0Ah executed */
    PR_U38_0400_ARGUMENT,       /* null pointers, RAM not exactly 1 MiB, IP not 0400h; nothing changed */
    PR_U38_0400_CALLEE,         /* 23DB stopped (its status in *callee_status); state as it left it */
    PR_U38_0400_TRANSFER        /* 23DB returned somewhere other than the instruction after its call */
} pr_u38_0400_status;

/* party_budget: 23DB's instruction budget (its cap on a cyclic party list). callee_status (optional) receives -1 when 23DB
 * did not run, otherwise its last status. */
pr_u38_0400_status pr_u38_handler_0400_cpu(uint8_t *ram, size_t ram_size, pr_u38_0400_cpu *cpu, size_t party_budget,
                                           int *callee_status);
#endif
