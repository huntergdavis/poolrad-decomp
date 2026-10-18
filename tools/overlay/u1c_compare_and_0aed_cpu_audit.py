#!/usr/bin/env python3
"""Original-byte full-state parity for raw-CPU U1C +0AED (ECL COMPARE AND) with U25 +0000/+0136 (through the loaded
overlay stub) and SYSTEM FillChar composed, plus a DOSBox-X oracle.

The differential places the hash-checked expanded START.EXE image at load segment 0 (SYSTEM and the overlay stub table
live), patches unit 0x25's stub entries 002B:0025 and 002B:002A to their loaded EA jumps, loads unit 0x25 at its own
segment and unit 0x1C at the caller's, and compares the entire 1 MiB RAM and all 14 CPU words between the original
under Unicorn and the C model, with no mask. The original decides each stop: its return (wherever RETF goes), a site
or stub whose live bytes differ at its fetch (CODE), a callee return elsewhere (TRANSFER), or U25 +0000 reaching its
operand header +0022 once more than the host budget (CALLEE). Media stays private.
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

ENTRY, END, BODY_RETF = 0x0AED, 0x0B52, 0x0B51
SITES = {0x0AFF: (0x16A8, None), 0x0B07: (0x002A, 0x0000), 0x0B19: (0x0025, 0x0136)}
OPERAND_HEADER = 0x0022
STUB_SEG, SYSTEM_CS = 0x002B, 0x0AF8
CALLER_CS, U25_CS, DS, SS, EDGE_SS = 0x8800, 0x7800, 0x1800, 0x2800, 0x2000
RET_CS, RET_IP = 0x9001, 0x7000
OK, CODE, CALLEE, TRANSFER = 0, 2, 3, 4
NAMES = {OK: "return", CODE: "CODE", CALLEE: "CALLEE", TRANSFER: "TRANSFER"}
FILLCHAR_SHA = "f1c5fade96970aa1dd1a9ffc2d876c7aea57bef8680d929d9f65d6c5190e17f7"
TAGS = (0, 0, 0, 0, 2, 0x81, 0x10, 0x55)


class Limits(ctypes.Structure):
    _fields_ = [("system_cs", ctypes.c_uint16), ("max_operands", ctypes.c_size_t), ("max_string_iterations", ctypes.c_size_t)]


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
    check(sha256(image[SYSTEM_CS * 16 + 0x16A8:SYSTEM_CS * 16 + 0x16BC]).hexdigest() == FILLCHAR_SHA, "SYSTEM FillChar differs")
    units = {u["stub_para"]: u for u in find_units(exe[parse_mz(exe)["header_bytes"]:], ovr)}
    u1c, u25 = units[0x1C], units[0x25]
    check(u25["entries"][1] == 0x0136 and u25["entries"][2] == 0x0000, "unit 0x25 entries 1/2 differ")
    code1c = ovr[u1c["ovr_offset"]:u1c["ovr_offset"] + u1c["code_size"]]
    code25 = ovr[u25["ovr_offset"]:u25["ovr_offset"] + u25["code_size"]]
    starts = sorted(f["rel"] for f in u1c["functions"])
    check(END == next(s for s in starts if s > ENTRY), "U1C +0AED bounds differ")
    body = code1c[ENTRY:END]
    decoded = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(body, ENTRY))
    calls = [(i.address, i.op_str) for i in decoded if "call" in i.mnemonic]
    fixups = set(u1c["fixups"])
    check(len(body) == 101 and len(decoded) == 40 and sum(i.size for i in decoded) == 101 and decoded[-1].mnemonic == "retf"
          and decoded[-1].op_str == "" and calls == [(0x0AFF, "0xaf8, 0x16a8"), (0x0B07, "0x2b, 0x2a"), (0x0B19, "0x2b, 0x25")]
          and all(s + 3 in fixups for s in SITES), f"U1C +0AED extent or calls differ: {calls}")
    target = dict(body=body, decoded=decoded, conditional=conditionals(decoded), sha=sha256(body).hexdigest(),
                  file_offset=u1c["ovr_offset"] + ENTRY)
    print(f"U1C +0AED (101/40, LCALL 0AF8:16A8, LCALL 002B:002A and 002B:0025 to U25 +0000/+0136, RETF) GAME file "
          f"{target['file_offset']:#x} sha {target['sha']}; U25 code {len(code25):#x} bytes")
    return target, code1c, code25, image


def library(directory):
    path = directory / "u1c0aed.so"
    sources = ["overlay_u1c_compare_and_0aed_cpu.c", "overlay_u25_loader_0000.c", "overlay_u25_slot_value_0136.c",
               "overlay_u25_string_cores.c", "overlay_u25_entry1_0136.c", "overlay_u25_load_word_0e0c.c", "overlay_u25_party_index_0756.c",
               "system_runtime_leaves_cpu.c"]
    result = subprocess.run(["cc", "-std=c11", "-O2", "-fPIC", "-shared", "-I", str(ROOT / "include")] +
                            [str(ROOT / "src/recovered" / s) for s in sources] + ["-o", str(path)], capture_output=True, text=True)
    if result.returncode != 0:
        raise ValueError(result.stderr[-3000:])
    function = ctypes.CDLL(str(path)).pr_u1c_compare_and_0aed_cpu
    function.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t, ctypes.POINTER(Cpu), ctypes.POINTER(Limits),
                         ctypes.POINTER(ctypes.c_int), ctypes.POINTER(ctypes.c_int)]
    function.restype = ctypes.c_int
    return function


def stream(operands):
    """The script bytes of four operands: tag, value and, for tags 1, 2, 3 and 81h, a third byte."""
    out = []
    for tag, value, third in operands:
        out += [tag, value] + ([third] if tag in (1, 2, 3, 0x81) else [])
    return out


def world_spec(rng, *, tags=None, equal=None):
    operands = []
    for k in range(4):
        tag = rng.choice(TAGS) if tags is None else tags[k]
        value, third = rng.randrange(256), rng.randrange(256)
        if tag == 0x81:                                              # string by address 4900h/4901h: the 'Q' or empty window string
            value, third = rng.choice((0, 1)), 0x49
        operands.append([tag, value, third])
    if equal is None:
        equal = rng.random() < 0.6
    if equal:                                                        # make the pairs equal more often than chance
        for a, b in ((0, 1), (2, 3)):
            if rng.random() < 0.8:
                operands[b] = list(operands[a])
    return dict(operands=operands)


def build(code1c, code25, image, rng, spec, *, sp=0x8000, ss=SS, flags=0x7202, background=None, patch=(), stub_patch=(), value_stub=None):
    memory = bytearray([0xA5]) * SIZE if background is None else bytearray(background)
    memory[0:len(image)] = image
    for base, code in ((physical(CALLER_CS, 0), code1c), (physical(U25_CS, 0), code25)):
        memory[base:base + len(code)] = code
    for offset, ip in ((0x0025, 0x0136), (0x002A, 0x0000)):          # the loaded stub: JMP FAR U25:entry
        memory[physical(STUB_SEG, offset):physical(STUB_SEG, offset) + 5] = struct.pack("<BHH", 0xEA, ip, U25_CS)
    if value_stub is not None:                                       # the value call through a stub copy at another segment
        put16(memory, CALLER_CS, 0x0B1C, value_stub)
        memory[physical(value_stub, 0x0025):physical(value_stub, 0x0025) + 5] = struct.pack("<BHH", 0xEA, 0x0136, U25_CS)
    for offset, value in patch:
        memory[physical(CALLER_CS, offset)] = value
    for offset, value in stub_patch:
        memory[physical(STUB_SEG, offset)] = value
    # ECL operand stream read through DS:49DE + 6700h + PC; DS:49D2 the string window for tag 81h
    put16(memory, DS, 0x49DE, 0x1000); put16(memory, DS, 0x49E0, 0x6300)
    put16(memory, DS, 0x49D2, 0x1000); put16(memory, DS, 0x49D4, 0x6000)
    put16(memory, DS, 0x49ED, 0x9900)
    for i, b in enumerate(stream(spec["operands"]) + [0x41, 0x42, 0x43]):
        memory[physical(0x6300, 0x1001 + i)] = b
    put16(memory, 0x6000, 0x1000, 0x51); put16(memory, 0x6000, 0x1002, 0)
    frame = physical(ss, sp)
    for i, value in enumerate([RET_IP, RET_CS]):
        put16(memory, ss, (sp + 2 * i) & 0xFFFF, value)
        put16(memory, 0, (frame + 2 * i) & (SIZE - 1), value)
    reg = tuple(rng.randrange(0x10000) for _ in range(7))
    return memory, Cpu(*reg, sp, CALLER_CS, ENTRY, DS, rng.randrange(0x10000), ss, flags), ss >= 0xF000


def compare(label, memory, cpu, high, target, function, max_operands=64, value_stub=None, budget=3000000):
    from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
    original = Uc(UC_ARCH_X86, UC_MODE_16)
    original.mem_map(0, 2 * SIZE)
    original.mem_write(0, bytes(memory))
    original.mem_write(SIZE, bytes(memory))
    for (name, _), register in zip(Cpu._fields_, REGISTERS):
        original.reg_write(register, getattr(cpu, name))
    base = physical(CALLER_CS, 0)
    body = (base + ENTRY, base + END)
    header = physical(U25_CS, OPERAND_HEADER)
    callee_regions = [(physical(U25_CS, 0), physical(U25_CS, 0) + 0x10000), (physical(SYSTEM_CS, 0), physical(SYSTEM_CS, 0) + 0x2000),
                      (physical(STUB_SEG, 0x0025), physical(STUB_SEG, 0x002F))]
    if value_stub is not None:
        callee_regions.append((physical(value_stub, 0x0025), physical(value_stub, 0x002A)))
    state = dict(stop=None, expected=None, in_callee=False, returning=False, previous=None, headers=0)
    visited, edges = set(), set()

    def site_good(emulator, at, offset, ip):
        raw = bytes(emulator.mem_read(at, 5))
        if raw[:3] != struct.pack("<BH", 0x9A, offset):
            return False
        if ip is None:
            return True
        stub = physical(raw[3] | raw[4] << 8, offset)
        return bytes(emulator.mem_read(stub, 3)) == struct.pack("<BH", 0xEA, ip)

    def halt(emulator, stop):
        state["stop"] = stop
        emulator.emu_stop()

    def hook(emulator, address, size, user):
        del size, user
        if state["returning"]:
            return halt(emulator, OK)
        inside_body = body[0] <= address < body[1]
        if not inside_body:
            if not any(lo <= address < hi for lo, hi in callee_regions):
                return halt(emulator, TRANSFER)
            if address == header:
                state["headers"] += 1
                if state["headers"] > max_operands:
                    return halt(emulator, CALLEE)
            state["in_callee"] = True
            state["previous"] = None
            return None
        if state["in_callee"]:
            state["in_callee"] = False
            if address != state["expected"]:
                return halt(emulator, TRANSFER)
        here = address - base
        if here in SITES:
            offset, ip = SITES[here]
            if not site_good(emulator, address, offset, ip):
                return halt(emulator, CODE)
            state["expected"] = address + 5
        visited.add(here)
        if state["previous"] in target["conditional"] and state["previous"] != here:
            edges.add((state["previous"], here))
        state["previous"] = here
        if here == BODY_RETF:
            state["returning"] = True
        return None

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
    callee, callee_status = ctypes.c_int(99), ctypes.c_int(99)
    limits = Limits(SYSTEM_CS, max_operands, 4000)
    status = function(ram, SIZE, ctypes.byref(cpu), ctypes.byref(limits), ctypes.byref(callee), ctypes.byref(callee_status))
    check(status == state["stop"], f"C status {status} (callee {callee.value}/{callee_status.value}), original stopped with "
          f"{NAMES[state['stop']]}: {label}")
    check(state["stop"] != CALLEE or (callee.value == 1 and callee_status.value == 2), f"C callee {callee.value}/{callee_status.value}: {label}")
    actual = bytes(ram)
    if actual != expected:
        differences = [i for i, (a, b) in enumerate(zip(actual, expected)) if a != b]
        raise ValueError(f"RAM mismatch {label}: {[(hex(d), actual[d], expected[d]) for d in differences[:6]]} ({len(differences)} bytes)")
    for (name, _), register in zip(Cpu._fields_, REGISTERS):
        mine, want = getattr(cpu, name), original.reg_read(register) & 0xFFFF
        check(mine == want, f"{name} mismatch {label}: C={mine:04x}, original={want:04x}")
    return state["stop"], visited, edges


def cases(rng):
    spec = []
    for i in range(160):
        spec.append((f"random-{i}", dict(spec=world_spec(rng), flags=(0x7202, 0x0602, 0x0ED7, 0x72D7)[i % 4], sp=rng.randrange(0x0800, 0xF000) & ~1)))
    for tags in ((0, 0, 0, 0), (2, 2, 2, 2), (0x81, 0x81, 0, 0), (0x10, 0x55, 0x10, 0x55), (0, 2, 0x81, 0x10)):
        for equal in (True, False):
            spec.append((f"tags-{'-'.join(f'{t:02x}' for t in tags)}-{int(equal)}", dict(spec=world_spec(rng, tags=tags, equal=equal))))
    for sp in (0x0016, 0x0400, 0xFFF0, 0xFFFC):                      # 0016h: FillChar's return frame across FFFFh (TRANSFER)
        spec.append((f"sp-{sp:04x}", dict(spec=world_spec(rng, tags=(0, 0, 0, 0)), sp=sp, ss=EDGE_SS)))
    spec.append(("ss-a20", dict(spec=world_spec(rng, tags=(0, 0, 0, 0)), sp=0x0400, ss=0xFFFF)))
    spec.append(("random-background", dict(spec=world_spec(rng), background=bytes(random.Random(0x1C0AED).randbytes(SIZE)))))
    spec.append(("code-fillchar", dict(spec=world_spec(rng), patch=((0x0AFF, 0x90),))))
    spec.append(("code-load-site", dict(spec=world_spec(rng), patch=((0x0B08, 0x2B),))))
    spec.append(("code-load-stub", dict(spec=world_spec(rng), stub_patch=((0x002B, 0x01),))))
    spec.append(("code-value-stub", dict(spec=world_spec(rng), stub_patch=((0x0026, 0x37),))))
    spec.append(("value-stub-over-frame", dict(spec=world_spec(rng, tags=(0, 0, 0, 0)), sp=0x7F2E, value_stub=0x2FF0)))
    for budget in (1, 2, 3):                                        # positive budgets (U25 rejects 0 as an argument)
        spec.append((f"operand-budget-{budget}", dict(spec=world_spec(rng, tags=(0, 0, 0, 0)), max_operands=budget)))
    return spec


def differential(target, code1c, code25, image, function):
    rng = random.Random(0x1C0AED)
    visited, edges, stops = set(), set(), {k: 0 for k in NAMES}
    for label, options in cases(rng):
        max_operands = options.pop("max_operands", 64)
        memory, cpu, high = build(code1c, code25, image, rng, options.pop("spec"), **options)
        stop, v, e = compare(f"+0AED {label}", memory, cpu, high, target, function, max_operands, options.get("value_stub"))
        stops[stop] += 1
        visited |= v
        edges |= e
    missing = sorted(hex(i.address) for i in target["decoded"] if i.address not in visited)
    reachable = {(s, o) for s, outs in target["conditional"].items() for o in outs}
    missing_edges = sorted((hex(s), hex(o)) for s, o in reachable - edges)
    check(not missing and not missing_edges, f"coverage incomplete: {missing} {missing_edges}")
    check(all(stops[k] for k in (CODE, CALLEE, TRANSFER)), f"a stop kind was not exercised: {stops}")
    print(f"U1C +0AED: {sum(stops.values())} original-byte cases passed ({stops[OK]} returns, {stops[CODE]} CODE, {stops[CALLEE]} CALLEE, "
          f"{stops[TRANSFER]} TRANSFER); full 1 MiB RAM and all 14 CPU words equal; {len(visited)}/{len(target['decoded'])} instruction "
          f"starts, {len(edges)}/{len(reachable)} conditional outcomes")
    return dict(stops={NAMES[k]: v for k, v in stops.items()}, starts=len(visited), outcomes=len(edges))


# ---- DOSBox-X synthetic oracle -------------------------------------------------------------------------
CALLER_N, U25_N, STUB_N, SYS_N, DATA_DS, SCRIPT_SEG = 0x8800, 0x7800, 0x7F00, 0x9000, 0x1900, 0x2C00
MAIN_AT, BLOBS_AT = 0x0300, 0x0F00
OUTPUT, HEADER, STACK_TOP = 0xC000, 0xC800, 0xF000
FRAME_LOW, FRAME_SIZE = 0xEF00, 0x100
DS_BYTES = [0x6F78 + i for i in range(6)] + [0x49ED, 0x49EE] + [base + k for base in (0x7006, 0x7046, 0x7086) for k in range(4)]
STREAM_SIZE = 12
CASE_SIZE = 2 + STREAM_SIZE + 2
RECORD_BYTES = 28 + FRAME_SIZE + len(DS_BYTES)
NATIVE_FLAGS = (0x0002, 0x00D7, 0x08D7, 0x0402, 0x0893, 0x0003)
ENTRY_REGS = dict(ax=0x1234, bx=0x3456, cx=0x5678, dx=0x789A, si=0x9ABC, di=0xBCDE, bp=0x7777)


def native_cases():
    rng = random.Random(0x1C0AEDD)
    cases = []
    for i in range(64):
        spec = world_spec(rng, tags=[rng.choice((0, 0, 0, 2, 0x10, 0x55)) for _ in range(4)], equal=i % 3 != 0)
        cases.append(dict(spec, flags=i % len(NATIVE_FLAGS)))
    return cases


def native_audit(target, code1c, code25, image, function):
    del target
    directory = ROOT / "resources/oracle-runs/u1c-0aed-synthetic"
    directory.mkdir(parents=True, exist_ok=True)
    unit = bytearray(code1c)
    for site, (offset, ip) in SITES.items():
        struct.pack_into("<H", unit, site + 3, STUB_N if ip is not None else SYS_N)
    fill = image[SYSTEM_CS * 16 + 0x16A8:SYSTEM_CS * 16 + 0x16BC]
    cases = native_cases()
    text = [".code16", ".intel_syntax noprefix", ".global _start", "_start:", "    jmp main", f".org {MAIN_AT - 0x100:#x}", "main:",
            "    mov ax,cs", "    mov ds,ax", "    mov es,ax", "    mov ss,ax", f"    mov sp,{STACK_TOP:#x}", "    cld"]
    for segment, offset, label, length in ((CALLER_N, 0, "unit1c", len(unit)), (U25_N, 0, "unit25", len(code25)), (SYS_N, 0x16A8, "fill", len(fill))):
        text += [f"    mov ax,{segment:#x}", "    mov es,ax", f"    mov si,offset {label}", f"    mov di,{offset:#x}", f"    mov cx,{length:#x}", "    rep movsb"]
    text += [f"    mov ax,{STUB_N:#x}", "    mov es,ax",
             "    mov byte ptr es:[0x25],0xea", "    mov word ptr es:[0x26],0x0136", f"    mov word ptr es:[0x28],{U25_N:#x}",
             "    mov byte ptr es:[0x2a],0xea", "    mov word ptr es:[0x2b],0x0000", f"    mov word ptr es:[0x2d],{U25_N:#x}",
             f"    mov ax,{DATA_DS:#x}", "    mov es,ax", "    mov word ptr es:[0x49de],0x1000", f"    mov word ptr es:[0x49e0],{SCRIPT_SEG:#x}",
             "    mov word ptr es:[0x49d2],0x1000", "    mov word ptr es:[0x49d4],0x6000",
             "    mov ax,cs", "    mov es,ax",
             f"    mov word ptr [{HEADER:#x}],cs", f"    mov word ptr [{HEADER + 2:#x}],offset returned",
             f"    mov word ptr [{HEADER + 4:#x}],{len(cases)}", f"    mov word ptr [{HEADER + 6:#x}],0",
             "    mov dx,offset filename", "    xor cx,cx", "    mov ah,0x3c", "    int 0x21", "    jc failure",
             "    mov word ptr [handle],ax", "    mov bx,ax", f"    mov dx,{HEADER:#x}", "    mov cx,8", "    mov ah,0x40", "    int 0x21", "    jc failure",
             "again:", "    cli", f"    mov sp,{STACK_TOP:#x}", "    cld", "    mov ax,cs", "    mov ds,ax", "    mov es,ax",
             "    mov al,0xa5", f"    mov di,{FRAME_LOW:#x}", f"    mov cx,{FRAME_SIZE}", "    rep stosb",
             "    mov ax,word ptr [index]", f"    mov bx,{CASE_SIZE}", "    mul bx", "    add ax,offset cases", "    mov si,ax",
             f"    mov ax,{DATA_DS:#x}", "    mov es,ax", "    mov word ptr es:[0x49ed],0x9900",
             "    mov al,0x5a", "    mov di,0x6f78", "    mov cx,6", "    rep stosb",
             "    mov di,0x7006", "    mov cx,4", "    rep stosb", "    mov di,0x7046", "    mov cx,4", "    rep stosb",
             "    mov di,0x7086", "    mov cx,4", "    rep stosb",
             f"    mov ax,{SCRIPT_SEG:#x}", "    mov es,ax", "    mov di,0x1001", "    push si", "    add si,2", f"    mov cx,{STREAM_SIZE}", "    rep movsb",
             "    pop si", "    mov ax,cs", "    mov es,ax",
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
             f"    mov ax,{DATA_DS:#x}", "    mov ds,ax"]
    for off in DS_BYTES:
        text += [f"    mov al,byte ptr [{off:#x}]", "    stosb"]
    text += ["    mov ax,cs", "    mov ds,ax", "    sti",
             "    mov bx,word ptr [handle]", f"    mov dx,{OUTPUT:#x}", f"    mov cx,{RECORD_BYTES}", "    mov ah,0x40", "    int 0x21", "    jc failure",
             f"    cmp ax,{RECORD_BYTES}", "    jne failure",
             "    inc word ptr [index]", "    mov ax,word ptr [index]", f"    cmp ax,{len(cases)}", "    jb again",
             "    mov bx,word ptr [handle]", "    mov ah,0x3e", "    int 0x21", "    mov ax,0x4c00", "    int 0x21",
             "failure:", "    mov ax,0x4c01", "    int 0x21",
             'filename: .asciz "RESULT.BIN"', "handle: .word 0", "index: .word 0", f"target_far: .word {ENTRY:#x}, {CALLER_N:#x}",
             "flags_table: .word " + ",".join(f"{f:#x}" for f in NATIVE_FLAGS)]
    fill_blob = BLOBS_AT
    u25_blob = (fill_blob + len(fill) + 0x1F) & ~0xF
    u1c_blob = (u25_blob + len(code25) + 0x1F) & ~0xF
    for label, blob, at in (("fill", fill, fill_blob), ("unit25", code25, u25_blob), ("unit1c", bytes(unit), u1c_blob)):
        text += [f".org {at - 0x100:#x}", f"{label}:"]
        for i in range(0, len(blob), 16):
            text.append("    .byte " + ",".join(f"{b:#x}" for b in blob[i:i + 16]))
    cases_at = u1c_blob + len(unit) + 0x10
    text += [f".org {cases_at - 0x100:#x}", "cases:"]
    for case in cases:
        raw = stream(case["operands"])
        check(len(raw) <= STREAM_SIZE, "operand stream too long")
        row = bytes([case["flags"], 0]) + bytes(raw + [0x41] * (STREAM_SIZE - len(raw))) + bytes(2)
        check(len(row) == CASE_SIZE, "case row size differs")
        text.append("    .byte " + ",".join(f"{b:#x}" for b in row))
    check(cases_at + CASE_SIZE * len(cases) <= OUTPUT, f"COM layout overlaps: cases end {cases_at + CASE_SIZE * len(cases):#x}")
    asm, obj = directory / "oracle.S", directory / "oracle.o"
    asm.write_text("\n".join(text) + "\n")
    subprocess.run(["as", "--32", str(asm), "-o", str(obj)], check=True)
    subprocess.run(["ld", "-m", "elf_i386", "-Ttext", "0x100", "--oformat", "binary", str(obj), "-o", str(directory / "ORACLE.COM")], check=True)
    check((directory / "ORACLE.COM").stat().st_size <= 0xBF00, "COM image too large")
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
    equal = 0
    for n, case in enumerate(cases):
        record = result[8 + n * RECORD_BYTES:8 + (n + 1) * RECORD_BYTES]
        memory = bytearray(SIZE)
        for seg, off, blob in ((CALLER_N, 0, bytes(unit)), (U25_N, 0, code25), (SYS_N, 0x16A8, fill)):
            at = physical(seg, off)
            memory[at:at + len(blob)] = blob
        memory[physical(STUB_N, 0x25):physical(STUB_N, 0x25) + 5] = struct.pack("<BHH", 0xEA, 0x0136, U25_N)
        memory[physical(STUB_N, 0x2A):physical(STUB_N, 0x2A) + 5] = struct.pack("<BHH", 0xEA, 0x0000, U25_N)
        put16(memory, DATA_DS, 0x49DE, 0x1000); put16(memory, DATA_DS, 0x49E0, SCRIPT_SEG)
        put16(memory, DATA_DS, 0x49D2, 0x1000); put16(memory, DATA_DS, 0x49D4, 0x6000)
        for i in range(FRAME_SIZE):
            memory[physical(segment, FRAME_LOW + i)] = 0xA5
        put16(memory, DATA_DS, 0x49ED, 0x9900)
        for off in [0x6F78 + i for i in range(6)] + [base + k for base in (0x7006, 0x7046, 0x7086) for k in range(4)]:
            memory[physical(DATA_DS, off)] = 0x5A
        raw = stream(case["operands"])
        for i, b in enumerate(raw + [0x41] * (STREAM_SIZE - len(raw))):
            memory[physical(SCRIPT_SEG, 0x1001 + i)] = b
        sp = STACK_TOP
        for value in (segment, returned):
            sp -= 2
            put16(memory, segment, sp, value)
        cpu = Cpu(*ENTRY_REGS.values(), sp, CALLER_N, ENTRY, DATA_DS, segment, segment, NATIVE_FLAGS[case["flags"]])
        ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
        callee, callee_status = ctypes.c_int(99), ctypes.c_int(99)
        limits = Limits(SYS_N, 64, 4000)
        label = f"native case {n} ({[o[0] for o in case['operands']]})"
        status = function(ram, SIZE, ctypes.byref(cpu), ctypes.byref(limits), ctypes.byref(callee), ctypes.byref(callee_status))
        check(status == 0, f"C status {status} ({callee.value}/{callee_status.value}) on {label}")
        for (name, _), want in zip(Cpu._fields_, struct.unpack_from("<14H", record)):
            check(getattr(cpu, name) == want, f"{label} {name}: C={getattr(cpu, name):04x}, DOS={want:04x}")
        got = bytes(ram[physical(segment, FRAME_LOW):physical(segment, FRAME_LOW) + FRAME_SIZE])
        got += bytes(ram[physical(DATA_DS, off)] for off in DS_BYTES)
        check(got == record[28:], f"{label} bytes differ")
        equal += ram[physical(DATA_DS, 0x6F78)] == 1
    digest = sha256(result).hexdigest()
    print(f"DOSBox-X normal-core/386 synthetic cases passed: {len(cases)} ({equal} equal); all 14 CPU words, the {FRAME_SIZE}-byte stack "
          f"window and {len(DS_BYTES)} data bytes; no mask")
    print(f"native result SHA-256 {digest}; COM segment {segment:04x}; U25 at {U25_N:04X} behind stub {STUB_N:04X}, FillChar at {SYS_N:04X}:16A8")
    return dict(cases=len(cases), equal=equal, result_sha256=digest)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--unicorn", action="store_true")
    parser.add_argument("--native", action="store_true")
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    target, code1c, code25, image = source()
    report = dict(target=dict(sha256=target["sha"], bytes=101, instructions=40, file_offset=target["file_offset"]))
    if args.unicorn or args.native:
        with tempfile.TemporaryDirectory(prefix="poolrad-u1c0aed-") as temp:
            function = library(Path(temp))
            if args.unicorn:
                report["unicorn"] = differential(target, code1c, code25, image, function)
            if args.native:
                report["native"] = native_audit(target, code1c, code25, image, function)
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(json.dumps(report, sort_keys=True, indent=2) + "\n")
    print(json.dumps(report, sort_keys=True))


if __name__ == "__main__":
    main()
