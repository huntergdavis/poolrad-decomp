#!/usr/bin/env python3
"""Original-byte fourth-call Move/JNE gate to resident +2656.

The starting +2646 image is provisional original-byte replay from a
directly captured prior return. Only address, hash, count, and CPU
evidence is printed. Optional output stays under ignored resources/.
"""

import argparse
from hashlib import sha256
from pathlib import Path

from unicorn import Uc, UC_ARCH_X86, UC_HOOK_CODE, UC_MODE_16
from unicorn.x86_const import (
    UC_X86_REG_AX, UC_X86_REG_BX, UC_X86_REG_CX, UC_X86_REG_DX,
    UC_X86_REG_SI, UC_X86_REG_DI, UC_X86_REG_BP, UC_X86_REG_SP,
    UC_X86_REG_CS, UC_X86_REG_IP, UC_X86_REG_DS, UC_X86_REG_ES,
    UC_X86_REG_SS, UC_X86_REG_EFLAGS,
)

ROOT = Path(__file__).resolve().parents[2]
RAM_BYTES = 1 << 20
PRIVATE_BYTES = 655360
START_SHA = "d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd"
PRE_SHA = "d6907db0096edac3304be6e836b5984dba4d9889842dd3a26fe0df35c44edda2"
ORIGINAL_GATE_SHA = "439a66c1f0c5069102fa75d200a37e0e164a48d7cb5fe78eb3a76e0830663b0d"
LOADED_GATE_SHA = "ba5fdcc57f25ecaf397c60a01727dd956431dc6306dc5a367584fd65c0164ba1"
LOADED_MOVE_SHA = "5bf8af52bdcd4af1b7e73b160b3ee02e6de7812b7e686aae02f73d9ccbc061aa"
SUCCESSOR_SHA = "8d65f2916cda57e7e5c6a4d71a70404e716b2e275c5489c132b48f3ed1371200"

REGS = (
    UC_X86_REG_CS, UC_X86_REG_IP, UC_X86_REG_AX, UC_X86_REG_BX,
    UC_X86_REG_CX, UC_X86_REG_DX, UC_X86_REG_SI, UC_X86_REG_DI,
    UC_X86_REG_BP, UC_X86_REG_SP, UC_X86_REG_DS, UC_X86_REG_ES,
    UC_X86_REG_SS, UC_X86_REG_EFLAGS,
)
PRE_CPU = (
    0x0C88, 0x2646, 8, 0x3E4E, 0, 0x14A0, 0x3E68,
    0x3E78, 0x3E80, 0x3E52, 0x14A0, 0x14A0, 0x1D16, 0x7216,
)
STOPS = {
    (0x131C, 0x0A3F): (
        "move_entry", 2,
        (0x131C, 0x0A3F, 8, 0x3E4E, 0, 0x14A0, 0x3E68,
         0x3E78, 0x3E80, 0x3E4E, 0x14A0, 0x14A0, 0x1D16, 0x7216),
    ),
    (0x0C88, 0x264B): (
        "move_return", 19,
        (0x0C88, 0x264B, 8, 0x3E4E, 0, 0x14A0, 0x2612,
         0x3E80, 0x3E80, 0x3E5C, 0x14A0, 0x1D16, 0x1D16, 0x7216),
    ),
    (0x0C88, 0x2651): (
        "jne", 21,
        (0x0C88, 0x2651, 0x3DEC, 0x3E4E, 0, 0x14A0, 0x2612,
         0x3E80, 0x3E80, 0x3E5C, 0x14A0, 0x1D16, 0x1D16, 0x7202),
    ),
    (0x0C88, 0x2656): (
        "successor", 22,
        (0x0C88, 0x2656, 0x3DEC, 0x3E4E, 0, 0x14A0, 0x2612,
         0x3E80, 0x3E80, 0x3E5C, 0x14A0, 0x1D16, 0x1D16, 0x7202),
    ),
}


def physical(segment, offset):
    return ((segment << 4) + offset) & 0xFFFFF


