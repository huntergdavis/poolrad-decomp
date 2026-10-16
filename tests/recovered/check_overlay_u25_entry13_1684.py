#!/usr/bin/env python3
"""Original-byte/full-state parity for U25:+1684 composed with +156D."""

import ctypes
from hashlib import sha256
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
BODY_SHA = "1ba4ee097c797c4490a51228436ff10b3f389406b396b5fc06c066d5c67dd225"
NEAR_SHA = "eb9f88fcfcdfff0e1b561428c0dedaa730b51b400a64e2aa66d8954e9d8a21b8"
STRSTORE_SHA = "009955722e8f73d0a0f7076aa418d141f642c5a729b2a58dfa7a7a3922989118"
UNIT = 0x74D3
BODY = (0x1684, 0x1788)
NEAR = (0x156D, 0x1664)
SET = (0x154D, 0x156D)
OWN_SET = (0x1664, 0x1684)
SYSTEM_FILE = 0xB79B - 0x0B25   # START.EXE file offset of SYSTEM AF8:0000
RAM_SIZE = 1 << 20
OUTER_STORE, U3B0, U77, LOAD_PIECES, OUTER_SET = range(5)
IN_SET, DELETE, UPCASE, STRLOAD, CHARSTR, CONCAT, STRSTORE = range(5, 12)
CLEANUP = (10, 0x16, 2, 4, 6, 6, 8, 2, 4, 2, 4, 10)
ARG_WORDS = (5, 11, 1, 2, 3, 3, 4, 1, 4, 3, 4, 5)
TARGET = {0x0DC0: IN_SET, 0x0CBC: DELETE, 0x182A: UPCASE, 0x0B0B: STRLOAD,
          0x0C3B: CHARSTR, 0x0B98: CONCAT, 0x0B25: STRSTORE}
SYSTEM_RETURNS = {0x0B0B: (0x0B22, "retf", "4"), 0x0B25: (0x0B46, "retf", "0xa"),
                  0x0B98: (0x0BC1, "retf", "4"), 0x0C3B: (0x0C4A, "retf", "2"),
                  0x0DC0: (0x0DDE, "retf", "6"), 0x0CBC: (0x0D0F, "retf", "8"),
                  0x182A: (0x183A, "retf", "2")}
REGISTERS = (UC_X86_REG_AX, UC_X86_REG_BX, UC_X86_REG_CX,
             UC_X86_REG_DX, UC_X86_REG_SI, UC_X86_REG_DI,
             UC_X86_REG_BP, UC_X86_REG_SP, UC_X86_REG_CS,
             UC_X86_REG_IP, UC_X86_REG_DS, UC_X86_REG_ES,
             UC_X86_REG_SS, UC_X86_REG_EFLAGS)
CS = 0xF000
SRC = (0x2000, 0x0100)
DST = (0x2100, 0x0040)


class Cpu(ctypes.Structure):
    _fields_ = [(name, ctypes.c_uint16) for name in
                "ax bx cx dx si di bp sp cs ip ds es ss flags".split()]


def physical(seg, off):
    return ((seg << 4) + (off & 0xFFFF)) & (RAM_SIZE - 1)


class Bytes:
    """Byte access over a bytearray, ctypes buffer or Unicorn engine."""

    def __init__(self, target):
        self.target = target

    def get(self, a):
        if isinstance(self.target, Uc):
            return self.target.mem_read(a, 1)[0]
        return self.target[a]

    def set(self, a, value):
        if isinstance(self.target, Uc):
            self.target.mem_write(a, bytes([value & 255]))
        else:
            self.target[a] = value & 255

    def get8(self, seg, off):
        return self.get(physical(seg, off))

    def set8(self, seg, off, value):
        self.set(physical(seg, off), value)

    def get16(self, seg, off):
        a = physical(seg, off)
        return self.get(a) | self.get((a + 1) & (RAM_SIZE - 1)) << 8

    def set16(self, seg, off, value):
        a = physical(seg, off)
        self.set(a, value)
        self.set((a + 1) & (RAM_SIZE - 1), value >> 8)


