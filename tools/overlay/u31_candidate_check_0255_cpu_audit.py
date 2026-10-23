#!/usr/bin/env python3
"""Original-byte full-state parity for raw-CPU U31 +0255, composing U3C2 entry 6 (+0909, with its U3C6 +0000 pair
helper and SYSTEM Move) through the loaded stub 03DB:003E and the resident zero test 00BA:23B7, plus a DOSBox-X oracle.

The differential places the hash-checked expanded START.EXE image at load segment 0, so the resident routine and SYSTEM
Move run from their original bytes, patches both overlay stubs (03DB:003E and 03E0:0025) to their loaded EA forms, loads
units 0x3C2, 0x3C6 and 0x31, and compares the entire 1 MiB RAM and all 14 CPU words between the original under Unicorn
and the C model, with no mask. The original decides each stop: the fetch after its own RETF 8 (return), a call site or the
stub whose live bytes differ when fetched (CODE), a U3C2 budget point reached with that budget spent, counted as the
merged U3C2 audit counts it (CALLEE), or a RETF that does not come back to the call's next instruction (TRANSFER). Any
fetch outside the loaded code is an error. U3C2's world follows its merged audit fixture. Media stays private.
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

ENTRY, END, BODY_RETF = 0x0255, 0x02EB, 0x02E8
BODY_SHA = "908b407691e90b37e132380ee3ad17d42209a64ecbbb2d29ce0fca2333abf552"
RESIDENT_SHA = "1591e4ada52c8b030f52ef6631d5274e94a2fb1df636f9d58fce5a15cc743c50"
U3C2_SHA = "38a673986257d14dbe4da3ede438a98c0b516884c6fc92c79228088a68dba9ed"   # U3C2 [0909,0BD9)
U3C6_SHA = "05e4840e7db71675470a0c93a3741dafdde3f709d55970021096960a09506207"   # U3C6 [0000,0068)
MOVE_SHA = "5bf8af52bdcd4af1b7e73b160b3ee02e6de7812b7e686aae02f73d9ccbc061aa"   # SYSTEM 0AF8:0A3F..0A56
SITES = {0x0287: b"\x9a\x3e\x00", 0x02A9: b"\x9a\xb7\x23"}
STUB_SITE, U3C2_RETURN, RESIDENT_RETURN = 0x0287, 0x028C, 0x02AE
U3C2_STUB, PAIR_STUB = (0x03DB, 0x003E), (0x03E0, 0x0025)
RES_CS, RES_IP, RES_RETF, RES_END = 0x00BA, 0x23B7, 0x23D8, 0x23DB
U3C2_RETF, LOOPS = 0x0BD6, (0x918, 0x993, 0x9BA, 0xA48, 0xA61, 0xB73)
SYSTEM_CS = 0x0AF8
CALLER_CS, U3C2_CS, U3C6_CS, DS, SS, REC, MAP = 0x8800, 0x3000, 0x4000, 0x5000, 0x2000, 0x6000, 0x8000
RET_CS, RET_IP, ACTIVE = 0x9001, 0x7000, 0x1800
OK, CODE, CALLEE, TRANSFER = 0, 2, 3, 4
NAMES = {OK: "return", CODE: "CODE", CALLEE: "CALLEE", TRANSFER: "TRANSFER"}
FIELD = (0, 0, 1, 1, 2, 0x80, 0xFF)
FLAGS = (0x0602, 0x0202, 0x0ED7, 0x0647, 0x72D7, 0x0403, 0x0213)
BUDGETS = (20000, 512, 65536)


def check(condition, message):
    if not condition:
        raise ValueError(message)


def source():
    exe = (ROOT / "resources/DOS/POOLRAD/START.EXE").read_bytes()
    ovr = (ROOT / "resources/DOS/POOLRAD/GAME.OVR").read_bytes()
    check(len(exe) == 65231 and sha256(exe).hexdigest() == EXE_SHA, "START.EXE provenance differs")
    check(len(ovr) == 206105 and sha256(ovr).hexdigest() == OVR_SHA, "GAME.OVR provenance differs")
    image = bytes(unpack(exe)[0])
    units = {u["stub_para"]: u for u in find_units(exe[parse_mz(exe)["header_bytes"]:], ovr)}
    u31, u3c2, u3c6 = units[0x31], units[0x3C2], units[0x3C6]
    code31, code3c2, code3c6 = (ovr[u["ovr_offset"]:u["ovr_offset"] + u["code_size"]] for u in (u31, u3c2, u3c6))
    md = Cs(CS_ARCH_X86, CS_MODE_16)
    check(sha256(code3c2[0x0909:0x0BD9]).hexdigest() == U3C2_SHA and u3c2["entries"][6] == 0x0909 and u3c2["memory_segment"] == U3C2_STUB[0]
          and sha256(code3c6[0:0x68]).hexdigest() == U3C6_SHA and u3c6["entries"][1] == 0 and u3c6["memory_segment"] == PAIR_STUB[0],
          "U3C2 +0909 or U3C6 +0000 differs")
    check(sha256(image[physical(SYSTEM_CS, 0x0A3F):physical(SYSTEM_CS, 0x0A57)]).hexdigest() == MOVE_SHA, "SYSTEM Move differs")
    check(all(code3c2[s:s + 5] == b"\x9a\x3f\x0a\xf8\x0a" for s in (0x131, 0x160, 0x181))
          and all(code3c2[s:s + 5] == b"\x9a\x25\x00\xe0\x03" for s in (0x92A, 0x9D8)), "U3C2 far-call operands differ")
    for (segment, offset), cold in ((U3C2_STUB, b"\xcd\x3f\x09\x09\x00"), (PAIR_STUB, b"\xcd\x3f\x00\x00\x00")):
        check(image[physical(segment, offset):physical(segment, offset) + 5] == cold, f"cold stub {segment:04X}:{offset:04X} differs")
    resident = image[physical(RES_CS, RES_IP):physical(RES_CS, RES_END)]
    rd = list(md.disasm(resident, RES_IP))
    check(sha256(resident).hexdigest() == RESIDENT_SHA and len(rd) == 13 and rd[-1].address == RES_RETF and rd[-1].op_str == "4",
          "resident 00BA:23B7 differs")
    body = code31[ENTRY:END]
    decoded = list(md.disasm(body, ENTRY))
    calls = [(i.address, i.op_str) for i in decoded if "call" in i.mnemonic]
    conditional = {i.address: (i.address + i.size, int(i.op_str, 16)) for i in decoded if i.mnemonic.startswith("j") and i.mnemonic != "jmp"}
    whole = list(md.disasm(code31, 0))
    callers = [(i.address, whole[k - 1].mnemonic, whole[k - 1].op_str) for k, i in enumerate(whole) if i.mnemonic == "call" and i.op_str == "0x255"]
    check(sha256(body).hexdigest() == BODY_SHA and len(body) == 150 and len(decoded) == 58 and decoded[-1].address == BODY_RETF
          and decoded[-1].op_str == "8" and calls == [(0x0287, "0x3db, 0x3e"), (0x02A9, "0xba, 0x23b7")]
          and all(site + 3 in set(u31["fixups"]) for site in SITES) and len(conditional) == 3 and callers == [(0x03C6, "push", "cs")],
          f"U31 +0255 extent, calls or caller differ: {calls} {callers}")
    target = dict(decoded=decoded, conditional=conditional, file_offset=u31["ovr_offset"] + ENTRY)
    print(f"U31 +0255 (150/58, LCALL 03DB:003E to U3C2 +0909, LCALL 00BA:23B7, RETF 8; caller +03C6 PUSH CS/CALL) GAME file "
          f"{target['file_offset']:#x} sha {BODY_SHA}; resident 36/13 sha {RESIDENT_SHA[:16]}")
    return target, code31, code3c2, code3c6, image


CHECKER_ROWS = ('OVL_0031_0255',)


def library(directory):
    path = directory / "u310255.so"
    sources = ["overlay_u31_candidate_check_0255_cpu.c", "overlay_u3c2_caller_0909.c", "overlay_u3c6_table_pair_0000.c",
               "overlay_u3c2_record_sort_002e_cpu.c", "system_runtime_leaves_cpu.c", "overlay_u3c2_entry4_057b.c", "overlay_u3c2_entry5_0422.c",
               "overlay_u3c2_line_init_01a0.c", "overlay_u3c2_sign_entry1_0000.c", "overlay_u3c2_entry3_02cc.c"]
    subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-O2", "-fPIC", "-shared", "-I", str(ROOT / "include"),
                    *[str(ROOT / "src/recovered" / s) for s in sources], "-o", str(path)], check=True, capture_output=True, text=True)
    function = ctypes.CDLL(str(path)).pr_u31_candidate_check_0255_cpu
    function.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t, ctypes.POINTER(Cpu), ctypes.c_uint16, ctypes.c_size_t,
                         ctypes.c_size_t, ctypes.c_uint, ctypes.POINTER(ctypes.c_int), ctypes.POINTER(ctypes.c_int)]
    function.restype = ctypes.c_int
    return function


def world(memory, *, party, mode, heights, shape):
    """U3C2's DS world, as its merged audit fixture builds it."""
    for t in range(256):
        for i, (x, y) in enumerate(shape):
            memory[physical(DS, 0x2844 + 8 * t + 2 * i)] = x & 255; memory[physical(DS, 0x2845 + 8 * t + 2 * i)] = y & 255
    for i, v in enumerate((0, 1, 1, 1, 0, -1, -1, -1, 0)):
        memory[physical(DS, 0x2736 + i)] = v & 255
    for i, v in enumerate((-1, -1, 0, 1, 1, 1, 0, -1, 0)):
        memory[physical(DS, 0x273F + i)] = v & 255
    for i in range(64):
        memory[physical(DS, 0x2745 + 4 * i)] = heights[0]; memory[physical(DS, 0x2746 + 4 * i)] = heights[1]
    for i in range(9):
        memory[physical(DS, 0x25C0 + i)] = 0
    for i in range(0x1800):
        memory[physical(DS, MAP + i)] = 0
    memory[physical(DS, MAP + 6)] = mode
    memory[physical(DS, 0x5F2A)] = len(party)
    for i, (x, y, t) in enumerate(party, 1):
        memory[physical(DS, 0x5F27 + 4 * i)] = x & 255; memory[physical(DS, 0x5F28 + 4 * i)] = y & 255; memory[physical(DS, 0x5F2A + 4 * i)] = t


