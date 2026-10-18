#!/usr/bin/env python3
"""Original-byte full-state parity for the raw-CPU SYSTEM leaves 0AF8:0B57 (Copy), 0C10 (string compare), 0C3B (char to
string) and 1685 (Move), plus a DOSBox-X oracle.

The differential places the hash-checked expanded START.EXE image at load segment 0, so each leaf runs from its original
bytes, and compares the entire 1 MiB RAM and all 14 CPU words between the original under Unicorn and the C model, with no
mask, after the leaf's RETF. Strings and blocks sit apart, overlapping in both directions, in the same place, in different
segments and across segment ends, so REP MOVSB/CMPSB reread bytes they have written and SI/DI wrap. Every case keeps its
stack frame apart from the data. Media stays private.
"""
import argparse
import ctypes
from hashlib import sha256
import json
import os
from pathlib import Path
import random
import struct
import subprocess
import tempfile

from capstone import CS_ARCH_X86, CS_MODE_16, Cs
from resident_loader_map import unpack
from u86_procvars_3413_audit import Cpu, ROOT, SIZE, EXE_SHA, REGISTERS, physical, put16

SYSTEM_CS = 0x0AF8
LEAVES = {"copy": (0x0B57, 0x0B98, 29, 0x0B95, "8"), "compare": (0x0C10, 0x0C3B, 20, 0x0C38, "8"),
          "char": (0x0C3B, 0x0C4D, 8, 0x0C4A, "2"), "move": (0x1685, 0x16A8, 16, 0x16A5, "0xa")}
SYMBOLS = {"copy": "pr_system_strcopy_0b57", "compare": "pr_system_strcmp_0c10", "char": "pr_system_charstr_0c3b",
           "move": "pr_system_move_1685"}
SS = 0x2800
RET_CS, RET_IP = 0x9001, 0x7000
FLAGS = (0x0602, 0x0202, 0x0ED7, 0x0647, 0x72D7, 0x0403, 0x0213)


def check(condition, message):
    if not condition:
        raise ValueError(message)


def source():
    exe = (ROOT / "resources/DOS/POOLRAD/START.EXE").read_bytes()
    check(len(exe) == 65231 and sha256(exe).hexdigest() == EXE_SHA, "START.EXE provenance differs")
    image = bytes(unpack(exe)[0])
    check(len(image) <= physical(SS, 0), "the image reaches the stack segment")
    md = Cs(CS_ARCH_X86, CS_MODE_16)
    spans = {}
    for name, (start, end, count, retf, pop) in LEAVES.items():
        span = image[physical(SYSTEM_CS, start):physical(SYSTEM_CS, end)]
        insns = list(md.disasm(span, start))
        check(len(insns) == count and sum(i.size for i in insns) == len(span) and insns[-1].address == retf
              and insns[-1].mnemonic == "retf" and insns[-1].op_str == pop, f"SYSTEM {name} differs")
        conditional = {i.address: (i.address + i.size, int(i.op_str, 16)) for i in insns if i.mnemonic.startswith("j") and i.mnemonic != "jmp"}
        spans[name] = dict(bytes=len(span), instructions=count, sha256=sha256(span).hexdigest(), decoded={i.address: i for i in insns},
                           conditional=conditional)
        print(f"SYSTEM {name} {start:04X}..{end - 1:04X}: {len(span)} B, {count} insns, {len(conditional)} conditional jumps, "
              f"sha {spans[name]['sha256']}")
    return image, spans


def library(directory):
    path = directory / "leaves.so"
    subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-O2", "-fPIC", "-shared", "-I", str(ROOT / "include"),
                    str(ROOT / "src/recovered/system_string_move_leaves_cpu.c"), "-o", str(path)], check=True, capture_output=True, text=True)
    lib = ctypes.CDLL(str(path))
    functions = {}
    for name, symbol in SYMBOLS.items():
        f = getattr(lib, symbol)
        f.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t, ctypes.POINTER(Cpu)]
        f.restype = ctypes.c_int
        functions[name] = f
    return functions


def place(memory, segment, offset, data):
    for i, value in enumerate(data):
        memory[physical(segment, (offset + i) & 0xFFFF)] = value


