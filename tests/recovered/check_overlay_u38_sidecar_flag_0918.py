#!/usr/bin/env python3
"""Original-byte/full-state parity for GAME.OVR U38:+0918 entry 22."""

import ctypes
from hashlib import sha256
from pathlib import Path
import subprocess
import sys
import tempfile

from capstone import Cs, CS_ARCH_X86, CS_MODE_16
from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE
from unicorn.x86_const import (
    UC_X86_REG_AX, UC_X86_REG_BX, UC_X86_REG_CX, UC_X86_REG_DX,
    UC_X86_REG_SI, UC_X86_REG_DI, UC_X86_REG_BP, UC_X86_REG_SP,
    UC_X86_REG_CS, UC_X86_REG_IP, UC_X86_REG_DS, UC_X86_REG_ES,
    UC_X86_REG_SS, UC_X86_REG_EFLAGS,
)

ROOT = Path(__file__).resolve().parents[2]
OVR = ROOT / "resources/DOS/POOLRAD/GAME.OVR"
OVR_SHA = "872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614"
EXE_SHA = "d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd"
BODY_SHA = "da2ea19cc6e9bfb6da374a2e8f3342839e094563651b39b5758cdc223ca4873e"
RAM_SIZE = 1 << 20
REGISTERS = (UC_X86_REG_AX, UC_X86_REG_BX, UC_X86_REG_CX,
             UC_X86_REG_DX, UC_X86_REG_SI, UC_X86_REG_DI,
             UC_X86_REG_BP, UC_X86_REG_SP, UC_X86_REG_CS,
             UC_X86_REG_IP, UC_X86_REG_DS, UC_X86_REG_ES,
             UC_X86_REG_SS, UC_X86_REG_EFLAGS)


class Cpu(ctypes.Structure):
    _fields_ = [(name, ctypes.c_uint16) for name in
                "ax bx cx dx si di bp sp cs ip ds es ss flags".split()]


def physical(seg, off):
    return ((seg << 4) + (off & 0xFFFF)) & (RAM_SIZE - 1)


def put16(memory, seg, off, value):
    a = physical(seg, off)
    memory[a] = value & 255
    memory[(a + 1) & (RAM_SIZE - 1)] = value >> 8 & 255


def get16(memory, seg, off):
    a = physical(seg, off)
    return memory[a] | memory[(a + 1) & (RAM_SIZE - 1)] << 8


def snap(cpu):
    return tuple(getattr(cpu, name) for name, _ in Cpu._fields_)


def source():
    data = OVR.read_bytes()
    assert len(data) == 206105 and sha256(data).hexdigest() == OVR_SHA
    exe = (ROOT / "resources/DOS/POOLRAD/START.EXE").read_bytes()
    assert len(exe) == 65231 and sha256(exe).hexdigest() == EXE_SHA
    sys.path.insert(0, str(ROOT / "tools/overlay"))
    from overlay_inventory import find_units, parse_mz
    units = find_units(exe[parse_mz(exe)["header_bytes"]:], data)
    unit = next(u for u in units if u["stub_para"] == 0x38)
    assert unit["ovr_offset"] == 0xEC5B
    assert unit["entries"][22:24] == [0x0918, 0x0947]
    assert 0x0931 in unit["fixups"]
    body = data[0xF573:0xF5A2]
    assert len(body) == 47 and sha256(body).hexdigest() == BODY_SHA
    ins = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(body, 0x0918))
    assert len(ins) == 18 and sum(i.size for i in ins) == len(body)
    assert [(i.address, i.op_str) for i in ins if i.mnemonic == "lcall"] == [
        (0x092E, "0xba, 0x2199")]
    assert ins[-1].address == 0x0944 and ins[-1].mnemonic == "retf"
    return body


def make_case(body, mode, ds, ss, sp, amount):
    ram = bytearray(RAM_SIZE)
    ram[physical(0xF000, 0x0918):physical(0xF000, 0x0918) + len(body)] = body
    cpu = Cpu(0xA5CD, 0x3456, 0x5678, 0x789A, 0x9ABC,
              0xBCDE, 0x7777, sp, 0xF000, 0x0918, ds, 0x1111,
              ss, 0x72D3)
    put16(ram, ss, sp, 0x1234)
    put16(ram, ss, sp + 2, 0xD000)
    for i in range(5):
        put16(ram, ss, sp + 4 + 2 * i, 0xAB00 + i)
    put16(ram, ds, 0x5D92, 0x2300)
    put16(ram, ds, 0x5D94, 0x4000)
    put16(ram, 0x4000, 0x237F, 0 if mode == 0 else 0x2200)
    put16(ram, 0x4000, 0x2381, 0 if mode == 0 else 0x5000)
    ram[physical(0x5000, 0x2200)] = 0x18 if mode == 1 else 0x55
    put16(ram, 0x5000, 0x2205, 0x2300 if mode == 2 else 0)
    put16(ram, 0x5000, 0x2207, 0x5000 if mode == 2 else 0)
    ram[physical(0x5000, 0x2300)] = 0x18
    put16(ram, 0x5000, 0x2305, 0)
    put16(ram, 0x5000, 0x2307, 0)
    ram[physical(ds, 0x681E)] = 0xA7
    ram[physical(ds, 0x6822)] = amount
    return ram, cpu


def arguments(ram, cpu):
    return tuple(get16(ram, cpu.ss, cpu.sp + 2 * i) for i in range(5))


