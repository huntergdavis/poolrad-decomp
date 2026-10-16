#!/usr/bin/env python3
"""Original-byte/full-state parity for U25:+1FB1 under a bounded resident callee."""

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
BODY_SHA = "8d6da69d20a93bea32da791cf65623426c7cd35910e9c9118158c564110882b3"
UNIT = 0x74D3
START, END = 0x1FB1, 0x207E
SITES, CLEANUP = (0x2004, 0x2027), 0x0A
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
    assert len(body) == 205 and sha256(body).hexdigest() == BODY_SHA
    ins = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(body, START))
    assert len(ins) == 74 and sum(x.size for x in ins) == 205
    assert [x.address for x in ins if x.mnemonic.startswith("ret")] == [0x207B]
    assert ins[-1].mnemonic == "retf" and ins[-1].op_str == "8"
    assert [(x.address, x.op_str) for x in ins if "call" in x.mnemonic] == \
        [(SITES[0], "0xba, 0x2199"), (SITES[1], "0xba, 0x2199")]
    for x in ins:
        if x.mnemonic.startswith("j"):
            assert START <= int(x.op_str, 16) < END, hex(x.address)
    # +207E..+20CB is +20CC's literal data: only +20CC's MOV DI loads it.
    unit = data[UNIT:UNIT + 0x221A]
    for target in (0x207E, 0x2081, 0x2089, 0x2096, 0x20A9):
        pattern = bytes([0xBF]) + target.to_bytes(2, "little")
        assert 0x20CC <= unit.index(pattern) < 0x2215, hex(target)
    assert data[UNIT + 0x20CC:UNIT + 0x20CF] == bytes.fromhex("5589e5")
    return body, ins, sha256(body).hexdigest()


class Case:
    def __init__(self, nodes, ss, sp, ah, alias, seed):
        # nodes: list of (value, keys) with keys a tuple of sidecar bytes
        self.nodes, self.ss, self.sp, self.ah = nodes, ss, sp, ah
        self.alias, self.seed = alias, seed

    def label(self):
        return (len(self.nodes), hex(self.ss), hex(self.sp), self.alias, self.seed)


NODE_SEG, SIDE_SEG, OUT_SEG = 0x4000, 0x4800, 0x2000


def make_case(body, case):
    memory = bytearray(RAM_SIZE)
    a = physical(CS, START)
    memory[a:a + len(body)] = body
    rng = random.Random(case.seed)
    for off in range(0, 0x40, 2):
        put16(memory, case.ss, case.sp - 0x30 + off, rng.randrange(0x10000))
    put16(memory, case.ss, case.sp, 0x1F1F)
    put16(memory, case.ss, case.sp + 2, 0xD000)
    put16(memory, case.ss, case.sp + 4, 0x0010)
    put16(memory, case.ss, case.sp + 6, OUT_SEG)
    out_max = 0x0010 if case.alias == "minmax" else 0x0020
    put16(memory, case.ss, case.sp + 8, out_max)
    put16(memory, case.ss, case.sp + 10, OUT_SEG)
    side = 0x0010
    for i, (value, keys) in enumerate(case.nodes):
        node = 0x0200 * (i + 1)
        memory[physical(NODE_SEG, node + 0x11C)] = value
        nxt = 0x0200 * (i + 2) if i + 1 < len(case.nodes) else 0
        put16(memory, NODE_SEG, node + 0x104, nxt)
        put16(memory, NODE_SEG, node + 0x106, NODE_SEG if nxt else 0)
        head = (0, 0)
        for key in reversed(keys):
            memory[physical(SIDE_SEG, side)] = key
            put16(memory, SIDE_SEG, side + 5, head[0])
            put16(memory, SIDE_SEG, side + 7, head[1])
            head = (side, SIDE_SEG)
            side += 9
        put16(memory, NODE_SEG, node + 0x7F, head[0])
        put16(memory, NODE_SEG, node + 0x81, head[1])
    head = (0x0200, NODE_SEG) if case.nodes else (0, 0)
    put16(memory, DS, 0x5D96, head[0])
    put16(memory, DS, 0x5D98, head[1])
    memory[physical(0, 0x011C)] = rng.randrange(256)
    if case.alias == "value" and case.nodes:
        # The min output aliases the head node's value byte.
        put16(memory, case.ss, case.sp + 4, 0x0200 + 0x11C)
        put16(memory, case.ss, case.sp + 6, NODE_SEG)
    cpu = Cpu((case.ah << 8) | 0xCD, 0x3456, 0x5678, 0x789A, 0x9ABC,
              0xBCDE, 0x7777, case.sp, CS, START, DS, 0x1111,
              case.ss, 0x72D3 if case.seed % 2 else 0x0202)
    return memory, cpu


