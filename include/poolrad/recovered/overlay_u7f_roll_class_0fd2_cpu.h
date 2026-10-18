#ifndef POOLRAD_RECOVERED_OVERLAY_U7F_ROLL_CLASS_0FD2_CPU_H
#define POOLRAD_RECOVERED_OVERLAY_U7F_ROLL_CLASS_0FD2_CPU_H

#include <stddef.h>
#include <stdint.h>

#define PR_U7F_0FD2_RAM_SIZE (1u << 20)

typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_u7f_0fd2_raw_cpu;

typedef enum {
    PR_U7F_0FD2_RAW_OK = 0,
    PR_U7F_0FD2_RAW_ARGUMENT,  /* null RAM/CPU or RAM that is not exactly 1 MiB; nothing changed */
    PR_U7F_0FD2_RAW_CODE,      /* the reached LCALL, or the 00B0:0048 stub it names, is not in its loaded form; IP at the site */
    PR_U7F_0FD2_RAW_CALLEE,    /* the composed U9E +0DE3 chain stopped; *callee_status holds its status */
    PR_U7F_0FD2_RAW_TRANSFER   /* the chain returned to a CS:IP other than the call's next instruction */
} pr_u7f_0fd2_raw_status;

/* Raw-CPU, full-state model of the complete GAME.OVR unit 0x7F routine
 * +0FD2..+100F (62 bytes, 27 instructions), entered after a far call with
 * SS:[SP] return IP and [SP+2] return CS; RETF. It is the raw-CPU
 * counterpart of the hook-composed pr_u7f_roll_composed_0fd2. After PUSH
 * BP; MOV BP,SP; SUB SP,2: roll(1, 14h) (MOV AL,1; PUSH AX; MOV AL,14h;
 * PUSH AX; LCALL 00B0:0048, the loaded stub's EA jump reaching U9E +0DE3,
 * run by the merged raw-RAM pr_u9e_entry8_fullstate_compose_0de3 with
 * max_system_calls); r := AL into [BP-2]; class := 1 when 1 <= r <= 0Eh,
 * 2 when 0Fh <= r <= 13h, else 3, into [BP-1]; AL := class; MOV SP,BP;
 * POP BP; RETF. Every operand is reread from RAM in source order. Flags:
 * the final flags are the last CMP's (r:0Eh for class 1, r:0Fh for r = 0,
 * r:13h otherwise); at a CODE stop they are SUB SP,2's. The site's segment
 * word and the stub's EA target are read from RAM. Offsets wrap at 16
 * bits, physical addresses at 1 MiB; POP and this routine's RETF frame are
 * read physically contiguous, and the chain keeps its own contract.
 * *callee_status (may be NULL) receives the chain's status, or -1 when it
 * did not run. */
pr_u7f_0fd2_raw_status pr_u7f_roll_class_0fd2_cpu(uint8_t *ram, size_t ram_size, pr_u7f_0fd2_raw_cpu *cpu, unsigned max_system_calls,
                                                  int *callee_status);

#endif
