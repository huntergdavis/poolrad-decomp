#!/usr/bin/env python3
"""Original-byte fourth resident +28D9 through the +291C call boundary.

Prints addresses, counts, RAM hashes, and CPU tuples only. An optional
snapshot is written only below ignored resources/.
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
SPAN_SHA = "99cefaacb29da0a174c631b7e7f3f38832f8d8ba2b6b383ec54259ff82ea6192"
RAM_SHA = "95c91ee38f22f5d3808a851ddfdb055aa2733b70bfd7bf3ba897ed8bed7f621c"
REGS = (UC_X86_REG_CS, UC_X86_REG_IP, UC_X86_REG_AX, UC_X86_REG_BX,
        UC_X86_REG_CX, UC_X86_REG_DX, UC_X86_REG_SI, UC_X86_REG_DI,
        UC_X86_REG_BP, UC_X86_REG_SP, UC_X86_REG_DS, UC_X86_REG_ES,
        UC_X86_REG_SS, UC_X86_REG_EFLAGS)
PRE_CPU = (0x0C88, 0x28D9, 1, 0, 104, 0, 0x2612, 8,
           0x3E80, 0x3E5C, 0x14A0, 0x3DE4, 0x1D16, 0x7297)
STOPS = (
    (0x28DB, 2, 1, 8, 0x3E5C, 0x7246, RAM_SHA, 0),
    (0x28E0, 3, 1, 8, 0x3E5C, 0x7246, RAM_SHA, 0),
    (0x28FC, 13, 8, 8, 0x3E5C, 0x7202,
     "3ba88f7e05672fdef6109f41433bb6fbfed1523152d2a8aee8d66af7447f3a49", 1),
    (0x2906, 17, 32, 8, 0x3E5C, 0x7202,
     "6b7b645394faee2650ad8ff79dc3566f27da9d881ae9cf5a35cd7050e8f373c2", 2),
    (0x2910, 21, 0x1458, 8, 0x3E5C, 0x7202,
     "6ac7b05ba59bb7e855a9db694fff524b21ec8477c550124d7a53eb06ff6907f5", 4),
    (0x2914, 23, 0x1458, 0x3E64, 0x3E5A, 0x7202,
     "cb596c84d8ca6f0ca556d866cdb3731534bf6894a02fe46ec6d45fcc435456f0", 6),
    (0x291C, 28, 0, 0x3E64, 0x3E54, 0x7202,
     "3ef5853b7c52f12f7ec562b8c9f98e91274bfd5c1dcaec4152130033a7b66ba6", 12),
)


def checked(data, digest, label):
    if sha256(data).hexdigest() != digest:
        raise ValueError(label + " hash mismatch")


def emulated(pre, cpu, stops, final_ip):
    base = 0x0C88 << 4
    emu = Uc(UC_ARCH_X86, UC_MODE_16)
    emu.mem_map(0, 1 << 20)
    emu.mem_write(0, pre + bytes((1 << 20) - PRIVATE_BYTES))
    for reg, value in zip(REGS, cpu):
        emu.reg_write(reg, value)
    observed = {}
    steps = 0

    def on_code(engine, address, size, user):
        nonlocal steps
        del size, user
        steps += 1
        ip = address - base
        if ip in stops:
            observed[ip] = (steps, bytes(engine.mem_read(0, PRIVATE_BYTES)),
                            tuple(engine.reg_read(reg) & 0xFFFF for reg in REGS))
            if ip == final_ip:
                engine.emu_stop()

    emu.hook_add(UC_HOOK_CODE, on_code)
    emu.emu_start(base + 0x28D9, 0, count=60)
    if set(observed) != stops:
        raise AssertionError("saved path missed a stop")
    return observed


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("pre", type=Path)
    parser.add_argument("--snapshot", type=Path)
    parser.add_argument("--snapshot-dir", type=Path)
    parser.add_argument("--native-pre", type=Path)
    parser.add_argument("--native-target", type=Path)
    parser.add_argument("--branch-other", action="store_true")
    args = parser.parse_args()
    pre = args.pre.read_bytes()
    if len(pre) != PRIVATE_BYTES:
        raise ValueError("starting RAM size mismatch")
    checked(pre, RAM_SHA, "starting RAM")
    start = (ROOT / "resources/DOS/POOLRAD/START.EXE").read_bytes()
    if len(start) != 65231:
        raise ValueError("START.EXE size mismatch")
    checked(start, START_SHA, "START.EXE")
    checked(start[0x6F52:0x6F95], SPAN_SHA, "file span")
    base = 0x0C88 << 4
    checked(pre[base + 0x28D9:base + 0x291C], SPAN_SHA, "loaded span")
    if args.native_pre is not None and args.native_pre.read_bytes() != pre:
        raise AssertionError("direct +28D9 RAM differs")
    observed = emulated(pre, PRE_CPU, {item[0] for item in STOPS}, 0x291C)
    for ip, expected_step, ax, di, sp, flags, expected_sha, changed in STOPS:
        step, ram, cpu = observed[ip]
        digest = sha256(ram).hexdigest()
        differences = sum(a != b for a, b in zip(pre, ram))
        expected_cpu = (0x0C88, ip, ax, 0, 104, 0, 0x2612, di,
                        0x3E80, sp, 0x14A0, 0x3DE4, 0x1D16, flags)
        print(f"+{ip:04X}: steps={step} changed_bytes={differences} RAM_sha256={digest}")
        print("CPU: " + " ".join(f"{value:04X}" for value in cpu))
        if (step, digest, differences, cpu) != (
                expected_step, expected_sha, changed, expected_cpu):
            raise AssertionError(f"+{ip:04X} original-byte state changed")
    final_ram = observed[0x291C][1]
    if args.native_target is not None:
        direct = args.native_target.read_bytes()
        mismatches = sum(a != b for a, b in zip(final_ram, direct))
        print(f"+291C_direct_native_RAM_mismatches={mismatches}")
        if len(direct) != PRIVATE_BYTES or mismatches:
            raise AssertionError("direct +291C RAM differs")
    if args.snapshot is not None:
        path = args.snapshot.resolve()
        if not path.is_relative_to((ROOT / "resources").resolve()):
            raise ValueError("snapshot must remain under ignored resources/")
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(final_ram)
        print(f"+291C_snapshot_sha256={sha256(final_ram).hexdigest()}")
    if args.snapshot_dir is not None:
        directory = args.snapshot_dir.resolve()
        if not directory.is_relative_to((ROOT / "resources").resolve()):
            raise ValueError("snapshots must remain under ignored resources/")
        directory.mkdir(parents=True, exist_ok=True)
        for ip, *_ in STOPS:
            (directory / f"ram-{ip:04x}.bin").write_bytes(observed[ip][1])
        print(f"private_stop_snapshots={len(STOPS)}")
    if args.branch_other:
        alternate = list(PRE_CPU)
        alternate[2] = 3
        other = emulated(pre, tuple(alternate), {0x28DB, 0x28DD}, 0x28DD)
        for ip, step, flags in ((0x28DB, 2, 0x7202),
                                (0x28DD, 3, 0x7202)):
            actual_step, ram, cpu = other[ip]
            expected = (0x0C88, ip, 3, 0, 104, 0, 0x2612, 8,
                        0x3E80, 0x3E5C, 0x14A0, 0x3DE4, 0x1D16, flags)
            if (actual_step, ram, cpu) != (step, pre, expected):
                raise AssertionError(f"alternate +{ip:04X} differs")
        print("selector=3 stop=+28DD FLAGS=7202 RAM_mismatches=0")


if __name__ == "__main__":
    main()
