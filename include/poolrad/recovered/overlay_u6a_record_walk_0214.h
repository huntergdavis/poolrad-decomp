#ifndef POOLRAD_RECOVERED_OVERLAY_U6A_RECORD_WALK_0214_H
#define POOLRAD_RECOVERED_OVERLAY_U6A_RECORD_WALK_0214_H

#include <stddef.h>
#include <stdint.h>

#define PR_U6A_0214_RAM_SIZE (1u << 20)

/* 14-word raw CPU state shared with the +0159 and +019C models. */
typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_u6a_0214_cpu;

typedef enum {
    PR_U6A_0214_OK = 0,
    PR_U6A_0214_ARGUMENT,   /* null RAM/CPU or RAM that is not exactly 1 MiB; nothing changed */
    PR_U6A_0214_CALLEE,     /* a callee model rejected its entry; its status is in *callee_status */
    PR_U6A_0214_TRANSFER,   /* a callee's RETF did not return to its call frame */
    PR_U6A_0214_LIMIT       /* the host cap on records was reached; CS:IP = entry + 13h (+0227) */
} pr_u6a_0214_status;

/* Complete GAME.OVR unit 0x6A:+0214..+025B (72 bytes, 27 instructions,
 * one conditional branch), entered after the far CALL with SS:[SP] return
 * IP and [SP+2] return CS and no arguments; RETF. It copies the far
 * pointer DS:5D96/5D98 into its frame and, while the pointer is not null,
 * calls +0159 (clear the record's high item bytes) and +019C (clear the
 * high bytes of its linked nodes) with the pointer as the argument and
 * then follows the record's far link at +104h/+106h. The final flags are
 * OR AX,[BP-2]'s on the null pointer; AX returns 0, DX and ES:DI as the
 * last link load left them.
 *
 * Both near calls push CS and the return offset relative to the entry
 * cpu->ip (0x0214 in the game) and enter the merged models at ip - BBh
 * (+0159) and ip - 78h (+019C, which reaches unit 0x86 +323D through its
 * loaded stub) on the same RAM and CPU words. Every operand is reread
 * from RAM; offsets wrap at 16 bits, physical addresses at 1 MiB; LES and
 * the RETF frame are read physically contiguous. A cyclic record list
 * never returns in DOS, so the host caps the records walked at max_records:
 * the next would stop before +0227 with every earlier effect kept (LIMIT).
 * A cyclic node list inside +019C is that model's contract and is not
 * capped here. */
pr_u6a_0214_status pr_u6a_record_walk_0214(
    uint8_t *ram, size_t ram_size, pr_u6a_0214_cpu *cpu, unsigned max_records, int *callee_status);

#endif
