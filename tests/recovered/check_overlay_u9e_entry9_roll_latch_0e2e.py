#!/usr/bin/env python3
"""Original-byte/full-state parity for GAME.OVR U9E:+0E2E entry 9."""

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
BODY_SHA = "08dc2c3413d0fcfa12d26b589873f619761741e3f38c235477e89ed3a7350443"
CALLEE_SHA = "7df225682ebb94351984048ef7a4fdccc8de60f9ef2450751a696fde74c583ba"
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
    ovr = OVR.read_bytes()
    exe = (ROOT / "resources/DOS/POOLRAD/START.EXE").read_bytes()
    assert len(ovr) == 206105 and sha256(ovr).hexdigest() == OVR_SHA
    assert len(exe) == 65231 and sha256(exe).hexdigest() == EXE_SHA
    sys.path.insert(0, str(ROOT / "tools/overlay"))
    from overlay_inventory import find_units, parse_mz
    units = {u["stub_para"]: u for u in
             find_units(exe[parse_mz(exe)["header_bytes"]:], ovr)}
    unit = units[0x9E]
    assert unit["ovr_offset"] == 0x2AEEA
    assert unit["entries"][9:11] == [0x0E2E, 0x0E52]
    body = ovr[0x2BD18:0x2BD3C]
    assert len(body) == 36 and sha256(body).hexdigest() == BODY_SHA
    ins = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(body, 0x0E2E))
    assert len(ins) == 16 and sum(i.size for i in ins) == len(body)
    assert [(i.address, i.op_str) for i in ins if i.mnemonic == "call"] == [
        (0x0E43, "0xde3")]
    assert ins[-1].address == 0x0E4F and ins[-1].mnemonic == "retf"
    callers = {}
    for u in units.values():
        code = ovr[u["ovr_offset"]:u["ovr_offset"] + u["code_size"]]
        hits = [fx - 3 for fx in u["fixups"] if fx >= 3 and
                fx + 1 < len(code) and code[fx - 3] == 0x9A and
                int.from_bytes(code[fx - 2:fx], "little") == 0x004D and
                int.from_bytes(code[fx:fx + 2], "little") == 0x00B0]
        if hits:
            callers[u["stub_para"]] = hits
    assert callers == {0x38: [0x1835, 0x1A37, 0x1A6A, 0x2B52, 0x2CE8,
                             0x2D6A], 0x5E: [0x1B4],
                       0x86: [0x10BD, 0x1460, 0x14E9, 0x2744, 0x306E]}
    return body


def callee_source():
    body = OVR.read_bytes()[0x2BCCD:0x2BD18]
    assert len(body) == 75 and sha256(body).hexdigest() == CALLEE_SHA
    ins = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(body, 0x0DE3))
    assert len(ins) == 29 and sum(i.size for i in ins) == len(body)
    return body


def make_case(body, count, sides, ds, ss, sp):
    ram = bytearray(RAM_SIZE)
    pos = physical(0xF000, 0x0E2E)
    ram[pos:pos + len(body)] = body
    cpu = Cpu(0xA5CD, 0x3456, 0x5678, 0x789A, 0x9ABC,
              0xBCDE, 0x7777, sp, 0xF000, 0x0E2E, ds, 0x1111,
              ss, 0x72D3)
    ram[physical(ds, 0x681C)] = 0xA7
    put16(ram, ds, 0x4428, 0x5678)
    put16(ram, ds, 0x442A, 0x1234)
    put16(ram, ss, sp, 0x1234)
    put16(ram, ss, sp + 2, 0xD000)
    put16(ram, ss, sp + 4, 0x7700 | sides)
    put16(ram, ss, sp + 6, 0x9900 | count)
    return ram, cpu


def arguments(ram, cpu):
    return (get16(ram, cpu.ss, cpu.sp + 4),
            get16(ram, cpu.ss, cpu.sp + 6))


def roll_python(ram, cpu):
    sides, count = arguments(ram, cpu)
    seed = get16(ram, cpu.ds, 0x4428) | get16(ram, cpu.ds, 0x442A) << 16
    result = 0
    for _ in range(count & 255):
        seed = (seed * 0x08088405 + 1) & 0xFFFFFFFF
        die = (seed >> 16) % (sides & 255) if sides & 255 else 0
        result = (result + die + 1) & 255
    put16(ram, cpu.ds, 0x4428, seed & 0xFFFF)
    put16(ram, cpu.ds, 0x442A, seed >> 16)
    cpu.ax = (cpu.ax & 0xFF00) | result
    cpu.bx = 0xBEEF
    cpu.dx = 0xCB67
    cpu.flags = 0x0247