def build(image, rng, *, leaf, words, data=(), ds=None, sp=0x4000, ss=SS, flags=0x0602, background=None):
    """The data placed in order (later blocks may overwrite earlier ones), then the far-call frame RET, *words at SS:SP."""
    memory = bytearray([0xA5]) * SIZE if background is None else bytearray(background)
    memory[0:len(image)] = image
    for segment, offset, blob in data:
        place(memory, segment, offset, blob)
    frame = physical(ss, sp)
    for i, value in enumerate((RET_IP, RET_CS, *words)):
        put16(memory, ss, (sp + 2 * i) & 0xFFFF, value)
        put16(memory, 0, (frame + 2 * i) & (SIZE - 1), value)
    reg = tuple(rng.randrange(0x10000) for _ in range(7))
    entry = LEAVES[leaf][0]
    return memory, Cpu(*reg, sp, SYSTEM_CS, entry, rng.randrange(0x10000) if ds is None else ds, rng.randrange(0x10000), ss, flags), ss >= 0xF000


def compare(label, leaf, memory, cpu, high, spans, functions):
    from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
    from unicorn.x86_const import UC_X86_REG_CS, UC_X86_REG_IP
    original = Uc(UC_ARCH_X86, UC_MODE_16)
    original.mem_map(0, 2 * SIZE)
    original.mem_write(0, bytes(memory)); original.mem_write(SIZE, bytes(memory))
    for (name, _), register in zip(Cpu._fields_, REGISTERS):
        original.reg_write(register, getattr(cpu, name))
    decoded, retf, conditional = spans[leaf]["decoded"], LEAVES[leaf][3], spans[leaf]["conditional"]
    state = dict(done=False, returned=False, unexpected=None, visited=set(), edges=set(), previous=None)

    def hook(emulator, address, size, user):
        del address, size, user
        cs, ip = emulator.reg_read(UC_X86_REG_CS), emulator.reg_read(UC_X86_REG_IP)
        if state["returned"]:
            state["done"] = True
            return emulator.emu_stop()
        if cs != SYSTEM_CS or ip not in decoded:
            state["unexpected"] = (cs, ip)
            return emulator.emu_stop()
        state["visited"].add(ip)
        if state["previous"] in conditional and state["previous"] != ip:
            state["edges"].add((state["previous"], ip))
        state["previous"] = ip
        state["returned"] = ip == retf
        return None

    def mirror(emulator, access, address, size, value, user):
        del access, user
        for byte in range(size):
            raw = address + byte; at = raw & (SIZE - 1)
            emulator.mem_write(at + SIZE if raw < SIZE else at, bytes([(value >> (8 * byte)) & 0xFF]))

    original.hook_add(UC_HOOK_CODE, hook)
    if high:
        original.hook_add(UC_HOOK_MEM_WRITE, mirror)
    original.emu_start(physical(SYSTEM_CS, LEAVES[leaf][0]), 2 * SIZE, count=1000000)
    check(state["unexpected"] is None and state["done"], f"original did not return cleanly ({state['unexpected']}): {label}")
    expected = bytes(original.mem_read(0, SIZE))
    ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
    check(functions[leaf](ram, SIZE, ctypes.byref(cpu)) == 0, f"C status: {label}")
    actual = bytes(ram)
    if actual != expected:
        differences = [i for i, (a, b) in enumerate(zip(actual, expected)) if a != b]
        raise ValueError(f"RAM mismatch {label}: {[(hex(d), actual[d], expected[d]) for d in differences[:6]]} ({len(differences)} bytes)")
    for (name, _), register in zip(Cpu._fields_, REGISTERS):
        mine, want = getattr(cpu, name), original.reg_read(register) & 0xFFFF
        check(mine == want, f"{name} mismatch {label}: C={mine:04x}, original={want:04x}")
    return state["visited"], state["edges"]


def text(rng, length):
    return bytes(rng.randrange(256) for _ in range(length))


def pstring(body):
    return bytes([len(body)]) + body


def segment(rng):
    return rng.randrange(0x3000, 0x8000)