def instructions(data, span):
    body = data[UNIT + span[0]:UNIT + span[1]]
    return body, list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(body, span[0]))


def source():
    data = OVR.read_bytes()
    assert len(data) == 206105 and sha256(data).hexdigest() == OVR_SHA
    body, ins = instructions(data, BODY)
    assert len(body) == 260 and sha256(body).hexdigest() == BODY_SHA
    assert len(ins) == 100 and sum(x.size for x in ins) == 260
    assert [x.address for x in ins if x.mnemonic.startswith("ret")] == [0x1785]
    assert ins[-1].mnemonic == "retf" and ins[-1].op_str == "0x12"
    assert [(x.address, x.op_str) for x in ins if x.mnemonic == "call"] == \
        [(0x16BF, "0x156d")]
    assert data[UNIT + BODY[1]:UNIT + BODY[1] + 3] == bytes.fromhex("5589e5")
    outer = {(0xAF8, 0x0B25): OUTER_STORE, (0x3C5, 0x002A): U3B0,
             (0x85, 0x0052): U77, (0xBA, 0x0767): LOAD_PIECES,
             (0xAF8, 0x0DC0): OUTER_SET}
    sites = {}
    for x in ins:
        if x.mnemonic == "lcall":
            target = tuple(int(v, 16) for v in x.op_str.split(", "))
            assert target in outer, hex(x.address)
            sites[x.address] = outer[target]
        if x.mnemonic.startswith("j"):
            assert BODY[0] <= int(x.op_str, 16) < BODY[1], hex(x.address)
    assert sites == {0x1699: OUTER_STORE, 0x16AD: OUTER_STORE, 0x16E6: U3B0,
                     0x16FB: U77, 0x1708: LOAD_PIECES, 0x171C: OUTER_SET}
    unit = data[UNIT:UNIT + 0x221A]
    assert unit.count(bytes.fromhex("bf6416")) == 1 and unit.index(bytes.fromhex("bf6416")) == 0x1717
    near, nins = instructions(data, NEAR)
    assert len(near) == 247 and sha256(near).hexdigest() == NEAR_SHA
    assert len(nins) == 98 and nins[-1].op_str == "8"
    for x in nins:
        if x.mnemonic == "lcall":
            seg, off = (int(v, 16) for v in x.op_str.split(", "))
            assert seg == 0xAF8 and off in TARGET, hex(x.address)
            sites[x.address] = TARGET[off]
    assert len(sites) == 13
    exe = EXE.read_bytes()
    assert sha256(exe[0xB79B:0xB7BF]).hexdigest() == STRSTORE_SHA
    for off, (ret, mnemonic, operand) in SYSTEM_RETURNS.items():
        tail = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(
            exe[SYSTEM_FILE + ret:SYSTEM_FILE + ret + 3], ret))[0]
        assert (tail.mnemonic, tail.op_str) == (mnemonic, operand), hex(off)
    return body, ins, (near, data[UNIT + SET[0]:UNIT + SET[1]],
                       data[UNIT + OWN_SET[0]:UNIT + OWN_SET[1]]), nins, sites


def args(mem, state, event):
    return tuple(mem.get16(state.ss, state.sp + 2 * i)
                 for i in range(ARG_WORDS[event]))


def copy_pascal(mem, dseg, doff, sseg, soff, cap):
    n = min(mem.get8(sseg, soff), cap)
    text = [mem.get8(sseg, soff + 1 + i) for i in range(n)]
    mem.set8(dseg, doff, n)
    for i, ch in enumerate(text):
        mem.set8(dseg, doff + 1 + i, ch)


