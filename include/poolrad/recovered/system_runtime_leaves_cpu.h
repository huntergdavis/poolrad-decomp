#ifndef POOLRAD_RECOVERED_SYSTEM_RUNTIME_LEAVES_CPU_H
#define POOLRAD_RECOVERED_SYSTEM_RUNTIME_LEAVES_CPU_H

#include <stddef.h>
#include <stdint.h>

#define PR_SYSTEM_LEAF_RAM_SIZE (1u << 20)

typedef struct {
    uint16_t ax, bx, cx, dx, si, di, bp, sp, cs, ip, ds, es, ss, flags;
} pr_system_leaf_cpu;

typedef enum {
    PR_SYSTEM_LEAF_RETURNED = 0,
    PR_SYSTEM_LEAF_ARGUMENT  /* null RAM/CPU or RAM that is not exactly 1 MiB; nothing changed */
} pr_system_leaf_status;

/* Raw-CPU, full-state models of four Turbo Pascal SYSTEM runtime leaves at
 * loaded segment 0AF8 (START.EXE expanded image). Each is entered after the
 * far CALL with SS:[SP] the return IP and [SP+2] the return CS; arguments
 * follow, read through SS:[BX+disp] with BX := SP. Offsets wrap at 16 bits,
 * physical addresses at 1 MiB; LES/LDS pairs and the RETF frame are read
 * physically contiguous, as Unicorn reads them. No arithmetic flag is
 * written unless stated; AH, the other registers and the other flags keep
 * their entry values unless stated. */

/* FillChar(var X; Count: Word; Value: Byte), 0AF8:16A8..16BB (20 bytes, 7
 * instructions): [SP+4] value byte, [SP+6] count, [SP+8]/[SP+0Ah] X.
 * BX := SP; ES:DI := X; CX := count; AL := value; CLD; REP STOSB writes AL
 * count times forward (DI wraps in ES); RETF 8 reads its frame after the
 * fill, so a fill over the frame changes where it returns. CX = 0, DI is
 * past the fill, DF = 0. */
pr_system_leaf_status pr_system_fillchar_16a8(uint8_t *ram, size_t ram_size, pr_system_leaf_cpu *cpu);

/* Move(var Source, Dest; Count: Word), 0AF8:0A3F..0A56 (24 bytes, 9
 * instructions): [SP+4] count, [SP+6]/[SP+8] Dest, [SP+0Ah]/[SP+0Ch]
 * Source. BX := SP; DX := DS; DS:SI := Source; ES:DI := Dest; CX := count;
 * CLD; REP MOVSB copies forward one byte at a time, so an overlapping
 * destination above the source reads back bytes it has just written;
 * DS := DX; RETF 0Ah. CX = 0, SI and DI are past the copy, DF = 0. */
pr_system_leaf_status pr_system_move_0a3f(uint8_t *ram, size_t ram_size, pr_system_leaf_cpu *cpu);

/* Set membership test (e in S, S a 32-byte set of byte), 0AF8:0DC0..0DE0
 * (33 bytes, 13 instructions): [SP+4]/[SP+6] the set pointer, [SP+8] the
 * element byte e. BX := SP; ES:DI := S; AL := e; BX := e >> 3; CL := e & 7
 * (CH kept); AL := 1 << CL; TEST ES:[BX+DI],AL; RETF 6. The final flags
 * are TEST's: CF = OF = 0, ZF/SF/PF from the masked byte, AF clear as both
 * oracles leave it; the earlier XOR, SHR, AND and SHL writes are folded. */
pr_system_leaf_status pr_system_set_in_0dc0(uint8_t *ram, size_t ram_size, pr_system_leaf_cpu *cpu);

/* UpCase(Ch: Char), 0AF8:182A..183C (19 bytes, 8 instructions): [SP+4]
 * the character. BX := SP; AL := ch; if 61h <= AL <= 7Ah, AL := AL - 20h;
 * RETF 2. The final flags are those of the last CMP or of the SUB. */
pr_system_leaf_status pr_system_upcase_182a(uint8_t *ram, size_t ram_size, pr_system_leaf_cpu *cpu);

#endif
