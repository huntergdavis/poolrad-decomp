#!/usr/bin/env python3
"""Original-byte full-state parity for raw-CPU U38 +0679 or its twin +0947 (--routine), composing the SYSTEM string
compare 0AF8:0C10.

The differential loads unit 0x38 at its original offsets and the original START.EXE SYSTEM bytes 0AF8:0C10..0C3A into
one 1 MiB image, and compares the entire RAM and all 14 CPU words between the original under Unicorn and the C model,
with no mask. The original decides each stop: the fetch after its RETF 0Ah (return), the LCALL fetched with other bytes
(CODE), or a fetch of the loop-back INC after more taken JNE loop-backs than the host cap (LIMIT). Any fetch outside the two
spans is an error. The argument, link and string records sit in separate 64 KiB segments, apart from the code, the data
segment and the stack. Media stays private.
"""
import argparse
import ctypes
from hashlib import sha256
import json
from pathlib import Path
import random
import struct
import subprocess
import tempfile

from capstone import CS_ARCH_X86, CS_MODE_16, Cs
from overlay_inventory import find_units, parse_mz
from resident_loader_map import unpack
from u86_procvars_3413_audit import Cpu, ROOT, SIZE, EXE_SHA, OVR_SHA, REGISTERS, physical, put16

CALLER_CS, DS, SS = 0x3000, 0x1000, 0x2000
A_SEG, B_SEG, P_SEG = 0x4000, 0x5000, 0x6000
SYSTEM_CS, STRCMP, STRCMP_END = 0x0AF8, 0x0C10, 0x0C3B
ROUTINES = {"0679": (0x0679, 0x0332, 4), "0947": (0x0947, 0x0372, 8)}
RET_CS, RET_IP = 0x9001, 0x7000
ARGS = (0x1111, 0x2222, 0x3333)
OK, CODE, CALLEE, TRANSFER, LIMIT = 0, 2, 3, 4, 5
NAMES = {OK: "return", CODE: "CODE", CALLEE: "CALLEE", TRANSFER: "TRANSFER", LIMIT: "LIMIT"}
FLAGS = (0x0602, 0x0202, 0x0ED7, 0x0647, 0x72D7, 0x0403, 0x0213)


def check(condition, message):
    if not condition:
        raise ValueError(message)


def select(name):
    """Point the routine constants at one twin; +0947 is +0679's code shifted by 2CEh."""
    global ROUTINE, ENTRY, END, BODY_RETF, SITE, LOOP_JNE, LOOP_HEAD, TABLE, LAST
    ROUTINE = name
    ENTRY, TABLE, LAST = ROUTINES[name]
    END, BODY_RETF, SITE, LOOP_JNE, LOOP_HEAD = ENTRY + 0x6B, ENTRY + 0x68, ENTRY + 0x54, ENTRY + 0x63, ENTRY + 0x3A


select("0679")


def source():
    exe = (ROOT / "resources/DOS/POOLRAD/START.EXE").read_bytes()
    ovr = (ROOT / "resources/DOS/POOLRAD/GAME.OVR").read_bytes()
    check(sha256(exe).hexdigest() == EXE_SHA and len(ovr) == 206105 and sha256(ovr).hexdigest() == OVR_SHA, "media provenance differs")
    u = next(u for u in find_units(exe[parse_mz(exe)["header_bytes"]:], ovr) if u["stub_para"] == 0x38)
    code = ovr[u["ovr_offset"]:u["ovr_offset"] + u["code_size"]]
    insns = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(code[ENTRY:END], ENTRY))
    calls = [(i.address, i.op_str) for i in insns if "call" in i.mnemonic]
    check(len(insns) == 37 and sum(i.size for i in insns) == END - ENTRY and insns[-1].address == BODY_RETF
          and (insns[-1].mnemonic, insns[-1].op_str) == ("retf", "0xa") and calls == [(SITE, "0xaf8, 0xc10")]
          and SITE + 3 in set(u["fixups"]), f"U38 +{ROUTINE} differs: {calls}")
    expanded = bytes(unpack(exe)[0])
    system = expanded[physical(SYSTEM_CS, STRCMP):physical(SYSTEM_CS, STRCMP_END)]
    leaf = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(system, STRCMP))
    check(len(leaf) == 20 and sum(i.size for i in leaf) == len(system) and (leaf[-1].mnemonic, leaf[-1].op_str) == ("retf", "8"),
          "SYSTEM string compare differs")
    conditional = {i.address: (i.address + i.size, int(i.op_str, 16)) for i in insns if i.mnemonic.startswith("j") and i.mnemonic != "jmp"}
    sha = sha256(code[ENTRY:END]).hexdigest()
    print(f"U38 +{ROUTINE} (107/37, LCALL 0AF8:0C10, RETF 0Ah) GAME file {u['ovr_offset'] + ENTRY:#x} sha {sha}; "
          f"SYSTEM 0C10..0C3A sha {sha256(system).hexdigest()[:16]}...")
    return dict(conditional=conditional, starts=[i.address for i in insns], sha=sha, file_offset=u["ovr_offset"] + ENTRY), code, system


