#!/usr/bin/env python3
"""Original-byte full-state parity for raw-CPU U7F +0B93 with SYSTEM string store and UpCase composed, plus DOSBox-X.

The differential places the hash-checked expanded START.EXE image at load segment 0, so the original
string store (0AF8:0B25) and UpCase (0AF8:182A) run live from +0B93's nine unrelocated, fixup-backed
far calls. It loads unit 0x7F (including the code-segment string literals at +0B5A..+0B92) and
compares the entire 1 MiB RAM and all 14 CPU words between the original under Unicorn and the C model,
with no mask. The original decides each stop: its return (wherever RETF 8 goes), a rewritten call site
(CODE), a callee return elsewhere (TRANSFER) or the space-loop cap (LIMIT). Media stays private.
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

ENTRY, CODE_END, LISTED_END = 0x0B93, 0x0CBA, 0x0CEB
STORE_SITES = (0x0BA9, 0x0C01, 0x0C16, 0x0C34, 0x0C51, 0x0C6E, 0x0C8B, 0x0CA8)
UPCASE_SITE, LOOP_HEAD, LOOP_JMP, BODY_RETF = 0x0BE6, 0x0BB3, 0x0BC6, 0x0CB7
SYSTEM_CS = 0x0AF8
STORE = (0x0B25, 0x0B49, 0x0B46, "009955722e8f73d0a0f7076aa418d141f642c5a729b2a58dfa7a7a3922989118")
UPCASE = (0x182A, 0x183D, 0x183A, "738c9a82c7faab6335f0952cc5d989d273cb85828a0735f34966220f370d012c")
CALLER_CS, SS, EDGE_SS = 0x8800, 0x2800, 0x2000
RET_CS, RET_IP = 0x9001, 0x7000
OK, CODE, TRANSFER, LIMIT = 0, 2, 3, 4
NAMES = {OK: "return", CODE: "CODE", TRANSFER: "TRANSFER", LIMIT: "LIMIT"}
LETTERS = (0x47, 0x50, 0x45, 0x53, 0x43, 0x4A)
# Every string-store call here passes cap FFh, so a length byte never exceeds it: the store's MOV AL,CL is unreachable.
UNREACHABLE = {"store": ({(0x0B39, 0x0B3B)}, {0x0B3B}), "body": (set(), set()), "upcase": (set(), set())}


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
    for name, (start, end, retf, sha) in (("store", STORE), ("upcase", UPCASE)):
        body = image[SYSTEM_CS * 16 + start:SYSTEM_CS * 16 + end]
        check(sha256(body).hexdigest() == sha, f"SYSTEM {name} bytes differ")
        leaves[name] = dict(start=start, end=end, retf=retf, body=body, decoded=list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(body, start)))
        leaves[name]["conditional"] = conditionals(leaves[name]["decoded"])
    unit = {u["stub_para"]: u for u in find_units(exe[parse_mz(exe)["header_bytes"]:], ovr)}[0x7F]
    code = ovr[unit["ovr_offset"]:unit["ovr_offset"] + unit["code_size"]]
    starts = sorted(f["rel"] for f in unit["functions"])
    check(LISTED_END == next(s for s in starts if s > ENTRY), "U7F +0B93 listed bounds differ")
    body = code[ENTRY:CODE_END]
    decoded = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(body, ENTRY))
    calls = [(i.address, i.op_str) for i in decoded if "call" in i.mnemonic]
    expected_calls = sorted([(s, "0xaf8, 0xb25") for s in STORE_SITES] + [(UPCASE_SITE, "0xaf8, 0x182a")])
    fixups = set(unit["fixups"])
    targets = [int(i.op_str, 16) for i in decoded if i.mnemonic.startswith("j")]
    check(sum(i.size for i in decoded) == len(body) == 295 and decoded[-1].mnemonic == "retf" and decoded[-1].op_str == "8"
          and decoded[-1].address == BODY_RETF and sorted(calls) == expected_calls and all(s + 3 in fixups for s, _ in calls)
          and all(ENTRY <= t < CODE_END for t in targets), f"U7F +0B93 extent or calls differ: {calls}")
    conditional = conditionals(decoded)
    target = dict(body=body, decoded=decoded, conditional=conditional, sha=sha256(body).hexdigest(), file_offset=unit["ovr_offset"] + ENTRY,
                  leaves=leaves)
    print(f"U7F +0B93 ({len(body)}/{len(decoded)} code bytes/insns; +0CBA..+0CEA are data; 8 x LCALL 0AF8:0B25, LCALL 0AF8:182A, RETF 8) "
          f"GAME file {target['file_offset']:#x} sha {target['sha']}; {len(conditional)} conditionals")
    return target, code, image


def library(directory):
    path = directory / "u7f.so"
    subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-O2", "-fPIC", "-shared", "-I", str(ROOT / "include"),
                    str(ROOT / "src/recovered/overlay_u7f_coin_type_0b93_cpu.c"), str(ROOT / "src/recovered/system_runtime_leaves_cpu.c"),
                    str(ROOT / "src/recovered/system_strstore_fullstate_0b25.c"), "-o", str(path)], check=True, capture_output=True, text=True)
    function = ctypes.CDLL(str(path)).pr_u7f_coin_type_0b93_cpu
    function.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t, ctypes.POINTER(Cpu), ctypes.c_uint, ctypes.POINTER(ctypes.c_int)]
    function.restype = ctypes.c_int
    return function


def text_for(rng, leading, letter, follow, n=None):
    chars = [0x20] * leading + [letter, follow] + [rng.randrange(0x20, 0x7F) for _ in range(40)]
    n = min(len(chars), 255) if n is None else n
    return n, chars


def build(image, code, rng, n, chars, *, sp=0x8000, ss=SS, src=None, dst=None, flags=0x7202, background=None, patch=(), stale=None):
    memory = bytearray([0xA5]) * SIZE if background is None else bytearray(background)
    memory[0:len(image)] = image
    base = physical(CALLER_CS, 0)
    memory[base:base + len(code)] = code
    for offset, value in patch:
        memory[physical(CALLER_CS, offset)] = value
    src = src or (0x4000 + rng.randrange(0x2000), rng.randrange(0x10000))
    dst = dst or (0x6000 + rng.randrange(0x1000), rng.randrange(0x10000))
    if stale is not None:                                            # the local's stale bytes, e.g. all spaces
        bp = (sp - 2) & 0xFFFF
        for i in range(0x104):
            memory[physical(ss, (bp - 0x104 + i) & 0xFFFF)] = stale
    memory[physical(src[0], src[1])] = n
    for i, value in enumerate(chars):
        memory[physical(src[0], (src[1] + 1 + i) & 0xFFFF)] = value
    frame = physical(ss, sp)
    for i, value in enumerate([RET_IP, RET_CS, dst[1], dst[0], src[1], src[0]]):   # both views agree at a segment end
        put16(memory, ss, (sp + 2 * i) & 0xFFFF, value)
        put16(memory, 0, (frame + 2 * i) & (SIZE - 1), value)
    reg = tuple(rng.randrange(0x10000) for _ in range(7))
    high = any(seg >= 0xF000 for seg in (ss, src[0], dst[0]))
    return memory, Cpu(*reg, sp, CALLER_CS, ENTRY, rng.randrange(0x10000), rng.randrange(0x10000), ss, flags), high


def compare(label, memory, cpu, high, target, function, cap, budget=400000):
    from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
    original = Uc(UC_ARCH_X86, UC_MODE_16)
    original.mem_map(0, 2 * SIZE)
    original.mem_write(0, bytes(memory))
    original.mem_write(SIZE, bytes(memory))
    for (name, _), register in zip(Cpu._fields_, REGISTERS):
        original.reg_write(register, getattr(cpu, name))
    base = physical(CALLER_CS, 0)
    body = (base + ENTRY, base + CODE_END)
    ranges = {name: (physical(SYSTEM_CS, leaf["start"]), physical(SYSTEM_CS, leaf["end"])) for name, leaf in target["leaves"].items()}
    sites = {base + s: (base + s + 5, bytes([0x9A, 0x25, 0x0B])) for s in STORE_SITES}
    sites[base + UPCASE_SITE] = (base + UPCASE_SITE + 5, bytes([0x9A, 0x2A, 0x18]))
    state = dict(stop=None, expected=None, check=None, returning=False, previous=None, loopbacks=0, last=None)
    seen = {k: set() for k in ("body", "store", "upcase")}
    edges = {k: set() for k in ("body", "store", "upcase")}

    def note(kind, here, conditional):
        seen[kind].add(here)
        if state["previous"] is not None and state["previous"][0] == kind and state["previous"][1] in conditional and state["previous"][1] != here:
            edges[kind].add((state["previous"][1], here))
        state["previous"] = (kind, here)

    def hook(emulator, address, size, user):
        del size, user
        last, state["last"] = state["last"], address
        if state["returning"]:
            state["stop"] = OK
            emulator.emu_stop()
            return
        if state["check"] is not None:
            expected, state["check"] = state["check"], None
            if address != expected:
                state["stop"] = TRANSFER
                emulator.emu_stop()
                return
        if address in sites:
            if bytes(memory[address:address + 3]) != sites[address][1]:
                state["stop"] = CODE
                emulator.emu_stop()
                return
            state["expected"] = sites[address][0]
        if address == base + LOOP_HEAD and last == base + LOOP_JMP:
            state["loopbacks"] += 1
            if state["loopbacks"] > cap:
                state["stop"] = LIMIT
                emulator.emu_stop()
                return
        if body[0] <= address < body[1]:
            note("body", address - base, target["conditional"])
            if address - base == BODY_RETF:
                state["returning"] = True
            return
        for name, (start, end) in ranges.items():
            if start <= address < end:
                leaf = target["leaves"][name]
                here = address - start + leaf["start"]
                note(name, here, leaf["conditional"])
                if here == leaf["retf"]:
                    state["check"] = state["expected"]
                return
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
    status = function(ram, SIZE, ctypes.byref(cpu), cap, ctypes.byref(callee))
    check(status == state["stop"], f"C status {status}, original stopped with {NAMES[state['stop']]}: {label}")
    actual = bytes(ram)
    if actual != expected:
        differences = [i for i, (a, b) in enumerate(zip(actual, expected)) if a != b]
        raise ValueError(f"RAM mismatch {label}: {[(hex(d), actual[d], expected[d]) for d in differences[:6]]} ({len(differences)} bytes)")
    for (name, _), register in zip(Cpu._fields_, REGISTERS):
        mine, want = getattr(cpu, name), original.reg_read(register) & 0xFFFF
        check(mine == want, f"{name} mismatch {label}: C={mine:04x}, original={want:04x}")
    return state["stop"], seen, edges


def cases(rng):
    spec = []
    for letter in LETTERS + (0x67, 0x41, 0x20 + 0x80, 0x00):
        for leading in (0, 1, 3):
            for follow in ((0x45, 0x65, 0x46) if letter == 0x47 else (rng.randrange(256),)):
                n, chars = text_for(rng, leading, letter, follow)
                spec.append((f"letter-{letter:02x}-{leading}-{follow:02x}", dict(n=n, chars=chars, flags=(0x7202, 0x0ED7)[leading & 1])))
    for i in range(60):
        letter = rng.choice(LETTERS + (rng.randrange(256),))
        n, chars = text_for(rng, rng.randrange(6), letter, rng.choice((0x45, 0x65, rng.randrange(256))), n=rng.choice((None, 0, 1, 2, 255)))
        spec.append((f"random-{i}", dict(n=n, chars=chars, sp=rng.randrange(0x0300, 0xFE00) & ~1, flags=(0x7202, 0x0602, 0x0ED7, 0xF2D7)[i % 4])))
    # an all-space local loops forever: LIMIT; and the wrap of i through L[0] and [BP-1]
    spec.append(("all-space", dict(n=0x20, chars=[0x20] * 40, stale=0x20, cap=600)))
    spec.append(("all-space-small-cap", dict(n=0x20, chars=[0x20] * 40, stale=0x20, cap=3)))
    spec.append(("spaces-to-255", dict(n=0xFF, chars=[0x20] * 254 + [0x47], stale=0x20, cap=600)))
    bp = 0x8000 - 2
    for name, dst in (("onto-local", (SS, (bp - 0x101) & 0xFFFF)), ("onto-result", (SS, (bp - 1) & 0xFFFF)),
                      ("onto-store-frame", (SS, (bp - 0x110) & 0xFFFF)), ("onto-own-frame", (SS, (bp + 2) & 0xFFFF))):
        for letter in (0x47, 0x50):
            n, chars = text_for(rng, 1, letter, 0x45)
            spec.append((f"dst-{name}-{letter:02x}", dict(n=n, chars=chars, dst=dst)))
    for sp in (0x0120, 0x0130, 0xFFF4, 0xFFF8, 0xFFFE):
        n, chars = text_for(rng, 2, 0x53, 0)
        spec.append((f"sp-{sp:04x}", dict(n=n, chars=chars, sp=sp, ss=EDGE_SS)))
    n, chars = text_for(rng, 2, 0x43, 0)
    spec.append(("ss-a20", dict(n=n, chars=chars, sp=0x0400, ss=0xFFFF)))
    spec.append(("src-dst-a20", dict(n=n, chars=chars, src=(0xFFFF, 0x0100), dst=(0xFFF8, 0x0300))))
    spec.append(("random-background", dict(n=n, chars=chars, background=bytes(random.Random(0x7F0B93).randbytes(SIZE)))))
    for k, (site, letter) in enumerate(((STORE_SITES[0], 0x50), (UPCASE_SITE, 0x47), (STORE_SITES[3], 0x50), (STORE_SITES[1], 0x47))):
        n, chars = text_for(rng, 1, letter, 0x45)
        spec.append((f"code-{site:04x}", dict(n=n, chars=chars, patch=((site + (k % 3), 0x90 if k % 3 == 0 else 0x41),))))
    return spec


def differential(target, code, image, function):
    rng = random.Random(0x7F0B93)
    seen = {k: set() for k in ("body", "store", "upcase")}
    edges = {k: set() for k in ("body", "store", "upcase")}
    stops = {k: 0 for k in NAMES}
    for label, options in cases(rng):
        cap = options.pop("cap", 600)
        n, chars = options.pop("n"), options.pop("chars")
        memory, cpu, high = build(image, code, rng, n, chars, **options)
        stop, s, e = compare(f"+0B93 {label}", memory, cpu, high, target, function, cap)
        stops[stop] += 1
        for kind in seen:
            seen[kind] |= s[kind]
            edges[kind] |= e[kind]
    report = {}
    for kind, decoded, conditional in (("body", target["decoded"], target["conditional"]),
                                       ("store", target["leaves"]["store"]["decoded"], target["leaves"]["store"]["conditional"]),
                                       ("upcase", target["leaves"]["upcase"]["decoded"], target["leaves"]["upcase"]["conditional"])):
        skip_edges, skip_starts = UNREACHABLE[kind]
        check(not (edges[kind] & skip_edges) and not (seen[kind] & skip_starts), f"{kind}: an unreachable edge was taken")
        missing = sorted(hex(i.address) for i in decoded if i.address not in seen[kind] and i.address not in skip_starts)
        reachable = {(s, o) for s, outs in conditional.items() for o in outs} - skip_edges
        missing_edges = sorted((hex(s), hex(o)) for s, o in reachable - edges[kind])
        check(not missing and not missing_edges, f"{kind} coverage incomplete: {missing} {missing_edges}")
        report[kind] = dict(starts=len(seen[kind]), reachable_starts=len(decoded) - len(skip_starts), outcomes=len(edges[kind]),
                            reachable_outcomes=len(reachable))
        print(f"  {kind}: {len(seen[kind])}/{len(decoded) - len(skip_starts)} reachable instruction starts, "
              f"{len(edges[kind])}/{len(reachable)} reachable conditional outcomes")
    check(all(stops[k] for k in (CODE, TRANSFER, LIMIT)), f"a stop kind was not exercised: {stops}")
    print(f"U7F +0B93: {sum(stops.values())} original-byte cases passed ({stops[OK]} returns, {stops[CODE]} CODE, {stops[TRANSFER]} TRANSFER, "
          f"{stops[LIMIT]} LIMIT); full 1 MiB RAM and all 14 CPU words equal")
    report["stops"] = {NAMES[k]: v for k, v in stops.items()}
    return report


# ---- DOSBox-X synthetic oracle -------------------------------------------------------------------------
CALLER_N, SYS_N, SRC_SEG, DST_SEG = 0x8800, 0x9000, 0x2000, 0x2800
MAIN_AT, STORE_BLOB, UPCASE_BLOB, UNIT_BLOB, CASES_AT = 0x0300, 0x0F00, 0x0F40, 0x1000, 0x5000
OUTPUT, HEADER, STACK_TOP = 0xC000, 0xC800, 0xF000
FRAME_LOW, FRAME_SIZE, DST_SIZE = 0xEEC0, 0x140, 0x100
CASE_SIZE = 4 + 40
RECORD_BYTES = 28 + FRAME_SIZE + DST_SIZE
NATIVE_FLAGS = (0x0002, 0x00D7, 0x08D7, 0x0402, 0x0893, 0x0003)
ENTRY_REGS = dict(ax=0x1234, bx=0x3456, cx=0x5678, dx=0x789A, si=0x9ABC, di=0xBCDE, bp=0x7777)


def native_cases():
    rng = random.Random(0x7F0B93D)
    rows = []
    for letter in LETTERS + (0x67, 0x41):
        for leading in (0, 2):
            for follow in ((0x45, 0x65, 0x46) if letter == 0x47 else (rng.randrange(256),)):
                rows.append((leading, letter, follow))
    for _ in range(40):
        rows.append((rng.randrange(6), rng.choice(LETTERS + (rng.randrange(256),)), rng.choice((0x45, 0x65, rng.randrange(256)))))
    cases = []
    for k, (leading, letter, follow) in enumerate(rows):
        n, chars = text_for(rng, leading, letter, follow)
        cases.append(dict(n=min(n, 39), chars=chars[:39], flags=k % len(NATIVE_FLAGS), dst_off=rng.randrange(0, DST_SIZE - 0x60)))
    return cases


def native_audit(target, code, function):
    directory = ROOT / "resources/oracle-runs/u7f-0b93-synthetic"
    directory.mkdir(parents=True, exist_ok=True)
    unit = bytearray(code)
    for site in STORE_SITES + (UPCASE_SITE,):
        struct.pack_into("<H", unit, site + 3, SYS_N)
    store, upcase = target["leaves"]["store"]["body"], target["leaves"]["upcase"]["body"]
    cases = native_cases()
    text = [".code16", ".intel_syntax noprefix", ".global _start", "_start:", "    jmp main", f".org {MAIN_AT - 0x100:#x}", "main:",
            "    mov ax,cs", "    mov ds,ax", "    mov es,ax", "    mov ss,ax", f"    mov sp,{STACK_TOP:#x}", "    cld"]
    for segment, offset, label, length in ((CALLER_N, 0, "unit", len(unit)), (SYS_N, STORE[0], "store", len(store)), (SYS_N, UPCASE[0], "upcase", len(upcase))):
        text += [f"    mov ax,{segment:#x}", "    mov es,ax", f"    mov si,offset {label}", f"    mov di,{offset:#x}", f"    mov cx,{length:#x}", "    rep movsb"]
    text += ["    mov ax,cs", "    mov es,ax",
             f"    mov word ptr [{HEADER:#x}],cs", f"    mov word ptr [{HEADER + 2:#x}],offset returned",
             f"    mov word ptr [{HEADER + 4:#x}],{len(cases)}", f"    mov word ptr [{HEADER + 6:#x}],0",
             "    mov dx,offset filename", "    xor cx,cx", "    mov ah,0x3c", "    int 0x21", "    jc failure",
             "    mov word ptr [handle],ax", "    mov bx,ax", f"    mov dx,{HEADER:#x}", "    mov cx,8", "    mov ah,0x40", "    int 0x21", "    jc failure",
             "again:", "    cli", f"    mov sp,{STACK_TOP:#x}", "    cld", "    mov ax,cs", "    mov ds,ax", "    mov es,ax",
             "    mov al,0xa5", f"    mov di,{FRAME_LOW:#x}", f"    mov cx,{FRAME_SIZE}", "    rep stosb",
             f"    mov ax,{DST_SEG:#x}", "    mov es,ax", "    xor di,di", "    mov al,0x5a", f"    mov cx,{DST_SIZE}", "    rep stosb",
             "    mov ax,word ptr [index]", f"    mov bx,{CASE_SIZE}", "    mul bx", "    add ax,offset cases", "    mov si,ax",
             f"    mov ax,{SRC_SEG:#x}", "    mov es,ax", "    xor di,di", "    add si,4", "    mov cx,40", "    rep movsb", "    sub si,44",
             "    mov ax,cs", "    mov es,ax",
             f"    mov ax,{SRC_SEG:#x}", "    push ax", "    xor ax,ax", "    push ax", f"    mov ax,{DST_SEG:#x}", "    push ax", "    push word ptr [si+2]",
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
             f"    mov si,{FRAME_LOW:#x}", f"    mov cx,{FRAME_SIZE}", "    rep movsb",
             f"    mov ax,{DST_SEG:#x}", "    mov ds,ax", "    xor si,si", f"    mov cx,{DST_SIZE}", "    rep movsb",
             "    mov ax,cs", "    mov ds,ax", "    sti",
             "    mov bx,word ptr [handle]", f"    mov dx,{OUTPUT:#x}", f"    mov cx,{RECORD_BYTES}", "    mov ah,0x40", "    int 0x21", "    jc failure",
             f"    cmp ax,{RECORD_BYTES}", "    jne failure",
             "    inc word ptr [index]", "    mov ax,word ptr [index]", f"    cmp ax,{len(cases)}", "    jb again",
             "    mov bx,word ptr [handle]", "    mov ah,0x3e", "    int 0x21", "    mov ax,0x4c00", "    int 0x21",
             "failure:", "    mov ax,0x4c01", "    int 0x21",
             'filename: .asciz "RESULT.BIN"', "handle: .word 0", "index: .word 0", f"target_far: .word {ENTRY:#x}, {CALLER_N:#x}",
             "flags_table: .word " + ",".join(f"{f:#x}" for f in NATIVE_FLAGS)]
    for label, blob, at in (("store", store, STORE_BLOB), ("upcase", upcase, UPCASE_BLOB), ("unit", bytes(unit), UNIT_BLOB)):
        text += [f".org {at - 0x100:#x}", f"{label}:"]
        for i in range(0, len(blob), 16):
            text.append("    .byte " + ",".join(f"{b:#x}" for b in blob[i:i + 16]))
    text += [f".org {CASES_AT - 0x100:#x}", "cases:"]
    for case in cases:
        row = struct.pack("<BBH", case["flags"], 0, case["dst_off"]) + bytes([case["n"]] + case["chars"])
        check(len(row) == CASE_SIZE, "case row size differs")
        text.append("    .byte " + ",".join(f"{b:#x}" for b in row))
    check(STORE_BLOB + len(store) <= UPCASE_BLOB and UPCASE_BLOB + len(upcase) <= UNIT_BLOB and UNIT_BLOB + len(unit) <= CASES_AT
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
    results = {}
    for n, case in enumerate(cases):
        record = result[8 + n * RECORD_BYTES:8 + (n + 1) * RECORD_BYTES]
        memory = bytearray(SIZE)
        for seg, off, blob in ((CALLER_N, 0, bytes(unit)), (SYS_N, STORE[0], store), (SYS_N, UPCASE[0], upcase)):
            at = physical(seg, off)
            memory[at:at + len(blob)] = blob
        for i in range(FRAME_SIZE):
            memory[physical(segment, FRAME_LOW + i)] = 0xA5
        for i in range(DST_SIZE):
            memory[physical(DST_SEG, i)] = 0x5A
        for i, value in enumerate([case["n"]] + case["chars"]):
            memory[physical(SRC_SEG, i)] = value
        sp = STACK_TOP
        for value in (SRC_SEG, 0, DST_SEG, case["dst_off"], segment, returned):
            sp -= 2
            put16(memory, segment, sp, value)
        cpu = Cpu(*ENTRY_REGS.values(), sp, CALLER_N, ENTRY, 0x1900, segment, segment, NATIVE_FLAGS[case["flags"]])
        ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
        callee = ctypes.c_int(99)
        label = f"native case {n}"
        check(function(ram, SIZE, ctypes.byref(cpu), 600, ctypes.byref(callee)) == 0, f"C status on {label}")
        for (name, _), want in zip(Cpu._fields_, struct.unpack_from("<14H", record)):
            check(getattr(cpu, name) == want, f"{label} {name}: C={getattr(cpu, name):04x}, DOS={want:04x}")
        got = bytes(ram[physical(segment, FRAME_LOW):physical(segment, FRAME_LOW) + FRAME_SIZE])
        got += bytes(ram[physical(DST_SEG, 0):physical(DST_SEG, 0) + DST_SIZE])
        check(got == record[28:], f"{label} bytes differ")
        results[cpu.ax & 0xFF] = results.get(cpu.ax & 0xFF, 0) + 1
    digest = sha256(result).hexdigest()
    print(f"DOSBox-X normal-core/386 synthetic cases passed: {len(cases)} (results {dict(sorted(results.items()))}); all 14 CPU words, the "
          f"{FRAME_SIZE:#x}-byte stack window and the {DST_SIZE:#x}-byte output window; no mask")
    print(f"native result SHA-256 {digest}; COM segment {segment:04x}; string store and UpCase copied to {SYS_N:04X}")
    return dict(cases=len(cases), results={str(k): v for k, v in sorted(results.items())}, result_sha256=digest)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--unicorn", action="store_true")
    parser.add_argument("--native", action="store_true")
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    target, code, image = source()
    report = dict(target=dict(sha256=target["sha"], bytes=len(target["body"]), instructions=len(target["decoded"]), file_offset=target["file_offset"]))
    if args.unicorn or args.native:
        with tempfile.TemporaryDirectory(prefix="poolrad-u7f-") as temp:
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
