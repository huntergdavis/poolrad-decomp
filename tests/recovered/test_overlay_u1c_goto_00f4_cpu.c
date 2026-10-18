/* Focused synthetic test for raw-CPU U1C +00F4 (ECL GOTO) composing U25 +0000 and +06AF through the overlay stub
 * (docs/re/U1C_GOTO_00F4_CPU.md), and for the public raw-CPU U25 +06AF entry it uses. No game bytes: only the two
 * five-byte far-call sites, the two loaded-stub EA jumps and one synthetic ECL operand. An independent reference
 * predicts the new script PC, AX, CX, DX and the final ADD flags. */
#include "poolrad/recovered/overlay_u1c_goto_00f4_cpu.h"
#include "poolrad/recovered/overlay_u25_loader_0000.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RAM PR_U1C_GOTO_RAM_SIZE
#define CHECK(x)                                                                  \
    do {                                                                          \
        if (!(x)) {                                                               \
            fprintf(stderr, "u1c 00f4 line %d: %s\n", __LINE__, #x);              \
            exit(1);                                                              \
        }                                                                         \
        ++checks;                                                                 \
    } while (0)

enum { CS = 0x8800, U25 = 0x7800, STUB = 0x002B, SYS = 0x0AF8, DS = 0x1800, SS = 0x2800, SP0 = 0x8000, BP1 = SP0 - 2, BP0 = 0x7777,
       RET_IP = 0x7000, RET_CS = 0x9001, FLAGS0 = 0x0602, STALE = 0x5C };

static uint8_t *ram;
static unsigned checks;

static size_t at(unsigned seg, unsigned off) { return (((size_t)seg << 4) + (off & 0xFFFFu)) & (RAM - 1u); }
static void w8(unsigned seg, unsigned off, unsigned v) { ram[at(seg, off)] = (uint8_t)v; }
static uint8_t r8(unsigned seg, unsigned off) { return ram[at(seg, off)]; }
static void w16(unsigned seg, unsigned off, unsigned v) { w8(seg, off, v & 0xFFu); w8(seg, off + 1u, v >> 8); }
static unsigned r16(unsigned seg, unsigned off) { return r8(seg, off) | (unsigned)r8(seg, off + 1u) << 8; }

static void site(unsigned off, unsigned target) { w8(CS, off, 0x9Au); w16(CS, off + 1u, target); w16(CS, off + 3u, STUB); }

static pr_u1c_goto_cpu world(unsigned tag, unsigned low, unsigned third, unsigned base)
{
    pr_u1c_goto_cpu cpu = {0x1234u, 0x3456u, 0x5678u, 0x789Au, 0x9ABCu, 0xBCDEu, BP0, SP0, CS, (uint16_t)(base + 0x00F4u),
                                DS, 0x4444u, SS, FLAGS0};
    memset(ram, STALE, RAM);
    site(base + 0x00FAu, 0x002Au); site(base + 0x0107u, 0x004Du);
    w8(STUB, 0x002Au, 0xEAu); w16(STUB, 0x002Bu, 0x0000u); w16(STUB, 0x002Du, U25);
    w8(STUB, 0x004Du, 0xEAu); w16(STUB, 0x004Eu, 0x06AFu); w16(STUB, 0x0050u, U25);
    w16(DS, 0x49DEu, 0x1000u); w16(DS, 0x49E0u, 0x6300u); w16(DS, 0x49EDu, 0x9900u);       /* one operand */
    w8(0x6300u, 0x1001u, tag); w8(0x6300u, 0x1002u, low); w8(0x6300u, 0x1003u, third);
    w16(SS, SP0, RET_IP); w16(SS, SP0 + 2u, RET_CS);
    return cpu;
}

static unsigned parity_even(unsigned v)
{
    unsigned ones = 0u;
    for (unsigned b = 0u; b < 8u; ++b) ones += (v >> b) & 1u;
    return (ones & 1u) == 0u;
}

/* ADD AX,DX with AX = high << 8 and DX = low: no carry, overflow or half carry; PF/ZF/SF from the word. */
static unsigned add_flags(unsigned word)
{
    unsigned f = 0u;
    if (parity_even(word & 0xFFu)) f |= 0x0004u;
    if (word == 0u) f |= 0x0040u;
    if (word & 0x8000u) f |= 0x0080u;
    return f;
}

static void jump(unsigned tag, unsigned low, unsigned third)
{
    pr_u1c_goto_cpu cpu = world(tag, low, third, 0u);
    pr_u1c_goto_limits limits = {SYS, 4u, 400u};
    pr_u1c_goto_callee callee;
    int callee_status = 99;
    unsigned high = (tag >= 1u && tag <= 3u) ? third : STALE, word = (high << 8) | low;
    CHECK(pr_u1c_goto_00f4_cpu(ram, RAM, &cpu, &limits, &callee, &callee_status) == PR_U1C_GOTO_OK);
    CHECK(callee == PR_U1C_GOTO_WORD && callee_status == 0);
    CHECK(r8(DS, 0x7006u) == tag && r8(DS, 0x7086u) == low && r8(DS, 0x7046u) == high && r16(DS, 0x49EDu) == word);
    CHECK(cpu.ax == word && cpu.dx == low && (cpu.cx & 0xFFu) == 8u && r16(SS, BP1) == BP0);
    CHECK((r16(SS, BP1 - 2u) & 0xFFu) == low && (r16(SS, BP1 - 4u) & 0xFFu) == high);   /* the pushed words keep the previous AH */
    CHECK(r16(SS, BP1 - 6u) == CS && r16(SS, BP1 - 8u) == 0x010Cu && r16(SS, BP1 - 0x0Au) == BP1 && r16(SS, BP1 - 0x0Cu) == word);
    CHECK(cpu.bp == BP0 && cpu.sp == SP0 + 4u && cpu.cs == RET_CS && cpu.ip == RET_IP && cpu.ds == DS && cpu.ss == SS);
    CHECK((cpu.flags & 0x08D5u) == add_flags(word) && (cpu.flags & ~0x08D5u) == (FLAGS0 & ~0x08D5u));
}

static void jumps(void)
{
    unsigned seed = 0x00F4u;
    static const unsigned tags[] = {0u, 1u, 2u, 3u, 2u, 2u, 0x10u};
    for (unsigned n = 0u; n < 600u; ++n) {
        unsigned low, third;
        seed = seed * 1103515245u + 12345u; low = (seed >> 16) & 0xFFu; third = (seed >> 8) & 0xFFu;
        jump(tags[n % (sizeof tags / sizeof tags[0])], low, third);
    }
    jump(2u, 0u, 0u);                                                      /* target 0: ZF */
    jump(2u, 0xFFu, 0x80u);                                                /* SF */
}

static void stops(void)
{
    pr_u1c_goto_cpu cpu;
    pr_u1c_goto_limits limits = {SYS, 4u, 400u}, tight = {SYS, 1u, 400u};
    pr_u1c_goto_callee callee;
    int callee_status = 99;
    cpu = world(2u, 0x34u, 0x12u, 0u);                                     /* the load stub is not EA 0000: CODE at the first site */
    w16(STUB, 0x002Bu, 0x0001u);
    CHECK(pr_u1c_goto_00f4_cpu(ram, RAM, &cpu, &limits, &callee, &callee_status) == PR_U1C_GOTO_CODE);
    CHECK(callee == PR_U1C_GOTO_NO_CALLEE && cpu.ip == 0x00FAu && cpu.flags == FLAGS0 && r16(SS, cpu.sp) == 0x1201u);
    cpu = world(2u, 0x34u, 0x12u, 0u);                                     /* the word stub moved: CODE at the second site */
    w16(STUB, 0x004Eu, 0x06B0u);
    CHECK(pr_u1c_goto_00f4_cpu(ram, RAM, &cpu, &limits, &callee, &callee_status) == PR_U1C_GOTO_CODE);
    CHECK(callee == PR_U1C_GOTO_LOAD_OPERANDS && cpu.ip == 0x0107u && (r16(SS, cpu.sp) & 0xFFu) == 0x12u
          && (r16(SS, cpu.sp + 2u) & 0xFFu) == 0x34u && r16(DS, 0x49EDu) == 0x9904u);
    cpu = world(2u, 0x34u, 0x12u, 0u);                                     /* the site is not 9A 2A 00 */
    w8(CS, 0x00FBu, 0x2Bu);
    CHECK(pr_u1c_goto_00f4_cpu(ram, RAM, &cpu, &limits, &callee, &callee_status) == PR_U1C_GOTO_CODE && cpu.ip == 0x00FAu);
    cpu = world(2u, 0x34u, 0x12u, 0u);                                     /* one operand fits a budget of one */
    CHECK(pr_u1c_goto_00f4_cpu(ram, RAM, &cpu, &tight, &callee, &callee_status) == PR_U1C_GOTO_OK);
    cpu = world(2u, 0x34u, 0x12u, 0u);                                     /* U25 +0000 rejects an entry SP below 022Ah: CALLEE */
    cpu.sp = 0x0210u;
    w16(SS, 0x0210u, RET_IP); w16(SS, 0x0212u, RET_CS);
    CHECK(pr_u1c_goto_00f4_cpu(ram, RAM, &cpu, &limits, &callee, &callee_status) == PR_U1C_GOTO_CALLEE);
    CHECK(callee == PR_U1C_GOTO_LOAD_OPERANDS && callee_status == PR_U25_0000_ARGUMENT && cpu.ip == 0x0000u && cpu.cs == U25);
    cpu = world(2u, 0x34u, 0x12u, 0x2000u);                                /* entered at another offset: the sites move with it */
    CHECK(pr_u1c_goto_00f4_cpu(ram, RAM, &cpu, &limits, &callee, &callee_status) == PR_U1C_GOTO_OK && r16(DS, 0x49EDu) == 0x1234u);
}

/* The public U25 +06AF entry on its own: AX := (byte[SP+4] << 8) + byte[SP+6]; IP must be 06AF. */
static void word_entry(void)
{
    pr_u25_0000_cpu cpu = {0x1111u, 0x2222u, 0x3333u, 0x4444u, 0x5555u, 0x6666u, BP0, SP0, U25, 0x06AFu, DS, 0x7777u, SS, 0x0202u}, before;
    memset(ram, STALE, RAM);
    w16(SS, SP0, RET_IP); w16(SS, SP0 + 2u, RET_CS); w16(SS, SP0 + 4u, 0xAB9Cu); w16(SS, SP0 + 6u, 0xCD5Eu);
    CHECK(pr_u25_word_06af(ram, RAM, &cpu) == PR_U25_0000_OK);
    CHECK(cpu.ax == 0x9C5Eu && cpu.dx == 0x5Eu && (cpu.cx & 0xFFu) == 8u && cpu.ip == RET_IP && cpu.cs == RET_CS && cpu.sp == SP0 + 8u);
    CHECK(cpu.bp == BP0 && r16(SS, SP0 - 4u) == 0x9C5Eu && (cpu.flags & 0x08D5u) == 0x0080u);   /* SF; 5Eh has odd parity */
    cpu.ip = 0x06B0u;
    before = cpu;
    CHECK(pr_u25_word_06af(ram, RAM, &cpu) == PR_U25_0000_ARGUMENT && memcmp(&cpu, &before, sizeof cpu) == 0);
    CHECK(pr_u25_word_06af(NULL, RAM, &cpu) == PR_U25_0000_ARGUMENT && pr_u25_word_06af(ram, RAM - 1u, &cpu) == PR_U25_0000_ARGUMENT);
    CHECK(pr_u25_word_06af(ram, RAM, NULL) == PR_U25_0000_ARGUMENT);
}

static void arguments(void)
{
    pr_u1c_goto_cpu cpu = world(2u, 1u, 2u, 0u), before = cpu;
    pr_u1c_goto_limits limits = {SYS, 4u, 400u};
    pr_u1c_goto_callee callee = PR_U1C_GOTO_WORD;
    int callee_status = 99;
    CHECK(pr_u1c_goto_00f4_cpu(NULL, RAM, &cpu, &limits, &callee, &callee_status) == PR_U1C_GOTO_ARGUMENT);
    CHECK(callee == PR_U1C_GOTO_NO_CALLEE && callee_status == -1);
    CHECK(pr_u1c_goto_00f4_cpu(ram, RAM, NULL, &limits, NULL, NULL) == PR_U1C_GOTO_ARGUMENT);
    CHECK(pr_u1c_goto_00f4_cpu(ram, RAM, &cpu, NULL, NULL, NULL) == PR_U1C_GOTO_ARGUMENT);
    CHECK(pr_u1c_goto_00f4_cpu(ram, RAM + 1u, &cpu, &limits, NULL, NULL) == PR_U1C_GOTO_ARGUMENT);
    CHECK(memcmp(&cpu, &before, sizeof cpu) == 0);
}

int main(void)
{
    ram = malloc(RAM);
    CHECK(ram != NULL);
    jumps();
    stops();
    word_entry();
    arguments();
    free(ram);
    printf("u1c +00f4 cpu: %u checks passed\n", checks);
    return 0;
}
