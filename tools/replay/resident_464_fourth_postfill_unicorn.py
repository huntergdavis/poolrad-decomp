#!/usr/bin/env python3
"""Trace the saved fourth resident +2921 local loop to the +2968 boundary.

Print addresses, counts, and hashes only. Optional RAM/CPU snapshots remain
beneath ignored resources/.
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
PRE_SHA = "4b02a5589727a60b13ab8e62ef21904c5257f9583015f7e8b902d7292f5dcc8c"
SPAN_SHA = "9b6905f9e3845793b2f958ef8dfb80ab09ff3d81a4da08613a5f1daecea195f9"
POST_SHA = "3f30630f17e536b66af4e0b8e1861e5924407598bdd1721338e6d28027f9c310"
REGS = (UC_X86_REG_CS, UC_X86_REG_IP, UC_X86_REG_AX, UC_X86_REG_BX,
        UC_X86_REG_CX, UC_X86_REG_DX, UC_X86_REG_SI, UC_X86_REG_DI,
        UC_X86_REG_BP, UC_X86_REG_SP, UC_X86_REG_DS, UC_X86_REG_ES,
        UC_X86_REG_SS, UC_X86_REG_EFLAGS)
PRE_CPU = (0x0C88, 0x2921, 0, 0x3E50, 0, 0, 0x2612, 0x3E68,
           0x3E80, 0x3E5C, 0x14A0, 0x1D16, 0x1D16, 0x7202)
POST_CPU = (0x0C88, 0x2968, 0, 0x3E50, 0, 0, 0x2612, 0x003E,
            0x3E80, 0x3E5C, 0x14A0, 0x1D16, 0x1D16, 0x7297)
IPS = (
    0x2921, 0x2925, 0x292A, 0x292D, 0x292F, 0x2931, 0x2935,
    0x2938, 0x293A, 0x2947, 0x294B,
    0x2927, 0x292A, 0x292D, 0x292F, 0x2931, 0x2935, 0x2938,
    0x293A, 0x293C, 0x293F, 0x2941, 0x2943, 0x2947, 0x294B,
    0x2927, 0x292A, 0x292D, 0x292F, 0x2931, 0x2935, 0x2938,
    0x293A, 0x293C, 0x293F, 0x2941, 0x2943, 0x2947, 0x294B,
    0x2927, 0x292A, 0x292D, 0x292F, 0x2931, 0x2935, 0x2938,
    0x293A, 0x2947, 0x294B, 0x294D, 0x2950, 0x2953, 0x2955,
    0x2958, 0x295D, 0x2960, 0x2965, 0x2968,
)


def checked(data, digest, label):
    if sha256(data).hexdigest() != digest:
        raise ValueError(label + " hash mismatch")


def replay(pre):
    emu = Uc(UC_ARCH_X86, UC_MODE_16)
    emu.mem_map(0, 1 << 20)
    emu.mem_write(0, pre + bytes((1 << 20) - PRIVATE_BYTES))
    for reg, value in zip(REGS, PRE_CPU):
        emu.reg_write(reg, value)
    observed = []

    def on_code(engine, address, size, user):
        del size, user
        stage = len(observed)
        if stage >= len(IPS):
            raise AssertionError("post-FillChar path exceeded the boundary")
        cpu = tuple(engine.reg_read(reg) & 0xFFFF for reg in REGS)
        if cpu[0] != 0x0C88 or cpu[1] != IPS[stage] or \
                address != (cpu[0] << 4) + cpu[1]:
            raise AssertionError(f"unexpected instruction boundary {stage}")
        ram = bytes(engine.mem_read(0, PRIVATE_BYTES))
        observed.append((cpu, ram))
        if stage == len(IPS) - 1:
            engine.emu_stop()

    emu.hook_add(UC_HOOK_CODE, on_code)
    emu.emu_start((0x0C88 << 4) + 0x2921, 0, count=70)
    if len(observed) != len(IPS):
        raise AssertionError("post-FillChar path ended early")
    if observed[0][0] != PRE_CPU or observed[-1][0] != POST_CPU:
        raise AssertionError("saved CPU boundary changed")
    checked(observed[-1][1], POST_SHA, "boundary RAM")
    return observed


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("pre", type=Path)
    parser.add_argument("--snapshot-dir", type=Path)
    parser.add_argument("--native-pre", type=Path)
    parser.add_argument("--native-target", type=Path)
    args = parser.parse_args()
    pre = args.pre.read_bytes()
    if len(pre) != PRIVATE_BYTES:
        raise ValueError("starting RAM size mismatch")
    checked(pre, PRE_SHA, "starting RAM")
    start = (ROOT / "resources/DOS/POOLRAD/START.EXE").read_bytes()
    if len(start) != 65231:
        raise ValueError("START.EXE size mismatch")
    checked(start, START_SHA, "START.EXE")
    base = 0x0C88 << 4
    checked(start[0x6F9A:0x6FE1], SPAN_SHA, "file span")
    checked(pre[base + 0x2921:base + 0x2968], SPAN_SHA, "loaded span")
    if args.native_pre is not None and args.native_pre.read_bytes() != pre:
        raise AssertionError("direct native +2921 RAM differs")
    observed = replay(pre)
    final_ram = observed[-1][1]
    print(f"start=0C88:2921 stop=0C88:2968 instructions={len(observed)-1} "
          f"checkpoints={len(observed)}")
    print(f"input_RAM_sha256={PRE_SHA}")
    print(f"boundary_RAM_sha256={sha256(final_ram).hexdigest()}")
    print("branch_mask_outcomes=miss,hit,hit,miss")
    print("boundary_CPU=" + " ".join(f"{value:04X}" for value in observed[-1][0]))
    print("boundary_changed_bytes=" + str(sum(a != b for a, b in zip(pre, final_ram))))
    if args.native_target is not None:
        direct = args.native_target.read_bytes()
        if len(direct) != PRIVATE_BYTES or direct != final_ram:
            raise AssertionError("direct native +2968 RAM differs")
        print("+2968_direct_native_RAM_mismatches=0")
    if args.snapshot_dir is not None:
        directory = args.snapshot_dir.resolve()
        if not directory.is_relative_to((ROOT / "resources").resolve()):
            raise ValueError("snapshots must remain under ignored resources/")
        directory.mkdir(parents=True, exist_ok=True)
        lines = []
        for stage, (cpu, ram) in enumerate(observed):
            (directory / f"ram-{stage:02d}.bin").write_bytes(ram)
            lines.append(" ".join(f"{value:04X}" for value in cpu))
        (directory / "cpu-stops.txt").write_text("\n".join(lines) + "\n")
        print(f"private_stop_snapshots={len(observed)}")


if __name__ == "__main__":
    main()