def library(directory):
    path = directory / "u38match.so"
    sources = ["overlay_u38_string_match_cpu.c", "system_string_move_leaves_cpu.c"]
    built = subprocess.run(["cc", "-std=c11", "-O2", "-fPIC", "-shared", "-I", str(ROOT / "include"),
                            *[str(ROOT / "src/recovered" / s) for s in sources], "-o", str(path)], capture_output=True, text=True)
    check(built.returncode == 0, built.stderr[-2000:])
    function = getattr(ctypes.CDLL(str(path)), f"pr_u38_string_match_{ROUTINE}_cpu")
    function.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t, ctypes.POINTER(Cpu), ctypes.c_uint, ctypes.POINTER(ctypes.c_int)]
    function.restype = ctypes.c_int
    return function


def pstring(rng, length=None):
    length = rng.choice((0, 1, 3, 8, 15, rng.randrange(16))) if length is None else length
    return bytes([length]) + bytes(rng.choice((0x41, 0x61, 0x5A, 0x20, rng.randrange(256))) for _ in range(length))


def variant(rng, name):
    """A table string equal to name, a prefix or extension of it, one byte off, or unrelated."""
    kind = rng.randrange(6)
    if kind < 2 or len(name) < 2:
        return name
    if kind == 2:
        return bytes([name[0] - 1]) + name[1:name[0]]
    if kind == 3 and name[0] < 15:
        return bytes([name[0] + 1]) + name[1:] + bytes([rng.randrange(256)])
    if kind == 4:
        at = rng.randrange(1, name[0] + 1)
        return name[:at] + bytes([name[at] ^ 0x01]) + name[at + 1:]
    return pstring(rng)


def build(code, system, rng, *, a=(A_SEG, 0x0100), b=(B_SEG, 0x0200), p=(P_SEG, 0x0300), f9f=1, f6c=1, name=None, table=None,
          count=0, sp=0x8000, ss=SS, flags=0x7202, patch=(), background=None):
    memory = bytearray([0xA5]) * SIZE if background is None else bytearray(background)
    at = physical(CALLER_CS, 0); memory[at:at + len(code)] = code
    for offset, value in patch:
        memory[physical(CALLER_CS, offset)] = value
    at = physical(SYSTEM_CS, STRCMP); memory[at:at + len(system)] = system
    name = pstring(rng) if name is None else name
    table = [variant(rng, name) for _ in range(LAST)] if table is None else table
    put16(memory, a[0], (a[1] + 0x108) & 0xFFFF, b[1]); put16(memory, a[0], (a[1] + 0x10A) & 0xFFFF, b[0])
    put16(memory, b[0], (b[1] + 0x0A) & 0xFFFF, p[1]); put16(memory, b[0], (b[1] + 0x0C) & 0xFFFF, p[0])
    for i, byte in enumerate(name):
        memory[physical(p[0], (p[1] + i) & 0xFFFF)] = byte
    memory[physical(p[0], (p[1] + 0x9F) & 0xFFFF)] = f9f
    memory[physical(p[0], (p[1] + 0x6C) & 0xFFFF)] = f6c
    for i, entry in enumerate(table):
        for j, byte in enumerate(entry):
            memory[physical(DS, TABLE + 16 * (i + 1) + j)] = byte
    memory[physical(DS, 0x6822)] = count
    frame = physical(ss, sp)
    for i, value in enumerate((RET_IP, RET_CS, *ARGS, a[1], a[0])):
        put16(memory, ss, (sp + 2 * i) & 0xFFFF, value)
        put16(memory, 0, (frame + 2 * i) & (SIZE - 1), value)
    cpu = Cpu(*(rng.randrange(0x10000) for _ in range(7)), sp, CALLER_CS, ENTRY, DS, rng.randrange(0x10000), ss, flags)
    return memory, cpu, ss >= 0xF000


