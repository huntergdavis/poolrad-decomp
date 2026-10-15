#!/usr/bin/env python3
"""Original-byte/full-state parity for one-node U1C:+0F73 continuation."""

import ctypes
from hashlib import sha256
from pathlib import Path
import subprocess
import tempfile

from capstone import Cs, CS_ARCH_X86, CS_MODE_16
from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_SP, UC_X86_REG_IP

import check_overlay_u1c_vertical_menu_prefix_0f08 as prefix

ROOT = Path(__file__).resolve().parents[2]
RAM_SIZE = prefix.RAM_SIZE
CALLS = {0x0F73: (0, 6, 0x0F78, 3), 0x0FAA: (1, 16, 0x0FAF, 8),
         0x0FD4: (2, 10, 0x0FD9, 5), 0x1053: (3, 40, 0x1058, 20),
         0x1060: (4, 4, 0x1065, 2), 0x106B: (5, 4, 0x1070, 2),
         0x107C: (6, 8, 0x1081, 4)}


def source():
    prefix.source()
    data = prefix.OVR.read_bytes()
    body = data[0x1CB9:0x1DCB]
    assert len(body) == 274
    assert sha256(body).hexdigest() == (
        "692863b3b57a534bd60b9920e560ab642b7fddf8cebcbf5b231259bdeea8ce6c")
    ins = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(body, 0x0F73))
    assert len(ins) == 110 and sum(x.size for x in ins) == len(body)
    assert {x.address for x in ins if x.mnemonic == "lcall"} == set(CALLS)
    assert ins[-1].address == 0x1084 and ins[-1].mnemonic == "retf"
    return data[0x1C4E:0x1DCB]


def words(memory, state):
    count = CALLS[state.ip][3]
    return tuple(prefix.get16(memory, state.ss, state.sp + 2 * i)
                 for i in range(count))


def apply_call(memory, state, event, selected):
    actual = words(memory, state)
    if event == 0:
        assert actual == ((state.bp - 0x106) & 0xFFFF, state.ss, 1)
        prefix.put16(memory, state.ss, state.bp - 0x106, 0x2200)
        prefix.put16(memory, state.ss, state.bp - 0x104, 0x4000)
        for i in range(0x2E):
            memory[prefix.physical(0x4000, 0x2200 + i)] = 0xA7
        for i in range(4):
            memory[prefix.physical(0x4000, 0x222A + i)] = 0
    elif event == 1:
        assert actual[0:2] == ((state.bp - 0x102) & 0xFFFF, state.ss)
        assert tuple(x & 255 for x in actual[2:]) == (1, 0xA, 0x16, 0x26, 0x11, 1)
    elif event == 2:
        index = memory[prefix.physical(state.ss, state.bp - 2)]
        assert actual == (0x28, 0x2200, 0x4000,
                          (0x6FC8 + (index << 8)) & 0xFFFF, state.ds), actual
        length = memory[prefix.physical(state.ds, actual[3])]
        for i in range(min(length, 40) + 1):
            memory[prefix.physical(0x4000, 0x2200 + i)] = memory[
                prefix.physical(state.ds, actual[3] + i)]
    elif event == 3:
        assert len(actual) == 20
        assert actual[0:2] == ((state.bp - 0x106) & 0xFFFF, state.ss)
        assert actual[2:4] == ((state.bp - 0x10E) & 0xFFFF, state.ss)
        assert actual[6:9] == (0, 0x2200, 0x4000)
        prefix.put16(memory, state.ss, state.bp - 0x10E, selected)
        memory[prefix.physical(state.ss, state.bp - 0x111)] = 0x42
    elif event == 4:
        assert actual[0] == selected
        assert actual[1] & 255 == 0x42
        prefix.put16(memory, state.ds, 0x8460, actual[0])
    elif event == 5:
        assert actual == ((state.bp - 0x106) & 0xFFFF, state.ss)
        prefix.put16(memory, state.ss, state.bp - 0x106, 0)
        prefix.put16(memory, state.ss, state.bp - 0x104, 0)
    else:
        assert event == 6 and tuple(x & 255 for x in actual) == (0x16, 0x26, 0x11, 1)
    state.ax = (0xA120 + event) & 0xFFFF
    state.dx = (0xB230 + event) & 0xFFFF
    state.flags = 0x0247 if event & 1 else 0x0A93


