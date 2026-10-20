#ifndef POOLRAD_RECOVERED_OVERLAY_U9E_CHARACTER_PAIR_115C_CPU_H
#define POOLRAD_RECOVERED_OVERLAY_U9E_CHARACTER_PAIR_115C_CPU_H
#include "poolrad/recovered/resident_00ba_character_0bb8_cpu.h"
/* U9E [115C,1287), composed with original-entry +10E4/+110D and resident
 * +0BB8. Enter after CALL at IP115C. Frame: result pointer at SP+4,
 * secondary word at SP+8, primary word at SP+10, member pointer at SP+12.
 * RETF12 consumes the live frame. Existing U9E leaf bodies share caller CS.
 * resident_cs must match the immutable far binding at caller CS:1266.
 * CPU/budget outside RAM; immutable code and loaded bindings are preconditions.
 * Exactly 1 MiB A20-off RAM, physically contiguous word/LES operands after
 * 16-bit EA wrapping; RETF pops wrap separately. Logical AF=0.
 * Each caller/resident instruction consumes one budget unit. The two existing
 * finite U9E leaf bodies do not consume this budget. LIMIT preserves partial
 * state at the next unexecuted instruction; no mid-call resume is provided.
 * TRANSFER preserves unexpected callee returns. ARGUMENT rejects invalid
 * host inputs/binding atomically.
 */
pr_res00ba_status pr_u9e_character_pair_115c(uint8_t *ram, size_t ram_size,
                                          pr_u9e_10e4_cpu *cpu,
                                          uint16_t resident_cs, size_t *budget);
#endif