def compare(label, memory, cpu, high, target, function, cap):
    from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
    original = Uc(UC_ARCH_X86, UC_MODE_16)
    original.mem_map(0, 2 * SIZE)
    original.mem_write(0, bytes(memory)); original.mem_write(SIZE, bytes(memory))
    for (name, _), register in zip(Cpu._fields_, REGISTERS):
        original.reg_write(register, getattr(cpu, name))
    start, end = physical(CALLER_CS, ENTRY), physical(CALLER_CS, END)
    legal = [(physical(SYSTEM_CS, STRCMP), physical(SYSTEM_CS, STRCMP_END))]
    state = dict(stop=None, returned=False, previous=None, loopbacks=0, illegal=None)
    visited, edges = set(), set()

    def hook(emulator, address, size, user):
        del size, user
        if state["returned"]:
            state["stop"] = OK
            return emulator.emu_stop()
        linear = address & (2 * SIZE - 1)
        if start <= linear < end:
            here = linear - start + ENTRY
            if here == LOOP_HEAD and state["previous"] == LOOP_JNE:
                state["loopbacks"] += 1
                if state["loopbacks"] > cap:
                    state["stop"] = LIMIT
                    return emulator.emu_stop()
            if here == SITE and bytes(emulator.mem_read(address, 3)) != b"\x9a\x10\x0c":
                state["stop"] = CODE
                return emulator.emu_stop()
            visited.add(here)
            if state["previous"] in target["conditional"] and state["previous"] != here:
                edges.add((state["previous"], here))
            state["previous"] = here
            state["returned"] = here == BODY_RETF
            return None
        state["previous"] = None
        if not any(lo <= linear < hi for lo, hi in legal):
            state["illegal"] = linear
            return emulator.emu_stop()
        return None

    def mirror(emulator, access, address, size, value, user):
        del access, user
        for byte in range(size):
            raw = address + byte; at = raw & (SIZE - 1)
            emulator.mem_write(at + SIZE if raw < SIZE else at, bytes([(value >> (8 * byte)) & 0xFF]))

    original.hook_add(UC_HOOK_CODE, hook)
    if high:
        original.hook_add(UC_HOOK_MEM_WRITE, mirror)
    original.emu_start(start, 2 * SIZE, count=200000)
    check(state["illegal"] is None and state["stop"] is not None, f"original did not stop cleanly ({state['illegal']}): {label}")
    expected = bytes(original.mem_read(0, SIZE))
    ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
    callee = ctypes.c_int(99)
    status = function(ram, SIZE, ctypes.byref(cpu), cap, ctypes.byref(callee))
    check(status == state["stop"], f"C status {NAMES.get(status, status)} ({callee.value}), original {NAMES[state['stop']]}: {label}")
    actual = bytes(ram)
    if actual != expected:
        differences = [i for i, (x, y) in enumerate(zip(actual, expected)) if x != y]
        raise ValueError(f"RAM mismatch {label}: {[(hex(d), actual[d], expected[d]) for d in differences[:6]]} ({len(differences)} bytes)")
    for (name, _), register in zip(Cpu._fields_, REGISTERS):
        mine, want = getattr(cpu, name), original.reg_read(register) & 0xFFFF
        check(mine == want, f"{name} mismatch {label}: C={mine:04x}, original={want:04x} ({NAMES[state['stop']]})")
    return state["stop"], visited, edges


