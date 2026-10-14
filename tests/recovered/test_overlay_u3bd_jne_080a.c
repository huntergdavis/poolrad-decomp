#include "poolrad/recovered/overlay_u3bd_jne_080a.h"

#include <assert.h>
#include <string.h>

static uint8_t ram[PR_U3BD_LOOP_RESET_RAM_BYTES];

static size_t at(uint16_t seg, uint16_t off)
{
    return (((size_t)seg << 4) + off) & 0xFFFFFu;
}

static void put_word(uint16_t seg, uint16_t off, uint16_t value)
{
    ram[at(seg, off)] = (uint8_t)value;
    ram[at(seg, (uint16_t)(off + 1u))] = (uint8_t)(value >> 8);
}

static uint16_t get_word(uint16_t seg, uint16_t off)
{
    return (uint16_t)(ram[at(seg, off)] |
                      ((uint16_t)ram[at(seg, (uint16_t)(off + 1u))] << 8));
}

static pr_u3bd_jne_080a_frame saved_frame(void)
{
    pr_u3bd_jne_080a_frame f = {
        1u, 0u, 4u, 13u, 0x3E68u, 0x004Du, 0x3EB6u, 0x3E9Eu,
        0x14A0u, 0x3DA1u, 0x1D16u, 0x2D14u, 0x7202u
    };
    return f;
}

static void setup_saved_values(const pr_u3bd_jne_080a_frame *f)
{
    memset(ram, 0, sizeof ram);
    ram[at(f->ss, (uint16_t)(f->bp - 0x14u))] = 0x0Eu;
    ram[at(f->ss, (uint16_t)(f->bp - 0x17u))] = 0u;
    put_word(f->ss, (uint16_t)(f->bp - 0x12u), 0u);
    put_word(f->ds, 0x0CAAu, 5u);
    ram[at(f->ds, 0x0CBEu)] = 4u;
}

static void saved_branch_and_callee_handoff(void)
{
    pr_u3bd_jne_080a_frame f = saved_frame();
    uint16_t ip = 0x080Au;
    setup_saved_values(&f);
    assert(pr_u3bd_jne_080a(ram, sizeof ram, &f, &ip, 1) ==
           PR_U3BD_080A_PRECALL_0211);
    assert(ip == 0x0860u && f.sp == 0x3E94u);
    assert(f.ax == 0u && f.flags == 0x7206u);
    assert(ram[at(f.ss, (uint16_t)(f.bp - 0x17u))] == 0x0Eu);
    assert(get_word(f.ss, 0x3E9Cu) == 5u);
    assert(get_word(f.ss, 0x3E9Au) == 4u);
    assert(get_word(f.ss, 0x3E98u) == 0x0Eu);
    assert(get_word(f.ss, 0x3E96u) == 0u);
    assert(get_word(f.ss, 0x3E94u) == f.cs);

    f = saved_frame();
    ip = 0x080Au;
    setup_saved_values(&f);
    assert(pr_u3bd_jne_080a(ram, sizeof ram, &f, &ip, 0) ==
           PR_U3BD_080A_CALLEE_0211);
    assert(ip == 0x0211u && f.sp == 0x3E92u);
    assert(get_word(f.ss, f.sp) == 0x0863u);
    assert(f.ax == 0u && f.flags == 0x7206u);
}

static void opposite_jne_arm_calls_local_reader(void)
{
    pr_u3bd_jne_080a_frame f = saved_frame();
    uint16_t ip = 0x080Au;
    f.ax = 0xAB00u;
    f.flags |= 0x0040u;
    memset(ram, 0, sizeof ram);
    ram[at(f.ss, (uint16_t)(f.bp - 0x0Du))] = 0x12u;
    ram[at(f.ss, (uint16_t)(f.bp - 0x0Fu))] = 0x34u;
    ram[at(f.ss, (uint16_t)(f.bp - 1u))] = 0x56u;
    assert(pr_u3bd_jne_080a(ram, sizeof ram, &f, &ip, 0) ==
           PR_U3BD_080A_CALLEE_047B);
    assert(ip == 0x047Bu && f.sp == 0x3E94u);
    assert(f.ax == 0xAB56u && f.flags == 0x7242u);
    assert(get_word(f.ss, 0x3E9Cu) == 0xAB12u);
    assert(get_word(f.ss, 0x3E9Au) == 0xAB34u);
    assert(get_word(f.ss, 0x3E98u) == 0xAB56u);
    assert(get_word(f.ss, 0x3E96u) == f.cs);
    assert(get_word(f.ss, 0x3E94u) == 0x081Cu);
}

