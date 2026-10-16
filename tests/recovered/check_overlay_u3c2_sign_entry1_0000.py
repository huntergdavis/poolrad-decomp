#!/usr/bin/env python3
"""Original U3C2 entry1 full-body sign helper parity through RETF 2."""

import argparse
import ctypes
import json
from hashlib import sha256
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
OVR_SHA = "872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614"
BODY_SHA = "5e8e5fc8a5907fee2efd922fc4f96682fd1f2e8e58cbebcd058d3f025948e5bf"
RAM_SIZE = 1 << 20
MASK = RAM_SIZE - 1
CS, SS, DS = 0xF000, 0x7000, 0x6000


def load_emulator():
    global Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE, REGISTERS
    global UC_X86_REG_CS, UC_X86_REG_IP
    from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE
    from unicorn.x86_const import (
        UC_X86_REG_AX, UC_X86_REG_BX, UC_X86_REG_CX, UC_X86_REG_DX,
        UC_X86_REG_SI, UC_X86_REG_DI, UC_X86_REG_BP, UC_X86_REG_SP,
        UC_X86_REG_CS, UC_X86_REG_IP, UC_X86_REG_DS, UC_X86_REG_ES,
        UC_X86_REG_SS, UC_X86_REG_EFLAGS,
    )
    REGISTERS = (UC_X86_REG_AX, UC_X86_REG_BX, UC_X86_REG_CX,
                 UC_X86_REG_DX, UC_X86_REG_SI, UC_X86_REG_DI,
                 UC_X86_REG_BP, UC_X86_REG_SP, UC_X86_REG_CS,
                 UC_X86_REG_IP, UC_X86_REG_DS, UC_X86_REG_ES,
                 UC_X86_REG_SS, UC_X86_REG_EFLAGS)


class Cpu(ctypes.Structure):
    _fields_ = [(name, ctypes.c_uint16) for name in
                "ax bx cx dx si di bp sp cs ip ds es ss flags".split()]


def physical(seg, off):
    return ((seg << 4) + (off & 0xFFFF)) & MASK


def put16(ram, seg, off, value):
    at = physical(seg, off)
    ram[at] = value & 255
    ram[(at + 1) & MASK] = (value >> 8) & 255


def source(media_root):
    from capstone import Cs, CS_ARCH_X86, CS_MODE_16
    ovr = (media_root / "GAME.OVR").read_bytes()
    assert len(ovr) == 206105 and sha256(ovr).hexdigest() == OVR_SHA
    body = ovr[0x2FB52:0x2FB80]
    assert len(body) == 46 and sha256(body).hexdigest() == BODY_SHA
    ins = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(body, 0))
    assert len(ins) == 17 and sum(i.size for i in ins) == 46
    assert ins[0].address == 0 and ins[-1].address == 0x2B
    assert ins[-1].mnemonic == "retf" and ins[-1].op_str == "2"
    assert not any(i.mnemonic in ("call", "lcall") for i in ins)
    return body


def load_cpu(emu, cpu):
    for (name, _), reg in zip(Cpu._fields_, REGISTERS):
        emu.reg_write(reg, getattr(cpu, name))


def frame(body, case):
    sp, value, flags = case
    ram = bytearray(RAM_SIZE)
    at = physical(CS, 0)
    ram[at:at + len(body)] = body
    for i in range(-12, 14):
        ram[physical(SS, sp + i)] = (i * 37 + value + flags) & 255
    put16(ram, SS, sp, 0x1234)
    put16(ram, SS, sp + 2, 0xD000)
    put16(ram, SS, sp + 4, value)
    cpu = Cpu(0xA55A, 0x1357, 0x2468, 0x3579, 0x468A, 0x579B,
              0xBEEF, sp, CS, 0, DS, 0x8000, SS, flags)
    return bytes(ram), cpu


def compare_case(body, model, case):
    ram, cpu = frame(body, case)
    oracle = Uc(UC_ARCH_X86, UC_MODE_16)
    oracle.mem_map(0, RAM_SIZE)
    oracle.mem_write(0, ram)
    load_cpu(oracle, cpu)
    trace = []

    def stop_at_caller(emu, address, size, user):
        del address, size, user
        cs = emu.reg_read(UC_X86_REG_CS) & 0xFFFF
        ip = emu.reg_read(UC_X86_REG_IP) & 0xFFFF
        trace.append((cs, ip))
        if (cs, ip) == (0xD000, 0x1234):
            emu.emu_stop()

    oracle.hook_add(UC_HOOK_CODE, stop_at_caller)
    oracle.emu_start(physical(CS, 0), RAM_SIZE, count=40)
    assert trace[0] == (CS, 0) and trace[-1] == (0xD000, 0x1234)
    assert (CS, 0x2B) in trace
    expected_ram = bytes(oracle.mem_read(0, RAM_SIZE))
    actual_ram = (ctypes.c_uint8 * RAM_SIZE).from_buffer_copy(ram)
    actual_cpu = Cpu.from_buffer_copy(cpu)
    assert model(actual_ram, RAM_SIZE, ctypes.byref(actual_cpu)) == 0
    if bytes(actual_ram) != expected_ram:
        diff = [i for i, (a, b) in enumerate(zip(actual_ram, expected_ram))
                if a != b]
        raise AssertionError((case, "RAM", [(hex(i), hex(actual_ram[i]),
                                             hex(expected_ram[i]))
                                            for i in diff[:12]]))
    for (name, _), reg in zip(Cpu._fields_, REGISTERS):
        expected = oracle.reg_read(reg) & 0xFFFF
        actual = getattr(actual_cpu, name)
        assert actual == expected, (case, name, hex(actual), hex(expected))
    _, value, _ = case
    answer = 0xFFFF if value & 0x8000 else 1 if value else 0
    assert actual_cpu.ax == answer
    assert actual_cpu.sp == (case[0] + 6) & 0xFFFF


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--static-only", action="store_true",
                        help="validate media/body/return shape; no emulator or C build")
    parser.add_argument("--media-root", type=Path,
                        default=ROOT / "resources/DOS/POOLRAD")
    args = parser.parse_args()
    body = source(args.media_root)
    if args.static_only:
        print(json.dumps({"mode": "static-only", "body_sha256": sha256(body).hexdigest(),
                          "bytes": len(body), "instructions": 17, "calls": 0,
                          "return": "002B RETF 2", "execution_cases": 0}))
        return
    load_emulator()
    with tempfile.TemporaryDirectory(dir=ROOT / "resources") as temp:
        shared = Path(temp) / "u3c2_0000.so"
        subprocess.run([
            "cc", "-shared", "-fPIC", "-std=c11", "-Wall", "-Wextra",
            "-Wpedantic", "-Werror", "-I", str(ROOT / "include"),
            str(ROOT / "src/recovered/overlay_u3c2_sign_entry1_0000.c"),
            "-o", str(shared)], check=True)
        model = ctypes.CDLL(str(shared)).pr_u3c2_sign_entry1_0000
        model.argtypes = (ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t,
                          ctypes.POINTER(Cpu))
        model.restype = ctypes.c_int
        values = (0x8000, 0xFFFF, 0xFEDC, 0xFF00, 0,
                  1, 0x00FF, 0x0100, 0x1234, 0x7FFF)
        cases = [(sp, value, flags)
                 for sp in (0x8000, 0x9001, 0)
                 for value in values
                 for flags in (0x0202, 0x0AD7)]
        for case in cases:
            compare_case(body, model, case)
    print(f"U3C2 entry1 +0000 complete original-byte full RAM/14-register parity: {len(cases)} cases PASS")


if __name__ == "__main__":
    main()