def lookup(ram, cpu):
    cursor_off, cursor_seg, key, char_off, char_seg = arguments(ram, cpu)
    assert (cursor_off, cursor_seg) == ((cpu.bp - 4) & 0xFFFF, cpu.ss)
    assert key & 255 == 0x18
    assert (char_off, char_seg) == (0x2300, 0x4000)
    off = get16(ram, char_seg, char_off + 0x7F)
    seg = get16(ram, char_seg, char_off + 0x81)
    put16(ram, cursor_seg, cursor_off, off)
    put16(ram, cursor_seg, cursor_off + 2, seg)
    found = 0
    for _ in range(3):
        off = get16(ram, cursor_seg, cursor_off)
        seg = get16(ram, cursor_seg, cursor_off + 2)
        if not (off | seg):
            break
        if ram[physical(seg, off)] == (key & 255):
            found = 1
            break
        nxt_off = get16(ram, seg, off + 5)
        nxt_seg = get16(ram, seg, off + 7)
        put16(ram, cursor_seg, cursor_off, nxt_off)
        put16(ram, cursor_seg, cursor_off + 2, nxt_seg)
    else:
        raise AssertionError("synthetic list did not terminate")
    cpu.ax = (cpu.ax & 0xFF00) | found
    cpu.dx = 0xCB67
    cpu.flags = 0x0247


def compare(label, model, callback_type, body):
    memory, cpu = make_case(body, *label)
    original = Uc(UC_ARCH_X86, UC_MODE_16)
    original.mem_map(0, RAM_SIZE)
    original.mem_write(0, bytes(memory))
    for (name, _), reg in zip(Cpu._fields_, REGISTERS):
        original.reg_write(reg, getattr(cpu, name))
    expected_calls = []
    reached = [False]

    def on_code(emu, address, size, user):
        del size, user
        if address == physical(0xF000, 0x092E):
            state = Cpu(*(emu.reg_read(reg) & 0xFFFF for reg in REGISTERS))
            snapshot = bytearray(emu.mem_read(0, RAM_SIZE))
            expected_calls.append((snap(state), arguments(snapshot, state)))
            lookup(snapshot, state)
            emu.mem_write(0, bytes(snapshot))
            for (name, _), reg in zip(Cpu._fields_, REGISTERS):
                emu.reg_write(reg, getattr(state, name))
            emu.reg_write(UC_X86_REG_SP, (state.sp + 10) & 0xFFFF)
            emu.reg_write(UC_X86_REG_IP, 0x0933)
        elif address == physical(0xD000, 0x1234):
            reached[0] = True
            emu.emu_stop()
        else:
            assert 0xF0918 <= address <= 0xF0944, (label, hex(address))

    original.hook_add(UC_HOOK_CODE, on_code)
    original.emu_start(physical(0xF000, 0x0918), RAM_SIZE, count=60)
    assert reached[0] and len(expected_calls) == 1, label
    expected_ram = bytes(original.mem_read(0, RAM_SIZE))
    ram = (ctypes.c_uint8 * RAM_SIZE).from_buffer_copy(memory)
    actual_calls = []

    @callback_type
    def external(context, ptr, size, state_ptr):
        del context
        assert size == RAM_SIZE
        state = state_ptr.contents
        actual_calls.append((snap(state), arguments(ptr, state)))
        lookup(ptr, state)
        return 1

    assert model(ram, RAM_SIZE, ctypes.byref(cpu), external, None) == 0
    if bytes(ram) != expected_ram:
        diff = [i for i, (a, b) in enumerate(zip(ram, expected_ram)) if a != b]
        raise AssertionError((label, "RAM", [hex(i) for i in diff[:12]]))
    assert actual_calls == expected_calls, (label, "calls", actual_calls, expected_calls)
    for (name, _), reg in zip(Cpu._fields_, REGISTERS):
        actual, expected = getattr(cpu, name), original.reg_read(reg) & 0xFFFF
        assert actual == expected, (label, name, hex(actual), hex(expected))


def main():
    body = source()
    callback_type = ctypes.CFUNCTYPE(
        ctypes.c_int, ctypes.c_void_p, ctypes.POINTER(ctypes.c_uint8),
        ctypes.c_size_t, ctypes.POINTER(Cpu))
    with tempfile.TemporaryDirectory(prefix="poolrad-u38-0918-") as tmp:
        library = Path(tmp) / "model.so"
        subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic",
                        "-Werror", "-shared", "-fPIC", "-I", str(ROOT / "include"),
                        str(ROOT / "src/recovered/overlay_u38_sidecar_flag_0918.c"),
                        "-o", str(library)], check=True)
        model = ctypes.CDLL(str(library)).pr_u38_sidecar_flag_0918
        model.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t,
                          ctypes.POINTER(Cpu), callback_type, ctypes.c_void_p]
        model.restype = ctypes.c_int
        cases = [(mode, ds, ss, sp, amount)
                 for mode in range(4)
                 for ds, ss, sp in ((0x3000, 0xE000, 0x8000),
                                    (0x3000, 0x3000, 0x708A),
                                    (0xF123, 0xE000, 0x0002))
                 for amount in (0, 4, 0x80, 0xFF)]
        for label in cases:
            compare(label, model, callback_type, body)
    print(f"u38:+0918 original-byte full-state parity: {len(cases)} cases")


if __name__ == "__main__":
    main()
