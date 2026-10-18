#ifndef POOLRAD_RECOVERED_RESIDENT_0709_SKIP_SPACES_0714_CPU_H
#define POOLRAD_RECOVERED_RESIDENT_0709_SKIP_SPACES_0714_CPU_H

#include <stddef.h>
#include <stdint.h>

#define PR_R0709_SKIP_RAM_SIZE (1u << 20)

typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_r0709_skip_cpu;

typedef enum {
    PR_R0709_SKIP_OK = 0,
    PR_R0709_SKIP_ARGUMENT,  /* null RAM/CPU, RAM that is not exactly 1 MiB, or IP not 0714h; nothing changed */
    PR_R0709_SKIP_LIMIT      /* max_loopbacks JMP 0717 loop-backs were taken; the next stops at +0717 */
} pr_r0709_skip_status;

/* Raw-CPU, full-state model of START.EXE resident 0709:0714..074D (58 bytes,
 * 19 instructions, call-free and port-free), a nested helper of the RES
 * 00BA:16EC closure's 0709:0768. Entered after a far call with the parent's
 * frame pointer P at [SP+4]; RETF 2. After PUSH BP; MOV BP,SP, with every
 * P reread from SS:[BP+6]:
 *  1. MOV AL,SS:[P-103h] (the index); CMP AL,SS:[P-101h] (the limit); JAE
 *     returns.
 *  2. MOV AL,SS:[P-103h]; XOR AH,AH; DI := P + AX; CMP BYTE SS:[DI-100h],20h;
 *     JNE returns.
 *  3. INC BYTE SS:[P-103h]; JMP 0717.
 *  4. POP BP; RETF 2.
 * On return the flags are the last CMP's, AL is the index read last (AH is
 * zero once a character was tested) and DI is P or P + index. Every operand
 * is reread from RAM in source order; offsets wrap at 16 bits, physical
 * addresses at 1 MiB; POP and the RETF frame are read physically contiguous.
 * The INC can alias the stack word that holds P, so max_loopbacks caps the
 * taken JMP 0717 (at most the limit minus the index when nothing aliases).
 * The code must sit at its original offsets (IP = 0714h). */
pr_r0709_skip_status pr_r0709_skip_spaces_0714_cpu(uint8_t *ram, size_t ram_size, pr_r0709_skip_cpu *cpu, unsigned max_loopbacks);

#endif