def apply_call(mem, state, event, case):
    """The seven SYSTEM routines, as decoded from START.EXE."""
    w = args(mem, state, event)
    if event == U3B0:
        n = case.key_index
        case.key_index += 1
        key, flag = case.script[n] if n < len(case.script) else (case.final, 0)
        mem.set8(w[1], w[0], flag)
        state.ax = 0x6C00 | key
        state.flags = (0x0457, 0x0A53)[case.seed & 1]
        return
    if event in (U77, LOAD_PIECES):
        state.ax = (state.ax ^ 0x1357) & 0xFFFF
        state.flags = 0x0896
        if case.touch and event == LOAD_PIECES:
            for off, value in zip((state.bp, state.bp + 2, state.bp + 4),
                                  (0x3344, 0x7788, 0x99AA)):
                mem.set16(state.ss, off, value)
        return
    if event == OUTER_SET:
        event = IN_SET
    if event in (OUTER_STORE, STRSTORE):
        copy_pascal(mem, w[2], w[1], w[4], w[3], w[0] & 255)
    elif event == IN_SET:
        ch = w[2] & 255
        bits = mem.get8(w[1], w[0] + (ch >> 3))
        member = (bits >> (ch & 7)) & 1
        state.flags = (state.flags & ~0x0040) | (0 if member else 0x0040)
    elif event == DELETE:
        count, index, off, seg = w
        length = mem.get8(seg, off)
        if count and 1 <= index <= length:
            for i in range(index, length):
                mem.set8(seg, off + i, mem.get8(seg, off + i + 1))
            mem.set8(seg, off, length - 1)
    elif event == UPCASE:
        ch = w[0] & 255
        state.ax = (state.ax & 0xFF00) | (ch - 0x20 if 0x61 <= ch <= 0x7A else ch)
    elif event == STRLOAD:
        copy_pascal(mem, w[3], w[2], w[1], w[0], 255)
    elif event == CHARSTR:
        mem.set8(w[2], w[1], 1)
        mem.set8(w[2], w[1] + 1, w[0])
    elif event == CONCAT:
        dl = mem.get8(w[3], w[2])
        sl = mem.get8(w[1], w[0])
        take = min(sl, 255 - dl)
        text = [mem.get8(w[1], w[0] + 1 + i) for i in range(take)]
        for i, ch in enumerate(text):
            mem.set8(w[3], w[2] + 1 + dl + i, ch)
        mem.set8(w[3], w[2], dl + take)
    state.cx = (state.cx & 0xFF00) | ((case.seed * 7 + event) & 0xFF)
    state.bx = (state.bx + 0x0101 * (event + 1)) & 0xFFFF
    if event != IN_SET:
        state.flags = (0x0A53, 0x0457, 0x0896)[event % 3]


class Case:
    def __init__(self, text, script, final, enter, set_kind, touch, ah, ss, sp, seed):
        self.text, self.script, self.final, self.enter = text, script, final, enter
        self.set_kind, self.touch = set_kind, touch
        self.ah, self.ss, self.sp, self.seed = ah, ss, sp, seed
        self.key_index = 0
        self.stale = True

    def label(self):
        return (self.text[:16], self.script[:4], self.final, self.enter,
                self.set_kind, self.touch, hex(self.ss), hex(self.sp), self.seed)


