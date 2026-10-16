#include "poolrad/recovered/overlay_u7f_roll_prefix_0fd2.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

static size_t physical(uint16_t seg, uint16_t off)
{
    return (((size_t)seg << 4) + off) & (PR_U7F_0FD2_RAM_SIZE - 1u);
}

static uint16_t word(const uint8_t *ram, uint16_t seg, uint16_t off)
{
    size_t at = physical(seg, off);
    return (uint16_t)(ram[at] |
        ((uint16_t)ram[(at + 1u) & (PR_U7F_0FD2_RAM_SIZE - 1u)] << 8));
}

static void one(uint8_t *ram, uint16_t sp, uint16_t ss, uint16_t ax)
{
    pr_u7f_0fd2_cpu cpu = {ax,0x3456u,0x5678u,0x789Au,0x9ABCu,
        0xBCDEu,0x7777u,sp,0x3000u,0x0FD2u,0x1000u,0x1111u,ss,0x7202u};
    uint16_t oldbp = cpu.bp;
    uint8_t high = (uint8_t)(ax >> 8);
    memset(ram, 0xA5, PR_U7F_0FD2_RAM_SIZE);
    assert(pr_u7f_roll_prefix_0fd2(ram,PR_U7F_0FD2_RAM_SIZE,&cpu)==PR_U7F_0FD2_AT_CALL);
    assert(cpu.cs==0x3000u && cpu.ip==0x0FDEu);
    assert(cpu.sp==(uint16_t)(sp-8u) && cpu.bp==(uint16_t)(sp-2u));
    assert(word(ram,ss,(uint16_t)(sp-2u))==oldbp);
    assert(word(ram,ss,(uint16_t)(sp-6u))==(uint16_t)((high<<8)|1u));
    assert(word(ram,ss,(uint16_t)(sp-8u))==(uint16_t)((high<<8)|0x14u));
    assert(cpu.ax==(uint16_t)((high<<8)|0x14u));
    assert(cpu.bx==0x3456u && cpu.cx==0x5678u && cpu.dx==0x789Au);
}

int main(void)
{
    uint8_t *ram = malloc(PR_U7F_0FD2_RAM_SIZE);
    pr_u7f_0fd2_cpu cpu={0}, before=cpu;
    assert(ram!=NULL);
    assert(pr_u7f_roll_prefix_0fd2(NULL,PR_U7F_0FD2_RAM_SIZE,&cpu)==PR_U7F_0FD2_ARGUMENT);
    assert(pr_u7f_roll_prefix_0fd2(ram,0u,&cpu)==PR_U7F_0FD2_ARGUMENT);
    assert(pr_u7f_roll_prefix_0fd2(ram,PR_U7F_0FD2_RAM_SIZE,NULL)==PR_U7F_0FD2_ARGUMENT);
    assert(memcmp(&cpu,&before,sizeof cpu)==0);
    one(ram,0x8000u,0x2000u,0xA5CDu);
    one(ram,0x0000u,0x2000u,0x12FFu);
    one(ram,0x0001u,0x2000u,0x34AAu);
    one(ram,0x0002u,0x2000u,0x5600u);
    one(ram,0xFFFEu,0x2000u,0x7801u);
    one(ram,0xFFFFu,0x2000u,0x9A14u);
    one(ram,0x0100u,0xFFFFu,0xBCDEu);
    free(ram);
    return 0;
}
