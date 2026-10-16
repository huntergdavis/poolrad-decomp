/* Synthetic full-state frames for original GAME.OVR U9E +1081. */
#include "poolrad/recovered/overlay_u9e_entry_1081.h"

#include <assert.h>
#include <stdlib.h>

#define RAM_SIZE PR_U9E_1081_RAM_SIZE

typedef struct {
    unsigned calls;
    unsigned mode;
} fixture;

static size_t at(uint16_t seg, uint16_t off)
{
    return (((size_t)seg << 4) + off) & (RAM_SIZE - 1u);
}

static uint16_t word(const uint8_t *ram, uint16_t seg, uint16_t off)
{
    size_t a = at(seg, off);
    return (uint16_t)(ram[a] | ((uint16_t)ram[(a + 1u) & (RAM_SIZE - 1u)] << 8));
}

static void put_word(uint8_t *ram, uint16_t seg, uint16_t off, uint16_t v)
{
    size_t a = at(seg, off);
    ram[a] = (uint8_t)v;
    ram[(a + 1u) & (RAM_SIZE - 1u)] = (uint8_t)(v >> 8);
}

static int callee(void *context, pr_u9e_1081_call_kind kind,
                  uint8_t *ram, size_t size, pr_u9e_1081_cpu *cpu)
{
    fixture *f = context;
    uint16_t sp = cpu->sp;
    uint16_t ret_ip = word(ram, cpu->ss, sp);
    uint16_t ret_cs = word(ram, cpu->ss, (uint16_t)(sp + 2u));
    assert(size == RAM_SIZE && kind == f->calls);
    assert(cpu->bp == 0x7FFEu && cpu->ss == 0xE000u);
    assert(ret_cs == 0xF000u);
    switch (kind) {
    case PR_U9E_1081_CALL_LOOKUP:
        assert(cpu->cs == 0xD100u && cpu->ip == 0x2199u);
        assert(ret_ip == 0x109Fu && sp == 0x7FE2u);
        assert(word(ram, cpu->ss, (uint16_t)(sp + 4u)) == 0x7FF9u);
        assert(word(ram, cpu->ss, (uint16_t)(sp + 6u)) == cpu->ss);
        assert(word(ram, cpu->ss, (uint16_t)(sp + 8u)) == 0xA52Au);
        assert(word(ram, cpu->ss, (uint16_t)(sp + 10u)) == 0x1234u);
        assert(word(ram, cpu->ss, (uint16_t)(sp + 12u)) == 0x4321u);
        put_word(ram, cpu->ss, (uint16_t)(cpu->bp - 5u), 0xABCDu);
        put_word(ram, cpu->ss, (uint16_t)(cpu->bp - 3u), 0xBCDEu);
        cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | (f->mode == 0u ? 0u : 1u));
        cpu->sp = (uint16_t)(sp + 14u); /* RES_00BA_2199 RETF 10. */
        break;
    case PR_U9E_1081_CALL_COPY:
        assert(cpu->cs == 0xD200u && cpu->ip == 0x0B0Bu);
        assert(ret_ip == 0x10B8u && sp == 0x7FE0u);
        assert(word(ram, cpu->ss, (uint16_t)(sp + 4u)) == 0x1078u);
        assert(word(ram, cpu->ss, (uint16_t)(sp + 6u)) == 0xF000u);
        assert(word(ram, cpu->ss, (uint16_t)(sp + 8u)) == 0x7FF0u);
        assert(word(ram, cpu->ss, (uint16_t)(sp + 10u)) == 0xE000u);
        assert(word(ram, cpu->ss, (uint16_t)(sp + 12u)) == 0x1234u);
        assert(word(ram, cpu->ss, (uint16_t)(sp + 14u)) == 0x4321u);
        cpu->ax = 0x4CAAu;
        cpu->sp = (uint16_t)(sp + 16u);
        break;
    case PR_U9E_1081_CALL_EMIT:
        assert(cpu->cs == 0xD100u && cpu->ip == 0x16ECu);
        assert(ret_ip == 0x10C3u && sp == 0x7FE8u);
        assert(word(ram, cpu->ss, (uint16_t)(sp + 4u)) == 0x4C01u);
        assert(word(ram, cpu->ss, (uint16_t)(sp + 6u)) == 0x4C0Au);
        cpu->ax = 0xDEADu;
        cpu->sp = (uint16_t)(sp + 8u);
        break;
    case PR_U9E_1081_CALL_LOCAL_0026:
        assert(cpu->cs == 0xF000u && cpu->ip == 0x0026u);
        assert(ret_ip == 0x10D7u && sp == 0x7FE2u);
        assert(word(ram, cpu->ss, (uint16_t)(sp + 4u)) == 0xABCDu);
        assert(word(ram, cpu->ss, (uint16_t)(sp + 6u)) == 0xBCDEu);
        assert(word(ram, cpu->ss, (uint16_t)(sp + 8u)) == 0xDE2Au);
        assert(word(ram, cpu->ss, (uint16_t)(sp + 10u)) == 0x1234u);
        assert(word(ram, cpu->ss, (uint16_t)(sp + 12u)) == 0x4321u);
        cpu->sp = (uint16_t)(sp + 14u);
        break;
    }
    ++f->calls;
    if (f->mode == 2u && kind == PR_U9E_1081_CALL_LOOKUP)
        return 0;
    cpu->cs = ret_cs;
    cpu->ip = ret_ip;
    return 1;
}

