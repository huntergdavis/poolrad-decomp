/* Minimal compile-gate smoke test for the U86 +263d full-state model. */
#include "poolrad/recovered/overlay_u86_status_263d_full.h"

#include <stdio.h>
#include <string.h>

#define CHECK(expr) do { if (!(expr)) { \
    fprintf(stderr, "check failed at line %d: %s\n", __LINE__, #expr); \
    return 1; \
} } while (0)

static uint8_t ram[PR_U86_263D_FULL_RAM_SIZE];

static size_t phys(uint16_t seg, uint16_t off) { return (((size_t)seg << 4) + off) & (PR_U86_263D_FULL_RAM_SIZE - 1u); }
/* word helpers match the model: the high byte is the next physical byte */
static void put16(uint16_t seg, uint16_t off, uint16_t v) { size_t a = phys(seg, off); ram[a] = (uint8_t)v; ram[(a + 1u) & (PR_U86_263D_FULL_RAM_SIZE - 1u)] = (uint8_t)(v >> 8); }
static uint16_t get16(uint16_t seg, uint16_t off) { size_t a = phys(seg, off); return (uint16_t)(ram[a] | (ram[(a + 1u) & (PR_U86_263D_FULL_RAM_SIZE - 1u)] << 8)); }

typedef struct {
    int n, abort_at, redirect;
    int pop0;                 /* 0B0B hook pops this many bytes (0: the contract, 8) */
    int shift1;               /* +08C3 hook moves SP by this and forges a matching return frame there */
    int new_ss, new_bp;       /* 0B0B hook changes SS by new_ss; +08C3 hook changes BP by new_bp */
    int patch_code;           /* 0B0B hook flips the opcode byte at CS:+2669 */
    int patch_disp;           /* 0B0B hook writes near displacement 0x0100 at CS:+2664 (a live operand) */
    uint16_t set_flags;       /* +08C3 hook leaves FLAGS = this when nonzero */
    pr_u86_263d_full_call call[2];
    uint16_t cs[2], ip[2], sp[2], flags[2], w[2][6];
} hooks_t;

static int hook(void *c, uint8_t *r, size_t size, pr_u86_263d_full_cpu *cpu, pr_u86_263d_full_call call)
{
    hooks_t *h = c; int i;
    (void)r; (void)size;
    if (h->n < 2) {
        h->call[h->n] = call; h->cs[h->n] = cpu->cs; h->ip[h->n] = cpu->ip; h->sp[h->n] = cpu->sp; h->flags[h->n] = cpu->flags;
        for (i = 0; i < 6; i++) h->w[h->n][i] = get16(cpu->ss, (uint16_t)(cpu->sp + 2 * i));
    }
    h->n++;
    if (h->abort_at == h->n) return 0;
    if (h->redirect) put16(cpu->ss, cpu->sp, 0x9999u);
    if (call == PR_U86_263D_FULL_0B0B) {          /* the lcall callee's own retf 4 */
        if (h->patch_code) ram[phys(0x6000u, 0x2669u)] ^= 0xFFu;
        if (h->patch_disp) put16(0x6000u, 0x2664u, 0x0100u);
        cpu->ip = get16(cpu->ss, cpu->sp);
        cpu->cs = get16(cpu->ss, (uint16_t)(cpu->sp + 2u));
        cpu->sp = (uint16_t)(cpu->sp + (h->pop0 ? h->pop0 : 8));   /* retf 4: return frame + 4 */
        cpu->ss = (uint16_t)(cpu->ss + h->new_ss);
    } else {
        if (h->shift1) {                          /* a broken callee: SP moved, return frame still matching */
            cpu->sp = (uint16_t)(cpu->sp + h->shift1);
            put16(cpu->ss, cpu->sp, 0x2666u); put16(cpu->ss, (uint16_t)(cpu->sp + 2u), 0x6000u);
        }
        cpu->bp = (uint16_t)(cpu->bp + h->new_bp);
        if (h->set_flags) cpu->flags = h->set_flags;
    }
    return 1;
}

