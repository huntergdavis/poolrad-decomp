#!/usr/bin/env python3
"""Original-byte positive-index fourth resident path to pre-JL +2699.

The +2656 input is original-byte replay from a directly captured +020A
caller prestate. Only address, hash, count, and CPU data is printed.
Optional output remains under ignored resources/.
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
SPAN_SHA = "0a0bee66e226ed987b134eb5e457a3310a2a190c1f009673cd2823fc2771c00d"
PRE_SHA = "8d65f2916cda57e7e5c6a4d71a70404e716b2e275c5489c132b48f3ed1371200"
POSITIVE_SHA = "9cf87e06c28a8293f625239b02ebb82244ad9cf9f38453394fbf91510d3d5438"

REGS = (
    UC_X86_REG_CS, UC_X86_REG_IP, UC_X86_REG_AX, UC_X86_REG_BX,
    UC_X86_REG_CX, UC_X86_REG_DX, UC_X86_REG_SI, UC_X86_REG_DI,
    UC_X86_REG_BP, UC_X86_REG_SP, UC_X86_REG_DS, UC_X86_REG_ES,
    UC_X86_REG_SS, UC_X86_REG_EFLAGS,
)
PRE_CPU = (
    0x0C88, 0x2656, 0x3DEC, 0x3E4E, 0, 0x14A0, 0x2612,
    0x3E80, 0x3E80, 0x3E5C, 0x14A0, 0x1D16, 0x1D16, 0x7202,
)
STOPS = {
    (0x0C88, 0x265A): (
        "pre_jge", 2, PRE_SHA, 0,
        (0x0C88, 0x265A, 0x3DEC, 0x3E4E, 0, 0x14A0, 0x2612,
         0x3E80, 0x3E80, 0x3E5C, 0x14A0, 0x1D16, 0x1D16, 0x7202),
    ),
    (0x0C88, 0x266E): (
        "positive_arm", 3, PRE_SHA, 0,
        (0x0C88, 0x266E, 0x3DEC, 0x3E4E, 0, 0x14A0, 0x2612,
         0x3E80, 0x3E80, 0x3E5C, 0x14A0, 0x1D16, 0x1D16, 0x7202),
    ),
    (0x0C88, 0x2679): (
        "shared", 7, POSITIVE_SHA, 1,
        (0x0C88, 0x2679, 2, 0x3E4E, 0, 0x14A0, 0x2612,
         0x3E80, 0x3E80, 0x3E5C, 0x14A0, 0x1D16, 0x1D16, 0x7246),
    ),
    (0x0C88, 0x2699): (
        "pre_jl", 21, POSITIVE_SHA, 1,
        (0x0C88, 0x2699, 21, 0, 13, 0, 0x2612,
         8, 0x3E80, 0x3E5C, 0x14A0, 0x3DE4, 0x1D16, 0x7246),
    ),
}


def physical(segment, offset):
    return ((segment << 4) + offset) & 0xFFFFF


def checked(data, digest, name):
    if sha256(data).hexdigest() != digest:
        raise ValueError(f"{name} hash mismatch")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("pre", type=Path, help="ignored +2656 replay RAM")
    parser.add_argument("--native-pre-jge", type=Path)
    parser.add_argument("--native-positive", type=Path)
    parser.add_argument("--native-pre-jl", type=Path)
    parser.add_argument("--output", type=Path,
                        help="write +2699 RAM under ignored resources/")
    args = parser.parse_args()
    pre = args.pre.read_bytes()
    if len(pre) != PRIVATE_BYTES:
        raise ValueError("starting RAM size mismatch")
    checked(pre, PRE_SHA, "starting RAM")
    start = (ROOT / "resources/DOS/POOLRAD/START.EXE").read_bytes()
    if len(start) != 65231:
        raise ValueError("START.EXE size mismatch")
    checked(start, START_SHA, "START.EXE")
    checked(start[0x6CCF:0x6D12], SPAN_SHA, "original resident span")
    checked(pre[physical(0x0C88, 0x2656):
                physical(0x0C88, 0x2699)],
            SPAN_SHA, "loaded resident span")

    emu = Uc(UC_ARCH_X86, UC_MODE_16)
    emu.mem_map(0, RAM_BYTES)
    emu.mem_write(0, pre + bytes(RAM_BYTES - PRIVATE_BYTES))
    for reg, value in zip(REGS, PRE_CPU):
        emu.reg_write(reg, value)
    targets = {
        physical(segment, offset): detail
        for (segment, offset), detail in STOPS.items()
    }
    observed = {}
    steps = 0

    def on_code(engine, address, size, user):
        nonlocal steps
        del size, user
        steps += 1
        detail = targets.get(address)
        if detail is None:
            return
        name = detail[0]
        observed[name] = (
            bytes(engine.mem_read(0, PRIVATE_BYTES)),
            tuple(engine.reg_read(reg) & 0xFFFF for reg in REGS),
            steps,
        )
        if name == "pre_jl":
            engine.emu_stop()

    emu.hook_add(UC_HOOK_CODE, on_code)
    emu.emu_start(physical(0x0C88, 0x2656), 0, count=100)
    if set(observed) != {detail[0] for detail in STOPS.values()}:
        raise AssertionError("positive path missed an expected stop")
    native = {
        "pre_jge": args.native_pre_jge,
        "positive_arm": args.native_positive,
        "pre_jl": args.native_pre_jl,
    }
    for name, expected_step, expected_sha, expected_changed, expected_cpu \
            in STOPS.values():
        ram, cpu, step = observed[name]
        changed = sum(a != b for a, b in zip(pre, ram))
        digest = sha256(ram).hexdigest()
        print(f"{name}: steps={step} changed_bytes={changed} RAM_sha256={digest}")
        print(name + "_CPU: " + " ".join(f"{value:04X}" for value in cpu))
        if step != expected_step or digest != expected_sha or \
           changed != expected_changed or cpu != expected_cpu:
            raise AssertionError(f"{name} original-byte state changed")
        if native.get(name) is not None:
            native_ram = native[name].read_bytes()
            if len(native_ram) != PRIVATE_BYTES:
                raise ValueError(f"{name} direct native RAM size mismatch")
            mismatches = sum(a != b for a, b in zip(ram, native_ram))
            print(f"{name}_direct_native_RAM_mismatches={mismatches}")
            if mismatches:
                raise AssertionError(f"{name} direct native RAM differs")
    if args.output is not None:
        destination = args.output.resolve()
        if not destination.is_relative_to((ROOT / "resources").resolve()):
            raise ValueError("output must be under ignored resources/")
        destination.parent.mkdir(parents=True, exist_ok=True)
        destination.write_bytes(observed["pre_jl"][0])


if __name__ == "__main__":
    main()
