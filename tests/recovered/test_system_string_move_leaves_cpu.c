/* Focused synthetic test for the raw-CPU SYSTEM leaves 0AF8:0B57 (Copy), 0C10 (string compare), 0C3B (char to string)
 * and 1685 (Move) (docs/re/SYSTEM_STRING_MOVE_LEAVES.md). No game bytes: synthetic strings, every result predicted by hand. */
#include "poolrad/recovered/system_string_move_leaves_cpu.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RAM PR_SYSTEM_LEAF_RAM_SIZE
#define CHECK(x)                                                                  \
    do {                                                                          \
        if (!(x)) {                                                               \
            fprintf(stderr, "string/move line %d: %s\n", __LINE__, #x);           \
            exit(1);                                                              \
        }                                                                         \
        ++checks;                                                                 \
    } while (0)

enum { SYS = 0x0AF8, SS = 0x2800, SP0 = 0x4000, RET_IP = 0x7000, RET_CS = 0x9001, DS0 = 0x1357, SEG = 0x4000, DSEG = 0x5000 };

static uint8_t *ram;
static unsigned checks;

static size_t at(unsigned seg, unsigned off) { return (((size_t)seg << 4) + (off & 0xFFFFu)) & (RAM - 1u); }
static void put16(unsigned seg, unsigned off, unsigned value)
{
    ram[at(seg, off)] = (uint8_t)value;
    ram[at(seg, off + 1u)] = (uint8_t)(value >> 8);
}
static void put(unsigned seg, unsigned off, const char *bytes, size_t length)
{
    size_t i;
    for (i = 0; i < length; ++i)
        ram[at(seg, off + (unsigned)i)] = (uint8_t)bytes[i];
}
static void pstring(unsigned seg, unsigned off, const char *text)
{
    ram[at(seg, off)] = (uint8_t)strlen(text);
    put(seg, off + 1u, text, strlen(text));
}
static int same(unsigned seg, unsigned off, const char *bytes, size_t length)
{
    size_t i;
    for (i = 0; i < length; ++i)
        if (ram[at(seg, off + (unsigned)i)] != (uint8_t)bytes[i])
            return 0;
    return 1;
}

/* A fresh world with the far-call frame RET, words[0..count) at SS:SP0. */
static pr_system_leaf_cpu frame(uint16_t ip, const unsigned *words, size_t count, uint16_t flags)
{
    pr_system_leaf_cpu cpu;
    size_t i;
    memset(ram, 0xA5, RAM);
    put16(SS, SP0, RET_IP); put16(SS, SP0 + 2u, RET_CS);
    for (i = 0; i < count; ++i)
        put16(SS, SP0 + 4u + 2u * (unsigned)i, words[i]);
    memset(&cpu, 0, sizeof cpu);
    cpu.ax = 0x1111; cpu.bx = 0x2222; cpu.cx = 0x3333; cpu.dx = 0x4444; cpu.si = 0x5555; cpu.di = 0x6666; cpu.bp = 0x7777;
    cpu.sp = SP0; cpu.cs = SYS; cpu.ip = ip; cpu.ds = DS0; cpu.es = 0x9999; cpu.ss = SS; cpu.flags = flags;
    return cpu;
}

static pr_system_leaf_cpu copy(unsigned index, unsigned count)
{
    const unsigned words[6] = {count, index, 0x0100, SEG, 0x0200, DSEG};
    pr_system_leaf_cpu cpu = frame(0x0B57, words, 6, 0x0602);
    pstring(SEG, 0x0100, "Hello, Phlan");
    CHECK(pr_system_strcopy_0b57(ram, RAM, &cpu) == PR_SYSTEM_LEAF_RETURNED);
    CHECK(cpu.cs == RET_CS && cpu.ip == RET_IP && cpu.sp == SP0 + 12u && cpu.bx == SP0 && cpu.dx == DS0 && cpu.ds == DS0 && cpu.es == DSEG);
    return cpu;
}

static pr_system_leaf_cpu compare(const char *left, const char *right)
{
    const unsigned words[4] = {0x0200, DSEG, 0x0100, SEG};
    pr_system_leaf_cpu cpu = frame(0x0C10, words, 4, 0x0602);
    pstring(SEG, 0x0100, left);
    pstring(DSEG, 0x0200, right);
    CHECK(pr_system_strcmp_0c10(ram, RAM, &cpu) == PR_SYSTEM_LEAF_RETURNED);
    CHECK(cpu.cs == RET_CS && cpu.ip == RET_IP && cpu.sp == SP0 + 12u && cpu.ds == DS0 && cpu.es == DSEG);
    return cpu;
}

static pr_system_leaf_cpu move(unsigned source, unsigned destination, unsigned count, uint16_t flags)
{
    const unsigned words[5] = {count, destination, SEG, source, SEG};
    pr_system_leaf_cpu cpu = frame(0x1685, words, 5, flags);
    put(SEG, source, "ABCDEFGHIJKLMNOP", 16);
    CHECK(pr_system_move_1685(ram, RAM, &cpu) == PR_SYSTEM_LEAF_RETURNED);
    CHECK(cpu.cs == RET_CS && cpu.ip == RET_IP && cpu.sp == SP0 + 14u && cpu.ax == 0x1111 && cpu.cx == 0 && cpu.ds == DS0 && cpu.es == SEG);
    return cpu;
}

int main(void)
{
    pr_system_leaf_cpu cpu;
    ram = malloc(RAM);
    if (ram == NULL)
        return 1;

    /* Copy("Hello, Phlan", 8, 5) = "Phlan": CMP 5,5 leaves ZF and PF; DF cleared. */
    cpu = copy(8, 5);
    CHECK(same(DSEG, 0x0200, "\x05Phlan", 6) && cpu.ax == 5 && cpu.cx == 0 && cpu.si == 0x010D && cpu.di == 0x0206);
    CHECK(cpu.flags == 0x0246);
    /* Index 0 becomes 1: "Hel"; CMP 12,3 = 9 leaves only PF. */
    cpu = copy(0, 3);
    CHECK(same(DSEG, 0x0200, "\x03Hel", 4) && cpu.ax == 3 && cpu.flags == 0x0206);
    /* Index 13 is past the 12 characters: the empty string with XOR AX,AX's flags. */
    cpu = copy(13, 5);
    CHECK(same(DSEG, 0x0200, "\x00", 1) && cpu.ax == 0 && cpu.si == 0x010D && cpu.di == 0x0201 && cpu.flags == 0x0246);
    /* A negative count becomes 0: CMP 11,0 = 11 (odd parity), AX := 0. */
    cpu = copy(2, 0xFFFF);
    CHECK(same(DSEG, 0x0200, "\x00", 1) && cpu.ax == 0 && cpu.flags == 0x0202);

    /* Compare equal strings: four equal bytes, then CMP 4,4. */
    cpu = compare("Pool", "Pool");
    CHECK(cpu.ax == 0x0404 && cpu.cx == 0 && cpu.si == 0x0105 && cpu.di == 0x0205 && cpu.flags == 0x0246);
    /* "Poo" < "Pool": three equal bytes, then CMP 3,4 = FFh (CF, PF, AF, SF). */
    cpu = compare("Poo", "Pool");
    CHECK(cpu.ax == 0x0403 && cpu.cx == 0 && cpu.flags == 0x0297);
    /* "Aa" < "Ba": the first CMPSB differs (41h - 42h = FFh); one byte of two compared, CMP AL,AH skipped. */
    cpu = compare("Aa", "Ba");
    CHECK(cpu.ax == 0x0202 && cpu.cx == 1 && cpu.si == 0x0102 && cpu.di == 0x0202 && cpu.flags == 0x0297);
    /* "" < "x": CL = 0, so CH keeps 33h and no byte is compared; CMP 0,1. */
    cpu = compare("", "x");
    CHECK(cpu.ax == 0x0100 && cpu.cx == 0x3300 && cpu.si == 0x0101 && cpu.di == 0x0201 && cpu.flags == 0x0297);

    /* Char to string: D := length 1 and the char; AL := the char; only DF changes. */
    {
        const unsigned words[3] = {0x9941, 0x0300, DSEG};
        cpu = frame(0x0C3B, words, 3, 0x0ED7);
        CHECK(pr_system_charstr_0c3b(ram, RAM, &cpu) == PR_SYSTEM_LEAF_RETURNED);
        CHECK(same(DSEG, 0x0300, "\x01" "A", 2) && cpu.ax == 0x1141 && cpu.di == 0x0302 && cpu.es == DSEG && cpu.bx == SP0);
        CHECK(cpu.sp == SP0 + 6u && cpu.cs == RET_CS && cpu.ip == RET_IP && cpu.flags == 0x0AD7 && cpu.dx == 0x4444 && cpu.ds == DS0);
    }
    /* At offset FFFFh the char wraps to offset 0000h of the same segment. */
    {
        const unsigned words[3] = {0x005A, 0xFFFF, DSEG};
        cpu = frame(0x0C3B, words, 3, 0x0202);
        CHECK(pr_system_charstr_0c3b(ram, RAM, &cpu) == PR_SYSTEM_LEAF_RETURNED);
        CHECK(ram[at(DSEG, 0xFFFF)] == 1 && ram[at(DSEG, 0x0000)] == 'Z' && cpu.di == 0x0001);
    }

    /* Move forward (source offset 200h >= destination 100h): CMP leaves PF; DF cleared. */
    cpu = move(0x0200, 0x0100, 8, 0x0602);
    CHECK(same(SEG, 0x0100, "ABCDEFGH", 8) && cpu.si == 0x0208 && cpu.di == 0x0108 && cpu.flags == 0x0206);
    /* Move backward over an overlap (100h -> 102h): memmove result, DF stays set, DEC DI 10Ah -> 109h leaves PF. */
    cpu = move(0x0100, 0x0102, 8, 0x0202);
    CHECK(same(SEG, 0x0100, "ABABCDEFGH", 10) && cpu.si == 0x00FF && cpu.di == 0x0101 && cpu.flags == 0x0606);
    /* Backward with ADD DI,CX carrying (FFF8h + 10h): CF set, DEC DI to 0007h; the destination wraps. */
    cpu = move(0x0100, 0xFFF8, 16, 0x0202);
    CHECK(same(SEG, 0xFFF8, "ABCDEFGH", 8) && same(SEG, 0x0000, "IJKLMNOP", 8) && cpu.si == 0x00FF && cpu.di == 0xFFF7);
    CHECK(cpu.flags == 0x0603);
    /* Backward with DI + CX = 8000h: DEC DI to 7FFFh sets OF, AF and PF. */
    cpu = move(0x1000, 0x7F00, 0x100, 0x0202);
    CHECK(cpu.di == 0x7EFF && cpu.si == 0x0FFF && cpu.flags == 0x0E16);

    /* Guards: nothing changes. */
    {
        const unsigned words[1] = {0};
        pr_system_leaf_cpu before;
        cpu = frame(0x0B57, words, 1, 0x0602);
        before = cpu;
        CHECK(pr_system_strcopy_0b57(NULL, RAM, &cpu) == PR_SYSTEM_LEAF_ARGUMENT);
        CHECK(pr_system_strcmp_0c10(ram, RAM - 1u, &cpu) == PR_SYSTEM_LEAF_ARGUMENT);
        CHECK(pr_system_charstr_0c3b(ram, RAM, NULL) == PR_SYSTEM_LEAF_ARGUMENT);
        CHECK(pr_system_move_1685(NULL, RAM, NULL) == PR_SYSTEM_LEAF_ARGUMENT);
        CHECK(memcmp(&cpu, &before, sizeof cpu) == 0);
    }

    free(ram);
    printf("string/move leaves: %u checks passed\n", checks);
    return 0;
}
