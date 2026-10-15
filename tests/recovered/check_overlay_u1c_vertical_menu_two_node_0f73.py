#!/usr/bin/env python3
"""Original-byte/full-state parity for U1C:+0F73 two-node branch."""

import ctypes
from pathlib import Path
import subprocess
import tempfile

from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_SP, UC_X86_REG_IP

import check_overlay_u1c_vertical_menu_prefix_0f08 as prefix
import check_overlay_u1c_vertical_menu_one_node_0f73 as one

ROOT = Path(__file__).resolve().parents[2]
RAM_SIZE = prefix.RAM_SIZE
SEQUENCE = (0, 1, 2, 2, 3, 4, 5, 6)


def apply_call(memory, state, event, selected):
    args = one.words(memory, state)
    if event == 0:
        assert args == ((state.bp - 0x106) & 0xFFFF, state.ss, 1)
        prefix.put16(memory, state.ss, state.bp - 0x106, 0x2200)
        prefix.put16(memory, state.ss, state.bp - 0x104, 0x4000)
        for node in (0x2200, 0x2300):
            for i in range(0x2E):
                memory[prefix.physical(0x4000, node + i)] = 0xA7
            for i in range(4):
                memory[prefix.physical(0x4000, node + 0x2A + i)] = 0
        prefix.put16(memory, 0x4000, 0x222A, 0x2300)
        prefix.put16(memory, 0x4000, 0x222C, 0x4000)
    elif event == 2:
        index = memory[prefix.physical(state.ss, state.bp - 2)]
        node = args[1]
        assert node in (0x2200, 0x2300)
        assert args == (0x28, node, 0x4000,
                        (0x6FC8 + (index << 8)) & 0xFFFF, state.ds), args
        length = memory[prefix.physical(state.ds, args[3])]
        for i in range(min(length, 40) + 1):
            memory[prefix.physical(0x4000, node + i)] = memory[
                prefix.physical(state.ds, args[3] + i)]
    elif event == 3:
        assert len(args) == 20
        assert args[:4] == ((state.bp - 0x106) & 0xFFFF, state.ss,
                            (state.bp - 0x10E) & 0xFFFF, state.ss)
        assert args[6:9] == (0, 0x2200, 0x4000)
        prefix.put16(memory, state.ss, state.bp - 0x10E, selected)
        memory[prefix.physical(state.ss, state.bp - 0x111)] = 0x42
    else:
        one.apply_call(memory, state, event, selected)
        return
    state.ax = (0xA120 + event) & 0xFFFF
    state.dx = (0xB230 + event) & 0xFFFF
    state.flags = 0x0247 if event & 1 else 0x0A93


def compare(label, model, first, callback_type, full):
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

    assert first(ram, RAM_SIZE, ctypes.byref(cpu), initial, None) == 0
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
        if off in one.CALLS:
            event, cleanup, next_ip, _ = one.CALLS[off]
            assert event == SEQUENCE[len(expected_calls)], label
            state = prefix.Cpu(*(emu.reg_read(reg) & 0xFFFF
                                 for reg in prefix.REGISTERS))
            snapshot = bytearray(emu.mem_read(0, RAM_SIZE))
            expected_calls.append((event, prefix.snap(state), one.words(snapshot, state)))
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
    assert reached[0] and len(expected_calls) == len(SEQUENCE), label
    expected_ram = bytes(original.mem_read(0, RAM_SIZE))
    actual_calls = []

    @callback_type
    def external(context, ptr, size, state_ptr, event):
        del context
        assert size == RAM_SIZE
        state = state_ptr.contents
        actual_calls.append((event, prefix.snap(state), one.words(ptr, state)))
        apply_call(ptr, state, event, selected)
        return 1

    assert model(ram, RAM_SIZE, ctypes.byref(boundary_cpu), external, None) == 0
    if bytes(ram) != expected_ram:
        diff = [i for i, (a, b) in enumerate(zip(ram, expected_ram)) if a != b]
        raise AssertionError((label, "RAM", [hex(i) for i in diff[:12]]))
    assert actual_calls == expected_calls, (label, "calls", actual_calls, expected_calls)
    for (name, _), reg in zip(prefix.Cpu._fields_, prefix.REGISTERS):
        actual, expected = getattr(boundary_cpu, name), original.reg_read(reg) & 0xFFFF
        assert actual == expected, (label, name, hex(actual), hex(expected))


def main():
    full = one.source()
    callback_type = ctypes.CFUNCTYPE(
        ctypes.c_int, ctypes.c_void_p, ctypes.POINTER(ctypes.c_uint8),
        ctypes.c_size_t, ctypes.POINTER(prefix.Cpu), ctypes.c_int)
    with tempfile.TemporaryDirectory(prefix="poolrad-u1c-0f73-two-") as tmp:
        library = Path(tmp) / "model.so"
        subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic",
                        "-Werror", "-shared", "-fPIC", "-I", str(ROOT / "include"),
                        str(ROOT / "src/recovered/overlay_u1c_vertical_menu_prefix_0f08.c"),
                        str(ROOT / "src/recovered/overlay_u1c_vertical_menu_one_node_0f73.c"),
                        str(ROOT / "src/recovered/overlay_u1c_vertical_menu_null_head_0f73.c"),
                        str(ROOT / "src/recovered/overlay_u1c_vertical_menu_two_node_0f73.c"),
                        "-o", str(library)], check=True)
        binary = ctypes.CDLL(str(library))
        first = binary.pr_u1c_vertical_menu_prefix_0f08
        model = binary.pr_u1c_vertical_menu_two_node_0f73
        for func in (first, model):
            func.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t,
                             ctypes.POINTER(prefix.Cpu), callback_type, ctypes.c_void_p]
            func.restype = ctypes.c_int
        cases = [(length, ds, ss, sp, selected)
                 for length in (0, 1, 40)
                 for ds, ss, sp in ((0x3000, 0xE000, 0x8000),
                                    (0x3000, 0x3000, 0x708A),
                                    (0xF123, 0xE000, 0x0002))
                 for selected in (0, 3)]
        for label in cases:
            compare(label, model, first, callback_type, full)
    print(f"u1c:+0f73 two-node original-byte parity: {len(cases)} cases")


if __name__ == "__main__":
    main()
