#!/usr/bin/env python3
"""Original-byte/full-state parity for U25:+0482 under a bounded callee."""

import ctypes
from hashlib import sha256
from itertools import product
from pathlib import Path
import random
import subprocess
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
BODY_SHA = "bcfe56989d4733d260e0f85a7146d4cb56de3041cbf20b7de27851f0f513df2b"
UNIT = 0x74D3
START, END = 0x0482, 0x0511
SITE, CLEANUP = 0x04C9, 6
RAM_SIZE = 1 << 20
REGISTERS = (UC_X86_REG_AX, UC_X86_REG_BX, UC_X86_REG_CX,
             UC_X86_REG_DX, UC_X86_REG_SI, UC_X86_REG_DI,
             UC_X86_REG_BP, UC_X86_REG_SP, UC_X86_REG_CS,
             UC_X86_REG_IP, UC_X86_REG_DS, UC_X86_REG_ES,
             UC_X86_REG_SS, UC_X86_REG_EFLAGS)
CS = 0xF000
DS = 0x3000


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


def source():
    data = OVR.read_bytes()
    assert len(data) == 206105 and sha256(data).hexdigest() == OVR_SHA
    body = data[UNIT + START:UNIT + END]
    assert len(body) == 143 and sha256(body).hexdigest() == BODY_SHA
    ins = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(body, START))
    assert len(ins) == 52 and sum(x.size for x in ins) == 143
    assert [x.address for x in ins if x.mnemonic.startswith("ret")] == [0x050E]
    assert ins[-1].mnemonic == "retf" and ins[-1].op_str == "6"
    assert [(x.address, x.op_str) for x in ins if "call" in x.mnemonic] == \
        [(SITE, "0x3d5, 0x2a")]
    for x in ins:
        if x.mnemonic.startswith("j"):
            assert START <= int(x.op_str, 16) < END, hex(x.address)
    assert data[UNIT + START - 3:UNIT + START] == bytes.fromhex("ca0e00")
    assert data[UNIT + END:UNIT + END + 3] == bytes.fromhex("5589e5")
    return body, ins, sha256(body).hexdigest()


class Case:
    def __init__(self, e6, dir_, y, x, results, touch, ah, ss, sp, seed):
        self.e6, self.dir, self.y, self.x = e6, dir_, y, x
        self.results, self.touch = results, touch
        self.ah, self.ss, self.sp, self.seed = ah, ss, sp, seed

    def label(self):
        return (hex(self.e6), self.dir, self.results, self.touch,
                hex(self.ss), hex(self.sp), self.seed)


def make_case(body, case):
    memory = bytearray(RAM_SIZE)
    a = physical(CS, START)
    memory[a:a + len(body)] = body
    rng = random.Random(case.seed)
    for off in range(0, 0x20, 2):
        put16(memory, case.ss, case.sp - 0x18 + off, rng.randrange(0x10000))
    put16(memory, case.ss, case.sp, 0x03FA)
    put16(memory, case.ss, case.sp + 2, 0xD000)
    put16(memory, case.ss, case.sp + 4, (rng.randrange(256) << 8) | case.dir)
    put16(memory, case.ss, case.sp + 6, (rng.randrange(256) << 8) | case.y)
    put16(memory, case.ss, case.sp + 8, (rng.randrange(256) << 8) | case.x)
    put16(memory, DS, 0x49D2, 0x0010)
    put16(memory, DS, 0x49D4, 0x5000)
    put16(memory, DS, 0x49D6, 0x0020)
    put16(memory, DS, 0x49D8, 0x5800)
    put16(memory, 0x5000, 0x0010 + 0x1CC, case.e6)
    put16(memory, 0x5800, 0x0020 + 0x582, 0xCAFE)
    cpu = Cpu((case.ah << 8) | 0xCD, 0x3456, 0x5678, 0x789A, 0x9ABC,
              0xBCDE, 0x7777, case.sp, CS, START, DS, 0x1111,
              case.ss, 0x72D3 if case.seed % 2 else 0x0202)
    return memory, cpu


def snap(state):
    return tuple(getattr(state, name) for name, _ in Cpu._fields_)


def args(memory, state):
    return tuple(get16(memory, state.ss, state.sp + 2 * i) for i in range(3))


def apply_call(memory, state, count, case):
    """Synthetic unit 3BD entry 2: a scripted AL with varied AH/flags."""
    result = case.results[count] if count < len(case.results) else 0
    state.ax = ((0x40 + count) << 8) | result
    state.flags = (0x0A53, 0x0457, 0x0896)[count % 3]
    state.dx = (state.dx + 0x1111) & 0xFFFF
    if case.touch == "args" and count == 0:
        # SS alias: rewrite the caller's direction byte for the next step.
        memory[physical(state.ss, state.bp + 6)] = (case.dir + 2) & 7
    elif case.touch == "count" and count == 0:
        memory[physical(state.ss, state.bp - 3)] = 1
    elif case.touch == "return":
        put16(memory, state.ss, state.bp, 0x3344)
        put16(memory, state.ss, state.bp + 2, 0x7788)
        put16(memory, state.ss, state.bp + 4, 0x99AA)


