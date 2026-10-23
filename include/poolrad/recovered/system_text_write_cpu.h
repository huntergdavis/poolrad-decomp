#ifndef POOLRAD_RECOVERED_SYSTEM_TEXT_WRITE_CPU_H
#define POOLRAD_RECOVERED_SYSTEM_TEXT_WRITE_CPU_H

#include <stddef.h>
#include <stdint.h>

#define PR_SYSTEM_TEXT_RAM_SIZE (1u << 20)

typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_system_text_cpu;

/* A Text file's driver function (TextRec InOutFunc at +14h or FlushFunc at +18h), a DECLARED environment input. The
 * service receives RAM and the CPU at the driver's entry (CS:IP = the far pointer from the TextRec; SP at the far return
 * address, with the TextRec far pointer at [SP+4]) and returns the complete CPU at the continuation: the SYSTEM CS, IP
 * 07FFh (InOutFunc) or 0890h (FlushFunc), the same SS and SP = entry SP + 8 (RETF 4), with AX the driver's result.
 * Returns 0 with *after filled, or nonzero to refuse. */
typedef int (*pr_system_text_driver)(void *context, uint8_t *ram, const pr_system_text_cpu *before, pr_system_text_cpu *after);

typedef struct {
    pr_system_text_driver driver;
    void *context;
} pr_system_text_env;

typedef enum {
    PR_SYSTEM_TEXT_RETURNED = 0,
    PR_SYSTEM_TEXT_ARGUMENT,  /* null pointers or RAM not exactly 1 MiB; nothing changed */
    PR_SYSTEM_TEXT_SERVICE    /* a driver call refused or did not return to its continuation; stopped at the driver entry */
} pr_system_text_status;

/* Raw-CPU, full-state models of the Turbo Pascal SYSTEM text output in the START.EXE SYSTEM segment (0AF8 at load
 * segment 0), each entered after a far call with SS:[SP] the return IP and [SP+2] the return CS; both run their near
 * helpers 07B8 (mode check, 36 bytes), 07DC (put character, 64 bytes) and 087B (flush, 29 bytes) with every near-call
 * return word written to the stack as on the CPU.
 *
 * Write(var f: Text; s: string; width) 0919..0956 (62 bytes, 32 instructions): [SP+4] width, [SP+6] the string, [SP+0Ah]
 * the TextRec; RETF 6 leaves the TextRec pointer on the stack. The mode check passes when InOutRes DS:[4426] is 0 and
 * Mode (+2) is D7B2h (fmOutput); a wrong mode sets InOutRes := 69h. On a pass the string is written right-aligned in
 * `width`: (width - length) spaces when that is positive, then its bytes. BufPos (+8) is stored back either way.
 *
 * Writeln(var f: Text) 084A..086D (36 bytes, 17 instructions): [SP+4] the TextRec; on a pass CR and LF are put; BufPos
 * is stored back, then the flush runs; RETF 4.
 *
 * Put character stores AL at BufPtr (+0Ch) + BufPos and advances BufPos; when it reaches BufSize (+4) it saves CX, SI
 * and DS, loads DS from the relocated word at CS:07EF (the SYSTEM data segment), stores BufPos and calls InOutFunc
 * (+14h); a nonzero AX becomes InOutRes; it then reloads AX = BufEnd (+0Ah), BX = BufPos, DX = BufSize and ES:DI =
 * BufPtr. The flush calls FlushFunc (+18h) only when its segment word (+1Ah) is nonzero and InOutRes is 0; a nonzero AX
 * becomes InOutRes. Every operand is reread from RAM in source order; LES/LDS pairs, POP and the RETF frame are read
 * physically contiguous; offsets wrap at 16 bits, physical addresses at 1 MiB. Flags follow every instruction (XOR AH,AH
 * and OR AX,AX clear AF; INC keeps CF; LOOP and the moves leave flags). CLD clears DF on Write's string path only.
 * *service_status (may be NULL) receives the last driver result's refusal (1) or 0, -1 when none was called. */
pr_system_text_status pr_system_write_str_0919(uint8_t *ram, size_t ram_size, pr_system_text_cpu *cpu,
                                               const pr_system_text_env *env, int *service_status);
pr_system_text_status pr_system_writeln_084a(uint8_t *ram, size_t ram_size, pr_system_text_cpu *cpu,
                                             const pr_system_text_env *env, int *service_status);

#endif
