#include "poolrad/recovered/overlay_u7d_update_03a3.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

static size_t at(uint16_t seg, uint16_t off)
{
    return (((size_t)seg << 4) + off) & (PR_U7D_03A3_RAM_SIZE - 1u);
}

static void put(uint8_t *ram, uint16_t seg, uint16_t off, uint16_t value)
{
    size_t p = at(seg, off);
    ram[p] = (uint8_t)value;
    ram[(p + 1u) & (PR_U7D_03A3_RAM_SIZE - 1u)] = (uint8_t)(value >> 8);
}

static uint16_t word(const uint8_t *ram, uint16_t seg, uint16_t off)
{
    size_t p = at(seg, off);
    return (uint16_t)(ram[p] |
        ((uint16_t)ram[(p + 1u) & (PR_U7D_03A3_RAM_SIZE - 1u)] << 8));
}

static pr_u7d_03a3_cpu frame(uint8_t *ram, uint16_t sp)
{
    pr_u7d_03a3_cpu cpu = {0x1234u,0x5678u,0x9ABCu,0xDEF0u,0x1111u,
                           0x2222u,0x3333u,sp,0x3000u,0x03A3u,0x1000u,
                           0x4000u,0x2000u,0x7202u};
    put(ram,cpu.ss,sp,0xCAFEu);
    put(ram,cpu.ss,(uint16_t)(sp+2u),0xBEEFu);
    if (sp>=0xFFFEu) {
        size_t linear_cs = (at(cpu.ss,sp)+2u) & (PR_U7D_03A3_RAM_SIZE-1u);
        ram[linear_cs]=0xADu;
        ram[(linear_cs+1u)&(PR_U7D_03A3_RAM_SIZE-1u)]=0xDEu;
    }
    put(ram,cpu.ds,0x5D96u,0u);
    put(ram,cpu.ds,0x5D98u,0u);
    for (unsigned i=0;i<7u;++i) {
        put(ram,cpu.ds,(uint16_t)(0x363Au+2u*i),0xFFFFu);
        put(ram,cpu.ds,(uint16_t)(0x6DC2u+2u*i),0u);
    }
    return cpu;
}

int main(void)
{
    uint8_t *ram = calloc(PR_U7D_03A3_RAM_SIZE,1u);
    pr_u7d_03a3_cpu cpu, before;
    assert(ram != NULL);
    cpu=frame(ram,0x8000u);
    before=cpu;
    assert(pr_u7d_update_03a3(NULL,PR_U7D_03A3_RAM_SIZE,&cpu)==PR_U7D_03A3_ARGUMENT);
    assert(pr_u7d_update_03a3(ram,0u,&cpu)==PR_U7D_03A3_ARGUMENT);
    assert(pr_u7d_update_03a3(ram,PR_U7D_03A3_RAM_SIZE,NULL)==PR_U7D_03A3_ARGUMENT);
    assert(memcmp(&cpu,&before,sizeof cpu)==0);

    /* Zero pending count follows JBE; preserve the carry callee's AX/DX. */
    assert(pr_u7d_update_03a3(ram,PR_U7D_03A3_RAM_SIZE,&cpu)==PR_U7D_03A3_OK);
    assert(cpu.ip==0xCAFEu && cpu.cs==0xBEEFu && cpu.sp==0x8004u);
    assert(cpu.bp==0x3333u && cpu.ds==0x1000u && cpu.ss==0x2000u);
    assert(word(ram,cpu.ds,0x6DCCu)==0u);

    /* Nonzero pending count multiplies, clears, and caps the accumulator. */
    cpu=frame(ram,0x8100u);
    put(ram,cpu.ds,0x3642u,4u);
    put(ram,cpu.ds,0x6DCAu,90u);
    put(ram,cpu.ds,0x6DCCu,3u);
    assert(pr_u7d_update_03a3(ram,PR_U7D_03A3_RAM_SIZE,&cpu)==PR_U7D_03A3_OK);
    assert(word(ram,cpu.ds,0x6DCAu)==99u && word(ram,cpu.ds,0x6DCCu)==0u);
    assert(cpu.ax==0u && cpu.dx==0u && cpu.sp==0x8104u);

    /* RETF reads CS at wrapped SS:0000, not the linear decoy. */
    cpu=frame(ram,0xFFFEu);
    assert(pr_u7d_update_03a3(ram,PR_U7D_03A3_RAM_SIZE,&cpu)==PR_U7D_03A3_OK);
    assert(cpu.sp==2u && cpu.ip==0xCAFEu && cpu.cs==0xBEEFu);

    /* IP itself straddles FFFF; CS is separately read at SS:0001. */
    cpu=frame(ram,0xFFFFu);
    assert(pr_u7d_update_03a3(ram,PR_U7D_03A3_RAM_SIZE,&cpu)==PR_U7D_03A3_OK);
    assert(cpu.sp==3u && cpu.ip==0xCAFEu && cpu.cs==0xBEEFu);

    /* Entry SP 0008 makes the composed +02F2 RETF pop IP at FFFE and
     * CS at wrapped SS:0000, rather than from the linear decoy. */
    cpu=frame(ram,0x0008u);
    {
        size_t linear_cs=(at(cpu.ss,0xFFFEu)+2u)&(PR_U7D_03A3_RAM_SIZE-1u);
        ram[linear_cs]=0xADu;
        ram[(linear_cs+1u)&(PR_U7D_03A3_RAM_SIZE-1u)]=0xDEu;
    }
    assert(pr_u7d_update_03a3(ram,PR_U7D_03A3_RAM_SIZE,&cpu)==PR_U7D_03A3_OK);
    assert(cpu.sp==0x000Cu && cpu.ip==0xCAFEu && cpu.cs==0xBEEFu);

    /* The carry callee can rewrite its return through a shared array/frame. */
    cpu=frame(ram,0x6DD4u);
    cpu.ss=cpu.ds;
    put(ram,cpu.ds,0x3642u,4u);
    assert(pr_u7d_update_03a3(ram,PR_U7D_03A3_RAM_SIZE,&cpu)==
           PR_U7D_03A3_CALLEE_TRANSFER);
    assert(cpu.ip!=0x03AFu || cpu.cs!=0x3000u);
    free(ram);
    return 0;
}
