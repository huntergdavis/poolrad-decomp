/* Focused synthetic test for the raw-CPU unit 0x25 +0411 LOAD MONSTER wrapper (docs/re/OVL_0025_0411_CPU.md). No game bytes: the call words, the
 * stub, the heap variables and the callee's code words are this test's. The 0EC4 callee is made to stop at its first call (CODE), so what is observable is
 * everything the wrapper does up to and around the call: the nil store, GetMem, the stored record pointer, and the frame handed to 0EC4. The full
 * closure runs in the audit. */
#include "poolrad/recovered/overlay_u25_entry5_0411_cpu.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned checks;
#define CHECK(x) do { ++checks; if (!(x)) { fprintf(stderr, "0411 line %d: %s\n", __LINE__, #x); exit(1); } } while (0)
#define RAM PR_BUS_RAM_SIZE
static size_t p(unsigned s, unsigned o) { return ((size_t)s * 16u + (o & 65535u)) & 1048575u; }
static void put(uint8_t *m, unsigned s, unsigned o, unsigned v) { size_t a = p(s, o); m[a] = (uint8_t)v; m[(a + 1u) & 1048575u] = (uint8_t)(v >> 8); }
static unsigned get(const uint8_t *m, unsigned s, unsigned o) { size_t a = p(s, o); return (unsigned)(m[a] | (m[(a + 1u) & 1048575u] << 8)); }

enum { W = 0x8800, SYS = 0x1AF8, STUB = 0x107C, U6F = 0x9000, DS = 0x1C7C, SS = 0x2000, SP0 = 0xF000, HEAP = 0x3000, OUT = 0x5800 };
static pr_u6f_0ec4_env env;
static pr_ega ega;

static pr_u25_0411_cpu setup(uint8_t *m, unsigned heap_paras)
{
    pr_u25_0411_cpu c = {0x1234, 0x2222, 0x3333, 0x4444, 0x5555, 0x6666, 0x789A, SP0, W, 0x0411, DS, 0x8888, SS, 0x0202};
    memset(m, 0x5A, RAM);
    m[p(W, 0x0427)] = 0x9A; put(m, W, 0x0428, 0x023F); put(m, W, 0x042A, SYS);
    m[p(W, 0x0444)] = 0x9A; put(m, W, 0x0445, 0x003E); put(m, W, 0x0447, STUB);
    m[p(STUB, 0x003E)] = 0xEA; put(m, STUB, 0x003F, 0x0EC4); put(m, STUB, 0x0041, U6F);        /* the declared trampoline */
    /* the heap: an empty free list, HeapPtr at HEAP:0, HeapEnd heap_paras above, the default HeapError (offset 00A9, this SYSTEM segment) */
    put(m, DS, 0x4404, 0); put(m, DS, 0x4406, HEAP); put(m, DS, 0x4408, 0); put(m, DS, 0x440A, HEAP + heap_paras);
    put(m, DS, 0x440C, 0); put(m, DS, 0x440E, HEAP); put(m, DS, 0x4414, 0x00A9); put(m, DS, 0x4416, SYS);
    /* frame: return, A6, AA, AE, id word */
    put(m, SS, SP0, 0x7000); put(m, SS, SP0 + 2, 0x9001); put(m, SS, SP0 + 4, 0x0110); put(m, SS, SP0 + 6, OUT);
    put(m, SS, SP0 + 8, 0x0120); put(m, SS, SP0 + 10, OUT); put(m, SS, SP0 + 12, 0x0100); put(m, SS, SP0 + 14, OUT); put(m, SS, SP0 + 16, 0xAB07);
    put(m, OUT, 0x0100, 0xEEEE); put(m, OUT, 0x0102, 0xEEEE); put(m, OUT, 0x0110, 0xEEEE); put(m, OUT, 0x0120, 0xEEEE);
    return c;
}

