#include "poolrad/recovered/overlay_u1c_script_call_frame_31a4.h"

int pr_rec_u1c_31a4_call_dispatch_frame(pr_rec_u1c_33bd_cpu *cpu,
                                         uint8_t *ram, size_t ram_size)
{
    uint32_t code_address;
    uint32_t cs_address, ip_address;
    uint32_t destinations[4];
    uint32_t i;
    uint16_t cs_sp, ip_sp;

    if (cpu == NULL || ram == NULL ||
        (ram_size != 655360u && ram_size != 1048576u) ||
        cpu->cs != 0x3582u || cpu->ip != 0x31a4u ||
        (cpu->flags & 0x0040u) == 0u)
        return 0;

    code_address = (((uint32_t)cpu->cs << 4) + cpu->ip) & 0xfffffu;
    if (code_address >= ram_size || code_address + 1u >= ram_size ||
        code_address + 2u >= ram_size || code_address + 3u >= ram_size ||
        ram[code_address] != 0x0eu || ram[code_address + 1u] != 0xe8u ||
        ram[code_address + 2u] != 0x85u || ram[code_address + 3u] != 0xfdu)
        return 0;

    cs_sp = (uint16_t)(cpu->sp - 2u);
    ip_sp = (uint16_t)(cs_sp - 2u);
    cs_address = (((uint32_t)cpu->ss << 4) + cs_sp) & 0xfffffu;
    ip_address = (((uint32_t)cpu->ss << 4) + ip_sp) & 0xfffffu;
    destinations[0] = cs_address;
    destinations[1] = (cs_address + 1u) & 0xfffffu;
    destinations[2] = ip_address;
    destinations[3] = (ip_address + 1u) & 0xfffffu;
    for (i = 0u; i < 4u; i++) {
        if (destinations[i] >= ram_size) return 0;
    }

    cpu->sp = cs_sp;
    ram[destinations[0]] = (uint8_t)cpu->cs;
    ram[destinations[1]] = (uint8_t)(cpu->cs >> 8);
    cpu->sp = ip_sp;
    ram[destinations[2]] = 0xa8u; /* CALL return IP +31A8 */
    ram[destinations[3]] = 0x31u;
    cpu->ip = 0x2f2du;
    return 1;
}