def copy_case(rng):
    mode = rng.choice(("apart", "apart", "d-after-s", "s-after-d", "same", "wrap"))
    slen, seg = rng.choice((0, 1, 2, 9, 128, 254, 255, rng.randrange(256))), segment(rng)
    if mode == "apart":
        s, d = (seg, rng.randrange(0xF000)), (segment(rng), rng.randrange(0xF000))
    elif mode == "d-after-s":
        off = rng.randrange(0xE000); s, d = (seg, off), (seg, off + rng.randrange(1, 40))
    elif mode == "s-after-d":
        off = rng.randrange(0xE000); d, s = (seg, off), (seg, off + rng.randrange(1, 40))
    elif mode == "same":
        s = d = (seg, rng.randrange(0xE000))
    else:
        s, d = (seg, rng.randrange(0xFF00, 0x10000)), (segment(rng), rng.randrange(0xFF00, 0x10000))
    index = rng.choice((0xFFFF, 0, 1, 2, slen, slen + 1, slen + 2, 0x7FFF, 0x8000, rng.randrange(0x10000), rng.randrange(1, slen + 2)))
    count = rng.choice((0xFFFF, 0, 1, 2, slen, 255, 0x7FFF, 0x8000, rng.randrange(0x10000), rng.randrange(300)))
    return mode, dict(words=(count, index, s[1], s[0], d[1], d[0]), data=[(d[0], d[1], text(rng, 256)), (s[0], s[1], pstring(text(rng, slen)))])


def compare_strings(rng, relation):
    left = text(rng, rng.choice((0, 1, 2, 5, 100, 255, rng.randrange(256))))
    if relation == "equal":
        right = left
    elif relation == "left-prefix":
        right = (left + text(rng, rng.randrange(1, 20)))[:255]
    elif relation == "right-prefix":
        right = left[:rng.randrange(len(left) + 1)]
    elif relation == "differ":
        left = left or text(rng, 1)
        k = rng.randrange(len(left))
        right = (left[:k] + bytes([(left[k] + rng.randrange(1, 256)) & 0xFF]) + left[k + 1:] + text(rng, rng.choice((0, 0, 3))))[:255]
    elif relation == "empty":
        left, right = b"", rng.choice((b"", text(rng, 3)))
    else:
        right = text(rng, rng.randrange(256))
    if rng.random() < 0.5:
        left, right = right, left
    return left, right


def compare_case(rng):
    relation = rng.choice(("equal", "left-prefix", "right-prefix", "differ", "differ", "random", "empty", "same"))
    mode = rng.choice(("apart", "apart", "overlap", "wrap"))
    left, right = compare_strings(rng, "equal" if relation == "same" else relation)
    seg = segment(rng)
    if relation == "same":
        l = r = (seg, rng.randrange(0xFF00) if mode != "wrap" else rng.randrange(0xFF00, 0x10000))
    elif mode == "apart":
        l, r = (seg, rng.randrange(0xF000)), (segment(rng), rng.randrange(0xF000))
    elif mode == "overlap":
        off = rng.randrange(0xE000); l, r = (seg, off), (seg, off + rng.randrange(1, 40))
    else:
        l, r = (seg, rng.randrange(0xFF00, 0x10000)), (segment(rng), rng.randrange(0xFF00, 0x10000))
    data = [(r[0], r[1], pstring(right)), (l[0], l[1], pstring(left))]
    return f"{relation}-{mode}", dict(words=(r[1], r[0], l[1], l[0]), data=data)


def char_case(rng):
    d = (segment(rng), rng.choice((rng.randrange(0x10000), 0xFFFF, 0xFFFE)))
    return "char", dict(words=(rng.randrange(0x10000), d[1], d[0]), data=[(d[0], d[1], text(rng, 4))])


def move_case(rng):
    mode = rng.choice(("apart", "forward-overlap", "backward-overlap", "same", "segments", "wrap-forward", "wrap-backward"))
    count, seg = rng.choice((0, 1, 2, 7, 64, 255, 256, 1000, rng.randrange(0x800))), segment(rng)
    if mode == "apart":
        s, d = (seg, rng.randrange(0xE000)), (segment(rng), rng.randrange(0xE000))
    elif mode == "forward-overlap":
        d = (seg, rng.randrange(0xE000)); s = (seg, d[1] + rng.randrange(1, 40))
    elif mode == "backward-overlap":
        s = (seg, rng.randrange(0xE000)); d = (seg, s[1] + rng.randrange(1, 40))
    elif mode == "same":
        s = d = (seg, rng.randrange(0xE000))
    elif mode == "segments":
        s, d = (seg, rng.randrange(0x8000)), (segment(rng), rng.randrange(0x8000))
    elif mode == "wrap-forward":
        s, d = (seg, rng.randrange(0xF800, 0x10000)), (segment(rng), rng.randrange(0xF800, 0x10000))
    else:
        s = (seg, rng.randrange(0xFE00, 0xFF00)); d = (seg, s[1] + rng.randrange(1, 0x100))
    data = [(d[0], d[1], text(rng, count + 8)), (s[0], s[1], text(rng, count + 8))]
    return mode, dict(words=(count, d[1], d[0], s[1], s[0]), data=data)


