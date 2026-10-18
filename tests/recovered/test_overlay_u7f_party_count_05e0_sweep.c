/* Focused synthetic sweep for the raw-CPU U7F +05E0 party count (docs/re/U7F_PARTY_COUNT_05E0.md): random party lists,
 * an independent count of members whose status byte (+84h) is 0 or B3h, entry flags with AF set (OR leaves AF clear), the
 * frame bytes and every byte outside the stack window. No game bytes. */
#include "poolrad/recovered/overlay_u7f_party_count_05e0.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RAM PR_U7F_05E0_RAM_SIZE
#define CHECK(x)                                                                  \
    do {                                                                          \
        if (!(x)) {                                                               \
            fprintf(stderr, "u7f 05e0 sweep line %d: %s\n", __LINE__, #x);        \
            exit(1);                                                              \
        }                                                                         \
        ++checks;                                                                 \
    } while (0)

enum { CS = 0x8800, DS = 0x1800, SS = 0x2800, PARTY = 0x5000, SP0 = 0x8000, BP1 = SP0 - 2, BP0 = 0x7777, RET_IP = 0x7000, RET_CS = 0x9001 };

static uint8_t *ram, *want;
static unsigned checks;

static size_t at(unsigned seg, unsigned off) { return (((size_t)seg << 4) + (off & 0xFFFFu)) & (RAM - 1u); }
static void w8(uint8_t *m, unsigned seg, unsigned off, unsigned v) { m[at(seg, off)] = (uint8_t)v; }
static void w16(uint8_t *m, unsigned seg, unsigned off, unsigned v) { w8(m, seg, off, v & 0xFFu); w8(m, seg, off + 1u, v >> 8); }
static unsigned r16(unsigned seg, unsigned off) { return ram[at(seg, off)] | (unsigned)ram[at(seg, off + 1u)] << 8; }

static void count(const uint8_t *statuses, unsigned n, unsigned flags)
{
    pr_u7f_05e0_cpu cpu = {0x1234u, 0x3456u, 0x5678u, 0x789Au, 0x9ABCu, 0xBCDEu, BP0, SP0, CS, 0x05E0u, DS, 0x4444u, SS, (uint16_t)flags};
    unsigned expected = 0u;
    memset(ram, 0x5C, RAM);
    for (unsigned k = 0u; k < n; ++k) {
        unsigned off = 0x0100u + 0x200u * k, next = k + 1u < n ? off + 0x200u : 0u;
        w8(ram, PARTY, off + 0x84u, statuses[k]);
        w16(ram, PARTY, off + 0x104u, next); w16(ram, PARTY, off + 0x106u, next ? PARTY : 0u);
        expected += statuses[k] == 0u || statuses[k] == 0xB3u;
    }
    w16(ram, DS, 0x5D96u, n ? 0x0100u : 0u); w16(ram, DS, 0x5D98u, n ? PARTY : 0u);
    w16(ram, SS, SP0, RET_IP); w16(ram, SS, SP0 + 2u, RET_CS);
    memcpy(want, ram, RAM);
    CHECK(pr_u7f_party_count_05e0(ram, RAM, &cpu) == PR_U7F_05E0_OK);
    CHECK((cpu.ax & 0xFFu) == (expected & 0xFFu) && ram[at(SS, BP1 - 6u)] == (expected & 0xFFu) && ram[at(SS, BP1 - 1u)] == (expected & 0xFFu));
    CHECK(r16(SS, BP1 - 5u) == 0u && r16(SS, BP1 - 3u) == 0u && r16(SS, BP1) == BP0);                 /* the walk ended at nil */
    CHECK(cpu.flags == ((flags & ~0x08D5u) | 0x0044u));                    /* OR's zero: ZF, PF; CF, OF, SF and AF clear */
    CHECK(cpu.bp == BP0 && cpu.sp == SP0 + 4u && cpu.cs == RET_CS && cpu.ip == RET_IP && cpu.ds == DS && cpu.ss == SS);
    CHECK(cpu.dx == 0u && (cpu.ax >> 8) == 0u);                            /* DX:AX last held the nil pointer */
    for (unsigned off = BP1 - 6u; off < SP0 + 4u; ++off) want[at(SS, off)] = ram[at(SS, off)];
    CHECK(memcmp(ram, want, RAM) == 0);
}

int main(void)
{
    static const uint8_t pool[] = {0x00, 0x00, 0xB3, 0xB3, 0x01, 0xB2, 0xB4, 0xFF, 0x80, 0x33};
    static const unsigned flags[] = {0x7202u, 0x0ED7u, 0x0013u, 0x0647u};
    uint8_t statuses[16];
    unsigned seed = 0x05E0u;
    ram = malloc(RAM);
    want = malloc(RAM);
    CHECK(ram != NULL && want != NULL);
    for (unsigned n = 0u; n < 2000u; ++n) {
        unsigned len;
        seed = seed * 1103515245u + 12345u; len = (seed >> 16) % 14u;
        for (unsigned k = 0u; k < len; ++k) { seed = seed * 1103515245u + 12345u; statuses[k] = pool[(seed >> 16) % 10u]; }
        count(statuses, len, flags[n & 3u]);
    }
    free(ram);
    free(want);
    printf("u7f +05e0 sweep: %u checks passed\n", checks);
    return 0;
}
