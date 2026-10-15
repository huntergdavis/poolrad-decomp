#!/usr/bin/env python3
"""Original-byte full-state parity for U38 public entry 43, +133F."""

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
BODY_SHA = "f5d32541e8f8b2ce0e4d08ed887a5ddf9c52829f449f74b8c1553a6b0bbfe8ce"


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
    assert [i for i, off in enumerate(unit["entries"]) if off == 0x133F] == [43]
    assert min(off for off in unit["entries"] if off > 0x133F) == 0x1355
    assert [off for off in unit["fixups"] if 0x133F <= off < 0x1355] == [0x134B]
    body = ovr[0xFF9A:0xFFB0]
    assert len(body) == 22 and sha256(body).hexdigest() == BODY_SHA
    ins = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(body, 0x133F))
    assert len(ins) == 9 and sum(i.size for i in ins) == len(body)
    assert [(i.address, i.mnemonic, i.op_str) for i in ins
            if i.mnemonic == "lcall"] == [(0x1348, "lcall", "0xba, 0x2637")]
    assert ins[-1].address == 0x1352 and ins[-1].mnemonic == "retf"
    return body


def call_frame(ram, cpu):
    return tuple(helper.get16(ram, cpu.ss, (cpu.sp + 2 * i) & 0xFFFF)
                 for i in range(5))


def stub(ram, cpu, result):
    helper.put16(ram, cpu.ss, (cpu.sp - 2) & 0xFFFF, cpu.bp)
    ram[helper.physical(cpu.ds, 0x6816)] = result ^ 0x5A
    cpu.ax = (cpu.ax & 0xFF00) | result


def make_case(body, label):
    result, start_ax, ds, ss, sp, loaded_segment = label
    ram = bytearray(helper.RAM_SIZE)
    at = helper.physical(0xF000, 0x133F)
    ram[at:at + len(body)] = body
    helper.put16(ram, 0xF000, 0x134B, loaded_segment)
    cpu = helper.Cpu(start_ax, 0x3456, 0x5678, 0x789A, 0x9ABC,
                     0xBCDE, 0x7777, sp, 0xF000, 0x133F, ds, 0x1111,
                     ss, 0x72D3)
    helper.put16(ram, ss, sp, 0x1234)
    helper.put16(ram, ss, (sp + 2) & 0xFFFF, 0xD000)
    for i in range(5):
        helper.put16(ram, ss, (sp + 4 + 2 * i) & 0xFFFF, 0xA100 + i)
    ram[helper.physical(ds, 0x6816)] = 0xA7
    return ram, cpu


def compare(label, model, callback_type, body):
    memory, cpu = make_case(body, label)
    original = Uc(UC_ARCH_X86, UC_MODE_16)
    original.mem_map(0, helper.RAM_SIZE)
    original.mem_write(0, bytes(memory))
    for (name, _), reg in zip(helper.Cpu._fields_, helper.REGISTERS):
        original.reg_write(reg, getattr(cpu, name))
    expected_calls = []
    reached = [False]
    call_at = helper.physical(label[5], 0x2637)

    def on_code(emu, address, size, user):
        del size, user
        if address == call_at:
            state = helper.Cpu(*(emu.reg_read(reg) & 0xFFFF
                                 for reg in helper.REGISTERS))
            snapshot = bytearray(emu.mem_read(0, helper.RAM_SIZE))
            expected_calls.append((helper.snap(state), call_frame(snapshot, state)))
            stub(snapshot, state, label[0])
            state.ip = helper.get16(snapshot, state.ss, state.sp)
            state.sp = (state.sp + 2) & 0xFFFF
            state.cs = helper.get16(snapshot, state.ss, state.sp)
            state.sp = (state.sp + 6) & 0xFFFF
            emu.mem_write(0, bytes(snapshot))
            for (name, _), reg in zip(helper.Cpu._fields_, helper.REGISTERS):
                emu.reg_write(reg, getattr(state, name))
        elif address == helper.physical(0xD000, 0x1234):
            reached[0] = True
            emu.emu_stop()
        else:
            assert 0xF133F <= address < 0xF1355, (label, hex(address))

    original.hook_add(UC_HOOK_CODE, on_code)
    original.emu_start(helper.physical(0xF000, 0x133F), helper.RAM_SIZE,
                       count=100)
    assert reached[0] and len(expected_calls) == 1, label
    expected_ram = bytes(original.mem_read(0, helper.RAM_SIZE))
    ram = (ctypes.c_uint8 * helper.RAM_SIZE).from_buffer_copy(memory)
    actual_calls = []

    @callback_type
    def external(context, ptr, size, state_ptr):
        del context
        assert size == helper.RAM_SIZE
        state = state_ptr.contents
        actual_calls.append((helper.snap(state), call_frame(ptr, state)))
        stub(ptr, state, label[0])
        return 1

    assert model(ram, helper.RAM_SIZE, ctypes.byref(cpu), external, None) == 0
    if bytes(ram) != expected_ram:
        diff = [i for i, (a, b) in enumerate(zip(ram, expected_ram)) if a != b]
        raise AssertionError((label, "RAM", diff[:20]))
    expected_cpu = helper.Cpu(*(original.reg_read(reg) & 0xFFFF
                                for reg in helper.REGISTERS))
    assert helper.snap(cpu) == helper.snap(expected_cpu), (
        label, helper.snap(cpu), helper.snap(expected_cpu))
    assert actual_calls == expected_calls, (label, actual_calls, expected_calls)


def main():
    body = source()
    with tempfile.TemporaryDirectory() as temp:
        library = Path(temp) / "u38_133f.so"
        subprocess.run(["cc", "-shared", "-fPIC", "-std=c11", "-Wall",
                        "-Wextra", "-Wpedantic", "-Werror", "-Iinclude",
                        "src/recovered/overlay_u38_target_reset_call_133f.c",
                        "-o", str(library)], cwd=ROOT, check=True)
        dll = ctypes.CDLL(str(library))
        callback_type = ctypes.CFUNCTYPE(ctypes.c_int, ctypes.c_void_p,
                                         ctypes.POINTER(ctypes.c_uint8),
                                         ctypes.c_size_t,
                                         ctypes.POINTER(helper.Cpu))
        model = dll.pr_u38_target_reset_call_133f
        model.argtypes = (ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t,
                          ctypes.POINTER(helper.Cpu), callback_type,
                          ctypes.c_void_p)
        model.restype = ctypes.c_int
        cases = [(r, ax, ds, ss, sp, seg)
                 for r in (0, 1, 0x80, 0xFF)
                 for ax in (0x0000, 0xA5CD)
                 for ds, ss, sp, seg in ((0x3000, 0xE000, 0x8000, 0xB123),
                                         (0x3000, 0x3000, 0x0004, 0x2222),
                                         (0x1234, 0xD000, 0x7FF0, 0x4321))]
        for label in cases:
            compare(label, model, callback_type, body)
    print(f"U38 +133F: {len(cases)} original-byte full-RAM/14-CPU cases pass")


if __name__ == "__main__":
    main()