def cases(rng):
    spec = []
    for i in range(150):
        spec.append((f"random-{i}", dict(a=(A_SEG, rng.choice((0x0000, 0xFF00, rng.randrange(0, 0xE000, 2)))),
                                         b=(B_SEG, rng.choice((0x0000, 0xFFF0, rng.randrange(0, 0xE000, 2)))),
                                         p=(P_SEG, rng.choice((0x0000, rng.randrange(0, 0xE000)))),
                                         f9f=rng.choice((1, 1, 1, 1, 0, 2, 0xFF)), f6c=rng.choice((1, 1, 1, 1, 0, 0x81)),
                                         count=rng.choice((0, 0x7F, 0xFF, rng.randrange(256))),
                                         flags=FLAGS[i % len(FLAGS)], sp=rng.randrange(0x0800, 0xF000) & ~1)))
    same = pstring(rng, 6)
    spec.append(("gate-9f", dict(f9f=0, name=same, table=[same] * LAST)))
    spec.append(("gate-6c", dict(f6c=2, name=same, table=[same] * LAST)))
    spec.append(("all-match", dict(name=same, table=[same] * LAST, count=0xFE)))
    spec.append(("no-match", dict(name=same, table=[pstring(rng, 5) for _ in range(LAST)])))
    spec.append(("empty-names", dict(name=b"\x00", table=[b"\x00", b"\x01A", b"\x00", b"\x02AB"] * (LAST // 4))))
    spec.append(("cap-zero", dict(name=same, table=[same] * LAST, cap=0)))
    spec.append(("cap-two", dict(name=same, table=[same] * LAST, cap=2)))
    spec.append(("code-site", dict(name=same, table=[same] * LAST, patch=((SITE + 1, 0x11),))))
    spec.append(("ss-a20", dict(name=same, table=[same, b"\x00", same, b"\x01Z"] * (LAST // 4), sp=0x0800, ss=0xFFFF)))
    spec.append(("df-set", dict(name=same, table=[same] * LAST, flags=0x0603)))
    spec.append(("random-background", dict(name=same, table=[same, b"\x01A", same, b"\x00"] * (LAST // 4),
                                               background=bytes(random.Random(int("38" + ROUTINE, 16)).randbytes(SIZE)))))
    return spec


def differential(target, code, system, function):
    rng = random.Random(int("38" + ROUTINE, 16))
    visited, edges, stops = set(), set(), {k: 0 for k in NAMES}
    for label, options in cases(rng):
        cap = options.pop("cap", 64)
        memory, cpu, high = build(code, system, rng, **options)
        stop, v, e = compare(f"+{ROUTINE} {label}", memory, cpu, high, target, function, cap)
        if not label.startswith("random"):
            print(f"  {label}: {NAMES[stop]}")
        stops[stop] += 1
        visited |= v; edges |= e
    reachable = {(s, o) for s, outs in target["conditional"].items() for o in outs}
    missing = sorted(hex(x) for x in target["starts"] if x not in visited)
    print(f"U38 +{ROUTINE}: {sum(stops.values())} original-byte cases ({', '.join(f'{v} {NAMES[k]}' for k, v in stops.items())}); full 1 MiB RAM "
          f"and all 14 CPU words equal, no mask; {len(visited)}/{len(target['starts'])} instruction starts, {len(edges & reachable)}/{len(reachable)} "
          f"conditional outcomes")
    check(not missing and reachable <= edges, f"coverage incomplete: {missing} {sorted(reachable - edges)}")
    check(stops[OK] and stops[CODE] and stops[LIMIT], f"a stop kind was not exercised: {stops}")
    return dict(stops={NAMES[k]: v for k, v in stops.items()}, starts=len(visited), outcomes=len(edges & reachable))


# ---- DOSBox-X synthetic oracle ---------------------------------------------------------------------------
# The LCALL names paragraph 0AF8 directly: with the COM at segment 0814, 0AF8:0C10 is COM offset 3A50h, so the SYSTEM
# bytes are placed there in the COM image and nothing else may use it. The routine span is copied to its own offsets.
CALLER_N, DATA_N = 0x8800, 0x5000
MAIN_AT, SYSTEM_AT, UNIT_AT, CASES_AT = 0x0300, 0x3A50, 0x3B00, 0x3C00
OUTPUT, HEADER, STACK_TOP = 0xC000, 0xD800, 0xF000
WINDOW_LOW, WINDOW_SIZE = STACK_TOP - 0x100, 0x100
NATIVE_CASES, CASE_SIZE = 32, 0x180
RECORD_BYTES = 28 + WINDOW_SIZE + 0x10000
NATIVE_FLAGS = (0x0002, 0x00D7, 0x08D7, 0x0402, 0x0893, 0x0003)
ENTRY_REGS = dict(ax=0x1234, bx=0x3456, cx=0x5678, dx=0x789A, si=0x9ABC, di=0xBCDE, bp=0x7777)


def native_world(rng):
    data = bytearray([0xA5]) * 0x10000
    def w16(offset, value):
        data[offset & 0xFFFF] = value & 0xFF; data[(offset + 1) & 0xFFFF] = value >> 8
    a, b, p = rng.randrange(0x8000, 0x8F00, 2), rng.randrange(0x9000, 0x9F00, 2), rng.randrange(0xA000, 0xAF00)
    w16(a + 0x108, b); w16(a + 0x10A, DATA_N); w16(b + 0x0A, p); w16(b + 0x0C, DATA_N)
    name = pstring(rng)
    data[p:p + len(name)] = name
    data[p + 0x9F] = rng.choice((1, 1, 1, 0)); data[p + 0x6C] = rng.choice((1, 1, 1, 2))
    for i in range(LAST):
        entry = variant(rng, name)
        data[TABLE + 16 * (i + 1):TABLE + 16 * (i + 1) + len(entry)] = entry
    data[0x6822] = rng.choice((0, 0xFF, rng.randrange(256)))
    return data, a


def native_memory(span, system, data):
    memory = bytearray(SIZE)
    at = physical(CALLER_N, ENTRY); memory[at:at + len(span)] = span
    at = physical(SYSTEM_CS, STRCMP); memory[at:at + len(system)] = system
    at = physical(DATA_N, 0); memory[at:at + 0x10000] = data
    return memory


def native_frame(memory, segment, returned, a):
    sp = STACK_TOP
    for value in (DATA_N, a, *reversed(ARGS), segment, returned):
        sp -= 2; put16(memory, segment, sp, value)
    return sp


def native_audit(code, system, function):
    import os
    directory = ROOT / f"resources/oracle-runs/u38-{ROUTINE}-synthetic"
    directory.mkdir(parents=True, exist_ok=True)
    span = bytes(code[ENTRY:END])
    check(physical(0x0814, SYSTEM_AT) == physical(SYSTEM_CS, STRCMP), "the SYSTEM hole is not at 0AF8:0C10")
    rng = random.Random(int("38" + ROUTINE + "D", 16))
    cases = []
    while len(cases) < NATIVE_CASES:
        (data, a), flags = native_world(rng), len(cases) % len(NATIVE_FLAGS)
        writes = [(o, data[o] | (data[o + 1] << 8)) for o in range(0, 0x10000, 2) if data[o] != 0xA5 or data[o + 1] != 0xA5]
        check(6 + 4 * len(writes) <= CASE_SIZE, f"native case needs {len(writes)} writes")
        cases.append(dict(flags=flags, data=data, a=a, writes=writes))
    table = b"".join((struct.pack("<BBHH", c["flags"], 0, len(c["writes"]), c["a"]) + b"".join(struct.pack("<HH", o, v) for o, v in c["writes"]))
                     .ljust(CASE_SIZE, b"\0") for c in cases)
    check(MAIN_AT < SYSTEM_AT and SYSTEM_AT + len(system) <= UNIT_AT and UNIT_AT + len(span) <= CASES_AT
          and CASES_AT + len(table) <= OUTPUT, "COM layout overlaps")
    t = [".code16", ".intel_syntax noprefix", ".global _start", "_start:", "    jmp main", f".org {MAIN_AT - 0x100:#x}", "main:",
         "    mov ax,cs", "    mov ds,ax", "    mov es,ax", "    mov ss,ax", f"    mov sp,{STACK_TOP:#x}", "    cld",
         f"    mov ax,{CALLER_N:#x}", "    mov es,ax", "    mov si,offset unit", f"    mov di,{ENTRY:#x}", f"    mov cx,{len(span):#x}", "    rep movsb",
         "    push cs", "    pop es", f"    mov word ptr [{HEADER:#x}],cs", f"    mov word ptr [{HEADER + 2:#x}],offset returned",
         f"    mov word ptr [{HEADER + 4:#x}],{len(cases)}", f"    mov word ptr [{HEADER + 6:#x}],0",
         "    mov dx,offset filename", "    xor cx,cx", "    mov ah,0x3c", "    int 0x21", "    jc failure",
         "    mov word ptr [handle],ax", "    mov bx,ax", f"    mov dx,{HEADER:#x}", "    mov cx,8", "    mov ah,0x40", "    int 0x21", "    jc failure",
         "again:", "    cli", f"    mov sp,{STACK_TOP:#x}", "    cld", "    push cs", "    pop ds", "    push cs", "    pop es",
         "    mov al,0xa5", f"    mov di,{WINDOW_LOW:#x}", f"    mov cx,{WINDOW_SIZE:#x}", "    rep stosb",
         "    mov ax,word ptr [index]", f"    mov bx,{CASE_SIZE}", "    mul bx", "    add ax,offset cases", "    mov bx,ax",
         f"    mov ax,{DATA_N:#x}", "    mov es,ax", "    xor di,di", "    mov cx,0x8000", "    mov ax,0xa5a5", "    rep stosw",
         "    mov cx,word ptr [bx+2]", "    jcxz written", "    lea si,[bx+6]", "write_loop:", "    mov di,word ptr [si]", "    mov ax,word ptr [si+2]",
         "    mov word ptr es:[di],ax", "    add si,4", "    loop write_loop", "written:",
         f"    mov ax,{DATA_N:#x}", "    push ax", "    push word ptr [bx+4]"]
    for value in reversed(ARGS):
        t += [f"    mov ax,{value:#x}", "    push ax"]
    t += ["    xor ah,ah", "    mov al,byte ptr [bx]", "    shl ax,1", "    mov si,ax",
          "    push word ptr [flags_table+si]", "    push cs", "    pop es", f"    mov ax,{DATA_N:#x}", "    mov ds,ax"]
    for reg, value in ENTRY_REGS.items():
        t.append(f"    mov {reg},{value:#x}")
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
          'filename: .asciz "RESULT.BIN"', "handle: .word 0", "index: .word 0", f"target_far: .word {ENTRY:#x}, {CALLER_N:#x}",
          "flags_table: .word " + ",".join(f"{f:#x}" for f in NATIVE_FLAGS)]
    for label, blob, where in (("system", system, SYSTEM_AT), ("unit", span, UNIT_AT), ("cases", table, CASES_AT)):
        t += [f".org {where - 0x100:#x}", f"{label}:"]
        for i in range(0, len(blob), 16):
            t.append("    .byte " + ",".join(f"{x:#x}" for x in blob[i:i + 16]))
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
    segment, returned, count, _ = struct.unpack_from("<4H", result)
    check(count == len(cases) and segment == 0x0814, f"native header differs or COM segment {segment:04x} is not 0814")
    matched = 0
    for n, case in enumerate(cases):
        record = result[8 + n * RECORD_BYTES:8 + (n + 1) * RECORD_BYTES]
        memory = native_memory(span, system, case["data"])
        for i in range(WINDOW_SIZE):
            memory[physical(segment, WINDOW_LOW + i)] = 0xA5
        sp = native_frame(memory, segment, returned, case["a"])
        cpu = Cpu(*ENTRY_REGS.values(), sp, CALLER_N, ENTRY, DATA_N, segment, segment, NATIVE_FLAGS[case["flags"]])
        ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
        check(function(ram, SIZE, ctypes.byref(cpu), 64, None) == OK, f"C status on native case {n}")
        for (reg, _), want in zip(Cpu._fields_, struct.unpack_from("<14H", record)):
            check(getattr(cpu, reg) == want, f"native case {n} {reg}: C={getattr(cpu, reg):04x}, DOS={want:04x}")
        got = bytes(ram[physical(segment, WINDOW_LOW):physical(segment, WINDOW_LOW) + WINDOW_SIZE]) + bytes(ram[physical(DATA_N, 0):physical(DATA_N, 0) + 0x10000])
        if got != record[28:]:
            differences = [i for i, (x, y) in enumerate(zip(got, record[28:])) if x != y]
            raise ValueError(f"native case {n} bytes differ at {[hex(d) for d in differences[:8]]} ({len(differences)} bytes)")
        matched += ram[physical(DATA_N, 0x6822)] != case["data"][0x6822]
    digest = sha256(result).hexdigest()
    print(f"DOSBox-X normal-core/386 synthetic cases passed: {len(cases)} ({matched} with a match); all 14 CPU words, the stack window and the "
          f"whole 64 KiB data segment; no mask")
    print(f"native result SHA-256 {digest}; COM segment {segment:04x}; SYSTEM at 0AF8:0C10 (COM 3A50h), U38 span at {CALLER_N:04X}:{ENTRY:04X}")
    return dict(cases=len(cases), matched=matched, result_sha256=digest)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--unicorn", action="store_true")
    parser.add_argument("--native", action="store_true")
    parser.add_argument("--output", type=Path)
    parser.add_argument("--routine", choices=sorted(ROUTINES), default="0679")
    args = parser.parse_args()
    select(args.routine)
    target, code, system = source()
    report = dict(routine=ROUTINE, target=dict(sha256=target["sha"], bytes=END - ENTRY, instructions=len(target["starts"]), file_offset=target["file_offset"]))
    if args.unicorn or args.native:
        with tempfile.TemporaryDirectory(prefix="poolrad-u38match-") as temp:
            function = library(Path(temp))
            if args.unicorn:
                report["unicorn"] = differential(target, code, system, function)
            if args.native:
                report["native"] = native_audit(code, system, function)
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(json.dumps(report, sort_keys=True, indent=2) + "\n")
    print(json.dumps(report, sort_keys=True))


if __name__ == "__main__":
    main()
