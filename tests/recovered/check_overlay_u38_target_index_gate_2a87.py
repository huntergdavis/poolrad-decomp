#!/usr/bin/env python3
"""Original-byte/full-state parity for GAME.OVR U38 public entry 99."""

import ctypes
from hashlib import sha256
from pathlib import Path
import subprocess
import tempfile

from capstone import Cs, CS_ARCH_X86, CS_MODE_16
from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE

import check_overlay_u38_target_select_101d as helper

ROOT = Path(__file__).resolve().parents[2]
BODY_SHA = "7e7c8ac21b1334814e21a84a1403ca7964c9397ea0477acde8a989e5d271fc40"


def source():
    ovr = helper.OVR.read_bytes()
    assert sha256(ovr).hexdigest() == helper.OVR_SHA
    exe = (ROOT / "resources/DOS/POOLRAD/START.EXE").read_bytes()
    assert sha256(exe).hexdigest() == helper.EXE_SHA
    from sys import path
    path.insert(0, str(ROOT / "tools/overlay"))
    from overlay_inventory import find_units, parse_mz
    unit = next(u for u in
                find_units(exe[parse_mz(exe)["header_bytes"]:], ovr)
                if u["stub_para"] == 0x38)
    assert unit["ovr_offset"] == 0xEC5B
    assert unit["entries"][99] == 0x2A87
    assert unit["entries"][100] == 0x2AE6
    assert not any(0x2A87 <= off < 0x2AE6 for off in unit["fixups"])
    body = ovr[0x116E2:0x11741]
    assert len(body) == 95 and sha256(body).hexdigest() == BODY_SHA
    ins = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(body, 0x2A87))
    assert len(ins) == 37 and sum(i.size for i in ins) == len(body)
    assert [(i.address, i.op_str) for i in ins if i.mnemonic == "call"] == [
        (0x2A96, "0x101d")]
    assert ins[-1].mnemonic == "retf" and ins[-1].address == 0x2AE3
    return body


def make_case(wrapper, callee, label):
    mode, target_index, table_flag, amount, ds, ss, sp, loaded_segment = label
    ram = bytearray(helper.RAM_SIZE)
    for off, body in ((0x101D, callee), (0x2A87, wrapper)):
        at = helper.physical(0xF000, off)
        ram[at:at + len(body)] = body
    helper.put16(ram, 0xF000, 0x1048, loaded_segment)
    cpu = helper.Cpu(0xA5CD, 0x3456, 0x5678, 0x789A, 0x9ABC,
                     0xBCDE, 0x7777, sp, 0xF000, 0x2A87, ds, 0x1111,
                     ss, 0x72D3)
    helper.put16(ram, ss, sp, 0x1234)
    helper.put16(ram, ss, sp + 2, 0xD000)
    for i in range(5):
        helper.put16(ram, ss, sp + 4 + 2 * i, 0xA100 + i)
    helper.put16(ram, ds, 0x5D92, 0x2300)
    helper.put16(ram, ds, 0x5D94, 0x4000)
    if mode != 0:
        helper.put16(ram, 0x4000, 0x23CC, 0x2200)
        helper.put16(ram, 0x4000, 0x23CE, 0x5000)
    for off, value in ((0x23F8, 0x3300), (0x23FA, 0x6000),
                       (0x23FC, 0x4400), (0x23FE, 0x7000)):
        helper.put16(ram, 0x4000, off, value)
    ram[helper.physical(0x5000, 0x222E)] = 7
    ram[helper.physical(ds, 0x5590 + 16 * target_index)] = helper.FLAGS[mode]
    for seg, off in ((0x5000, 0x2200), (0x6000, 0x3300),
                     (0x7000, 0x4400)):
        ram[helper.physical(seg, off + 0x2E)] = target_index
    ram[helper.physical(ds, 0x5589 + 16 * target_index)] = table_flag
    ram[helper.physical(ds, 0x6818)] = amount
    return ram, cpu


