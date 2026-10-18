/* Focused synthetic test for GAME.OVR unit 0x3C6:+0068 (docs/re/U3C6_SLOT_OFFSETS_0068.md). */
#include "poolrad/recovered/overlay_u3c6_slot_offsets_0068.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

_Static_assert(sizeof(pr_u3c6_0068_cpu) == 28, "all fourteen CPU words");

#define RAM PR_U3C6_0068_RAM_SIZE
#define CHECK(x)                                                              \
    do {                                                                      \
        if (!(x)) {                                                           \
            fprintf(stderr, "%s:%d: check failed: %s\n", __FILE__, __LINE__, #x); \
            exit(1);                                                          \
        }                                                                     \
        ++checks;                                                             \
    } while (0)

enum { DS = 0x1000, SS = 0x2000, GS = 0x5000, CODE = 0x3000, RET_IP = 0x0987, RET_CS = 0x4321 };

static uint8_t ram[RAM], want[RAM];
static unsigned checks;

static size_t at(unsigned segment, unsigned offset)
{
    return (((size_t)segment << 4) + (offset & 0xFFFFu)) & (RAM - 1u);
}

static void put16(uint8_t *memory, unsigned segment, unsigned offset, unsigned value)
{
    memory[at(segment, offset)] = (uint8_t)value;
    memory[at(segment, offset + 1u)] = (uint8_t)(value >> 8);
}

static unsigned get16(const uint8_t *memory, unsigned segment, unsigned offset)
{
    return memory[at(segment, offset)] | (unsigned)memory[at(segment, offset + 1u)] << 8;
}

/* Independent CMP r/m8 flag rule (CF PF AF ZF SF OF; other bits kept). */
static uint16_t cmp_flags(unsigned flags, unsigned left, unsigned right)
{
    unsigned result = (left - right) & 0xFFu, ones = 0u;
    for (unsigned bit = 0u; bit < 8u; ++bit)
        ones += (result >> bit) & 1u;
    flags &= ~0x08D5u;
    if (left < right) flags |= 0x0001u;
    if ((ones & 1u) == 0u) flags |= 0x0004u;
    if ((left ^ right ^ result) & 0x10u) flags |= 0x0010u;
    if (result == 0u) flags |= 0x0040u;
    if (result & 0x80u) flags |= 0x0080u;
    if ((left ^ right) & (left ^ result) & 0x80u) flags |= 0x0800u;
    return (uint16_t)flags;
}

/* Background pattern, count n at DS:5F2A, far pointer DS:6716 = GS:go with header bytes g2/g3,
 * and the far return frame RET_CS:RET_IP at ss:sp. */
static pr_u3c6_0068_cpu world(unsigned seed, unsigned n, unsigned go, unsigned g2, unsigned g3, unsigned ss, unsigned sp)
{
    pr_u3c6_0068_cpu cpu = {0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0x6666, 0x7777, 0, CODE, 0x0068, DS, 0x9999, 0, 0x0202};
    for (size_t i = 0; i < RAM; ++i)
        ram[i] = (uint8_t)(i * 29u + (i >> 11) + seed);
    ram[at(DS, 0x5F2Au)] = (uint8_t)n;
    put16(ram, DS, 0x6716u, go);
    put16(ram, DS, 0x6718u, GS);
    ram[at(GS, go + 2u)] = (uint8_t)g2;
    ram[at(GS, go + 3u)] = (uint8_t)g3;
    put16(ram, ss, sp, RET_IP);
    put16(ram, ss, sp + 2u, RET_CS);
    cpu.ss = (uint16_t)ss;
    cpu.sp = (uint16_t)sp;
    return cpu;
}

/* Expected memory after `passes` passes when no store aliases the frame: stores in program order, so a
 * slot-table byte that an earlier pass overwrote (n > 48h) is read back as written. */
static void reference(unsigned ss, unsigned sp, unsigned n, unsigned passes, unsigned go)
{
    memcpy(want, ram, RAM);
    put16(want, ss, sp - 2u, 0x7777u);
    want[at(ss, sp - 4u)] = (uint8_t)n;
    if (n != 0u)
        want[at(ss, sp - 3u)] = (uint8_t)passes;
    for (unsigned s = 1u; s <= passes; ++s) {
        want[at(DS, 0x604Au + s)] = (uint8_t)(want[at(DS, 0x5F27u + 4u * s)] - want[at(GS, go + 2u)]);
        want[at(DS, 0x6092u + s)] = (uint8_t)(want[at(DS, 0x5F28u + 4u * s)] - want[at(GS, go + 3u)]);
    }
}

/* Registers after the last pass s of a non-aliasing call: AX = s, DI = s, ES = GS, DX = CBW(g3) high byte with
 * the second store's low byte; BX, CX and SI unchanged. */
static void check_pass_registers(const pr_u3c6_0068_cpu *cpu, unsigned s, unsigned g3)
{
    CHECK(cpu->ax == s && cpu->di == s && cpu->es == GS);
    CHECK(cpu->dx == ((g3 & 0x80u ? 0xFF00u : 0u) | want[at(DS, 0x6092u + s)]));
    CHECK(cpu->bx == 0x2222 && cpu->cx == 0x3333 && cpu->si == 0x5555 && cpu->ds == DS && cpu->ss == SS);
}

static void check_returned(const pr_u3c6_0068_cpu *cpu, unsigned sp)
{
    CHECK(cpu->bp == 0x7777 && cpu->sp == (uint16_t)(sp + 4u) && cpu->ip == RET_IP && cpu->cs == RET_CS);
}

int main(void)
{
    static const uint8_t first[4] = {0, 0x10, 0x80, 0x00}, second[4] = {0, 0x20, 0x7F, 0xFF};
    pr_u3c6_0068_cpu cpu, saved;

    /* n = 0: MOV AL,1; CMP 1,0; JA leaves at once. Only the frame bytes change; AH, DX, ES:DI are kept. */
    cpu = world(1u, 0u, 0x0100u, 0x05u, 0xFEu, SS, 0x8000u);
    reference(SS, 0x8000u, 0u, 0u, 0x0100u);
    CHECK(pr_u3c6_slot_offsets_0068(ram, RAM, &cpu, 0u) == PR_U3C6_0068_OK);
    CHECK(memcmp(ram, want, RAM) == 0);
    CHECK(cpu.ax == 0x1101 && cpu.dx == 0x4444 && cpu.di == 0x6666 && cpu.es == 0x9999);
    CHECK(cpu.flags == cmp_flags(0x0202u, 1u, 0u));
    check_returned(&cpu, 0x8000u);

    /* n = 3 with G[+2] = +5 and G[+3] = -2; two loop-backs is exactly enough. */
    cpu = world(2u, 3u, 0x0100u, 0x05u, 0xFEu, SS, 0x8000u);
    for (unsigned s = 1u; s <= 3u; ++s) {
        ram[at(DS, 0x5F27u + 4u * s)] = first[s];
        ram[at(DS, 0x5F28u + 4u * s)] = second[s];
    }
    reference(SS, 0x8000u, 3u, 3u, 0x0100u);
    CHECK(pr_u3c6_slot_offsets_0068(ram, RAM, &cpu, 2u) == PR_U3C6_0068_OK);
    CHECK(memcmp(ram, want, RAM) == 0);
    CHECK(ram[at(DS, 0x604Bu)] == 0x0B && ram[at(DS, 0x604Cu)] == 0x7B && ram[at(DS, 0x604Du)] == 0xFB);
    CHECK(ram[at(DS, 0x6093u)] == 0x22 && ram[at(DS, 0x6094u)] == 0x81 && ram[at(DS, 0x6095u)] == 0x01);
    check_pass_registers(&cpu, 3u, 0xFEu);
    CHECK(cpu.dx == 0xFF01 && cpu.flags == cmp_flags(0x0202u, 3u, 3u));
    check_returned(&cpu, 0x8000u);

    /* n = FFh: from slot 49h the table bytes are earlier stores; 254 loop-backs is exactly enough. */
    cpu = world(3u, 0xFFu, 0x0100u, 0x9Cu, 0x6Bu, SS, 0x8000u);
    reference(SS, 0x8000u, 0xFFu, 0xFFu, 0x0100u);
    CHECK(pr_u3c6_slot_offsets_0068(ram, RAM, &cpu, 254u) == PR_U3C6_0068_OK);
    CHECK(memcmp(ram, want, RAM) == 0);
    check_pass_registers(&cpu, 0xFFu, 0x6Bu);
    CHECK(cpu.flags == cmp_flags(0x0202u, 0xFFu, 0xFFu));
    check_returned(&cpu, 0x8000u);

    /* G at offset FFFEh: the header bytes G[+2] and G[+3] wrap to GS:0000 and GS:0001. */
    cpu = world(4u, 2u, 0xFFFEu, 0x7Fu, 0x80u, SS, 0x8000u);
    reference(SS, 0x8000u, 2u, 2u, 0xFFFEu);
    CHECK(pr_u3c6_slot_offsets_0068(ram, RAM, &cpu, 254u) == PR_U3C6_0068_OK);
    CHECK(memcmp(ram, want, RAM) == 0);
    check_pass_registers(&cpu, 2u, 0x80u);
    check_returned(&cpu, 0x8000u);

    /* Host cap: n = 30h with max_loopbacks 4 stops before the fifth INC at +0080, all effects of five passes kept. */
    cpu = world(5u, 0x30u, 0x0100u, 0x81u, 0x7Fu, SS, 0x8000u);
    reference(SS, 0x8000u, 0x30u, 5u, 0x0100u);
    CHECK(pr_u3c6_slot_offsets_0068(ram, RAM, &cpu, 4u) == PR_U3C6_0068_LIMIT);
    CHECK(memcmp(ram, want, RAM) == 0);
    check_pass_registers(&cpu, 5u, 0x7Fu);
    CHECK(cpu.flags == cmp_flags(0x0202u, 5u, 0x30u) && cpu.flags == 0x0283);
    CHECK(cpu.cs == CODE && cpu.ip == 0x0080 && cpu.bp == 0x7FFE && cpu.sp == 0x7FFC);

    /* SS = DS, entry SP 606Dh: [BP-2] is DS:6069 = 604A+1Fh and [BP-1] is DS:606A = 604A+20h. Pass 1Fh rewrites
     * the count to 12h and pass 20h the counter to 10h, so passes 11h and 12h follow and the call returns. */
    cpu = world(6u, 0x40u, 0x0100u, 0x03u, 0x11u, DS, 0x606Du);
    ram[at(DS, 0x5F27u + 4u * 0x1Fu)] = 0x12 + 0x03;
    ram[at(DS, 0x5F27u + 4u * 0x20u)] = 0x10 + 0x03;
    CHECK(pr_u3c6_slot_offsets_0068(ram, RAM, &cpu, 254u) == PR_U3C6_0068_OK);
    CHECK(ram[at(DS, 0x6069u)] == 0x12 && ram[at(DS, 0x606Au)] == 0x12 && cpu.ax == 0x0012);
    CHECK(get16(ram, DS, 0x606Bu) == 0x7777 && get16(ram, DS, 0x606Du) == RET_IP && get16(ram, DS, 0x606Fu) == RET_CS);
    CHECK(cpu.bp == 0x7777 && cpu.sp == 0x6071 && cpu.ip == RET_IP && cpu.cs == RET_CS);

    /* Same frame, count := 60h and counter := 05h: the counter cycles 06h..20h and never meets 60h, so the original
     * never returns. The 1001st taken JNE is the 24th of the 36th cycle: counter 05h + 24 = 1Dh. */
    cpu = world(7u, 0x40u, 0x0100u, 0x03u, 0x11u, DS, 0x606Du);
    ram[at(DS, 0x5F27u + 4u * 0x1Fu)] = 0x60 + 0x03;
    ram[at(DS, 0x5F27u + 4u * 0x20u)] = 0x05 + 0x03;
    CHECK(pr_u3c6_slot_offsets_0068(ram, RAM, &cpu, 1000u) == PR_U3C6_0068_LIMIT);
    CHECK(ram[at(DS, 0x6069u)] == 0x60 && ram[at(DS, 0x606Au)] == 0x1D && cpu.ax == 0x001D);
    CHECK(cpu.flags == cmp_flags(0x0202u, 0x1Du, 0x60u));
    CHECK(cpu.cs == CODE && cpu.ip == 0x0080 && cpu.bp == 0x606B && cpu.sp == 0x6069);
    CHECK(get16(ram, DS, 0x606Bu) == 0x7777 && get16(ram, DS, 0x606Du) == RET_IP && get16(ram, DS, 0x606Fu) == RET_CS);

    /* Argument rejections leave RAM and CPU unchanged. */
    cpu = world(8u, 3u, 0x0100u, 0x05u, 0xFEu, SS, 0x8000u);
    memcpy(want, ram, RAM);
    saved = cpu;
    CHECK(pr_u3c6_slot_offsets_0068(NULL, RAM, &cpu, 254u) == PR_U3C6_0068_ARGUMENT);
    CHECK(pr_u3c6_slot_offsets_0068(ram, RAM, NULL, 254u) == PR_U3C6_0068_ARGUMENT);
    CHECK(pr_u3c6_slot_offsets_0068(ram, 0u, &cpu, 254u) == PR_U3C6_0068_ARGUMENT);
    CHECK(pr_u3c6_slot_offsets_0068(ram, RAM - 1u, &cpu, 254u) == PR_U3C6_0068_ARGUMENT);
    CHECK(pr_u3c6_slot_offsets_0068(ram, (size_t)RAM + 1u, &cpu, 254u) == PR_U3C6_0068_ARGUMENT);
    CHECK(memcmp(ram, want, RAM) == 0 && memcmp(&cpu, &saved, sizeof cpu) == 0);

    printf("PASS: u3C6 +0068 slot offsets, %u checks (n = 0/3/FFh, offset wrap, host cap, frame aliasing, arguments)\n",
           checks);
    return 0;
}
