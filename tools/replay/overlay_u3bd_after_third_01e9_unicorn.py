#!/usr/bin/env python3
"""Optional original-byte U3BD +01E9..+020A parent setup audit.

Input and output are provisional original-byte RAM images pending direct DOS
stops. Only hashes, counts, and CPU words are printed.
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
ORIGINAL_SPAN_SHA = "dab914875d9752e2374a769ad20b7ba91674c38653451e9f346df3f8c2eb6092"
PRE_SHA = "d5876edf3a5bab07f2bcf4531a6a0d804837a380dab89a6e3716e01da654f24b"
NEXT_SHA = "454dec9cca598103bbb945e97557649c655801c10a5ae58267b9388a4d2b99da"
REGS = (
    UC_X86_REG_CS, UC_X86_REG_IP, UC_X86_REG_AX, UC_X86_REG_BX,
    UC_X86_REG_CX, UC_X86_REG_DX, UC_X86_REG_SI, UC_X86_REG_DI,
    UC_X86_REG_BP, UC_X86_REG_SP, UC_X86_REG_DS, UC_X86_REG_ES,
    UC_X86_REG_SS, UC_X86_REG_EFLAGS,
)
PRE_CPU = (
    0x2D14, 0x01E9, 1, 0x3E4E, 0, 0x14A0, 0x3E68,
    0x007A, 0x3E98, 0x3E98, 0x14A0, 0x3DE4, 0x1D16, 0x7246,
)
NEXT_CPU = (
    0x2D14, 0x020A, 6, 0x3E4E, 0, 0x14A0, 0x3E68,
    0x260A, 0x3E98, 0x3E86, 0x14A0, 0x3DE4, 0x1D16, 0x7246,
)


def physical(segment, offset):
    return ((segment << 4) + offset) & 0xFFFFF


def checked(path, digest):
    data = path.read_bytes()
    if len(data) != PRIVATE_BYTES or sha256(data).hexdigest() != digest:
        raise ValueError(f"hash/size mismatch: {path.name}")
    return data


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("pre", type=Path, help="ignored +01E9 replay image")
    parser.add_argument("--native-next", type=Path,
                        help="optional direct DOS +020A RAM image")
    args = parser.parse_args()
    pre = checked(args.pre, PRE_SHA)
    ovr = (ROOT / "resources/DOS/POOLRAD/GAME.OVR").read_bytes()
    if sha256(ovr[0x2EAE0:0x2EB01]).hexdigest() != ORIGINAL_SPAN_SHA:
        raise ValueError("original parent span hash mismatch")
    start = physical(0x2D14, 0x01E9)
    end = physical(0x2D14, 0x020A)
    if sha256(pre[start:end]).hexdigest() != ORIGINAL_SPAN_SHA:
        raise ValueError("loaded parent span hash mismatch")

    emu = Uc(UC_ARCH_X86, UC_MODE_16)
    emu.mem_map(0, RAM_BYTES)
    emu.mem_write(0, pre + bytes(RAM_BYTES - PRIVATE_BYTES))
    for reg, value in zip(REGS, PRE_CPU):
        emu.reg_write(reg, value)
    stop = []
    steps = 0

    def on_code(engine, address, size, user):
        nonlocal steps
        del size, user
        steps += 1
        if address == end:
            stop.append((tuple(engine.reg_read(reg) & 0xFFFF for reg in REGS),
                         bytes(engine.mem_read(0, PRIVATE_BYTES)), steps))
            engine.emu_stop()

    emu.hook_add(UC_HOOK_CODE, on_code)
    emu.emu_start(start, 0, count=1000)
    if len(stop) != 1:
        raise AssertionError("+020A was not reached exactly once")
    cpu, ram, steps = stop[0]
    digest = sha256(ram).hexdigest()
    changed = sum(a != b for a, b in zip(pre, ram))
    print(f"next_call: steps={steps} changed_bytes={changed} RAM_sha256={digest}")
    print("next_call_CPU: " + " ".join(f"{v:04X}" for v in cpu))
    if digest != NEXT_SHA or cpu != NEXT_CPU or changed != 3:
        raise AssertionError("original-byte parent successor changed")
    if args.native_next is not None:
        native = args.native_next.read_bytes()
        if len(native) != PRIVATE_BYTES:
            raise ValueError("direct native +020A size mismatch")
        differences = sum(a != b for a, b in zip(ram, native))
        print(f"direct_native_020A_RAM_mismatches={differences}")
        if differences:
            raise AssertionError("direct native +020A differs")


if __name__ == "__main__":
    main()
