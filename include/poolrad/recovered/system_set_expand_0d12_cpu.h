#ifndef POOLRAD_RECOVERED_SYSTEM_SET_EXPAND_0D12_CPU_H
#define POOLRAD_RECOVERED_SYSTEM_SET_EXPAND_0D12_CPU_H

#include "poolrad/recovered/system_runtime_leaves_cpu.h"

/* Raw-CPU, full-state model of the Turbo Pascal SYSTEM set expansion at
 * START.EXE 0AF8:0D12..0D3B (42 bytes, 18 instructions), a pure callee of
 * the RES 00BA:16EC closure's 0709:0768. Entered after a far call with
 * [SP+4] the count word (BL data bytes, BH leading zero bytes), [SP+6] the
 * packed source and [SP+0Ah] the 32-byte destination; RETF 6. CLD; MOV
 * BX,SP; MOV DX,DS; LES DI and LDS SI from the frame (pairs read physically
 * contiguous); BX := the count; CX := BH (XOR CH,CH); XOR AL,AL; REP STOSB;
 * CL := BL; REP MOVSB; CL := 20h - BL - BH (SUB, SUB: the final flags; the
 * byte wraps when BL + BH exceeds 32, so up to 255 zero bytes are stored);
 * REP STOSB; MOV DS,DX; RETF 6. REP steps one byte at a time and rereads
 * each source byte, so an overlapping source behaves as on the CPU; SI and
 * DI wrap at 16 bits, physical addresses at 1 MiB. On return AL = 0 (AH
 * kept), BX = the count, CX = 0, DX = DS, DF clear. Null RAM or CPU, or
 * RAM that is not exactly 1 MiB, returns ARGUMENT before any change;
 * otherwise RETURNED with CS:IP at the popped return address. */
pr_system_leaf_status pr_system_set_expand_0d12(uint8_t *ram, size_t ram_size, pr_system_leaf_cpu *cpu);

#endif