def cases(rng):
    makers = (("copy", copy_case), ("compare", compare_case), ("char", char_case), ("move", move_case))
    spec = []
    for i in range(480):
        leaf, maker = makers[i % 4]
        mode, options = maker(rng)
        spec.append((f"random-{i}-{leaf}-{mode}", leaf, dict(options, flags=FLAGS[i % len(FLAGS)], sp=rng.randrange(0x0400, 0x7000) & ~1)))
    hello = pstring(b"Hello, Phlan")
    s0, d0 = (0x4000, 0x0100), (0x5000, 0x0200)
    for name, index, count in (("copy-index-0", 0, 3), ("copy-index-negative", 0x8000, 3), ("copy-index-len", 12, 5),
                               ("copy-index-past", 13, 5), ("copy-count-negative", 2, 0xFFFF), ("copy-count-huge", 2, 0x7FFF),
                               ("copy-count-zero", 2, 0)):
        spec.append((name, "copy", dict(words=(count, index, s0[1], s0[0], d0[1], d0[0]), data=[(s0[0], s0[1], hello)])))
    for name, left, right in (("compare-equal", b"Pool", b"Pool"), ("compare-left-shorter", b"Poo", b"Pool"), ("compare-first-differs", b"Aa", b"Ba"),
                              ("compare-last-differs", b"PooL", b"Pool"), ("compare-both-empty", b"", b""), ("compare-left-empty", b"", b"x")):
        spec.append((name, "compare", dict(words=(d0[1], d0[0], s0[1], s0[0]), data=[(s0[0], s0[1], pstring(left)), (d0[0], d0[1], pstring(right))])))
    spec.append(("char-wrap", "char", dict(words=(0x4142, 0xFFFF, 0x5000))))
    for name, s, d, count in (("move-zero-forward", (0x4000, 0x200), (0x4000, 0x100), 0), ("move-zero-backward", (0x4000, 0x100), (0x4000, 0x200), 0),
                              ("move-di-7fff", (0x4000, 0x1000), (0x4000, 0x7F00), 0x100), ("move-di-zero", (0x4000, 0x1000), (0x4000, 0xFFF0), 0x11),
                              ("move-wrap-carry", (0x4000, 0xFF00), (0x4000, 0xFF80), 0x100)):
        spec.append((name, "move", dict(words=(count, d[1], d[0], s[1], s[0]), data=[(s[0], s[1], text(rng, count + 8))])))
    for leaf in LEAVES:
        _, options = dict(copy=copy_case, compare=compare_case, char=char_case, move=move_case)[leaf](rng)
        spec.append((f"{leaf}-ss-a20", leaf, dict(options, sp=0x0400, ss=0xFFFF)))
        _, options = dict(copy=copy_case, compare=compare_case, char=char_case, move=move_case)[leaf](rng)
        spec.append((f"{leaf}-df-set", leaf, dict(options, flags=0x0603)))
        _, options = dict(copy=copy_case, compare=compare_case, char=char_case, move=move_case)[leaf](rng)
        spec.append((f"{leaf}-random-background", leaf, dict(options, background=bytes(random.Random(0x1685 + len(leaf)).randbytes(SIZE)))))
    return spec


