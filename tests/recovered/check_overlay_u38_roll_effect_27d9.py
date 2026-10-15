#!/usr/bin/env python3
"""Original-byte full-state parity for GAME.OVR U38 public entry 84."""

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
BODY_SHA = "8247af44aa38ad460b80fe1491657b6a84b0228d483b706c4f91f3664180aee5"


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
    assert unit["entries"][84:86] == [0x27D9, 0x2814]
    assert [i for i, off in enumerate(unit["entries"]) if off == 0x27D9] == [84]
    assert [off for off in unit["fixups"] if 0x27D9 <= off < 0x2814] == [
        0x2800, 0x280E]
    body = ovr[0x11434:0x1146F]
    assert len(body) == 59 and sha256(body).hexdigest() == BODY_SHA
    ins = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(body, 0x27D9))
    assert len(ins) == 28 and sum(i.size for i in ins) == len(body)
    assert [(i.address, i.mnemonic, i.op_str) for i in ins
            if "call" in i.mnemonic] == [
        (0x27FD, "lcall", "0xb0, 0x48"),
        (0x280B, "lcall", "0xb0, 0x52")]
    assert ins[-1].address == 0x2811 and ins[-1].mnemonic == "retf"
    return body


def frame(ram, cpu, words):
    return tuple(helper.get16(ram, cpu.ss, cpu.sp + 2 * i)
                 for i in range(words))


def roll_stub(ram, cpu):
    helper.put16(ram, cpu.ss, cpu.sp - 2, cpu.bp)
    value = ram[helper.physical(cpu.ds, 0x6900)]
    cpu.ax = (cpu.ax & 0xFF00) | value


def effect_stub(ram, cpu):
    helper.put16(ram, cpu.ss, cpu.sp - 2, cpu.bp)
    ram[helper.physical(cpu.ds, 0x6818)] = 0x5A
    cpu.ax = 0xBEEF


