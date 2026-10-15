#!/usr/bin/env python3
"""Original-byte fourth RES_0464:+2632 prefix audit to first runtime call.

The +020A starting image comes from provisional original-byte replay.
Only address, hash, count, and CPU evidence is printed. Optional output
must remain under ignored resources/.
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
PRE_SHA = "454dec9cca598103bbb945e97557649c655801c10a5ae58267b9388a4d2b99da"
ORIGINAL_CALL_SHA = "a734c1740b95619bedad6325a3ac8d1413024a9cbb9302bbf3c15831cfb20cfe"
LOADED_CALL_SHA = "0d18bd5c35579c163826a0665cd04cf9a8b7b054c3f930e7633f0c6bdc4352f4"
PREFIX_SHA = "0987655ef499dd6c7ba5d5c9c63bf124c9e1715647b0e7949c883e5a22ff9f30"
ORIGINAL_MOVE_CALL_SHA = "29a16310ee045858c5b1fecbb30ec4e438ced3f943e627fc7e261ff319c7965c"
LOADED_MOVE_CALL_SHA = "5edd05f1f8251f93185453fdb7f6901fa80650c85244e9e985109823bd18cfbd"
ENTRY_SHA = "7cf5907411261888e2bb6c9d1474adea7853de50b3cd4b64be5f98c60b51e8c3"
STOP_SHA = "d6907db0096edac3304be6e836b5984dba4d9889842dd3a26fe0df35c44edda2"

REGS = (
    UC_X86_REG_CS, UC_X86_REG_IP, UC_X86_REG_AX, UC_X86_REG_BX,
    UC_X86_REG_CX, UC_X86_REG_DX, UC_X86_REG_SI, UC_X86_REG_DI,
    UC_X86_REG_BP, UC_X86_REG_SP, UC_X86_REG_DS, UC_X86_REG_ES,
    UC_X86_REG_SS, UC_X86_REG_EFLAGS,
)
PRE_CPU = (
    0x2D14, 0x020A, 6, 0x3E4E, 0, 0x14A0, 0x3E68,
    0x260A, 0x3E98, 0x3E86, 0x14A0, 0x3DE4, 0x1D16, 0x7246,
)
ENTRY_CPU = (
    0x0C88, 0x2632, 6, 0x3E4E, 0, 0x14A0, 0x3E68,
    0x260A, 0x3E98, 0x3E82, 0x14A0, 0x3DE4, 0x1D16, 0x7246,
)
STOP_CPU = (
    0x0C88, 0x2646, 8, 0x3E4E, 0, 0x14A0, 0x3E68,
    0x3E78, 0x3E80, 0x3E52, 0x14A0, 0x14A0, 0x1D16, 0x7216,
)


def physical(segment, offset):
    return ((segment << 4) + offset) & 0xFFFFF


def checked(data, digest, name):
    if sha256(data).hexdigest() != digest:
        raise ValueError(f"{name} hash mismatch")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("pre", type=Path, help="ignored provisional +020A RAM")
    parser.add_argument("--native-entry", type=Path,
                        help="optional direct DOS +2632 RAM")
    parser.add_argument("--native-stop", type=Path,
                        help="optional direct DOS +2646 RAM")
    parser.add_argument("--output", type=Path,
                        help="write +2646 RAM under ignored resources/")
    args = parser.parse_args()
    pre = args.pre.read_bytes()
    if len(pre) != PRIVATE_BYTES:
        raise ValueError("starting RAM size mismatch")
    checked(pre, PRE_SHA, "starting RAM")
    start = (ROOT / "resources/DOS/POOLRAD/START.EXE").read_bytes()
    if len(start) != 65231:
        raise ValueError("START.EXE size mismatch")
    checked(start, START_SHA, "START.EXE")
    ovr = (ROOT / "resources/DOS/POOLRAD/GAME.OVR").read_bytes()
    checked(ovr[0x2EB01:0x2EB06], ORIGINAL_CALL_SHA, "original far call")
    checked(pre[physical(0x2D14, 0x020A):
                physical(0x2D14, 0x020A) + 5],
            LOADED_CALL_SHA, "loaded far call")
    checked(start[0x6CAB:0x6CBF], PREFIX_SHA, "original resident prefix")
    checked(pre[physical(0x0C88, 0x2632):
                physical(0x0C88, 0x2646)],
            PREFIX_SHA, "loaded resident prefix")
    checked(start[0x6CBF:0x6CC4], ORIGINAL_MOVE_CALL_SHA,
            "original runtime Move call")
    checked(pre[physical(0x0C88, 0x2646):
                physical(0x0C88, 0x2646) + 5],
            LOADED_MOVE_CALL_SHA, "loaded runtime Move call")

    emu = Uc(UC_ARCH_X86, UC_MODE_16)
    emu.mem_map(0, RAM_BYTES)
    emu.mem_write(0, pre + bytes(RAM_BYTES - PRIVATE_BYTES))
    for reg, value in zip(REGS, PRE_CPU):
        emu.reg_write(reg, value)
    targets = {
        physical(0x0C88, 0x2632): "entry",
        physical(0x0C88, 0x2646): "pre_move",
    }
    stops = {}
    steps = 0

    def on_code(engine, address, size, user):
        nonlocal steps
        del size, user
        steps += 1
        name = targets.get(address)
        if name is None:
            return
        stops[name] = (
            bytes(engine.mem_read(0, PRIVATE_BYTES)),
            tuple(engine.reg_read(reg) & 0xFFFF for reg in REGS),
            steps,
        )
        if name == "pre_move":
            engine.emu_stop()

    emu.hook_add(UC_HOOK_CODE, on_code)
    emu.emu_start(physical(0x2D14, 0x020A), 0, count=100)
    if set(stops) != {"entry", "pre_move"}:
        raise AssertionError("fourth-call prefix did not reach both stops")
    for name, digest, expected_cpu, expected_step, changed, native_path in (
        ("entry", ENTRY_SHA, ENTRY_CPU, 2, 2, args.native_entry),
        ("pre_move", STOP_SHA, STOP_CPU, 13, 11, args.native_stop),
    ):
        ram, cpu, step = stops[name]
        difference_count = sum(a != b for a, b in zip(pre, ram))
        print(f"{name}: steps={step} changed_bytes={difference_count} "
              f"RAM_sha256={sha256(ram).hexdigest()}")
        print(name + "_CPU: " + " ".join(f"{word:04X}" for word in cpu))
        if step != expected_step or difference_count != changed or \
           cpu != expected_cpu or sha256(ram).hexdigest() != digest:
            raise AssertionError(f"{name} original-byte parity changed")
        if native_path is not None:
            native = native_path.read_bytes()
            if len(native) != PRIVATE_BYTES:
                raise ValueError(f"{name} direct native RAM size mismatch")
            mismatches = sum(a != b for a, b in zip(ram, native))
            print(f"{name}_direct_native_RAM_mismatches={mismatches}")
            if mismatches:
                raise AssertionError(f"{name} direct native RAM differs")
    if args.output is not None:
        destination = args.output.resolve()
        if not destination.is_relative_to((ROOT / "resources").resolve()):
            raise ValueError("output must be under ignored resources/")
        destination.parent.mkdir(parents=True, exist_ok=True)
        destination.write_bytes(stops["pre_move"][0])


if __name__ == "__main__":
    main()
