/* Focused synthetic test for the raw-CPU resident 0709:0714 space skip (docs/re/RES_0709_SKIP_SPACES_0714_CPU.md). No game
 * bytes. */
#include "poolrad/recovered/resident_0709_skip_spaces_0714_cpu.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(x)                                                                  \
    do {                                                                          \
        if (!(x)) {                                                               \
            fprintf(stderr, "skip spaces line %d: %s\n", __LINE__, #x);           \
            exit(1);                                                              \
        }                                                                         \
        ++checks;                                                                 \
    } while (0)

enum { RAM = 1u << 20, SS0 = 0x2000, SP0 = 0x8000, P = 0x6000 };

static uint8_t *ram;
static unsigned checks;

static size_t at(uint16_t offset) { return (((size_t)SS0 << 4) + offset) & (RAM - 1u); }

/* Parent frame P: index byte P-103h, limit byte P-101h, characters from P-100h. */
static pr_r0709_skip_cpu frame(uint8_t index, uint8_t limit, const char *text)
{
    pr_r0709_skip_cpu cpu = {0x1234u, 0x3456u, 0x5678u, 0x789Au, 0x9ABCu, 0xBCDEu, 0x7777u, SP0, 0x0709u, 0x0714u, 0x1000u, 0x1111u, SS0, 0x7202u};
    memset(ram, 0xA5, RAM);
    ram[at(SP0)] = 0x00; ram[at(SP0 + 1u)] = 0x70; ram[at(SP0 + 2u)] = 0x01; ram[at(SP0 + 3u)] = 0x70;
    ram[at(SP0 + 4u)] = (uint8_t)P; ram[at(SP0 + 5u)] = (uint8_t)(P >> 8);
    ram[at(P - 0x103u)] = index; ram[at(P - 0x101u)] = limit;
    memcpy(&ram[at(P - 0x100u)], text, strlen(text));
    return cpu;
}

int main(void)
{
    pr_r0709_skip_cpu cpu;
    ram = malloc(RAM);
    if (ram == NULL)
        return 1;

    /* "  AB" from index 0, limit 4: skips two spaces to index 2; CMP 'A',20h = 21h sets only PF; DI = P + 2. */
    cpu = frame(0, 4, "  AB");
    CHECK(pr_r0709_skip_spaces_0714_cpu(ram, RAM, &cpu, 2) == PR_R0709_SKIP_OK && ram[at(P - 0x103u)] == 2);
    CHECK(cpu.ax == 0x0002u && cpu.di == P + 2u && cpu.flags == 0x7206u);
    CHECK(cpu.cs == 0x7001u && cpu.ip == 0x7000u && cpu.sp == SP0 + 6u && cpu.bp == 0x7777u);

    /* All spaces up to the limit: stops when index 3 reaches limit 3 (CMP 3,3: ZF, PF); AH stays from the last XOR (0). */
    cpu = frame(0, 3, "   x");
    CHECK(pr_r0709_skip_spaces_0714_cpu(ram, RAM, &cpu, 3) == PR_R0709_SKIP_OK && ram[at(P - 0x103u)] == 3);
    CHECK(cpu.ax == 0x0003u && cpu.di == P && cpu.flags == 0x7246u);

    /* Index already at or above the limit: no character test, AH kept, DI = P. */
    cpu = frame(5, 5, " ");
    CHECK(pr_r0709_skip_spaces_0714_cpu(ram, RAM, &cpu, 0) == PR_R0709_SKIP_OK && cpu.ax == 0x1205u && cpu.di == P && ram[at(P - 0x103u)] == 5);

    /* Cap 1 with three spaces: the second JMP 0717 stops with LIMIT at +0717, index 2. */
    cpu = frame(0, 4, "   B");
    CHECK(pr_r0709_skip_spaces_0714_cpu(ram, RAM, &cpu, 1) == PR_R0709_SKIP_LIMIT && cpu.ip == 0x0717u && ram[at(P - 0x103u)] == 2);
    CHECK(cpu.sp == SP0 - 2u && cpu.bp == SP0 - 2u);

    /* Guards change nothing. */
    cpu = frame(0, 4, "  AB");
    cpu.ip = 0x0715u;
    CHECK(pr_r0709_skip_spaces_0714_cpu(ram, RAM, &cpu, 9) == PR_R0709_SKIP_ARGUMENT && cpu.sp == SP0 && ram[at(P - 0x103u)] == 0);
    CHECK(pr_r0709_skip_spaces_0714_cpu(NULL, RAM, &cpu, 9) == PR_R0709_SKIP_ARGUMENT);
    CHECK(pr_r0709_skip_spaces_0714_cpu(ram, RAM - 1u, &cpu, 9) == PR_R0709_SKIP_ARGUMENT);

    free(ram);
    printf("skip spaces 0714: %u checks passed\n", checks);
    return 0;
}
