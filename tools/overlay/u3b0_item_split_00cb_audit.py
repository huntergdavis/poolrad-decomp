#!/usr/bin/env python3
"""Original-byte full-state parity for raw-CPU U3B0 +00CB with SYSTEM string store, FillChar and set membership composed, plus DOSBox-X.

The differential places the hash-checked expanded START.EXE image at load segment 0, so the original string
store (0AF8:0B25), FillChar (0AF8:16A8) and set membership test (0AF8:0DC0) run live from +00CB's three
unrelocated, fixup-backed far calls. It loads unit 0x3B0 (including the 32-byte set constant at +00AB) and
compares the entire 1 MiB RAM and all 14 CPU words between the original under Unicorn and the C model, with no
mask. The original decides each stop: its return (wherever RETF 6 goes), a rewritten call site (CODE, read live)
or a callee return elsewhere (TRANSFER). Parent links that put the pairs on the scan's own locals show the scan
still ends. Media stays private.
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
from overlay_inventory import find_units, parse_mz
from resident_loader_map import unpack
from u86_procvars_3413_audit import Cpu, ROOT, SIZE, EXE_SHA, OVR_SHA, REGISTERS, physical, put16

ENTRY, CODE_END, LISTED_END, BODY_RETF = 0x00CB, 0x01BA, 0x01DA, 0x01B7
SITES = {0x00DF: "store", 0x00F4: "fill", 0x012B: "set"}
LOOP_HEAD, LOOP_JNE = 0x0118, 0x0193
SYSTEM_CS = 0x0AF8
LEAVES = dict(store=(0x0B25, 0x0B49, "009955722e8f73d0a0f7076aa418d141f642c5a729b2a58dfa7a7a3922989118"),
              fill=(0x16A8, 0x16BC, "f1c5fade96970aa1dd1a9ffc2d876c7aea57bef8680d929d9f65d6c5190e17f7"),
              set=(0x0DC0, 0x0DE1, "753159dded0f3f45bb0352cb9483a15271d85888b6c1b127f8631879473ec62c"))
CALLER_CS, SS, EDGE_SS = 0x8800, 0x2800, 0x2000
RET_CS, RET_IP = 0x9001, 0x7000
OK, CODE, TRANSFER = 0, 2, 3
NAMES = {OK: "return", CODE: "CODE", TRANSFER: "TRANSFER"}
ALPHABET = b"ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 abcdefghijklmnopqrstuvwxyz-.,'/\x80\xc1\xff"


def check(condition, message):
    if not condition:
        raise ValueError(message)


def conditionals(decoded):
    return {i.address: (i.address + i.size, int(i.op_str, 16)) for i in decoded if i.mnemonic.startswith("j") and i.mnemonic != "jmp"}


def source():
    exe = (ROOT / "resources/DOS/POOLRAD/START.EXE").read_bytes()
    ovr = (ROOT / "resources/DOS/POOLRAD/GAME.OVR").read_bytes()
    check(len(exe) == 65231 and sha256(exe).hexdigest() == EXE_SHA, "START.EXE provenance differs")
    check(len(ovr) == 206105 and sha256(ovr).hexdigest() == OVR_SHA, "GAME.OVR provenance differs")
    image = bytes(unpack(exe)[0])
    leaves = {}
    for name, (start, end, sha) in LEAVES.items():
        body = image[SYSTEM_CS * 16 + start:SYSTEM_CS * 16 + end]
        check(sha256(body).hexdigest() == sha, f"SYSTEM {name} bytes differ")
        decoded = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(body, start))
        check(decoded[-1].mnemonic == "retf" and sum(i.size for i in decoded) == len(body), f"SYSTEM {name} extent differs")
        leaves[name] = dict(start=start, end=end, retf=decoded[-1].address, body=body, decoded=decoded, conditional=conditionals(decoded))
    unit = {u["stub_para"]: u for u in find_units(exe[parse_mz(exe)["header_bytes"]:], ovr)}[0x3B0]
    code = ovr[unit["ovr_offset"]:unit["ovr_offset"] + unit["code_size"]]
    starts = sorted(f["rel"] for f in unit["functions"])
    check(LISTED_END == next(s for s in starts if s > ENTRY), "U3B0 +00CB listed bounds differ")
    body = code[ENTRY:CODE_END]
    decoded = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(body, ENTRY))
    calls = sorted((i.address, i.op_str) for i in decoded if "call" in i.mnemonic)
    fixups = set(unit["fixups"])
    check(sum(i.size for i in decoded) == len(body) == 239 and len(decoded) == 93 and decoded[-1].address == BODY_RETF
          and decoded[-1].op_str == "6" and calls == [(0x00DF, "0xaf8, 0xb25"), (0x00F4, "0xaf8, 0x16a8"), (0x012B, "0xaf8, 0xdc0")]
          and all(s + 3 in fixups for s, _ in calls), f"U3B0 +00CB extent or calls differ: {calls}")
    conditional = conditionals(decoded)
    target = dict(body=body, decoded=decoded, conditional=conditional, sha=sha256(body).hexdigest(), file_offset=unit["ovr_offset"] + ENTRY,
                  leaves=leaves)
    print(f"U3B0 +00CB ({len(body)}/{len(decoded)} code bytes/insns; +01BA..+01D9 are not code; LCALL 0AF8:0B25, 0AF8:16A8, 0AF8:0DC0, "
          f"RETF 6) GAME file {target['file_offset']:#x} sha {target['sha']}; {len(conditional)} conditionals")
    return target, code, image


def library(directory):
    path = directory / "u3b0i.so"
    subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-O2", "-fPIC", "-shared", "-I", str(ROOT / "include"),
                    str(ROOT / "src/recovered/overlay_u3b0_item_split_00cb_cpu.c"), str(ROOT / "src/recovered/system_runtime_leaves_cpu.c"),
                    str(ROOT / "src/recovered/system_strstore_fullstate_0b25.c"), "-o", str(path)], check=True, capture_output=True, text=True)
    function = ctypes.CDLL(str(path)).pr_u3b0_item_split_00cb_cpu
    function.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t, ctypes.POINTER(Cpu), ctypes.POINTER(ctypes.c_int)]
    function.restype = ctypes.c_int
    return function


def text_for(rng, length=None, alphabet=ALPHABET):
    length = rng.randrange(0, 56) if length is None else length
    return length, [rng.choice(alphabet) for _ in range(length)]


def build(image, code, rng, length, chars, *, sp=0x8000, ss=SS, src=None, link=None, flags=0x7202, background=None, patch=(), base=0x0000,
          pairs=None, caller=CALLER_CS):
    memory = bytearray([0xA5]) * SIZE if background is None else bytearray(background)
    memory[0:len(image)] = image
    start = physical(caller, base)
    memory[start:start + len(code)] = code
    for offset, value in patch:
        memory[physical(caller, (base + offset) & 0xFFFF)] = value
    src = src or (0x4000 + rng.randrange(0x2000), rng.randrange(0x10000))
    link = ((sp + 0x100) & 0xFFFF) if link is None else link
    if pairs is not None:                                            # stale parent bytes where the pairs land
        for i in range(0x60):
            memory[physical(ss, (link - 0x8F + i) & 0xFFFF)] = pairs
    memory[physical(src[0], src[1])] = length
    for i, value in enumerate(chars):
        memory[physical(src[0], (src[1] + 1 + i) & 0xFFFF)] = value
    frame = physical(ss, sp)
    for i, value in enumerate([RET_IP, RET_CS, link, src[1], src[0]]):   # both views agree at a segment end
        put16(memory, ss, (sp + 2 * i) & 0xFFFF, value)
        put16(memory, 0, (frame + 2 * i) & (SIZE - 1), value)
    reg = tuple(rng.randrange(0x10000) for _ in range(7))
    high = any(seg >= 0xF000 for seg in (ss, src[0]))
    return memory, Cpu(*reg, sp, caller, (base + ENTRY) & 0xFFFF, rng.randrange(0x10000), rng.randrange(0x10000), ss, flags), high


def compare(label, memory, cpu, high, target, function, base=0x0000, budget=400000):
    from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
    original = Uc(UC_ARCH_X86, UC_MODE_16)
    original.mem_map(0, 2 * SIZE)
    original.mem_write(0, bytes(memory))
    original.mem_write(SIZE, bytes(memory))
    for (name, _), register in zip(Cpu._fields_, REGISTERS):
        original.reg_write(register, getattr(cpu, name))
    origin = physical(cpu.cs, base)
    body = (origin + ENTRY, origin + CODE_END)
    ranges = {name: (physical(SYSTEM_CS, leaf["start"]), physical(SYSTEM_CS, leaf["end"])) for name, leaf in target["leaves"].items()}
    sites = {origin + s: (origin + s + 5, bytes([0x9A]) + struct.pack("<H", target["leaves"][name]["start"])) for s, name in SITES.items()}
    state = dict(stop=None, expected=None, check=None, returning=False, previous=None, last=None, loops=0)
    seen = {k: set() for k in ("body", *LEAVES)}
    edges = {k: set() for k in seen}

    def note(kind, here, conditional):
        seen[kind].add(here)
        if state["previous"] is not None and state["previous"][0] == kind and state["previous"][1] in conditional and state["previous"][1] != here:
            edges[kind].add((state["previous"][1], here))
        state["previous"] = (kind, here)

    def halt(emulator, stop):
        state["stop"] = stop
        emulator.emu_stop()

    def hook(emulator, address, size, user):
        del size, user
        last, state["last"] = state["last"], address
        if address == origin + LOOP_HEAD and last == origin + LOOP_JNE:
            state["loops"] += 1
            check(state["loops"] <= 254, f"the scan ran past 254 loop-backs: {label}")
        if state["returning"]:
            return halt(emulator, OK)
        if state["check"] is not None:
            expected, state["check"] = state["check"], None
            if address != expected:
                return halt(emulator, TRANSFER)
        if address in sites:
            if bytes(emulator.mem_read(address, 3)) != sites[address][1]:
                return halt(emulator, CODE)
            state["expected"] = sites[address][0]
        if body[0] <= address < body[1]:
            note("body", address - origin, target["conditional"])
            if address - origin == BODY_RETF:
                state["returning"] = True
            return None
        for name, (start, end) in ranges.items():
            if start <= address < end:
                leaf = target["leaves"][name]
                here = address - start + leaf["start"]
                note(name, here, leaf["conditional"])
                if here == leaf["retf"]:
                    state["check"] = state["expected"]
                return None
        raise ValueError(f"unexpected fetch at {address:#x}: {label}")

    def mirror(emulator, access, address, size, value, user):
        del access, user
        for byte in range(size):
            raw = address + byte
            at = raw & (SIZE - 1)
            emulator.mem_write(at + SIZE if raw < SIZE else at, bytes([(value >> (8 * byte)) & 0xFF]))

    original.hook_add(UC_HOOK_CODE, hook)
    if high:
        original.hook_add(UC_HOOK_MEM_WRITE, mirror)
    original.emu_start(body[0], 2 * SIZE, count=budget)
    check(state["stop"] is not None, f"original did not stop: {label}")
    expected = bytes(original.mem_read(0, SIZE))
    ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
    callee = ctypes.c_int(99)
    status = function(ram, SIZE, ctypes.byref(cpu), ctypes.byref(callee))
    check(status == state["stop"], f"C status {status}, original stopped with {NAMES[state['stop']]}: {label}")
    actual = bytes(ram)
    if actual != expected:
        differences = [i for i, (a, b) in enumerate(zip(actual, expected)) if a != b]
        raise ValueError(f"RAM mismatch {label}: {[(hex(d), actual[d], expected[d]) for d in differences[:6]]} ({len(differences)} bytes)")
    for (name, _), register in zip(Cpu._fields_, REGISTERS):
        mine, want = getattr(cpu, name), original.reg_read(register) & 0xFFFF
        check(mine == want, f"{name} mismatch {label}: C={mine:04x}, original={want:04x}")
    return (state["stop"], state["loops"]), seen, edges


def cases(rng):
    spec = []
    for i in range(150):
        length, chars = text_for(rng)
        spec.append((f"random-{i}", dict(length=length, chars=chars, sp=rng.randrange(0x0400, 0xEE00) & ~1,
                                          flags=(0x7202, 0x0602, 0x0ED7, 0xF2D7)[i % 4], pairs=rng.choice((None, 0, 0xFF)))))
    for name, text in (("empty", b""), ("no-member", b"no capitals here"), ("first", b"Alpha"), ("adjacent", b"AB"),
                       ("words", b" Gold Gems Jewelry 7"), ("all-forty", b"Q" * 40), ("truncated", b"Q R S T U V W X Y Z 0 1 2 3 4 5 6 7 8 9 A B"),
                       ("long-255", bytes(rng.choice(ALPHABET) for _ in range(255)))):
        spec.append((f"text-{name}", dict(length=len(text), chars=list(text))))
    # pairs that climb through the call frames onto i, k, n and the bound: the scan still ends
    sp = 0x8000
    for delta in (0x27, 0x28, 0x25, 0x26, 0x21):
        for caller in (CALLER_CS, 0x8811):
            spec.append((f"pairs-on-locals-{delta:02x}-{caller:04x}", dict(length=40, chars=list(b"Q" * 40), link=(sp - 2 + delta) & 0xFFFF,
                                                                            caller=caller)))
    spec.append(("pairs-on-locals-sparse", dict(length=40, chars=list(b"Q q " * 10), link=(sp - 2 + 0x27) & 0xFFFF)))
    for delta in range(0x00, 0x61):                                  # every parent link from BP to BP + 60h, both caller CS kinds
        for caller in (CALLER_CS, 0x8811):
            spec.append((f"sweep-{delta:02x}-{caller:04x}", dict(length=40, chars=list(b"Q" * 40), link=(sp - 2 + delta) & 0xFFFF,
                                                                  caller=caller)))
    spec.append(("fill-own-frame", dict(length=5, chars=list(b"ABCDE"), link=(sp - 2 + 0x53) & 0xFFFF)))     # TRANSFER at FillChar's RETF
    spec.append(("fill-local-copy", dict(length=12, chars=list(b"Gold Gems Ax"), link=(sp - 2 + 0x8D - 0x29) & 0xFFFF)))
    spec.append(("fill-own-link", dict(length=6, chars=list(b"A B C "), link=(sp - 2 + 0x8D - 0x20) & 0xFFFF)))
    for k, sp in enumerate((0x0040, 0x0060, 0xFFF0, 0xFFF8, 0xFFFE)):
        length, chars = text_for(rng, 20)
        spec.append((f"sp-{sp:04x}", dict(length=length, chars=chars, sp=sp, ss=EDGE_SS)))
    length, chars = text_for(rng, 30)
    spec.append(("ss-a20", dict(length=length, chars=chars, sp=0x0400, ss=0xFFFF)))
    spec.append(("src-a20", dict(length=length, chars=chars, src=(0xFFFF, 0x0100))))
    spec.append(("src-wrap", dict(length=length, chars=chars, src=(0x4000, 0xFFF0))))
    spec.append(("random-background", dict(length=length, chars=chars, background=bytes(random.Random(0x3B000CB).randbytes(SIZE)))))
    spec.append(("relocated-entry", dict(length=length, chars=chars, base=0x1000)))
    for k, (site, offset, value) in enumerate(((0x00DF, 0, 0x90), (0x00F4, 1, 0x41), (0x012B, 2, 0x0E), (0x012B, 0, 0xCB))):
        length, chars = text_for(rng, 9)
        spec.append((f"code-{site:04x}-{offset}", dict(length=length, chars=chars, patch=((site + offset, value),), flags=(0x7202, 0x0ED7)[k & 1])))
    return spec


def differential(target, code, image, function):
    rng = random.Random(0x3B000CB)
    seen = {k: set() for k in ("body", *LEAVES)}
    edges = {k: set() for k in seen}
    stops = {k: 0 for k in NAMES}
    most = 0
    for label, options in cases(rng):
        base = options.get("base", 0)
        length, chars = options.pop("length"), options.pop("chars")
        memory, cpu, high = build(image, code, rng, length, chars, **options)
        (stop, loops), s, e = compare(f"+00CB {label}", memory, cpu, high, target, function, base)
        stops[stop] += 1
        most = max(most, loops)
        for kind in seen:
            seen[kind] |= s[kind]
            edges[kind] |= e[kind]
    report = {}
    for kind in seen:
        decoded, conditional = (target["decoded"], target["conditional"]) if kind == "body" else \
            (target["leaves"][kind]["decoded"], target["leaves"][kind]["conditional"])
        missing = sorted(hex(i.address) for i in decoded if i.address not in seen[kind])
        reachable = {(s, o) for s, outs in conditional.items() for o in outs}
        missing_edges = sorted((hex(s), hex(o)) for s, o in reachable - edges[kind])
        check(not missing and not missing_edges, f"{kind} coverage incomplete: {missing} {missing_edges}")
        report[kind] = dict(starts=len(seen[kind]), instructions=len(decoded), outcomes=len(edges[kind]), reachable_outcomes=len(reachable))
        print(f"  {kind}: {len(seen[kind])}/{len(decoded)} instruction starts, {len(edges[kind])}/{len(reachable)} conditional outcomes")
    check(all(stops[k] for k in (CODE, TRANSFER)), f"a stop kind was not exercised: {stops}")
    print(f"U3B0 +00CB: {sum(stops.values())} original-byte cases passed ({stops[OK]} returns, {stops[CODE]} CODE, {stops[TRANSFER]} TRANSFER); "
          f"full 1 MiB RAM and all 14 CPU words equal; at most {most} scan loop-backs in any case")
    report["stops"] = {NAMES[k]: v for k, v in stops.items()}
    report["most_loopbacks"] = most
    return report


# ---- DOSBox-X synthetic oracle -------------------------------------------------------------------------
CALLER_N, SYS_N, SRC_SEG = 0x8800, 0x9000, 0x2000
MAIN_AT, STORE_BLOB, FILL_BLOB, SET_BLOB, UNIT_BLOB, CASES_AT = 0x0300, 0x0E00, 0x0E40, 0x0E80, 0x1000, 0x3000
OUTPUT, HEADER, STACK_TOP = 0xC000, 0xC800, 0xF000
FRAME_LOW, FRAME_SIZE, LINK = 0xEF80, 0x100, 0xF0A0
TEXT_SIZE = 60
CASE_SIZE = 2 + 1 + TEXT_SIZE + 1
RECORD_BYTES = 28 + FRAME_SIZE
NATIVE_FLAGS = (0x0002, 0x00D7, 0x08D7, 0x0402, 0x0893, 0x0003)
ENTRY_REGS = dict(ax=0x1234, bx=0x3456, cx=0x5678, dx=0x789A, si=0x9ABC, di=0xBCDE, bp=0x7777)


def native_cases():
    rng = random.Random(0x3B00CBD)
    texts = [b"", b"no capitals here", b"Alpha", b"AB", b" Gold Gems Jewelry 7", b"Q" * 40, b"Q R S T U V W X Y Z 0 1 2 3 4 5 6 7 8 9 A B",
             bytes(rng.choice(ALPHABET) for _ in range(TEXT_SIZE))]
    for _ in range(88):
        texts.append(bytes(rng.choice(ALPHABET) for _ in range(rng.randrange(0, TEXT_SIZE + 1))))
    return [dict(text=t, flags=k % len(NATIVE_FLAGS)) for k, t in enumerate(texts)]


def native_audit(target, code, function):
    directory = ROOT / "resources/oracle-runs/u3b0-00cb-synthetic"
    directory.mkdir(parents=True, exist_ok=True)
    unit = bytearray(code)
    for site in SITES:
        struct.pack_into("<H", unit, site + 3, SYS_N)
    blobs = {name: target["leaves"][name]["body"] for name in LEAVES}
    cases = native_cases()
    text = [".code16", ".intel_syntax noprefix", ".global _start", "_start:", "    jmp main", f".org {MAIN_AT - 0x100:#x}", "main:",
            "    mov ax,cs", "    mov ds,ax", "    mov es,ax", "    mov ss,ax", f"    mov sp,{STACK_TOP:#x}", "    cld"]
    for segment, offset, label, length in ((CALLER_N, 0, "unit", len(unit)), (SYS_N, LEAVES["store"][0], "store", len(blobs["store"])),
                                           (SYS_N, LEAVES["fill"][0], "fill", len(blobs["fill"])), (SYS_N, LEAVES["set"][0], "set", len(blobs["set"]))):
        text += [f"    mov ax,{segment:#x}", "    mov es,ax", f"    mov si,offset {label}", f"    mov di,{offset:#x}", f"    mov cx,{length:#x}", "    rep movsb"]
    text += ["    mov ax,cs", "    mov es,ax",
             f"    mov word ptr [{HEADER:#x}],cs", f"    mov word ptr [{HEADER + 2:#x}],offset returned",
             f"    mov word ptr [{HEADER + 4:#x}],{len(cases)}", f"    mov word ptr [{HEADER + 6:#x}],0",
             "    mov dx,offset filename", "    xor cx,cx", "    mov ah,0x3c", "    int 0x21", "    jc failure",
             "    mov word ptr [handle],ax", "    mov bx,ax", f"    mov dx,{HEADER:#x}", "    mov cx,8", "    mov ah,0x40", "    int 0x21", "    jc failure",
             "again:", "    cli", f"    mov sp,{STACK_TOP:#x}", "    cld", "    mov ax,cs", "    mov ds,ax", "    mov es,ax",
             "    mov al,0xa5", f"    mov di,{FRAME_LOW:#x}", f"    mov cx,{FRAME_SIZE}", "    rep stosb",
             "    mov ax,word ptr [index]", f"    mov bx,{CASE_SIZE}", "    mul bx", "    add ax,offset cases", "    mov si,ax",
             f"    mov ax,{SRC_SEG:#x}", "    mov es,ax", "    xor di,di", "    add si,2", f"    mov cx,{TEXT_SIZE + 1}", "    rep movsb",
             f"    sub si,{TEXT_SIZE + 3}", "    mov ax,cs", "    mov es,ax",
             f"    mov ax,{SRC_SEG:#x}", "    push ax", "    xor ax,ax", "    push ax", f"    mov ax,{LINK:#x}", "    push ax",
             "    mov bl,byte ptr [si]", "    xor bh,bh", "    shl bx,1", "    push word ptr [flags_table+bx]", "    popf",
             "    mov ax,0x1900", "    mov ds,ax"]
    for name, value in ENTRY_REGS.items():
        text.append(f"    mov {name},{value:#x}")
    text += ["    call dword ptr cs:[target_far]", "returned:"]
    for i, reg in enumerate("ax bx cx dx si di bp sp cs".split()):
        text.append(f"    mov word ptr cs:[{OUTPUT + 2 * i:#x}],{reg}")
    text += [f"    mov word ptr cs:[{OUTPUT + 18:#x}],offset returned", f"    mov word ptr cs:[{OUTPUT + 20:#x}],ds",
             f"    mov word ptr cs:[{OUTPUT + 22:#x}],es", f"    mov word ptr cs:[{OUTPUT + 24:#x}],ss",
             f"    mov sp,{STACK_TOP + 0x800:#x}", "    pushf", "    pop ax", f"    mov word ptr cs:[{OUTPUT + 26:#x}],ax",
             "    cld", "    mov ax,cs", "    mov es,ax", "    mov ds,ax", f"    mov di,{OUTPUT + 28:#x}",
             f"    mov si,{FRAME_LOW:#x}", f"    mov cx,{FRAME_SIZE}", "    rep movsb", "    sti",
             "    mov bx,word ptr [handle]", f"    mov dx,{OUTPUT:#x}", f"    mov cx,{RECORD_BYTES}", "    mov ah,0x40", "    int 0x21", "    jc failure",
             f"    cmp ax,{RECORD_BYTES}", "    jne failure",
             "    inc word ptr [index]", "    mov ax,word ptr [index]", f"    cmp ax,{len(cases)}", "    jb again",
             "    mov bx,word ptr [handle]", "    mov ah,0x3e", "    int 0x21", "    mov ax,0x4c00", "    int 0x21",
             "failure:", "    mov ax,0x4c01", "    int 0x21",
             'filename: .asciz "RESULT.BIN"', "handle: .word 0", "index: .word 0", f"target_far: .word {ENTRY:#x}, {CALLER_N:#x}",
             "flags_table: .word " + ",".join(f"{f:#x}" for f in NATIVE_FLAGS)]
    for label, blob, at in (("store", blobs["store"], STORE_BLOB), ("fill", blobs["fill"], FILL_BLOB), ("set", blobs["set"], SET_BLOB),
                            ("unit", bytes(unit), UNIT_BLOB)):
        text += [f".org {at - 0x100:#x}", f"{label}:"]
        for i in range(0, len(blob), 16):
            text.append("    .byte " + ",".join(f"{b:#x}" for b in blob[i:i + 16]))
    text += [f".org {CASES_AT - 0x100:#x}", "cases:"]
    for case in cases:
        padded = case["text"] + bytes([0x5A]) * (TEXT_SIZE - len(case["text"]))
        row = struct.pack("<BB", case["flags"], 0) + bytes([len(case["text"])]) + padded + bytes(1)
        check(len(row) == CASE_SIZE, "case row size differs")
        text.append("    .byte " + ",".join(f"{b:#x}" for b in row))
    check(STORE_BLOB + len(blobs["store"]) <= FILL_BLOB and FILL_BLOB + len(blobs["fill"]) <= SET_BLOB
          and SET_BLOB + len(blobs["set"]) <= UNIT_BLOB and UNIT_BLOB + len(unit) <= CASES_AT
          and CASES_AT + CASE_SIZE * len(cases) <= OUTPUT, "COM layout overlaps")
    asm, obj = directory / "oracle.S", directory / "oracle.o"
    asm.write_text("\n".join(text) + "\n")
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
                       env=environment, stdout=log, stderr=subprocess.STDOUT, check=True, timeout=300)
    result = result_path.read_bytes()
    check(len(result) == 8 + len(cases) * RECORD_BYTES, f"native output incomplete: {len(result)} bytes")
    segment, returned, count, _ = struct.unpack_from("<4H", result)
    check(count == len(cases), "native header differs")
    items = {}
    for n, case in enumerate(cases):
        record = result[8 + n * RECORD_BYTES:8 + (n + 1) * RECORD_BYTES]
        memory = bytearray(SIZE)
        for seg, off, blob in ((CALLER_N, 0, bytes(unit)), (SYS_N, LEAVES["store"][0], blobs["store"]), (SYS_N, LEAVES["fill"][0], blobs["fill"]),
                               (SYS_N, LEAVES["set"][0], blobs["set"])):
            at = physical(seg, off)
            memory[at:at + len(blob)] = blob
        for i in range(FRAME_SIZE):
            memory[physical(segment, FRAME_LOW + i)] = 0xA5
        source_bytes = bytes([len(case["text"])]) + case["text"] + bytes([0x5A]) * (TEXT_SIZE - len(case["text"]))
        memory[physical(SRC_SEG, 0):physical(SRC_SEG, 0) + len(source_bytes)] = source_bytes
        sp = STACK_TOP
        for value in (SRC_SEG, 0, LINK, segment, returned):
            sp -= 2
            put16(memory, segment, sp, value)
        cpu = Cpu(*ENTRY_REGS.values(), sp, CALLER_N, ENTRY, 0x1900, segment, segment, NATIVE_FLAGS[case["flags"]])
        ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
        callee = ctypes.c_int(99)
        label = f"native case {n}"
        check(function(ram, SIZE, ctypes.byref(cpu), ctypes.byref(callee)) == 0 and callee.value == 0, f"C status on {label}")
        for (name, _), want in zip(Cpu._fields_, struct.unpack_from("<14H", record)):
            check(getattr(cpu, name) == want, f"{label} {name}: C={getattr(cpu, name):04x}, DOS={want:04x}")
        got = bytes(ram[physical(segment, FRAME_LOW):physical(segment, FRAME_LOW) + FRAME_SIZE])
        check(got == record[28:], f"{label} bytes differ")
        k = ram[physical(segment, LINK - 0x8E)]
        items[k] = items.get(k, 0) + 1
    digest = sha256(result).hexdigest()
    print(f"DOSBox-X normal-core/386 synthetic cases passed: {len(cases)} (item counts {dict(sorted(items.items()))}); all 14 CPU words and the "
          f"{FRAME_SIZE:#x}-byte window (locals, every call frame and the parent's pairs); no mask")
    print(f"native result SHA-256 {digest}; COM segment {segment:04x}; string store, FillChar and set membership copied to {SYS_N:04X}")
    return dict(cases=len(cases), items={str(k): v for k, v in sorted(items.items())}, result_sha256=digest)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--unicorn", action="store_true")
    parser.add_argument("--native", action="store_true")
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    target, code, image = source()
    report = dict(target=dict(sha256=target["sha"], bytes=len(target["body"]), instructions=len(target["decoded"]), file_offset=target["file_offset"]))
    if args.unicorn or args.native:
        with tempfile.TemporaryDirectory(prefix="poolrad-u3b0i-") as temp:
            function = library(Path(temp))
            if args.unicorn:
                report["unicorn"] = differential(target, code, image, function)
            if args.native:
                report["native"] = native_audit(target, code, function)
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(json.dumps(report, sort_keys=True, indent=2) + "\n")
    print(json.dumps(report, sort_keys=True))


if __name__ == "__main__":
    main()
