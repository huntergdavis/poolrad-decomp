#!/usr/bin/env python3
"""Original-byte full-state parity for U38 public entry 104, with +0FAC hooked."""

import ctypes
from hashlib import sha256
from pathlib import Path
import subprocess
import sys
import tempfile

from capstone import Cs, CS_ARCH_X86, CS_MODE_16
from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE

import check_overlay_u38_target_select_101d as helper

ROOT = Path(__file__).resolve().parents[2]
PUBLIC_SHA = "3814d2a3bd6e04f11ffe90f79dd045db903a7465d1c5bd8198ea9ffe84767538"
CODE_SHA = "10817b3e62b587de5facc5278c0fc6bd0e09fe0eb53a80e2c445d795582ac5fe"
TAIL_SHA = "84fe9ec3307cb912bd26e9dafdd2b0c423fe57ae895878ffbb486064048fd167"
CALLEE_SHA = "7fdce32e9c2572752ae62a3d0dab47f6aec34c857b3e0dfe380465a981b1bce9"


def source():
    ovr = helper.OVR.read_bytes()
    exe = (ROOT / "resources/DOS/POOLRAD/START.EXE").read_bytes()
    assert len(ovr) == 206105 and sha256(ovr).hexdigest() == helper.OVR_SHA
    assert len(exe) == 65231 and sha256(exe).hexdigest() == helper.EXE_SHA
    sys.path.insert(0, str(ROOT / "tools/overlay"))
    from overlay_inventory import find_units, parse_mz
    unit = next(u for u in find_units(exe[parse_mz(exe)["header_bytes"]:],
                                    ovr) if u["stub_para"] == 0x38)
    assert unit["ovr_offset"] == 0xEC5B
    assert unit["entries"][104:106] == [0x2BC8, 0x2C1C]
    assert [i for i, off in enumerate(unit["entries"]) if off == 0x2BC8] == [104]
    assert not any(0x2BC8 <= off < 0x2C1C for off in unit["fixups"])
    public = ovr[0x11823:0x11877]
    code, tail = public[:73], public[73:]
    assert len(public) == 84 and sha256(public).hexdigest() == PUBLIC_SHA
    assert len(code) == 73 and sha256(code).hexdigest() == CODE_SHA
    assert len(tail) == 11 and sha256(tail).hexdigest() == TAIL_SHA
    assert sha256(ovr[0xFC07:0xFC78]).hexdigest() == CALLEE_SHA
    ins = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(code, 0x2BC8))
    assert len(ins) == 26 and sum(i.size for i in ins) == len(code)
    assert [(i.address, i.op_str) for i in ins if i.mnemonic == "call"] == [
        (0x2C08, "0xfac")]
    assert ins[-1].address == 0x2C0E and ins[-1].mnemonic == "retf"
    return public


def callback_frame(ram, cpu):
    return tuple(helper.get16(ram, cpu.ss, cpu.sp + 2 * i)
                 for i in range(5))


def stub(ram, cpu):
    helper.put16(ram, cpu.ss, cpu.sp - 2, cpu.bp)
    ram[helper.physical(cpu.ds, 0x6818)] = 0x5A
    cpu.ax = 0xBEEF


def make_case(public, label):
    linked, marker, ds, ss, sp = label
    ram = bytearray(helper.RAM_SIZE)
    at = helper.physical(0xF000, 0x2BC8)
    ram[at:at + len(public)] = public
    cpu = helper.Cpu(0xA5CD, 0x3456, 0x5678, 0x789A, 0x9ABC,
                     0xBCDE, 0x7777, sp, 0xF000, 0x2BC8, ds, 0x1111,
                     ss, 0x72D3)
    helper.put16(ram, ss, sp, 0x1234)
    helper.put16(ram, ss, sp + 2, 0xD000)
    for i in range(5):
        helper.put16(ram, ss, sp + 4 + 2 * i, 0xA100 + i)
    helper.put16(ram, ds, 0x5D92, 0x2300)
    helper.put16(ram, ds, 0x5D94, 0x4000)
    if linked:
        helper.put16(ram, 0x4000, 0x23CC, 0x2200)
        helper.put16(ram, 0x4000, 0x23CE, 0x5000)
    ram[helper.physical(0x5000, 0x222E)] = marker
    ram[helper.physical(ds, 0x6818)] = 0xA7
    return ram, cpu


