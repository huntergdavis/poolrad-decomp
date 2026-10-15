#!/usr/bin/env python3
"""Hash-locked original-byte fourth resident +275B JL decision.

Prints address, count, RAM hash, and CPU only. Input remains ignored.
"""
import argparse
from hashlib import sha256
from pathlib import Path
from unicorn import Uc, UC_ARCH_X86, UC_HOOK_CODE, UC_MODE_16
from unicorn.x86_const import (
    UC_X86_REG_CS, UC_X86_REG_IP, UC_X86_REG_AX, UC_X86_REG_BX,
    UC_X86_REG_CX, UC_X86_REG_DX, UC_X86_REG_SI, UC_X86_REG_DI,
    UC_X86_REG_BP, UC_X86_REG_SP, UC_X86_REG_DS, UC_X86_REG_ES,
    UC_X86_REG_SS, UC_X86_REG_EFLAGS,
)

ROOT = Path(__file__).resolve().parents[2]
PRIVATE_BYTES = 655360
START_SHA = "d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd"
SPAN_SHA = "4cd53691e3aeeb340b82a72a9a6a63018ab03ed3c8bd0a6effc95a6a1d4a7d20"
RAM_SHA = "95c91ee38f22f5d3808a851ddfdb055aa2733b70bfd7bf3ba897ed8bed7f621c"
REGS = (UC_X86_REG_CS, UC_X86_REG_IP, UC_X86_REG_AX, UC_X86_REG_BX,
        UC_X86_REG_CX, UC_X86_REG_DX, UC_X86_REG_SI, UC_X86_REG_DI,
        UC_X86_REG_BP, UC_X86_REG_SP, UC_X86_REG_DS, UC_X86_REG_ES,
        UC_X86_REG_SS, UC_X86_REG_EFLAGS)
PRE_CPU = (0x0C88, 0x275B, 168, 0, 104, 0, 0x2612, 8,
           0x3E80, 0x3E5C, 0x14A0, 0x3DE4, 0x1D16, 0x7246)
POST_CPU = (0x0C88, 0x275D, 168, 0, 104, 0, 0x2612, 8,
            0x3E80, 0x3E5C, 0x14A0, 0x3DE4, 0x1D16, 0x7246)


def checked(data, digest, label):
    if sha256(data).hexdigest() != digest:
        raise ValueError(label + " hash mismatch")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("pre", type=Path)
    parser.add_argument("--native-pre", type=Path)
    parser.add_argument("--native-fallthrough", type=Path)
    args = parser.parse_args()
    pre = args.pre.read_bytes()
    if len(pre) != PRIVATE_BYTES:
        raise ValueError("starting RAM size mismatch")
    checked(pre, RAM_SHA, "starting RAM")
    start = (ROOT / "resources/DOS/POOLRAD/START.EXE").read_bytes()
    if len(start) != 65231:
        raise ValueError("START.EXE size mismatch")
    checked(start, START_SHA, "START.EXE")
    checked(start[0x6DD4:0x6DD6], SPAN_SHA, "file span")
    base = 0x0C88 << 4
    checked(pre[base + 0x275B:base + 0x275D], SPAN_SHA, "loaded span")
    if args.native_pre is not None:
        if args.native_pre.read_bytes() != pre:
            raise AssertionError("direct +275B RAM differs")
    emu = Uc(UC_ARCH_X86, UC_MODE_16)
    emu.mem_map(0, 1 << 20)
    emu.mem_write(0, pre + bytes((1 << 20) - PRIVATE_BYTES))
    for reg, value in zip(REGS, PRE_CPU):
        emu.reg_write(reg, value)
    observed = None
    steps = 0

    def on_code(engine, address, size, user):
        nonlocal observed, steps
        del size, user
        steps += 1
        if address == base + 0x275D:
            observed = (bytes(engine.mem_read(0, PRIVATE_BYTES)),
                        tuple(engine.reg_read(reg) & 0xFFFF for reg in REGS))
            engine.emu_stop()

    emu.hook_add(UC_HOOK_CODE, on_code)
    emu.emu_start(base + 0x275B, 0, count=4)
    if observed is None:
        raise AssertionError("saved JL did not fall through")
    ram, cpu = observed
    digest = sha256(ram).hexdigest()
    changed = sum(a != b for a, b in zip(pre, ram))
    print(f"+275D: steps={steps} changed_bytes={changed} RAM_sha256={digest}")
    print("CPU: " + " ".join(f"{value:04X}" for value in cpu))
    if steps != 2 or digest != RAM_SHA or changed != 0 or cpu != POST_CPU:
        raise AssertionError("original-byte +275B decision changed")
    if args.native_fallthrough is not None:
        direct = args.native_fallthrough.read_bytes()
        mismatches = sum(a != b for a, b in zip(ram, direct))
        print(f"+275D_direct_native_RAM_mismatches={mismatches}")
        if len(direct) != PRIVATE_BYTES or mismatches:
            raise AssertionError("direct +275D RAM differs")


if __name__ == "__main__":
    main()