def compare(label, model, helper, callback_type, body):
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
        if address == physical(0xF000, 0x0DE3):
            state = Cpu(*(emu.reg_read(reg) & 0xFFFF for reg in REGISTERS))
            snapshot = bytearray(emu.mem_read(0, RAM_SIZE))
            assert get16(snapshot, state.ss, state.sp) == 0x0E46
            assert get16(snapshot, state.ss, state.sp + 2) == 0xF000
            expected_calls.append((snap(state), arguments(snapshot, state)))
            roll_python(snapshot, state)
            state.ip = get16(snapshot, state.ss, state.sp)
            state.sp = (state.sp + 2) & 0xFFFF
            state.cs = get16(snapshot, state.ss, state.sp)
            state.sp = (state.sp + 6) & 0xFFFF
            emu.mem_write(0, bytes(snapshot))
            for (name, _), reg in zip(Cpu._fields_, REGISTERS):
                emu.reg_write(reg, getattr(state, name))
        elif address == physical(0xD000, 0x1234):
            reached[0] = True
            emu.emu_stop()
        else:
            assert 0xF0E2E <= address < 0xF0E52, (label, hex(address))

    original.hook_add(UC_HOOK_CODE, on_code)
    original.emu_start(physical(0xF000, 0x0E2E), RAM_SIZE, count=60)
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
        sides, count = arguments(ptr, state)
        seed = ctypes.c_uint32(get16(ptr, state.ds, 0x4428) |
                               get16(ptr, state.ds, 0x442A) << 16)
        al = ctypes.c_uint8()
        assert helper(count, sides, ctypes.byref(seed), ctypes.byref(al), None) == 0
        put16(ptr, state.ds, 0x4428, seed.value & 0xFFFF)
        put16(ptr, state.ds, 0x442A, seed.value >> 16)
        state.ax = (state.ax & 0xFF00) | al.value
        state.bx = 0xBEEF
        state.dx = 0xCB67
        state.flags = 0x0247
        return 1

    assert model(ram, RAM_SIZE, ctypes.byref(cpu), external, None) == 0
    if bytes(ram) != expected_ram:
        diff = [i for i, (a, b) in enumerate(zip(ram, expected_ram)) if a != b]
        raise AssertionError((label, "RAM", [hex(i) for i in diff[:12]]))
    assert actual_calls == expected_calls, (label, "calls", actual_calls, expected_calls)
    for (name, _), reg in zip(Cpu._fields_, REGISTERS):
        actual, expected = getattr(cpu, name), original.reg_read(reg) & 0xFFFF
        assert actual == expected, (label, name, hex(actual), hex(expected))


def compare_composed(count, sides, wrapper, callee, helper):
    memory, cpu = make_case(wrapper, count, sides, 0x3000, 0xE000, 0x8000)
    pos = physical(0xF000, 0x0DE3)
    memory[pos:pos + len(callee)] = callee
    original = Uc(UC_ARCH_X86, UC_MODE_16)
    original.mem_map(0, RAM_SIZE)
    original.mem_write(0, bytes(memory))
    for (name, _), reg in zip(Cpu._fields_, REGISTERS):
        original.reg_write(reg, getattr(cpu, name))
    calls = [0]
    reached = [False]

    def read_word(emu, seg, off):
        a = physical(seg, off)
        return int.from_bytes(emu.mem_read(a, 2), "little")

    def on_code(emu, address, size, user):
        del size, user
        if address == physical(0x0AF8, 0x11C0):
            ss = emu.reg_read(UC_X86_REG_SS) & 0xFFFF
            sp = emu.reg_read(UC_X86_REG_SP) & 0xFFFF
            ds = emu.reg_read(UC_X86_REG_DS) & 0xFFFF
            assert read_word(emu, ss, sp) == 0x0E13
            assert read_word(emu, ss, sp + 2) == 0xF000
            rng = read_word(emu, ss, sp + 4)
            assert rng <= 255
            seed = read_word(emu, ds, 0x4428) | read_word(emu, ds, 0x442A) << 16
            seed = (seed * 0x08088405 + 1) & 0xFFFFFFFF
            a = physical(ds, 0x4428)
            emu.mem_write(a, seed.to_bytes(4, "little"))
            emu.reg_write(UC_X86_REG_AX, (seed >> 16) % rng if rng else 0)
            emu.reg_write(UC_X86_REG_SP, (sp + 6) & 0xFFFF)
            emu.reg_write(UC_X86_REG_CS, 0xF000)
            emu.reg_write(UC_X86_REG_IP, 0x0E13)
            calls[0] += 1
        elif address == physical(0xD000, 0x1234):
            reached[0] = True
            emu.emu_stop()
        else:
            assert 0xF0DE3 <= address < 0xF0E52, (count, sides, hex(address))

    original.hook_add(UC_HOOK_CODE, on_code)
    original.emu_start(physical(0xF000, 0x0E2E), RAM_SIZE, count=10000)
    assert reached[0] and calls[0] == count, (count, sides, calls[0])
    seed = ctypes.c_uint32(0x12345678)
    al = ctypes.c_uint8()
    assert helper(count, sides, ctypes.byref(seed), ctypes.byref(al), None) == 0
    assert (original.reg_read(UC_X86_REG_AX) & 255) == al.value
    assert original.reg_read(UC_X86_REG_SP) == 0x8008
    assert original.reg_read(UC_X86_REG_CS) == 0xD000
    assert original.reg_read(UC_X86_REG_IP) == 0x1234
    assert original.mem_read(physical(0x3000, 0x681C), 1)[0] == count
    actual_seed = read_word(original, 0x3000, 0x4428) | \
                  read_word(original, 0x3000, 0x442A) << 16
    assert actual_seed == seed.value, (count, sides, hex(actual_seed), hex(seed.value))


