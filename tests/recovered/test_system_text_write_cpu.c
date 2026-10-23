/* Focused synthetic test for SYSTEM Write(string) 0919 and Writeln 084A (docs/re/SYSTEM_TEXT_WRITE_CPU.md). No game
 * bytes: only the relocated data-segment word at CS:07EF; the drivers are C mirrors of a stub that zeroes BufPos. */
#include "poolrad/recovered/system_text_write_cpu.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(x)                                                                  \
    do {                                                                          \
        if (!(x)) {                                                               \
            fprintf(stderr, "system text line %d: %s\n", __LINE__, #x);           \
            exit(1);                                                              \
        }                                                                         \
        ++checks;                                                                 \
    } while (0)

enum { RAM = 1u << 20, SYS = 0x1AF8, DS0 = 0x1C7C, SS0 = 0x7000, SP0 = 0xF000, T = 0x6000, STUB = 0x5000,
       REC = 0x0100, BUF = 0x0400, STR = 0x0800 };

static uint8_t *ram;
static unsigned checks, io_calls, flush_calls, refuse;
static uint16_t io_result, flush_result;

static size_t at(uint16_t seg, uint16_t off) { return (((size_t)seg << 4) + off) & (RAM - 1u); }
static void put16(uint16_t seg, uint16_t off, uint16_t v) { ram[at(seg, off)] = (uint8_t)v; ram[at(seg, (uint16_t)(off + 1u))] = (uint8_t)(v >> 8); }
static uint16_t get16(uint16_t seg, uint16_t off) { return (uint16_t)(ram[at(seg, off)] | (ram[at(seg, (uint16_t)(off + 1u))] << 8)); }

/* The declared driver: PUSH BP; LES DI,[BP+6]; ES:[DI+8] := 0; MOV AX,result; POP BP; RETF 4. */
static int driver(void *ctx, uint8_t *r, const pr_system_text_cpu *b, pr_system_text_cpu *a)
{
    uint16_t result;
    (void)ctx; (void)r;
    if (refuse || b->cs != STUB || (b->ip != 0 && b->ip != 0x20u))
        return 1;
    result = b->ip == 0 ? io_result : flush_result;
    b->ip == 0 ? ++io_calls : ++flush_calls;
    *a = *b;
    put16(b->ss, (uint16_t)(b->sp - 2u), b->bp);
    a->di = get16(b->ss, (uint16_t)(b->sp + 4u)); a->es = get16(b->ss, (uint16_t)(b->sp + 6u));
    put16(a->es, (uint16_t)(a->di + 8u), 0);
    a->ax = result;
    a->ip = get16(b->ss, b->sp); a->cs = get16(b->ss, (uint16_t)(b->sp + 2u)); a->sp = (uint16_t)(b->sp + 8u);
    return 0;
}

static pr_system_text_cpu world(uint16_t ip, uint16_t mode, uint16_t res, uint16_t size, uint16_t pos, const char *s, uint16_t width)
{
    pr_system_text_cpu c = {0x1234u, 0x3456u, 0x5678u, 0x789Au, 0x9ABCu, 0xBCDEu, 0x7777u, SP0, SYS, ip, DS0, 0x1111u, SS0, 0x0002u};
    size_t k;
    memset(ram, 0x5A, RAM);
    io_calls = flush_calls = refuse = 0; io_result = flush_result = 0;
    put16(SYS, 0x07EFu, DS0);
    put16(DS0, 0x4426u, res);
    put16(T, REC + 2u, mode); put16(T, REC + 4u, size); put16(T, REC + 8u, pos); put16(T, REC + 0x0Au, 0x4242u);
    put16(T, REC + 0x0Cu, BUF); put16(T, REC + 0x0Eu, T);
    put16(T, REC + 0x14u, 0); put16(T, REC + 0x16u, STUB); put16(T, REC + 0x18u, 0x20u); put16(T, REC + 0x1Au, STUB);
    put16(SS0, SP0, 0x7000u); put16(SS0, SP0 + 2u, 0x9001u);
    if (ip == 0x0919u) {
        ram[at(T, STR)] = (uint8_t)strlen(s);
        for (k = 0; s[k] != '\0'; ++k)
            ram[at(T, (uint16_t)(STR + 1u + k))] = (uint8_t)s[k];
        put16(SS0, SP0 + 4u, width); put16(SS0, SP0 + 6u, STR); put16(SS0, SP0 + 8u, T);
        put16(SS0, SP0 + 10u, REC); put16(SS0, SP0 + 12u, T);
    } else {
        put16(SS0, SP0 + 4u, REC); put16(SS0, SP0 + 6u, T);
    }
    return c;
}

int main(void)
{
    pr_system_text_env env = {driver, NULL};
    pr_system_text_cpu c;
    int svc = 99;
    ram = malloc(RAM);
    if (ram == NULL)
        return 1;

    /* Write('HI':4) into an empty 8-byte buffer: two pad spaces, then the bytes; no driver call; RETF 6. */
    c = world(0x0919u, 0xD7B2u, 0, 8, 0, "HI", 4);
    CHECK(pr_system_write_str_0919(ram, RAM, &c, &env, &svc) == PR_SYSTEM_TEXT_RETURNED && svc == -1 && io_calls == 0);
    CHECK(memcmp(&ram[at(T, BUF)], "  HI", 4) == 0 && ram[at(T, BUF + 4u)] == 0x5A && get16(T, REC + 8u) == 4);
    CHECK(c.cs == 0x9001u && c.ip == 0x7000u && c.sp == SP0 + 10u && c.ss == SS0 && get16(SS0, SP0 + 10u) == REC);

    /* Write('ABC') into a 2-byte buffer: 'A','B' fill it, InOutFunc empties it (BufPos 0), 'C' lands at offset 0. */
    c = world(0x0919u, 0xD7B2u, 0, 2, 0, "ABC", 0);
    CHECK(pr_system_write_str_0919(ram, RAM, &c, &env, &svc) == PR_SYSTEM_TEXT_RETURNED && svc == 0 && io_calls == 1);
    CHECK(ram[at(T, BUF)] == 'C' && ram[at(T, BUF + 1u)] == 'B' && get16(T, REC + 8u) == 1 && get16(DS0, 0x4426u) == 0);
    CHECK(get16(SS0, (uint16_t)(SP0 - 2u)) != 0x5A5Au && c.cs == 0x9001u && c.sp == SP0 + 10u);

    /* Writeln: CR LF, BufPos 2 stored, FlushFunc called (BufPos := 0) and its AX 66h becomes InOutRes; RETF 4. */
    c = world(0x084Au, 0xD7B2u, 0, 8, 0, "", 0);
    flush_result = 0x66u;
    CHECK(pr_system_writeln_084a(ram, RAM, &c, &env, &svc) == PR_SYSTEM_TEXT_RETURNED && svc == 0 && flush_calls == 1 && io_calls == 0);
    CHECK(ram[at(T, BUF)] == 0x0D && ram[at(T, BUF + 1u)] == 0x0A && get16(T, REC + 8u) == 0 && get16(DS0, 0x4426u) == 0x66u);
    CHECK(c.ax == 0x66u && c.cs == 0x9001u && c.ip == 0x7000u && c.sp == SP0 + 8u);

    /* Writeln on an input-mode file: InOutRes := 69h, nothing put, no flush. */
    c = world(0x084Au, 0xD7B1u, 0, 8, 3, "", 0);
    CHECK(pr_system_writeln_084a(ram, RAM, &c, &env, &svc) == PR_SYSTEM_TEXT_RETURNED && svc == -1 && flush_calls == 0);
    CHECK(get16(DS0, 0x4426u) == 0x69u && ram[at(T, BUF + 3u)] == 0x5A && get16(T, REC + 8u) == 3);

    /* A pending InOutRes skips Write entirely but keeps the result code. */
    c = world(0x0919u, 0xD7B2u, 0x67u, 8, 1, "HI", 9);
    CHECK(pr_system_write_str_0919(ram, RAM, &c, &env, &svc) == PR_SYSTEM_TEXT_RETURNED && svc == -1);
    CHECK(get16(DS0, 0x4426u) == 0x67u && ram[at(T, BUF + 1u)] == 0x5A && get16(T, REC + 8u) == 1 && c.sp == SP0 + 10u);

    /* A refused FlushFunc stops at the driver entry with SERVICE. */
    c = world(0x084Au, 0xD7B2u, 0, 8, 0, "", 0);
    refuse = 1;
    CHECK(pr_system_writeln_084a(ram, RAM, &c, &env, &svc) == PR_SYSTEM_TEXT_SERVICE && svc == 1);
    CHECK(c.cs == STUB && c.ip == 0x20u && get16(SS0, c.sp) == 0x0890u && get16(SS0, (uint16_t)(c.sp + 2u)) == SYS);

    /* Bad arguments change nothing. */
    c = world(0x084Au, 0xD7B2u, 0, 8, 0, "", 0);
    CHECK(pr_system_writeln_084a(ram, RAM - 1u, &c, &env, &svc) == PR_SYSTEM_TEXT_ARGUMENT && c.ip == 0x084Au);
    CHECK(pr_system_write_str_0919(ram, RAM, &c, NULL, &svc) == PR_SYSTEM_TEXT_ARGUMENT && c.sp == SP0);

    free(ram);
    printf("system text write: %u checks\n", checks);
    return 0;
}