def compare(label, model, callback_type, wrapper, callee):
    memory, cpu = make_case(wrapper, callee, label)
    original = Uc(UC_ARCH_X86, UC_MODE_16)
    original.mem_map(0, helper.RAM_SIZE)
    original.mem_write(0, bytes(memory))
    for (name, _), reg in zip(helper.Cpu._fields_, helper.REGISTERS):
        original.reg_write(reg, getattr(cpu, name))
    expected_calls = []
    reached = [False]

    def on_code(emu, address, size, user):
        del size, user
        if address == helper.physical(label[7], 0x2E8B):
            state = helper.Cpu(*(emu.reg_read(reg) & 0xFFFF
                                 for reg in helper.REGISTERS))
            snapshot = bytearray(emu.mem_read(0, helper.RAM_SIZE))
            expected_calls.append((helper.snap(state), helper.frame(snapshot, state)))
            helper.select_python(snapshot, state)
            state.ip = helper.get16(snapshot, state.ss, state.sp)
            state.sp = (state.sp + 2) & 0xFFFF
            state.cs = helper.get16(snapshot, state.ss, state.sp)
            state.sp = (state.sp + 10) & 0xFFFF
            emu.mem_write(0, bytes(snapshot))
            for (name, _), reg in zip(helper.Cpu._fields_, helper.REGISTERS):
                emu.reg_write(reg, getattr(state, name))
        elif address == helper.physical(0xD000, 0x1234):
            reached[0] = True
            emu.emu_stop()
        else:
            assert (0xF101D <= address < 0xF1086 or
                    0xF2A87 <= address < 0xF2AE6), (label, hex(address))

    original.hook_add(UC_HOOK_CODE, on_code)
    original.emu_start(helper.physical(0xF000, 0x2A87), helper.RAM_SIZE,
                       count=130)
    assert reached[0] and len(expected_calls) == (label[0] != 0), label
    expected_ram = bytes(original.mem_read(0, helper.RAM_SIZE))
    ram = (ctypes.c_uint8 * helper.RAM_SIZE).from_buffer_copy(memory)
    actual_calls = []

    @callback_type
    def external(context, ptr, size, state_ptr):
        del context
        assert size == helper.RAM_SIZE
        state = state_ptr.contents
        actual_calls.append((helper.snap(state), helper.frame(ptr, state)))
        helper.select_python(ptr, state)
        return 1

    assert model(ram, helper.RAM_SIZE, ctypes.byref(cpu), external, None) == 0
    if bytes(ram) != expected_ram:
        diff = [i for i, (a, b) in enumerate(zip(ram, expected_ram)) if a != b]
        raise AssertionError((label, "RAM", [hex(i) for i in diff[:12]]))
    assert actual_calls == expected_calls, (label, "calls", actual_calls,
                                            expected_calls)
    for (name, _), reg in zip(helper.Cpu._fields_, helper.REGISTERS):
        actual, expected = getattr(cpu, name), original.reg_read(reg) & 0xFFFF
        assert actual == expected, (label, name, hex(actual), hex(expected))


def main():
    wrapper, callee = source(), helper.source()
    callback_type = ctypes.CFUNCTYPE(
        ctypes.c_int, ctypes.c_void_p, ctypes.POINTER(ctypes.c_uint8),
        ctypes.c_size_t, ctypes.POINTER(helper.Cpu))
    with tempfile.TemporaryDirectory(prefix="poolrad-u38-2a87-") as tmp:
        library = Path(tmp) / "model.so"
        subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic",
                        "-Werror", "-shared", "-fPIC", "-I", str(ROOT / "include"),
                        str(ROOT / "src/recovered/overlay_u38_target_select_101d.c"),
                        str(ROOT / "src/recovered/resident_character_target_select.c"),
                        str(ROOT / "src/recovered/overlay_u38_target_index_gate_2a87.c"),
                        "-o", str(library)], check=True)
        model = ctypes.CDLL(str(library)).pr_u38_target_index_gate_2a87
        model.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t,
                          ctypes.POINTER(helper.Cpu), callback_type,
                          ctypes.c_void_p]
        model.restype = ctypes.c_int
        cases = [(mode, index, flag, amount, *layout, seg)
                 for seg in (0x00BA, 0x3456)
                 for mode in range(8)
                 for index in (0, 7, 255)
                 for flag, amount in ((0, 255), (1, 1), (2, 2),
                                      (0x81, 0))
                 for layout in ((0x3000, 0xE000, 0x8000),
                                (0x3000, 0x3000, 0x7000),
                                (0xF123, 0xE000, 0x0002))]
        for label in cases:
            compare(label, model, callback_type, wrapper, callee)
    print(f"u38:+2a87 composed original-byte full-state parity: {len(cases)} cases")


if __name__ == "__main__":
    main()
