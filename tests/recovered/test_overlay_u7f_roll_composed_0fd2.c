#include "poolrad/recovered/overlay_u7f_roll_composed_0fd2.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define RAM_SIZE PR_U7F_0FDE_RAM_SIZE

static size_t physical(uint16_t seg, uint16_t off)
{
    return (((size_t)seg << 4) + off) & (RAM_SIZE - 1u);
}

static uint16_t word(const uint8_t *ram, uint16_t seg, uint16_t off)
{
    size_t at = physical(seg, off);
    return (uint16_t)(ram[at] | ((uint16_t)ram[(at + 1u) & (RAM_SIZE - 1u)] << 8));
}

static void put16(uint8_t *ram, uint16_t seg, uint16_t off, uint16_t value)
{
    size_t at = physical(seg, off);
    ram[at] = (uint8_t)value;
    ram[(at + 1u) & (RAM_SIZE - 1u)] = (uint8_t)(value >> 8);
}

typedef struct { int mode, seen; uint8_t roll; uint16_t word; } hook_state;   /* word: the loader-relocated segment half of the LCALL at 0FDE (CS:0FE1) */

static int controlled_roll(void *context, uint8_t *ram, size_t size,
                           pr_u7f_0fde_cpu *cpu)
{
    hook_state *state = context;
    uint16_t return_ip, return_cs;
    assert(size == RAM_SIZE && cpu->cs == state->word && cpu->ip == 0x0048u);   /* entered at the code word's segment (17498aef), not a constant 00B0 */
    assert(word(ram, cpu->ss, (uint16_t)(cpu->sp + 4u)) == 0x0014u);
    assert(word(ram, cpu->ss, (uint16_t)(cpu->sp + 6u)) == 0x0001u);
    state->seen++;
    if (state->mode == 1) return 0;
    if (state->mode == 2) {
        cpu->cs = 0x4321u;
        cpu->ip = 0x1234u;
        return 1;
    }
    return_ip = word(ram, cpu->ss, cpu->sp);
    return_cs = word(ram, cpu->ss, (uint16_t)(cpu->sp + 2u));
    cpu->ax = (uint16_t)(0xD200u | state->roll);
    cpu->bx = 0xA432u;
    cpu->dx = 0x6142u;
    cpu->ds = 0x4111u;
    cpu->es = 0x6222u;
    cpu->flags = 0x7202u;
    ram[physical(cpu->ds, 0x1234u)] = 0x5Au;
    cpu->sp = (uint16_t)(cpu->sp + 8u); /* RETF 4 */
    cpu->cs = return_cs;
    cpu->ip = return_ip;
    return 1;
}