def compare(label, model, prefix_model, callback_type, full):
    length, ds, ss, sp, selected = label
    memory, cpu, _, n, touch = prefix.make_case(
        full, length, ds, ss, sp, 0x8000, 1, False)
    ram = (ctypes.c_uint8 * RAM_SIZE).from_buffer_copy(memory)

    @callback_type
    def initial(context, ptr, size, state_ptr, event):
        del context
        assert size == RAM_SIZE
        prefix.apply_call(ptr, state_ptr.contents, event, length, n, touch)
        return 1

    assert prefix_model(ram, RAM_SIZE, ctypes.byref(cpu), initial, None) == 0
    assert cpu.ip == 0x0F73
    boundary = bytes(ram)
    boundary_cpu = prefix.Cpu(*prefix.snap(cpu))

    original = Uc(UC_ARCH_X86, UC_MODE_16)
    original.mem_map(0, RAM_SIZE)
    original.mem_write(0, boundary)
    for (name, _), reg in zip(prefix.Cpu._fields_, prefix.REGISTERS):
        original.reg_write(reg, getattr(cpu, name))
    expected_calls = []
    reached = [False]

    def on_code(emu, address, size, user):
        del size, user
        off = address - 0xF0000
        if off in CALLS:
            event, cleanup, next_ip, _ = CALLS[off]
            state = prefix.Cpu(*(emu.reg_read(reg) & 0xFFFF
                                 for reg in prefix.REGISTERS))
            snapshot = bytearray(emu.mem_read(0, RAM_SIZE))
            expected_calls.append((event, prefix.snap(state), words(snapshot, state)))
            apply_call(snapshot, state, event, selected)
            emu.mem_write(0, bytes(snapshot))
            for (name, _), reg in zip(prefix.Cpu._fields_, prefix.REGISTERS):
                emu.reg_write(reg, getattr(state, name))
            emu.reg_write(UC_X86_REG_SP, (state.sp + cleanup) & 0xFFFF)
            emu.reg_write(UC_X86_REG_IP, next_ip)
        elif address == prefix.physical(0xD000, 0x0712):
            reached[0] = True
            emu.emu_stop()
        else:
            assert 0xF0F73 <= address <= 0xF1084, (label, hex(address))

    original.hook_add(UC_HOOK_CODE, on_code)
    original.emu_start(0xF0F73, RAM_SIZE, count=160)
    assert reached[0] and len(expected_calls) == 7, label
    expected_ram = bytes(original.mem_read(0, RAM_SIZE))
    actual_calls = []

    @callback_type
    def external(context, ptr, size, state_ptr, event):
        del context
        assert size == RAM_SIZE
        state = state_ptr.contents
        actual_calls.append((event, prefix.snap(state), words(ptr, state)))
        apply_call(ptr, state, event, selected)
        return 1

    status = model(ram, RAM_SIZE, ctypes.byref(boundary_cpu), external, None)
    assert status == 0, (label, status)
    if bytes(ram) != expected_ram:
        diff = [i for i, (a, b) in enumerate(zip(ram, expected_ram)) if a != b]
        raise AssertionError((label, "RAM", [hex(i) for i in diff[:12]]))
    assert actual_calls == expected_calls, (label, "calls", actual_calls, expected_calls)
    for (name, _), reg in zip(prefix.Cpu._fields_, prefix.REGISTERS):
        actual, expected = getattr(boundary_cpu, name), original.reg_read(reg) & 0xFFFF
        assert actual == expected, (label, name, hex(actual), hex(expected))


def main():
    full = source()
    callback_type = ctypes.CFUNCTYPE(
        ctypes.c_int, ctypes.c_void_p, ctypes.POINTER(ctypes.c_uint8),
        ctypes.c_size_t, ctypes.POINTER(prefix.Cpu), ctypes.c_int)
    with tempfile.TemporaryDirectory(prefix="poolrad-u1c-0f73-") as tmp:
        library = Path(tmp) / "model.so"
        subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic",
                        "-Werror", "-shared", "-fPIC", "-I", str(ROOT / "include"),
                        str(ROOT / "src/recovered/overlay_u1c_vertical_menu_prefix_0f08.c"),
                        str(ROOT / "src/recovered/overlay_u1c_vertical_menu_one_node_0f73.c"),
                        "-o", str(library)], check=True)
        binary = ctypes.CDLL(str(library))
        first = binary.pr_u1c_vertical_menu_prefix_0f08
        second = binary.pr_u1c_vertical_menu_one_node_0f73
        for model in (first, second):
            model.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t,
                              ctypes.POINTER(prefix.Cpu), callback_type, ctypes.c_void_p]
            model.restype = ctypes.c_int
        cases = [(length, ds, ss, sp, selected)
                 for length in (0, 1, 40)
                 for ds, ss, sp in ((0x3000, 0xE000, 0x8000),
                                    (0x3000, 0x3000, 0x708A),
                                    (0xF123, 0xE000, 0x0002))
                 for selected in (0, 3)]
        for label in cases:
            compare(label, second, first, callback_type, full)
    print(f"u1c:+0f73 one-node original-byte parity: {len(cases)} cases")


if __name__ == "__main__":
    main()
