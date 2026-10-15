#!/usr/bin/env python3
"""Replay the fourth RES_0464 +2A04 jump and +2962 JBE gate.

Prints addresses, counts, CPU tuples, and hashes only. Optional full
RAM snapshots remain beneath ignored resources/.
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
RAM_BYTES = 655360
START_SHA = "d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd"
PRE_SHA = "fad5029ace030b75902a05c478247f3462e24a5d1968abc062ff8a5eb4af5aff"
POST_SHA = "c78f443d72bc1550ed523b2549b6974e973585edb2d27b117477ee0030b4acf6"
JUMP_SHA = "2206490c93d265f0bda4f2a27f5a35a5ff81799af78d4dc18c59c2d526bc7c96"
TARGET_SHA = "d4b5392bea1f7a3d74e5abc1d7ab5765d12c6277292e350ae8c65d5f779b2e7f"
REGS = (UC_X86_REG_CS, UC_X86_REG_IP, UC_X86_REG_AX, UC_X86_REG_BX,
        UC_X86_REG_CX, UC_X86_REG_DX, UC_X86_REG_SI, UC_X86_REG_DI,
        UC_X86_REG_BP, UC_X86_REG_SP, UC_X86_REG_DS, UC_X86_REG_ES,
        UC_X86_REG_SS, UC_X86_REG_EFLAGS)
PRE_CPU = (0x0C88, 0x2A04, 0, 0x3E4E, 0, 0x14A0, 0x3E68,
           0x007C, 0x3E80, 0x3E5C, 0x14A0, 0x3DE4, 0x1D16, 0x7297)
POST_CPU = (0x0C88, 0x2981, 0xFFFF, 0x3E4E, 0, 0x14A0, 0x3E68,
            0x007E, 0x3E80, 0x3E5C, 0x14A0, 0x3DE4, 0x1D16, 0x7286)
IPS = (0x2A04, 0x2962, 0x2965, 0x2968, 0x296D, 0x2970,
       0x2973, 0x2976, 0x2978, 0x297C, 0x297F, 0x2981)


def checked(data, digest, label):
    if sha256(data).hexdigest() != digest:
        raise ValueError(label + " hash mismatch")


def replay(pre):
    emu = Uc(UC_ARCH_X86, UC_MODE_16)
    emu.mem_map(0, 1 << 20)
    emu.mem_write(0, pre + bytes((1 << 20) - RAM_BYTES))
    for reg, value in zip(REGS, PRE_CPU):
        emu.reg_write(reg, value)
    observed = []

    def on_code(engine, address, size, user):
        del size, user
        stage = len(observed)
        if stage >= len(IPS):
            raise AssertionError("successor exceeded first JBE boundary")
        cpu = tuple(engine.reg_read(reg) & 0xFFFF for reg in REGS)
        if cpu[0] != 0x0C88 or cpu[1] != IPS[stage] or \
                address != (cpu[0] << 4) + cpu[1]:
            raise AssertionError(f"unexpected +2A04 boundary {stage}")
        observed.append((cpu, bytes(engine.mem_read(0, RAM_BYTES))))
        if stage == len(IPS) - 1:
            engine.emu_stop()

    emu.hook_add(UC_HOOK_CODE, on_code)
    emu.emu_start((0x0C88 << 4) + 0x2A04, 0, count=16)
    if len(observed) != len(IPS) or observed[0][0] != PRE_CPU or \
            observed[-1][0] != POST_CPU:
        raise AssertionError("+2A04 CPU path changed")
    checked(observed[-1][1], POST_SHA, "+2A04 output RAM")
    return observed


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("pre", type=Path)
    parser.add_argument("--snapshot-dir", type=Path)
    args = parser.parse_args()
    pre = args.pre.read_bytes()
    if len(pre) != RAM_BYTES:
        raise ValueError("starting RAM size mismatch")
    checked(pre, PRE_SHA, "starting RAM")
    start = (ROOT / "resources/DOS/POOLRAD/START.EXE").read_bytes()
    if len(start) != 65231:
        raise ValueError("START.EXE size mismatch")
    checked(start, START_SHA, "START.EXE")
    base = 0x0C88 << 4
    checked(start[0x707D:0x7080], JUMP_SHA, "jump file span")
    checked(pre[base + 0x2A04:base + 0x2A07],
            JUMP_SHA, "jump loaded span")
    checked(start[0x6FDB:0x6FFA], TARGET_SHA, "target file span")
    checked(pre[base + 0x2962:base + 0x2981],
            TARGET_SHA, "target loaded span")
    observed = replay(pre)
    print(f"start=0C88:2A04 stop=0C88:2981 "
          f"instructions={len(observed)-1} checkpoints={len(observed)}")
    print(f"input_RAM_sha256={PRE_SHA}")
    print(f"boundary_RAM_sha256={POST_SHA}")
    print("boundary_CPU=" +
          " ".join(f"{v:04X}" for v in observed[-1][0]))
    print("changed_addresses=" + " ".join(
        f"{i:05X}" for i, (a, b) in enumerate(
            zip(pre, observed[-1][1])) if a != b))
    if args.snapshot_dir is not None:
        directory = args.snapshot_dir.resolve()
        if not directory.is_relative_to((ROOT / "resources").resolve()):
            raise ValueError("snapshots must remain under ignored resources/")
        directory.mkdir(parents=True, exist_ok=True)
        for stage, (cpu, ram) in enumerate(observed):
            (directory / f"ram-{stage:02d}.bin").write_bytes(ram)
        (directory / "cpu-stops.txt").write_text(
            "\n".join(" ".join(f"{v:04X}" for v in cpu)
                      for cpu, _ in observed) + "\n")
        print(f"private_successor_snapshots={len(observed)}")


if __name__ == "__main__":
    main()
