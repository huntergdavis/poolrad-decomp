#!/usr/bin/env python3
"""Original-byte full-state parity for raw-CPU U6F +1446 (the party insertion), composing SYSTEM FillChar and set
membership and, through the loaded stub 00AC:0025, U9B +0000, plus a DOSBox-X oracle.

The differential places the hash-checked expanded START.EXE image at load segment 0, so both SYSTEM leaves run from their
original bytes, patches the stub to its loaded EA form, loads units 0x9B and 0x6F, and compares the entire 1 MiB RAM and all
14 CPU words between the original under Unicorn and the C model, with no mask. The original decides each stop: the fetch
after its own RETF 4 (return), a call site or the stub whose live bytes differ when fetched (CODE), a list-walk loop-back
past the host cap (LIMIT), a U9B leaf RETF that does not come back into U9B +0000 (CALLEE), or a callee RETF that does not
come back to the call's next instruction (TRANSFER). Media stays private.
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

ENTRY, END, BODY_RETF, SET_AT = 0x1446, 0x157F, 0x157C, 0x1426
BODY_SHA = "370b29767e075c947ae418f44cb57f92e50127c1d519ba029cd7eacb882aa245"
SET_SHA = "d22f8e3450655784853879c7ecfee4ff36192352f6ffe0d40593dee56f583350"
FILL_SHA = "f1c5fade96970aa1dd1a9ffc2d876c7aea57bef8680d929d9f65d6c5190e17f7"
SET_IN_SHA = "753159dded0f3f45bb0352cb9483a15271d85888b6c1b127f8631879473ec62c"
U9B_SPANS = ((0x0000, 0x01AF, 0x01AC, "260fb152b8d892e56d4ee8dd3c22714c09d5d696490b0828a4653a9c9e5aee71"),
             (0x01AF, 0x0255, 0x0252, "5dd90fb8796e7aa15a770e4868aaceeee0a0886e0251a9fc7c10f3f32935652c"),
             (0x0255, 0x0329, 0x0326, "f19017361e6ac6197e38eeee7b70f438d29962a34675ff43903ac712444dd326"),
             (0x0329, 0x045E, 0x045B, "f5ff12924988b708edcf3756213532a2c5ba1b4860a5f63e11ffcd229297f3d6"))
SITES = {0x14CA: b"\x9a\xa8\x16", 0x14F2: b"\x9a\xc0\x0d", 0x1574: b"\x9a\x25\x00"}
STUB_SITE, U9B_RETURN = 0x1574, 0x1579
LOOP_BACKS = {0x1487: 0x14A9, 0x14DC: 0x151C}  # loop head: the JMP that returns to it
SYSTEM_CS, STUB_CS, STUB_IP = 0x0AF8, 0x00AC, 0x0025
LEAF_RETF = {0x16B9: 0x14CF, 0x0DDE: 0x14F7}  # SYSTEM FillChar and set-IN RETF: the U6F continuation
U9B_LEAF_RETF = {0x0252: 0x00EF, 0x0326: 0x0194, 0x045B: 0x01A9}  # U9B leaf RETF 4: the +0000 continuation
U9B_RETF = 0x01AC
CALLER_CS, U9B_CS, DS, SS, EDGE_SS = 0x8800, 0x4800, 0x1800, 0x2800, 0x2000
RET_CS, RET_IP = 0x9001, 0x7000
OK, CODE, CALLEE, TRANSFER, LIMIT = 0, 2, 3, 4, 5
NAMES = {OK: "return", CODE: "CODE", CALLEE: "CALLEE", TRANSFER: "TRANSFER", LIMIT: "LIMIT"}
CLASSES = (0, 0, 1, 2, 6, 7, 0x7F, 0x80, 0xFF)
SLOTS = tuple(range(8)) * 3 + (8, 9, 0x10, 0x7F, 0x80, 0xFE, 0xFF)
FLAGS = (0x7202, 0x0202, 0x0ED7, 0x0647, 0x72D7, 0x0403, 0x0213)


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
    u6f, u9b = units[0x6F], units[0x9B]
    code6f = ovr[u6f["ovr_offset"]:u6f["ovr_offset"] + u6f["code_size"]]
    code9b = ovr[u9b["ovr_offset"]:u9b["ovr_offset"] + u9b["code_size"]]
    md = Cs(CS_ARCH_X86, CS_MODE_16)
    for start, end, retf, digest in U9B_SPANS:
        decoded = list(md.disasm(code9b[start:end], start))
        check(sha256(code9b[start:end]).hexdigest() == digest and decoded[-1].address == retf and decoded[-1].mnemonic == "retf"
              and decoded[-1].op_str == "4" and sum(i.mnemonic.startswith("ret") for i in decoded) == 1, f"U9B span {start:#x} differs")
    check(0x0000 in u9b["entries"] and u9b["memory_segment"] == STUB_CS, "unit 0x9B entry or stub segment differs")
    check(image[physical(STUB_CS, STUB_IP):physical(STUB_CS, STUB_IP) + 5] == b"\xcd\x3f\x00\x00\x00", "cold 00AC:0025 stub differs")
    for start, length, retf, operand, digest in ((0x16A8, 20, 0x16B9, "8", FILL_SHA), (0x0DC0, 33, 0x0DDE, "6", SET_IN_SHA)):
        span = image[physical(SYSTEM_CS, start):physical(SYSTEM_CS, start) + length]
        decoded = list(md.disasm(span, start))
        check(sha256(span).hexdigest() == digest and decoded[-1].address == retf and decoded[-1].op_str == operand, f"SYSTEM {start:#x} differs")
    check(ENTRY in u6f["entries"], "U6F +1446 is not a unit entry")
    body = code6f[ENTRY:END]
    decoded = list(md.disasm(body, ENTRY))
    calls = [(i.address, i.op_str) for i in decoded if "call" in i.mnemonic]
    conditional = {i.address: (i.address + i.size, int(i.op_str, 16)) for i in decoded if i.mnemonic.startswith("j") and i.mnemonic != "jmp"}
    check(sha256(body).hexdigest() == BODY_SHA and len(body) == 313 and len(decoded) == 99 and sum(i.size for i in decoded) == 313
          and decoded[-1].address == BODY_RETF and decoded[-1].mnemonic == "retf" and decoded[-1].op_str == "4"
          and calls == [(0x14CA, "0xaf8, 0x16a8"), (0x14F2, "0xaf8, 0xdc0"), (0x1574, "0xac, 0x25")]
          and all(site + 3 in set(u6f["fixups"]) for site in SITES) and len(conditional) == 6, f"U6F +1446 extent or calls differ: {calls}")
    members = [n for n in range(256) if code6f[SET_AT + n // 8] >> (n % 8) & 1]
    check(sha256(code6f[SET_AT:ENTRY]).hexdigest() == SET_SHA and members == list(range(8)), "the CS:1426 set differs")
    target = dict(decoded=decoded, conditional=conditional, file_offset=u6f["ovr_offset"] + ENTRY)
    print(f"U6F +1446 (313/99, LCALL 0AF8:16A8, 0AF8:0DC0, 00AC:0025 to U9B +0000, RETF 4) GAME file {target['file_offset']:#x} "
          f"sha {BODY_SHA}; set at +1426 has members 0..7")
    return target, code6f, code9b, image


CHECKER_ROWS = ('OVL_006F_1446',)


def library(directory):
    path = directory / "u6f1446.so"
    sources = ["overlay_u6f_party_insert_1446_cpu.c", "overlay_u9b_entry_zero_0000.c", "overlay_u9b_record_table_0255.c",
               "overlay_u9b_record_fields_0329.c", "system_runtime_leaves_cpu.c"]
    subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-O2", "-fPIC", "-shared", "-I", str(ROOT / "include"),
                    *[str(ROOT / "src/recovered" / s) for s in sources], "-o", str(path)], check=True, capture_output=True, text=True)
    function = ctypes.CDLL(str(path)).pr_u6f_party_insert_1446_cpu
    function.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t, ctypes.POINTER(Cpu), ctypes.c_uint,
                         ctypes.POINTER(ctypes.c_int), ctypes.POINTER(ctypes.c_int)]
    function.restype = ctypes.c_int
    return function


def build(code6f, code9b, image, rng, *, nodes=(), incoming=(0x6000, 0x0200), incoming_next=(0, 0), sp=0x8000, ss=SS, ds=DS,
          flags=0x7202, background=None, patch=(), stub=None, base=0, window=(0x3800, 0x0100), counter=None, active=None, writes=()):
    """Nodes are (segment, offset, slot) from the head; each links to the next and the last is nil."""
    memory = bytearray([0xA5]) * SIZE if background is None else bytearray(background)
    memory[0:len(image)] = image
    at = physical(STUB_CS, STUB_IP)
    memory[at:at + 5] = struct.pack("<BHH", 0xEA, 0x0000, U9B_CS) if stub is None else stub
    at = physical(U9B_CS, 0); memory[at:at + len(code9b)] = code9b
    at = physical(CALLER_CS, base); memory[at:at + len(code6f)] = code6f
    for offset, value in patch:
        memory[physical(CALLER_CS, (base + offset) & 0xFFFF)] = value
    for address in range(0x31FA, 0x4300):
        memory[physical(ds, address)] = rng.randrange(256)
    segment, offset = incoming
    for index in range(8):
        memory[physical(segment, (offset + 0x96 + index) & 0xFFFF)] = rng.choice(CLASSES)
    memory[physical(segment, (offset + 0x9C) & 0xFFFF)] = rng.choice((0, 1, 2, 0x7F, 0x80))
    memory[physical(segment, (offset + 0x12) & 0xFFFF)] = rng.randrange(256)
    put16(memory, segment, (offset + 0x104) & 0xFFFF, incoming_next[1]); put16(memory, segment, (offset + 0x106) & 0xFFFF, incoming_next[0])
    for i, (seg, off, slot) in enumerate(nodes):
        link = nodes[i + 1][:2] if i + 1 < len(nodes) else (0, 0)
        memory[physical(seg, (off + 0xBF) & 0xFFFF)] = slot
        put16(memory, seg, (off + 0x104) & 0xFFFF, link[1]); put16(memory, seg, (off + 0x106) & 0xFFFF, link[0])
    head = nodes[0][:2] if nodes else (0, 0)
    act = (rng.randrange(0x10000), rng.randrange(0x10000)) if active is None else active
    for address, value in ((0x5D96, head[1]), (0x5D98, head[0]), (0x5D92, act[1]), (0x5D94, act[0]), (0x49D6, window[1]), (0x49D8, window[0])):
        put16(memory, ds, address, value)
    put16(memory, window[0], (window[1] + 0x67C) & 0xFFFF, rng.randrange(0x10000) if counter is None else counter)
    frame = physical(ss, sp)
    for i, value in enumerate((RET_IP, RET_CS, offset, segment)):
        put16(memory, ss, (sp + 2 * i) & 0xFFFF, value)
        put16(memory, 0, (frame + 2 * i) & (SIZE - 1), value)
    for seg, off, data in writes:
        for j, value in enumerate(data):
            memory[physical(seg, (off + j) & 0xFFFF)] = value
    reg = tuple(rng.randrange(0x10000) for _ in range(7))
    return memory, Cpu(*reg, sp, CALLER_CS, (base + ENTRY) & 0xFFFF, ds, rng.randrange(0x10000), ss, flags), ss >= 0xF000


def compare(label, memory, cpu, high, target, function, cap=64, base=0, budget=3000000):
    from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
    from unicorn.x86_const import UC_X86_REG_CS, UC_X86_REG_IP
    original = Uc(UC_ARCH_X86, UC_MODE_16)
    original.mem_map(0, 2 * SIZE)
    original.mem_write(0, bytes(memory)); original.mem_write(SIZE, bytes(memory))
    for (name, _), register in zip(Cpu._fields_, REGISTERS):
        original.reg_write(register, getattr(cpu, name))
    origin = physical(CALLER_CS, base)
    body = (origin + ENTRY, origin + END)
    expects = {physical(SYSTEM_CS, retf): ((CALLER_CS, (base + back) & 0xFFFF), TRANSFER) for retf, back in LEAF_RETF.items()}
    expects[physical(U9B_CS, U9B_RETF)] = ((CALLER_CS, (base + U9B_RETURN) & 0xFFFF), TRANSFER)
    expects.update({physical(U9B_CS, retf): ((U9B_CS, back), CALLEE) for retf, back in U9B_LEAF_RETF.items()})
    state = dict(stop=None, expect=None, stub=False, returned=False, previous=None, loops=0, sites={s: 0 for s in SITES})
    visited, edges = set(), set()

    def hook(emulator, address, size, user):
        del size, user
        if state["returned"]:
            state["stop"] = OK
            return emulator.emu_stop()
        if state["expect"] is not None:
            want, kind = state["expect"]
            state["expect"] = None
            if (emulator.reg_read(UC_X86_REG_CS), emulator.reg_read(UC_X86_REG_IP)) != want:
                state["stop"] = kind
                return emulator.emu_stop()
        if state["stub"]:
            state["stub"] = False
            if bytes(emulator.mem_read(address, 3)) != b"\xea\x00\x00":
                state["stop"] = CODE
                return emulator.emu_stop()
        if body[0] <= address < body[1]:
            here = address - origin
            if here in SITES:
                if bytes(emulator.mem_read(address, 3)) != SITES[here]:
                    state["stop"] = CODE
                    return emulator.emu_stop()
                state["sites"][here] += 1
                state["stub"] = here == STUB_SITE
            if here in LOOP_BACKS and state["previous"] == LOOP_BACKS[here]:
                state["loops"] += 1
                if state["loops"] > cap:
                    state["stop"] = LIMIT
                    return emulator.emu_stop()
            visited.add(here)
            if state["previous"] in target["conditional"] and state["previous"] != here:
                edges.add((state["previous"], here))
            state["previous"] = here
            state["returned"] = here == BODY_RETF
            return None
        state["previous"] = None
        state["expect"] = expects.get(address)
        return None

    def mirror(emulator, access, address, size, value, user):
        del access, user
        for byte in range(size):
            raw = address + byte; at = raw & (SIZE - 1)
            emulator.mem_write(at + SIZE if raw < SIZE else at, bytes([(value >> (8 * byte)) & 0xFF]))

    original.hook_add(UC_HOOK_CODE, hook)
    if high:
        original.hook_add(UC_HOOK_MEM_WRITE, mirror)
    original.emu_start(body[0], 2 * SIZE, count=budget)
    check(state["stop"] is not None, f"original did not stop: {label}")
    expected = bytes(original.mem_read(0, SIZE))
    ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
    callee, callee_status = ctypes.c_int(99), ctypes.c_int(99)
    status = function(ram, SIZE, ctypes.byref(cpu), cap, ctypes.byref(callee), ctypes.byref(callee_status))
    check(status == state["stop"], f"C status {status} (callee {callee.value}/{callee_status.value}), original stopped with "
          f"{NAMES[state['stop']]}: {label}")
    actual = bytes(ram)
    if actual != expected:
        differences = [i for i, (a, b) in enumerate(zip(actual, expected)) if a != b]
        raise ValueError(f"RAM mismatch {label}: {[(hex(d), actual[d], expected[d]) for d in differences[:6]]} ({len(differences)} bytes)")
    for (name, _), register in zip(Cpu._fields_, REGISTERS):
        mine, want = getattr(cpu, name), original.reg_read(register) & 0xFFFF
        check(mine == want, f"{name} mismatch {label}: C={mine:04x}, original={want:04x} ({NAMES[state['stop']]})")
    return state["stop"], visited, edges, state["sites"], (callee.value, callee_status.value)


def random_nodes(rng, count):
    return [(rng.randrange(0x5000, 0x7000), rng.randrange(0x0000, 0xFE00), rng.choice(SLOTS)) for _ in range(count)]


def cases(rng):
    spec = []
    for i in range(300):
        spec.append((f"random-{i}", dict(nodes=random_nodes(rng, rng.choice((0, 0, 1, 2, 3, 4, 5, 6, 8, 10))),
                                         incoming=(rng.randrange(0x5000, 0x7000), rng.randrange(0x0000, 0xFE00)),
                                         window=(rng.randrange(0x3800, 0x4000), rng.randrange(0x0000, 0x7000)),
                                         counter=rng.choice((None, None, 0xFFFF, 0x7FFF, 0x000F)),
                                         flags=FLAGS[i % len(FLAGS)], sp=rng.randrange(0x0400, 0xF000) & ~1)))
    full = [(0x5000 + 0x40 * n, 0x0010 * n, n) for n in range(8)]
    spec.append(("all-eight-occupied", dict(nodes=full)))
    spec.append(("seven-occupied", dict(nodes=full[:3] + full[4:])))
    spec.append(("out-of-set-slots", dict(nodes=[(0x5100, 0x0100, s) for s in (8, 0x10, 0x80, 0xFF)][:1]
                                          + [(0x5200 + n, 0x0300, s) for n, s in enumerate((0x10, 0x80, 0xFF))])))
    spec.append(("empty-head", dict(nodes=[])))
    spec.append(("counter-wrap", dict(nodes=full[:2], counter=0xFFFF)))
    spec.append(("counter-sign", dict(nodes=full[:2], counter=0x7FFF)))
    for sp in (0x0040, 0x0100, 0xFFC0, 0xFFE8):
        spec.append((f"sp-{sp:04x}", dict(nodes=full[:3], sp=sp, ss=EDGE_SS)))
    spec.append(("ss-a20", dict(nodes=full[:3], sp=0x0400, ss=0xFFFF)))
    spec.append(("random-background", dict(nodes=full[:5], background=bytes(random.Random(0x6F1446).randbytes(SIZE)))))
    spec.append(("relocated-entry", dict(nodes=full[:4], base=0x2000)))
    spec.append(("preserved-df-if", dict(nodes=full[:2], flags=0x0603)))
    for site in SITES:
        spec.append((f"code-site-{site:04x}", dict(nodes=full[:2], patch=((site + 1, 0x00),))))
    spec.append(("code-site-opcode", dict(nodes=full[:2], patch=((0x14F2, 0xEA),))))
    spec.append(("code-stub-cold", dict(nodes=full[:2], stub=b"\xcd\x3f\x00\x00\x00")))
    spec.append(("code-stub-entry", dict(nodes=full[:2], stub=struct.pack("<BHH", 0xEA, 0x01AF, U9B_CS))))
    cyclic = [(0x5300, 0x0200, 3)]
    spec.append(("limit-loop-a", dict(nodes=cyclic, writes=((0x5300, 0x0304, (0x00, 0x02, 0x00, 0x53)),), cap=5)))
    spec.append(("limit-loop-b", dict(nodes=full[:3], incoming_next=full[0][:2], cap=9)))
    spec.append(("cap-exact", dict(nodes=full[:4], cap=8)))
    spec.append(("cap-short", dict(nodes=full[:4], cap=7)))
    spec.append(("cap-zero-empty", dict(nodes=[], cap=0)))
    # Loop A links the incoming record into set byte 1..4 at CS:1427, admitting slot 18; the head's mark lands on [BP+6].
    spec.append(("set-and-frame-alias", dict(nodes=[(0x5000, 0x0100, 18), (CALLER_CS, 0x1323, 3)], incoming=(0x6000, 0x0400))))
    # DS = SS and the head words are the cursor at [BP-4]; the marks land on the active pointer.
    spec.append(("ds-frame-alias", dict(nodes=full[:4], ss=DS, sp=0x5D9C)))
    spec.append(("window-return-alias", dict(nodes=full[:2], window=(SS, (0x8000 - 0x67C) & 0xFFFF))))
    # The incoming slot byte is mark[1], and then the pointer byte at [BP-10h].
    spec.append(("slot-mark-alias", dict(nodes=full[:3], incoming=(SS, (0x8000 - 2 - 0x0C + 1 - 0xBF) & 0xFFFF))))
    spec.append(("slot-pointer-alias", dict(nodes=full[:3], incoming=(SS, (0x8000 - 2 - 0x10 - 0xBF) & 0xFFFF))))
    # U9B clears record +2Dh, the low byte of its own return IP.
    spec.append(("u9b-return-alias", dict(nodes=full[:2], incoming=(SS, (0x8000 - 2 - 0x18 - 0x2D) & 0xFFFF))))
    spec.append(("fill-frame-wrap", dict(nodes=full[:2], sp=0x001C, ss=EDGE_SS)))
    # A U9B leaf's RETF 4 reads a return IP that the leaf overwrote through the record (the merged U9B audit's layout).
    spec.append(("u9b-leaf-return-alias", dict(nodes=full[:2], incoming=(SS, (0x8000 - 2 - 0x97) & 0xFFFF),
                                               writes=((DS, 0x31FA, bytes(0x4300 - 0x31FA)),))))
    return spec


def differential(target, code6f, code9b, image, function):
    rng = random.Random(0x6F1446)
    visited, edges, stops = set(), set(), {k: 0 for k in NAMES}
    sites = {s: 0 for s in SITES}
    for label, options in cases(rng):
        cap = options.pop("cap", 64)
        base = options.get("base", 0)
        memory, cpu, high = build(code6f, code9b, image, rng, **options)
        stop, v, e, s, callee = compare(f"+1446 {label}", memory, cpu, high, target, function, cap, base)
        if not label.startswith("random"):
            print(f"  {label}: {NAMES[stop]} callee {callee}")
        stops[stop] += 1
        visited |= v
        edges |= e
        for site, count in s.items():
            sites[site] += count
    reachable = {(s, o) for s, outs in target["conditional"].items() for o in outs}
    missing = sorted(hex(i.address) for i in target["decoded"] if i.address not in visited)
    missing_edges = sorted((hex(s), hex(o)) for s, o in reachable - edges)
    check(not missing and not missing_edges, f"coverage incomplete: {missing} {missing_edges}")
    check(all(stops.values()), f"a stop kind was not exercised: {stops}")
    print(f"U6F +1446: {sum(stops.values())} original-byte cases passed ({', '.join(f'{v} {NAMES[k]}' for k, v in stops.items())}); "
          f"full 1 MiB RAM and all 14 CPU words equal; {len(visited)}/{len(target['decoded'])} instruction starts, "
          f"{len(edges)}/{len(reachable)} conditional outcomes; call-site fetches {dict((hex(k), v) for k, v in sites.items())}")
    return dict(stops={NAMES[k]: v for k, v in stops.items()}, starts=len(visited), outcomes=len(edges),
                site_fetches={f"{k:04X}": v for k, v in sites.items()})


# ---- DOSBox-X synthetic oracle -------------------------------------------------------------------------
CALLER_N, U9B_N, SYS_N, STUB_N, ARENA, DATA_DS = 0x8800, 0x8000, 0x7000, 0x7F00, 0x6000, 0x1900
MAIN_AT, TABLES_AT, BACKGROUND_AT, U9B_AT, FILL_AT, SET_IN_AT, CASES_AT, CODE_AT = 0x0300, 0x0800, 0x2000, 0x3000, 0x3500, 0x3520, 0x3600, 0x5000
OUTPUT, HEADER, STACK_TOP = 0xC000, 0xD800, 0xF000
WINDOW_LOW, WINDOW_SIZE, ARENA_SIZE, WOFF = STACK_TOP - 0x100, 0x100, 0x1000, 0x0884
CASE_SIZE, MAX_NODES, NATIVE_CASES = 40, 8, 160
RECORD_BYTES = 28 + WINDOW_SIZE + 12 + ARENA_SIZE
NATIVE_FLAGS = (0x0002, 0x00D7, 0x08D7, 0x0402, 0x0893, 0x0003)
ENTRY_REGS = dict(ax=0x1234, bx=0x3456, cx=0x5678, dx=0x789A, si=0x9ABC, di=0xBCDE, bp=0x7777)
ACTIVE_OFF, ACTIVE_SEG = 0x2222, 0x1111


def native_cases(rng):
    cases = []
    for n in range(NATIVE_CASES):
        count = n if n <= MAX_NODES else rng.randrange(MAX_NODES + 1)
        filled = count if n <= MAX_NODES else rng.randrange(count + 1)   # slots 0..filled-1 occupied, so the search selects >= filled
        slots = rng.sample(range(filled), filled) + [rng.choice(SLOTS) for _ in range(count - filled)]
        rng.shuffle(slots)
        offsets = [p * 0x110 + rng.randrange(8) for p in rng.sample(range(13), count + 1)]
        cases.append(dict(flags=n % len(NATIVE_FLAGS), count=count, counter=rng.choice((0xFFFF, 0x7FFF, 0x000F, rng.randrange(0x10000))),
                          incoming=offsets[0], classes=[rng.choice(CLASSES) for _ in range(8)], gate=rng.choice((0, 1, 2, 0x7F, 0x80)),
                          age=rng.randrange(256), nodes=list(zip(offsets[1:], slots))))
    return cases


def case_bytes(case):
    data = struct.pack("<BBHH8sBB", case["flags"], case["count"], case["counter"], case["incoming"], bytes(case["classes"]), case["gate"], case["age"])
    for offset, slot in case["nodes"]:
        data += struct.pack("<HB", offset, slot)
    return data + bytes(CASE_SIZE - len(data))


def arena_state(memory, case, background):
    """The harness's per-case writes, in its order: the background, the incoming record, the list, DS globals, the counter."""
    at = physical(ARENA, 0); memory[at:at + ARENA_SIZE] = background
    p = case["incoming"]
    for index, value in enumerate(case["classes"]):
        memory[physical(ARENA, p + 0x96 + index)] = value
    memory[physical(ARENA, p + 0x9C)] = case["gate"]; memory[physical(ARENA, p + 0x12)] = case["age"]
    put16(memory, ARENA, p + 0x104, 0); put16(memory, ARENA, p + 0x106, 0)
    nodes = case["nodes"]
    for i, (offset, slot) in enumerate(nodes):
        memory[physical(ARENA, offset + 0xBF)] = slot
        put16(memory, ARENA, offset + 0x104, nodes[i + 1][0] if i + 1 < len(nodes) else 0)
        put16(memory, ARENA, offset + 0x106, ARENA if i + 1 < len(nodes) else 0)
    head = (nodes[0][0], ARENA) if nodes else (0, 0)
    for address, value in ((0x5D96, head[0]), (0x5D98, head[1]), (0x5D92, ACTIVE_OFF), (0x5D94, ACTIVE_SEG), (0x49D6, WOFF), (0x49D8, ARENA)):
        put16(memory, DATA_DS, address, value)
    put16(memory, ARENA, WOFF + 0x67C, case["counter"])


