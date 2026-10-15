#!/usr/bin/env python3
"""Derive fourth RES_0464 +29D7 from +29AB; replay first exit branch.

Prints only addresses, counts, CPU tuples, and hashes. Optional full
RAM snapshots must remain beneath ignored resources/.
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
PARENT_SHA = "4c0f78a0462057168f8b7b4edd86d66af4da0f8dcf7d51c5a8079ecb4a0adde2"
ARM_SHA = "8c40857679a81d2a497eacbe20ca4b59227a57617bf307fa7827876941efa4c7"
CALL_LOADED_SHA = "6146e8864328699c80452dda85e44aa7a62c81f7dd40d0ed51e811e96832af76"
SPAN_SHA = "77c99f2b8a4298ab945d6692895d264a293029f10415df8bfa25d33b4bfef5b7"
EXIT_SHA = "39e9beed0ef26736226b15b280ba1bb3d2e904f9e6d93e87f008f0e190d82eb3"
REGS = (UC_X86_REG_CS, UC_X86_REG_IP, UC_X86_REG_AX, UC_X86_REG_BX,
        UC_X86_REG_CX, UC_X86_REG_DX, UC_X86_REG_SI, UC_X86_REG_DI,
        UC_X86_REG_BP, UC_X86_REG_SP, UC_X86_REG_DS, UC_X86_REG_ES,
        UC_X86_REG_SS, UC_X86_REG_EFLAGS)
PARENT_CPU = (0x0C88, 0x29AB, 1, 0x3E4E, 0, 0x14A0, 0x3E68, 0x1483,
              0x3E80, 0x3E5C, 0x14A0, 0x3DE4, 0x1D16, 0x7297)
EXIT_CPU = (0x0C88, 0x29D7, 0x000B, 0x3E4E, 0, 0x14A0, 0x3E68,
            0x14AB, 0x3E80, 0x3E5C, 0x14A0, 0x3DE4, 0x1D16, 0x7246)
BOUNDARY_CPU = (0x0C88, 0x29E6, 0x148B, 0x3E4E, 0, 0x14A0,
                0x3E68, 0x007C, 0x3E80, 0x3E5C, 0x14A0,
                0x3DE4, 0x1D16, 0x7206)
IPS = (0x29D7, 0x29DA, 0x29DB, 0x29DE, 0x29E0, 0x29E4, 0x29E6)


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
    move_calls = [0]
    inc_arms = [0]

    def on_code(engine, address, size, user):
        del size, user
        cpu = tuple(engine.reg_read(reg) & 0xFFFF for reg in REGS)
        if not observed and cpu[:2] != EXIT_CPU[:2]:
            predecessor[0] += 1
            if cpu[:2] == (0x0C88, 0x29C6):
                move_calls[0] += 1
            if cpu[:2] == (0x0C88, 0x29AB):
                inc_arms[0] += 1
            if predecessor[0] > 400:
                raise AssertionError("exit predecessor limit")
            return
        stage = len(observed)
        if stage >= len(IPS) or cpu[0] != 0x0C88 or \
                cpu[1] != IPS[stage] or \
                address != (cpu[0] << 4) + cpu[1]:
            raise AssertionError(f"unexpected exit branch boundary {stage}")
        observed.append((cpu, bytes(engine.mem_read(0, RAM_BYTES))))
        if stage == len(IPS) - 1:
            engine.emu_stop()

    emu.hook_add(UC_HOOK_CODE, on_code)
    emu.emu_start((0x0C88 << 4) + 0x29AB, 0, count=400)
    if predecessor[0] != 320 or move_calls[0] != 10 or \
            inc_arms[0] != 10 or len(observed) != len(IPS) or \
            observed[0][0] != EXIT_CPU or \
            observed[-1][0] != BOUNDARY_CPU:
        raise AssertionError("fourth loop or first exit branch changed")
    checked(observed[0][1], EXIT_SHA, "exit entry RAM")
    checked(observed[-1][1], EXIT_SHA, "branch boundary RAM")
    return predecessor[0], move_calls[0], inc_arms[0], observed


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
    checked(start[0x7024:0x703F], ARM_SHA, "loop arm file span")
    checked(parent[base + 0x29AB:base + 0x29C6], ARM_SHA,
            "loop arm loaded span")
    checked(parent[base + 0x29C6:base + 0x29D7],
            CALL_LOADED_SHA, "relocated Move/caller span")
    checked(start[0x7050:0x705F], SPAN_SHA, "exit file span")
    checked(parent[base + 0x29D7:base + 0x29E6],
            SPAN_SHA, "exit loaded span")
    predecessor, calls, arms, observed = replay(parent)
    print(f"predecessor_instructions={predecessor} "
          f"Move_calls={calls} increment_arms={arms} "
          f"exit_instructions={len(observed)-1} "
          f"exit_checkpoints={len(observed)}")
    print(f"exit_RAM_sha256={EXIT_SHA}")
    print("exit_entry_CPU=" + " ".join(f"{v:04X}" for v in EXIT_CPU))
    print("first_successor_CPU=" +
          " ".join(f"{v:04X}" for v in BOUNDARY_CPU))
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
        print(f"private_exit_snapshots={len(observed)}")


if __name__ == "__main__":
    main()