def build(code31, code3c2, code3c6, image, rng, *, party=((12, 12, 1),), origin=(10, 10), radius=100, row=None, fields=None, active=None,
          sp=0xF000, ss=SS, flags=0x0602, background=None, patch=(), stub=None, base=0, mode=1, heights=(0, 0),
          shape=((0, 0), (1, 0), (0, 1), (-1, 0)), writes=()):
    memory = bytearray([0xA5]) * SIZE if background is None else bytearray(background)
    memory[0:len(image)] = image
    at = physical(*U3C2_STUB); memory[at:at + 5] = struct.pack("<BHH", 0xEA, 0x0909, U3C2_CS) if stub is None else stub
    at = physical(*PAIR_STUB); memory[at:at + 5] = struct.pack("<BHH", 0xEA, 0x0000, U3C6_CS)
    for segment, code in ((U3C2_CS, code3c2), (U3C6_CS, code3c6)):
        at = physical(segment, 0); memory[at:at + len(code)] = code
    at = physical(CALLER_CS, base); memory[at:at + len(code31)] = code31
    for offset, value in patch:
        memory[physical(CALLER_CS, (base + offset) & 0xFFFF)] = value
    world(memory, party=party, mode=mode, heights=heights, shape=shape)
    row = rng.randrange(256) if row is None else row
    memory[physical(DS, 0x3209 + 16 * row)] = radius
    put16(memory, DS, 0x6716, MAP); put16(memory, DS, 0x6718, DS)
    fields = [rng.choice(FIELD) for _ in range(11)] if fields is None else fields
    for k, value in enumerate(fields):
        offset = 0x0200 * k + rng.randrange(16)
        put16(memory, DS, 0x65B9 + 4 * k, offset); put16(memory, DS, 0x65BB + 4 * k, REC)
        memory[physical(REC, offset + 0x10E)] = value
    put16(memory, DS, 0x5D92, ACTIVE); put16(memory, DS, 0x5D94, REC)
    memory[physical(REC, ACTIVE + 0x10E)] = rng.choice(FIELD) if active is None else active
    frame = physical(ss, sp)
    for i, value in enumerate((RET_IP, RET_CS, rng.randrange(0x10000), (rng.randrange(256) << 8) | row,
                               (rng.randrange(256) << 8) | (origin[1] & 255), (rng.randrange(256) << 8) | (origin[0] & 255))):
        put16(memory, ss, (sp + 2 * i) & 0xFFFF, value)
        put16(memory, 0, (frame + 2 * i) & (SIZE - 1), value)
    for segment, offset, data in writes:
        for j, value in enumerate(data):
            memory[physical(segment, (offset + j) & 0xFFFF)] = value
    reg = tuple(rng.randrange(0x10000) for _ in range(7))
    return memory, Cpu(*reg, sp, CALLER_CS, (base + ENTRY) & 0xFFFF, DS, rng.randrange(0x10000), ss, flags), ss >= 0xF000


