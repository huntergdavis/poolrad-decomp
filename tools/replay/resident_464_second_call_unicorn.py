#!/usr/bin/env python3
"""Optional original-byte audit of the second saved run014 RES_0464 call.

Execute ignored loaded bytes from the native +01BE caller prestate and compare
the full RAM and all 14 CPU fields at +2632 and +01C3. No bytes are printed.
"""

import argparse
from hashlib import sha256
from pathlib import Path

from unicorn import Uc, UC_ARCH_X86, UC_HOOK_CODE, UC_MODE_16
from unicorn.x86_const import (
    UC_X86_REG_AX, UC_X86_REG_BX, UC_X86_REG_CX, UC_X86_REG_DX,
    UC_X86_REG_SI, UC_X86_REG_DI, UC_X86_REG_BP, UC_X86_REG_SP,
    UC_X86_REG_CS, UC_X86_REG_IP, UC_X86_REG_DS, UC_X86_REG_ES,
    UC_X86_REG_SS, UC_X86_REG_EFLAGS,
)

ROOT = Path(__file__).resolve().parents[2]
RAM_BYTES = 1 << 20
PRIVATE_BYTES = 655360
START_SHA = "d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd"
BODY_SHA = "ee35f98f513d5144e92e55e2afafbe176bba9ab65acc905ea151c19c5a9d713a"
CALL_SHA = "a734c1740b95619bedad6325a3ac8d1413024a9cbb9302bbf3c15831cfb20cfe"
LOADED_CALL_SHA = "0d18bd5c35579c163826a0665cd04cf9a8b7b054c3f930e7633f0c6bdc4352f4"
PRE_SHA = "4fab836fb3c536e5ea78e6be81765c3c01aa8d86566a919c810eb749763beac0"
ENTRY_SHA = "7a4593ce6f8043fda34209915e84f1f936bdb6dccbc9e72d2fb24a20a5c2cd2a"
RETURN_SHA = "722cac9dd2a9c8e002649255183878a5687054ed65f19105f09d228ea0cf154c"

REGS = (
    UC_X86_REG_CS, UC_X86_REG_IP, UC_X86_REG_AX, UC_X86_REG_BX,
    UC_X86_REG_CX, UC_X86_REG_DX, UC_X86_REG_SI, UC_X86_REG_DI,
    UC_X86_REG_BP, UC_X86_REG_SP, UC_X86_REG_DS, UC_X86_REG_ES,
    UC_X86_REG_SS, UC_X86_REG_EFLAGS,
)
CALLER_PRE = (
    0x2D14, 0x01BE, 0x000F, 0x3E4E, 0, 0x14A0, 0x3E68,
    0x260A, 0x3E98, 0x3E86, 0x14A0, 0x3DE4, 0x1D16, 0x7246,
)
RESIDENT_ENTRY = (
    0x0C88, 0x2632, 0x000F, 0x3E4E, 0, 0x14A0, 0x3E68,
    0x260A, 0x3E98, 0x3E82, 0x14A0, 0x3DE4, 0x1D16, 0x7246,
)
CALLER_RETURN = (
    0x2D14, 0x01C3, 0, 0x3E4E, 0, 0x14A0, 0x3E68,
    0x0076, 0x3E98, 0x3E98, 0x14A0, 0x3DE4, 0x1D16, 0x7246,
)


def physical(segment, offset):
    return ((segment << 4) + offset) & 0xFFFFF


def verified(path, size, digest):
    data = path.read_bytes()
    if len(data) != size or sha256(data).hexdigest() != digest:
        raise ValueError(f"hash/size mismatch: {path.name}")
    return data


def cpu(emu):
    return tuple(emu.reg_read(reg) & 0xFFFF for reg in REGS)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("run_dir", type=Path, help="ignored run014 captures")
    args = parser.parse_args()
    run = args.run_dir
    start = verified(ROOT / "resources/DOS/POOLRAD/START.EXE", 65231,
                     START_SHA)
    ovr = (ROOT / "resources/DOS/POOLRAD/GAME.OVR").read_bytes()
    if sha256(start[0x6CAB:0x7086]).hexdigest() != BODY_SHA:
        raise ValueError("resident original body hash mismatch")
    if sha256(ovr[0x2EAB5:0x2EABA]).hexdigest() != CALL_SHA:
        raise ValueError("original second far-call hash mismatch")
    pre = verified(run / "ram-01be-second-res-pre.bin", PRIVATE_BYTES,
                   PRE_SHA)
    entered = verified(run / "ram-2632-second-res-entry.bin", PRIVATE_BYTES,
                       ENTRY_SHA)
    returned = verified(run / "ram-01c3-second-res-caller-return.bin",
                        PRIVATE_BYTES, RETURN_SHA)
    call = physical(0x2D14, 0x01BE)
    if sha256(pre[call:call + 5]).hexdigest() != LOADED_CALL_SHA:
        raise ValueError("loaded second far-call hash mismatch")

    emu = Uc(UC_ARCH_X86, UC_MODE_16)
    emu.mem_map(0, RAM_BYTES)
    emu.mem_write(0, pre + bytes(RAM_BYTES - PRIVATE_BYTES))
    for reg, value in zip(REGS, CALLER_PRE):
        emu.reg_write(reg, value)
    stops = {}
    steps = 0

    def on_code(engine, address, size, user):
        nonlocal steps
        del size, user
        steps += 1
        if address == physical(0x0C88, 0x2632) and "resident_entry" not in stops:
            stops["resident_entry"] = (cpu(engine), bytes(
                engine.mem_read(0, PRIVATE_BYTES)), steps)
        if address == physical(0x2D14, 0x01C3):
            stops["caller_return"] = (cpu(engine), bytes(
                engine.mem_read(0, PRIVATE_BYTES)), steps)
            engine.emu_stop()

    emu.hook_add(UC_HOOK_CODE, on_code)
    emu.emu_start(call, 0, count=200000)
    if set(stops) != {"resident_entry", "caller_return"}:
        raise AssertionError(f"expected stops missing: {sorted(stops)}")
    for name, native, expected_cpu in (
        ("resident_entry", entered, RESIDENT_ENTRY),
        ("caller_return", returned, CALLER_RETURN),
    ):
        regs, ram, stop_steps = stops[name]
        mismatches = sum(a != b for a, b in zip(ram, native))
        print(f"{name}: steps={stop_steps} RAM_mismatches={mismatches} "
              f"RAM_sha256={sha256(ram).hexdigest()}")
        print(name + "_CPU: " + " ".join(f"{v:04X}" for v in regs))
        if mismatches or regs != expected_cpu:
            raise AssertionError(f"{name} RAM/CPU mismatch")


if __name__ == "__main__":
    main()
