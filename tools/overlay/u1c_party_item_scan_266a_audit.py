#!/usr/bin/env python3
"""Original-byte full-state parity for raw-CPU U1C +266A with U25 +0000/+0136 (through the loaded overlay stub) and
SYSTEM FillChar composed, plus a DOSBox-X oracle.

The differential places the hash-checked expanded START.EXE image at load segment 0 (SYSTEM and the overlay stub
table live), patches unit 0x25's stub entries 002B:0025 and 002B:002A to their loaded EA jumps, loads unit 0x25 at
its own segment and unit 0x1C at the caller's, and compares the entire 1 MiB RAM and all 14 CPU words between the
original under Unicorn and the C model, with no mask. The original decides each stop: its return (wherever RETF
goes), a rewritten site or stub (CODE), a callee return elsewhere (TRANSFER) or the walk cap (LIMIT). Media stays
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
from resident_loader_map import unpack
from u86_procvars_3413_audit import Cpu, ROOT, SIZE, EXE_SHA, OVR_SHA, REGISTERS, physical, put16

ENTRY, END = 0x266A, 0x271F
SITES = {0x2673: (0x002A, 0x0000), 0x267F: (0x0025, 0x0136), 0x2693: (0x16A8, None)}
LOOP_HEADS = {0x26CB: 0x2704, 0x26AA: 0x2719}
BODY_RETF = 0x271E
STUB_SEG, SYSTEM_CS = 0x002B, 0x0AF8
CALLER_CS, U25_CS, DS, SS, EDGE_SS = 0x8800, 0x7800, 0x1800, 0x2800, 0x2000
RET_CS, RET_IP = 0x9001, 0x7000
OK, CODE, CALLEE, TRANSFER, LIMIT = 0, 2, 3, 4, 5
NAMES = {OK: "return", CODE: "CODE", TRANSFER: "TRANSFER", LIMIT: "LIMIT"}
FILLCHAR_SHA = "f1c5fade96970aa1dd1a9ffc2d876c7aea57bef8680d929d9f65d6c5190e17f7"


class Limits(ctypes.Structure):
    _fields_ = [("system_cs", ctypes.c_uint16), ("max_operands", ctypes.c_size_t), ("max_string_iterations", ctypes.c_size_t),
                ("max_loopbacks", ctypes.c_uint)]


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
    check(END == next(s for s in starts if s > ENTRY), "U1C +266A bounds differ")
    body = code1c[ENTRY:END]
    decoded = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(body, ENTRY))
    calls = [(i.address, i.op_str) for i in decoded if "call" in i.mnemonic]
    fixups = set(u1c["fixups"])
    check(len(body) == 181 and len(decoded) == 61 and sum(i.size for i in decoded) == 181 and decoded[-1].mnemonic == "retf"
          and decoded[-1].op_str == "" and calls == [(0x2673, "0x2b, 0x2a"), (0x267F, "0x2b, 0x25"), (0x2693, "0xaf8, 0x16a8")]
          and all(s + 3 in fixups for s in SITES), f"U1C +266A extent or calls differ: {calls}")
    target = dict(body=body, decoded=decoded, conditional=conditionals(decoded), sha=sha256(body).hexdigest(),
                  file_offset=u1c["ovr_offset"] + ENTRY)
    print(f"U1C +266A (181/61, LCALL 002B:002A and 002B:0025 to U25 +0000/+0136, LCALL 0AF8:16A8, RETF) GAME file "
          f"{target['file_offset']:#x} sha {target['sha']}; U25 code {len(code25):#x} bytes")
    return target, code1c, code25, image


def library(directory):
    path = directory / "u1c.so"
    sources = ["overlay_u1c_party_item_scan_266a_cpu.c", "overlay_u25_loader_0000.c", "overlay_u25_slot_value_0136.c",
               "overlay_u25_string_cores.c", "overlay_u25_entry1_0136.c", "overlay_u25_load_word_0e0c.c", "overlay_u25_party_index_0756.c",
               "system_runtime_leaves_cpu.c"]
    result = subprocess.run(["cc", "-std=c11", "-O2", "-fPIC", "-shared", "-I", str(ROOT / "include")] +
                            [str(ROOT / "src/recovered" / s) for s in sources] + ["-o", str(path)], capture_output=True, text=True)
    if result.returncode != 0:
        raise ValueError(result.stderr[-3000:])
    function = ctypes.CDLL(str(path)).pr_u1c_party_item_scan_266a_cpu
    function.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t, ctypes.POINTER(Cpu), ctypes.POINTER(Limits),
                         ctypes.POINTER(ctypes.c_int), ctypes.POINTER(ctypes.c_int)]
    function.restype = ctypes.c_int
    return function


def world_spec(rng, *, tag=None, value=None, members=None, items=None, match=None):
    members = rng.randrange(0, 5) if members is None else members
    tag = rng.choice((0, 0, 0, 2, 0x81, 0x10, 0x55)) if tag is None else tag
    value = rng.randrange(256) if value is None else value
    third = rng.randrange(256)
    if tag == 0x81:                                                  # string by address 4900h/4901h: the 'Q' or empty window string
        value, third = rng.choice((0, 1)), 0x49
    elif tag == 0x80:                                                # a short packed string from the stream
        value = rng.randrange(0, 7)
    lists = [[rng.randrange(256) for _ in range(rng.randrange(0, 5) if items is None else items)] for _ in range(members)]
    if match and lists and any(lists):
        k = rng.choice([i for i, l in enumerate(lists) if l])
        lists[k][rng.randrange(len(lists[k]))] = value                # an item whose type equals the operand value
    return dict(tag=tag, value=value, third=third, lists=lists)


def build(code1c, code25, image, rng, spec, *, sp=0x8000, ss=SS, flags=0x7202, background=None, patch=(), stub_patch=(),
          cyclic=None, party_seg=0x5000, item_seg=0x5800):
    memory = bytearray([0xA5]) * SIZE if background is None else bytearray(background)
    memory[0:len(image)] = image
    for base, code in ((physical(CALLER_CS, 0), code1c), (physical(U25_CS, 0), code25)):
        memory[base:base + len(code)] = code
    for offset, value in patch:
        memory[physical(CALLER_CS, offset)] = value
    for offset, ip in ((0x0025, 0x0136), (0x002A, 0x0000)):          # the loaded stub: JMP FAR U25:entry
        memory[physical(STUB_SEG, offset):physical(STUB_SEG, offset) + 5] = struct.pack("<BHH", 0xEA, ip, U25_CS)
    for offset, value in stub_patch:
        memory[physical(STUB_SEG, offset)] = value
    # ECL operand stream: one operand (tag, value[, third]) read through DS:49DE + 6700h + PC
    put16(memory, DS, 0x49DE, 0x1000); put16(memory, DS, 0x49E0, 0x6300)
    put16(memory, DS, 0x49D2, 0x1000); put16(memory, DS, 0x49D4, 0x6000)
    put16(memory, DS, 0x49ED, 0x9900)
    for i, b in enumerate((spec["tag"], spec["value"], spec["third"], 0x41, 0x42, 0x43)):
        memory[physical(0x6300, 0x1001 + i)] = b
    put16(memory, 0x6000, 0x1000, 0x51); put16(memory, 0x6000, 0x1002, 0)
    # party list at party_seg, each member's item list at item_seg
    members = []
    item_off = 0x0100
    for k, items in enumerate(spec["lists"]):
        member = 0x0100 + 0x200 * k
        members.append(member)
        offsets = []
        for t in items:
            offsets.append(item_off)
            memory[physical(item_seg, item_off + 0x2E)] = t
            item_off += 0x40
        for j, off in enumerate(offsets):
            nxt = offsets[j + 1] if j + 1 < len(offsets) else (offsets[0] if cyclic == "items" and k == 0 else 0)
            put16(memory, item_seg, off + 0x2A, nxt); put16(memory, item_seg, off + 0x2C, item_seg if nxt else 0)
        put16(memory, party_seg, member + 0xC8, offsets[0] if offsets else 0)
        put16(memory, party_seg, member + 0xCA, item_seg if offsets else 0)
    for k, member in enumerate(members):
        nxt = members[k + 1] if k + 1 < len(members) else (members[0] if cyclic == "party" else 0)
        put16(memory, party_seg, member + 0x104, nxt); put16(memory, party_seg, member + 0x106, party_seg if nxt else 0)
    put16(memory, DS, 0x5D96, members[0] if members else 0); put16(memory, DS, 0x5D98, party_seg if members else 0)
    frame = physical(ss, sp)
    for i, value in enumerate([RET_IP, RET_CS]):
        put16(memory, ss, (sp + 2 * i) & 0xFFFF, value)
        put16(memory, 0, (frame + 2 * i) & (SIZE - 1), value)
    reg = tuple(rng.randrange(0x10000) for _ in range(7))
    high = ss >= 0xF000 or party_seg >= 0xF000 or item_seg >= 0xF000
    return memory, Cpu(*reg, sp, CALLER_CS, ENTRY, DS, rng.randrange(0x10000), ss, flags), high


def compare(label, memory, cpu, high, target, function, cap, budget=3000000):
    from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
    original = Uc(UC_ARCH_X86, UC_MODE_16)
    original.mem_map(0, 2 * SIZE)
    original.mem_write(0, bytes(memory))
    original.mem_write(SIZE, bytes(memory))
    for (name, _), register in zip(Cpu._fields_, REGISTERS):
        original.reg_write(register, getattr(cpu, name))
    base = physical(CALLER_CS, 0)
    body = (base + ENTRY, base + END)
    callee_regions = [(physical(U25_CS, 0), physical(U25_CS, 0) + 0x10000), (physical(SYSTEM_CS, 0), physical(SYSTEM_CS, 0) + 0x2000),
                      (physical(STUB_SEG, 0x0025), physical(STUB_SEG, 0x002F))]
    sites = {}
    for site, (offset, ip) in SITES.items():
        at = base + site
        good = bytes(memory[at:at + 3]) == struct.pack("<BH", 0x9A, offset)
        if good and ip is not None:
            seg = memory[at + 3] | memory[at + 4] << 8
            stub = physical(seg, offset)
            good = bytes(memory[stub:stub + 3]) == struct.pack("<BH", 0xEA, ip)
        sites[at] = (good, at + 5)
    jumps = {base + jmp: base + head for head, jmp in LOOP_HEADS.items()}
    state = dict(stop=None, expected=None, in_callee=False, returning=False, previous=None, loopbacks=0, last=None)
    visited, edges = set(), set()

    def hook(emulator, address, size, user):
        del size, user
        last, state["last"] = state["last"], address
        if state["returning"]:
            state["stop"] = OK
            emulator.emu_stop()
            return
        inside_body = body[0] <= address < body[1]
        inside_callee = any(lo <= address < hi for lo, hi in callee_regions)
        if state["in_callee"] and not inside_callee:
            state["in_callee"] = False
            if address != state["expected"]:
                state["stop"] = TRANSFER
                emulator.emu_stop()
                return
        if not inside_body and not inside_callee:
            state["stop"] = TRANSFER
            emulator.emu_stop()
            return
        if inside_callee:
            state["in_callee"] = True
            state["previous"] = None
            return
        if address in sites:
            good, after = sites[address]
            if not good:
                state["stop"] = CODE
                emulator.emu_stop()
                return
            state["expected"] = after
        if last in jumps and address == jumps[last]:
            state["loopbacks"] += 1
            if state["loopbacks"] > cap:
                state["stop"] = LIMIT
                emulator.emu_stop()
                return
        here = address - base
        visited.add(here)
        if state["previous"] in target["conditional"] and state["previous"] != here:
            edges.add((state["previous"], here))
        state["previous"] = here
        if here == BODY_RETF:
            state["returning"] = True

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
    limits = Limits(SYSTEM_CS, 4, 400, cap)
    status = function(ram, SIZE, ctypes.byref(cpu), ctypes.byref(limits), ctypes.byref(callee), ctypes.byref(callee_status))
    check(status == state["stop"], f"C status {status} (callee {callee.value}/{callee_status.value}), original stopped with "
          f"{NAMES[state['stop']]}: {label}")
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
    for i in range(150):
        spec.append((f"random-{i}", dict(spec=world_spec(rng, match=i % 2 == 0), flags=(0x7202, 0x0602, 0x0ED7, 0xF2D7)[i % 4],
                                          sp=rng.randrange(0x0800, 0xF000) & ~1)))
    for tag in (0, 2, 0x81, 0x10, 0x80, 0x80):
        spec.append((f"tag-{tag:02x}-match", dict(spec=world_spec(rng, tag=tag, members=3, items=3, match=True))))
    spec.append(("empty-party", dict(spec=world_spec(rng, members=0))))
    spec.append(("members-without-items", dict(spec=world_spec(rng, members=3, items=0))))
    spec.append(("cyclic-party", dict(spec=world_spec(rng, tag=0, value=0x77, members=3, items=0), cyclic="party", cap=40)))
    spec.append(("cyclic-items", dict(spec=world_spec(rng, tag=0, value=0x77, members=2, items=3), cyclic="items", cap=40)))
    spec.append(("cap-1", dict(spec=world_spec(rng, tag=0, value=0x77, members=3, items=3), cap=1)))
    for sp in (0x0400, 0xFFF0, 0xFFFC):
        spec.append((f"sp-{sp:04x}", dict(spec=world_spec(rng, members=2, items=2, match=True), sp=sp, ss=EDGE_SS)))
    spec.append(("ss-a20", dict(spec=world_spec(rng, members=2, items=2, match=True), sp=0x0400, ss=0xFFFF)))
    spec.append(("lists-a20", dict(spec=world_spec(rng, members=2, items=2, match=True), party_seg=0xFFF8, item_seg=0xFFF0)))
    spec.append(("random-background", dict(spec=world_spec(rng, members=3, items=3, match=True),
                                           background=bytes(random.Random(0x1C266A).randbytes(SIZE)))))
    spec.append(("code-site1", dict(spec=world_spec(rng), patch=((0x2673, 0x90),))))
    spec.append(("code-stub2", dict(spec=world_spec(rng, tag=0), stub_patch=((0x0026, 0x37),))))
    spec.append(("code-fillchar", dict(spec=world_spec(rng, tag=0), patch=((0x2694, 0xA9),))))
    return spec


def differential(target, code1c, code25, image, function):
    rng = random.Random(0x1C266A)
    visited, edges, stops = set(), set(), {k: 0 for k in NAMES}
    for label, options in cases(rng):
        cap = options.pop("cap", 1000)
        memory, cpu, high = build(code1c, code25, image, rng, options.pop("spec"), **options)
        stop, v, e = compare(f"+266A {label}", memory, cpu, high, target, function, cap)
        stops[stop] += 1
        visited |= v
        edges |= e
    missing = sorted(hex(i.address) for i in target["decoded"] if i.address not in visited)
    reachable = {(s, o) for s, outs in target["conditional"].items() for o in outs}
    missing_edges = sorted((hex(s), hex(o)) for s, o in reachable - edges)
    check(not missing and not missing_edges, f"coverage incomplete: {missing} {missing_edges}")
    check(all(stops[k] for k in (CODE, LIMIT)), f"a stop kind was not exercised: {stops}")
    print(f"U1C +266A: {sum(stops.values())} original-byte cases passed ({stops[OK]} returns, {stops[CODE]} CODE, {stops[TRANSFER]} TRANSFER, "
          f"{stops[LIMIT]} LIMIT); full 1 MiB RAM and all 14 CPU words equal; {len(visited)}/{len(target['decoded'])} instruction starts, "
          f"{len(edges)}/{len(reachable)} conditional outcomes")
    return dict(stops={NAMES[k]: v for k, v in stops.items()}, starts=len(visited), outcomes=len(edges))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--unicorn", action="store_true")
    parser.add_argument("--native", action="store_true")
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    target, code1c, code25, image = source()
    report = dict(target=dict(sha256=target["sha"], bytes=181, instructions=61, file_offset=target["file_offset"]))
    if args.unicorn or args.native:
        with tempfile.TemporaryDirectory(prefix="poolrad-u1c-") as temp:
            function = library(Path(temp))
            if args.unicorn:
                report["unicorn"] = differential(target, code1c, code25, image, function)
            if args.native:
                from u1c_party_item_scan_266a_native import native_audit
                report["native"] = native_audit(target, code1c, code25, image, function)
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(json.dumps(report, sort_keys=True, indent=2) + "\n")
    print(json.dumps(report, sort_keys=True))


if __name__ == "__main__":
    main()
