#!/usr/bin/env python3
"""Original-byte full-state parity for raw-CPU U3C2 +002E with SYSTEM Move composed, plus a DOSBox-X oracle.

The differential places the hash-checked expanded START.EXE image at load segment 0, so the original
Move runs live at 0AF8:0A3F from +002E's three unrelocated, fixup-backed far calls. It loads unit
0x3C2 and compares the entire 1 MiB RAM and all 14 CPU words between the original under Unicorn and
the C model, with no mask. The original decides each stop: its return (wherever RETF goes), a
rewritten call site (CODE), a Move return elsewhere (TRANSFER) or the host loop-back cap (LIMIT).
Media stays private.
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

ENTRY, END = 0x002E, 0x01A0
SITES = (0x0131, 0x0160, 0x0181)
LOOP_INCS = (0x0072, 0x0056)
UNREACHABLE = {(0x004C, 0x004E)}            # 1 <= [BP-8] = n-1 always: [BP-8] was just written from n > 1
UNREACHABLE_STARTS = {0x004E}
MOVE_CS, MOVE_IP, MOVE_END, MOVE_RETF = 0x0AF8, 0x0A3F, 0x0A57, 0x0A54
MOVE_SHA = "5bf8af52bdcd4af1b7e73b160b3ee02e6de7812b7e686aae02f73d9ccbc061aa"
CALLER_CS, DS, SS, EDGE_SS = 0x8800, 0x1800, 0x2800, 0x2000
RET_CS, RET_IP = 0x9001, 0x7000
OK, CODE, TRANSFER, LIMIT = 0, 2, 3, 4
NAMES = {OK: "return", CODE: "CODE", TRANSFER: "TRANSFER", LIMIT: "LIMIT"}


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
    move = image[MOVE_CS * 16 + MOVE_IP:MOVE_CS * 16 + MOVE_END]
    check(sha256(move).hexdigest() == MOVE_SHA, "SYSTEM 0AF8:0A3F differs from the pinned Move")
    unit = {u["stub_para"]: u for u in find_units(exe[parse_mz(exe)["header_bytes"]:], ovr)}[0x3C2]
    code = ovr[unit["ovr_offset"]:unit["ovr_offset"] + unit["code_size"]]
    starts = sorted(f["rel"] for f in unit["functions"])
    check(END == next(s for s in starts if s > ENTRY), "U3C2 +002E bounds differ")
    body = code[ENTRY:END]
    decoded = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(body, ENTRY))
    calls = [(i.address, i.op_str) for i in decoded if "call" in i.mnemonic]
    fixups = set(unit["fixups"])
    check(len(body) == 370 and len(decoded) == 158 and sum(i.size for i in decoded) == 370 and decoded[-1].mnemonic == "retf"
          and decoded[-1].op_str == "" and calls == [(s, "0xaf8, 0xa3f") for s in SITES] and all(s + 3 in fixups for s in SITES),
          f"U3C2 +002E extent or calls differ: {calls}")
    conditional = conditionals(decoded)
    check(len(conditional) == 9, "U3C2 +002E conditional count differs")
    target = dict(body=body, decoded=decoded, conditional=conditional, sha=sha256(body).hexdigest(), file_offset=unit["ovr_offset"] + ENTRY,
                  move=move, move_decoded=list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(move, MOVE_IP)))
    print(f"U3C2 +002E (370/158, 3 x LCALL 0AF8:0A3F, RETF) GAME file {target['file_offset']:#x} sha {target['sha']}")
    return target, code, image


CHECKER_ROWS = ('OVL_03C2_002E',)


def library(directory):
    path = directory / "u3c2.so"
    subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-O2", "-fPIC", "-shared", "-I", str(ROOT / "include"),
                    str(ROOT / "src/recovered/overlay_u3c2_record_sort_002e_cpu.c"), str(ROOT / "src/recovered/system_runtime_leaves_cpu.c"),
                    "-o", str(path)], check=True, capture_output=True, text=True)
    function = ctypes.CDLL(str(path)).pr_u3c2_record_sort_002e_cpu
    function.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t, ctypes.POINTER(Cpu), ctypes.c_uint, ctypes.POINTER(ctypes.c_int)]
    function.restype = ctypes.c_int
    return function


def records_for(rng, n, mode):
    data = [rng.randrange(256) for _ in range(3 * 257)]
    for i in range(1, n + 1):
        if mode == 1: data[3 * i + 1] = n - i                      # reversed
        if mode == 2: data[3 * i + 1] = 7                          # all tied: the parity rule decides
        if mode == 3: data[3 * i + 1] &= 3                         # many ties
        if mode == 4: data[3 * i + 1] = i                          # sorted
    data[2] = n
    return bytes(data)


def build(image, code, rng, records, *, sp=0x8000, ss=SS, ds=DS, flags=0x7202, background=None, patch=()):
    memory = bytearray([0xA5]) * SIZE if background is None else bytearray(background)
    memory[0:len(image)] = image
    base = physical(CALLER_CS, 0)
    memory[base:base + len(code)] = code
    for offset, value in patch:
        memory[physical(CALLER_CS, offset)] = value
    for i, value in enumerate(records):
        memory[physical(ds, (0x6718 + i) & 0xFFFF)] = value
    frame = physical(ss, sp)
    for i, value in enumerate([RET_IP, RET_CS]):                    # both views agree at a segment end
        put16(memory, ss, (sp + 2 * i) & 0xFFFF, value)
        put16(memory, 0, (frame + 2 * i) & (SIZE - 1), value)
    reg = tuple(rng.randrange(0x10000) for _ in range(7))
    high = ss >= 0xF000 or ds >= 0xF000
    return memory, Cpu(*reg, sp, CALLER_CS, ENTRY, ds, rng.randrange(0x10000), ss, flags), high


def compare(label, memory, cpu, high, target, function, cap, budget=8000000):
    from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
    original = Uc(UC_ARCH_X86, UC_MODE_16)
    original.mem_map(0, 2 * SIZE)
    original.mem_write(0, bytes(memory))
    original.mem_write(SIZE, bytes(memory))
    for (name, _), register in zip(Cpu._fields_, REGISTERS):
        original.reg_write(register, getattr(cpu, name))
    base = physical(CALLER_CS, 0)
    body, move = (base + ENTRY, base + END), (physical(MOVE_CS, MOVE_IP), physical(MOVE_CS, MOVE_END))
    sites = {base + s: base + s + 5 for s in SITES}
    site_bytes = {base + s: bytes(memory[base + s:base + s + 3]) for s in SITES}
    incs = {base + s for s in LOOP_INCS}
    state = dict(stop=None, expected=None, check=None, previous=None, loopbacks=0, last_jump=None)
    visited, edges, move_visited = set(), set(), set()

    def hook(emulator, address, size, user):
        del size, user
        if state["check"] is not None:
            expected, state["check"] = state["check"], None
            if address != expected:
                state["stop"] = TRANSFER
                emulator.emu_stop()
                return
        if address in sites:
            if site_bytes[address] != bytes([0x9A, 0x3F, 0x0A]):
                state["stop"] = CODE
                emulator.emu_stop()
                return
            state["expected"] = sites[address]
        if address in incs and state["last_jump"]:
            state["loopbacks"] += 1
            if state["loopbacks"] > cap:
                state["stop"] = LIMIT
                emulator.emu_stop()
                return
        state["last_jump"] = address - base in (0x018E, 0x0199)
        if body[0] <= address < body[1]:
            here = address - base
            visited.add(here)
            if state["previous"] in target["conditional"] and state["previous"] != here:
                edges.add((state["previous"], here))
            state["previous"] = here
        elif move[0] <= address < move[1]:
            here = address - move[0] + MOVE_IP
            move_visited.add(here)
            state["previous"] = None
            if here == MOVE_RETF:
                state["check"] = state["expected"]
        else:
            state["stop"] = OK
            emulator.emu_stop()

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
    return state["stop"], visited, edges, move_visited


def cases(rng):
    spec = []
    for n in range(0, 13):
        for mode in range(5):
            spec.append((f"n{n}-mode{mode}", dict(records=records_for(rng, n, mode), flags=(0x7202, 0x0ED7, 0x0602)[mode % 3])))
    for i in range(40):
        n = rng.choice((rng.randrange(13, 40), rng.randrange(40, 90)))
        spec.append((f"random-{i}", dict(records=records_for(rng, n, rng.randrange(5)), sp=rng.randrange(0x0200, 0xFE00) & ~1)))
    spec.append(("n255", dict(records=records_for(rng, 255, 3))))
    spec.append(("n254", dict(records=records_for(rng, 254, 1))))
    # SS = DS: the temp at BP-7 overwrites the count byte DS:671A, and records reach the stack locals
    for sp, n in ((0x6723, 8), (0x6723, 30), (0x6722, 12), (0x6730, 20), (0x6760, 40), (0x6740, 25)):
        for mode in (0, 2):
            spec.append((f"alias-sp{sp:04x}-n{n}-{mode}", dict(records=records_for(rng, n, mode), sp=sp, ss=DS, cap=4000)))
    # records over Move's own frame: a swap rewrites its return address (TRANSFER)
    for sp in (0x6740 + 0x10, 0x6748, 0x6750):
        spec.append((f"move-frame-sp{sp:04x}", dict(records=records_for(rng, 30, 1), sp=sp, ss=DS, cap=4000)))
    for sp in (0x0008, 0x0010, 0xFFF8, 0xFFFE):
        spec.append((f"sp-{sp:04x}", dict(records=records_for(rng, 9, 1), sp=sp, ss=EDGE_SS)))
    spec.append(("ss-a20", dict(records=records_for(rng, 9, 1), sp=0x0400, ss=0xFFFF)))
    spec.append(("ds-a20", dict(records=records_for(rng, 9, 1), ds=0xFFF0)))
    spec.append(("random-background", dict(records=records_for(rng, 20, 0), background=bytes(random.Random(0x3C2).randbytes(SIZE)))))
    for cap in (0, 1, 5, 17):
        spec.append((f"cap-{cap}", dict(records=records_for(rng, 12, 1), cap=cap)))
    for k, site in enumerate(SITES):
        spec.append((f"code-{k}", dict(records=records_for(rng, 6, 1), patch=((site + k % 3, 0x90 if k == 0 else 0x40),))))
    return spec


def differential(target, code, image, function):
    rng = random.Random(0x3C2002E)
    visited, edges, move_visited, stops = set(), set(), set(), {k: 0 for k in NAMES}
    for label, options in cases(rng):
        cap = options.pop("cap", 100000)
        records = options.pop("records")
        memory, cpu, high = build(image, code, rng, records, **options)
        stop, v, e, mv = compare(f"+002E {label}", memory, cpu, high, target, function, cap)
        visited |= v
        edges |= e
        move_visited |= mv
        stops[stop] += 1
    check(not (edges & UNREACHABLE) and not (visited & UNREACHABLE_STARTS), "an unreachable edge was taken")
    missing = sorted(hex(i.address) for i in target["decoded"] if i.address not in visited and i.address not in UNREACHABLE_STARTS)
    reachable = {(s, o) for s, outs in target["conditional"].items() for o in outs} - UNREACHABLE
    missing_edges = sorted((hex(s), hex(o)) for s, o in reachable - edges)
    check(not missing and not missing_edges and len(move_visited) == len(target["move_decoded"]),
          f"coverage incomplete: {missing} {missing_edges} move {len(move_visited)}")
    check(all(stops[k] for k in (CODE, LIMIT)), f"a stop kind was not exercised: {stops}")
    print(f"U3C2 +002E: {sum(stops.values())} original-byte cases passed ({stops[OK]} returns, {stops[CODE]} CODE, {stops[TRANSFER]} TRANSFER, "
          f"{stops[LIMIT]} LIMIT); full 1 MiB RAM and all 14 CPU words equal; {len(visited)}/{len(target['decoded']) - 1} reachable "
          f"instruction starts, {len(edges)}/{len(reachable)} reachable conditional outcomes (the JBE at +004C is always taken); "
          f"Move {len(move_visited)}/{len(target['move_decoded'])}")
    return dict(stops={NAMES[k]: v for k, v in stops.items()}, starts=len(visited), outcomes=len(edges), reachable_outcomes=len(reachable))


# ---- DOSBox-X synthetic oracle -------------------------------------------------------------------------
CALLER_N, MOVE_N, DATA_DS = 0x8800, 0x9000, 0x1900
MAIN_AT, MOVE_BLOB, UNIT_BLOB, CASES_AT = 0x0300, 0x0F00, 0x1000, 0x3000
OUTPUT, HEADER, STACK_TOP = 0xC000, 0xC800, 0xF000
FRAME_LOW, FRAME_SIZE, REC_SIZE = 0xEFC0, 0x40, 0x300
CASE_SIZE = 2 + REC_SIZE
RECORD_BYTES = 28 + FRAME_SIZE + REC_SIZE
NATIVE_FLAGS = (0x0002, 0x00D7, 0x08D7, 0x0402, 0x0893, 0x0003)
ENTRY_REGS = dict(ax=0x1234, bx=0x3456, cx=0x5678, dx=0x789A, si=0x9ABC, di=0xBCDE, bp=0x7777)


def native_cases():
    rng = random.Random(0x3C2D05)
    rows = []
    for n in range(0, 13):
        rows.append(records_for(rng, n, n % 5))
    for i in range(27):
        rows.append(records_for(rng, rng.choice((rng.randrange(13, 60), rng.randrange(60, 200))), i % 5))
    return [dict(records=r[:REC_SIZE], flags=k % len(NATIVE_FLAGS)) for k, r in enumerate(rows)]


def native_audit(target, code, function):
    directory = ROOT / "resources/oracle-runs/u3c2-002e-synthetic"
    directory.mkdir(parents=True, exist_ok=True)
    unit = bytearray(code)
    for site in SITES:
        struct.pack_into("<H", unit, site + 3, MOVE_N)
    cases = native_cases()
    text = [".code16", ".intel_syntax noprefix", ".global _start", "_start:", "    jmp main", f".org {MAIN_AT - 0x100:#x}", "main:",
            "    mov ax,cs", "    mov ds,ax", "    mov es,ax", "    mov ss,ax", f"    mov sp,{STACK_TOP:#x}", "    cld"]
    for segment, offset, label, length in ((CALLER_N, 0, "unit", len(unit)), (MOVE_N, MOVE_IP, "move", len(target["move"]))):
        text += [f"    mov ax,{segment:#x}", "    mov es,ax", f"    mov si,offset {label}", f"    mov di,{offset:#x}", f"    mov cx,{length:#x}", "    rep movsb"]
    text += ["    mov ax,cs", "    mov es,ax",
             f"    mov word ptr [{HEADER:#x}],cs", f"    mov word ptr [{HEADER + 2:#x}],offset returned",
             f"    mov word ptr [{HEADER + 4:#x}],{len(cases)}", f"    mov word ptr [{HEADER + 6:#x}],0",
             "    mov dx,offset filename", "    xor cx,cx", "    mov ah,0x3c", "    int 0x21", "    jc failure",
             "    mov word ptr [handle],ax", "    mov bx,ax", f"    mov dx,{HEADER:#x}", "    mov cx,8", "    mov ah,0x40", "    int 0x21", "    jc failure",
             "again:", "    cli", f"    mov sp,{STACK_TOP:#x}", "    cld", "    mov ax,cs", "    mov ds,ax", "    mov es,ax",
             "    mov al,0xa5", f"    mov di,{FRAME_LOW:#x}", f"    mov cx,{FRAME_SIZE}", "    rep stosb",
             "    mov ax,word ptr [index]", f"    mov bx,{CASE_SIZE}", "    mul bx", "    add ax,offset cases", "    mov si,ax",
             f"    mov ax,{DATA_DS:#x}", "    mov es,ax", "    mov di,0x6718", "    add si,2", f"    mov cx,{REC_SIZE}", "    rep movsb",
             f"    sub si,{2 + REC_SIZE}", "    mov ax,cs", "    mov es,ax",
             "    mov bl,byte ptr [si]", "    xor bh,bh", "    shl bx,1", "    push word ptr [flags_table+bx]", "    popf",
             f"    mov ax,{DATA_DS:#x}", "    mov ds,ax"]
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
             f"    mov ax,{DATA_DS:#x}", "    mov ds,ax", "    mov si,0x6718", f"    mov cx,{REC_SIZE}", "    rep movsb",
             "    mov ax,cs", "    mov ds,ax", "    sti",
             "    mov bx,word ptr [handle]", f"    mov dx,{OUTPUT:#x}", f"    mov cx,{RECORD_BYTES}", "    mov ah,0x40", "    int 0x21", "    jc failure",
             f"    cmp ax,{RECORD_BYTES}", "    jne failure",
             "    inc word ptr [index]", "    mov ax,word ptr [index]", f"    cmp ax,{len(cases)}", "    jb again",
             "    mov bx,word ptr [handle]", "    mov ah,0x3e", "    int 0x21", "    mov ax,0x4c00", "    int 0x21",
             "failure:", "    mov ax,0x4c01", "    int 0x21",
             'filename: .asciz "RESULT.BIN"', "handle: .word 0", "index: .word 0", f"target_far: .word {ENTRY:#x}, {CALLER_N:#x}",
             "flags_table: .word " + ",".join(f"{f:#x}" for f in NATIVE_FLAGS)]
    for label, blob, at in (("move", target["move"], MOVE_BLOB), ("unit", bytes(unit), UNIT_BLOB)):
        text += [f".org {at - 0x100:#x}", f"{label}:"]
        for i in range(0, len(blob), 16):
            text.append("    .byte " + ",".join(f"{b:#x}" for b in blob[i:i + 16]))
    text += [f".org {CASES_AT - 0x100:#x}", "cases:"]
    for case in cases:
        row = bytes([case["flags"], 0]) + case["records"]
        check(len(row) == CASE_SIZE, "case row size differs")
        for i in range(0, len(row), 32):
            text.append("    .byte " + ",".join(f"{b:#x}" for b in row[i:i + 32]))
    check(UNIT_BLOB + len(unit) <= CASES_AT and CASES_AT + CASE_SIZE * len(cases) <= OUTPUT, "COM layout overlaps")
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
    swaps = 0
    for n, case in enumerate(cases):
        record = result[8 + n * RECORD_BYTES:8 + (n + 1) * RECORD_BYTES]
        memory = bytearray(SIZE)
        for seg, off, blob in ((CALLER_N, 0, bytes(unit)), (MOVE_N, MOVE_IP, target["move"])):
            at = physical(seg, off)
            memory[at:at + len(blob)] = blob
        for i in range(FRAME_SIZE):
            memory[physical(segment, FRAME_LOW + i)] = 0xA5
        for i, value in enumerate(case["records"]):
            memory[physical(DATA_DS, 0x6718 + i)] = value
        sp = STACK_TOP
        for value in (segment, returned):
            sp -= 2
            put16(memory, segment, sp, value)
        cpu = Cpu(*ENTRY_REGS.values(), sp, CALLER_N, ENTRY, DATA_DS, segment, segment, NATIVE_FLAGS[case["flags"]])
        ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
        callee = ctypes.c_int(99)
        label = f"native case {n} (n={case['records'][2]})"
        check(function(ram, SIZE, ctypes.byref(cpu), 100000, ctypes.byref(callee)) == 0, f"C status on {label}")
        swaps += callee.value == 0
        for (name, _), want in zip(Cpu._fields_, struct.unpack_from("<14H", record)):
            check(getattr(cpu, name) == want, f"{label} {name}: C={getattr(cpu, name):04x}, DOS={want:04x}")
        got = bytes(ram[physical(segment, FRAME_LOW):physical(segment, FRAME_LOW) + FRAME_SIZE])
        got += bytes(ram[physical(DATA_DS, 0x6718):physical(DATA_DS, 0x6718) + REC_SIZE])
        check(got == record[28:], f"{label} bytes differ")
    digest = sha256(result).hexdigest()
    print(f"DOSBox-X normal-core/386 synthetic cases passed: {len(cases)} ({swaps} with swaps); all 14 CPU words, the {FRAME_SIZE}-byte stack "
          f"window and the {REC_SIZE:#x}-byte record table; no mask")
    print(f"native result SHA-256 {digest}; COM segment {segment:04x}; Move copied to {MOVE_N:04X}:0A3F")
    return dict(cases=len(cases), with_swaps=swaps, result_sha256=digest)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--compile-only", action="store_true",
                        help="build and bind the candidate library only; no media")
    parser.add_argument("--unicorn", action="store_true")
    parser.add_argument("--native", action="store_true")
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    if args.compile_only:  # same compiler command and symbol binding as the audit; no media
        with tempfile.TemporaryDirectory(prefix='poolrad-checker-') as _t:
            library(Path(_t))
        print('CHECKER_BUILD_OK')
        return
    target, code, image = source()
    report = dict(target=dict(sha256=target["sha"], bytes=370, instructions=158, file_offset=target["file_offset"]))
    if args.unicorn or args.native:
        with tempfile.TemporaryDirectory(prefix="poolrad-u3c2-") as temp:
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