def native_audit(target, code6f, code9b, image, function):
    del target
    directory = ROOT / "resources/oracle-runs/u6f-1446-synthetic"
    directory.mkdir(parents=True, exist_ok=True)
    fill = image[physical(SYSTEM_CS, 0x16A8):physical(SYSTEM_CS, 0x16A8) + 20]
    set_in = image[physical(SYSTEM_CS, 0x0DC0):physical(SYSTEM_CS, 0x0DC0) + 33]
    unit = bytearray(code6f)
    for site, segment in ((0x14CA, SYS_N), (0x14F2, SYS_N), (0x1574, STUB_N)):
        struct.pack_into("<H", unit, site + 3, segment)
    rng = random.Random(0x6F1446D)
    tables = bytes(rng.randrange(256) for _ in range(0x4300 - 0x31FA))
    background = bytes(rng.randrange(256) for _ in range(ARENA_SIZE))
    cases = native_cases(rng)
    check(TABLES_AT + len(tables) <= BACKGROUND_AT and BACKGROUND_AT + ARENA_SIZE <= U9B_AT and U9B_AT + len(code9b) <= FILL_AT
          and FILL_AT + len(fill) <= SET_IN_AT and SET_IN_AT + len(set_in) <= CASES_AT and CASES_AT + CASE_SIZE * len(cases) <= CODE_AT
          and CODE_AT + len(unit) <= OUTPUT and OUTPUT + RECORD_BYTES <= HEADER, "COM layout overlaps")
    text = [".code16", ".intel_syntax noprefix", ".global _start", "_start:", "    jmp main", f".org {MAIN_AT - 0x100:#x}", "main:",
            "    mov ax,cs", "    mov ds,ax", "    mov es,ax", "    mov ss,ax", f"    mov sp,{STACK_TOP:#x}", "    cld"]
    for segment, offset, label, length in ((CALLER_N, 0, "unit", len(unit)), (U9B_N, 0, "u9b", len(code9b)), (SYS_N, 0x16A8, "fill", len(fill)),
                                           (SYS_N, 0x0DC0, "set_in", len(set_in)), (DATA_DS, 0x31FA, "tables", len(tables))):
        text += [f"    mov ax,{segment:#x}", "    mov es,ax", f"    mov si,offset {label}", f"    mov di,{offset:#x}", f"    mov cx,{length:#x}", "    rep movsb"]
    text += [f"    mov ax,{STUB_N:#x}", "    mov es,ax", f"    mov byte ptr es:[{STUB_IP:#x}],0xea", f"    mov word ptr es:[{STUB_IP + 1:#x}],0",
             f"    mov word ptr es:[{STUB_IP + 3:#x}],{U9B_N:#x}", "    push cs", "    pop es",
             f"    mov word ptr [{HEADER:#x}],cs", f"    mov word ptr [{HEADER + 2:#x}],offset returned",
             f"    mov word ptr [{HEADER + 4:#x}],{len(cases)}", f"    mov word ptr [{HEADER + 6:#x}],0",
             "    mov dx,offset filename", "    xor cx,cx", "    mov ah,0x3c", "    int 0x21", "    jc failure",
             "    mov word ptr [handle],ax", "    mov bx,ax", f"    mov dx,{HEADER:#x}", "    mov cx,8", "    mov ah,0x40", "    int 0x21", "    jc failure",
             "again:", "    cli", f"    mov sp,{STACK_TOP:#x}", "    cld", "    push cs", "    pop ds", "    push cs", "    pop es",
             "    mov al,0xa5", f"    mov di,{WINDOW_LOW:#x}", f"    mov cx,{WINDOW_SIZE:#x}", "    rep stosb",
             "    mov ax,word ptr [index]", f"    mov bx,{CASE_SIZE}", "    mul bx", "    add ax,offset cases", "    mov word ptr [case_ptr],ax",
             f"    mov ax,{ARENA:#x}", "    mov es,ax", "    mov si,offset background", "    xor di,di", f"    mov cx,{ARENA_SIZE:#x}", "    rep movsb",
             "    mov si,word ptr [case_ptr]", "    mov bx,word ptr [si+4]", "    lea di,[bx+0x96]", "    push si", "    add si,6", "    mov cx,8",
             "    rep movsb", "    pop si", "    mov al,byte ptr [si+14]", "    mov byte ptr es:[bx+0x9c],al", "    mov al,byte ptr [si+15]",
             "    mov byte ptr es:[bx+0x12],al", "    mov word ptr es:[bx+0x104],0", "    mov word ptr es:[bx+0x106],0",
             "    lea dx,[si+16]", "    mov cl,byte ptr [si+1]", "    xor ch,ch", "    jcxz nodes_done", "node_loop:", "    mov bx,dx",
             "    mov di,word ptr [bx]", "    mov al,byte ptr [bx+2]", "    mov byte ptr es:[di+0xbf],al", "    xor ax,ax", "    xor bp,bp",
             "    cmp cx,1", "    je node_link", "    mov ax,word ptr [bx+3]", f"    mov bp,{ARENA:#x}", "node_link:",
             "    mov word ptr es:[di+0x104],ax", "    mov word ptr es:[di+0x106],bp", "    add dx,3", "    loop node_loop", "nodes_done:",
             f"    mov ax,{DATA_DS:#x}", "    mov es,ax", "    xor ax,ax", "    xor dx,dx", "    cmp byte ptr [si+1],0", "    je set_head",
             "    mov ax,word ptr [si+16]", f"    mov dx,{ARENA:#x}", "set_head:", "    mov word ptr es:[0x5d96],ax", "    mov word ptr es:[0x5d98],dx",
             f"    mov word ptr es:[0x5d92],{ACTIVE_OFF:#x}", f"    mov word ptr es:[0x5d94],{ACTIVE_SEG:#x}",
             f"    mov word ptr es:[0x49d6],{WOFF:#x}", f"    mov word ptr es:[0x49d8],{ARENA:#x}",
             f"    mov ax,{ARENA:#x}", "    mov es,ax", "    mov ax,word ptr [si+2]", f"    mov word ptr es:[{WOFF + 0x67C:#x}],ax",
             f"    mov ax,{ARENA:#x}", "    push ax", "    push word ptr [si+4]",
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
             f"    mov si,{WINDOW_LOW:#x}", f"    mov cx,{WINDOW_SIZE:#x}", "    rep movsb",
             f"    mov ax,{DATA_DS:#x}", "    mov ds,ax", "    mov si,0x5d92", "    mov cx,8", "    rep movsb", "    mov si,0x49d6", "    mov cx,4", "    rep movsb",
             f"    mov ax,{ARENA:#x}", "    mov ds,ax", "    xor si,si", f"    mov cx,{ARENA_SIZE:#x}", "    rep movsb",
             "    push cs", "    pop ds", "    sti",
             "    mov bx,word ptr [handle]", f"    mov dx,{OUTPUT:#x}", f"    mov cx,{RECORD_BYTES}", "    mov ah,0x40", "    int 0x21", "    jc failure",
             f"    cmp ax,{RECORD_BYTES}", "    jne failure",
             "    inc word ptr [index]", "    mov ax,word ptr [index]", f"    cmp ax,{len(cases)}", "    jb again",
             "    mov bx,word ptr [handle]", "    mov ah,0x3e", "    int 0x21", "    mov ax,0x4c00", "    int 0x21",
             "failure:", "    mov ax,0x4c01", "    int 0x21",
             'filename: .asciz "RESULT.BIN"', "handle: .word 0", "index: .word 0", "case_ptr: .word 0", f"target_far: .word {ENTRY:#x}, {CALLER_N:#x}",
             "flags_table: .word " + ",".join(f"{f:#x}" for f in NATIVE_FLAGS)]
    for label, blob, at in (("tables", tables, TABLES_AT), ("background", background, BACKGROUND_AT), ("u9b", code9b, U9B_AT),
                            ("fill", fill, FILL_AT), ("set_in", set_in, SET_IN_AT), ("cases", b"".join(case_bytes(c) for c in cases), CASES_AT),
                            ("unit", bytes(unit), CODE_AT)):
        text += [f".org {at - 0x100:#x}", f"{label}:"]
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
                       env=environment, stdout=log, stderr=subprocess.STDOUT, check=True, timeout=300)
    result = result_path.read_bytes()
    check(len(result) == 8 + len(cases) * RECORD_BYTES, f"native output incomplete: {len(result)} bytes")
    segment, returned, count, _ = struct.unpack_from("<4H", result)
    check(count == len(cases) and segment == 0x0814, f"native header differs or COM segment {segment:04x} is not 0814")
    slots, lengths = {}, {}
    for n, case in enumerate(cases):
        record = result[8 + n * RECORD_BYTES:8 + (n + 1) * RECORD_BYTES]
        memory = bytearray(SIZE)
        for seg, off, blob in ((CALLER_N, 0, bytes(unit)), (U9B_N, 0, code9b), (SYS_N, 0x16A8, fill), (SYS_N, 0x0DC0, set_in), (DATA_DS, 0x31FA, tables)):
            at = physical(seg, off); memory[at:at + len(blob)] = blob
        at = physical(STUB_N, STUB_IP); memory[at:at + 5] = struct.pack("<BHH", 0xEA, 0x0000, U9B_N)
        arena_state(memory, case, background)
        for i in range(WINDOW_SIZE):
            memory[physical(segment, WINDOW_LOW + i)] = 0xA5
        sp = STACK_TOP
        for value in (ARENA, case["incoming"], segment, returned):
            sp -= 2; put16(memory, segment, sp, value)
        cpu = Cpu(*ENTRY_REGS.values(), sp, CALLER_N, ENTRY, DATA_DS, segment, segment, NATIVE_FLAGS[case["flags"]])
        ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
        callee, callee_status = ctypes.c_int(99), ctypes.c_int(99)
        check(function(ram, SIZE, ctypes.byref(cpu), 64, ctypes.byref(callee), ctypes.byref(callee_status)) == OK, f"C status on native case {n}")
        for (name, _), want in zip(Cpu._fields_, struct.unpack_from("<14H", record)):
            check(getattr(cpu, name) == want, f"native case {n} {name}: C={getattr(cpu, name):04x}, DOS={want:04x}")
        got = bytes(ram[physical(segment, WINDOW_LOW):physical(segment, WINDOW_LOW) + WINDOW_SIZE])
        got += bytes(ram[physical(DATA_DS, 0x5D92):physical(DATA_DS, 0x5D92) + 8]) + bytes(ram[physical(DATA_DS, 0x49D6):physical(DATA_DS, 0x49D6) + 4])
        got += bytes(ram[physical(ARENA, 0):physical(ARENA, 0) + ARENA_SIZE])
        if got != record[28:]:
            differences = [i for i, (a, b) in enumerate(zip(got, record[28:])) if a != b]
            raise ValueError(f"native case {n} bytes differ at {differences[:8]} ({len(differences)} bytes)")
        slot = ram[physical(ARENA, case["incoming"] + 0xBF)]
        slots[slot] = slots.get(slot, 0) + 1
        lengths[case["count"]] = lengths.get(case["count"], 0) + 1
    digest = sha256(result).hexdigest()
    print(f"DOSBox-X normal-core/386 synthetic insertions passed: {len(cases)} (list lengths {dict(sorted(lengths.items()))}, "
          f"selected slots {dict(sorted(slots.items()))}); all 14 CPU words, the {WINDOW_SIZE}-byte stack window, the DS head/active/window1 "
          f"words and the {ARENA_SIZE}-byte record arena; no mask")
    print(f"native result SHA-256 {digest}; COM segment {segment:04x}; U9B at {U9B_N:04X}, SYSTEM leaves at {SYS_N:04X}, stub {STUB_N:04X}:0025")
    return dict(cases=len(cases), lengths={str(k): v for k, v in sorted(lengths.items())}, slots={str(k): v for k, v in sorted(slots.items())},
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
    target, code6f, code9b, image = source()
    report = dict(target=dict(sha256=BODY_SHA, bytes=313, instructions=99, file_offset=target["file_offset"]))
    if args.unicorn or args.native:
        with tempfile.TemporaryDirectory(prefix="poolrad-u6f1446-") as temp:
            function = library(Path(temp))
            if args.unicorn:
                report["unicorn"] = differential(target, code6f, code9b, image, function)
            if args.native:
                report["native"] = native_audit(target, code6f, code9b, image, function)
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(json.dumps(report, sort_keys=True, indent=2) + "\n")
    print(json.dumps(report, sort_keys=True))


if __name__ == "__main__":
    main()