def make_case(body, label):
    flag, roll_value, ah, first_seg, second_seg, ds, ss, sp = label
    ram = bytearray(helper.RAM_SIZE)
    at = helper.physical(0xF000, 0x27D9)
    ram[at:at + len(body)] = body
    helper.put16(ram, 0xF000, 0x2800, first_seg)
    helper.put16(ram, 0xF000, 0x280E, second_seg)
    cpu = helper.Cpu((ah << 8) | 0xCD, 0x3456, 0x5678, 0x789A,
                     0x9ABC, 0xBCDE, 0x7777, sp, 0xF000, 0x27D9,
                     ds, 0x1111, ss, 0x72D3)
    helper.put16(ram, ss, sp, 0x1234)
    helper.put16(ram, ss, sp + 2, 0xD000)
    for i in range(5):
        helper.put16(ram, ss, sp + 4 + 2 * i, 0xA100 + i)
    ram[helper.physical(ds, 0x6819)] = flag
    ram[helper.physical(ds, 0x6818)] = 0xA7
    ram[helper.physical(ds, 0x6900)] = roll_value
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

    def on_code(emu, address, size, user):
        del size, user
        which = None
        if address == helper.physical(label[3], 0x0048):
            which = "roll"
        elif address == helper.physical(label[4], 0x0052):
            which = "effect"
        if which:
            state = helper.Cpu(*(emu.reg_read(reg) & 0xFFFF
                                 for reg in helper.REGISTERS))
            snapshot = bytearray(emu.mem_read(0, helper.RAM_SIZE))
            expected_calls.append((which, helper.snap(state),
                                   frame(snapshot, state, 7 if which == "roll" else 8)))
            if which == "roll":
                roll_stub(snapshot, state)
                cleanup = 4
            else:
                effect_stub(snapshot, state)
                cleanup = 12
            state.ip = helper.get16(snapshot, state.ss, state.sp)
            state.sp = (state.sp + 2) & 0xFFFF
            state.cs = helper.get16(snapshot, state.ss, state.sp)
            state.sp = (state.sp + 2 + cleanup) & 0xFFFF
            emu.mem_write(0, bytes(snapshot))
            for (name, _), reg in zip(helper.Cpu._fields_, helper.REGISTERS):
                emu.reg_write(reg, getattr(state, name))
        elif address == helper.physical(0xD000, 0x1234):
            reached[0] = True
            emu.emu_stop()
        else:
            assert 0xF27D9 <= address < 0xF2814, (label, hex(address))

    original.hook_add(UC_HOOK_CODE, on_code)
    original.emu_start(helper.physical(0xF000, 0x27D9), helper.RAM_SIZE,
                       count=100)
    assert reached[0]
    assert [x[0] for x in expected_calls] == (
        [] if label[0] & 0x11 else ["roll", "effect"]), label
    expected_ram = bytes(original.mem_read(0, helper.RAM_SIZE))
    ram = (ctypes.c_uint8 * helper.RAM_SIZE).from_buffer_copy(memory)
    actual_calls = []

    @callback_type
    def roll(context, ptr, size, state_ptr):
        del context
        assert size == helper.RAM_SIZE
        state = state_ptr.contents
        actual_calls.append(("roll", helper.snap(state), frame(ptr, state, 7)))
        roll_stub(ptr, state)
        return 1

    @callback_type
    def effect(context, ptr, size, state_ptr):
        del context
        assert size == helper.RAM_SIZE
        state = state_ptr.contents
        actual_calls.append(("effect", helper.snap(state), frame(ptr, state, 8)))
        effect_stub(ptr, state)
        return 1

    assert model(ram, helper.RAM_SIZE, ctypes.byref(cpu),
                 roll, effect, None) == 0
    if bytes(ram) != expected_ram:
        diff = [i for i, (a, b) in enumerate(zip(ram, expected_ram)) if a != b]
        raise AssertionError((label, "RAM", [hex(i) for i in diff[:12]]))
    assert actual_calls == expected_calls, (label, "calls", actual_calls,
                                            expected_calls)
    for (name, _), reg in zip(helper.Cpu._fields_, helper.REGISTERS):
        actual, expected = getattr(cpu, name), original.reg_read(reg) & 0xFFFF
        assert actual == expected, (label, name, hex(actual), hex(expected))


def main():
    body = source()
    callback_type = ctypes.CFUNCTYPE(
        ctypes.c_int, ctypes.c_void_p, ctypes.POINTER(ctypes.c_uint8),
        ctypes.c_size_t, ctypes.POINTER(helper.Cpu))
    with tempfile.TemporaryDirectory(prefix="poolrad-u38-27d9-") as tmp:
        library = Path(tmp) / "model.so"
        subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic",
                        "-Werror", "-shared", "-fPIC", "-I", str(ROOT / "include"),
                        str(ROOT / "src/recovered/overlay_u38_roll_effect_27d9.c"),
                        "-o", str(library)], check=True)
        model = ctypes.CDLL(str(library)).pr_u38_roll_effect_27d9
        model.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t,
                          ctypes.POINTER(helper.Cpu), callback_type,
                          callback_type, ctypes.c_void_p]
        model.restype = ctypes.c_int
        cases = [(flag, roll_value, ah, *segs, *layout)
                 for flag in (0, 1, 0x10, 0x11)
                 for roll_value in (0, 1, 0x7F, 0xFF)
                 for ah in (0, 0xA5)
                 for segs in ((0x3456, 0x4567), (0x00BA, 0x2345))
                 for layout in ((0x3000, 0xE000, 0x8000),
                                (0x3000, 0x3000, 0x7000),
                                (0xF123, 0xE000, 0x0002))]
        for label in cases:
            compare(label, model, callback_type, body)
    print(f"u38:+27d9 original-byte full-state parity: {len(cases)} cases")


if __name__ == "__main__":
    main()
