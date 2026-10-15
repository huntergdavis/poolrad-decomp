#!/usr/bin/env python3
"""Original-byte fourth resident +275D JG/CMP/JAE through +277D.

Prints addresses, counts, RAM hashes, and CPU tuples only.
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
SPAN_SHA = "515dae830436fbd579a67a53f28d36e7ff28debe0e37660a7c4a13cb5237961a"
RAM_SHA = "95c91ee38f22f5d3808a851ddfdb055aa2733b70bfd7bf3ba897ed8bed7f621c"
REGS = (UC_X86_REG_CS, UC_X86_REG_IP, UC_X86_REG_AX, UC_X86_REG_BX,
        UC_X86_REG_CX, UC_X86_REG_DX, UC_X86_REG_SI, UC_X86_REG_DI,
        UC_X86_REG_BP, UC_X86_REG_SP, UC_X86_REG_DS, UC_X86_REG_ES,
        UC_X86_REG_SS, UC_X86_REG_EFLAGS)
PRE_CPU = (0x0C88, 0x275D, 168, 0, 104, 0, 0x2612, 8,
           0x3E80, 0x3E5C, 0x14A0, 0x3DE4, 0x1D16, 0x7246)
STOPS = (
    (0x275F, 2, (0x0C88, 0x275F, 168, 0, 104, 0, 0x2612, 8,
                 0x3E80, 0x3E5C, 0x14A0, 0x3DE4, 0x1D16, 0x7246)),
    (0x2761, 3, (0x0C88, 0x2761, 168, 0, 104, 0, 0x2612, 8,
                 0x3E80, 0x3E5C, 0x14A0, 0x3DE4, 0x1D16, 0x7202)),
    (0x277D, 4, (0x0C88, 0x277D, 168, 0, 104, 0, 0x2612, 8,
                 0x3E80, 0x3E5C, 0x14A0, 0x3DE4, 0x1D16, 0x7202)),
)


def checked(data, digest, label):
    if sha256(data).hexdigest() != digest:
        raise ValueError(label + " hash mismatch")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("pre", type=Path)
    parser.add_argument("--native-pre", type=Path)
    parser.add_argument("--native-target", type=Path)
    args = parser.parse_args()
    pre = args.pre.read_bytes()
    if len(pre) != PRIVATE_BYTES:
        raise ValueError("starting RAM size mismatch")
    checked(pre, RAM_SHA, "starting RAM")
    start = (ROOT / "resources/DOS/POOLRAD/START.EXE").read_bytes()
    if len(start) != 65231:
        raise ValueError("START.EXE size mismatch")
    checked(start, START_SHA, "START.EXE")
    checked(start[0x6DD6:0x6DDC], SPAN_SHA, "file span")
    base = 0x0C88 << 4
    checked(pre[base + 0x275D:base + 0x2763], SPAN_SHA, "loaded span")
    if args.native_pre is not None and args.native_pre.read_bytes() != pre:
        raise AssertionError("direct +275D RAM differs")
    emu = Uc(UC_ARCH_X86, UC_MODE_16)
    emu.mem_map(0, 1 << 20)
    emu.mem_write(0, pre + bytes((1 << 20) - PRIVATE_BYTES))
    for reg, value in zip(REGS, PRE_CPU):
        emu.reg_write(reg, value)
    observed = {}
    steps = 0
    targets = {item[0] for item in STOPS}

    def on_code(engine, address, size, user):
        nonlocal steps
        del size, user
        steps += 1
        ip = address - base
        if ip in targets:
            observed[ip] = (steps, bytes(engine.mem_read(0, PRIVATE_BYTES)),
                            tuple(engine.reg_read(reg) & 0xFFFF for reg in REGS))
            if ip == 0x277D:
                engine.emu_stop()

    emu.hook_add(UC_HOOK_CODE, on_code)
    emu.emu_start(base + 0x275D, 0, count=8)
    if set(observed) != targets:
        raise AssertionError("saved path missed a stop")
    for ip, expected_step, expected_cpu in STOPS:
        step, ram, cpu = observed[ip]
        digest = sha256(ram).hexdigest()
        changed = sum(a != b for a, b in zip(pre, ram))
        print(f"+{ip:04X}: steps={step} changed_bytes={changed} RAM_sha256={digest}")
        print("CPU: " + " ".join(f"{value:04X}" for value in cpu))
        if (step, digest, changed, cpu) != (
                expected_step, RAM_SHA, 0, expected_cpu):
            raise AssertionError(f"+{ip:04X} original-byte state changed")
        if ip == 0x277D and args.native_target is not None:
            direct = args.native_target.read_bytes()
            mismatches = sum(a != b for a, b in zip(ram, direct))
            print(f"+277D_direct_native_RAM_mismatches={mismatches}")
            if len(direct) != PRIVATE_BYTES or mismatches:
                raise AssertionError("direct +277D RAM differs")


if __name__ == "__main__":
    main()
