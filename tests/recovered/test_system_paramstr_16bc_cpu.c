/* Focused synthetic test for the raw-CPU SYSTEM ParamStr 0AF8:16BC + 1712 (docs/re/SYSTEM_PARAMSTR_16BC_CPU.md). No game bytes. */
#include "poolrad/recovered/system_paramstr_16bc_cpu.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned checks;
#define CHECK(x) do { ++checks; if (!(x)) { fprintf(stderr, "paramstr line %d: %s\n", __LINE__, #x); exit(1); } } while (0)
static size_t p(unsigned s, unsigned o) { return ((size_t)s * 16u + (o & 65535u)) & 1048575u; }
static void put(uint8_t *m, unsigned s, unsigned o, unsigned v) { size_t a = p(s, o); m[a] = (uint8_t)v; m[(a + 1u) & 1048575u] = (uint8_t)(v >> 8); }
static unsigned get(const uint8_t *m, unsigned s, unsigned o) { size_t a = p(s, o); return m[a] | (unsigned)m[(a + 1u) & 1048575u] << 8; }

enum { SS = 0x2000, DS = 0x5000, PSP = 0x4000, DEST = 0x3000 };
/* PSP command tail as DOS builds it: count byte at :0080, the bytes from :0081, then 0Dh (not counted, not read). */
static pr_paramstr_cpu setup(uint8_t *ram, const char *tail, unsigned n)
{
    pr_paramstr_cpu c = {0x1234, 0x3456, 0x5678, 0x789A, 0x9ABC, 0xBCDE, 0x7777, 0x8000, 0x6000, 0x16BC, DS, 0x1111, SS, 0x0402};
    size_t len = strlen(tail), k;
    memset(ram, 0x5A, PR_PARAMSTR_RAM_SIZE);
    ram[p(PSP, 0x80)] = (uint8_t)len;
    for (k = 0; k < len; ++k) ram[p(PSP, 0x81u + (unsigned)k)] = (uint8_t)tail[k];
    ram[p(PSP, 0x81u + (unsigned)len)] = 0x0D;
    put(ram, DS, 0x4422, PSP);                                           /* PrefixSeg */
    put(ram, SS, 0x8000, 0x7000); put(ram, SS, 0x8002, 0x9001);          /* caller's far return */
    put(ram, SS, 0x8004, n); put(ram, SS, 0x8006, 0x0100); put(ram, SS, 0x8008, DEST);
    return c;
}
static int dest_is(const uint8_t *ram, const char *word)
{
    size_t len = strlen(word), k;
    if (ram[p(DEST, 0x100)] != len) return 0;
    for (k = 0; k < len; ++k) if (ram[p(DEST, 0x101u + (unsigned)k)] != (uint8_t)word[k]) return 0;
    return 1;
}