def snap(state):
    return tuple(getattr(state, name) for name, _ in Cpu._fields_)


def args(memory, state):
    return tuple(get16(memory, state.ss, state.sp + 2 * i) for i in range(5))


def apply_call(memory, state, count, case):
    """BA:2199 as documented in RESIDENT_SIDECAR_LOOKUP.md: walk the
    character's +7F/+81 sidecar list for the key's low byte."""
    cur_off, cur_seg, key, char_off, char_seg = args(memory, state)
    del count
    found = 0
    put16(memory, cur_seg, cur_off, get16(memory, char_seg, char_off + 0x7F))
    put16(memory, cur_seg, cur_off + 2, get16(memory, char_seg, char_off + 0x81))
    for _ in range(64):
        node, seg = get16(memory, cur_seg, cur_off), get16(memory, cur_seg, cur_off + 2)
        if (node == 0 and seg == 0) or found:
            break
        if memory[physical(seg, node)] == key & 255:
            found = 1
        else:
            put16(memory, cur_seg, cur_off, get16(memory, seg, node + 5))
            put16(memory, cur_seg, cur_off + 2, get16(memory, seg, node + 7))
    state.ax = (state.ax & 0xFF00) | found
    state.bx = (state.bx + 0x0101) & 0xFFFF
    state.flags = (0x0A53, 0x0457, 0x0896)[case.seed % 3]


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
    shapes = ([], [(0x40, ())], [(0x90, (0x27,)), (0x10, (0x2A,)), (0x40, ())],
              [(0x80, (0x2A, 0x27)), (0x01, (0x05, 0x2A)), (0xFF, (0x27,))],
              [(v, ()) for v in (5, 250, 7, 0, 128)])
    for nodes in shapes:
        for ss, sp in ((0xE000, 0x8000), (DS, 0x5DA0), (0xE000, 0x0010), (0xE000, 0xFFF0)):
            out.append(Case(nodes, ss, sp, 0xA5, None, len(out)))
    for alias in ("minmax", "value"):
        out.append(Case(shapes[2], 0xE000, 0x8000, 0x11, alias, len(out)))
    rng = random.Random(0x1FB1)
    for _ in range(40):
        nodes = [(rng.randrange(256),
                  tuple(rng.choice((0x27, 0x2A, 0x05, 0x27)) for _ in range(rng.randrange(0, 4))))
                 for _ in range(rng.randrange(0, 8))]
        out.append(Case(nodes, rng.choice((0xE000, 0x3000)),
                        rng.choice((0x8000, 0x0300, 0xFFF8)), rng.randrange(256),
                        rng.choice((None, None, "minmax")), len(out)))
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
    with tempfile.TemporaryDirectory(prefix="poolrad-u25-1fb1-") as tmp:
        library = Path(tmp) / "model.so"
        subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic",
                        "-Werror", "-shared", "-fPIC", "-I", str(ROOT / "include"),
                        str(ROOT / "src/recovered/overlay_u25_entry23_1fb1.c"),
                        "-o", str(library)], check=True)
        model = ctypes.CDLL(str(library)).pr_u25_entry23_1fb1
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
    print(f"u25:+1FB1 full-state original-byte parity ({digest[:16]}): "
          f"{len(all_cases)} cases, {len(visited & starts)}/{len(starts)} "
          f"instruction starts, {outcomes}/{2 * len(branches)} conditional outcomes")
    assert visited == starts and outcomes == 2 * len(branches)


if __name__ == "__main__":
    main()
