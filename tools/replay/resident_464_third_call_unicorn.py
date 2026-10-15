#!/usr/bin/env python3
"""Optional original-byte audit of the third saved run014 RES_0464 call.

Starts at the independently measured second-call return. The third caller
prestate and resident entry are checked against saved same-run native images.
The third return is original-byte provisional until a direct native image is
supplied with --native-return. Only hashes, counts, and CPU words are printed.
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
SECOND_RETURN_SHA = "722cac9dd2a9c8e002649255183878a5687054ed65f19105f09d228ea0cf154c"
THIRD_PRE_SHA = "99fd4ea498e847b2ed225d3c399b1ebe0cb8d2db203664dd7ad3605e31c9075b"
THIRD_ENTRY_SHA = "725ba34a5bba4f591c03125530ca7f652b215ad41527b8b235563241240c3ab9"
THIRD_REPLAY_RETURN_SHA = "d5876edf3a5bab07f2bcf4531a6a0d804837a380dab89a6e3716e01da654f24b"

REGS = (
    UC_X86_REG_CS, UC_X86_REG_IP, UC_X86_REG_AX, UC_X86_REG_BX,
    UC_X86_REG_CX, UC_X86_REG_DX, UC_X86_REG_SI, UC_X86_REG_DI,
    UC_X86_REG_BP, UC_X86_REG_SP, UC_X86_REG_DS, UC_X86_REG_ES,
    UC_X86_REG_SS, UC_X86_REG_EFLAGS,
)
SECOND_RETURN_CPU = (
    0x2D14, 0x01C3, 0, 0x3E4E, 0, 0x14A0, 0x3E68,
    0x0076, 0x3E98, 0x3E98, 0x14A0, 0x3DE4, 0x1D16, 0x7246,
)
THIRD_PRE_CPU = (
    0x2D14, 0x01E4, 7, 0x3E4E, 0, 0x14A0, 0x3E68,
    0x260A, 0x3E98, 0x3E86, 0x14A0, 0x3DE4, 0x1D16, 0x7246,
)
THIRD_ENTRY_CPU = (
    0x0C88, 0x2632, 7, 0x3E4E, 0, 0x14A0, 0x3E68,
    0x260A, 0x3E98, 0x3E82, 0x14A0, 0x3DE4, 0x1D16, 0x7246,
)
THIRD_RETURN_CPU = (
    0x2D14, 0x01E9, 1, 0x3E4E, 0, 0x14A0, 0x3E68,
    0x007A, 0x3E98, 0x3E98, 0x14A0, 0x3DE4, 0x1D16, 0x7246,
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
    parser.add_argument("--native-return", type=Path,
                        help="optional direct DOS debugger +01E9 RAM image")
    args = parser.parse_args()
    run = args.run_dir
    start = verified(ROOT / "resources/DOS/POOLRAD/START.EXE", 65231,
                     START_SHA)
    ovr = (ROOT / "resources/DOS/POOLRAD/GAME.OVR").read_bytes()
    if sha256(start[0x6CAB:0x7086]).hexdigest() != BODY_SHA:
        raise ValueError("resident original body hash mismatch")
    if sha256(ovr[0x2EADB:0x2EAE0]).hexdigest() != CALL_SHA:
        raise ValueError("original third far-call hash mismatch")
    second_return = verified(run / "ram-01c3-second-res-caller-return.bin",
                             PRIVATE_BYTES, SECOND_RETURN_SHA)
    third_pre = verified(run / "ram-01e4-third-res-pre.bin", PRIVATE_BYTES,
                         THIRD_PRE_SHA)
    third_entry = verified(run / "ram-2632-third-res-entry.bin",
                           PRIVATE_BYTES, THIRD_ENTRY_SHA)
    call = physical(0x2D14, 0x01E4)
    if sha256(third_pre[call:call + 5]).hexdigest() != LOADED_CALL_SHA:
        raise ValueError("loaded third far-call hash mismatch")

    emu = Uc(UC_ARCH_X86, UC_MODE_16)
    emu.mem_map(0, RAM_BYTES)
    emu.mem_write(0, second_return + bytes(RAM_BYTES - PRIVATE_BYTES))
    for reg, value in zip(REGS, SECOND_RETURN_CPU):
        emu.reg_write(reg, value)
    stops = {}
    steps = 0
    targets = {
        call: "third_pre",
        physical(0x0C88, 0x2632): "third_entry",
        physical(0x2D14, 0x01E9): "third_return",
    }

    def on_code(engine, address, size, user):
        nonlocal steps
        del size, user
        steps += 1
        name = targets.get(address)
        if name is None or name in stops:
            return
        stops[name] = (cpu(engine), bytes(engine.mem_read(0, PRIVATE_BYTES)),
                       steps)
        if name == "third_return":
            engine.emu_stop()

    emu.hook_add(UC_HOOK_CODE, on_code)
    emu.emu_start(physical(0x2D14, 0x01C3), 0, count=200000)
    if set(stops) != set(targets.values()):
        raise AssertionError(f"expected stops missing: {sorted(stops)}")
    for name, expected_ram, expected_cpu in (
        ("third_pre", third_pre, THIRD_PRE_CPU),
        ("third_entry", third_entry, THIRD_ENTRY_CPU),
    ):
        regs, ram, stop_steps = stops[name]
        mismatches = sum(a != b for a, b in zip(ram, expected_ram))
        print(f"{name}: steps={stop_steps} RAM_mismatches={mismatches} "
              f"RAM_sha256={sha256(ram).hexdigest()}")
        print(name + "_CPU: " + " ".join(f"{v:04X}" for v in regs))
        if mismatches or regs != expected_cpu:
            raise AssertionError(f"{name} RAM/CPU mismatch")
    regs, ram, stop_steps = stops["third_return"]
    digest = sha256(ram).hexdigest()
    print(f"third_return_original_bytes: steps={stop_steps} RAM_sha256={digest}")
    print("third_return_CPU: " + " ".join(f"{v:04X}" for v in regs))
    if digest != THIRD_REPLAY_RETURN_SHA or regs != THIRD_RETURN_CPU:
        raise AssertionError("original-byte return changed")
    if args.native_return is not None:
        native = args.native_return.read_bytes()
        if len(native) != PRIVATE_BYTES:
            raise ValueError("direct native return size mismatch")
        mismatches = sum(a != b for a, b in zip(ram, native))
        print(f"direct_native_return_RAM_mismatches={mismatches}")
        if mismatches:
            raise AssertionError("direct native return differs")


if __name__ == "__main__":
    main()
