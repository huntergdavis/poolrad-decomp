#!/usr/bin/env python3
"""Original-byte full-state parity for U38 public entry 55, +1699."""

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
BODY_SHA = "26dcadfce0b6eea4f5c1f898eb4eae5ddba59c48914064cf0c9a946f1dcf14ff"
DISPATCH_SHA = "43cff4c17d026213e5b7fbc0ffeb0a549c481931bd8f3919dd92cf8e87f5444c"
ROLL_SHA = "7df225682ebb94351984048ef7a4fdccc8de60f9ef2450751a696fde74c583ba"


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
    assert [i for i, off in enumerate(unit["entries"]) if off == 0x1699] == [55]
    assert min(off for off in unit["entries"] if off > 0x1699) == 0x16BB
    assert [off for off in unit["fixups"] if 0x1699 <= off < 0x16BB] == [0x16AB]
    body = ovr[0x102F4:0x10316]
    assert len(body) == 34 and sha256(body).hexdigest() == BODY_SHA
    assert sha256(ovr[0x10246:0x102B8]).hexdigest() == DISPATCH_SHA
    assert sha256(ovr[0x2BCCD:0x2BD18]).hexdigest() == ROLL_SHA
    ins = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(body, 0x1699))
    assert len(ins) == 17 and sum(i.size for i in ins) == len(body)
    assert [(i.address, i.mnemonic, i.op_str) for i in ins
            if i.mnemonic in ("call", "lcall")] == [
                (0x16A8, "lcall", "0xb0, 0x48"),
                (0x16B4, "call", "0x15eb")]
    assert ins[-1].address == 0x16B8 and ins[-1].mnemonic == "retf"
    return body


def call_frame(ram, cpu):
    return tuple(helper.get16(ram, cpu.ss, cpu.sp + 2 * i)
                 for i in range(6))


def stub(ram, cpu, result):
    helper.put16(ram, cpu.ss, cpu.sp - 2, cpu.bp)
    if cpu.ip == 0x0048:
        ram[helper.physical(cpu.ds, 0x680F)] = 0x3C
        cpu.ax = (cpu.ax & 0xFF00) | result
    else:
        assert cpu.ip == 0x15EB
        ram[helper.physical(cpu.ds, 0x6818)] = 0x2C
        cpu.ax = 0xBEEF


def make_case(body, label):
    result, start_ax, ds, ss, sp, loaded_segment = label
    ram = bytearray(helper.RAM_SIZE)
    at = helper.physical(0xF000, 0x1699)
    ram[at:at + len(body)] = body
    helper.put16(ram, 0xF000, 0x16AB, loaded_segment)
    cpu = helper.Cpu(start_ax, 0x3456, 0x5678, 0x789A, 0x9ABC,
                     0xBCDE, 0x7777, sp, 0xF000, 0x1699, ds, 0x1111,
                     ss, 0x72D3)
    helper.put16(ram, ss, sp, 0x1234)
    helper.put16(ram, ss, sp + 2, 0xD000)
    for i in range(5):
        helper.put16(ram, ss, sp + 4 + 2 * i, 0xA100 + i)
    ram[helper.physical(ds, 0x680F)] = 0xA7
    ram[helper.physical(ds, 0x6818)] = 0xB8
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
    roll_at = helper.physical(label[5], 0x0048)
    dispatch_at = helper.physical(0xF000, 0x15EB)

    def on_code(emu, address, size, user):
        del size, user
        if address in (roll_at, dispatch_at):
            state = helper.Cpu(*(emu.reg_read(reg) & 0xFFFF
                                 for reg in helper.REGISTERS))
            snapshot = bytearray(emu.mem_read(0, helper.RAM_SIZE))
            expected_calls.append((helper.snap(state), call_frame(snapshot, state)))
            stub(snapshot, state, label[0])
            state.ip = helper.get16(snapshot, state.ss, state.sp)
            state.sp = (state.sp + 2) & 0xFFFF
            state.cs = helper.get16(snapshot, state.ss, state.sp)
            state.sp = (state.sp + (6 if address == roll_at else 10)) & 0xFFFF
            emu.mem_write(0, bytes(snapshot))
            for (name, _), reg in zip(helper.Cpu._fields_, helper.REGISTERS):
                emu.reg_write(reg, getattr(state, name))
        elif address == helper.physical(0xD000, 0x1234):
            reached[0] = True
            emu.emu_stop()
        else:
            assert 0xF1699 <= address < 0xF16BB, (label, hex(address))

    original.hook_add(UC_HOOK_CODE, on_code)
    original.emu_start(helper.physical(0xF000, 0x1699), helper.RAM_SIZE,
                       count=100)
    assert reached[0] and len(expected_calls) == 2, label
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
        raise AssertionError((label, "RAM", [hex(i) for i in diff[:12]]))
    assert actual_calls == expected_calls, (label, actual_calls, expected_calls)
    for (name, _), reg in zip(helper.Cpu._fields_, helper.REGISTERS):
        actual, expected = getattr(cpu, name), original.reg_read(reg) & 0xFFFF
        assert actual == expected, (label, name, hex(actual), hex(expected))


def main():
    body = source()
    callback_type = ctypes.CFUNCTYPE(
        ctypes.c_int, ctypes.c_void_p, ctypes.POINTER(ctypes.c_uint8),
        ctypes.c_size_t, ctypes.POINTER(helper.Cpu))
    with tempfile.TemporaryDirectory(prefix="poolrad-u38-1699-") as tmp:
        library = Path(tmp) / "model.so"
        subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic",
                        "-Werror", "-shared", "-fPIC", "-I", str(ROOT / "include"),
                        str(ROOT / "src/recovered/overlay_u38_roll_dispatch_1699.c"),
                        "-o", str(library)], check=True)
        model = ctypes.CDLL(str(library)).pr_u38_roll_dispatch_1699
        model.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t,
                          ctypes.POINTER(helper.Cpu), callback_type,
                          ctypes.c_void_p]
        model.restype = ctypes.c_int
        cases = [(result, start_ax, *layout, segment)
                 for segment in (0x00BA, 0x3456)
                 for result in (0, 5, 0x80, 0xFF)
                 for start_ax in (0x00CD, 0xA5CD)
                 for layout in ((0x3000, 0xE000, 0x8000),
                                (0x3000, 0x3000, 0x7000),
                                (0xF123, 0xE000, 0x0002))]
        for label in cases:
            compare(label, model, callback_type, body)
    print(f"u38:+1699 original-byte full-state parity: {len(cases)} cases")


if __name__ == "__main__":
    main()
