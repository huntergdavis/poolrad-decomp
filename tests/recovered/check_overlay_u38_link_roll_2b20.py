#!/usr/bin/env python3
"""Full-state original U38:+2B20 / U9E entry-9 composition parity."""

import ctypes
from hashlib import sha256
from pathlib import Path
import subprocess
import sys
import tempfile

from capstone import Cs, CS_ARCH_X86, CS_MODE_16
from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE

import check_overlay_u9e_entry9_roll_latch_0e2e as latch

ROOT = Path(__file__).resolve().parents[2]
BODY_SHA = "9a322acac07936548b9d1257854e48fc256e29bf2dec0f4c58dfab9ca639366b"
STUB = bytes.fromhex("e9de0d")  # :004D JMP +0E2E, synthetic loader dispatch


def source():
    ovr = latch.OVR.read_bytes()
    exe = (ROOT / "resources/DOS/POOLRAD/START.EXE").read_bytes()
    assert len(ovr) == 206105 and sha256(ovr).hexdigest() == latch.OVR_SHA
    assert len(exe) == 65231 and sha256(exe).hexdigest() == latch.EXE_SHA
    sys.path.insert(0, str(ROOT / "tools/overlay"))
    from overlay_inventory import find_units, parse_mz
    unit = next(u for u in find_units(exe[parse_mz(exe)["header_bytes"]:],
                                    ovr) if u["stub_para"] == 0x38)
    assert unit["ovr_offset"] == 0xEC5B
    assert unit["entries"][101:103] == [0x2B20, 0x2B63]
    assert [off for off in unit["fixups"] if 0x2B20 <= off < 0x2B63] == [
        0x2B55]
    body = ovr[0x1177B:0x117BE]
    assert len(body) == 67 and sha256(body).hexdigest() == BODY_SHA
    ins = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(body, 0x2B20))
    assert len(ins) == 25 and sum(i.size for i in ins) == len(body)
    assert [(i.address, i.op_str) for i in ins if i.mnemonic == "lcall"] == [
        (0x2B52, "0xb0, 0x4d")]
    assert ins[-1].address == 0x2B60 and ins[-1].mnemonic == "retf"
    return body


def make_case(wrapper, callee, label):
    mode, ds, ss, sp, loaded_segment, seed, override = label
    ram = bytearray(latch.RAM_SIZE)
    for seg, off, body in ((0xF000, 0x2B20, wrapper),
                           (loaded_segment, 0x004D, STUB),
                           (loaded_segment, 0x0E2E, callee)):
        at = latch.physical(seg, off)
        ram[at:at + len(body)] = body
    latch.put16(ram, 0xF000, 0x2B55, loaded_segment)
    cpu = latch.Cpu(0xA5CD, 0x3456, 0x5678, 0x789A, 0x9ABC,
                    0xBCDE, 0x7777, sp, 0xF000, 0x2B20, ds, 0x1111,
                    ss, 0x72D3)
    latch.put16(ram, ss, sp, 0x1234)
    latch.put16(ram, ss, sp + 2, 0xD000)
    for i in range(5):
        latch.put16(ram, ss, sp + 4 + 2 * i, 0xA100 + i)
    latch.put16(ram, ds, 0x5D92, 0x2300)
    latch.put16(ram, ds, 0x5D94, 0x4000)
    if mode != 0:
        latch.put16(ram, 0x4000, 0x23CC, 0x2200)
        latch.put16(ram, 0x4000, 0x23CE, 0x5000)
    ram[latch.physical(0x5000, 0x222E)] = 0x55 if mode == 2 else 0x54
    ram[latch.physical(ds, 0x6818)] = 0xA7
    ram[latch.physical(ds, 0x681C)] = 0xB8
    latch.put16(ram, ds, 0x4428, seed & 0xFFFF)
    latch.put16(ram, ds, 0x442A, seed >> 16)
    return ram, cpu


def frame(ram, cpu, words):
    return tuple(latch.get16(ram, cpu.ss, cpu.sp + 2 * i)
                 for i in range(words))


def roll_effect(ram, cpu, override):
    latch.roll_python(ram, cpu)
    if override is not None:
        cpu.ax = (cpu.ax & 0xFF00) | override