int main(void)
{
    uint8_t *ram = malloc(PR_PARAMSTR_RAM_SIZE);
    pr_paramstr_cpu c, before;
    CHECK(ram != NULL);

    /* " STING", n = 1: skip the blank at 0081h, the word is 0082h..0086h. Length byte 5 then "STING". */
    c = setup(ram, " STING", 1);
    CHECK(pr_system_paramstr_16bc(ram, PR_PARAMSTR_RAM_SIZE, &c) == PR_PARAMSTR_RETURNED && dest_is(ram, "STING"));
    /* AX takes CX (0) by XCHG; BX counted one word; DX 1 -> 0; SI past the word in the PSP, DI past the copy;
     * DS and BP restored; RETF 2 from SP 8000h -> 8006h. Flags from DEC DX (ZF PF) with CF from SUB (0) and CLD. */
    CHECK(c.ax == 0 && c.bx == 1 && c.cx == 0 && c.dx == 0 && c.si == 0x0087 && c.di == 0x0106);
    CHECK(c.ds == DS && c.es == DEST && c.bp == 0x7777 && c.cs == 0x9001 && c.ip == 0x7000 && c.sp == 0x8006);
    CHECK(c.flags == 0x0046);
    /* The frame below SP: saved BP 7777h at 7FFEh, the CALL's return 16C9h and then the pushed ES at 7FFCh/7FFAh. */
    CHECK(get(ram, SS, 0x7FFE) == 0x7777 && get(ram, SS, 0x7FFC) == DS && get(ram, SS, 0x7FFA) == PSP);

    /* n = 2 past the last word: length 0, no text; SUB 0 sets ZF PF. */
    c = setup(ram, " STING", 2);
    CHECK(pr_system_paramstr_16bc(ram, PR_PARAMSTR_RAM_SIZE, &c) == PR_PARAMSTR_RETURNED);
    CHECK(ram[p(DEST, 0x100)] == 0 && ram[p(DEST, 0x101)] == 0x5A && c.bx == 1 && c.dx == 1 && c.flags == 0x0046);

    /* "STING foo", n = 2 -> "foo"; tabs and control bytes are blanks, bytes above 7Fh are word bytes (JA is unsigned). */
    c = setup(ram, "STING foo", 2);
    CHECK(pr_system_paramstr_16bc(ram, PR_PARAMSTR_RAM_SIZE, &c) == PR_PARAMSTR_RETURNED && dest_is(ram, "foo"));
    c = setup(ram, "\t\x01" "AB\x80\x20x", 1);
    CHECK(pr_system_paramstr_16bc(ram, PR_PARAMSTR_RAM_SIZE, &c) == PR_PARAMSTR_RETURNED && dest_is(ram, "AB\x80"));
    /* An empty tail: length byte 0. */
    c = setup(ram, "", 1);
    CHECK(pr_system_paramstr_16bc(ram, PR_PARAMSTR_RAM_SIZE, &c) == PR_PARAMSTR_RETURNED && ram[p(DEST, 0x100)] == 0);
    /* Only the counted bytes are read: a count of 3 over " STING" gives "ST"; the 0Dh terminator is never read. */
    c = setup(ram, " STING", 1); ram[p(PSP, 0x80)] = 3;
    CHECK(pr_system_paramstr_16bc(ram, PR_PARAMSTR_RAM_SIZE, &c) == PR_PARAMSTR_RETURNED && dest_is(ram, "ST"));

    /* ARGUMENT: n = 0 (mode 0 is not modelled), bad SP or IP, wrong size; nothing changed. */
    c = setup(ram, " STING", 0); before = c;
    CHECK(pr_system_paramstr_16bc(ram, PR_PARAMSTR_RAM_SIZE, &c) == PR_PARAMSTR_ARGUMENT);
    put(ram, SS, 0x8004, 1);
    CHECK(pr_system_paramstr_16bc(NULL, PR_PARAMSTR_RAM_SIZE, &c) == PR_PARAMSTR_ARGUMENT);
    CHECK(pr_system_paramstr_16bc(ram, PR_PARAMSTR_RAM_SIZE - 1u, &c) == PR_PARAMSTR_ARGUMENT);
    c.sp = 0x0005; CHECK(pr_system_paramstr_16bc(ram, PR_PARAMSTR_RAM_SIZE, &c) == PR_PARAMSTR_ARGUMENT);
    c.sp = 0xFFF7; CHECK(pr_system_paramstr_16bc(ram, PR_PARAMSTR_RAM_SIZE, &c) == PR_PARAMSTR_ARGUMENT);
    c.sp = 0x8000; c.ip = 0x16BD; CHECK(pr_system_paramstr_16bc(ram, PR_PARAMSTR_RAM_SIZE, &c) == PR_PARAMSTR_ARGUMENT);
    c.ip = 0x16BC; CHECK(memcmp(&c, &before, sizeof c) == 0 && ram[p(DEST, 0x100)] == 0x5A);

    free(ram);
    printf("paramstr 16BC: %u semantic checks passed\n", checks);
    return 0;
}