def make_case(body, near_sets, case):
    near, original_set, own_set = near_sets
    case.key_index = 0
    memory = bytearray(RAM_SIZE)
    mem = Bytes(memory)
    ds = 0x3000
    for span, code in ((BODY, body), (NEAR, near)):
        a = physical(CS, span[0])
        memory[a:a + len(code)] = code
    upper = bytes(sum(1 << b for b in range(8) if 0x41 <= 8 * i + b <= 0x5A)
                  for i in range(32))
    table = original_set if case.set_kind == "original" else upper
    a = physical(CS, SET[0])
    memory[a:a + 32] = table
    a = physical(CS, OWN_SET[0])
    # The game table at +1664 is only pinned, not used, so every scripted
    # input loop is guaranteed to end on an upper-case key.
    del own_set
    memory[a:a + 32] = upper
    rng = random.Random(case.seed)
    for seg, off, text in ((0x2000, 0x0200, b"Title"), (0x2000, 0x0300, case.text)):
        for i in range(256):
            mem.set8(seg, off + i, rng.randrange(256))
        mem.set8(seg, off, len(text))
        for i, ch in enumerate(text):
            mem.set8(seg, off + 1 + i, ch)
    for off in range(0, 0x400, 2):
        mem.set16(case.ss, case.sp - 0x3C0 + off, rng.randrange(0x10000))
    mem.set16(case.ss, case.sp, 0x1236)
    mem.set16(case.ss, case.sp + 2, 0xD000)
    for i in range(5):
        mem.set16(case.ss, case.sp + 4 + 2 * i, rng.randrange(0x10000))
    mem.set8(case.ss, case.sp + 6, case.enter)
    mem.set16(case.ss, case.sp + 14, 0x0300)
    mem.set16(case.ss, case.sp + 16, 0x2000)
    mem.set16(case.ss, case.sp + 18, 0x0200)
    mem.set16(case.ss, case.sp + 20, 0x2000)
    mem.set16(ds, 0x5D92, 0x4321)
    mem.set16(ds, 0x5D94, 0x8765)
    cpu = Cpu((case.ah << 8) | 0xCD, 0x3456, 0x5678, 0x789A, 0x9ABC,
              0xBCDE, 0x7777, case.sp, CS, BODY[0], ds, 0x1111,
              case.ss, 0x72D3 if case.seed % 2 else 0x0202)
    return memory, cpu


def snap(state):
    return tuple(getattr(state, name) for name, _ in Cpu._fields_)


def run_original(memory, cpu, sites, case):
    emu = Uc(UC_ARCH_X86, UC_MODE_16)
    emu.mem_map(0, RAM_SIZE)
    emu.mem_write(0, bytes(memory))
    for (name, _), reg in zip(Cpu._fields_, REGISTERS):
        emu.reg_write(reg, getattr(cpu, name))
    mem = Bytes(emu)
    spans = [(physical(CS, a), physical(CS, b)) for a, b in (BODY, NEAR)]
    calls, visited, edges = [], set(), set()
    returned, last = [False], [None]

    def on_code(engine, address, size, user):
        del size, user
        if not any(a <= address < b for a, b in spans):
            returned[0] = True
            engine.emu_stop()
            return
        off = address - (CS << 4)
        if last[0] is not None:
            edges.add((last[0], off))
        last[0] = off
        visited.add(off)
        if off in sites:
            event = sites[off]
            state = Cpu(*(engine.reg_read(reg) & 0xFFFF for reg in REGISTERS))
            calls.append((event, snap(state), args(mem, state, event)))
            apply_call(mem, state, event, case)
            for (name, _), reg in zip(Cpu._fields_, REGISTERS):
                engine.reg_write(reg, getattr(state, name))
            engine.reg_write(UC_X86_REG_SP, (state.sp + CLEANUP[event]) & 0xFFFF)
            engine.reg_write(UC_X86_REG_IP, off + 5)
            last[0] = None

    emu.hook_add(UC_HOOK_CODE, on_code)
    emu.emu_start(physical(CS, BODY[0]), RAM_SIZE, count=100000)
    assert returned[0], case.label()
    state = tuple(emu.reg_read(reg) & 0xFFFF for reg in REGISTERS)
    return bytes(emu.mem_read(0, RAM_SIZE)), state, calls, visited, edges