int main(void)
{
    static uint8_t ram[RAM], copy[RAM];
    pr_bus bus = {ram, &ega, NULL, NULL, NULL};
    pr_u25_0411_diagnostic d;
    pr_u25_0411_cpu c, before;
    unsigned bad, in_sp;
    pr_ega_reset(&ega);
    memset(&env, 0, sizeof env); env.heap_steps = 1000;
    /* GetMem succeeds, the record pointer is stored, and 0EC4 (its first call opcode is not 9Ah) stops with CODE: the wrapper reports it unchanged. */
    c = setup(ram, 0x100);
    CHECK(pr_u25_entry5_0411_cpu(&bus, RAM, &c, &env, &d) == PR_U25_0411_CODE);
    CHECK(d.callee == PR_U25_0411_ENTRY6 && d.callee_status == PR_U6F_0EC4_CODE && d.calls == 1u && d.site == 0x0444u);
    CHECK(get(ram, OUT, 0x0100) != 0xEEEEu || get(ram, OUT, 0x0102) != 0xEEEEu);             /* AE^ holds GetMem's pointer, not the junk and not nil */
    CHECK(get(ram, OUT, 0x0102) >= HEAP && get(ram, OUT, 0x0102) < HEAP + 0x100u + 0x20u);      /* its segment is in the heap */
    in_sp = (SP0 - 2u - 4u - 6u - 4u) & 0xFFFFu;                                                /* BP, locals, id + R, the far return frame */
    CHECK(get(ram, SS, in_sp) == 0x0449u && get(ram, SS, in_sp + 2u) == W);                      /* the frame 0EC4 was entered with */
    CHECK(get(ram, SS, in_sp + 4u) == get(ram, OUT, 0x0100) && get(ram, SS, in_sp + 6u) == get(ram, OUT, 0x0102));
    CHECK((get(ram, SS, in_sp + 8u) & 0xFFu) == 0x07u);                                         /* the id byte */
    CHECK(get(ram, OUT, 0x0110) == 0xEEEEu && get(ram, OUT, 0x0120) == 0xEEEEu);                 /* A6^ and AA^ not yet written */
    /* GetMem's opcode is not the original: CODE at the call, its argument pushed but no return frame. */
    c = setup(ram, 0x100); ram[p(W, 0x0427)] = 0x90;
    CHECK(pr_u25_entry5_0411_cpu(&bus, RAM, &c, &env, &d) == PR_U25_0411_CODE && c.ip == 0x0427u && d.calls == 0u && d.callee == PR_U25_0411_GETMEM);
    CHECK(c.sp == ((SP0 - 2u - 4u - 2u) & 0xFFFFu) && get(ram, SS, c.sp) == 0x011Du);
    CHECK(get(ram, OUT, 0x0100) == 0u && get(ram, OUT, 0x0102) == 0u);                           /* the nil store happened */
    /* The stub entry is not a far JMP, or not to 0EC4: CODE at the call after GetMem ran. */
    c = setup(ram, 0x100); ram[p(STUB, 0x003E)] = 0x90;
    CHECK(pr_u25_entry5_0411_cpu(&bus, RAM, &c, &env, &d) == PR_U25_0411_CODE && c.ip == 0x0444u && d.calls == 1u && d.callee == PR_U25_0411_ENTRY6);
    c = setup(ram, 0x100); put(ram, STUB, 0x003F, 0x0EC5);
    CHECK(pr_u25_entry5_0411_cpu(&bus, RAM, &c, &env, &d) == PR_U25_0411_CODE && c.ip == 0x0444u && d.calls == 1u);
    c = setup(ram, 0x100); ram[p(W, 0x0444)] = 0x90;
    CHECK(pr_u25_entry5_0411_cpu(&bus, RAM, &c, &env, &d) == PR_U25_0411_CODE && c.ip == 0x0444u);
    /* The stub segment word is read live: pointing it elsewhere makes the stub bytes 5Ah, not a JMP. */
    c = setup(ram, 0x100); put(ram, W, 0x0447, STUB + 1u);
    CHECK(pr_u25_entry5_0411_cpu(&bus, RAM, &c, &env, &d) == PR_U25_0411_CODE && c.ip == 0x0444u);
    /* GetMem fails (no heap): RunError, AE^ stays nil. */
    c = setup(ram, 0x0002);
    CHECK(pr_u25_entry5_0411_cpu(&bus, RAM, &c, &env, &d) == PR_U25_0411_RUNERROR && d.callee == PR_U25_0411_GETMEM && d.calls == 0u);
    CHECK(get(ram, OUT, 0x0100) == 0u && get(ram, OUT, 0x0102) == 0u && c.ax == 203u);
    /* Host rejections are atomic. */
    for (bad = 0; bad < 4u; ++bad) {
        c = setup(ram, 0x100);
        if (bad == 0) c.ip = 0x0412;
        if (bad == 1) c.sp = 0x0A03;
        if (bad == 2) c.sp = 0xFFEF;
        if (bad == 3) c.sp = 0;
        before = c; memcpy(copy, ram, RAM);
        CHECK(pr_u25_entry5_0411_cpu(&bus, RAM, &c, &env, &d) == PR_U25_0411_ARGUMENT);
        CHECK(memcmp(&c, &before, sizeof c) == 0 && memcmp(copy, ram, RAM) == 0);
    }
    c = setup(ram, 0x100);
    CHECK(pr_u25_entry5_0411_cpu(NULL, RAM, &c, &env, &d) == PR_U25_0411_ARGUMENT);
    CHECK(pr_u25_entry5_0411_cpu(&bus, RAM - 1u, &c, &env, &d) == PR_U25_0411_ARGUMENT);
    CHECK(pr_u25_entry5_0411_cpu(&bus, RAM, NULL, &env, &d) == PR_U25_0411_ARGUMENT);
    CHECK(pr_u25_entry5_0411_cpu(&bus, RAM, &c, NULL, &d) == PR_U25_0411_ARGUMENT);
    printf("%u semantic checks passed\n", checks);
    return 0;
}
