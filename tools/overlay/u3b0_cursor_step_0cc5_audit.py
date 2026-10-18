#!/usr/bin/env python3
"""Original-byte full-state parity for raw-CPU U3B0 +0CC5 with the strict U3B0 +0B03 (and its +0000) composed, plus a DOSBox-X oracle.

The differential loads unit 0x3B0 (so +0B03 and +0000 run live from their original bytes) and compares the entire
1 MiB RAM and all 14 CPU words between the original under Unicorn and the C model, with no mask. The original
decides each stop: its return (wherever RETF 4 goes), +0B03 returning elsewhere (TRANSFER), or a stop inside
+0B03 (CALLEE): its loop-back cap, +0000 returning elsewhere, or a relocated entry whose +0000 call does not reach
IP 0 (compared at +0000's first fetch). The C callee and leaf statuses must name the same inner stop. Media stays
private.
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
from u86_procvars_3413_audit import Cpu, ROOT, SIZE, EXE_SHA, OVR_SHA, REGISTERS, physical, put16

ENTRY, END, BODY_RETF, CALL_SITE, CALL_RETURN = 0x0CC5, 0x0D88, 0x0D85, 0x0D7F, 0x0D82
STEP, STEP_END, STEP_RETF = 0x0B03, 0x0C33, 0x0C30
LEAF_END, LEAF_RETF = 0x0067, 0x0064
LEAF_CALLS = {0x0B39: 0x0B3C, 0x0BC4: 0x0BC7}
LOOP_HEADS = {0x0B17: 0x0B99, 0x0B9F: 0x0C2A}          # +0B03's loop head -> the JMP that loops back to it
CALLER_CS, DS, SS, EDGE_SS = 0x8800, 0x1800, 0x2800, 0x2000
RET_CS, RET_IP = 0x9001, 0x7000
OK, CALLEE, TRANSFER = 0, 2, 3
STEP_OK, STEP_CALLEE, STEP_TRANSFER, STEP_LIMIT = 0, 2, 3, 4
NAMES = {OK: "return", CALLEE: "CALLEE", TRANSFER: "TRANSFER"}
INNER = {STEP_OK: "ok", STEP_CALLEE: "relocated +0000", STEP_TRANSFER: "+0000 RETF elsewhere", STEP_LIMIT: "+0B03 cap"}


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
    unit = {u["stub_para"]: u for u in find_units(exe[parse_mz(exe)["header_bytes"]:], ovr)}[0x3B0]
    code = ovr[unit["ovr_offset"]:unit["ovr_offset"] + unit["code_size"]]
    starts = sorted(f["rel"] for f in unit["functions"])
    check(0x0D9A == next(s for s in starts if s > ENTRY) and STEP_END == next(s for s in starts if s > STEP), "U3B0 +0CC5 or +0B03 bounds differ")
    body = code[ENTRY:END]
    decoded = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(body, ENTRY))
    calls = [(i.address, i.op_str) for i in decoded if "call" in i.mnemonic]
    check(len(body) == 195 and len(decoded) == 68 and sum(i.size for i in decoded) == 195 and decoded[-1].address == BODY_RETF
          and decoded[-1].op_str == "4" and calls == [(CALL_SITE, "0xb03")], f"U3B0 +0CC5 extent or calls differ: {calls}")
    step = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(code[STEP:STEP_END], STEP))
    leaf = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(code[0:LEAF_END], 0))
    check(step[-1].address == STEP_RETF and step[-1].op_str == "4" and leaf[-1].address == LEAF_RETF and leaf[-1].op_str == "6",
          "U3B0 +0B03 or +0000 RETF differs")
    target = dict(body=body, decoded=decoded, conditional=conditionals(decoded), sha=sha256(body).hexdigest(),
                  file_offset=unit["ovr_offset"] + ENTRY, step=step, leaf=leaf)
    print(f"U3B0 +0CC5 (195/68, PUSH CS; CALL +0B03, RETF 4) GAME file {target['file_offset']:#x} sha {target['sha']}; "
          f"{len(target['conditional'])} conditionals")
    return target, code


def library(directory):
    path = directory / "u3b0c.so"
    subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-O2", "-fPIC", "-shared", "-I", str(ROOT / "include"),
                    str(ROOT / "src/recovered/overlay_u3b0_cursor_step_0cc5_cpu.c"),
                    str(ROOT / "src/recovered/overlay_u3b0_step_scan_0b03_cpu.c"), str(ROOT / "src/recovered/overlay_u3b0_entry1_0000.c"),
                    "-o", str(path)], check=True, capture_output=True, text=True)
    function = ctypes.CDLL(str(path)).pr_u3b0_cursor_step_0cc5_cpu
    function.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t, ctypes.POINTER(Cpu), ctypes.c_uint,
                         ctypes.POINTER(ctypes.c_int), ctypes.POINTER(ctypes.c_int)]
    function.restype = ctypes.c_int
    return function


def world_spec(rng, *, nodes=None, flags=None, limit=None, index=None, w56=None, floor=None, direction=None):
    count = rng.randrange(1, 12) if nodes is None else nodes
    return dict(nodes=count, flags=flags or [rng.choice((0, 1, 1, 1, 0x80)) for _ in range(count)],
                limit=rng.randrange(-2, 12) if limit is None else limit, index=rng.randrange(-3, 14) if index is None else index,
                w56=rng.randrange(-2, 14) if w56 is None else w56, floor=rng.randrange(-2, 6) if floor is None else floor,
                direction=rng.choice((0, 1, 0xFF)) if direction is None else direction)


def build(code, rng, spec, *, sp=0x8000, ss=SS, link=None, x=None, node_seg=0x5000, cyclic=False, flags=0x7202, background=None,
          base=0x0000):
    memory = bytearray([0xA5]) * SIZE if background is None else bytearray(background)
    start = physical(CALLER_CS, base)
    memory[start:start + len(code)] = code
    link = ((sp + 0x80) & 0xFFFF) if link is None else link            # the parent's BP, above this routine's frame
    x = x or (0x6000, 0x0100)
    put16(memory, ss, (link - 0x59) & 0xFFFF, spec["limit"] & 0xFFFF)
    put16(memory, ss, (link - 0x56) & 0xFFFF, spec["w56"] & 0xFFFF)
    put16(memory, ss, (link + 0x0A) & 0xFFFF, x[1])
    put16(memory, ss, (link + 0x0C) & 0xFFFF, x[0])
    offsets = [0x0100 + 0x40 * k for k in range(spec["nodes"])]
    for k, off in enumerate(offsets):
        memory[physical(node_seg, off + 0x29)] = spec["flags"][k]
        nxt = offsets[k + 1] if k + 1 < len(offsets) else (offsets[0] if cyclic else 0)
        put16(memory, node_seg, off + 0x2A, nxt)
        put16(memory, node_seg, off + 0x2C, node_seg if nxt else 0)
    put16(memory, ss, (link + 0x14) & 0xFFFF, offsets[0] if offsets else 0)
    put16(memory, ss, (link + 0x16) & 0xFFFF, node_seg if offsets else 0)
    put16(memory, x[0], x[1], spec["index"] & 0xFFFF)                 # before the floor and frame: an X^ aliasing them starts as them
    put16(memory, DS, 0x6B29, spec["floor"] & 0xFFFF)
    frame = physical(ss, sp)
    for i, value in enumerate([RET_IP, RET_CS, link, spec["direction"] | (rng.randrange(256) << 8)]):
        put16(memory, ss, (sp + 2 * i) & 0xFFFF, value)
        put16(memory, 0, (frame + 2 * i) & (SIZE - 1), value)
    reg = tuple(rng.randrange(0x10000) for _ in range(7))
    high = any(seg >= 0xF000 for seg in (ss, x[0], node_seg))
    return memory, Cpu(*reg, sp, CALLER_CS, (base + ENTRY) & 0xFFFF, DS, rng.randrange(0x10000), ss, flags), high


def compare(label, memory, cpu, high, target, function, cap, base=0x0000, budget=6000000):
    from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
    original = Uc(UC_ARCH_X86, UC_MODE_16)
    original.mem_map(0, 2 * SIZE)
    original.mem_write(0, bytes(memory))
    original.mem_write(SIZE, bytes(memory))
    for (name, _), register in zip(Cpu._fields_, REGISTERS):
        original.reg_write(register, getattr(cpu, name))
    origin = physical(CALLER_CS, base)
    ranges = dict(body=(origin + ENTRY, origin + END), step=(origin + STEP, origin + STEP_END), leaf=(origin, origin + LEAF_END))
    tables = dict(body=target["conditional"], step=conditionals(target["step"]), leaf=conditionals(target["leaf"]))
    leaf_calls = {origin + site: origin + back for site, back in LEAF_CALLS.items()}
    jumps = {origin + jmp: origin + head for head, jmp in LOOP_HEADS.items()}
    state = dict(stop=None, inner=STEP_OK, leaf=-1, check=None, returning=False, previous=None, loopbacks=0, last=None, leaf_back=None)
    seen = {k: set() for k in ranges}
    edges = {k: set() for k in ranges}

    def halt(emulator, stop, inner=None):
        state["stop"] = stop
        if inner is not None:
            state["inner"] = inner
        emulator.emu_stop()

    def hook(emulator, address, size, user):
        del size, user
        last, state["last"] = state["last"], address
        if state["returning"]:
            return halt(emulator, OK)
        if state["check"] is not None:
            (expected, kind), state["check"] = state["check"], None
            if address != expected:
                return halt(emulator, TRANSFER) if kind == "step" else halt(emulator, CALLEE, STEP_TRANSFER)
        if last in jumps and address == jumps[last]:
            state["loopbacks"] += 1
            if state["loopbacks"] > cap:
                return halt(emulator, CALLEE, STEP_LIMIT)
        if last in leaf_calls:
            state["leaf_back"] = leaf_calls[last]
            if base != 0:                                            # the near CALL reached IP base, not 0: +0000's model rejects it
                state["leaf"] = 1
                return halt(emulator, CALLEE, STEP_CALLEE)
            state["leaf"] = 0
        for kind, (low, high_end) in ranges.items():
            if low <= address < high_end:
                here = address - origin
                seen[kind].add(here)
                previous = state["previous"]
                if previous is not None and previous[0] == kind and previous[1] in tables[kind] and previous[1] != here:
                    edges[kind].add((previous[1], here))
                state["previous"] = (kind, here)
                if kind == "body" and here == BODY_RETF:
                    state["returning"] = True
                elif kind == "step" and here == STEP_RETF:
                    state["check"] = (origin + CALL_RETURN, "step")
                elif kind == "leaf" and here == LEAF_RETF:
                    state["check"] = (state["leaf_back"], "leaf")
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
    original.emu_start(ranges["body"][0], 2 * SIZE, count=budget)
    check(state["stop"] is not None, f"original did not stop: {label}")
    expected = bytes(original.mem_read(0, SIZE))
    ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
    callee_status, leaf_status = ctypes.c_int(99), ctypes.c_int(99)
    status = function(ram, SIZE, ctypes.byref(cpu), cap, ctypes.byref(callee_status), ctypes.byref(leaf_status))
    check(status == state["stop"], f"C status {status}, original stopped with {NAMES[state['stop']]}: {label}")
    check(callee_status.value == state["inner"] and leaf_status.value == state["leaf"],
          f"C callee/leaf status {callee_status.value}/{leaf_status.value}, original {INNER[state['inner']]}/{state['leaf']}: {label}")
    actual = bytes(ram)
    if actual != expected:
        differences = [i for i, (a, b) in enumerate(zip(actual, expected)) if a != b]
        raise ValueError(f"RAM mismatch {label}: {[(hex(d), actual[d], expected[d]) for d in differences[:6]]} ({len(differences)} bytes)")
    for (name, _), register in zip(Cpu._fields_, REGISTERS):
        mine, want = getattr(cpu, name), original.reg_read(register) & 0xFFFF
        check(mine == want, f"{name} mismatch {label}: C={mine:04x}, original={want:04x}")
    return (state["stop"], state["inner"]), seen, edges


def cases(rng):
    spec = []
    for i in range(160):
        spec.append((f"random-{i}", dict(spec=world_spec(rng), flags=(0x7202, 0x0602, 0x0ED7, 0xF2D7)[i % 4],
                                          sp=rng.randrange(0x0400, 0xF000) & ~1)))
    for direction in (1, 0, 0xFF):                                   # each wraparound and clamp edge of the step
        for limit, index, w56, floor in ((5, 6, 40, 2), (9, 6, 7, 0), (9, 2, 40, 0), (4, 2, 40, 2), (20, 9, 6, 0), (3, 0x7FFF, 0x7FFF, -0x8000),
                                         (3, 0x7FFF, -0x7FFF, -0x8000), (0, 2, 4, 1), (-1, 2, 4, 1), (6, -0x8000, 9, 0)):
            spec.append((f"edge-d{direction}-{limit}-{index}-{w56}-{floor}",
                         dict(spec=world_spec(rng, nodes=12, flags=[1, 0] * 6, limit=limit, index=index, w56=w56, floor=floor,
                                              direction=direction))))
    for direction in (1, 0):                                         # walks that run to the limit after the step
        spec.append((f"walk-d{direction}", dict(spec=world_spec(rng, nodes=12, flags=[1] * 12, limit=6, index=3, w56=9, floor=0,
                                                                  direction=direction))))
    spec.append(("nil-head", dict(spec=world_spec(rng, nodes=0, flags=[], limit=3, index=0, direction=1))))
    spec.append(("cyclic", dict(spec=world_spec(rng, nodes=4, flags=[1] * 4, limit=10, index=1, w56=40, floor=0, direction=1), cyclic=True)))
    # X^ aliasing the parent's words, this frame's link, d, return slots and +0B03's return IP (TRANSFER)
    sp = 0x8000
    bp = sp - 2
    link = sp + 0x80
    aliases = dict(limit=(SS, (link - 0x59) & 0xFFFF), w56=(SS, (link - 0x56) & 0xFFFF), link=(SS, bp + 6), d=(SS, bp + 8),
                   own_return=(SS, bp + 2), step_return=(SS, bp - 16), pushed_ax=(SS, bp - 10), floor=(DS, 0x6B29))
    for name, x in aliases.items():
        for direction in (1, 0):
            spec.append((f"x-on-{name}-d{direction}", dict(spec=world_spec(rng, nodes=12, flags=[1] * 11 + [0], limit=5, index=2, w56=40,
                                                                             floor=0, direction=direction), x=x, cap=200)))
    for cap in (0, 2):
        spec.append((f"cap-{cap}", dict(spec=world_spec(rng, nodes=12, flags=[1] * 12, limit=8, index=0, w56=40, floor=0, direction=1), cap=cap)))
    for name, link in (("w56-on-step-count", 0x8040), ("parent-under-step-pushes", 0x803C)):   # +0B03's frame clobbers the parent words
        for direction in (1, 0):
            spec.append((f"{name}-d{direction}", dict(spec=world_spec(rng, nodes=12, flags=[1] * 12, limit=6, index=2, w56=9,
                                                                         direction=direction), link=link, cap=200)))
    for k, sp in enumerate((0x0080, 0x00A0, 0xFFB0, 0xFF90)):        # segment ends, keeping +0000's RETF frame off the boundary
        spec.append((f"sp-{sp:04x}", dict(spec=world_spec(rng, nodes=6, limit=4, direction=k & 1), sp=sp, ss=EDGE_SS)))
    spec.append(("ss-a20", dict(spec=world_spec(rng, nodes=6, limit=4, direction=1), sp=0x0400, ss=0xFFFF)))
    spec.append(("nodes-a20", dict(spec=world_spec(rng, nodes=6, limit=4, direction=1), node_seg=0xFFF8)))
    spec.append(("x-a20", dict(spec=world_spec(rng, nodes=6, limit=4, direction=0), x=(0xFFFF, 0x0108))))
    spec.append(("random-background", dict(spec=world_spec(rng, nodes=8, limit=5, direction=1), background=bytes(random.Random(0x3B00CC5).randbytes(SIZE)))))
    spec.append(("relocated-entry", dict(spec=world_spec(rng, nodes=6, limit=4, direction=1), base=0x1000)))
    spec.append(("relocated-no-walk", dict(spec=world_spec(rng, nodes=6, limit=0, direction=1), base=0x1000)))
    return spec


def differential(target, code, function):
    rng = random.Random(0x3B00CC5)
    seen = {k: set() for k in ("body", "step", "leaf")}
    edges = {k: set() for k in seen}
    stops = {}
    for label, options in cases(rng):
        cap = options.pop("cap", 1000)
        base = options.get("base", 0)
        memory, cpu, high = build(code, rng, options.pop("spec"), **options)
        stop, s, e = compare(f"+0CC5 {label}", memory, cpu, high, target, function, cap, base)
        stops[stop] = stops.get(stop, 0) + 1
        for kind in seen:
            seen[kind] |= s[kind]
            edges[kind] |= e[kind]
    missing = sorted(hex(i.address) for i in target["decoded"] if i.address not in seen["body"])
    reachable = {(s, o) for s, outs in target["conditional"].items() for o in outs}
    missing_edges = sorted((hex(s), hex(o)) for s, o in reachable - edges["body"])
    check(not missing and not missing_edges, f"body coverage incomplete: {missing} {missing_edges}")
    kinds = {(OK, STEP_OK), (TRANSFER, STEP_OK), (CALLEE, STEP_LIMIT), (CALLEE, STEP_CALLEE)}
    check(kinds <= set(stops), f"a stop kind was not exercised: {stops}")
    summary = ", ".join(f"{n} {NAMES[k[0]]}" + (f" ({INNER[k[1]]})" if k[0] == CALLEE else "") for k, n in sorted(stops.items()))
    print(f"U3B0 +0CC5: {sum(stops.values())} original-byte cases passed ({summary}); full 1 MiB RAM and all 14 CPU words equal; "
          f"{len(seen['body'])}/{len(target['decoded'])} instruction starts, {len(edges['body'])}/{len(reachable)} conditional outcomes; "
          f"+0B03 {len(seen['step'])}/{len(target['step'])} and +0000 {len(seen['leaf'])}/{len(target['leaf'])} starts")
    return dict(stops={f"{NAMES[k[0]]}/{INNER[k[1]]}": v for k, v in sorted(stops.items())}, starts=len(seen["body"]),
                outcomes=len(edges["body"]), step_starts=len(seen["step"]), leaf_starts=len(seen["leaf"]))


# ---- DOSBox-X synthetic oracle -------------------------------------------------------------------------
CALLER_N, NODE_SEG, X_SEG, DATA_DS = 0x8800, 0x2000, 0x2800, 0x1900
MAIN_AT, UNIT_BLOB, CASES_AT = 0x0300, 0x1000, 0x3000
OUTPUT, HEADER, STACK_TOP = 0xC000, 0xC800, 0xF000
FRAME_LOW, FRAME_SIZE, LINK = 0xEF00, 0x100, 0xEFF0
CASE_SIZE = 16 + 12
RECORD_BYTES = 28 + FRAME_SIZE + 2 + 2
NATIVE_FLAGS = (0x0002, 0x00D7, 0x08D7, 0x0402, 0x0893, 0x0003)
ENTRY_REGS = dict(ax=0x1234, bx=0x3456, cx=0x5678, dx=0x789A, si=0x9ABC, di=0xBCDE, bp=0x7777)


def native_cases():
    rng = random.Random(0x3B0CC5D)
    cases = []
    for i in range(96):
        s = world_spec(rng, nodes=12, flags=[rng.choice((0, 1, 1, 1, 1)) for _ in range(12)])
        cases.append(dict(s, flagsx=i % len(NATIVE_FLAGS), high=rng.randrange(256)))
    return cases


def native_audit(target, code, function):
    directory = ROOT / "resources/oracle-runs/u3b0-0cc5-synthetic"
    directory.mkdir(parents=True, exist_ok=True)
    cases = native_cases()
    text = [".code16", ".intel_syntax noprefix", ".global _start", "_start:", "    jmp main", f".org {MAIN_AT - 0x100:#x}", "main:",
            "    mov ax,cs", "    mov ds,ax", "    mov es,ax", "    mov ss,ax", f"    mov sp,{STACK_TOP:#x}", "    cld",
            f"    mov ax,{CALLER_N:#x}", "    mov es,ax", "    mov si,offset unit", "    xor di,di", f"    mov cx,{len(code):#x}", "    rep movsb",
            # the 12-node list at NODE_SEG:0100 + 40h k, linked in order, nil-terminated
            f"    mov ax,{NODE_SEG:#x}", "    mov es,ax"]
    for k in range(12):
        off = 0x0100 + 0x40 * k
        nxt = off + 0x40 if k < 11 else 0
        text += [f"    mov word ptr es:[{off + 0x2A:#x}],{nxt:#x}", f"    mov word ptr es:[{off + 0x2C:#x}],{NODE_SEG if nxt else 0:#x}"]
    text += ["    mov ax,cs", "    mov es,ax",
             f"    mov word ptr [{HEADER:#x}],cs", f"    mov word ptr [{HEADER + 2:#x}],offset returned",
             f"    mov word ptr [{HEADER + 4:#x}],{len(cases)}", f"    mov word ptr [{HEADER + 6:#x}],0",
             "    mov dx,offset filename", "    xor cx,cx", "    mov ah,0x3c", "    int 0x21", "    jc failure",
             "    mov word ptr [handle],ax", "    mov bx,ax", f"    mov dx,{HEADER:#x}", "    mov cx,8", "    mov ah,0x40", "    int 0x21", "    jc failure",
             # the low 64 bytes of physical memory, which a nil node pointer reads at 0000:0029
             "    push ds", "    xor ax,ax", "    mov ds,ax", "    xor dx,dx", "    mov cx,64", "    mov ah,0x40", "    int 0x21", "    pop ds", "    jc failure",
             "again:", "    cli", f"    mov sp,{STACK_TOP:#x}", "    cld", "    mov ax,cs", "    mov ds,ax", "    mov es,ax",
             "    mov al,0xa5", f"    mov di,{FRAME_LOW:#x}", f"    mov cx,{FRAME_SIZE}", "    rep stosb",
             "    mov ax,word ptr [index]", f"    mov bx,{CASE_SIZE}", "    mul bx", "    add ax,offset cases", "    mov si,ax",
             # node flags
             f"    mov ax,{NODE_SEG:#x}", "    mov es,ax"]
    for k in range(12):
        text += [f"    mov al,byte ptr [si+{16 + k}]", f"    mov byte ptr es:[{0x0100 + 0x40 * k + 0x29:#x}],al"]
    text += [f"    mov ax,{X_SEG:#x}", "    mov es,ax", "    mov ax,word ptr [si+2]", "    mov word ptr es:[0x0100],ax",
             f"    mov ax,{DATA_DS:#x}", "    mov es,ax", "    mov ax,word ptr [si+6]", "    mov word ptr es:[0x6b29],ax",
             "    mov ax,cs", "    mov es,ax",
             "    mov ax,word ptr [si]", f"    mov word ptr [{LINK - 0x59:#x}],ax", "    mov ax,word ptr [si+4]", f"    mov word ptr [{LINK - 0x56:#x}],ax",
             f"    mov word ptr [{LINK + 0x0A:#x}],0x0100", f"    mov word ptr [{LINK + 0x0C:#x}],{X_SEG:#x}",
             f"    mov word ptr [{LINK + 0x14:#x}],0x0100", f"    mov word ptr [{LINK + 0x16:#x}],{NODE_SEG:#x}",
             f"    mov sp,{LINK - 0x60:#x}",
             "    push word ptr [si+8]", f"    mov ax,{LINK:#x}", "    push ax",
             "    mov bl,byte ptr [si+10]", "    xor bh,bh", "    shl bx,1", "    push word ptr [flags_table+bx]", "    popf",
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
             f"    mov ax,{X_SEG:#x}", "    mov ds,ax", "    mov si,0x0100", "    movsw",
             f"    mov ax,{DATA_DS:#x}", "    mov ds,ax", "    mov si,0x6b29", "    movsw",
             "    mov ax,cs", "    mov ds,ax", "    sti",
             "    mov bx,word ptr [handle]", f"    mov dx,{OUTPUT:#x}", f"    mov cx,{RECORD_BYTES}", "    mov ah,0x40", "    int 0x21", "    jc failure",
             f"    cmp ax,{RECORD_BYTES}", "    jne failure",
             "    inc word ptr [index]", "    mov ax,word ptr [index]", f"    cmp ax,{len(cases)}", "    jb again",
             "    mov bx,word ptr [handle]", "    mov ah,0x3e", "    int 0x21", "    mov ax,0x4c00", "    int 0x21",
             "failure:", "    mov ax,0x4c01", "    int 0x21",
             'filename: .asciz "RESULT.BIN"', "handle: .word 0", "index: .word 0", f"target_far: .word {ENTRY:#x}, {CALLER_N:#x}",
             "flags_table: .word " + ",".join(f"{f:#x}" for f in NATIVE_FLAGS)]
    text += [f".org {UNIT_BLOB - 0x100:#x}", "unit:"]
    for i in range(0, len(code), 16):
        text.append("    .byte " + ",".join(f"{b:#x}" for b in code[i:i + 16]))
    text += [f".org {CASES_AT - 0x100:#x}", "cases:"]
    for case in cases:
        row = struct.pack("<hhhhHBB", case["limit"], case["index"], case["w56"], case["floor"], case["direction"] | (case["high"] << 8),
                          case["flagsx"], 0)
        row += bytes(4) + bytes(case["flags"][:12])
        check(len(row) == CASE_SIZE, "case row size differs")
        text.append("    .byte " + ",".join(f"{b:#x}" for b in row))
    check(UNIT_BLOB + len(code) <= CASES_AT and CASES_AT + CASE_SIZE * len(cases) <= OUTPUT, "COM layout overlaps")
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
    check(len(result) == 8 + 64 + len(cases) * RECORD_BYTES, f"native output incomplete: {len(result)} bytes")
    segment, returned, count, _ = struct.unpack_from("<4H", result)
    check(count == len(cases), "native header differs")
    low = result[8:72]
    for n, case in enumerate(cases):
        record = result[72 + n * RECORD_BYTES:72 + (n + 1) * RECORD_BYTES]
        memory = bytearray(SIZE)
        memory[0:64] = low
        memory[physical(CALLER_N, 0):physical(CALLER_N, 0) + len(code)] = code
        for k in range(12):
            off = 0x0100 + 0x40 * k
            nxt = off + 0x40 if k < 11 else 0
            put16(memory, NODE_SEG, off + 0x2A, nxt)
            put16(memory, NODE_SEG, off + 0x2C, NODE_SEG if nxt else 0)
            memory[physical(NODE_SEG, off + 0x29)] = case["flags"][k]
        put16(memory, X_SEG, 0x0100, case["index"] & 0xFFFF)
        put16(memory, DATA_DS, 0x6B29, case["floor"] & 0xFFFF)
        for i in range(FRAME_SIZE):
            memory[physical(segment, FRAME_LOW + i)] = 0xA5
        put16(memory, segment, LINK - 0x59, case["limit"] & 0xFFFF)
        put16(memory, segment, LINK - 0x56, case["w56"] & 0xFFFF)
        put16(memory, segment, LINK + 0x0A, 0x0100)
        put16(memory, segment, LINK + 0x0C, X_SEG)
        put16(memory, segment, LINK + 0x14, 0x0100)
        put16(memory, segment, LINK + 0x16, NODE_SEG)
        sp = LINK - 0x60
        for value in (case["direction"] | (case["high"] << 8), LINK, segment, returned):
            sp -= 2
            put16(memory, segment, sp, value)
        cpu = Cpu(*ENTRY_REGS.values(), sp, CALLER_N, ENTRY, DATA_DS, segment, segment, NATIVE_FLAGS[case["flagsx"]])
        ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
        callee, leaf = ctypes.c_int(99), ctypes.c_int(99)
        label = f"native case {n}"
        check(function(ram, SIZE, ctypes.byref(cpu), 1000, ctypes.byref(callee), ctypes.byref(leaf)) == 0 and callee.value == 0,
              f"C status on {label}")
        for (name, _), want in zip(Cpu._fields_, struct.unpack_from("<14H", record)):
            check(getattr(cpu, name) == want, f"{label} {name}: C={getattr(cpu, name):04x}, DOS={want:04x}")
        got = bytes(ram[physical(segment, FRAME_LOW):physical(segment, FRAME_LOW) + FRAME_SIZE])
        got += bytes(ram[physical(X_SEG, 0x0100):physical(X_SEG, 0x0100) + 2]) + bytes(ram[physical(DATA_DS, 0x6B29):physical(DATA_DS, 0x6B29) + 2])
        check(got == record[28:], f"{label} bytes differ")
    digest = sha256(result).hexdigest()
    walked = sum(1 for case in cases if (case["limit"] & 0xFFFF) and case["limit"] > 0)
    print(f"DOSBox-X normal-core/386 synthetic cases passed: {len(cases)} ({walked} reach +0B03's walk); all 14 CPU words, "
          f"the {FRAME_SIZE:#x}-byte stack window (this frame, +0B03's and +0000's, and the parent words), the index word and DS:6B29; no mask")
    print(f"native result SHA-256 {digest}; COM segment {segment:04x}")
    return dict(cases=len(cases), result_sha256=digest)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--unicorn", action="store_true")
    parser.add_argument("--native", action="store_true")
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    target, code = source()
    report = dict(target=dict(sha256=target["sha"], bytes=195, instructions=68, file_offset=target["file_offset"]))
    if args.unicorn or args.native:
        with tempfile.TemporaryDirectory(prefix="poolrad-u3b0c-") as temp:
            function = library(Path(temp))
            if args.unicorn:
                report["unicorn"] = differential(target, code, function)
            if args.native:
                report["native"] = native_audit(target, code, function)
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(json.dumps(report, sort_keys=True, indent=2) + "\n")
    print(json.dumps(report, sort_keys=True))


if __name__ == "__main__":
    main()
