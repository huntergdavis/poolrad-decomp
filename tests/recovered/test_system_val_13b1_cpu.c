/* Focused synthetic test for the raw-CPU SYSTEM Val (longint) 0AF8:13B1 + 12CE (docs/re/SYSTEM_VAL_13B1_CPU.md). No game bytes. */
#include "poolrad/recovered/system_val_13b1_cpu.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned checks;
#define CHECK(x) do { ++checks; if (!(x)) { fprintf(stderr, "val line %d: %s\n", __LINE__, #x); exit(1); } } while (0)
static size_t p(unsigned s, unsigned o) { return ((size_t)s * 16u + (o & 65535u)) & 1048575u; }
static void put(uint8_t *m, unsigned s, unsigned o, unsigned v) { size_t a = p(s, o); m[a] = (uint8_t)v; m[(a + 1u) & 1048575u] = (uint8_t)(v >> 8); }
static unsigned get(const uint8_t *m, unsigned s, unsigned o) { size_t a = p(s, o); return m[a] | (unsigned)m[(a + 1u) & 1048575u] << 8; }

enum { SS = 0x2000, STR = 0x3000, CODE = 0x4000 };
static pr_val_cpu run(uint8_t *ram, const char *text)
{
    pr_val_cpu c = {0x1234, 0x3456, 0x5678, 0x789A, 0x9ABC, 0xBCDE, 0x7777, 0x8000, 0x6000, 0x13B1, 0x5000, 0x1111, SS, 0x0002};
    size_t n = strlen(text);
    memset(ram, 0x5A, PR_VAL_RAM_SIZE);
    ram[p(STR, 0x100)] = (uint8_t)n; memcpy(ram + p(STR, 0x101), text, n);
    put(ram, SS, 0x8000, 0x7000); put(ram, SS, 0x8002, 0x9001);           /* far return */
    put(ram, SS, 0x8004, 0x0200); put(ram, SS, 0x8006, CODE);             /* @Code */
    put(ram, SS, 0x8008, 0x0100); put(ram, SS, 0x800A, STR);              /* @S */
    CHECK(pr_system_val_13b1(ram, PR_VAL_RAM_SIZE, &c) == PR_VAL_RETURNED);
    CHECK(c.cs == 0x9001 && c.ip == 0x7000 && c.sp == 0x800C && c.bp == 0x7777);
    return c;
}

int main(void)
{
    uint8_t *ram = malloc(PR_VAL_RAM_SIZE);
    pr_val_cpu c, before;
    CHECK(ram != NULL);

    /* "123": 123 = 7Bh, Code 0; every byte consumed (CX 0, the Code stored); ES:DI end on @Code (LES DI,[BP+6] at 13D8);
     * SI 0 decremented to FFFFh (no sign) and that DEC SI's flags with CLC: SF, PF (FFh), AF set: 0002h -> 0096h. */
    c = run(ram, "123");
    CHECK(c.ax == 0x007B && c.dx == 0 && get(ram, CODE, 0x200) == 0 && c.cx == 0 && c.di == 0x0200 && c.es == CODE && c.si == 0xFFFF);
    CHECK(c.flags == 0x0096);
    /* "-45": -45 = FFFFFFD3h; NOT/NOT/ADD AX,1/ADC DX,0 leaves ADC's flags (FFFFh: SF, PF), then CLC: 0086h. */
    c = run(ram, "-45");
    CHECK(c.dx == 0xFFFF && c.ax == 0xFFD3 && get(ram, CODE, 0x200) == 0 && c.flags == 0x0086);
    /* "$1F" and "$ff": hex, case-folded. "  42": leading blanks skipped. "+7": a plus sign. */
    c = run(ram, "$1F"); CHECK(c.ax == 0x001F && c.dx == 0 && get(ram, CODE, 0x200) == 0);
    c = run(ram, "$ff"); CHECK(c.ax == 0x00FF && get(ram, CODE, 0x200) == 0);
    c = run(ram, "  42"); CHECK(c.ax == 42 && get(ram, CODE, 0x200) == 0);
    c = run(ram, "+7"); CHECK(c.ax == 7 && get(ram, CODE, 0x200) == 0);
    /* Errors: Code is the 1-based position of the first bad character and DX:AX = 0; XOR DX,DX's flags (0046h). */
    c = run(ram, "12a"); CHECK(get(ram, CODE, 0x200) == 3 && c.ax == 0 && c.dx == 0 && c.flags == 0x0046);
    c = run(ram, ""); CHECK(get(ram, CODE, 0x200) == 1 && c.ax == 0);
    c = run(ram, "-"); CHECK(get(ram, CODE, 0x200) == 2);
    c = run(ram, "$"); CHECK(get(ram, CODE, 0x200) == 2);
    /* Overflow: 2147483647 parses; 2147483648 sets SF after the last step (JS), so Code 10; so does -2147483648. */
    c = run(ram, "2147483647"); CHECK(c.dx == 0x7FFF && c.ax == 0xFFFF && get(ram, CODE, 0x200) == 0);
    c = run(ram, "2147483648"); CHECK(get(ram, CODE, 0x200) == 10 && c.dx == 0 && c.ax == 0);
    c = run(ram, "-2147483648"); CHECK(get(ram, CODE, 0x200) == 11);
    /* Hex overflow: a ninth hex digit shifts a bit out of DX. */
    c = run(ram, "$100000000"); CHECK(get(ram, CODE, 0x200) == 10);

    /* ARGUMENT: rejected before any change. */
    memset(ram, 0x5A, PR_VAL_RAM_SIZE);
    {
        pr_val_cpu a = {0, 0, 0, 0, 0, 0, 0, 0x8000, 0x6000, 0x13B1, 0, 0, SS, 2};
        c = a; before = c;
        CHECK(pr_system_val_13b1(NULL, PR_VAL_RAM_SIZE, &c) == PR_VAL_ARGUMENT);
        CHECK(pr_system_val_13b1(ram, PR_VAL_RAM_SIZE - 1u, &c) == PR_VAL_ARGUMENT);
        c.sp = 0x0009; CHECK(pr_system_val_13b1(ram, PR_VAL_RAM_SIZE, &c) == PR_VAL_ARGUMENT);
        c.sp = 0xFFF5; CHECK(pr_system_val_13b1(ram, PR_VAL_RAM_SIZE, &c) == PR_VAL_ARGUMENT);
        c.sp = 0x8000; c.ip = 0x13B2; CHECK(pr_system_val_13b1(ram, PR_VAL_RAM_SIZE, &c) == PR_VAL_ARGUMENT);
        c.ip = 0x13B1; CHECK(memcmp(&c, &before, sizeof c) == 0 && ram[p(SS, 0x7FFE)] == 0x5A);
    }

    free(ram);
    printf("val 13B1: %u semantic checks passed\n", checks);
    return 0;
}