def compare_case(body, near_and_set, sites, case, model, callback_type):
    memory, cpu = make_case(body, near_and_set, case)
    case.key_index = 0
    expected_ram, expected_cpu, expected_calls, visited, edges = \
        run_original(memory, cpu, sites, case)
    ram = (ctypes.c_uint8 * RAM_SIZE).from_buffer_copy(memory)
    case.key_index = 0
    actual_calls = []

    @callback_type
    def external(context, ram_pointer, ram_size, state_pointer, event):
        del context
        assert ram_size == RAM_SIZE
        state = state_pointer.contents
        mem = Bytes(ram_pointer)
        actual_calls.append((event, snap(state), args(mem, state, event)))
        apply_call(mem, state, event, case)
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
    texts = (b"~Yes ~No", b"~a~b~c", b"", b"plain", b"~~x")
    scripts = (((ord("N"), 0),), ((ord("x"), 0), (ord("Y"), 0)),
               ((0x0D, 0),), ((0x0D, 0), (ord("Q"), 0)),
               ((ord("N"), 1), (ord("Y"), 0)), ((0, 0), (ord("B"), 0)))
    for text in texts:
        for script in scripts:
            for enter in (0, 1):
                out.append(Case(text, script, ord("Y"), enter, "upper", None,
                                0xA5, 0xE000, 0x8000, len(out)))
    for ss, sp in ((0xE000, 0x0600), (0x2000, 0x0700), (0xE000, 0xFFC0)):
        out.append(Case(b"~Yes ~No", ((ord("N"), 1),), ord("N"), 0, "upper",
                        None, 0x11, ss, sp, len(out)))
    out.append(Case(b"~Yes ~No", ((ord("N"), 1),), ord("N"), 0, "upper",
                    "return", 0x11, 0xE000, 0x8000, len(out)))
    rng = random.Random(0x1684)
    for _ in range(30):
        script = tuple((rng.choice((0x0D, 0, ord("x"), ord("Y"), ord("N"), ord("Q"))),
                        rng.choice((0, 0, 1))) for _ in range(rng.randrange(0, 4)))
        out.append(Case(bytes(rng.choice(b"~~aYNq ") for _ in range(rng.randrange(0, 30))),
                        script, ord("Y"), rng.randrange(2),
                        rng.choice(("original", "upper")), None, rng.randrange(256),
                        rng.choice((0xE000, 0x3000)), rng.choice((0x8000, 0x0600)),
                        len(out)))
    return out


def main():
    body, ins, near_and_set, cins, sites = source()
    starts = {x.address for x in ins + cins}
    branches = {x.address: (int(x.op_str, 16), x.address + x.size)
                for x in ins + cins
                if x.mnemonic.startswith("j") and x.mnemonic != "jmp"}
    callback_type = ctypes.CFUNCTYPE(
        ctypes.c_int, ctypes.c_void_p, ctypes.POINTER(ctypes.c_uint8),
        ctypes.c_size_t, ctypes.POINTER(Cpu), ctypes.c_uint)
    visited, edges = set(), set()
    with tempfile.TemporaryDirectory(prefix="poolrad-u25-1684-") as tmp:
        library = Path(tmp) / "model.so"
        subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic",
                        "-Werror", "-shared", "-fPIC", "-I", str(ROOT / "include"),
                        str(ROOT / "src/recovered/overlay_u25_entry13_1684.c"),
                        str(ROOT / "src/recovered/overlay_u25_tilde_caps_156d.c"),
                        "-o", str(library)], check=True)
        model = ctypes.CDLL(str(library)).pr_u25_entry13_1684
        model.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t,
                          ctypes.POINTER(Cpu), callback_type, ctypes.c_void_p]
        model.restype = ctypes.c_int
        all_cases = cases()
        for case in all_cases:
            v, e = compare_case(body, near_and_set, sites, case, model, callback_type)
            visited |= v
            edges |= e
    outcomes = sum((site, t) in edges for site, pair in branches.items()
                   for t in pair)
    print(f"u25:+1684 (+156D) full-state original-byte parity: {len(all_cases)} cases, "
          f"{len(visited & starts)}/{len(starts)} instruction starts, "
          f"{outcomes}/{2 * len(branches)} conditional outcomes")
    assert visited == starts and outcomes == 2 * len(branches)


if __name__ == "__main__":
    main()
