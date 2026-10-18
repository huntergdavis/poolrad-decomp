#ifndef POOLRAD_RECOVERED_RESIDENT_0709_DELAY_WRAPPER_0DBE_CPU_H
#define POOLRAD_RECOVERED_RESIDENT_0709_DELAY_WRAPPER_0DBE_CPU_H

#include <stddef.h>
#include <stdint.h>

#define PR_R0709_DELAY_CPU_RAM_SIZE (1u << 20)

typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_r0709_delay_cpu;

typedef enum {
    PR_R0709_DELAY_CPU_OK = 0,
    PR_R0709_DELAY_CPU_ARGUMENT,  /* null RAM/CPU, RAM that is not exactly 1 MiB, or IP not 0DBEh; nothing changed */
    PR_R0709_DELAY_CPU_CODE,      /* the +0DCC LCALL is not 9A 9C 02; stopped at it with the count pushed */
    PR_R0709_DELAY_CPU_CALLEE,    /* the delay stopped; *callee_status holds its status */
    PR_R0709_DELAY_CPU_TRANSFER   /* the delay's RETF 2 popped a CS:IP other than +0DD1 in this CS */
} pr_r0709_delay_cpu_status;

/* Raw-CPU, full-state model of START.EXE resident 0709:0DBE..0DD2 (21
 * bytes, 10 instructions), the CPU/stack-effect form of the callback
 * model resident_0709_delay_wrapper_0dbe.h. Entered after a far call; RETF.
 * PUSH BP; MOV BP,SP; MOV AL,DS:[49E2h]; XOR AH,AH; MOV DX,0E1h; MUL DX;
 * PUSH AX; LCALL 0A40:029C, the raw CRT Delay pr_a40_delay_029c_cpu, for
 * AL * 225 milliseconds; POP BP; RETF. The LCALL's opcode and offset are
 * checked at the site, and its segment is read from RAM. MUL leaves DX = 0
 * and CF = OF = 0 (a byte times 225 fits); its architecturally undefined
 * flags are set as Unicorn does (SF, ZF and PF from AX, AF clear) and only
 * show at a CODE stop, since Delay's OR DX,DX overwrites them. Every
 * operand is reread from RAM; POP and the RETF frame are read physically
 * contiguous; offsets wrap at 16 bits, physical addresses at 1 MiB.
 * max_loopbacks is passed to the delay. The code must sit at its original
 * offsets (IP = 0DBEh). *callee_status (may be NULL) receives the delay's
 * status, or -1. */
pr_r0709_delay_cpu_status pr_r0709_delay_wrapper_0dbe_cpu(uint8_t *ram, size_t ram_size, pr_r0709_delay_cpu *cpu,
                                                          uint64_t max_loopbacks, int *callee_status);

#endif