def differential(spans, functions):
    rng = random.Random(0x0B570C10)
    visited, edges, counts = {k: set() for k in LEAVES}, {k: set() for k in LEAVES}, {k: 0 for k in LEAVES}
    for label, leaf, options in cases(rng):
        memory, cpu, high = build(spans["image"], rng, leaf=leaf, **options)
        v, e = compare(label, leaf, memory, cpu, high, spans, functions)
        visited[leaf] |= v
        edges[leaf] |= e
        counts[leaf] += 1
    for leaf in LEAVES:
        reachable = {(s, o) for s, outs in spans[leaf]["conditional"].items() for o in outs}
        check(visited[leaf] == set(spans[leaf]["decoded"]), f"{leaf} instruction coverage incomplete")
        check(edges[leaf] == reachable, f"{leaf} conditional coverage incomplete: {sorted(reachable - edges[leaf])}")
    print(f"SYSTEM Copy/compare/char/Move: {sum(counts.values())} original-byte cases ({counts}); full 1 MiB RAM and all 14 CPU words equal "
          f"after each RETF, no mask; every instruction and both outcomes of every conditional jump executed in all four leaves")
    return dict(cases=counts, starts={k: len(v) for k, v in visited.items()}, outcomes={k: len(v) for k, v in edges.items()})


# ---- DOSBox-X synthetic oracle -------------------------------------------------------------------------
SYS_N, DATA_N = 0x7000, 0x5000
SPAN_LOW, SPAN_HIGH = 0x0B00, 0x1700
MAIN_AT, SPAN_AT, BLOB_AT, CASES_AT = 0x0300, 0x0C00, 0x1800, 0x2800
OUTPUT, HEADER, STACK_TOP = 0xC000, 0xD800, 0xF000
WINDOW_LOW, WINDOW_SIZE, BLOB_SIZE = STACK_TOP - 0x40, 0x40, 0x1000
CASE_SIZE, NATIVE_CASES, MAX_CHUNKS = 48, 128, 5
RECORD_BYTES = 28 + WINDOW_SIZE + 0x10000
NATIVE_FLAGS = (0x0002, 0x00D7, 0x08D7, 0x0402, 0x0893, 0x0003)
ENTRY_REGS = dict(ax=0x1234, bx=0x3456, cx=0x5678, dx=0x789A, si=0x9ABC, di=0xBCDE, bp=0x7777)
NAMES = tuple(LEAVES)


def native_cases(rng, blob):
    """Each case: the leaf, its flags, the pushed words (first pushed first) and blob chunks (offset, length, blob start)
    copied into the data segment in order; blob[0..255] holds the byte values 0..255 for length bytes."""
    cases = []
    for n in range(NATIVE_CASES):
        leaf = NAMES[n % 4]
        chunks = []

        def string(offset, body_start, length):
            chunks.append((offset & 0xFFFF, 1, length)); chunks.append(((offset + 1) & 0xFFFF, length, body_start))

        def spot(wrap):
            return rng.randrange(0xFF00, 0x10000) if wrap else rng.randrange(0x0000, 0xF000)

        wrap = rng.random() < 0.2
        if leaf == "copy":
            slen = rng.choice((0, 1, 9, 128, 255, rng.randrange(256)))
            s, d = spot(wrap), spot(rng.random() < 0.2)
            if rng.random() < 0.3:
                d = (s + rng.randrange(-30, 30)) & 0xFFFF
            string(s, rng.randrange(256, BLOB_SIZE - 256), slen)
            index = rng.choice((0xFFFF, 0, 1, 2, slen, slen + 1, 0x8000, rng.randrange(1, slen + 2)))
            count = rng.choice((0xFFFF, 0, 1, slen, 255, 0x7FFF, rng.randrange(300)))
            words = (DATA_N, d, DATA_N, s, index, count)
        elif leaf == "compare":
            ll, start = rng.choice((0, 1, 5, 100, 255, rng.randrange(256))), rng.randrange(256, BLOB_SIZE - 256)
            relation = rng.choice(("equal", "prefix", "differ", "random"))
            rl = ll if relation in ("equal", "differ") else rng.randrange(256)
            rstart = start if relation in ("equal", "prefix", "differ") else rng.randrange(256, BLOB_SIZE - 256)
            l, r = spot(wrap), spot(rng.random() < 0.2)
            string(r, rstart, rl)
            string(l, start, ll)
            if relation == "differ" and ll:
                k = rng.randrange(ll)
                chunks.append(((r + 1 + k) & 0xFFFF, 1, (blob[start + k] + rng.randrange(1, 256)) & 0xFF))
            words = (DATA_N, l, DATA_N, r)
        elif leaf == "char":
            words = (DATA_N, rng.choice((spot(False), 0xFFFF)), rng.randrange(0x10000))
        else:
            count = rng.choice((0, 1, 7, 256, rng.randrange(0x300)))
            s = spot(wrap)
            d = (s + rng.choice((rng.randrange(-40, 40), rng.randrange(0x10000)))) & 0xFFFF
            chunks.append((s, count, rng.randrange(256, BLOB_SIZE - 0x300)))
            words = (DATA_N, s, DATA_N, d, count)
        check(len(chunks) <= MAX_CHUNKS, "too many native chunks")
        cases.append(dict(leaf=n % 4, flags=n % len(NATIVE_FLAGS), words=words, chunks=chunks))
    return cases


