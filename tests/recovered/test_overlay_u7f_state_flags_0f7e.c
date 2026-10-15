#include "poolrad/recovered/overlay_u7f_state_flags_0f7e.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"line %d: %s\n",__LINE__,#x); exit(1); } } while (0)
static void put(uint8_t *ram, size_t at, uint16_t value)
{
    ram[at] = (uint8_t)value;
    ram[at+1u] = (uint8_t)(value >> 8u);
}
int main(void)
{
    static const uint16_t heads[] = {0,1,0x80,0x100,0x8000,0xFFFF};
    static const uint16_t arithmetic[] = {0x44,0,0,4,0x84,0x84};
    uint8_t *ram = malloc(PR_U7F_0F7E_RAM_SIZE);
    unsigned mask, form, same, cases = 0;
    CHECK(ram != NULL);
    for (mask=0; mask<128u; ++mask) for (form=0; form<6u; ++form)
        for (same=0; same<2u; ++same) {
            uint8_t input[32];
            unsigned i;
            pr_u7f_0f7e_cpu cpu = {0x1234,0x3456,0x5678,0x789A,0x9ABC,
                0xBCDE,0x7777,0x8000,0x3000,0x0F7E,0x1000,0x1111,0x2000,0x7603};
            memset(ram,0xA5,PR_U7F_0F7E_RAM_SIZE);
            for (i=0; i<7u; ++i) {
                uint16_t value = (mask & (1u << i)) ? (uint16_t)(1u << (i+1u)) : 0u;
                put(ram,0x167F4u+4u*i,(i%2u) ? 0u : value);
                put(ram,0x167F6u+4u*i,(i%2u) ? value : 0u);
            }
            put(ram,0x16810,(mask%2u) ? heads[form] : 0u);
            put(ram,0x16812,(mask%2u) ? 0u : heads[form]);
            memcpy(input,ram+0x167F4,sizeof input);
            put(ram,0x28000,0x7000); put(ram,0x28002,0x3000);
            put(ram,0x28004,same ? 0x100u : 0x200u);
            put(ram,0x28006,same ? 0x4000u : 0x5000u);
            put(ram,0x28008,0x100); put(ram,0x2800A,0x4000);
            CHECK(pr_u7f_state_flags_0f7e(ram,PR_U7F_0F7E_RAM_SIZE,&cpu)==PR_U7F_0F7E_OK);
            CHECK(ram[0x40100] == (uint8_t)(mask != 0u || (same && heads[form] != 0u)));
            if (!same) CHECK(ram[0x50200] == (uint8_t)(heads[form] != 0u));
            CHECK(memcmp(input,ram+0x167F4,sizeof input)==0);
            CHECK(cpu.ax==heads[form] && cpu.flags==(uint16_t)(0x7602u|arithmetic[form]));
            CHECK(cpu.bx==0x3456 && cpu.cx==0x5678 && cpu.dx==0x789A && cpu.si==0x9ABC);
            CHECK(cpu.bp==0x7777 && cpu.sp==0x800C && cpu.cs==0x3000 && cpu.ip==0x7000);
            CHECK(cpu.ds==0x1000 && cpu.ss==0x2000);
            ++cases;
        }
    {
        pr_u7f_0f7e_cpu cpu = {0}, before = cpu;
        CHECK(pr_u7f_state_flags_0f7e(NULL,PR_U7F_0F7E_RAM_SIZE,&cpu)==PR_U7F_0F7E_ARGUMENT);
        CHECK(pr_u7f_state_flags_0f7e(ram,0u,&cpu)==PR_U7F_0F7E_ARGUMENT);
        CHECK(pr_u7f_state_flags_0f7e(ram,PR_U7F_0F7E_RAM_SIZE,NULL)==PR_U7F_0F7E_ARGUMENT);
        CHECK(memcmp(&before,&cpu,sizeof cpu)==0);
    }
    free(ram);
    printf("u7F +0F7E: %u independent mask/head/output cases and guards passed\n",cases);
    return 0;
}