static void run(unsigned mode)
{
    uint8_t *ram = calloc(RAM_SIZE, 1u);
    pr_u9e_1081_cpu cpu = {0};
    fixture f = {0u, mode};
    pr_u9e_1081_io io = {0xD100u, 0xD200u, callee, &f};
    pr_u9e_1081_call_kind stopped = PR_U9E_1081_CALL_EMIT;
    pr_u9e_1081_status status;
    assert(ram != NULL);
    cpu.ax = 0xA5CCu; cpu.bp = 0x7777u; cpu.sp = 0x8000u;
    cpu.cs = 0xF000u; cpu.ip = 0x1081u; cpu.ss = 0xE000u;
    cpu.ds = 0x3000u; cpu.es = 0x4000u; cpu.flags = 0x0202u;
    put_word(ram, cpu.ss, 0x8000u, 0x4567u);
    put_word(ram, cpu.ss, 0x8002u, 0x9876u);
    put_word(ram, cpu.ss, 0x8004u, 0xBE2Au);
    put_word(ram, cpu.ss, 0x8006u, 0x1234u);
    put_word(ram, cpu.ss, 0x8008u, 0x4321u);
    status = pr_u9e_entry_1081(ram, RAM_SIZE, &cpu, &io, &stopped);
    if (mode == 2u) {
        assert(status == PR_U9E_1081_CALLEE_FAILED);
        assert(f.calls == 1u && stopped == PR_U9E_1081_CALL_LOOKUP);
        assert(cpu.cs == 0xD100u && cpu.ip == 0x2199u);
    } else {
        assert(status == PR_U9E_1081_RETURNED);
        assert(f.calls == (mode == 0u ? 1u : 4u));
        assert(stopped == (mode == 0u ? PR_U9E_1081_CALL_LOOKUP :
                                             PR_U9E_1081_CALL_LOCAL_0026));
        assert(cpu.ax == (mode == 0u ? 0xA500u : 0xDE01u));
        assert(cpu.bp == 0x7777u && cpu.sp == 0x800Au);
        assert(cpu.ip == 0x4567u && cpu.cs == 0x9876u);
        assert(ram[at(cpu.ss, 0x7FFDu)] == (mode == 0u ? 0u : 1u));
        assert(word(ram, cpu.ss, 0x7FF9u) == 0xABCDu);
        assert(word(ram, cpu.ss, 0x7FFBu) == 0xBCDEu);
    }
    free(ram);
}

int main(void)
{
    run(0u);
    run(1u);
    run(2u);
    return 0;
}