def case_bytes(case):
    data = struct.pack("<BBBB", case["leaf"], case["flags"], len(case["words"]), len(case["chunks"]))
    data += struct.pack(f"<{len(case['words'])}H", *case["words"]) + bytes(12 - 2 * len(case["words"]))
    data += b"".join(struct.pack("<HHH", *chunk) for chunk in case["chunks"])
    check(len(data) <= CASE_SIZE, "native case too large")
    return data + bytes(CASE_SIZE - len(data))


def native_memory(case, blob):
    data = bytearray([0xA5]) * 0x10000
    for offset, length, start in case["chunks"]:
        for i in range(length):
            data[(offset + i) & 0xFFFF] = blob[start + i]
    return data


def native_audit(spans, functions):
    directory = ROOT / "resources/oracle-runs/system-string-move-leaves-synthetic"
    directory.mkdir(parents=True, exist_ok=True)
    image = spans["image"]
    span = image[physical(SYSTEM_CS, SPAN_LOW):physical(SYSTEM_CS, SPAN_HIGH)]
    rng = random.Random(0x0B571685)
    blob = bytes(range(256)) + bytes(rng.randrange(256) for _ in range(BLOB_SIZE - 256))
    cases = native_cases(rng, blob)
    check(SPAN_AT + len(span) <= BLOB_AT and BLOB_AT + len(blob) <= CASES_AT and CASES_AT + CASE_SIZE * len(cases) <= OUTPUT, "COM layout overlaps")
    t = [".code16", ".intel_syntax noprefix", ".global _start", "_start:", "    jmp main", f".org {MAIN_AT - 0x100:#x}", "main:",
         "    mov ax,cs", "    mov ds,ax", "    mov es,ax", "    mov ss,ax", f"    mov sp,{STACK_TOP:#x}", "    cld",
         f"    mov ax,{SYS_N:#x}", "    mov es,ax", "    mov si,offset span", f"    mov di,{SPAN_LOW:#x}", f"    mov cx,{len(span):#x}", "    rep movsb",
         "    push cs", "    pop es", f"    mov word ptr [{HEADER:#x}],cs", f"    mov word ptr [{HEADER + 2:#x}],offset returned",
         f"    mov word ptr [{HEADER + 4:#x}],{len(cases)}", f"    mov word ptr [{HEADER + 6:#x}],0",
         "    mov dx,offset filename", "    xor cx,cx", "    mov ah,0x3c", "    int 0x21", "    jc failure",
         "    mov word ptr [handle],ax", "    mov bx,ax", f"    mov dx,{HEADER:#x}", "    mov cx,8", "    mov ah,0x40", "    int 0x21", "    jc failure",
         "again:", "    cli", f"    mov sp,{STACK_TOP:#x}", "    cld", "    push cs", "    pop ds", "    push cs", "    pop es",
         "    mov al,0xa5", f"    mov di,{WINDOW_LOW:#x}", f"    mov cx,{WINDOW_SIZE:#x}", "    rep stosb",
         "    mov ax,word ptr [index]", f"    mov bx,{CASE_SIZE}", "    mul bx", "    add ax,offset cases", "    mov bx,ax",
         f"    mov ax,{DATA_N:#x}", "    mov es,ax", "    xor di,di", "    mov cx,0x8000", "    mov ax,0xa5a5", "    rep stosw",
         "    lea ax,[bx+16]", "    mov word ptr [chunk_ptr],ax", "    xor ah,ah", "    mov al,byte ptr [bx+3]", "    mov word ptr [chunk_count],ax",
         "chunk_loop:", "    cmp word ptr [chunk_count],0", "    je chunks_done", "    mov bp,word ptr [chunk_ptr]",
         "    mov di,word ptr ds:[bp]", "    mov cx,word ptr ds:[bp+2]", "    mov si,offset blob", "    add si,word ptr ds:[bp+4]", "    rep movsb",
         "    add word ptr [chunk_ptr],6", "    dec word ptr [chunk_count]", "    jmp chunk_loop", "chunks_done:",
         "    xor ch,ch", "    mov cl,byte ptr [bx+2]", "    lea si,[bx+4]", "push_loop:", "    push word ptr [si]", "    add si,2", "    loop push_loop",
         "    xor ah,ah", "    mov al,byte ptr [bx]", "    shl ax,1", "    mov si,ax", "    mov ax,word ptr [entry_table+si]", "    mov word ptr [target_far],ax",
         "    xor ah,ah", "    mov al,byte ptr [bx+1]", "    shl ax,1", "    mov si,ax",
         "    push word ptr [flags_table+si]", "    push cs", "    pop es", "    mov ax,0x4444", "    mov ds,ax"]
    for name, value in ENTRY_REGS.items():
        t.append(f"    mov {name},{value:#x}")
    t += ["    popf", "    call dword ptr cs:[target_far]", "returned:"]
    for i, reg in enumerate("ax bx cx dx si di bp sp cs".split()):
        t.append(f"    mov word ptr cs:[{OUTPUT + 2 * i:#x}],{reg}")
    t += [f"    mov word ptr cs:[{OUTPUT + 18:#x}],offset returned", f"    mov word ptr cs:[{OUTPUT + 20:#x}],ds",
          f"    mov word ptr cs:[{OUTPUT + 22:#x}],es", f"    mov word ptr cs:[{OUTPUT + 24:#x}],ss",
          f"    mov sp,{STACK_TOP + 0x800:#x}", "    pushf", "    pop ax", f"    mov word ptr cs:[{OUTPUT + 26:#x}],ax",
          "    cld", "    push cs", "    pop ds", "    push cs", "    pop es", f"    mov di,{OUTPUT + 28:#x}",
          f"    mov si,{WINDOW_LOW:#x}", f"    mov cx,{WINDOW_SIZE:#x}", "    rep movsb", "    sti",
          "    mov bx,word ptr [handle]", f"    mov dx,{OUTPUT:#x}", f"    mov cx,{28 + WINDOW_SIZE}", "    mov ah,0x40", "    int 0x21", "    jc failure",
          f"    mov ax,{DATA_N:#x}", "    mov ds,ax", "    xor dx,dx", "    mov cx,0x8000", "    mov ah,0x40", "    int 0x21", "    jc failure",
          "    mov dx,0x8000", "    mov cx,0x8000", "    mov ah,0x40", "    int 0x21", "    jc failure", "    push cs", "    pop ds",
          "    inc word ptr [index]", "    mov ax,word ptr [index]", f"    cmp ax,{len(cases)}", "    jb again",
          "    mov bx,word ptr [handle]", "    mov ah,0x3e", "    int 0x21", "    mov ax,0x4c00", "    int 0x21",
          "failure:", "    mov ax,0x4c01", "    int 0x21",
          'filename: .asciz "RESULT.BIN"', "handle: .word 0", "index: .word 0", "chunk_ptr: .word 0", "chunk_count: .word 0",
          f"target_far: .word 0x0b57, {SYS_N:#x}", "entry_table: .word " + ",".join(f"{LEAVES[name][0]:#x}" for name in NAMES),
          "flags_table: .word " + ",".join(f"{f:#x}" for f in NATIVE_FLAGS)]
    table = b"".join(case_bytes(c) for c in cases)
    for label, data, where in (("span", span, SPAN_AT), ("blob", blob, BLOB_AT), ("cases", table, CASES_AT)):
        t += [f".org {where - 0x100:#x}", f"{label}:"]
        for i in range(0, len(data), 16):
            t.append("    .byte " + ",".join(f"{b:#x}" for b in data[i:i + 16]))
    asm, obj = directory / "oracle.S", directory / "oracle.o"
    asm.write_text("\n".join(t) + "\n")
    subprocess.run(["as", "--32", str(asm), "-o", str(obj)], check=True)
    subprocess.run(["ld", "-m", "elf_i386", "-Ttext", "0x100", "--oformat", "binary", str(obj), "-o", str(directory / "ORACLE.COM")], check=True)
    config = directory / "dosbox.conf"
    config.write_text("[sdl]\noutput=surface\n[dosbox]\nmemsize=16\n[cpu]\ncore=normal\ncputype=386\ncycles=max\n"
                      "[mixer]\nnosound=true\n[midi]\nmididevice=none\n"
                      "[autoexec]\nmount c " + str(directory.resolve()) + "\nc:\nORACLE.COM\nexit\n")
    result_path = directory / "RESULT.BIN"
    if result_path.exists():
        result_path.unlink()
    environment = dict(os.environ, SDL_VIDEODRIVER="dummy", SDL_AUDIODRIVER="dummy")
    with (directory / "dosbox.log").open("w") as log:
        subprocess.run(["dosbox-x", "-conf", str(config), "-fastlaunch", "-exit", "-noconsole"],
                       env=environment, stdout=log, stderr=subprocess.STDOUT, check=True, timeout=600)
    result = result_path.read_bytes()
    check(len(result) == 8 + len(cases) * RECORD_BYTES, f"native output incomplete: {len(result)} bytes")
    segment_, returned, count, _ = struct.unpack_from("<4H", result)
    check(count == len(cases) and segment_ == 0x0814, f"native header differs or COM segment {segment_:04x} is not 0814")
    tally = {name: 0 for name in NAMES}
    for n, case in enumerate(cases):
        record = result[8 + n * RECORD_BYTES:8 + (n + 1) * RECORD_BYTES]
        leaf = NAMES[case["leaf"]]
        memory = bytearray(SIZE)
        at = physical(SYS_N, SPAN_LOW); memory[at:at + len(span)] = span
        at = physical(DATA_N, 0); memory[at:at + 0x10000] = native_memory(case, blob)
        for i in range(WINDOW_SIZE):
            memory[physical(segment_, WINDOW_LOW + i)] = 0xA5
        sp = STACK_TOP
        for value in (*case["words"], segment_, returned):
            sp -= 2; put16(memory, segment_, sp, value)
        cpu = Cpu(*ENTRY_REGS.values(), sp, SYS_N, LEAVES[leaf][0], 0x4444, segment_, segment_, NATIVE_FLAGS[case["flags"]])
        ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
        check(functions[leaf](ram, SIZE, ctypes.byref(cpu)) == 0, f"C status on native case {n}")
        for (name, _), want in zip(Cpu._fields_, struct.unpack_from("<14H", record)):
            check(getattr(cpu, name) == want, f"native case {n} ({leaf}) {name}: C={getattr(cpu, name):04x}, DOS={want:04x}")
        got = bytes(ram[physical(segment_, WINDOW_LOW):physical(segment_, WINDOW_LOW) + WINDOW_SIZE]) + bytes(ram[physical(DATA_N, 0):physical(DATA_N, 0) + 0x10000])
        if got != record[28:]:
            differences = [i for i, (a, b) in enumerate(zip(got, record[28:])) if a != b]
            raise ValueError(f"native case {n} ({leaf}) bytes differ at {[hex(d) for d in differences[:8]]} ({len(differences)} bytes)")
        tally[leaf] += 1
    digest = sha256(result).hexdigest()
    print(f"DOSBox-X normal-core/386 synthetic cases passed: {len(cases)} ({tally}); all 14 CPU words, the stack window and the whole "
          f"64 KiB data segment; no mask")
    print(f"native result SHA-256 {digest}; COM segment {segment_:04x}; SYSTEM span {SPAN_LOW:04X}..{SPAN_HIGH - 1:04X} at {SYS_N:04X}")
    return dict(cases=len(cases), leaves=tally, result_sha256=digest)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--unicorn", action="store_true")
    parser.add_argument("--native", action="store_true")
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    image, spans = source()
    spans["image"] = image
    report = dict(target={k: dict(bytes=v["bytes"], instructions=v["instructions"], sha256=v["sha256"]) for k, v in spans.items() if k in LEAVES})
    if args.unicorn or args.native:
        with tempfile.TemporaryDirectory(prefix="poolrad-strmove-") as temp:
            functions = library(Path(temp))
            if args.unicorn:
                report["unicorn"] = differential(spans, functions)
            if args.native:
                report["native"] = native_audit(spans, functions)
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(json.dumps(report, sort_keys=True, indent=2) + "\n")
    print(json.dumps(report, sort_keys=True))


if __name__ == "__main__":
    main()
