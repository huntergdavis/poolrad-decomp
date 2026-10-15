#!/usr/bin/env python3
"""Replay the fourth RES_0464 +29A6 saved path to its Move call entry.

Only addresses, CPU tuples, counts, and hashes are printed. Optional
snapshots must stay under ignored resources/.
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
PRE_SHA = "0155b5aa519bacdffa71e1a20ae0326f77bf37754265ab100e04c6f25e4acd0f"
SPAN_SHA = "4b87d4d848d0c54b731f76026acfc65220a0b7cdabe7c690e028dc99393b33a1"
REGS = (UC_X86_REG_CS, UC_X86_REG_IP, UC_X86_REG_AX, UC_X86_REG_BX,
        UC_X86_REG_CX, UC_X86_REG_DX, UC_X86_REG_SI, UC_X86_REG_DI,
        UC_X86_REG_BP, UC_X86_REG_SP, UC_X86_REG_DS, UC_X86_REG_ES,
        UC_X86_REG_SS, UC_X86_REG_EFLAGS)
PRE_CPU = (0x0C88, 0x29A6, 0x1401, 0x3E50, 0, 0, 0x2612, 0x007C,
           0x3E80, 0x3E5C, 0x14A0, 0x1D16, 0x1D16, 0x7297)
IPS = (0x29A6, 0x29A9, 0x29AF, 0x29B2, 0x29B3, 0x29B4, 0x29B7,
       0x29BA, 0x29BC, 0x29C0, 0x29C1, 0x29C2, 0x29C5, 0x29C6)


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
            raise AssertionError("path exceeded runtime Move boundary")
        cpu = tuple(engine.reg_read(reg) & 0xFFFF for reg in REGS)
        if cpu[0] != 0x0C88 or cpu[1] != IPS[stage] or address != (
                (cpu[0] << 4) + cpu[1]):
            raise AssertionError(f"unexpected instruction boundary {stage}")
        observed.append((cpu, bytes(engine.mem_read(0, RAM_BYTES))))
        if stage == len(IPS) - 1:
            engine.emu_stop()

    emu.hook_add(UC_HOOK_CODE, on_code)
    emu.emu_start((0x0C88 << 4) + 0x29A6, 0, count=20)
    if len(observed) != len(IPS) or observed[0][0] != PRE_CPU:
        raise AssertionError("saved boundary changed")
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
    checked(start[0x701F:0x703F], SPAN_SHA, "file span")
    checked(pre[base + 0x29A6:base + 0x29C6], SPAN_SHA, "loaded span")
    observed = replay(pre)
    final_ram = observed[-1][1]
    print(f"start=0C88:29A6 stop=0C88:29C6 instructions={len(observed)-1} "
          f"checkpoints={len(observed)}")
    print(f"input_RAM_sha256={PRE_SHA}")
    print(f"boundary_RAM_sha256={sha256(final_ram).hexdigest()}")
    print("boundary_CPU=" + " ".join(f"{v:04X}" for v in observed[-1][0]))
    print("boundary_changed_bytes=" + str(sum(a != b for a, b in zip(pre, final_ram))))
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
        print(f"private_stop_snapshots={len(observed)}")


if __name__ == "__main__":
    main()