def checked(data, digest, name):
    if sha256(data).hexdigest() != digest:
        raise ValueError(f"{name} hash mismatch")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("pre", type=Path, help="ignored +2646 replay RAM")
    parser.add_argument("--native-pre", type=Path,
                        help="optional direct DOS +2646 RAM")
    parser.add_argument("--native-successor", type=Path,
                        help="optional direct DOS +2656 RAM")
    parser.add_argument("--output", type=Path,
                        help="write +2656 RAM under ignored resources/")
    args = parser.parse_args()
    pre = args.pre.read_bytes()
    if len(pre) != PRIVATE_BYTES:
        raise ValueError("starting RAM size mismatch")
    checked(pre, PRE_SHA, "starting RAM")
    start = (ROOT / "resources/DOS/POOLRAD/START.EXE").read_bytes()
    if len(start) != 65231:
        raise ValueError("START.EXE size mismatch")
    checked(start, START_SHA, "START.EXE")
    checked(start[0x6CBF:0x6CCF], ORIGINAL_GATE_SHA,
            "original resident call and JNE gate")
    checked(pre[physical(0x0C88, 0x2646):
                physical(0x0C88, 0x2656)], LOADED_GATE_SHA,
            "loaded resident call and JNE gate")
    checked(pre[physical(0x131C, 0x0A3F):
                physical(0x131C, 0x0A57)], LOADED_MOVE_SHA,
            "loaded runtime Move body")
    if args.native_pre is not None:
        native = args.native_pre.read_bytes()
        if len(native) != PRIVATE_BYTES or native != pre:
            raise AssertionError("direct native +2646 differs")
        print("pre_direct_native_RAM_mismatches=0")

    emu = Uc(UC_ARCH_X86, UC_MODE_16)
    emu.mem_map(0, RAM_BYTES)
    emu.mem_write(0, pre + bytes(RAM_BYTES - PRIVATE_BYTES))
    for reg, value in zip(REGS, PRE_CPU):
        emu.reg_write(reg, value)
    targets = {
        physical(segment, offset): detail
        for (segment, offset), detail in STOPS.items()
    }
    observed = {}
    steps = 0

    def on_code(engine, address, size, user):
        nonlocal steps
        del size, user
        steps += 1
        detail = targets.get(address)
        if detail is None:
            return
        name = detail[0]
        observed[name] = (
            bytes(engine.mem_read(0, PRIVATE_BYTES)),
            tuple(engine.reg_read(reg) & 0xFFFF for reg in REGS),
            steps,
        )
        if name == "successor":
            engine.emu_stop()

    emu.hook_add(UC_HOOK_CODE, on_code)
    emu.emu_start(physical(0x0C88, 0x2646), 0, count=500)
    if set(observed) != {detail[0] for detail in STOPS.values()}:
        raise AssertionError("Move/JNE audit missed a boundary")
    for name, expected_step, expected_cpu in STOPS.values():
        ram, cpu, step = observed[name]
        changed = sum(a != b for a, b in zip(pre, ram))
        digest = sha256(ram).hexdigest()
        print(f"{name}: steps={step} changed_bytes={changed} RAM_sha256={digest}")
        print(name + "_CPU: " + " ".join(f"{value:04X}" for value in cpu))
        if step != expected_step or cpu != expected_cpu or changed != 2 or \
           digest != SUCCESSOR_SHA:
            raise AssertionError(f"{name} original-byte state changed")
    if args.native_successor is not None:
        native = args.native_successor.read_bytes()
        if len(native) != PRIVATE_BYTES:
            raise ValueError("direct native +2656 RAM size mismatch")
        differences = sum(
            a != b for a, b in zip(native, observed["successor"][0])
        )
        print(f"successor_direct_native_RAM_mismatches={differences}")
        if differences:
            raise AssertionError("direct native +2656 differs")
    if args.output is not None:
        destination = args.output.resolve()
        if not destination.is_relative_to((ROOT / "resources").resolve()):
            raise ValueError("output must be under ignored resources/")
        destination.parent.mkdir(parents=True, exist_ok=True)
        destination.write_bytes(observed["successor"][0])


if __name__ == "__main__":
    main()
