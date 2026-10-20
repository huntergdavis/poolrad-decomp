#ifndef POOLRAD_RECOVERED_OVERLAY_U33_PARTY_ATTACH_12E1_CPU_H
#define POOLRAD_RECOVERED_OVERLAY_U33_PARTY_ATTACH_12E1_CPU_H
#include "poolrad/recovered/resident_00ba_character_0bb8_cpu.h"
typedef pr_u9e_10e4_cpu pr_u33_12e1_cpu;
typedef enum {
    PR_U33_12E1_RETURNED = 0,
    PR_U33_12E1_ARGUMENT,
    PR_U33_12E1_LIMIT,
    PR_U33_12E1_CALLEE,
    PR_U33_12E1_TRANSFER
} pr_u33_12e1_status;
/* Original U33 [12E1,1403), post-CALL entry IP12E1; no arguments, RETF.
 * Exactly 1 MiB A20-off RAM. CPU, budget and diagnostic storage are outside
 * RAM and mutually distinct. Immutable loaded code/bindings are required.
 * Entry SP must be [0100,FFF0]. The composed GetMem/FillChar profile requires
 * a well-formed heap and active stack frames that remain unwrapped and are
 * not corrupted by party/heap/output writes; this is not an arbitrary-alias
 * contract. Operands retain source order and live memory rereads.
 * Caller/resident instructions each consume one shared budget unit;
 * finite existing GetMem/FillChar bodies are uncharged. max_heap_steps
 * separately caps each GetMem free-list walk. LIMIT leaves live partial
 * state before the next instruction; invocation is entry-only, no resume.
 * CALLEE preserves GetMem failures/stops; TRANSFER preserves unexpected
 * returns. getmem_status is optional: -1 if no GetMem ran, otherwise its
 * last exact pr_system_getmem_status. Invalid host inputs or any of the
 * three immutable far bindings return ARGUMENT without changing outputs.
 * Word/LES operands are contiguous after 16-bit EA wrapping. Caller and
 * resident RETF pop IP/CS separately; existing callees retain their
 * documented contiguous-return behavior within the nonwrapping profile.
 * Logical AF=0; SHR uses DOSBox-X normal/386 AF=1. IDIV's undefined flags
 * follow normal/386: AF/SF/OF clear, ZF for zero remainder and odd
 * quotient, CF for remainder low two bits 1 or 2, and PF from the
 * combined word parity of remainder and quotient; no flags are masked.
 */
pr_u33_12e1_status pr_u33_party_attach_12e1_cpu(
    uint8_t *ram, size_t ram_size, pr_u33_12e1_cpu *cpu,
    uint16_t resident_cs, uint16_t system_cs, size_t *budget,
    unsigned max_heap_steps, int *getmem_status);
#endif