def compare(label, model, callback_type, public):
    memory, cpu = make_case(public, label)
    original = Uc(UC_ARCH_X86, UC_MODE_16)
    original.mem_map(0, helper.RAM_SIZE)
    original.mem_write(0, bytes(memory))
    for (name, _), reg in zip(helper.Cpu._fields_, helper.REGISTERS):
        original.reg_write(reg, getattr(cpu, name))
    expected_calls = []
    reached = [False]

    def on_code(emu, address, size, user):
        del size, user
        if address == helper.physical(0xF000, 0x0FAC):
            state = helper.Cpu(*(emu.reg_read(reg) & 0xFFFF
                                 for reg in helper.REGISTERS))
            snapshot = bytearray(emu.mem_read(0, helper.RAM_SIZE))
            expected_calls.append((helper.snap(state), callback_frame(snapshot, state)))
            stub(snapshot, state)
            state.ip = helper.get16(snapshot, state.ss, state.sp)
            state.sp = (state.sp + 2) & 0xFFFF
            state.cs = helper.get16(snapshot, state.ss, state.sp)
            state.sp = (state.sp + 8) & 0xFFFF
            emu.mem_write(0, bytes(snapshot))
            for (name, _), reg in zip(helper.Cpu._fields_, helper.REGISTERS):
                emu.reg_write(reg, getattr(state, name))
        elif address == helper.physical(0xD000, 0x1234):
            reached[0] = True
            emu.emu_stop()
        else:
            assert 0xF2BC8 <= address < 0xF2C11, (label, hex(address))

    original.hook_add(UC_HOOK_CODE, on_code)
    original.emu_start(helper.physical(0xF000, 0x2BC8), helper.RAM_SIZE,
                       count=100)
    assert reached[0] and len(expected_calls) == (
        label[0] and label[1] in (0x57, 0x58)), label
    expected_ram = bytes(original.mem_read(0, helper.RAM_SIZE))
    ram = (ctypes.c_uint8 * helper.RAM_SIZE).from_buffer_copy(memory)
    actual_calls = []

    @callback_type
    def external(context, ptr, size, state_ptr):
        del context
        assert size == helper.RAM_SIZE
        state = state_ptr.contents
        actual_calls.append((helper.snap(state), callback_frame(ptr, state)))
        stub(ptr, state)
        return 1

    assert model(ram, helper.RAM_SIZE, ctypes.byref(cpu), external, None) == 0
    if bytes(ram) != expected_ram:
        diff = [i for i, (a, b) in enumerate(zip(ram, expected_ram)) if a != b]
        raise AssertionError((label, "RAM", [hex(i) for i in diff[:12]]))
    assert actual_calls == expected_calls, (label, actual_calls, expected_calls)
    for (name, _), reg in zip(helper.Cpu._fields_, helper.REGISTERS):
        actual, expected = getattr(cpu, name), original.reg_read(reg) & 0xFFFF
        assert actual == expected, (label, name, hex(actual), hex(expected))


def main():
    public = source()
    callback_type = ctypes.CFUNCTYPE(
        ctypes.c_int, ctypes.c_void_p, ctypes.POINTER(ctypes.c_uint8),
        ctypes.c_size_t, ctypes.POINTER(helper.Cpu))
    with tempfile.TemporaryDirectory(prefix="poolrad-u38-2bc8-") as tmp:
        library = Path(tmp) / "model.so"
        subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic",
                        "-Werror", "-shared", "-fPIC", "-I", str(ROOT / "include"),
                        str(ROOT / "src/recovered/overlay_u38_link_message_2bc8.c"),
                        "-o", str(library)], check=True)
        model = ctypes.CDLL(str(library)).pr_u38_link_message_2bc8
        model.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t,
                          ctypes.POINTER(helper.Cpu), callback_type,
                          ctypes.c_void_p]
        model.restype = ctypes.c_int
        cases = [(linked, marker, *layout)
                 for linked in (0, 1)
                 for marker in (0, 0x56, 0x57, 0x58, 0x80)
                 for layout in ((0x3000, 0xE000, 0x8000),
                                (0x3000, 0x3000, 0x7000),
                                (0xF123, 0xE000, 0x0002))]
        for label in cases:
            compare(label, model, callback_type, public)
    print(f"u38:+2bc8 original-byte full-state parity: {len(cases)} cases")


if __name__ == "__main__":
    main()
