#!/usr/bin/env python3
"""Original-byte/full-state parity for U25:+1CC3 under register-ABI SYSTEM hooks."""

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
EXE = ROOT / "resources/DOS/POOLRAD/START.EXE"
OVR_SHA = "872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614"
BODY_SHA = "cfffb7e933808123e58434886f10226994973bdb120d358f09cfcf97f2ea90ca"
UNIT = 0x74D3
START, END = 0x1CC3, 0x1DF0
SITES = {0x1CD0 + 0x2A * k + d: e for k in range(7)
         for d, e in ((0, 0), (0x0E, 1), (0x13, 2))}
CLEANUP = 0
TARGETS = ("0xaf8, 0x119a", "0xaf8, 0x1186", "0xaf8, 0x119e")
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
    assert len(body) == 301 and sha256(body).hexdigest() == BODY_SHA
    ins = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(body, START))
    assert len(ins) == 81 and sum(x.size for x in ins) == 301
    assert [x.address for x in ins if x.mnemonic.startswith("ret")] == [0x1DED]
    assert ins[-1].mnemonic == "retf" and ins[-1].op_str == "0xa"
    calls = {x.address: x.op_str for x in ins if "call" in x.mnemonic}
    assert calls == {site: TARGETS[event] for site, event in SITES.items()}
    assert not any(x.mnemonic.startswith("j") for x in ins)
    assert data[UNIT + START - 3:UNIT + START] == bytes.fromhex("ca0200")
    assert data[UNIT + END:UNIT + END + 3] == bytes.fromhex("5589e5")
    # The three helpers are register-only with a plain RETF (cleanup 0).
    exe = EXE.read_bytes()
    base = 0xB79B - 0x0B25
    for entry, size in ((0x1186, 6), (0x119A, 4), (0x119E, 8)):
        tail = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(
            exe[base + entry:base + entry + size], entry))[-1]
        assert (tail.mnemonic, tail.op_str) == ("retf", ""), hex(entry)
    return body, ins, sha256(body).hexdigest()


class Case:
    def __init__(self, words, factor, ss, sp, ah, alias, seed):
        self.words, self.factor, self.ss, self.sp = words, factor, ss, sp
        self.ah, self.alias, self.seed = ah, alias, seed

    def label(self):
        return (self.words[:2], self.factor, hex(self.ss), hex(self.sp),
                self.alias, self.seed)


REC_SEG = 0x4000


def make_case(body, case):
    memory = bytearray(RAM_SIZE)
    a = physical(CS, START)
    memory[a:a + len(body)] = body
    rng = random.Random(case.seed)
    for off in range(0, 0x40, 2):
        put16(memory, case.ss, case.sp - 0x30 + off, rng.randrange(0x10000))
    put16(memory, case.ss, case.sp, 0x1E24)
    put16(memory, case.ss, case.sp + 2, 0xD000)
    rec_seg, rec_off = REC_SEG, 0x0100
    if case.alias == "stack":
        # The record overlaps the caller frame: field +94 hits the factor.
        rec_seg, rec_off = case.ss, (case.sp + 0x0A - 0x94) & 0xFFFF
    put16(memory, case.ss, case.sp + 4, rec_off)
    put16(memory, case.ss, case.sp + 6, rec_seg)
    for i, w in enumerate(case.factor):
        put16(memory, case.ss, case.sp + 8 + 2 * i, w)
    for k, w in enumerate(case.words):
        put16(memory, rec_seg, rec_off + 0x88 + 2 * k, w)
    cpu = Cpu((case.ah << 8) | 0xCD, 0x3456, 0x5678, 0x789A, 0x9ABC,
              0xBCDE, 0x7777, case.sp, CS, START, DS, 0x1111,
              case.ss, 0x72D3 if case.seed % 2 else 0x0202)
    return memory, cpu


def snap(state):
    return tuple(getattr(state, name) for name, _ in Cpu._fields_)