/* SP0 = sp on SS 0xE000 with the caller's far return frame 0xD000:0x1234 there */
static void setup_at(pr_u86_263d_full_cpu *cpu, hooks_t *h, uint16_t ss, uint16_t sp)
{
    memset(ram, 0, sizeof ram); memset(h, 0, sizeof *h); memset(cpu, 0, sizeof *cpu);
    ram[phys(0x3000u, 0x681Bu)] = 0x5Au;
    cpu->ax = 0xAAAAu; cpu->bp = 0x7777u; cpu->cs = 0x6000u;
    cpu->ds = 0x3000u; cpu->es = 0x1111u; cpu->flags = 0x0002u;
    /* the original call operands: lcall 0AF8:0B0B at CS:+265E/+2660, near disp E25D (+08C3) at CS:+2664 */
    put16(0x6000u, 0x265Eu, 0x0B0Bu); put16(0x6000u, 0x2660u, 0x0AF8u); put16(0x6000u, 0x2664u, 0xE25Du);
    cpu->ss = ss; cpu->sp = sp;
    put16(ss, sp, 0x1234u); put16(ss, (uint16_t)(sp + 2u), 0xD000u);
}

static void setup(pr_u86_263d_full_cpu *cpu, hooks_t *h) { setup_at(cpu, h, 0xE000u, 0x8000u); }

