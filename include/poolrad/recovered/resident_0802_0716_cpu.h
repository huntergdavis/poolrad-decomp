#ifndef POOLRAD_RECOVERED_RESIDENT_0802_0716_CPU_H
#define POOLRAD_RECOVERED_RESIDENT_0802_0716_CPU_H
#include <stddef.h>
#include <stdint.h>
#include "poolrad/recovered/system_blockread_14fb_cpu.h"
#include "poolrad/recovered/system_filepos_1593_cpu.h"
#include "poolrad/recovered/system_seek_1563_cpu.h"

#define PR_R0802_0716_RAM_SIZE (1u << 20)

/* Raw-CPU, full-state model of START.EXE resident 0802:[0716,094A) (564 bytes, 242 instructions, RETF 12h), the game's
 * ECL/DAX block loader (docs/re/RES_0802_0716_CPU.md). Entered after a far call: [SP] IP, [SP+2] CS, [SP+4] the far
 * address of the output block pointer P, [SP+8] the far address of the output unpacked-size word U, [SP+0Ch] the block id
 * (a word slot, read as a byte), [SP+0Eh] the far extension string, [SP+12h] the far name string.
 *
 *   0716 PUSH BP; MOV BP,SP; SUB SP,23Eh
 *   071D name -> [BP-15h], ext -> [BP-2Ah] (StrStore, max 20); [BP-23Dh] := DS:547C + name (string load, concat)
 *   0775 CALL 05CD (open): path, ext, &header length [BP-135h], FileRecs [BP-0B3h] and [BP-133h]   -> stop OPEN
 *   0778 AL = 0: U^ := 0, P^ := nil, return. Otherwise loop over 9-byte directory rows from file [BP-133h]:
 *   0793 BlockRead id (1), offset (4), U^ (2), packed size [BP-2Ch] (2), each followed by IOCheck
 *   080B packed > 8CA0h: Str(packed) into [BP-13Dh], message CS:070C + that, CALL 0193 (prompt) -> stop PROMPT;
 *        084D LCALL 0899:0000 (QUIT)                                                              -> stop QUIT
 *   0852 id != wanted: FilePos (+ IOCheck); loop while position < header length (signed 32-bit compare)
 *   0884 id still != wanted: U^ := 0, P^ := nil, CALL 06E7 (close both)                            -> stop CLOSE
 *   08B3 CALL 0020 (DS:557E := packed buffer of [BP-2Ch] bytes)                                      -> stop ALLOC
 *   08BF P^ := GetMem(U^); Seek(file [BP-0B3h], header + offset) + IOCheck; BlockRead(it, DS:557E^, packed) + IOCheck
 *   091D CALL 06E7 (close both)                                                                      -> stop CLOSE
 *   0935 CALL 050F (decode DS:557E -> P^, &packed, U)                                                -> stop DECODE
 *   0941 CALL 0000 (free DS:557E, packed)                                                            -> stop FREE
 *   0944 MOV SP,BP; POP BP; RETF 12h
 *
 * Composition. Of its 23 far calls, 22 go to SYSTEM models on main (StrStore 0B25, string load 0B0B, concat 0B98, BlockRead
 * 14FB, IOCheck 04A9, Str 1366, FilePos 1593, GetMem 023F, Seek 1563) and one to the credited 0899:0000, so this body's only unknowns are its 7 near-call sites.
 * Each far-call segment word is read live from the code (loader relocations); each callee must return to the
 * instruction after its call, or the stop is CALLEE. The near callees (0802:05CD, 0193, 06E7, 0020, 050F, 0000, all
 * entered by PUSH CS; CALL and returning by RETF) are not yet modelled: each is a NAMED STOP with the call executed (CS
 * and the return IP pushed, IP = the callee), resumable at the return IP with the post-callee state (the two-phase seam
 * of #1311). The resume entries are 0778h, 084Dh, 08B0h, 08BFh, 0920h, 0938h and 0944h.
 *
 * Declared environment: the DOS read service of BlockRead and the LSEEK services of FilePos and Seek, each under its own
 * model's contract (fail-closed AH list, continuation check, permitted writes). GetMem's free-list walk is capped by
 * getmem_steps. Terminal stops: QUIT (the call into the credited 0899:0000 executed; it never returns), RUNERROR
 * (IOCheck or GetMem jumped to 0AF8:00E2) and DIVIDE (BlockRead's recsize-0 INT 0). Exactly 1 MiB of A20-off RAM holding
 * the relocated image (the SYSTEM models need their code at the original offsets); IP 0716h or a resume entry; entry SP
 * in [0300h,FFEAh] at 0716h (the frame is 23Eh bytes below BP). */
typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_r0802_0716_cpu;

typedef struct {
    pr_blockread_dos_service read;   void *read_context;
    pr_filepos_dos_service filepos;  void *filepos_context;
    pr_seek_dos_service seek;        void *seek_context;
    unsigned getmem_steps;
} pr_r0802_0716_env;

typedef enum {
    PR_R0802_0716_RETURNED = 0,  /* RETF 12h executed */
    PR_R0802_0716_ARGUMENT,      /* null pointers, RAM size, IP not an entry, entry SP out of range; nothing changed */
    PR_R0802_0716_OPEN,          /* CALL 05CD executed (resume at 0778h) */
    PR_R0802_0716_PROMPT,        /* CALL 0193 executed (resume at 084Dh) */
    PR_R0802_0716_CLOSE,         /* CALL 06E7 executed (resume at 08B0h or 0920h) */
    PR_R0802_0716_ALLOC,         /* CALL 0020 executed (resume at 08BFh) */
    PR_R0802_0716_DECODE,        /* CALL 050F executed (resume at 0938h) */
    PR_R0802_0716_FREE,          /* CALL 0000 executed (resume at 0944h) */
    PR_R0802_0716_QUIT,          /* TERMINAL: LCALL 0899:0000 executed; CS:IP = the live target */
    PR_R0802_0716_RUNERROR,      /* TERMINAL: IOCheck or GetMem at 0AF8:00E2 (RunError), AX the error */
    PR_R0802_0716_DIVIDE,        /* TERMINAL: BlockRead's DIV by recsize 0 (INT 0), CPU at the DIV */
    PR_R0802_0716_SERVICE,       /* a declared DOS service declined or broke its continuation; CPU at that INT */
    PR_R0802_0716_CALLEE         /* a composed model stopped otherwise or returned elsewhere (see the diagnostic) */
} pr_r0802_0716_status;

typedef struct {
    uint16_t site;      /* IP of the call this stop or CALLEE concerns (0 when none) */
    int callee_status;  /* the composed model's own status (-1 when none) */
} pr_r0802_0716_diagnostic;

pr_r0802_0716_status pr_r0802_0716(uint8_t *ram, size_t ram_size, pr_r0802_0716_cpu *cpu, const pr_r0802_0716_env *env,
                                   pr_r0802_0716_diagnostic *diagnostic);
#endif