def main():
    body = source()
    callee = callee_source()
    callback_type = ctypes.CFUNCTYPE(
        ctypes.c_int, ctypes.c_void_p, ctypes.POINTER(ctypes.c_uint8),
        ctypes.c_size_t, ctypes.POINTER(Cpu))
    with tempfile.TemporaryDirectory(prefix="poolrad-u9e-0e2e-") as tmp:
        library = Path(tmp) / "model.so"
        subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic",
                        "-Werror", "-shared", "-fPIC", "-I", str(ROOT / "include"),
                        str(ROOT / "src/recovered/overlay_u9e_entry9_roll_latch_0e2e.c"),
                        str(ROOT / "src/recovered/overlay_u9e_entry8_roll.c"),
                        # 08de57fb added pr_u9e_roll_latch_0e2e_cpu to the same file; the
                        # library cannot load without that function's dice-chain closure.
                        *(str(ROOT / "src/recovered" / name) for name in (
                            "overlay_u9e_entry8_fullstate_compose_0de3.c",
                            "overlay_u9e_entry8_fullstate_prefix_0de3.c",
                            "overlay_u9e_entry8_system_call_0e0e.c",
                            "overlay_u9e_entry8_fullstate_postcall_0e13.c",
                            "overlay_u9e_entry8_fullstate_loop_0e05.c",
                            "overlay_u9e_entry8_fullstate_tail_0e1f.c",
                            "system_random_fullstate_11c0.c",
                            "system_seed_fullstate_120f.c")),
                        "-o", str(library)], check=True)
        dll = ctypes.CDLL(str(library))
        model = dll.pr_u9e_entry9_roll_latch_0e2e
        model.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t,
                          ctypes.POINTER(Cpu), callback_type, ctypes.c_void_p]
        model.restype = ctypes.c_int
        helper = dll.pr_u9e_entry8_roll
        helper.argtypes = [ctypes.c_uint16, ctypes.c_uint16,
                           ctypes.POINTER(ctypes.c_uint32),
                           ctypes.POINTER(ctypes.c_uint8), ctypes.c_void_p]
        helper.restype = ctypes.c_int
        layouts = ((0x3000, 0xE000, 0x8000),
                   (0x3000, 0x3000, 0x7000),
                   (0xF123, 0xE000, 0x0002),
                   (0x3000, 0x3000, 0x6818),
                   (0x3000, 0x3000, 0x6816),
                   (0xE000, 0xE000, 0x0008))
        cases = [(count, sides, *layout)
                 for count in (0, 1, 3, 255)
                 for sides in (0, 1, 6, 255)
                 for layout in layouts]
        for label in cases:
            compare(label, model, helper, callback_type, body)
        for count in (0, 1, 3, 255):
            for sides in (0, 1, 6, 255):
                compare_composed(count, sides, body, callee, helper)
    print(f"u9e:+0e2e original-byte full-state parity: {len(cases)} cases")
    print("u9e:+0e2e original wrapper plus entry 8 AL/seed parity: 16 cases")


if __name__ == "__main__":
    main()