int main(void)
{
    pr_u86_263d_full_cpu cpu; hooks_t h;
    setup(&cpu, &h);
    CHECK(pr_u86_status_263d_full(ram, sizeof ram, &cpu, hook, &h) == PR_U86_263D_FULL_OK && h.n == 2);
    /* the string load: raw lcall frame, then the pool address CS:2631 and SS:[bp-0xC] */
    CHECK(h.call[0] == PR_U86_263D_FULL_0B0B && h.cs[0] == 0x0AF8u && h.ip[0] == 0x0B0Bu && h.sp[0] == 0x7FDCu);
    CHECK(h.w[0][0] == 0x2662u && h.w[0][1] == 0x6000u && h.w[0][2] == 0x2631u && h.w[0][3] == 0x6000u && h.w[0][4] == 0x7FF2u && h.w[0][5] == 0xE000u);
    /* the local routine: return +2666, CS, the kept string pointer, then the four zero bytes and DS:[0x681B] (AH carried) */
    CHECK(h.call[1] == PR_U86_263D_FULL_08C3 && h.cs[1] == 0x6000u && h.ip[1] == 0x08C3u && h.sp[1] == 0x7FE0u);
    CHECK(h.w[1][0] == 0x2666u && h.w[1][1] == 0x6000u && h.w[1][2] == 0x7FF2u && h.w[1][3] == 0xE000u && h.w[1][4] == 0xAA00u && h.w[1][5] == 0xAA00u);
    CHECK(get16(0xE000u, 0x7FF0u) == 0xAA5Au && get16(0xE000u, 0x7FEEu) == 0xAA00u);
    CHECK(cpu.ip == 0x1234u && cpu.cs == 0xD000u && cpu.sp == 0x8004u && cpu.bp == 0x7777u);
    CHECK(cpu.ax == 0xAA00u && cpu.di == 0x2631u && cpu.es == 0x1111u && cpu.flags == 0x0002u);

    /* discriminating SUB flags and AH: SP0 0x8006, BP 0x8004 - 0xC = 0x7FF8 sets AF and OF and
     * clears CF/PF/ZF/SF from 0x08D7; both hooks enter with 0x0812; the final FLAGS are +08C3's */
    setup_at(&cpu, &h, 0xE000u, 0x8006u); cpu.ax = 0x5A3Cu; cpu.flags = 0x08D7u; ram[phys(0x3000u, 0x681Bu)] = 0xC3u; h.set_flags = 0x0C55u;
    CHECK(pr_u86_status_263d_full(ram, sizeof ram, &cpu, hook, &h) == PR_U86_263D_FULL_OK && h.n == 2);
    CHECK(h.flags[0] == 0x0812u && h.flags[1] == 0x0812u && h.sp[0] == 0x7FE2u && h.sp[1] == 0x7FE6u);
    CHECK(h.w[1][2] == 0x7FF8u && h.w[1][4] == 0x5A00u && h.w[1][5] == 0x5A00u && get16(0xE000u, 0x7FF6u) == 0x5AC3u);
    CHECK(cpu.flags == 0x0C55u && cpu.ax == 0x5A00u && cpu.sp == 0x800Au && cpu.ip == 0x1234u && cpu.cs == 0xD000u);

    /* aborts and foreign returns */
    setup(&cpu, &h); h.abort_at = 1;
    CHECK(pr_u86_status_263d_full(ram, sizeof ram, &cpu, hook, &h) == PR_U86_263D_FULL_CALL_ABORT && cpu.cs == 0x0AF8u && cpu.ip == 0x0B0Bu && cpu.sp == 0x7FDCu);
    setup(&cpu, &h); h.abort_at = 2;
    CHECK(pr_u86_status_263d_full(ram, sizeof ram, &cpu, hook, &h) == PR_U86_263D_FULL_CALL_ABORT && cpu.ip == 0x08C3u && cpu.sp == 0x7FE0u);
    setup(&cpu, &h); h.redirect = 1;
    CHECK(pr_u86_status_263d_full(ram, sizeof ram, &cpu, hook, &h) == PR_U86_263D_FULL_TRANSFER && cpu.ip == 0x9999u);

    /* stack contract: the right CS:IP with a wrong SS, SP or BP still fails closed */
    setup(&cpu, &h); h.pop0 = 6;                  /* 0B0B under-pops (SP 0x7FE2, not 0x7FE4) */
    CHECK(pr_u86_status_263d_full(ram, sizeof ram, &cpu, hook, &h) == PR_U86_263D_FULL_TRANSFER && h.n == 1 && cpu.ip == 0x2662u && cpu.sp == 0x7FE2u);
    setup(&cpu, &h); h.pop0 = 10;                 /* 0B0B over-pops into the kept destination pointer */
    CHECK(pr_u86_status_263d_full(ram, sizeof ram, &cpu, hook, &h) == PR_U86_263D_FULL_TRANSFER && h.n == 1 && cpu.ip == 0x2662u && cpu.sp == 0x7FE6u);
    setup(&cpu, &h); h.new_ss = 1;                /* 0B0B returns with another SS */
    CHECK(pr_u86_status_263d_full(ram, sizeof ram, &cpu, hook, &h) == PR_U86_263D_FULL_TRANSFER && h.n == 1 && cpu.ss == 0xE001u);
    setup(&cpu, &h); h.shift1 = -2;               /* +08C3 hook pushes two bytes, return frame forged at 0x7FDE */
    CHECK(pr_u86_status_263d_full(ram, sizeof ram, &cpu, hook, &h) == PR_U86_263D_FULL_TRANSFER && h.n == 2 && cpu.ip == 0x08C3u && cpu.sp == 0x7FDEu);
    setup(&cpu, &h); h.shift1 = 2;                /* +08C3 hook pops two bytes, return frame forged at 0x7FE2 */
    CHECK(pr_u86_status_263d_full(ram, sizeof ram, &cpu, hook, &h) == PR_U86_263D_FULL_TRANSFER && h.n == 2 && cpu.ip == 0x08C3u && cpu.sp == 0x7FE2u);
    setup(&cpu, &h); h.new_bp = 2;                /* +08C3 hook changes BP: SP 0x7FF2 is not BP - 0xC after retf 0xE */
    CHECK(pr_u86_status_263d_full(ram, sizeof ram, &cpu, hook, &h) == PR_U86_263D_FULL_TRANSFER && h.n == 2 && cpu.ip == 0x2666u && cpu.sp == 0x7FF2u && cpu.bp == 0x8000u);

    /* RETF/stack-word policy: the window [SP0 - 0x24, SP0 + 4) may not wrap; nothing runs if it would */
    setup_at(&cpu, &h, 0xE000u, 0x0023u);
    CHECK(pr_u86_status_263d_full(ram, sizeof ram, &cpu, hook, &h) == PR_U86_263D_FULL_STACK_BOUNDARY && h.n == 0 && cpu.sp == 0x0023u && cpu.bp == 0x7777u);
    setup_at(&cpu, &h, 0xE000u, 0xFFFDu);
    CHECK(pr_u86_status_263d_full(ram, sizeof ram, &cpu, hook, &h) == PR_U86_263D_FULL_STACK_BOUNDARY && h.n == 0 && cpu.sp == 0xFFFDu);
    setup_at(&cpu, &h, 0xE000u, 0x0024u);
    CHECK(pr_u86_status_263d_full(ram, sizeof ram, &cpu, hook, &h) == PR_U86_263D_FULL_OK && h.sp[0] == 0x0000u && cpu.sp == 0x0028u && cpu.ip == 0x1234u);
    setup_at(&cpu, &h, 0xE000u, 0xFFFCu);
    CHECK(pr_u86_status_263d_full(ram, sizeof ram, &cpu, hook, &h) == PR_U86_263D_FULL_OK && h.sp[0] == 0xFFD8u && cpu.sp == 0x0000u && cpu.cs == 0xD000u);

    /* code aliasing is not modelled: a stack window over the body stops before anything runs */
    setup_at(&cpu, &h, 0x6000u, 0x2682u);         /* SS = CS: the pushes would overwrite +2662..+2669 */
    CHECK(pr_u86_status_263d_full(ram, sizeof ram, &cpu, hook, &h) == PR_U86_263D_FULL_CODE_ALIAS && h.n == 0 && cpu.sp == 0x2682u && get16(0x6000u, 0x2664u) == 0xE25Du);
    setup_at(&cpu, &h, 0x6001u, 0x2672u);         /* the same physical bytes through another segment */
    CHECK(pr_u86_status_263d_full(ram, sizeof ram, &cpu, hook, &h) == PR_U86_263D_FULL_CODE_ALIAS && h.n == 0);
    setup_at(&cpu, &h, 0x6000u, 0x268Du);         /* the window's lowest byte is +2669 */
    CHECK(pr_u86_status_263d_full(ram, sizeof ram, &cpu, hook, &h) == PR_U86_263D_FULL_CODE_ALIAS && h.n == 0);
    setup_at(&cpu, &h, 0x6000u, 0x268Eu);         /* SS = CS, window [266A, 2692) clears the body */
    CHECK(pr_u86_status_263d_full(ram, sizeof ram, &cpu, hook, &h) == PR_U86_263D_FULL_OK && h.n == 2 && h.sp[0] == 0x266Au && h.ip[1] == 0x08C3u && cpu.sp == 0x2692u);
    setup(&cpu, &h); h.patch_code = 1;            /* a hook rewrites an opcode the body still executes */
    CHECK(pr_u86_status_263d_full(ram, sizeof ram, &cpu, hook, &h) == PR_U86_263D_FULL_CODE_ALIAS && h.n == 1 && cpu.ip == 0x2662u);

    /* live operands: patched code bytes move both targets, before the run or from the lcall hook */
    setup(&cpu, &h); put16(0x6000u, 0x265Eu, 0x1234u); put16(0x6000u, 0x2660u, 0x0BCDu); put16(0x6000u, 0x2664u, 0x0100u);
    CHECK(pr_u86_status_263d_full(ram, sizeof ram, &cpu, hook, &h) == PR_U86_263D_FULL_OK && h.n == 2);
    CHECK(h.cs[0] == 0x0BCDu && h.ip[0] == 0x1234u && h.ip[1] == 0x2766u && h.cs[1] == 0x6000u);
    setup(&cpu, &h); h.patch_disp = 1;
    CHECK(pr_u86_status_263d_full(ram, sizeof ram, &cpu, hook, &h) == PR_U86_263D_FULL_OK && h.n == 2 && h.ip[1] == 0x2766u);

    CHECK(pr_u86_status_263d_full(NULL, sizeof ram, &cpu, hook, &h) == PR_U86_263D_FULL_ARGUMENT);
    CHECK(pr_u86_status_263d_full(ram, sizeof ram - 1u, &cpu, hook, &h) == PR_U86_263D_FULL_ARGUMENT);
    CHECK(pr_u86_status_263d_full(ram, sizeof ram, &cpu, NULL, &h) == PR_U86_263D_FULL_ARGUMENT);
    puts("overlay_u86_status_263d_full: ok");
    return 0;
}