def args(memory, state):
    return (state.ax, state.bx, state.cx, state.dx, state.si, state.di)


def apply_call(memory, state, count, case):
    """Synthetic register-ABI stand-ins (not Real48 arithmetic): the
    'real' lives in BX, the factor in CX; CX/SI/DI are clobbered as the
    real multiply does, so the caller's reloads are exercised."""
    del memory
    event = count % 3
    if event == 0:
        state.bx = (state.ax * 3 + state.dx + case.seed) & 0xFFFF
        state.ax = 0x1111
    elif event == 1:
        state.bx = (state.bx * (state.cx | 1) ^ state.si ^ state.di) & 0xFFFF
    else:
        state.ax = state.bx
        state.dx = (state.bx >> 15) * 0xFFFF
    state.cx, state.si, state.di = 0xDEAD, 0xBEEF, (0xF00D + count) & 0xFFFF
    state.flags = (0x0A13, 0x0457, 0x0896)[event]


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
        if off in SITES:
            state = Cpu(*(engine.reg_read(reg) & 0xFFFF for reg in REGISTERS))
            snapshot = bytearray(engine.mem_read(0, RAM_SIZE))
            calls.append((snap(state), args(snapshot, state)))
            apply_call(snapshot, state, len(calls) - 1, case)
            engine.mem_write(0, bytes(snapshot))
            for (name, _), reg in zip(Cpu._fields_, REGISTERS):
                engine.reg_write(reg, getattr(state, name))
            engine.reg_write(UC_X86_REG_SP, (state.sp + CLEANUP) & 0xFFFF)
            engine.reg_write(UC_X86_REG_IP, off + 5)
            last[0] = None

    emu.hook_add(UC_HOOK_CODE, on_code)
    emu.emu_start(start, RAM_SIZE, count=20000)
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
        assert ram_size == RAM_SIZE and event == len(actual_calls) % 3
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
    shapes = ([0] * 7, list(range(1, 8)), [0xFFFF, 0x8000, 0x7FFF, 1, 0, 2, 3])
    factors = ((0, 0, 0), (0x0081, 0, 0x4000), (0xFFFF, 0xFFFF, 0xFFFF))
    for words in shapes:
        for factor in factors:
            for ss, sp in ((0xE000, 0x8000), (0xE000, 0x0004), (0xE000, 0xFFF0)):
                out.append(Case(words, factor, ss, sp, 0xA5, None, len(out)))
    for ss, sp in ((0xE000, 0x8000), (0xE000, 0x0200)):
        out.append(Case(shapes[1], factors[1], ss, sp, 0x11, "stack", len(out)))
    rng = random.Random(0x1CC3)
    for _ in range(40):
        out.append(Case([rng.randrange(0x10000) for _ in range(7)],
                        tuple(rng.randrange(0x10000) for _ in range(3)),
                        rng.choice((0xE000, DS)), rng.choice((0x8000, 0x0300, 0xFFF8)),
                        rng.randrange(256), rng.choice((None, None, "stack")), len(out)))
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
    with tempfile.TemporaryDirectory(prefix="poolrad-u25-1cc3-") as tmp:
        library = Path(tmp) / "model.so"
        subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic",
                        "-Werror", "-shared", "-fPIC", "-I", str(ROOT / "include"),
                        str(ROOT / "src/recovered/overlay_u25_entry20_1cc3.c"),
                        "-o", str(library)], check=True)
        model = ctypes.CDLL(str(library)).pr_u25_entry20_1cc3
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
    print(f"u25:+1CC3 full-state original-byte parity ({digest[:16]}): "
          f"{len(all_cases)} cases, {len(visited & starts)}/{len(starts)} "
          f"instruction starts, {outcomes}/{2 * len(branches)} conditional outcomes")
    assert visited == starts and outcomes == 2 * len(branches)


if __name__ == "__main__":
    main()