def compare(label, model, callee_model, callback_type, wrapper, callee):
    memory, cpu = make_case(wrapper, callee, label)
    original = Uc(UC_ARCH_X86, UC_MODE_16)
    original.mem_map(0, latch.RAM_SIZE)
    original.mem_write(0, bytes(memory))
    for (name, _), reg in zip(latch.Cpu._fields_, latch.REGISTERS):
        original.reg_write(reg, getattr(cpu, name))
    expected_far, expected_roll = [], []
    reached = [False]

    def on_code(emu, address, size, user):
        del size, user
        if address == latch.physical(label[4], 0x004D):
            state = latch.Cpu(*(emu.reg_read(reg) & 0xFFFF
                                for reg in latch.REGISTERS))
            snapshot = bytearray(emu.mem_read(0, latch.RAM_SIZE))
            expected_far.append((latch.snap(state), frame(snapshot, state, 4)))
        elif address == latch.physical(label[4], 0x0DE3):
            state = latch.Cpu(*(emu.reg_read(reg) & 0xFFFF
                                for reg in latch.REGISTERS))
            snapshot = bytearray(emu.mem_read(0, latch.RAM_SIZE))
            expected_roll.append((latch.snap(state), frame(snapshot, state, 4)))
            roll_effect(snapshot, state, label[6])
            state.ip = latch.get16(snapshot, state.ss, state.sp)
            state.sp = (state.sp + 2) & 0xFFFF
            state.cs = latch.get16(snapshot, state.ss, state.sp)
            state.sp = (state.sp + 6) & 0xFFFF
            emu.mem_write(0, bytes(snapshot))
            for (name, _), reg in zip(latch.Cpu._fields_, latch.REGISTERS):
                emu.reg_write(reg, getattr(state, name))
        elif address == latch.physical(0xD000, 0x1234):
            reached[0] = True
            emu.emu_stop()
        else:
            assert (0xF2B20 <= address < 0xF2B63 or
                    address == latch.physical(label[4], 0x004D) or
                    latch.physical(label[4], 0x0E2E) <= address <
                    latch.physical(label[4], 0x0E52)), (label, hex(address))

    original.hook_add(UC_HOOK_CODE, on_code)
    original.emu_start(latch.physical(0xF000, 0x2B20), latch.RAM_SIZE,
                       count=100)
    assert reached[0]
    assert len(expected_far) == len(expected_roll) == (label[0] == 2)
    expected_ram = bytes(original.mem_read(0, latch.RAM_SIZE))
    ram = (ctypes.c_uint8 * latch.RAM_SIZE).from_buffer_copy(memory)
    actual_far, actual_roll = [], []

    @callback_type
    def roll(context, ptr, size, state_ptr):
        del context
        assert size == latch.RAM_SIZE
        state = state_ptr.contents
        actual_roll.append((latch.snap(state), frame(ptr, state, 4)))
        roll_effect(ptr, state, label[6])
        return 1

    @callback_type
    def external(context, ptr, size, state_ptr):
        del context
        assert size == latch.RAM_SIZE
        state = state_ptr.contents
        actual_far.append((latch.snap(state), frame(ptr, state, 4)))
        state.ip = 0x0E2E  # synthetic loader-stub JMP
        assert callee_model(ptr, size, state_ptr, roll, None) == 0
        return 1

    assert model(ram, latch.RAM_SIZE, ctypes.byref(cpu), external, None) == 0
    if bytes(ram) != expected_ram:
        diff = [i for i, (a, b) in enumerate(zip(ram, expected_ram)) if a != b]
        raise AssertionError((label, "RAM", [hex(i) for i in diff[:12]]))
    assert actual_far == expected_far, (label, "far", actual_far,
                                        expected_far)
    assert actual_roll == expected_roll, (label, "roll", actual_roll,
                                          expected_roll)
    for (name, _), reg in zip(latch.Cpu._fields_, latch.REGISTERS):
        actual, expected = getattr(cpu, name), original.reg_read(reg) & 0xFFFF
        assert actual == expected, (label, name, hex(actual), hex(expected))


def main():
    wrapper, callee = source(), latch.source()
    callback_type = ctypes.CFUNCTYPE(
        ctypes.c_int, ctypes.c_void_p, ctypes.POINTER(ctypes.c_uint8),
        ctypes.c_size_t, ctypes.POINTER(latch.Cpu))
    with tempfile.TemporaryDirectory(prefix="poolrad-u38-2b20-") as tmp:
        library = Path(tmp) / "model.so"
        subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic",
                        "-Werror", "-shared", "-fPIC", "-I", str(ROOT / "include"),
                        str(ROOT / "src/recovered/overlay_u9e_entry9_roll_latch_0e2e.c"),
                        str(ROOT / "src/recovered/overlay_u38_link_roll_2b20.c"),
                        str(ROOT / "src/recovered/system_random_fullstate_11c0.c"),
                        str(ROOT / "src/recovered/system_seed_fullstate_120f.c"),
                        str(ROOT / "src/recovered/overlay_u9e_entry8_fullstate_compose_0de3.c"),
                        str(ROOT / "src/recovered/overlay_u9e_entry8_fullstate_loop_0e05.c"),
                        str(ROOT / "src/recovered/overlay_u9e_entry8_system_call_0e0e.c"),
                        str(ROOT / "src/recovered/overlay_u9e_entry8_fullstate_tail_0e1f.c"),
                        str(ROOT / "src/recovered/overlay_u9e_entry8_roll.c"),
                        str(ROOT / "src/recovered/overlay_u9e_entry8_fullstate_postcall_0e13.c"),
                        str(ROOT / "src/recovered/overlay_u9e_entry8_fullstate_prefix_0de3.c"),
                        "-o", str(library)], check=True)
        model = ctypes.CDLL(str(library)).pr_u38_link_roll_2b20
        callee_model = ctypes.CDLL(str(library)).pr_u9e_entry9_roll_latch_0e2e
        for fn in (model, callee_model):
            fn.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t,
                           ctypes.POINTER(latch.Cpu), callback_type,
                           ctypes.c_void_p]
            fn.restype = ctypes.c_int
        cases = [(mode, *layout, segment, seed, override)
                 for segment in (0xB000, 0xD123)
                 for mode in (0, 1, 2)
                 for layout in ((0x3000, 0xE000, 0x8000),
                                (0x3000, 0x3000, 0x7000),
                                (0xF123, 0xE000, 0x0002))
                 for seed, override in ((0x12345678, None),
                                        (0xFF001122, 0x0F),
                                        (0x87654321, 0xFF))]
        for label in cases:
            compare(label, model, callee_model, callback_type,
                    wrapper, callee)
    print(f"u38:+2b20/U9E-entry9 original-byte full-state parity: {len(cases)} cases")


if __name__ == "__main__":
    main()