def compare(label, memory, cpu, high, target, function, code3c2, code3c6, budgets=BUDGETS, base=0, count=20000000):
    from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
    from unicorn.x86_const import UC_X86_REG_CS, UC_X86_REG_IP
    original = Uc(UC_ARCH_X86, UC_MODE_16)
    original.mem_map(0, 2 * SIZE)
    original.mem_write(0, bytes(memory)); original.mem_write(SIZE, bytes(memory))
    for (name, _), register in zip(Cpu._fields_, REGISTERS):
        original.reg_write(register, getattr(cpu, name))
    origin = physical(CALLER_CS, base)
    body = (origin + ENTRY, origin + END)
    regions = [body, (physical(U3C2_CS, 0), physical(U3C2_CS, len(code3c2))), (physical(U3C6_CS, 0), physical(U3C6_CS, len(code3c6))),
               (physical(SYSTEM_CS, 0x0A3F), physical(SYSTEM_CS, 0x0A57)), (physical(RES_CS, RES_IP), physical(RES_CS, RES_END)),
               (physical(*U3C2_STUB), physical(*U3C2_STUB) + 5), (physical(*PAIR_STUB), physical(*PAIR_STUB) + 5)]
    expects = {physical(U3C2_CS, U3C2_RETF): (CALLER_CS, (base + U3C2_RETURN) & 0xFFFF),
               physical(RES_CS, RES_RETF): (CALLER_CS, (base + RESIDENT_RETURN) & 0xFFFF)}
    state = dict(stop=None, expect=None, stub=False, returned=False, previous=None, last=None, remaining=list(budgets),
                 sites={s: 0 for s in SITES}, unexpected=None)
    visited, edges = set(), set()

    def hook(emulator, address, size, user):
        del size, user
        cs, ip = emulator.reg_read(UC_X86_REG_CS), emulator.reg_read(UC_X86_REG_IP)
        if state["returned"]:
            state["stop"] = OK
            return emulator.emu_stop()
        if state["expect"] is not None:
            want = state["expect"]
            state["expect"] = None
            if (cs, ip) != want:
                state["stop"] = TRANSFER
                return emulator.emu_stop()
        if state["stub"]:
            state["stub"] = False
            if bytes(emulator.mem_read(address, 3)) != b"\xea\x09\x09":
                state["stop"] = CODE
                return emulator.emu_stop()
        if not any(low <= address < high for low, high in regions):
            state["unexpected"] = (cs, ip)
            return emulator.emu_stop()
        if body[0] <= address < body[1]:
            here = address - origin
            if here in SITES:
                if bytes(emulator.mem_read(address, 3)) != SITES[here]:
                    state["stop"] = CODE
                    return emulator.emu_stop()
                state["sites"][here] += 1
                state["stub"] = here == STUB_SITE
            visited.add(here)
            if state["previous"] in target["conditional"] and state["previous"] != here:
                edges.add((state["previous"], here))
            state["previous"] = here
            state["returned"] = here == BODY_RETF
        else:
            state["previous"] = None
            if cs == U3C2_CS:
                # The merged U3C2 audit's budget points: caller loop heads, line visits before +04DD, and the sort's
                # taken loop-backs to +0056/+0072 from +018E/+0199; the line and sort budgets restart at each call.
                index = 0 if ip in LOOPS else 1 if ip == 0x4DD else 2 if ip in (0x72, 0x56) and state["last"] in ((U3C2_CS, 0x18E), (U3C2_CS, 0x199)) else None
                if index is not None:
                    if not state["remaining"][index]:
                        state["stop"] = CALLEE
                        return emulator.emu_stop()
                    state["remaining"][index] -= 1
                if ip == 0xAF5:
                    state["remaining"][1] = budgets[1]
                if ip == 0xBD0:
                    state["remaining"][2] = budgets[2]
            state["expect"] = expects.get(address)
        state["last"] = (cs, ip)
        return None

    def mirror(emulator, access, address, size, value, user):
        del access, user
        for byte in range(size):
            raw = address + byte; at = raw & (SIZE - 1)
            emulator.mem_write(at + SIZE if raw < SIZE else at, bytes([(value >> (8 * byte)) & 0xFF]))

    original.hook_add(UC_HOOK_CODE, hook)
    if high:
        original.hook_add(UC_HOOK_MEM_WRITE, mirror)
    original.emu_start(body[0], 2 * SIZE, count=count)
    check(state["unexpected"] is None, f"original left the loaded code at {state['unexpected']}: {label}")
    check(state["stop"] is not None, f"original did not stop: {label}")
    expected = bytes(original.mem_read(0, SIZE))
    ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
    callee, callee_status = ctypes.c_int(99), ctypes.c_int(99)
    status = function(ram, SIZE, ctypes.byref(cpu), U3C6_CS, *budgets, ctypes.byref(callee), ctypes.byref(callee_status))
    check(status == state["stop"], f"C status {status} (callee {callee.value}/{callee_status.value}), original stopped with "
          f"{NAMES[state['stop']]}: {label}")
    actual = bytes(ram)
    if actual != expected:
        differences = [i for i, (a, b) in enumerate(zip(actual, expected)) if a != b]
        raise ValueError(f"RAM mismatch {label}: {[(hex(d), actual[d], expected[d]) for d in differences[:6]]} ({len(differences)} bytes)")
    for (name, _), register in zip(Cpu._fields_, REGISTERS):
        mine, want = getattr(cpu, name), original.reg_read(register) & 0xFFFF
        check(mine == want, f"{name} mismatch {label}: C={mine:04x}, original={want:04x} ({NAMES[state['stop']]})")
    count_now = memory[physical(DS, 0x671A)] if status != OK else bytes(ram)[physical(DS, 0x671A)]
    return state["stop"], visited, edges, state["sites"], (callee.value, callee_status.value), bytes(ram)[physical(DS, 0x671A)], cpu.ax & 0xFF


