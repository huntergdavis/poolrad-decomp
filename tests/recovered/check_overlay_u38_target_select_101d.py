#!/usr/bin/env python3
"""Original-byte/full-state parity for GAME.OVR U38:+101D helper."""

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
BODY_SHA = "632b609ad14b0fff262cd578a81e144d5dbeab6418ee8ef68303ff9e60cea33a"
RAM_SIZE = 1 << 20
REGISTERS = (UC_X86_REG_AX, UC_X86_REG_BX, UC_X86_REG_CX,
             UC_X86_REG_DX, UC_X86_REG_SI, UC_X86_REG_DI,
             UC_X86_REG_BP, UC_X86_REG_SP, UC_X86_REG_CS,
             UC_X86_REG_IP, UC_X86_REG_DS, UC_X86_REG_ES,
             UC_X86_REG_SS, UC_X86_REG_EFLAGS)
FLAGS = (0, 0x10, 0, 0x0A, 0x09, 0x88, 0x08, 0x89)


class Cpu(ctypes.Structure):
    _fields_ = [(name, ctypes.c_uint16) for name in
                "ax bx cx dx si di bp sp cs ip ds es ss flags".split()]


def physical(seg, off):
    return ((seg << 4) + (off & 0xFFFF)) & (RAM_SIZE - 1)


def get16(ram, seg, off):
    a = physical(seg, off)
    return ram[a] | ram[(a + 1) & (RAM_SIZE - 1)] << 8


def put16(ram, seg, off, value):
    a = physical(seg, off)
    ram[a] = value & 255
    ram[(a + 1) & (RAM_SIZE - 1)] = value >> 8 & 255


def snap(cpu):
    return tuple(getattr(cpu, name) for name, _ in Cpu._fields_)


def source():
    ovr = OVR.read_bytes()
    exe = (ROOT / "resources/DOS/POOLRAD/START.EXE").read_bytes()
    assert len(ovr) == 206105 and sha256(ovr).hexdigest() == OVR_SHA
    assert len(exe) == 65231 and sha256(exe).hexdigest() == EXE_SHA
    sys.path.insert(0, str(ROOT / "tools/overlay"))
    from overlay_inventory import find_units, parse_mz
    unit = next(u for u in
                find_units(exe[parse_mz(exe)["header_bytes"]:], ovr)
                if u["stub_para"] == 0x38)
    assert unit["ovr_offset"] == 0xEC5B
    assert unit["entries"][35] == 0x1086
    assert 0x1048 in unit["fixups"]
    body = ovr[0xFC78:0xFCE1]
    assert len(body) == 105 and sha256(body).hexdigest() == BODY_SHA
    ins = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(body, 0x101D))
    assert len(ins) == 36 and sum(i.size for i in ins) == len(body)
    assert [(i.address, i.op_str) for i in ins if i.mnemonic == "lcall"] == [
        (0x1045, "0xba, 0x2e8b")]
    assert ins[-1].address == 0x1083 and ins[-1].mnemonic == "retf"
    return body


def make_case(body, mode, ds, ss, sp, loaded_segment):
    ram = bytearray(RAM_SIZE)
    pos = physical(0xF000, 0x101D)
    ram[pos:pos + len(body)] = body
    put16(ram, 0xF000, 0x1048, loaded_segment)
    cpu = Cpu(0xA5CD, 0x3456, 0x5678, 0x789A, 0x9ABC,
              0xBCDE, 0x7777, sp, 0xF000, 0x101D, ds, 0x1111,
              ss, 0x72D3)
    put16(ram, ss, sp, 0x1234)
    put16(ram, ss, sp + 2, 0xD000)
    put16(ram, ss, sp + 4, 0x2300)
    put16(ram, ss, sp + 6, 0x4000)
    if mode != 0:
        put16(ram, 0x4000, 0x23CC, 0x2200)
        put16(ram, 0x4000, 0x23CE, 0x5000)
    put16(ram, 0x4000, 0x23F8, 0x3300)
    put16(ram, 0x4000, 0x23FA, 0x6000)
    put16(ram, 0x4000, 0x23FC, 0x4400)
    put16(ram, 0x4000, 0x23FE, 0x7000)
    ram[physical(0x5000, 0x222E)] = 7
    ram[physical(ds, 0x5600)] = FLAGS[mode]
    return ram, cpu


def frame(ram, cpu):
    return tuple(get16(ram, cpu.ss, cpu.sp + 2 * i) for i in range(6))


