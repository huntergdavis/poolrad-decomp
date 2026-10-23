#!/usr/bin/env python3
"""Original-byte full-state parity for raw-CPU U68 +04DF, composing the U9E entry 8 dice chain through stub 00B0:0048 and
the raw U68 +0109.

The differential loads unit 0x68 at its original offsets, the original unit 0x9E +0DE3..+0E51 span, the original SYSTEM
RANDOM bytes at 0AF8:11C0 and the loaded-state stub EA 0DE3 at 00B0:0048 into one 1 MiB image (the layout of the merged
U33 dice-chain audits), and compares the entire RAM and all 14 CPU words between the original under Unicorn and the C
model, with no mask. The original decides each stop: the fetch after its RETF (return), the +0513 LCALL fetched with
other bytes (CODE), or a fetch of +04F6 after more taken JMP 04F6 loop-backs than the host cap (LIMIT). Any fetch outside
the legal spans is an error. Every case keeps its stack frame apart from the data. Media stays private.
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
from u86_procvars_3413_audit import Cpu, ROOT, SIZE, EXE_SHA, OVR_SHA, REGISTERS, physical, put16
import u33_grid_pair_0d13_audit as dice

CALLER_CS, DICE_CS, DS, SS = 0x3000, 0x4000, 0x1000, 0x2000
ENTRY, END, BODY_RETF, SITE, LOOP_JMP, LOOP_HEAD = 0x04DF, 0x05AB, 0x05AA, 0x0513, 0x0543, 0x04F6
MARK_SPAN = (0x0109, 0x0236)
RET_CS, RET_IP = 0x9001, 0x7000
OK, CODE, CALLEE, TRANSFER, LIMIT = 0, 2, 3, 4, 5
NAMES = {OK: "return", CODE: "CODE", CALLEE: "CALLEE", TRANSFER: "TRANSFER", LIMIT: "LIMIT"}
FLAGS = (0x0602, 0x0202, 0x0ED7, 0x0647, 0x72D7, 0x0403, 0x0213)


def check(condition, message):
    if not condition:
        raise ValueError(message)


def source():
    exe = (ROOT / "resources/DOS/POOLRAD/START.EXE").read_bytes()
    ovr = (ROOT / "resources/DOS/POOLRAD/GAME.OVR").read_bytes()
    check(sha256(exe).hexdigest() == EXE_SHA and len(ovr) == 206105 and sha256(ovr).hexdigest() == OVR_SHA, "media provenance differs")
    u = next(u for u in find_units(exe[parse_mz(exe)["header_bytes"]:], ovr) if u["stub_para"] == 0x68)
    code = ovr[u["ovr_offset"]:u["ovr_offset"] + u["code_size"]]
    insns = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(code[ENTRY:END], ENTRY))
    calls = [(i.address, i.op_str) for i in insns if "call" in i.mnemonic]
    check(len(insns) == 81 and sum(i.size for i in insns) == END - ENTRY and insns[-1].address == BODY_RETF and insns[-1].mnemonic == "retf"
          and calls == [(SITE, "0xb0, 0x48"), (0x055D, "0x109"), (0x059B, "0x109")] and SITE + 3 in set(u["fixups"]), f"U68 +04DF differs: {calls}")
    _, chain = dice.source()
    conditional = {i.address: (i.address + i.size, int(i.op_str, 16)) for i in insns if i.mnemonic.startswith("j") and i.mnemonic != "jmp"}
    sha = sha256(code[ENTRY:END]).hexdigest()
    print(f"U68 +04DF (204/81, LCALL 00B0:0048, two PUSH CS; CALL +0109, RETF) GAME file {u['ovr_offset'] + ENTRY:#x} sha {sha}")
    return dict(conditional=conditional, starts=[i.address for i in insns], sha=sha, file_offset=u["ovr_offset"] + ENTRY), code, chain


CHECKER_ROWS = ('OVL_0068_04DF',)


def library(directory):
    path = directory / "u6804df.so"
    sources = ["overlay_u68_movement_flag_04df_cpu.c", "overlay_u68_map_update_0109_cpu.c",
               "overlay_u9e_entry8_fullstate_compose_0de3.c", "overlay_u9e_entry8_fullstate_prefix_0de3.c",
               "overlay_u9e_entry8_fullstate_loop_0e05.c", "overlay_u9e_entry8_fullstate_postcall_0e13.c",
               "overlay_u9e_entry8_fullstate_tail_0e1f.c", "overlay_u9e_entry8_system_call_0e0e.c",
               "system_random_fullstate_11c0.c", "system_seed_fullstate_120f.c"]
    built = subprocess.run(["cc", "-std=c11", "-O2", "-fPIC", "-shared", "-I", str(ROOT / "include"),
                            *[str(ROOT / "src/recovered" / s) for s in sources], "-o", str(path)], capture_output=True, text=True)
    check(built.returncode == 0, built.stderr[-2000:])
    function = ctypes.CDLL(str(path)).pr_u68_movement_flag_04df_cpu
    function.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t, ctypes.POINTER(Cpu), ctypes.c_uint, ctypes.POINTER(ctypes.c_int)]
    function.restype = ctypes.c_int
    return function


def build(code, chain, rng, *, members, seed, x, y, d, map_at=(0x6000, 0x0100), cycle=False, sp=0x8000, ss=SS, flags=0x7202,
          patch=(), background=None):
    memory = bytearray([0xA5]) * SIZE if background is None else bytearray(background)
    at = physical(CALLER_CS, 0); memory[at:at + len(code)] = code
    for offset, value in patch:
        memory[physical(CALLER_CS, offset)] = value
    at = physical(DICE_CS, dice.U9E_SPAN[0]); memory[at:at + len(chain["dice"])] = chain["dice"]
    at = physical(dice.SYSTEM_CS, dice.SYSTEM_OFFSET); memory[at:at + len(chain["system"])] = chain["system"]
    at = physical(dice.STUB_PARAGRAPH, dice.STUB_OFFSET); memory[at:at + 5] = struct.pack("<BHH", 0xEA, dice.ENTRY8, DICE_CS)
    put16(memory, DS, 0x4428, seed & 0xFFFF); put16(memory, DS, 0x442A, seed >> 16)
    head = members[0][:2] if members else (0, 0)
    put16(memory, DS, 0x5D96, head[1]); put16(memory, DS, 0x5D98, head[0])
    for k, (seg, off, skill, busy) in enumerate(members):
        nxt = members[k + 1][:2] if k + 1 < len(members) else (members[0][:2] if cycle else (0, 0))
        memory[physical(seg, (off + 0x78) & 0xFFFF)] = skill
        memory[physical(seg, (off + 0x10C) & 0xFFFF)] = busy
        put16(memory, seg, (off + 0x104) & 0xFFFF, nxt[1]); put16(memory, seg, (off + 0x106) & 0xFFFF, nxt[0])
    memory[physical(DS, 0x6AAD)] = x; memory[physical(DS, 0x6AAE)] = y; memory[physical(DS, 0x6AAF)] = d
    for i in range(0x110):
        memory[physical(DS, 0x2736 + i)] = rng.choice((0x00, 0x01, 0xFF))
    put16(memory, DS, 0x6A5C, map_at[1]); put16(memory, DS, 0x6A5E, map_at[0])
    for i in range(0x100):
        memory[physical(map_at[0], (map_at[1] + 0x300 + i) & 0xFFFF)] = rng.randrange(256)
    frame = physical(ss, sp)
    for i, value in enumerate((RET_IP, RET_CS)):
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
    legal = [(physical(CALLER_CS, MARK_SPAN[0]), physical(CALLER_CS, MARK_SPAN[1])),
             (physical(DICE_CS, dice.U9E_SPAN[0]), physical(DICE_CS, dice.U9E_SPAN[1])),
             (physical(dice.SYSTEM_CS, dice.SYSTEM_OFFSET), physical(dice.SYSTEM_CS, dice.SYSTEM_OFFSET + 0x85)),
             (physical(dice.STUB_PARAGRAPH, dice.STUB_OFFSET), physical(dice.STUB_PARAGRAPH, dice.STUB_OFFSET + 5))]
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
            if here == LOOP_HEAD and state["previous"] == LOOP_JMP:
                state["loopbacks"] += 1
                if state["loopbacks"] > cap:
                    state["stop"] = LIMIT
                    return emulator.emu_stop()
            if here == SITE and bytes(emulator.mem_read(address, 3)) != b"\x9a\x48\x00":
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
    original.emu_start(start, 2 * SIZE, count=4000000)
    check(state["illegal"] is None and state["stop"] is not None, f"original did not stop cleanly ({state['illegal']}): {label}")
    expected = bytes(original.mem_read(0, SIZE))
    ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
    chain_status = ctypes.c_int(99)
    status = function(ram, SIZE, ctypes.byref(cpu), cap, ctypes.byref(chain_status))
    check(status == state["stop"], f"C status {NAMES.get(status, status)} ({chain_status.value}), original {NAMES[state['stop']]}: {label}")
    actual = bytes(ram)
    if actual != expected:
        differences = [i for i, (a, b) in enumerate(zip(actual, expected)) if a != b]
        raise ValueError(f"RAM mismatch {label}: {[(hex(d), actual[d], expected[d]) for d in differences[:6]]} ({len(differences)} bytes)")
    for (name, _), register in zip(Cpu._fields_, REGISTERS):
        mine, want = getattr(cpu, name), original.reg_read(register) & 0xFFFF
        check(mine == want, f"{name} mismatch {label}: C={mine:04x}, original={want:04x} ({NAMES[state['stop']]})")
    return state["stop"], visited, edges


def party(rng, count):
    return [(0x5000 + 0x200 * k, rng.choice((0x0000, rng.randrange(0x0000, 0x1000, 2))), rng.choice((0, 5, 50, 99, 100, 0xFF, rng.randrange(256))),
             rng.choice((0, 0, 0, 1))) for k in range(count)]


def cases(rng):
    spec = []
    for i in range(150):
        spec.append((f"random-{i}", dict(members=party(rng, rng.randrange(0, 7)), seed=rng.randrange(1 << 32),
                                         x=rng.choice((0, 3, 15, 16, 0xFF, rng.randrange(16))), y=rng.choice((0, 7, 15, 0x80, rng.randrange(16))),
                                         d=rng.choice((0, 2, 4, 6, 1, 7, 0xFF, rng.randrange(256))),
                                         map_at=(0x6000, rng.choice((0x0100, rng.randrange(0x0000, 0xF000), 0xFD00))),
                                         flags=FLAGS[i % len(FLAGS)], sp=rng.randrange(0x0800, 0xF000) & ~1)))
    spec.append(("empty-party", dict(members=[], seed=1, x=3, y=4, d=2)))
    spec.append(("all-busy", dict(members=[(0x5000, 0, 0xFF, 1), (0x5200, 0, 0xFF, 1)], seed=7, x=3, y=4, d=2)))
    spec.append(("sure-success", dict(members=[(0x5000, 0, 0xFF, 0)], seed=99, x=3, y=4, d=2)))
    spec.append(("cyclic-party", dict(members=[(0x5000, 0, 0, 1), (0x5200, 0, 0, 1)], seed=5, x=3, y=4, d=2, cycle=True, cap=9)))
    spec.append(("cap-zero", dict(members=[(0x5000, 0, 0, 1), (0x5200, 0, 0, 1)], seed=5, x=3, y=4, d=2, cap=0)))
    spec.append(("code-site", dict(members=[(0x5000, 0, 0xFF, 0)], seed=5, x=3, y=4, d=2, patch=((SITE + 1, 0x49),))))
    spec.append(("ss-a20", dict(members=[(0x5000, 0, 0xFF, 0)], seed=11, x=3, y=4, d=6, sp=0x0800, ss=0xFFFF)))
    spec.append(("df-set", dict(members=[(0x5000, 0, 0xFF, 0)], seed=13, x=3, y=4, d=0, flags=0x0603)))
    spec.append(("random-background", dict(members=party(rng, 3), seed=17, x=3, y=4, d=4, background=bytes(random.Random(0x6804DF).randbytes(SIZE)))))
    return spec


def differential(target, code, chain, function):
    rng = random.Random(0x6804DF)
    visited, edges, stops = set(), set(), {k: 0 for k in NAMES}
    for label, options in cases(rng):
        cap = options.pop("cap", 64)
        memory, cpu, high = build(code, chain, rng, **options)
        stop, v, e = compare(f"+04DF {label}", memory, cpu, high, target, function, cap)
        if not label.startswith("random"):
            print(f"  {label}: {NAMES[stop]}")
        stops[stop] += 1
        visited |= v; edges |= e
    reachable = {(s, o) for s, outs in target["conditional"].items() for o in outs}
    missing = sorted(hex(a) for a in target["starts"] if a not in visited)
    print(f"U68 +04DF: {sum(stops.values())} original-byte cases ({', '.join(f'{v} {NAMES[k]}' for k, v in stops.items())}); full 1 MiB RAM "
          f"and all 14 CPU words equal, no mask; {len(visited)}/{len(target['starts'])} instruction starts, {len(edges & reachable)}/{len(reachable)} "
          f"conditional outcomes")
    check(not missing and reachable <= edges, f"coverage incomplete: {missing} {sorted(reachable - edges)}")
    check(stops[OK] and stops[CODE] and stops[LIMIT], f"a stop kind was not exercised: {stops}")
    return dict(stops={NAMES[k]: v for k, v in stops.items()}, starts=len(visited), outcomes=len(edges & reachable))


# ---- DOSBox-X synthetic oracle ---------------------------------------------------------------------------
# SYSTEM RANDOM must sit at absolute 0AF8:11C0 (the merged dice chain hardcodes that paragraph): with the COM at segment
# 0814 that is COM offset 4000h, so the SYSTEM bytes are placed there in the COM image and nothing else may use it.
CALLER_N, DICE_N, STUB_N, DATA_N = 0x8800, 0x9000, 0x9800, 0x5000
MAIN_AT, CASES_AT, SYSTEM_AT, UNIT_AT, DICE_AT = 0x0300, 0x0C00, 0x4000, 0x4200, 0xB000
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
    seed = rng.randrange(1 << 32)
    w16(0x4428, seed & 0xFFFF); w16(0x442A, seed >> 16)
    members = [0x8000 + 0x200 * k for k in range(rng.randrange(0, 5))]
    w16(0x5D96, members[0] if members else 0); w16(0x5D98, DATA_N if members else 0)
    for k, m in enumerate(members):
        data[m + 0x78] = rng.choice((0, 50, 99, 0xFF)); data[m + 0x10C] = rng.choice((0, 0, 1))
        nxt = members[k + 1] if k + 1 < len(members) else 0
        w16(m + 0x104, nxt); w16(m + 0x106, DATA_N if nxt else 0)
    data[0x6AAD] = rng.choice((0, 3, 15, 16, rng.randrange(16))); data[0x6AAE] = rng.choice((0, 7, 15, 0xFF, rng.randrange(16)))
    data[0x6AAF] = rng.choice((0, 2, 4, 6, 1, 7, rng.randrange(8)))
    # only the 9 + 8 table bytes a direction 0..7 can read; the map stays at the A5 fill (the Unicorn half covers random maps),
    # which keeps every case within one CASE_SIZE slot of word writes
    for i in range(0x11):
        data[0x2736 + i] = rng.choice((0x00, 0x01, 0xFF))
    w16(0x6A5C, 0xC000); w16(0x6A5E, DATA_N)
    return data


def native_memory(unit, chain, system, data):
    memory = bytearray(SIZE)
    at = physical(CALLER_N, 0); memory[at:at + len(unit)] = unit
    at = physical(DICE_N, dice.U9E_SPAN[0]); memory[at:at + len(chain["dice"])] = chain["dice"]
    at = physical(STUB_N, dice.STUB_OFFSET); memory[at:at + 5] = struct.pack("<BHH", 0xEA, dice.ENTRY8, DICE_N)
    at = physical(dice.SYSTEM_CS, dice.SYSTEM_OFFSET); memory[at:at + len(system)] = system
    at = physical(DATA_N, 0); memory[at:at + 0x10000] = data
    return memory


def native_audit(code, chain, function):
    import os
    directory = ROOT / "resources/oracle-runs/u68-04df-synthetic"
    directory.mkdir(parents=True, exist_ok=True)
    unit = bytearray(code)
    struct.pack_into("<H", unit, SITE + 3, STUB_N)
    system = chain["system"]
    check(physical(0x0814, SYSTEM_AT) == physical(dice.SYSTEM_CS, dice.SYSTEM_OFFSET), "the SYSTEM hole is not at 0AF8:11C0")
    rng = random.Random(0x6804DFD)
    cases = []
    while len(cases) < NATIVE_CASES:
        data, flags = native_world(rng), len(cases) % len(NATIVE_FLAGS)
        memory = native_memory(unit, chain, system, data)
        sp = STACK_TOP - 4
        put16(memory, 0x0814, sp, 0); put16(memory, 0x0814, sp + 2, 0x0814)
        cpu = Cpu(*ENTRY_REGS.values(), sp, CALLER_N, ENTRY, DATA_N, 0x0814, 0x0814, NATIVE_FLAGS[flags])
        ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
        if function(ram, SIZE, ctypes.byref(cpu), 64, None) != OK:
            continue
        writes = [(o, data[o] | (data[o + 1] << 8)) for o in range(0, 0x10000, 2) if data[o] != 0xA5 or data[o + 1] != 0xA5]
        check(4 + 4 * len(writes) <= CASE_SIZE, f"native case needs {len(writes)} writes")
        cases.append(dict(flags=flags, data=data, writes=writes))
    table = b"".join((struct.pack("<BBH", c["flags"], 0, len(c["writes"])) + b"".join(struct.pack("<HH", o, v) for o, v in c["writes"])).ljust(CASE_SIZE, b"\0")
                     for c in cases)
    dice_blob = chain["dice"]
    check(CASES_AT + len(table) <= SYSTEM_AT and SYSTEM_AT + len(system) <= UNIT_AT and UNIT_AT + len(unit) <= DICE_AT
          and DICE_AT + len(dice_blob) <= OUTPUT, "COM layout overlaps")
    t = [".code16", ".intel_syntax noprefix", ".global _start", "_start:", "    jmp main", f".org {MAIN_AT - 0x100:#x}", "main:",
         "    mov ax,cs", "    mov ds,ax", "    mov es,ax", "    mov ss,ax", f"    mov sp,{STACK_TOP:#x}", "    cld"]
    for segment, label, offset, length in ((CALLER_N, "unit", 0, len(unit)), (DICE_N, "dice_span", dice.U9E_SPAN[0], len(dice_blob))):
        t += [f"    mov ax,{segment:#x}", "    mov es,ax", f"    mov si,offset {label}", f"    mov di,{offset:#x}", f"    mov cx,{length:#x}", "    rep movsb"]
    t += [f"    mov ax,{STUB_N:#x}", "    mov es,ax", f"    mov byte ptr es:[{dice.STUB_OFFSET:#x}],0xea", f"    mov word ptr es:[{dice.STUB_OFFSET + 1:#x}],{dice.ENTRY8:#x}",
          f"    mov word ptr es:[{dice.STUB_OFFSET + 3:#x}],{DICE_N:#x}",
          "    push cs", "    pop es", f"    mov word ptr [{HEADER:#x}],cs", f"    mov word ptr [{HEADER + 2:#x}],offset returned",
          f"    mov word ptr [{HEADER + 4:#x}],{len(cases)}", f"    mov word ptr [{HEADER + 6:#x}],0",
          "    mov dx,offset filename", "    xor cx,cx", "    mov ah,0x3c", "    int 0x21", "    jc failure",
          "    mov word ptr [handle],ax", "    mov bx,ax", f"    mov dx,{HEADER:#x}", "    mov cx,8", "    mov ah,0x40", "    int 0x21", "    jc failure",
          "again:", "    cli", f"    mov sp,{STACK_TOP:#x}", "    cld", "    push cs", "    pop ds", "    push cs", "    pop es",
          "    mov al,0xa5", f"    mov di,{WINDOW_LOW:#x}", f"    mov cx,{WINDOW_SIZE:#x}", "    rep stosb",
          "    mov ax,word ptr [index]", f"    mov bx,{CASE_SIZE}", "    mul bx", "    add ax,offset cases", "    mov bx,ax",
          f"    mov ax,{DATA_N:#x}", "    mov es,ax", "    xor di,di", "    mov cx,0x8000", "    mov ax,0xa5a5", "    rep stosw",
          "    mov cx,word ptr [bx+2]", "    jcxz written", "    lea si,[bx+4]", "write_loop:", "    mov di,word ptr [si]", "    mov ax,word ptr [si+2]",
          "    mov word ptr es:[di],ax", "    add si,4", "    loop write_loop", "written:",
          "    xor ah,ah", "    mov al,byte ptr [bx]", "    shl ax,1", "    mov si,ax",
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
    for label, blob, where in (("cases", table, CASES_AT), ("system", system, SYSTEM_AT), ("unit", bytes(unit), UNIT_AT), ("dice_span", dice_blob, DICE_AT)):
        t += [f".org {where - 0x100:#x}", f"{label}:"]
        for i in range(0, len(blob), 16):
            t.append("    .byte " + ",".join(f"{b:#x}" for b in blob[i:i + 16]))
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
    found = 0
    for n, case in enumerate(cases):
        record = result[8 + n * RECORD_BYTES:8 + (n + 1) * RECORD_BYTES]
        memory = native_memory(unit, chain, system, case["data"])
        for i in range(WINDOW_SIZE):
            memory[physical(segment, WINDOW_LOW + i)] = 0xA5
        sp = STACK_TOP
        for value in (segment, returned):
            sp -= 2; put16(memory, segment, sp, value)
        cpu = Cpu(*ENTRY_REGS.values(), sp, CALLER_N, ENTRY, DATA_N, segment, segment, NATIVE_FLAGS[case["flags"]])
        ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
        check(function(ram, SIZE, ctypes.byref(cpu), 64, None) == OK, f"C status on native case {n}")
        for (reg, _), want in zip(Cpu._fields_, struct.unpack_from("<14H", record)):
            check(getattr(cpu, reg) == want, f"native case {n} {reg}: C={getattr(cpu, reg):04x}, DOS={want:04x}")
        got = bytes(ram[physical(segment, WINDOW_LOW):physical(segment, WINDOW_LOW) + WINDOW_SIZE]) + bytes(ram[physical(DATA_N, 0):physical(DATA_N, 0) + 0x10000])
        if got != record[28:]:
            differences = [i for i, (a, b) in enumerate(zip(got, record[28:])) if a != b]
            raise ValueError(f"native case {n} bytes differ at {[hex(d) for d in differences[:8]]} ({len(differences)} bytes)")
        found += (cpu.ax & 0xFF) != 0
    digest = sha256(result).hexdigest()
    print(f"DOSBox-X normal-core/386 synthetic cases passed: {len(cases)} ({found} with a success); all 14 CPU words, the stack window and the "
          f"whole 64 KiB data segment; no mask")
    print(f"native result SHA-256 {digest}; COM segment {segment:04x}; SYSTEM at 0AF8:11C0 (COM 4000h), U9E at {DICE_N:04X}, stub at {STUB_N:04X}")
    return dict(cases=len(cases), successes=found, result_sha256=digest)


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
    target, code, chain = source()
    report = dict(target=dict(sha256=target["sha"], bytes=204, instructions=81, file_offset=target["file_offset"]))
    if args.unicorn or args.native:
        with tempfile.TemporaryDirectory(prefix="poolrad-u6804df-") as temp:
            function = library(Path(temp))
            if args.unicorn:
                report["unicorn"] = differential(target, code, chain, function)
            if args.native:
                report["native"] = native_audit(code, chain, function)
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(json.dumps(report, sort_keys=True, indent=2) + "\n")
    print(json.dumps(report, sort_keys=True))


if __name__ == "__main__":
    main()
