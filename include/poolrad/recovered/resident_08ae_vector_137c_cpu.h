#ifndef POOLRAD_RECOVERED_RESIDENT_08AE_VECTOR_137C_CPU_H
#define POOLRAD_RECOVERED_RESIDENT_08AE_VECTOR_137C_CPU_H
#include <stddef.h>
#include <stdint.h>
#define PR_R08AE_137C_RAM_SIZE (1u << 20)
typedef struct {
    uint16_t ax, bx, cx, dx, si, di, bp, sp, cs, ip, ds, es, ss, flags;
} pr_r08ae_137c_cpu;
typedef enum {
    PR_R08AE_137C_OK = 0, PR_R08AE_137C_ARGUMENT, PR_R08AE_137C_ENV_DOS,
    PR_R08AE_137C_ENV_PORT, PR_R08AE_137C_STOP_1388, PR_R08AE_137C_STOP_139E, PR_R08AE_137C_STOP_13A0, PR_R08AE_137C_STOP_13A3, PR_R08AE_137C_STOP_13A5, PR_R08AE_137C_STOP_13AA, PR_R08AE_137C_STOP_13AE
} pr_r08ae_137c_status;
/* Declared environment. Neither hook is the real DOS or PIT.
 *
 * dos: called for each `INT 21h` after the hardware frame (FLAGS, CS, next IP) is
 * pushed. It performs the documented DOS vector-table service ONLY (AH=35h loads
 * ES:BX, AH=25h stores DX:DS, both indexed by AL) over a table the environment owns,
 * and reports the flags the handler returns with. It must refuse (nonzero, changing
 * nothing) any other AH. The model then returns as DOS does, `RETF 2`: it pops IP
 * and CS, drops the pushed FLAGS word and loads *flags_out.
 * port_out: called before each `OUT`; nonzero refuses that port. */
typedef struct {
    void *context;
    int (*dos)(void *context, uint8_t *ram, pr_r08ae_137c_cpu *cpu, uint16_t *flags_out);
    int (*port_out)(void *context, uint16_t port, uint8_t value);
} pr_r08ae_137c_env;
/* Original [137C,13B1), RETF, 53 bytes / 24 instructions, straight-line. Saves the
 * current INT 08h vector at CS:1378 (offset) and CS:137A (segment) -- FOUR BYTES
 * BEFORE THE ENTRY, outside the span -- installs DS:DX = (relocated seg):13EA with
 * INT 21h AH=25h, then writes OUT 43h<-B6h and OUT 40h<-B1h,13h. The two segment
 * immediates at CS:137F and CS:1394 are loader relocations and are read live.
 * Exactly 1 MiB A20-off ordinary RAM; IP=137C, SP in [000C,FFFC]. The span is
 * immutable and the stack (all pushes, including the two INT frames) must not overlap
 * CS:1378..13B0. Besides the stack the body writes only CS:1378..137B, and the
 * environment writes only its own vector table. No MMIO, interrupts, faults or
 * arbitrary-IP resume. ENV_DOS stops after the frame push with CS:IP at the INT;
 * ENV_PORT stops before the OUT with nothing executed. ARGUMENT preserves CPU/RAM. */
pr_r08ae_137c_status pr_r08ae_vector_137c_cpu(
    uint8_t *ram, size_t ram_size, pr_r08ae_137c_cpu *cpu, const pr_r08ae_137c_env *env);
/* Diagnostic entry at the observed pre-INT state, stopping before 1388. This
 * explicitly supplied DOS callback may implement the narrow captured handler
 * effects, including RAM writes; it must validate its own input and refuse
 * without mutation when unsupported. On refusal the hardware frame remains
 * pushed, as with the normal entry. The caller supplies the captured CPU and
 * RAM and is responsible for their provenance and valid stack. */
pr_r08ae_137c_status pr_r08ae_vector_137c_from_1386(
    uint8_t *ram, size_t ram_size, pr_r08ae_137c_cpu *cpu, const pr_r08ae_137c_env *env);
/* Resume at 1388 and stop before the second INT at 139E. The stack and
 * immutable resident span obey the 137C contract. ARGUMENT preserves state. */
pr_r08ae_137c_status pr_r08ae_vector_137c_from_1388_to_139e(
    uint8_t *ram, size_t ram_size, pr_r08ae_137c_cpu *cpu);
/* Diagnostic second INT entry. An explicitly supplied callback may model
 * bounded handler RAM effects, validating input and refusing unsupported
 * states without mutation. Success stops before 13A0; refusal retains the
 * hardware frame. The caller is responsible for capture provenance. */
pr_r08ae_137c_status pr_r08ae_vector_137c_from_139e(
    uint8_t *ram, size_t ram_size, pr_r08ae_137c_cpu *cpu, const pr_r08ae_137c_env *env);
/* Resume at 13A0 and stop before the first port output at 13A3. The stack
 * and immutable resident span obey the 137C contract. ARGUMENT preserves state. */
pr_r08ae_137c_status pr_r08ae_vector_137c_from_13a0_to_13a3(
    uint8_t *ram, size_t ram_size, pr_r08ae_137c_cpu *cpu);
/* Diagnostic 137C entry: identical control flow to the normal entry, but
 * explicitly permits a supplied DOS callback to model bounded handler RAM
 * effects. The callback validates its input and refuses unsupported states
 * without mutation. Port effects still require an explicit port callback;
 * absent callbacks refuse. The caller establishes provenance and memory bounds. */
pr_r08ae_137c_status pr_r08ae_vector_137c_cpu_diagnostic(
    uint8_t *ram, size_t ram_size, pr_r08ae_137c_cpu *cpu, const pr_r08ae_137c_env *env);
/* Resume at 13A3 for the first OUT only, stopping at 13A5. An absent or
 * refusing port callback stops at 13A3. The callback owns any device effects;
 * the CPU model itself makes no PIT-state claim. ARGUMENT preserves state. */
pr_r08ae_137c_status pr_r08ae_vector_137c_first_port(
    uint8_t *ram, size_t ram_size, pr_r08ae_137c_cpu *cpu, const pr_r08ae_137c_env *env);
/* Resume at 13A5, execute MOV AX,13B1 and the second OUT only, and stop at
 * 13AA. A missing/refusing port callback stops before the OUT at 13A8, after
 * MOV. Device effects belong to the callback; ARGUMENT preserves state. */
pr_r08ae_137c_status pr_r08ae_vector_137c_second_port(
    uint8_t *ram, size_t ram_size, pr_r08ae_137c_cpu *cpu, const pr_r08ae_137c_env *env);
/* Resume at 13AA through MOV AL,AH and the third OUT, stopping at 13AE.
 * An absent/refusing port callback stops before the OUT at 13AC after MOV.
 * The callback owns device effects; ARGUMENT preserves state. */
pr_r08ae_137c_status pr_r08ae_vector_137c_third_port(
    uint8_t *ram, size_t ram_size, pr_r08ae_137c_cpu *cpu, const pr_r08ae_137c_env *env);
/* Resume at 13AE through POP ES, POP DS and RETF. The ordinary 137C
 * memory/stack constraints apply; the caller supplies the continuation state.
 * ARGUMENT preserves state. */
pr_r08ae_137c_status pr_r08ae_vector_137c_return(
    uint8_t *ram, size_t ram_size, pr_r08ae_137c_cpu *cpu);
#endif