def select_python(ram, cpu):
    ret_ip, ret_cs, out_off, out_seg, char_off, char_seg = frame(ram, cpu)
    assert (ret_ip, ret_cs, char_off, char_seg) == (0x104A, 0xF000,
                                                  0x2300, 0x4000)
    link_off = get16(ram, char_seg, char_off + 0xCC)
    link_seg = get16(ram, char_seg, char_off + 0xCE)
    put16(ram, out_seg, out_off, 0)
    put16(ram, out_seg, out_off + 2, 0)
    flags = 0
    if link_off | link_seg:
        index = ram[physical(link_seg, link_off + 0x2E)]
        flags = ram[physical(cpu.ds, 0x5590 + (index << 4))]
        if flags & 0x10:
            put16(ram, out_seg, out_off, link_off)
            put16(ram, out_seg, out_off + 2, link_seg)
        if flags & 0x08:
            if flags & 0x01:
                put16(ram, out_seg, out_off,
                      get16(ram, char_seg, char_off + 0xF8))
                put16(ram, out_seg, out_off + 2,
                      get16(ram, char_seg, char_off + 0xFA))
            if flags & 0x80:
                put16(ram, out_seg, out_off,
                      get16(ram, char_seg, char_off + 0xFC))
                put16(ram, out_seg, out_off + 2,
                      get16(ram, char_seg, char_off + 0xFE))
    combined = get16(ram, out_seg, out_off) | get16(ram, out_seg, out_off + 2)
    cpu.ax = (combined & 0xFF00) | (1 if combined or flags == 0x0A else 0)
    cpu.bx = 0xBEEF
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
        if address == physical(label[4], 0x2E8B):
            state = Cpu(*(emu.reg_read(reg) & 0xFFFF for reg in REGISTERS))
            snapshot = bytearray(emu.mem_read(0, RAM_SIZE))
            expected_calls.append((snap(state), frame(snapshot, state)))
            select_python(snapshot, state)
            state.ip = get16(snapshot, state.ss, state.sp)
            state.sp = (state.sp + 2) & 0xFFFF
            state.cs = get16(snapshot, state.ss, state.sp)
            state.sp = (state.sp + 10) & 0xFFFF
            emu.mem_write(0, bytes(snapshot))
            for (name, _), reg in zip(Cpu._fields_, REGISTERS):
                emu.reg_write(reg, getattr(state, name))
        elif address == physical(0xD000, 0x1234):
            reached[0] = True
            emu.emu_stop()
        else:
            assert 0xF101D <= address < 0xF1086, (label, hex(address))

    original.hook_add(UC_HOOK_CODE, on_code)
    original.emu_start(physical(0xF000, 0x101D), RAM_SIZE, count=80)
    assert reached[0] and len(expected_calls) == (label[0] != 0), label
    expected_ram = bytes(original.mem_read(0, RAM_SIZE))
    ram = (ctypes.c_uint8 * RAM_SIZE).from_buffer_copy(memory)
    actual_calls = []

    @callback_type
    def external(context, ptr, size, state_ptr):
        del context
        assert size == RAM_SIZE
        state = state_ptr.contents
        actual_calls.append((snap(state), frame(ptr, state)))
        select_python(ptr, state)
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
    with tempfile.TemporaryDirectory(prefix="poolrad-u38-101d-") as tmp:
        library = Path(tmp) / "model.so"
        subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic",
                        "-Werror", "-shared", "-fPIC", "-I", str(ROOT / "include"),
                        str(ROOT / "src/recovered/overlay_u38_target_select_101d.c"),
                        str(ROOT / "src/recovered/resident_character_target_select.c"),
                        "-o", str(library)], check=True)
        model = ctypes.CDLL(str(library)).pr_u38_target_select_101d
        model.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t,
                          ctypes.POINTER(Cpu), callback_type, ctypes.c_void_p]
        model.restype = ctypes.c_int
        cases = [(mode, *layout, loaded_segment)
                 for loaded_segment in (0x00BA, 0x3456, 0xD123)
                 for mode in range(8)
                 for layout in ((0x3000, 0xE000, 0x8000),
                                (0x3000, 0x3000, 0x7000),
                                (0xF123, 0xE000, 0x0002))]
        for label in cases:
            compare(label, model, callback_type, body)
    print(f"u38:+101d original-byte full-state parity: {len(cases)} cases")


if __name__ == "__main__":
    main()
