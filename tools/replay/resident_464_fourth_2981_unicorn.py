#!/usr/bin/env python3
"""Replay fourth resident +2981 two-word DS writer to +29A4 JG successor.

Print addresses, CPU tuples, counts, and hashes only. Optional RAM/CPU
snapshots stay beneath ignored resources/.
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
PRE_SHA = "5f4367fda50ad7fcefa0cde9cd7e9d9b488125b1171e80ab0d21f85909d645ff"
SPAN_SHA = "946580045b636749790c945d8455f85f3aa7f1924688ea4ba19b341d3b7fdc8e"
POST_SHA = "0155b5aa519bacdffa71e1a20ae0326f77bf37754265ab100e04c6f25e4acd0f"
REGS = (UC_X86_REG_CS, UC_X86_REG_IP, UC_X86_REG_AX, UC_X86_REG_BX,
        UC_X86_REG_CX, UC_X86_REG_DX, UC_X86_REG_SI, UC_X86_REG_DI,
        UC_X86_REG_BP, UC_X86_REG_SP, UC_X86_REG_DS, UC_X86_REG_ES,
        UC_X86_REG_SS, UC_X86_REG_EFLAGS)
PRE_CPU = (0x0C88, 0x2981, 0xFFFF, 0x3E50, 0, 0, 0x2612, 0x007C,
           0x3E80, 0x3E5C, 0x14A0, 0x1D16, 0x1D16, 0x7286)
POST_CPU = (0x0C88, 0x29A6, 0x1401, 0x3E50, 0, 0, 0x2612, 0x007C,
            0x3E80, 0x3E5C, 0x14A0, 0x1D16, 0x1D16, 0x7297)
IPS = (0x2981, 0x2984, 0x2987, 0x2989, 0x298D, 0x2990, 0x2993,
       0x2995, 0x2999, 0x299C, 0x299F, 0x29A1, 0x29A4, 0x29A6)


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
            raise AssertionError("+2981 exceeded first branch successor")
        cpu = tuple(engine.reg_read(reg) & 0xFFFF for reg in REGS)
        if cpu[0] != 0x0C88 or cpu[1] != IPS[stage] or \
                address != (cpu[0] << 4) + cpu[1]:
            raise AssertionError(f"unexpected instruction boundary {stage}")
        observed.append((cpu, bytes(engine.mem_read(0, PRIVATE_BYTES))))
        if stage == len(IPS) - 1:
            engine.emu_stop()

    emu.hook_add(UC_HOOK_CODE, on_code)
    emu.emu_start((0x0C88 << 4) + 0x2981, 0, count=20)
    if len(observed) != len(IPS):
        raise AssertionError("+2981 path ended before first branch")
    if observed[0][0] != PRE_CPU or observed[-1][0] != POST_CPU:
        raise AssertionError("saved CPU boundary changed")
    checked(observed[-1][1], POST_SHA, "branch RAM")
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
    checked(start[0x6FFA:0x701F], SPAN_SHA, "file span")
    checked(pre[base + 0x2981:base + 0x29A6], SPAN_SHA, "loaded span")
    if args.native_pre is not None and args.native_pre.read_bytes() != pre:
        raise AssertionError("direct native +2981 RAM differs")
    observed = replay(pre)
    final_ram = observed[-1][1]
    print(f"start=0C88:2981 stop=0C88:29A6 instructions={len(observed)-1} "
          f"checkpoints={len(observed)}")
    print(f"input_RAM_sha256={PRE_SHA}")
    print(f"boundary_RAM_sha256={sha256(final_ram).hexdigest()}")
    print("boundary_CPU=" + " ".join(f"{value:04X}" for value in observed[-1][0]))
    print("boundary_changed_bytes=" + str(sum(a != b for a, b in zip(pre, final_ram))))
    if args.native_target is not None:
        direct = args.native_target.read_bytes()
        if len(direct) != PRIVATE_BYTES or direct != final_ram:
            raise AssertionError("direct native +29A6 RAM differs")
        print("+29A6_direct_native_RAM_mismatches=0")
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
