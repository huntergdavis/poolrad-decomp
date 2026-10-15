#!/usr/bin/env python3
"""Replay saved fourth-call +291C through relocated FillChar return.

Print only addresses, CPU tuples, counts, and hashes. Optional RAM snapshots
must stay beneath ignored resources/.
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
PRE_SHA = "3ef5853b7c52f12f7ec562b8c9f98e91274bfd5c1dcaec4152130033a7b66ba6"
CALL_FILE_SHA = "adc5985dcb761563534658cb03a15d9afdf2305d7e2b82f800875d921f91ee3a"
CALL_LOADED_SHA = "2776d6ebc232e7277e121d58341055aaa57b341e24b5edb8a4d116e421da1db3"
RUNTIME_SHA = "f1c5fade96970aa1dd1a9ffc2d876c7aea57bef8680d929d9f65d6c5190e17f7"
POST_SHA = "4b02a5589727a60b13ab8e62ef21904c5257f9583015f7e8b902d7292f5dcc8c"
STAGE_SHA = (
    PRE_SHA,
    "30ea943cda3b86939967419dccba3681a65dbddc215b48384c89a9a15202c218",
    "30ea943cda3b86939967419dccba3681a65dbddc215b48384c89a9a15202c218",
    "30ea943cda3b86939967419dccba3681a65dbddc215b48384c89a9a15202c218",
    "30ea943cda3b86939967419dccba3681a65dbddc215b48384c89a9a15202c218",
    "30ea943cda3b86939967419dccba3681a65dbddc215b48384c89a9a15202c218",
    "30ea943cda3b86939967419dccba3681a65dbddc215b48384c89a9a15202c218",
    "41b5f82ccbefdf6915ce23fef723d907f4a308c7bb12fe57d91a4cc9cf090d0b",
    "0b5a0f1b0cc48b91894be99a4a0f9c12e6ee29234d22d6895c82c7f481b342c0",
    POST_SHA, POST_SHA, POST_SHA, POST_SHA,
)
STAGE_ADDR = (
    (0x0C88, 0x291C), (0x131C, 0x16A8), (0x131C, 0x16AA),
    (0x131C, 0x16AE), (0x131C, 0x16B2), (0x131C, 0x16B6),
    (0x131C, 0x16B7), (0x131C, 0x16B7), (0x131C, 0x16B7),
    (0x131C, 0x16B7), (0x131C, 0x16B7), (0x131C, 0x16B9),
    (0x0C88, 0x2921),
)
REGS = (UC_X86_REG_CS, UC_X86_REG_IP, UC_X86_REG_AX, UC_X86_REG_BX,
        UC_X86_REG_CX, UC_X86_REG_DX, UC_X86_REG_SI, UC_X86_REG_DI,
        UC_X86_REG_BP, UC_X86_REG_SP, UC_X86_REG_DS, UC_X86_REG_ES,
        UC_X86_REG_SS, UC_X86_REG_EFLAGS)
PRE_CPU = (0x0C88, 0x291C, 0, 0, 104, 0, 0x2612, 0x3E64,
           0x3E80, 0x3E54, 0x14A0, 0x3DE4, 0x1D16, 0x7202)
POST_CPU = (0x0C88, 0x2921, 0, 0x3E50, 0, 0, 0x2612, 0x3E68,
            0x3E80, 0x3E5C, 0x14A0, 0x1D16, 0x1D16, 0x7202)


def checked(data, digest, label):
    if sha256(data).hexdigest() != digest:
        raise ValueError(label + " hash mismatch")


def expected_cpu(stage):
    cpu = list(PRE_CPU)
    cpu[0], cpu[1] = STAGE_ADDR[stage]
    if stage >= 1:
        cpu[9] = 0x3E50
    if stage >= 2:
        cpu[3] = 0x3E50
    if stage >= 3:
        cpu[11] = 0x1D16
    if stage >= 4:
        cpu[4] = 4
    if 7 <= stage <= 10:
        cpu[4] = 10 - stage
        cpu[7] = 0x3E64 + stage - 6
    if stage >= 11:
        cpu[4] = 0
        cpu[7] = 0x3E68
    if stage == 12:
        cpu[9] = 0x3E5C
    return tuple(cpu)


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
        if stage >= len(STAGE_ADDR):
            raise AssertionError("execution exceeded bounded FillChar path")
        cpu = tuple(engine.reg_read(reg) & 0xFFFF for reg in REGS)
        if (cpu[0], cpu[1]) != STAGE_ADDR[stage] or \
                address != (cpu[0] << 4) + cpu[1]:
            raise AssertionError("unexpected instruction boundary")
        ram = bytes(engine.mem_read(0, PRIVATE_BYTES))
        if cpu != expected_cpu(stage) or sha256(ram).hexdigest() != STAGE_SHA[stage]:
            raise AssertionError(f"original-byte checkpoint {stage} changed")
        observed.append((cpu, ram))
        if stage == len(STAGE_ADDR) - 1:
            engine.emu_stop()

    emu.hook_add(UC_HOOK_CODE, on_code)
    emu.emu_start((0x0C88 << 4) + 0x291C, 0, count=20)
    if len(observed) != len(STAGE_ADDR):
        raise AssertionError("FillChar path ended early")
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
    call_at = (0x0C88 << 4) + 0x291C
    runtime_at = (0x131C << 4) + 0x16A8
    checked(start[0x6F95:0x6F9A], CALL_FILE_SHA, "original far call")
    checked(pre[call_at:call_at + 5], CALL_LOADED_SHA, "relocated far call")
    checked(pre[runtime_at:runtime_at + 20], RUNTIME_SHA, "loaded FillChar")
    if args.native_pre is not None and args.native_pre.read_bytes() != pre:
        raise AssertionError("direct native +291C RAM differs")
    observed = replay(pre)
    for stage, (cpu, ram) in enumerate(observed):
        delta = sum(a != b for a, b in zip(pre, ram))
        print(f"stage={stage:02d} {cpu[0]:04X}:{cpu[1]:04X} "
              f"RAM_sha256={sha256(ram).hexdigest()} changed_bytes={delta}")
        print("CPU: " + " ".join(f"{value:04X}" for value in cpu))
    if observed[-1][0] != POST_CPU or sha256(observed[-1][1]).hexdigest() != POST_SHA:
        raise AssertionError("FillChar return changed")
    if args.native_target is not None:
        direct = args.native_target.read_bytes()
        if len(direct) != PRIVATE_BYTES or direct != observed[-1][1]:
            raise AssertionError("direct native +2921 RAM differs")
        print("+2921_direct_native_RAM_mismatches=0")
    if args.snapshot_dir is not None:
        directory = args.snapshot_dir.resolve()
        if not directory.is_relative_to((ROOT / "resources").resolve()):
            raise ValueError("snapshots must remain under ignored resources/")
        directory.mkdir(parents=True, exist_ok=True)
        for stage, (_, ram) in enumerate(observed):
            (directory / f"ram-{stage:02d}.bin").write_bytes(ram)
        print(f"private_stop_snapshots={len(observed)}")


if __name__ == "__main__":
    main()
