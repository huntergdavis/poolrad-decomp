#ifndef POOLRAD_RECOVERED_SYSTEM_TEXTASSIGN_INSERT_CPU_H
#define POOLRAD_RECOVERED_SYSTEM_TEXTASSIGN_INSERT_CPU_H

#include "poolrad/recovered/system_runtime_leaves_cpu.h"

typedef enum {
    PR_SYSTEM_INSERT_RETURNED = 0,
    PR_SYSTEM_INSERT_ARGUMENT,  /* null RAM/CPU, RAM that is not exactly 1 MiB, or IP not 0C68h; nothing changed */
    PR_SYSTEM_INSERT_TRANSFER   /* a callee's RETF popped a CS:IP other than the call's next instruction in this CS */
} pr_system_insert_status;

/* Raw-CPU, full-state models of two Turbo Pascal SYSTEM routines in the
 * START.EXE SYSTEM segment (0AF8 at load segment 0), each entered after a
 * far call (SS:[SP] return IP, [SP+2] return CS). Pointer pairs (LES/LDS),
 * word stores, POP and the RETF frames are read or written physically
 * contiguous; offsets wrap at 16 bits, physical addresses at 1 MiB.
 *
 * Text Assign 04F7..053B (69 bytes, 37 instructions): [SP+4] the name S,
 * [SP+8] the text record T (both far); RETF 8. MOV BX,SP; PUSH DS; CLD; T's
 * words := 0 (handle), D7B0h (mode), 80h (buffer size), 0, 0, 0, then its
 * buffer pointer := (T+80h, T's segment) as LEA AX,[DI+74h] computes it, its
 * open routine := CS:060E, 14 more words := 0, then min(length(S), 79)
 * characters of S at T+48 and a NUL byte. AX = 0, CX = 0 and the arithmetic
 * flags are XOR AL,AL's; DS returns through the stack (reread). Null RAM or
 * CPU, or RAM that is not exactly 1 MiB, returns ARGUMENT before any change.
 *
 * Insert 0C68..0CBB (84 bytes, 43 instructions): [SP+4] Index, [SP+6] the
 * maximum length M of S, [SP+8] S, [SP+0Ch] Source (far); RETF 0Ch. After
 * PUSH BP; MOV BP,SP; SUB SP,200h, with T1 = SS:BP-100h and T2 = SS:BP-200h,
 * its five near calls (PUSH CS; CALL) run the merged raw-CPU leaves: T1 :=
 * Copy(S, 1, Index - 1); T1 := T1 + Source; T2 := Copy(S, Index, 255);
 * T1 := T1 + T2; S := T1 (StrStore with M). MOV SP,BP; POP BP; RETF 0Ch.
 * The registers and flags at return are StrStore's. The code must sit at
 * its original offsets, hence IP = 0C68h; the leaves cannot fail once the
 * arguments are valid, so a stop is only TRANSFER. */
pr_system_leaf_status pr_system_text_assign_04f7(uint8_t *ram, size_t ram_size, pr_system_leaf_cpu *cpu);
pr_system_insert_status pr_system_insert_0c68(uint8_t *ram, size_t ram_size, pr_system_leaf_cpu *cpu);

#endif
