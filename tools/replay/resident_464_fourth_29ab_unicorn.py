#!/usr/bin/env python3
"""Derive fourth-call +29AB through Move; replay to the next call entry.

Only addresses, counts, CPU tuples, and hashes are printed. Optional
per-instruction RAM snapshots stay under ignored resources/.
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
PARENT_SHA = "eb0bcc2d94e3fe2291a0ceabf577f330dfa10e49bb87f9c9f430b98ce967b47d"
CALL_FILE_SHA = "cc1a43a44a97b085fe511ea515d4465c9590d65643add6a2555cde89fb5f78bc"
CALL_LOADED_SHA = "6146e8864328699c80452dda85e44aa7a62c81f7dd40d0ed51e811e96832af76"
ARM_SHA = "8c40857679a81d2a497eacbe20ca4b59227a57617bf307fa7827876941efa4c7"
PRE_SHA = "4c0f78a0462057168f8b7b4edd86d66af4da0f8dcf7d51c5a8079ecb4a0adde2"
POST_SHA = "ee20ffd4610472c91661b7948fc6e3cbff2c876674f3556d2f2cf13040ff2600"
REGS = (UC_X86_REG_CS, UC_X86_REG_IP, UC_X86_REG_AX, UC_X86_REG_BX,
        UC_X86_REG_CX, UC_X86_REG_DX, UC_X86_REG_SI, UC_X86_REG_DI,
        UC_X86_REG_BP, UC_X86_REG_SP, UC_X86_REG_DS, UC_X86_REG_ES,
        UC_X86_REG_SS, UC_X86_REG_EFLAGS)
PARENT_CPU = (0x0C88, 0x29C6, 4, 0x3E50, 0, 0, 0x2612, 0x147F,
              0x3E80, 0x3E52, 0x14A0, 0x3DE4, 0x1D16, 0x7202)
PRE_CPU = (0x0C88, 0x29AB, 1, 0x3E4E, 0, 0x14A0, 0x3E68, 0x1483,
           0x3E80, 0x3E5C, 0x14A0, 0x3DE4, 0x1D16, 0x7297)
POST_CPU = (0x0C88, 0x29C6, 4, 0x3E4E, 0, 0x14A0, 0x3E68, 0x1483,
            0x3E80, 0x3E52, 0x14A0, 0x3DE4, 0x1D16, 0x7212)
IPS = (0x29AB, 0x29AF, 0x29B2, 0x29B3, 0x29B4, 0x29B7, 0x29BA,
       0x29BC, 0x29C0, 0x29C1, 0x29C2, 0x29C5, 0x29C6)


def checked(data, digest, label):
    if sha256(data).hexdigest() != digest:
        raise ValueError(label + " hash mismatch")


def replay(parent):
    emu = Uc(UC_ARCH_X86, UC_MODE_16)
    emu.mem_map(0, 1 << 20)
    emu.mem_write(0, parent + bytes((1 << 20) - RAM_BYTES))
    for reg, value in zip(REGS, PARENT_CPU):
        emu.reg_write(reg, value)
    observed = []
    predecessor = [0]

    def on_code(engine, address, size, user):
        del size, user
        cpu = tuple(engine.reg_read(reg) & 0xFFFF for reg in REGS)
        if not observed and cpu[:2] != PRE_CPU[:2]:
            predecessor[0] += 1
            if predecessor[0] > 64:
                raise AssertionError("Move/caller predecessor limit")
            return
        stage = len(observed)
        if stage >= len(IPS) or cpu[0] != 0x0C88 or \
                cpu[1] != IPS[stage] or \
                address != (cpu[0] << 4) + cpu[1]:
            raise AssertionError(f"unexpected alternate-arm boundary {stage}")
        observed.append((cpu, bytes(engine.mem_read(0, RAM_BYTES))))
        if stage == len(IPS) - 1:
            engine.emu_stop()

    emu.hook_add(UC_HOOK_CODE, on_code)
    emu.emu_start((0x0C88 << 4) + 0x29C6, 0, count=100)
    if predecessor[0] != 20 or len(observed) != len(IPS) or \
            observed[0][0] != PRE_CPU or observed[-1][0] != POST_CPU:
        raise AssertionError("alternate-arm route or CPU changed")
    checked(observed[0][1], PRE_SHA, "alternate-arm input RAM")
    checked(observed[-1][1], POST_SHA, "alternate-arm output RAM")
    return predecessor[0], observed


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("parent", type=Path)
    parser.add_argument("--snapshot-dir", type=Path)
    args = parser.parse_args()
    parent = args.parent.read_bytes()
    if len(parent) != RAM_BYTES:
        raise ValueError("parent RAM size mismatch")
    checked(parent, PARENT_SHA, "parent RAM")
    start = (ROOT / "resources/DOS/POOLRAD/START.EXE").read_bytes()
    if len(start) != 65231:
        raise ValueError("START.EXE size mismatch")
    checked(start, START_SHA, "START.EXE")
    base = 0x0C88 << 4
    checked(start[0x703F:0x7050], CALL_FILE_SHA, "call/caller file span")
    checked(parent[base + 0x29C6:base + 0x29D7],
            CALL_LOADED_SHA, "relocated call/caller span")
    checked(start[0x7024:0x703F], ARM_SHA, "alternate-arm file span")
    checked(parent[base + 0x29AB:base + 0x29C6],
            ARM_SHA, "alternate-arm loaded span")
    predecessor, observed = replay(parent)
    print(f"Move_and_caller_instructions={predecessor} "
          f"arm_instructions={len(observed)-1} "
          f"arm_checkpoints={len(observed)}")
    print(f"arm_input_RAM_sha256={PRE_SHA}")
    print(f"arm_boundary_RAM_sha256={POST_SHA}")
    print("arm_entry_CPU=" + " ".join(f"{v:04X}" for v in PRE_CPU))
    print("arm_boundary_CPU=" + " ".join(f"{v:04X}" for v in POST_CPU))
    print("arm_changed_bytes=" + str(sum(
        a != b for a, b in zip(observed[0][1], observed[-1][1]))))
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
        print(f"private_arm_snapshots={len(observed)}")


if __name__ == "__main__":
    main()