int main(void)
{
    uint8_t *ram = malloc(RAM_SIZE);
    pr_u7f_0fd2_composed_cpu cpu, before;
    hook_state hook = {0,0,19u,0x00B0u};
    static const struct { uint8_t roll, tier; } cases[] = {
        {0u,3u}, {1u,1u}, {14u,1u}, {15u,2u}, {19u,2u},
        {20u,3u}, {255u,3u}
    };
    size_t i;
    assert(ram != NULL);
    memset(ram, 0xA5, RAM_SIZE);
    cpu = (pr_u7f_0fd2_composed_cpu){0x0000u,0x3456u,0x5678u,0x789Au,
        0x9ABCu,0xBCDEu,0x7777u,0x8000u,0x3000u,0x0FD2u,0x1000u,
        0x2222u,0x2000u,0x7202u};
    put16(ram, cpu.ss, cpu.sp, 0x2468u);
    put16(ram, cpu.ss, (uint16_t)(cpu.sp + 2u), 0x1357u);
    put16(ram, cpu.cs, 0x0FE1u, hook.word);
    before = cpu;
    assert(pr_u7f_roll_composed_0fd2(NULL,RAM_SIZE,&cpu,controlled_roll,&hook)
           == PR_U7F_0FD2_COMPOSED_ARGUMENT);
    assert(pr_u7f_roll_composed_0fd2(ram,0u,&cpu,controlled_roll,&hook)
           == PR_U7F_0FD2_COMPOSED_ARGUMENT);
    assert(pr_u7f_roll_composed_0fd2(ram,RAM_SIZE,NULL,controlled_roll,&hook)
           == PR_U7F_0FD2_COMPOSED_ARGUMENT);
    assert(pr_u7f_roll_composed_0fd2(ram,RAM_SIZE,&cpu,NULL,&hook)
           == PR_U7F_0FD2_COMPOSED_ARGUMENT);
    assert(memcmp(&cpu,&before,sizeof cpu)==0 && hook.seen==0);
    for (i = 0u; i < sizeof cases / sizeof cases[0]; ++i) {
        memset(ram, 0xA5, RAM_SIZE);
        cpu = before; hook.mode = 0; hook.seen = 0; hook.roll = cases[i].roll;
        put16(ram, cpu.ss, cpu.sp, 0x2468u);
        put16(ram, cpu.ss, (uint16_t)(cpu.sp + 2u), 0x1357u);
        put16(ram, cpu.cs, 0x0FE1u, hook.word);
        assert(pr_u7f_roll_composed_0fd2(ram,RAM_SIZE,&cpu,controlled_roll,&hook)
               == PR_U7F_0FD2_COMPOSED_RETURNED);
        assert(hook.seen==1 && cpu.ax==(uint16_t)(0xD200u|cases[i].tier));
        assert(cpu.bp==0x7777u && cpu.sp==0x8004u);
        assert(cpu.cs==0x1357u && cpu.ip==0x2468u);
        assert(cpu.bx==0xA432u && cpu.dx==0x6142u);
        assert(cpu.ds==0x4111u && cpu.es==0x6222u);
        assert(ram[physical(cpu.ds,0x1234u)]==0x5Au);
        assert(ram[physical(cpu.ss,0x7FFCu)]==cases[i].roll);
        assert(ram[physical(cpu.ss,0x7FFDu)]==cases[i].tier);
    }
    for (i = 0u; i < 2u; ++i) {
        memset(ram, 0xA5, RAM_SIZE);
        cpu = before; cpu.sp = i ? 0xFFFFu : 0u;
        hook.mode = 0; hook.seen = 0; hook.roll = 14u;
        put16(ram, cpu.ss, cpu.sp, 0x2468u);
        put16(ram, cpu.ss, (uint16_t)(cpu.sp + 2u), 0x1357u);
        put16(ram, cpu.cs, 0x0FE1u, hook.word);
        assert(pr_u7f_roll_composed_0fd2(ram,RAM_SIZE,&cpu,controlled_roll,&hook)
               == PR_U7F_0FD2_COMPOSED_RETURNED);
        assert(hook.seen==1 && cpu.ax==0xD201u);
        assert(cpu.cs==0x1357u && cpu.ip==0x2468u);
        assert(cpu.sp==(uint16_t)((i ? 0xFFFFu : 0u)+4u));
    }

    cpu = before; hook.mode = 1; hook.seen = 0;
    assert(pr_u7f_roll_composed_0fd2(ram,RAM_SIZE,&cpu,controlled_roll,&hook)
           == PR_U7F_0FD2_COMPOSED_CALL_ABORT);
    assert(hook.seen==1 && cpu.cs==hook.word && cpu.ip==0x0048u);
    cpu = before; hook.mode = 2; hook.seen = 0;
    assert(pr_u7f_roll_composed_0fd2(ram,RAM_SIZE,&cpu,controlled_roll,&hook)
           == PR_U7F_0FD2_COMPOSED_TRANSFER);
    assert(hook.seen==1 && cpu.cs==0x4321u && cpu.ip==0x1234u);
    /* other load bases: the relocated word names another segment and the roll is entered there (a constant 00B0 would fail the callback's assert) */
    {
        static const uint16_t words[] = {0x1234u, 0x0001u, 0xFFB0u, 0x10B0u};
        for (i = 0u; i < sizeof words / sizeof words[0]; ++i) {
            memset(ram, 0xA5, RAM_SIZE);
            cpu = before; hook.mode = 0; hook.seen = 0; hook.roll = 14u; hook.word = words[i];
            put16(ram, cpu.ss, cpu.sp, 0x2468u);
            put16(ram, cpu.ss, (uint16_t)(cpu.sp + 2u), 0x1357u);
            put16(ram, cpu.cs, 0x0FE1u, hook.word);
            assert(pr_u7f_roll_composed_0fd2(ram,RAM_SIZE,&cpu,controlled_roll,&hook)
                   == PR_U7F_0FD2_COMPOSED_RETURNED);
            assert(hook.seen==1 && cpu.ax==0xD201u && cpu.cs==0x1357u && cpu.ip==0x2468u);
        }
    }
    free(ram);
    return 0;
}