def random_party(rng, origin):
    """Members near the origin, inside the 55 x 30 map area the merged U3C2 audit uses."""
    return tuple((min(54, max(0, origin[0] + rng.randrange(-8, 9))), min(29, max(0, origin[1] + rng.randrange(-8, 9))),
                  rng.choice((1, 1, 1, 1, 0, 2))) for _ in range(rng.choice((0, 1, 2, 3, 4, 5, 6, 8, 10))))


def cases(rng):
    spec = []
    for i in range(150):
        origin = (rng.randrange(55), rng.randrange(30))
        spec.append((f"random-{i}", dict(party=random_party(rng, origin), origin=origin, radius=rng.choice((0, 3, 6, 12, 24, 100, 255)),
                                         flags=FLAGS[i % len(FLAGS)], sp=rng.randrange(0x0400, 0xF000) & ~1,
                                         heights=rng.choice(((0, 0),) * 4 + ((1, 0), (0, 1))), mode=rng.choice((1,) * 9 + (0,)))))
    near = ((12, 12, 1), (11, 9, 1), (8, 13, 1), (14, 10, 1))
    spec.append(("empty-party", dict(party=())))
    spec.append(("one-match", dict(party=near[:1], fields=[1] * 11, active=0)))
    spec.append(("one-mismatch", dict(party=near[:1], fields=[0] * 11, active=0)))
    spec.append(("active-nonzero-all-zero", dict(party=near, fields=[0] * 11, active=7)))
    spec.append(("active-nonzero-mixed", dict(party=near, fields=[0, 0, 0, 1, 0] + [0] * 6, active=0x80)))
    spec.append(("all-match-four", dict(party=near, fields=[1] * 11, active=0)))
    spec.append(("ten-candidates", dict(party=tuple((10 + (k % 5) - 2, 10 + (k // 5) - 1, 1) for k in range(10)))))
    # The lowest SP the U3C2 model accepts, and frames at the segment top; SS 1000h keeps the frame's contiguous view
    # off U3C2's code at 30000h, which the merged model requires.
    for sp, ss in ((0x00A6, SS), (0x00A7, SS), (0xFFF0, 0x1000), (0xFFFE, 0x1000)):
        spec.append((f"sp-{sp:04x}", dict(party=near[:2], sp=sp, ss=ss)))
    spec.append(("ss-a20", dict(party=near[:2], sp=0x0400, ss=0xFFFF)))
    spec.append(("random-background", dict(party=near[:3], background=bytes(random.Random(0x310255).randbytes(SIZE)))))
    spec.append(("relocated-entry", dict(party=near[:3], base=0x2000)))
    spec.append(("preserved-df-if", dict(party=near[:2], flags=0x0603)))
    for site in SITES:
        spec.append((f"code-site-{site:04x}", dict(party=near[:2], patch=((site + 1, 0x00),))))
    spec.append(("code-stub-cold", dict(party=near[:2], stub=b"\xcd\x3f\x09\x09\x00")))
    spec.append(("code-stub-entry", dict(party=near[:2], stub=struct.pack("<BHH", 0xEA, 0x0422, U3C2_CS))))
    spec.append(("u3c2-caller-limit", dict(party=near[:2], budgets=(1, 512, 65536))))
    spec.append(("u3c2-line-limit", dict(party=near[:2], budgets=(20000, 1, 65536))))
    spec.append(("u3c2-sort-limit", dict(party=tuple((21 - k, 21 - k, 1) for k in range(1, 6)), budgets=(20000, 512, 1))))
    # SS = DS and U3C2 entry SP = 671Bh: its candidate records overwrite its own return frame (the merged test's layout).
    spec.append(("u3c2-return-alias", dict(party=near[:1], ss=DS, sp=0x671B + 0x18)))
    # Candidate pointer k aims its byte +10Eh at the resident routine's local [BP-0Fh], written just before the read.
    spec.append(("resident-local-alias", dict(party=near[:2], ss=DS, sp=0xE000,
                                              writes=[(DS, 0x65B9 + 4 * k, struct.pack("<HH", (0xE000 - 2 - 0x0F - 0x10E) & 0xFFFF, DS)) for k in range(11)])))
    return spec


def differential(target, code31, code3c2, code3c6, image, function):
    rng = random.Random(0x310255)
    visited, edges, stops = set(), set(), {k: 0 for k in NAMES}
    sites, counts, results = {s: 0 for s in SITES}, {}, {}
    for label, options in cases(rng):
        budgets = options.pop("budgets", BUDGETS)
        base = options.get("base", 0)
        memory, cpu, high = build(code31, code3c2, code3c6, image, rng, **options)
        stop, v, e, s, callee, count, al = compare(f"+0255 {label}", memory, cpu, high, target, function, code3c2, code3c6, budgets, base)
        if not label.startswith("random"):
            print(f"  {label}: {NAMES[stop]} callee {callee} count {count} AL {al}")
        stops[stop] += 1
        visited |= v
        edges |= e
        for site, n in s.items():
            sites[site] += n
        if stop == OK:
            counts[count] = counts.get(count, 0) + 1
            results[al] = results.get(al, 0) + 1
    reachable = {(s, o) for s, outs in target["conditional"].items() for o in outs}
    missing = sorted(hex(i.address) for i in target["decoded"] if i.address not in visited)
    missing_edges = sorted((hex(s), hex(o)) for s, o in reachable - edges)
    check(not missing and not missing_edges, f"coverage incomplete: {missing} {missing_edges}")
    check(all(stops.values()), f"a stop kind was not exercised: {stops}")
    print(f"U31 +0255: {sum(stops.values())} original-byte cases passed ({', '.join(f'{v} {NAMES[k]}' for k, v in stops.items())}); "
          f"full 1 MiB RAM and all 14 CPU words equal; {len(visited)}/{len(target['decoded'])} instruction starts, "
          f"{len(edges)}/{len(reachable)} conditional outcomes; site fetches {dict((hex(k), v) for k, v in sites.items())}; "
          f"returned candidate counts {dict(sorted(counts.items()))}, results {dict(sorted(results.items()))}")
    return dict(stops={NAMES[k]: v for k, v in stops.items()}, starts=len(visited), outcomes=len(edges),
                site_fetches={f"{k:04X}": v for k, v in sites.items()}, counts={str(k): v for k, v in sorted(counts.items())},
                results={str(k): v for k, v in sorted(results.items())})


# ---- DOSBox-X synthetic oracle -------------------------------------------------------------------------
CALLER_N, U3C2_N, U3C6_N, SYS_N, STUB_N, PSTUB_N, RES_N, DATA_DS, REC_N = 0x8800, 0x3000, 0x4000, 0x7000, 0x7F00, 0x7E00, 0x7A00, 0x5000, 0x6000
MAIN_AT, U3C2_AT, U3C6_AT, MOVE_AT, RESIDENT_AT, TABLES_AT, CASES_AT, CODE_AT = 0x0300, 0x0C00, 0x1800, 0x2B00, 0x2B20, 0x2B50, 0x2B80, 0x3600
OUTPUT, HEADER, STACK_TOP = 0xC000, 0xD800, 0xF000
WINDOW_LOW, WINDOW_SIZE, REC_WORDS, NATIVE_ROW = STACK_TOP - 0x200, 0x200, 0x1000, 0x21
CASE_SIZE, NATIVE_CASES = 52, 48
RECORD_BYTES = 28 + WINDOW_SIZE + 0x10000
NATIVE_FLAGS = (0x0002, 0x00D7, 0x08D7, 0x0402, 0x0893, 0x0003)
ENTRY_REGS = dict(ax=0x1234, bx=0x3456, cx=0x5678, dx=0x789A, si=0x9ABC, di=0xBCDE, bp=0x7777)
SHAPE8 = bytes((0, 0, 1, 0, 0, 1, 0xFF, 0))
DXDY = bytes(v & 255 for v in (0, 1, 1, 1, 0, -1, -1, -1, 0, -1, -1, 0, 1, 1, 1, 0, -1, 0))


def native_cases(rng):
    cases = []
    for n in range(NATIVE_CASES):
        origin = (rng.randrange(4, 50), rng.randrange(4, 26))
        size = n % 11 if n < 22 else rng.randrange(11)
        party = [(min(54, max(0, origin[0] + rng.randrange(-6, 7))), min(29, max(0, origin[1] + rng.randrange(-6, 7))),
                  rng.choice((1, 1, 1, 0, 2))) for _ in range(size)]
        cases.append(dict(flags=n % len(NATIVE_FLAGS), party=party, origin=origin, radius=rng.choice((3, 6, 12, 24, 100, 255)),
                          active=rng.choice(FIELD), mode=rng.choice((1, 1, 1, 0)), fields=[rng.choice(FIELD) for _ in range(11)]))
    return cases


def case_bytes(case):
    data = struct.pack("<8B", case["flags"], len(case["party"]), case["origin"][0], case["origin"][1], case["radius"], NATIVE_ROW,
                       case["active"], case["mode"]) + bytes(case["fields"]) + bytes(1)
    for member in case["party"]:
        data += bytes(member)
    return data + bytes(CASE_SIZE - len(data))


def native_state(memory, case):
    """The harness's per-case writes, in its order: the DS segment and its U3C2 world, then the records."""
    at = physical(DATA_DS, 0); memory[at:at + 0x10000] = bytes([0xA5]) * 0x10000
    world(memory, party=case["party"], mode=case["mode"], heights=(0, 0), shape=((0, 0), (1, 0), (0, 1), (-1, 0)))
    memory[physical(DATA_DS, 0x3209 + 16 * NATIVE_ROW)] = case["radius"]
    put16(memory, DATA_DS, 0x6716, MAP); put16(memory, DATA_DS, 0x6718, DATA_DS)
    for k in range(11):
        put16(memory, DATA_DS, 0x65B9 + 4 * k, 0x0201 * k); put16(memory, DATA_DS, 0x65BB + 4 * k, REC_N)
    put16(memory, DATA_DS, 0x5D92, ACTIVE); put16(memory, DATA_DS, 0x5D94, REC_N)
    at = physical(REC_N, 0); memory[at:at + 2 * REC_WORDS] = bytes([0xA5]) * (2 * REC_WORDS)
    for k, value in enumerate(case["fields"]):
        memory[physical(REC_N, 0x0201 * k + 0x10E)] = value
    memory[physical(REC_N, ACTIVE + 0x10E)] = case["active"]


def native_audit(target, code31, code3c2, code3c6, image, function):
    del target
    check(DS == DATA_DS, "the world builder writes DS 5000h")
    directory = ROOT / "resources/oracle-runs/u31-0255-synthetic"
    directory.mkdir(parents=True, exist_ok=True)
    move = image[physical(SYSTEM_CS, 0x0A3F):physical(SYSTEM_CS, 0x0A57)]
    resident = image[physical(RES_CS, RES_IP):physical(RES_CS, RES_END)]
    unit = bytearray(code31)
    struct.pack_into("<H", unit, 0x0287 + 3, STUB_N); struct.pack_into("<H", unit, 0x02A9 + 3, RES_N)
    u3c2 = bytearray(code3c2)
    for site in (0x131, 0x160, 0x181):
        struct.pack_into("<H", u3c2, site + 3, SYS_N)
    for site in (0x92A, 0x9D8):
        struct.pack_into("<H", u3c2, site + 3, PSTUB_N)
    rng = random.Random(0x310255D)
    cases = native_cases(rng)
    check(U3C2_AT + len(u3c2) <= U3C6_AT and U3C6_AT + len(code3c6) <= MOVE_AT and MOVE_AT + len(move) <= RESIDENT_AT
          and RESIDENT_AT + len(resident) <= TABLES_AT and TABLES_AT + len(SHAPE8) + len(DXDY) <= CASES_AT
          and CASES_AT + CASE_SIZE * len(cases) <= CODE_AT and CODE_AT + len(unit) <= OUTPUT and OUTPUT + 28 + WINDOW_SIZE <= HEADER,
          "COM layout overlaps")
    text = [".code16", ".intel_syntax noprefix", ".global _start", "_start:", "    jmp main", f".org {MAIN_AT - 0x100:#x}", "main:",
            "    mov ax,cs", "    mov ds,ax", "    mov es,ax", "    mov ss,ax", f"    mov sp,{STACK_TOP:#x}", "    cld"]
    for segment, offset, label, length in ((CALLER_N, 0, "unit", len(unit)), (U3C2_N, 0, "u3c2", len(u3c2)), (U3C6_N, 0, "u3c6", len(code3c6)),
                                           (SYS_N, 0x0A3F, "move", len(move)), (RES_N, RES_IP, "resident", len(resident))):
        text += [f"    mov ax,{segment:#x}", "    mov es,ax", f"    mov si,offset {label}", f"    mov di,{offset:#x}", f"    mov cx,{length:#x}", "    rep movsb"]
    for segment, offset, ip, cs in ((STUB_N, 0x3E, 0x0909, U3C2_N), (PSTUB_N, 0x25, 0x0000, U3C6_N)):
        text += [f"    mov ax,{segment:#x}", "    mov es,ax", f"    mov byte ptr es:[{offset:#x}],0xea", f"    mov word ptr es:[{offset + 1:#x}],{ip:#x}",
                 f"    mov word ptr es:[{offset + 3:#x}],{cs:#x}"]
    text += ["    push cs", "    pop es",
             f"    mov word ptr [{HEADER:#x}],cs", f"    mov word ptr [{HEADER + 2:#x}],offset returned",
             f"    mov word ptr [{HEADER + 4:#x}],{len(cases)}", f"    mov word ptr [{HEADER + 6:#x}],0",
             "    mov dx,offset filename", "    xor cx,cx", "    mov ah,0x3c", "    int 0x21", "    jc failure",
             "    mov word ptr [handle],ax", "    mov bx,ax", f"    mov dx,{HEADER:#x}", "    mov cx,8", "    mov ah,0x40", "    int 0x21", "    jc failure",
             "again:", "    cli", f"    mov sp,{STACK_TOP:#x}", "    cld", "    push cs", "    pop ds", "    push cs", "    pop es",
             "    mov al,0xa5", f"    mov di,{WINDOW_LOW:#x}", f"    mov cx,{WINDOW_SIZE:#x}", "    rep stosb",
             "    mov ax,word ptr [index]", f"    mov bx,{CASE_SIZE}", "    mul bx", "    add ax,offset cases", "    mov word ptr [case_ptr],ax",
             f"    mov ax,{DATA_DS:#x}", "    mov es,ax", "    xor di,di", "    mov cx,0x8000", "    mov ax,0xa5a5", "    rep stosw",
             "    mov di,0x2844", "    mov bx,256", "shape_loop:", "    mov si,offset shape8", "    mov cx,8", "    rep movsb", "    dec bx", "    jnz shape_loop",
             "    mov di,0x2736", "    mov si,offset dxdy", "    mov cx,18", "    rep movsb",
             "    mov di,0x2745", "    mov cx,64", "height_loop:", "    mov word ptr es:[di],0", "    add di,4", "    loop height_loop",
             "    mov di,0x25c0", "    mov cx,9", "    xor al,al", "    rep stosb",
             f"    mov di,{MAP:#x}", "    mov cx,0x1800", "    xor al,al", "    rep stosb",
             "    mov si,word ptr [case_ptr]", "    mov al,byte ptr [si+7]", f"    mov byte ptr es:[{MAP + 6:#x}],al",
             "    mov al,byte ptr [si+1]", "    mov byte ptr es:[0x5f2a],al", "    xor ch,ch", "    mov cl,byte ptr [si+1]", "    jcxz party_done",
             "    lea bx,[si+20]", "    mov di,0x5f2b", "party_loop:", "    mov al,byte ptr [bx]", "    mov byte ptr es:[di],al",
             "    mov al,byte ptr [bx+1]", "    mov byte ptr es:[di+1],al", "    mov al,byte ptr [bx+2]", "    mov byte ptr es:[di+3],al",
             "    add bx,3", "    add di,4", "    loop party_loop", "party_done:",
             "    mov al,byte ptr [si+4]", f"    mov byte ptr es:[{0x3209 + 16 * NATIVE_ROW:#x}],al",
             f"    mov word ptr es:[0x6716],{MAP:#x}", f"    mov word ptr es:[0x6718],{DATA_DS:#x}",
             "    mov di,0x65b9", "    xor ax,ax", "    mov cx,11", "pointer_loop:", "    mov word ptr es:[di],ax", f"    mov word ptr es:[di+2],{REC_N:#x}",
             "    add ax,0x0201", "    add di,4", "    loop pointer_loop",
             f"    mov word ptr es:[0x5d92],{ACTIVE:#x}", f"    mov word ptr es:[0x5d94],{REC_N:#x}",
             f"    mov ax,{REC_N:#x}", "    mov es,ax", "    xor di,di", f"    mov cx,{REC_WORDS:#x}", "    mov ax,0xa5a5", "    rep stosw",
             "    lea bx,[si+8]", "    mov di,0x010e", "    mov cx,11", "field_loop:", "    mov al,byte ptr [bx]", "    mov byte ptr es:[di],al",
             "    inc bx", "    add di,0x0201", "    loop field_loop",
             "    mov al,byte ptr [si+6]", f"    mov byte ptr es:[{ACTIVE + 0x10E:#x}],al",
             "    mov al,byte ptr [si+2]", "    mov ah,0x5c", "    push ax", "    mov al,byte ptr [si+3]", "    mov ah,0x5b", "    push ax",
             f"    mov ax,{0x5A00 | NATIVE_ROW:#x}", "    push ax", "    mov ax,0x1357", "    push ax",
             "    mov bl,byte ptr [si]", "    xor bh,bh", "    shl bx,1", "    push word ptr [flags_table+bx]",
             "    push cs", "    pop es", f"    mov ax,{DATA_DS:#x}", "    mov ds,ax"]
    for name, value in ENTRY_REGS.items():
        text.append(f"    mov {name},{value:#x}")
    text += ["    popf", "    call dword ptr cs:[target_far]", "returned:"]
    for i, reg in enumerate("ax bx cx dx si di bp sp cs".split()):
        text.append(f"    mov word ptr cs:[{OUTPUT + 2 * i:#x}],{reg}")
    text += [f"    mov word ptr cs:[{OUTPUT + 18:#x}],offset returned", f"    mov word ptr cs:[{OUTPUT + 20:#x}],ds",
             f"    mov word ptr cs:[{OUTPUT + 22:#x}],es", f"    mov word ptr cs:[{OUTPUT + 24:#x}],ss",
             f"    mov sp,{STACK_TOP + 0x800:#x}", "    pushf", "    pop ax", f"    mov word ptr cs:[{OUTPUT + 26:#x}],ax",
             "    cld", "    push cs", "    pop ds", "    push cs", "    pop es", f"    mov di,{OUTPUT + 28:#x}",
             f"    mov si,{WINDOW_LOW:#x}", f"    mov cx,{WINDOW_SIZE:#x}", "    rep movsb", "    sti",
             "    mov bx,word ptr [handle]", f"    mov dx,{OUTPUT:#x}", f"    mov cx,{28 + WINDOW_SIZE}", "    mov ah,0x40", "    int 0x21", "    jc failure",
             f"    mov ax,{DATA_DS:#x}", "    mov ds,ax", "    xor dx,dx", "    mov cx,0x8000", "    mov ah,0x40", "    int 0x21", "    jc failure",
             "    mov dx,0x8000", "    mov cx,0x8000", "    mov ah,0x40", "    int 0x21", "    jc failure", "    push cs", "    pop ds",
             "    inc word ptr [index]", "    mov ax,word ptr [index]", f"    cmp ax,{len(cases)}", "    jb again",
             "    mov bx,word ptr [handle]", "    mov ah,0x3e", "    int 0x21", "    mov ax,0x4c00", "    int 0x21",
             "failure:", "    mov ax,0x4c01", "    int 0x21",
             'filename: .asciz "RESULT.BIN"', "handle: .word 0", "index: .word 0", "case_ptr: .word 0", f"target_far: .word {ENTRY:#x}, {CALLER_N:#x}",
             "flags_table: .word " + ",".join(f"{f:#x}" for f in NATIVE_FLAGS)]
    for label, blob, at in (("u3c2", bytes(u3c2), U3C2_AT), ("u3c6", code3c6, U3C6_AT), ("move", move, MOVE_AT), ("resident", resident, RESIDENT_AT),
                            ("shape8", SHAPE8 + DXDY, TABLES_AT), ("cases", b"".join(case_bytes(c) for c in cases), CASES_AT), ("unit", bytes(unit), CODE_AT)):
        text += [f".org {at - 0x100:#x}", f"{label}:"]
        if label == "shape8":
            text += ["    .byte " + ",".join(f"{b:#x}" for b in SHAPE8), "dxdy:", "    .byte " + ",".join(f"{b:#x}" for b in DXDY)]
            continue
        for i in range(0, len(blob), 16):
            text.append("    .byte " + ",".join(f"{b:#x}" for b in blob[i:i + 16]))
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
                       env=environment, stdout=log, stderr=subprocess.STDOUT, check=True, timeout=600)
    result = result_path.read_bytes()
    check(len(result) == 8 + len(cases) * RECORD_BYTES, f"native output incomplete: {len(result)} bytes")
    segment, returned, count, _ = struct.unpack_from("<4H", result)
    check(count == len(cases) and segment == 0x0814, f"native header differs or COM segment {segment:04x} is not 0814")
    counts, results = {}, {}
    for n, case in enumerate(cases):
        record = result[8 + n * RECORD_BYTES:8 + (n + 1) * RECORD_BYTES]
        memory = bytearray(SIZE)
        for seg, off, blob in ((CALLER_N, 0, bytes(unit)), (U3C2_N, 0, bytes(u3c2)), (U3C6_N, 0, code3c6), (SYS_N, 0x0A3F, move), (RES_N, RES_IP, resident)):
            at = physical(seg, off); memory[at:at + len(blob)] = blob
        for (seg, off), ip, cs in (((STUB_N, 0x3E), 0x0909, U3C2_N), ((PSTUB_N, 0x25), 0x0000, U3C6_N)):
            at = physical(seg, off); memory[at:at + 5] = struct.pack("<BHH", 0xEA, ip, cs)
        native_state(memory, case)
        for i in range(WINDOW_SIZE):
            memory[physical(segment, WINDOW_LOW + i)] = 0xA5
        sp = STACK_TOP
        for value in (0x5C00 | case["origin"][0], 0x5B00 | case["origin"][1], 0x5A00 | NATIVE_ROW, 0x1357, segment, returned):
            sp -= 2; put16(memory, segment, sp, value)
        cpu = Cpu(*ENTRY_REGS.values(), sp, CALLER_N, ENTRY, DATA_DS, segment, segment, NATIVE_FLAGS[case["flags"]])
        ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
        callee, callee_status = ctypes.c_int(99), ctypes.c_int(99)
        status = function(ram, SIZE, ctypes.byref(cpu), U3C6_N, *BUDGETS, ctypes.byref(callee), ctypes.byref(callee_status))
        check(status == OK, f"C status {status} on native case {n}")
        for (name, _), want in zip(Cpu._fields_, struct.unpack_from("<14H", record)):
            check(getattr(cpu, name) == want, f"native case {n} {name}: C={getattr(cpu, name):04x}, DOS={want:04x}")
        got = bytes(ram[physical(segment, WINDOW_LOW):physical(segment, WINDOW_LOW) + WINDOW_SIZE])
        got += bytes(ram[physical(DATA_DS, 0):physical(DATA_DS, 0) + 0x10000])
        if got != record[28:]:
            differences = [i for i, (a, b) in enumerate(zip(got, record[28:])) if a != b]
            raise ValueError(f"native case {n} bytes differ at {[hex(d) for d in differences[:8]]} ({len(differences)} bytes)")
        listed = ram[physical(DATA_DS, 0x671A)]
        counts[listed] = counts.get(listed, 0) + 1
        results[cpu.ax & 0xFF] = results.get(cpu.ax & 0xFF, 0) + 1
    digest = sha256(result).hexdigest()
    print(f"DOSBox-X normal-core/386 synthetic cases passed: {len(cases)} (candidate counts {dict(sorted(counts.items()))}, results "
          f"{dict(sorted(results.items()))}); all 14 CPU words, the {WINDOW_SIZE}-byte stack window and the whole 64 KiB DS segment; no mask")
    print(f"native result SHA-256 {digest}; COM segment {segment:04x}; U3C2 at {U3C2_N:04X}, U3C6 at {U3C6_N:04X}, SYSTEM Move at {SYS_N:04X}, "
          f"resident at {RES_N:04X}, stubs {STUB_N:04X}:003E and {PSTUB_N:04X}:0025")
    return dict(cases=len(cases), counts={str(k): v for k, v in sorted(counts.items())}, results={str(k): v for k, v in sorted(results.items())},
                result_sha256=digest)


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
    target, code31, code3c2, code3c6, image = source()
    report = dict(target=dict(sha256=BODY_SHA, bytes=150, instructions=58, file_offset=target["file_offset"]))
    if args.unicorn or args.native:
        with tempfile.TemporaryDirectory(prefix="poolrad-u310255-") as temp:
            function = library(Path(temp))
            from loaded_stub_checks import verify_refusals
            memory,cpu,_ = build(code31,code3c2,code3c6,image,random.Random(0))
            report["loaded_stub_guards"] = verify_refusals(memory,cpu,
                lambda ram,state: function(ram,SIZE,ctypes.byref(state),U3C6_CS,*BUDGETS,None,None),
                [(U3C2_CS,p) for p in (0x92A,0x9D8)])
            if args.unicorn:
                report["unicorn"] = differential(target, code31, code3c2, code3c6, image, function)
            if args.native:
                report["native"] = native_audit(target, code31, code3c2, code3c6, image, function)
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(json.dumps(report, sort_keys=True, indent=2) + "\n")
    print(json.dumps(report, sort_keys=True))


if __name__ == "__main__":
    main()
