#!/usr/bin/env python3
"""Original-byte first RES_0464:2632 call from one ignored run014 RAM image.

This is an optional local audit. It executes loaded original bytes from the
native +0198 prestate and compares the full RAM at +2A07 and +019D with
same-run native snapshots. It prints addresses, counts, hashes, and registers.
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
PRE_SHA = "76451936350f53c3cc9732cc1ec48fed834aa0e54a25292f76609280ecd0ca21"
RETURN_SHA = "0bc7e0b0b162c6b1768d22177b721917236df0b673c183612fa2b49f3d42a45b"

REGS = (
    UC_X86_REG_CS, UC_X86_REG_IP, UC_X86_REG_AX, UC_X86_REG_BX,
    UC_X86_REG_CX, UC_X86_REG_DX, UC_X86_REG_SI, UC_X86_REG_DI,
    UC_X86_REG_BP, UC_X86_REG_SP, UC_X86_REG_DS, UC_X86_REG_ES,
    UC_X86_REG_SS, UC_X86_REG_EFLAGS,
)
ENTRY = (
    0x2D14, 0x0198, 0x000B, 0x0000, 0x0004, 0x000F, 0x02DF,
    0x260A, 0x3E98, 0x3E86, 0x14A0, 0x3996, 0x1D16, 0x7246,
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
    parser.add_argument("run_dir", type=Path, help="ignored run014 capture directory")
    args = parser.parse_args()
    run = args.run_dir
    start = verified(ROOT / "resources/DOS/POOLRAD/START.EXE", 65231, START_SHA)
    original_call = (ROOT / "resources/DOS/POOLRAD/GAME.OVR").read_bytes()
    if sha256(start[0x6CAB:0x7086]).hexdigest() != BODY_SHA:
        raise ValueError("resident original body hash mismatch")
    if sha256(original_call[0x2EA8F:0x2EA94]).hexdigest() != CALL_SHA:
        raise ValueError("original first far-call hash mismatch")
    pre = verified(run / "ram-0198-first-res-pre.bin", PRIVATE_BYTES, PRE_SHA)
    loop = verified(run / "ram-2a07-loop-exit.bin", PRIVATE_BYTES, RETURN_SHA)
    returned = verified(run / "ram-019d-resident-return.bin",
                        PRIVATE_BYTES, RETURN_SHA)
    if sha256(pre[physical(0x2D14, 0x0198):
                  physical(0x2D14, 0x0198) + 5]).hexdigest() != LOADED_CALL_SHA:
        raise ValueError("loaded far-call hash mismatch")

    emu = Uc(UC_ARCH_X86, UC_MODE_16)
    emu.mem_map(0, RAM_BYTES)
    emu.mem_write(0, pre + bytes(RAM_BYTES - PRIVATE_BYTES))
    for reg, value in zip(REGS, ENTRY):
        emu.reg_write(reg, value)
    captured = {}
    steps = 0

    def on_code(engine, address, size, user):
        nonlocal steps
        del size, user
        steps += 1
        if address == physical(0x0C88, 0x2632) and "resident_entry" not in captured:
            captured["resident_entry"] = (cpu(engine), bytes(
                engine.mem_read(0, PRIVATE_BYTES)), steps)
        if address == physical(0x0C88, 0x2A07) and "loop_exit" not in captured:
            captured["loop_exit"] = (cpu(engine), bytes(
                engine.mem_read(0, PRIVATE_BYTES)), steps)
        if address == physical(0x2D14, 0x019D):
            captured["caller_return"] = (cpu(engine), bytes(
                engine.mem_read(0, PRIVATE_BYTES)), steps)
            engine.emu_stop()

    emu.hook_add(UC_HOOK_CODE, on_code)
    emu.emu_start(physical(0x2D14, 0x0198), 0, count=200000)
    if set(captured) != {"resident_entry", "loop_exit", "caller_return"}:
        raise AssertionError(f"expected stops missing: {sorted(captured)}")
    for name, native in (("loop_exit", loop), ("caller_return", returned)):
        regs, ram, stop_steps = captured[name]
        mismatches = sum(a != b for a, b in zip(ram, native))
        print(f"{name}: steps={stop_steps} RAM_mismatches={mismatches} "
              f"RAM_sha256={sha256(ram).hexdigest()}")
        print(name + "_CPU: " + " ".join(f"{value:04X}" for value in regs))
        if mismatches:
            raise AssertionError(f"{name} RAM mismatch")
    entry_cpu, entry_ram, _ = captured["resident_entry"]
    print("resident_entry_RAM_sha256=" + sha256(entry_ram).hexdigest())
    print("resident_entry_CPU: " + " ".join(f"{value:04X}" for value in entry_cpu))


if __name__ == "__main__":
    main()