def run_original(memory, cpu, body, case):
    emu = Uc(UC_ARCH_X86, UC_MODE_16)
    emu.mem_map(0, RAM_SIZE)
    emu.mem_write(0, bytes(memory))
    for (name, _), reg in zip(Cpu._fields_, REGISTERS):
        emu.reg_write(reg, getattr(cpu, name))
    start = physical(CS, START)
    calls, visited, edges = [], set(), set()
    returned, last = [False], [None]

    def on_code(engine, address, size, user):
        del size, user
        if not start <= address < start + len(body):
            returned[0] = True
            engine.emu_stop()
            return
        off = address - (CS << 4)
        if last[0] is not None:
            edges.add((last[0], off))
        last[0] = off
        visited.add(off)
        if off == SITE:
            state = Cpu(*(engine.reg_read(reg) & 0xFFFF for reg in REGISTERS))
            snapshot = bytearray(engine.mem_read(0, RAM_SIZE))
            calls.append((snap(state), args(snapshot, state)))
            apply_call(snapshot, state, len(calls) - 1, case)
            engine.mem_write(0, bytes(snapshot))
            for (name, _), reg in zip(Cpu._fields_, REGISTERS):
                engine.reg_write(reg, getattr(state, name))
            engine.reg_write(UC_X86_REG_SP, (state.sp + CLEANUP) & 0xFFFF)
            engine.reg_write(UC_X86_REG_IP, SITE + 5)
            last[0] = None

    emu.hook_add(UC_HOOK_CODE, on_code)
    emu.emu_start(start, RAM_SIZE, count=2000)
    assert returned[0], case.label()
    state = tuple(emu.reg_read(reg) & 0xFFFF for reg in REGISTERS)
    return bytes(emu.mem_read(0, RAM_SIZE)), state, calls, visited, edges


def compare_case(body, case, model, callback_type):
    memory, cpu = make_case(body, case)
    expected_ram, expected_cpu, expected_calls, visited, edges = \
        run_original(memory, cpu, body, case)
    ram = (ctypes.c_uint8 * RAM_SIZE).from_buffer_copy(memory)
    actual_calls = []

    @callback_type
    def external(context, ram_pointer, ram_size, state_pointer, event):
        del context
        assert ram_size == RAM_SIZE and event == 0
        state = state_pointer.contents
        actual_calls.append((snap(state), args(ram_pointer, state)))
        apply_call(ram_pointer, state, len(actual_calls) - 1, case)
        return 1

    status = model(ram, RAM_SIZE, ctypes.byref(cpu), external, None)
    assert status == 0, (case.label(), status)
    if bytes(ram) != expected_ram:
        diff = [i for i, (a, b) in enumerate(zip(ram, expected_ram)) if a != b]
        raise AssertionError((case.label(), "RAM", [hex(i) for i in diff[:12]]))
    assert actual_calls == expected_calls, (case.label(), "calls")
    for (name, _), observed in zip(Cpu._fields_, expected_cpu):
        actual = getattr(cpu, name)
        if name == "flags":
            observed &= 0x0FD5
            actual &= 0x0FD5
        assert actual == observed, (case.label(), name, hex(actual), hex(observed))
    return visited, edges


def cases():
    out = []
    scripts = ((0, 0), (0, 1), (5,), (0x80,), (0, 0xFF))
    for e6, dir_, results in product((0, 1, 0x8000), (0, 2, 4, 6, 1, 7, 0xFF),
                                     scripts):
        out.append(Case(e6, dir_, 0x00, 0xFF, results, None, 0xA5,
                        0xE000, 0x8000, len(out)))
    for touch, (ss, sp) in product(("args", "count", "return"),
                                   ((0xE000, 0x8000), (DS, 0x49E0),
                                    (0xE000, 0x0004), (0xE000, 0xFFF0))):
        out.append(Case(1, 2, 0x10, 0x20, (0, 0), touch, 0x42, ss, sp, len(out)))
    rng = random.Random(0x0482)
    for _ in range(40):
        out.append(Case(rng.choice((0, rng.randrange(0x10000))),
                        rng.choice((0, 2, 4, 6, rng.randrange(256))),
                        rng.randrange(256), rng.randrange(256),
                        tuple(rng.choice((0, 0, rng.randrange(256)))
                              for _ in range(2)),
                        rng.choice((None, "args", "count")), rng.randrange(256),
                        rng.choice((0xE000, DS)),
                        rng.choice((0x8000, 0x0006, 0xFFF8)), len(out)))
    return out


def main():
    body, ins, digest = source()
    starts = {x.address for x in ins}
    branches = {x.address: (int(x.op_str, 16), x.address + x.size)
                for x in ins if x.mnemonic.startswith("j") and x.mnemonic != "jmp"}
    callback_type = ctypes.CFUNCTYPE(
        ctypes.c_int, ctypes.c_void_p, ctypes.POINTER(ctypes.c_uint8),
        ctypes.c_size_t, ctypes.POINTER(Cpu), ctypes.c_int)
    visited, edges = set(), set()
    with tempfile.TemporaryDirectory(prefix="poolrad-u25-0482-") as tmp:
        library = Path(tmp) / "model.so"
        subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic",
                        "-Werror", "-shared", "-fPIC", "-I", str(ROOT / "include"),
                        str(ROOT / "src/recovered/overlay_u25_step_probe_0482.c"),
                        "-o", str(library)], check=True)
        model = ctypes.CDLL(str(library)).pr_u25_step_probe_0482
        model.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t,
                          ctypes.POINTER(Cpu), callback_type, ctypes.c_void_p]
        model.restype = ctypes.c_int
        all_cases = cases()
        for case in all_cases:
            v, e = compare_case(body, case, model, callback_type)
            visited |= v
            edges |= e
    outcomes = sum((site, t) in edges for site, pair in branches.items()
                   for t in pair)
    print(f"u25:+0482 full-state original-byte parity ({digest[:16]}): "
          f"{len(all_cases)} cases, {len(visited & starts)}/{len(starts)} "
          f"instruction starts, {outcomes}/{2 * len(branches)} conditional outcomes")
    assert visited == starts and outcomes == 2 * len(branches)


if __name__ == "__main__":
    main()
