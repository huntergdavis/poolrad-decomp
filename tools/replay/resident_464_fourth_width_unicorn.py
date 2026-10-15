#!/usr/bin/env python3
"""Original-byte fourth resident +2699 through pre-JGE +26FB.

Prints only addresses, counts, CPU tuples and SHA-256 digests.
Private images remain under ignored resources.
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
SPAN_SHA = "90090ebf66c6cf2c96c48cdf82c96223ea73c3c8eebf07600cea03f8dd83ecd8"
PRE_SHA = "9cf87e06c28a8293f625239b02ebb82244ad9cf9f38453394fbf91510d3d5438"
POST_SHA = "a96589cea30f44509fe4d9f26c7739ce56c6a637f273a1e75b44b93fc9705509"
REGS = (UC_X86_REG_CS, UC_X86_REG_IP, UC_X86_REG_AX, UC_X86_REG_BX,
        UC_X86_REG_CX, UC_X86_REG_DX, UC_X86_REG_SI, UC_X86_REG_DI,
        UC_X86_REG_BP, UC_X86_REG_SP, UC_X86_REG_DS, UC_X86_REG_ES,
        UC_X86_REG_SS, UC_X86_REG_EFLAGS)
PRE_CPU = (0x0C88, 0x2699, 21, 0, 13, 0, 0x2612, 8,
           0x3E80, 0x3E5C, 0x14A0, 0x3DE4, 0x1D16, 0x7246)
STOPS = (
    (0x269B, 2, PRE_SHA, (0x0C88, 0x269B, 21, 0, 13, 0, 0x2612, 8, 0x3E80, 0x3E5C, 0x14A0, 0x3DE4, 0x1D16, 0x7246)),
    (0x269D, 3, PRE_SHA, (0x0C88, 0x269D, 21, 0, 13, 0, 0x2612, 8, 0x3E80, 0x3E5C, 0x14A0, 0x3DE4, 0x1D16, 0x7246)),
    (0x269F, 4, PRE_SHA, (0x0C88, 0x269F, 21, 0, 13, 0, 0x2612, 8, 0x3E80, 0x3E5C, 0x14A0, 0x3DE4, 0x1D16, 0x7212)),
    (0x26CD, 5, PRE_SHA, (0x0C88, 0x26CD, 21, 0, 13, 0, 0x2612, 8, 0x3E80, 0x3E5C, 0x14A0, 0x3DE4, 0x1D16, 0x7212)),
    (0x26F7, 23, POST_SHA, (0x0C88, 0x26F7, 8, 0, 13, 0, 0x2612, 8, 0x3E80, 0x3E5C, 0x14A0, 0x3DE4, 0x1D16, 0x7246)),
    (0x26FB, 24, POST_SHA, (0x0C88, 0x26FB, 8, 0, 13, 0, 0x2612, 8, 0x3E80, 0x3E5C, 0x14A0, 0x3DE4, 0x1D16, 0x7202)),
)


def checked(data, digest, label):
    if sha256(data).hexdigest() != digest:
        raise ValueError(label + " hash mismatch")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("pre", type=Path)
    parser.add_argument("--native-pre-jl", type=Path)
    parser.add_argument("--native-fallthrough", type=Path)
    parser.add_argument("--output", type=Path)
    parser.add_argument("--stage-dir", type=Path)
    args = parser.parse_args()
    pre = args.pre.read_bytes()
    if len(pre) != PRIVATE_BYTES:
        raise ValueError("starting RAM size mismatch")
    checked(pre, PRE_SHA, "starting RAM")
    start = (ROOT / "resources/DOS/POOLRAD/START.EXE").read_bytes()
    if len(start) != 65231:
        raise ValueError("START.EXE size mismatch")
    checked(start, START_SHA, "START.EXE")
    checked(start[0x6D12:0x6D74], SPAN_SHA, "file span")
    base = 0x0C88 << 4
    checked(pre[base + 0x2699:base + 0x26FB], SPAN_SHA, "loaded span")
    emu = Uc(UC_ARCH_X86, UC_MODE_16)
    emu.mem_map(0, 1 << 20)
    emu.mem_write(0, pre + bytes((1 << 20) - PRIVATE_BYTES))
    for reg, value in zip(REGS, PRE_CPU):
        emu.reg_write(reg, value)
    observed = {}
    step = 0
    targets = {item[0] for item in STOPS}

    def on_code(engine, address, size, user):
        nonlocal step
        del size, user
        step += 1
        ip = address - base
        if ip in targets:
            observed[ip] = (step, bytes(engine.mem_read(0, PRIVATE_BYTES)),
                            tuple(engine.reg_read(reg) & 0xFFFF for reg in REGS))
            if ip == 0x26FB:
                engine.emu_stop()

    emu.hook_add(UC_HOOK_CODE, on_code)
    emu.emu_start(base + 0x2699, 0, count=100)
    if set(observed) != targets:
        raise AssertionError("saved path missed a stop")
    if args.native_pre_jl is not None:
        direct = args.native_pre_jl.read_bytes()
        if len(direct) != PRIVATE_BYTES or direct != pre:
            raise AssertionError("direct +2699 RAM differs")
        print("+2699_direct_native_RAM_mismatches=0")
    for ip, expected_step, expected_sha, expected_cpu in STOPS:
        actual_step, ram, cpu = observed[ip]
        digest = sha256(ram).hexdigest()
        changed = sum(a != b for a, b in zip(pre, ram))
        print(f"+{ip:04X}: steps={actual_step} changed_bytes={changed} RAM_sha256={digest}")
        print("CPU: " + " ".join(f"{value:04X}" for value in cpu))
        if (actual_step, digest, cpu) != (expected_step, expected_sha, expected_cpu):
            raise AssertionError(f"+{ip:04X} original-byte state changed")
        if ip == 0x269B and args.native_fallthrough is not None:
            direct = args.native_fallthrough.read_bytes()
            mismatches = sum(a != b for a, b in zip(ram, direct))
            print(f"+269B_direct_native_RAM_mismatches={mismatches}")
            if len(direct) != PRIVATE_BYTES or mismatches:
                raise AssertionError("direct +269B RAM differs")
    if args.stage_dir is not None:
        stage_dir = args.stage_dir.resolve()
        if not stage_dir.is_relative_to((ROOT / "resources").resolve()):
            raise ValueError("stage images must remain under ignored resources")
        stage_dir.mkdir(parents=True, exist_ok=True)
        for ip, _, _, _ in STOPS:
            (stage_dir / f"ram-{ip:04x}.bin").write_bytes(observed[ip][1])
    if args.output is not None:
        destination = args.output.resolve()
        if not destination.is_relative_to((ROOT / "resources").resolve()):
            raise ValueError("output must remain under ignored resources")
        destination.parent.mkdir(parents=True, exist_ok=True)
        destination.write_bytes(observed[0x26FB][1])


if __name__ == "__main__":
    main()
