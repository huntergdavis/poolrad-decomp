#!/usr/bin/env python3
"""Original-byte full-state parity for u25 +1FB1 (public entry 23) with RES_00BA_2199 composed.

The differential places the hash-checked expanded START.EXE image at load segment 0,
so the unrelocated LCALL 00BA:2199 sites reach the original resident bytes, loads
unit 0x25, and runs the original under Unicorn against the composed C entry
pr_u25_entry23_1fb1_composed on a copy. It compares the entire 1 MiB RAM and all 14
CPU words. Worlds are character lists (value byte +11Ch, link +104h/+106h) whose
nodes carry sidecar lists (+7Fh) with keys 27h/2Ah, plus aliasing outputs, cyclic
lists (compared at the host cap's stop) and segment-end stacks. Media stays private.
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

U25_BASE = 0x74D3
ENTRY, END, RETF_AT, LOOP_HEAD = 0x1FB1, 0x207E, 0x207B, 0x1FE0
BODY_SHA = "8d6da69d20a93bea32da791cf65623426c7cd35910e9c9118158c564110882b3"
CALLS = [(0x2004, "lcall", "0xba, 0x2199"), (0x2027, "lcall", "0xba, 0x2199")]
CALLER_CS, DS, SS = 0x3800, 0x1800, 0x2800
CAP = 300
LOOKUP_CAP = 40
RES_ENTRY, RES_LOOP = 0x2D39, 0x2D59        # physical, with the image at segment 0


def check(condition, message):
    if not condition:
        raise ValueError(message)


def source():
    exe = (ROOT / "resources/DOS/POOLRAD/START.EXE").read_bytes()
    ovr = (ROOT / "resources/DOS/POOLRAD/GAME.OVR").read_bytes()
    check(len(exe) == 65231 and sha256(exe).hexdigest() == EXE_SHA, "START.EXE provenance differs")
    check(len(ovr) == 206105 and sha256(ovr).hexdigest() == OVR_SHA, "GAME.OVR provenance differs")
    units = {u["stub_para"]: u for u in find_units(exe[parse_mz(exe)["header_bytes"]:], ovr)}
    u25 = units[0x25]
    check(u25["ovr_offset"] == U25_BASE, "unit 0x25 placement differs")
    code = ovr[U25_BASE:U25_BASE + u25["code_size"]]
    body = code[ENTRY:END]
    decoded = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(body, ENTRY))
    check(sha256(body).hexdigest() == BODY_SHA and len(body) == 205 and len(decoded) == 74 and decoded[-1].address == RETF_AT
          and decoded[-1].mnemonic == "retf" and decoded[-1].op_str == "8", "+1FB1 extent differs")
    calls = [(i.address, i.mnemonic, i.op_str) for i in decoded if "call" in i.mnemonic or i.mnemonic.startswith("int")]
    check(calls == CALLS, f"+1FB1 calls differ: {calls}")
    fixups = set(u25["fixups"])
    check(all(site + 3 in fixups for site, _, _ in CALLS), "a far call segment word is not fixup-backed")
    check(ENTRY in u25["entries"] and u25["entries"].index(ENTRY) == 23, "+1FB1 is not public entry 23")
    sites = {i.address for i in decoded}
    check(all(int(i.op_str, 16) in sites for i in decoded if i.mnemonic.startswith("j")), "+1FB1 branch leaves the body")
    conditional = {i.address: (i.address + i.size, int(i.op_str, 16)) for i in decoded if i.mnemonic.startswith("j") and i.mnemonic != "jmp"}
    check(len(conditional) == 5, "+1FB1 conditional count differs")
    image = bytes(unpack(exe)[0])
    check(sha256(image[0x2D39:0x2DA8]).hexdigest() == "721e86627eac3e72deed41d2a26283e7c95444d99fc4f762a9714b11504dfa9c",
          "resident RES_00BA_2199 bytes differ")
    print(f"u25 +1FB1 (public entry 23, {len(body)}/{len(decoded)}, 2 x LCALL 00BA:2199, RETF 8) sha {BODY_SHA[:16]}...")
    return dict(body=body, decoded=decoded, conditional=conditional, sha=BODY_SHA), code, image


CHECKER_ROWS = ('OVL_0025_1FB1',)


def library(directory):
    path = directory / "u25.so"
    sources = ["overlay_u25_entry23_1fb1.c", "resident_00ba_sidecar_lookup_2199_cpu.c"]
    subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-O2", "-fPIC", "-shared",
                    "-I", str(ROOT / "include"), *[str(ROOT / "src/recovered" / s) for s in sources],
                    "-o", str(path)], check=True, capture_output=True, text=True)
    f = ctypes.CDLL(str(path)).pr_u25_entry23_1fb1_composed
    f.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t, ctypes.POINTER(Cpu), ctypes.c_uint, ctypes.c_uint,
                  ctypes.POINTER(ctypes.c_int)]
    f.restype = ctypes.c_int
    return f


def put_word(memory, segment, offset, value):
    at = physical(segment, offset & 0xFFFF)
    memory[at] = value & 0xFF
    memory[(at + 1) & (SIZE - 1)] = (value >> 8) & 0xFF


def build(code25, image, rng, *, nodes=3, cycle_to=None, keysets=None, sp=0x8000, ss=SS, flags=0x7202, regs=None,
          background=None, alias=None, outputs=None, compact=False):
    memory = bytearray([0xA5]) * SIZE if background is None else bytearray(background)
    memory[0:len(image)] = image
    at = physical(CALLER_CS, 0); memory[at:at + len(code25)] = code25
    where = [((0x6000 + 0x20 * i) & 0xFFFF, 0x0010 + 16 * rng.randrange(0x08)) for i in range(nodes)]
    values, keys = [], []
    for i, (seg, off) in enumerate(where):
        for k in range(0x120):
            memory[physical(seg, off + k)] = rng.randrange(256)
        value = rng.randrange(256)
        memory[physical(seg, off + 0x11C)] = value
        nxt = where[i + 1] if i + 1 < nodes else (where[cycle_to] if cycle_to is not None else (0, 0))
        put_word(memory, seg, off + 0x104, nxt[1]); put_word(memory, seg, off + 0x106, nxt[0])
        chosen = list(keysets[i]) if keysets is not None else ([0x27] if rng.random() < 0.35 else []) + \
            ([0x2A] if rng.random() < 0.35 else []) + rng.sample([b for b in range(256) if b not in (0x27, 0x2A)], rng.randrange(0, 3))
        rng.shuffle(chosen)
        head = (0, 0)
        for j, key in reversed(list(enumerate(chosen))):
            node = (0x0040 + 0x20 * j + rng.randrange(0x10), 0x7000 + 0x40 * i)
            memory[physical(node[1], node[0])] = key
            put_word(memory, node[1], node[0] + 5, head[0]); put_word(memory, node[1], node[0] + 7, head[1])
            head = node
        put_word(memory, seg, off + 0x7F, head[0]); put_word(memory, seg, off + 0x81, head[1])
        values.append(value); keys.append(chosen)
    head = where[0] if nodes else (0, 0)
    put16(memory, DS, 0x5D96, head[1]); put16(memory, DS, 0x5D98, head[0])
    out = outputs or ((0x6800, 0x0010), (0x6800, 0x0020))          # (min, max) as (segment, offset)
    if alias == "min-is-value" and nodes:
        out = ((where[0][0], where[0][1] + 0x11C), out[1])
    elif alias == "max-is-link" and nodes:
        out = (out[0], (where[0][0], where[0][1] + 0x104))
    elif alias == "max-in-frame":
        out = (out[0], (ss, (sp - 9) & 0xFFFF))                    # the value byte [BP-9]
    elif alias == "min-on-cursor":
        out = ((ss, (sp - 10) & 0xFFFF), out[1])                   # the cursor cell [BP-8]
    for seg, off in out:
        memory[physical(seg, off)] = rng.randrange(256)
    words = [0x7000, 0x7001, out[0][1], out[0][0], out[1][1], out[1][0]]
    frame = physical(ss, sp)
    for i, value in enumerate(words):
        put16(memory, ss, (sp + 2 * i) & 0xFFFF, value)
        memory[(frame + 2 * i) & (SIZE - 1)] = value & 0xFF
        memory[(frame + 2 * i + 1) & (SIZE - 1)] = value >> 8
    reg = regs or tuple(rng.randrange(0x10000) for _ in range(7))
    cpu = Cpu(*reg, sp, CALLER_CS, ENTRY, DS, rng.randrange(0x10000), ss, flags)
    expect = None
    if alias is None and cycle_to is None and nodes:
        lo = hi = values[0]
        for value, chosen in zip(values, keys):
            v = (value << 1) & 0xFF if 0x27 in chosen else (value >> 1 if 0x2A in chosen else value)
            hi = v if v > hi else hi
            lo = v if v < lo else lo
        expect = (lo, hi, out)
    return memory, cpu, expect, ss >= 0xF000, out


def compare(label, memory, cpu, high, target, function, cap=CAP, lookup_cap=LOOKUP_CAP, budget=3000000):
    from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
    original = Uc(UC_ARCH_X86, UC_MODE_16)
    original.mem_map(0, 2 * SIZE)
    original.mem_write(0, bytes(memory)); original.mem_write(SIZE, bytes(memory))
    for (name, _), register in zip(Cpu._fields_, REGISTERS):
        original.reg_write(register, getattr(cpu, name))
    start = physical(cpu.cs, ENTRY); body_end = start + len(target["body"]); ret = physical(0x7001, 0x7000)
    previous, visited, edges, returned, stopped, arrivals = [None], set(), set(), [False], [False], [0]
    in_lookup, res_arrivals, lookup_stop = [0], [0], [False]

    def instruction(emulator, address, size, user):
        del size, user
        here = address - start + ENTRY
        if here == LOOP_HEAD:
            arrivals[0] += 1
            if arrivals[0] == cap + 2:
                stopped[0] = True; emulator.emu_stop(); return
        visited.add(here)
        if previous[0] in target["conditional"]:
            edges.add((previous[0], here))
        previous[0] = here

    def outside(emulator, address, size, user):
        del size, user
        previous[0] = None
        if address == RES_ENTRY:
            res_arrivals[0] = 0
        elif address == RES_LOOP:
            res_arrivals[0] += 1
            if res_arrivals[0] == lookup_cap + 2:
                lookup_stop[0] = True; emulator.emu_stop()

    def leave(emulator, address, size, user):
        del address, size, user
        returned[0] = True
        emulator.emu_stop()

    def mirror(emulator, access, address, size, value, user):
        del access, user
        for byte in range(size):
            raw = address + byte; at = raw & (SIZE - 1)
            emulator.mem_write(at + SIZE if raw < SIZE else at, bytes([(value >> (8 * byte)) & 0xFF]))

    original.hook_add(UC_HOOK_CODE, instruction, begin=start, end=body_end - 1)
    original.hook_add(UC_HOOK_CODE, outside, begin=0, end=start - 1)
    original.hook_add(UC_HOOK_CODE, outside, begin=body_end, end=ret - 1)
    original.hook_add(UC_HOOK_CODE, leave, begin=ret, end=ret)
    if high:
        original.hook_add(UC_HOOK_MEM_WRITE, mirror)
    original.emu_start(start, 2 * SIZE, count=budget)
    check(returned[0] or stopped[0] or lookup_stop[0], f"original neither returned nor reached a cap: {label}")
    expected = bytes(original.mem_read(0, SIZE))
    ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
    callee = ctypes.c_int(0)
    status = function(ram, SIZE, ctypes.byref(cpu), cap, lookup_cap, ctypes.byref(callee))
    want_status = 0 if returned[0] else (6 if stopped[0] else 4)
    check(status == want_status and (not lookup_stop[0] or callee.value == 2),
          f"C status {status} (callee {callee.value}), original {'returned' if returned[0] else 'cap' if stopped[0] else 'lookup cap'}: {label}")
    actual = bytes(ram)
    if actual != expected:
        differences = [i for i, (a, b) in enumerate(zip(actual, expected)) if a != b]
        raise ValueError(f"RAM mismatch {label}: {[(hex(d), actual[d], expected[d]) for d in differences[:6]]} ({len(differences)} bytes)")
    for (name, _), register in zip(Cpu._fields_, REGISTERS):
        got, want = getattr(cpu, name), original.reg_read(register) & 0xFFFF
        check(got == want, f"{name} mismatch {label}: C={got:04x}, original={want:04x}")
    if stopped[0]:
        return None, f"+1FE0#{arrivals[0]}", ram
    if lookup_stop[0]:
        return None, f"21B9#{res_arrivals[0]}", ram
    return visited, edges, ram


def cases():
    spec = []
    for i in range(240):
        spec.append((f"random-{i}", dict(nodes=random.Random(i).randrange(1, 9), flags=(0x7202, 0x0202, 0x0ED7, 0x0647)[i % 4])))
    for n in range(1, 6):
        for pattern in ([0x27], [0x2A], [0x27, 0x2A], [0x2A, 0x27], [], [0x11]):
            spec.append((f"keys-{n}-{'-'.join(f'{k:02x}' for k in pattern) or 'none'}", dict(nodes=n, keysets=[pattern] * n)))
    spec.append(("null-head", dict(nodes=0)))
    for alias in ("min-is-value", "max-is-link", "max-in-frame", "min-on-cursor"):
        for i in range(6):
            spec.append((f"{alias}-{i}", dict(nodes=1 + i % 4, alias=alias)))
    for sp in (0x0020, 0x0040, 0xFFE0, 0xFFF0, 0xFFF4):
        spec.append((f"sp-{sp:04x}", dict(nodes=3, sp=sp, ss=0x1C00, background=bytes(SIZE))))
    spec.append(("ss-a20", dict(nodes=2, ss=0xFFFF, sp=0x0400)))
    spec.append(("random-background", dict(nodes=4, background=bytes(random.Random(9).randbytes(SIZE)))))
    spec.append(("preserved-df-if", dict(nodes=2, flags=0x0603)))
    for n, back in ((1, 0), (2, 0), (3, 1), (5, 2)):
        spec.append((f"cycle-{n}-to-{back}", dict(nodes=n, cycle_to=back)))
    return spec


def differential(target, code25, image, function):
    rng = random.Random(0x251FB1)
    visited, edges, count, limited, semantic = set(), set(), 0, [], 0
    for label, options in cases():
        memory, cpu, expect, high, _ = build(code25, image, rng, **options)
        v, e, ram = compare(f"+1FB1 {label}", memory, cpu, high, target, function)
        if v is None:
            check(label.startswith("cycle-") or label.startswith("max-is-link") or label.startswith("min-is-value"),
                  f"unexpected cap stop for {label}")
            limited.append(f"{label}@{e}")
            continue
        visited |= v; edges |= e; count += 1
        if expect is not None:
            lo, hi, out = expect
            got = (ram[physical(out[0][0], out[0][1])], ram[physical(out[1][0], out[1][1])])
            check(got == (lo, hi), f"{label}: min/max {got}, documented {(lo, hi)}")
            semantic += 1
    check({x.split("@")[0] for x in limited} >= {"cycle-1-to-0", "cycle-2-to-0", "cycle-3-to-1", "cycle-5-to-2"},
          f"cycles did not all stop: {limited}")
    missing_sites = sorted(hex(s) for s in set(i.address for i in target["decoded"]) - visited)
    missing_edges = sorted((hex(s), hex(o)) for s, outs in target["conditional"].items() for o in outs if (s, o) not in edges)
    print(f"+1FB1: {count} returning original-byte cases passed, full 1 MiB RAM and all 14 CPU words equal; {semantic} with min/max "
          f"checked against the documented walk; {len(visited)}/{len(target['decoded'])} instruction starts, "
          f"{len(edges)}/{2 * len(target['conditional'])} conditional outcomes")
    print(f"+1FB1: {len(limited)} host-cap stops equal the original stopped at the same loop-head arrival: {', '.join(limited)}")
    check(not missing_sites and not missing_edges, f"+1FB1 coverage incomplete: {missing_sites} {missing_edges}")
    return dict(returning=count, semantic=semantic, limit_stops=len(limited), starts=len(visited), outcomes=len(edges))


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
    target, code25, image = source()
    report = dict(target=dict(sha256=target["sha"], bytes=len(target["body"]), instructions=len(target["decoded"])))
    if args.unicorn or args.native:
        with tempfile.TemporaryDirectory(prefix="poolrad-u25-1fb1-") as temp:
            function = library(Path(temp))
            if args.unicorn:
                report["unicorn"] = differential(target, code25, image, function)
            if args.native:
                from u25_1fb1_composed_native import native_audit
                report["native"] = native_audit(target, code25, image, function)
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(json.dumps(report, sort_keys=True, indent=2) + "\n")
    print(json.dumps(report, sort_keys=True))


if __name__ == "__main__":
    main()