static void zero_tile_branches_to_0865(void)
{
    pr_u3bd_jne_080a_frame f = saved_frame();
    uint16_t ip = 0x080Au;
    memset(ram, 0xA5, sizeof ram);
    ram[at(f.ss, (uint16_t)(f.bp - 0x14u))] = 0u;
    assert(pr_u3bd_jne_080a(ram, sizeof ram, &f, &ip, 0) ==
           PR_U3BD_080A_BRANCH_0865);
    assert(ip == 0x0865u && f.sp == 0x3E9Eu);
    assert(f.flags == 0x7246u);
    assert(ram[at(f.ss, (uint16_t)(f.bp - 0x17u))] == 0xA5u);
}

static void nonzero_previous_tile_calls_first(void)
{
    pr_u3bd_jne_080a_frame f = saved_frame();
    uint16_t ip = 0x080Au;
    setup_saved_values(&f);
    ram[at(f.ss, (uint16_t)(f.bp - 0x17u))] = 2u;
    put_word(f.ds, 0x0CBCu, 0x0010u);
    put_word(f.ss, (uint16_t)(f.bp - 0x12u), 3u);
    ram[at(f.ds, 0x0CD0u)] = 6u;
    assert(pr_u3bd_jne_080a(ram, sizeof ram, &f, &ip, 0) ==
           PR_U3BD_080A_FIRST_CALLEE_0211);
    assert(ip == 0x0211u && f.sp == 0x3E92u);
    assert(f.ax == 9u && f.flags == 0x7206u);
    assert(get_word(f.ss, 0x3E9Cu) == 0x0012u);
    assert(get_word(f.ss, 0x3E9Au) == 6u);
    assert(get_word(f.ss, 0x3E98u) == 2u);
    assert(get_word(f.ss, 0x3E96u) == 9u);
    assert(get_word(f.ss, 0x3E94u) == f.cs);
    assert(get_word(f.ss, 0x3E92u) == 0x0847u);
}

static void invalid_host_inputs_do_not_mutate(void)
{
    pr_u3bd_jne_080a_frame f = saved_frame();
    pr_u3bd_jne_080a_frame original = f;
    uint16_t ip = 0x080Au;
    memset(ram, 0xA5, sizeof ram);
    assert(pr_u3bd_jne_080a(NULL, sizeof ram, &f, &ip, 0) == PR_U3BD_080A_INVALID);
    assert(pr_u3bd_jne_080a(ram, sizeof ram - 1u, &f, &ip, 0) == PR_U3BD_080A_INVALID);
    assert(pr_u3bd_jne_080a(ram, sizeof ram, NULL, &ip, 0) == PR_U3BD_080A_INVALID);
    assert(pr_u3bd_jne_080a(ram, sizeof ram, &f, NULL, 0) == PR_U3BD_080A_INVALID);
    assert(pr_u3bd_jne_080a(ram, sizeof ram, &f, &ip, 2) == PR_U3BD_080A_INVALID);
    f.cs = 0x2D15u;
    assert(pr_u3bd_jne_080a(ram, sizeof ram, &f, &ip, 0) == PR_U3BD_080A_INVALID);
    f = original;
    ip = 0x080Bu;
    assert(pr_u3bd_jne_080a(ram, sizeof ram, &f, &ip, 0) == PR_U3BD_080A_INVALID);
    assert(memcmp(&f, &original, sizeof f) == 0);
    assert(ip == 0x080Bu && ram[at(f.ss, f.sp)] == 0xA5u);
}

int main(void)
{
    saved_branch_and_callee_handoff();
    opposite_jne_arm_calls_local_reader();
    zero_tile_branches_to_0865();
    nonzero_previous_tile_calls_first();
    invalid_host_inputs_do_not_mutate();
    return 0;
}
