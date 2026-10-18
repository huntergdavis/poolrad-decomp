#!/usr/bin/env python3
"""Original-byte full-state parity for resident RES_00BA_2199 as a raw-CPU model, plus a DOSBox-X oracle.

The routine's 111 bytes come from the hash-checked expanded START.EXE image that
resident_loader_map.unpack rebuilds. The Unicorn differential places that whole
image at load segment 0800h and compares the entire 1 MiB RAM and all 14 CPU words
for random and directed sidecar worlds. Those include cursor cells that alias the
frame, the arguments, the character head or a node, cyclic lists (compared at the
host cap's stop), stacks at both segment ends, A20 and straddling node words. The
DOSBox-X oracle runs fixed worlds at absolute segments. Media stays private.
"""
import argparse
import csv
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
from resident_loader_map import unpack
from u86_procvars_3413_audit import Cpu, ROOT, SIZE, EXE_SHA, REGISTERS, physical, put16

ENTRY, IMAGE_AT, END, RETF_AT = 0x2199, 0x2D39, 0x2DA8, 0x2205
LOOP_HEAD = ENTRY + 0x20
LOAD = 0x0800
CS = LOAD + 0x00BA
SS = 0x2000
CAP = 600


def check(condition, message):
    if not condition:
        raise ValueError(message)


def source():
    exe = (ROOT / "resources/DOS/POOLRAD/START.EXE").read_bytes()
    check(len(exe) == 65231 and sha256(exe).hexdigest() == EXE_SHA, "START.EXE provenance differs")
    image = bytes(unpack(exe)[0])
    body = image[IMAGE_AT:END]
    decoded = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(body, ENTRY))
    check(body[:3] == b"\x55\x89\xe5" and len(body) == 111 and len(decoded) == 37 and sum(i.size for i in decoded) == 111
          and decoded[-1].address == RETF_AT and decoded[-1].mnemonic == "retf" and decoded[-1].op_str == "0xa",
          "RES_00BA_2199 extent differs")
    check(not any("call" in i.mnemonic or i.mnemonic.startswith("int") for i in decoded), "RES_00BA_2199 is not a leaf")
    check(not any(i.mnemonic in ("jmp", "je", "jne") and not ENTRY <= int(i.op_str, 16) < ENTRY + 111
                  for i in decoded if i.mnemonic.startswith("j")), "a branch leaves the body")
    conditional = {i.address: (i.address + i.size, int(i.op_str, 16)) for i in decoded if i.mnemonic.startswith("j") and i.mnemonic != "jmp"}
    check(len(conditional) == 3, "RES_00BA_2199 conditional count differs")
    check(not any("ds:" in i.op_str or (("[" in i.op_str) and "es:" not in i.op_str and "bp" not in i.op_str) for i in decoded),
          "RES_00BA_2199 touches DS")
    row = next(r for r in csv.reader((ROOT / "docs/re/resident_callable_ledger.csv").open()) if r and r[0] == "RES_00BA_2199")
    target = dict(body=body, decoded=decoded, conditional=conditional, sha=sha256(body).hexdigest())
    print(f"RES_00BA_2199 (expanded {IMAGE_AT:#06x}..{END - 1:#06x}, {len(body)}/{len(decoded)}, leaf, RETF 0Ah) "
          f"sha {target['sha'][:16]}...; callable ledger: {','.join(row[1:10])}")
    return target, image


def library(directory):
    path = directory / "res2199.so"
    subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-O2", "-fPIC", "-shared",
                    "-I", str(ROOT / "include"), str(ROOT / "src/recovered/resident_00ba_sidecar_lookup_2199_cpu.c"),
                    "-o", str(path)], check=True, capture_output=True, text=True)
    f = ctypes.CDLL(str(path)).pr_res_00ba_2199_cpu
    f.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t, ctypes.POINTER(Cpu), ctypes.c_uint]
    f.restype = ctypes.c_int
    return f


def put_word(memory, segment, offset, value):
    """A word as the CPU reads it at effective address `offset`: 16-bit EA wrap, then two contiguous bytes."""
    at = physical(segment, offset & 0xFFFF)
    memory[at] = value & 0xFF
    memory[(at + 1) & (SIZE - 1)] = (value >> 8) & 0xFF


def put_frame(memory, ss, sp, words):
    """Return frame and arguments in both the segment-wrapped and the physically contiguous views."""
    for i, value in enumerate(words):
        put16(memory, ss, (sp + 2 * i) & 0xFFFF, value)
    base = physical(ss, sp)
    for i, value in enumerate(words):
        memory[(base + 2 * i) & (SIZE - 1)] = value & 0xFF
        memory[(base + 2 * i + 1) & (SIZE - 1)] = value >> 8


def build(image, rng, *, nodes=3, hit=None, cycle_to=None, cursor="free", sp=0x8000, ss=SS, flags=0x7202, regs=None,
          background=None, key_high=None, straddle=False, a20=False):
    memory = bytearray([0xA5]) * SIZE if background is None else bytearray(background)
    memory[LOAD * 16:LOAD * 16 + len(image)] = image
    slots = rng.sample(range(0x30, 0xA0), nodes + 2)

    def place(slot):
        d = rng.randrange(0x10)
        return ((slot << 8) - d) & 0xFFFF, (16 * d + rng.randrange(0, 0xEF0)) & 0xFFFF

    where = [place(slots[i]) for i in range(nodes)]
    if straddle and nodes:
        where[-1] = ((slots[nodes - 1] << 8) - 0x1000 + 0x0F) & 0xFFFF, 0xFFFA     # +5 word at EA FFFF, +7 at EA 0001
    if a20 and nodes:
        where[0] = 0xFFFF, 0x0100 + 16 * rng.randrange(0x40)                         # wraps to the first 64 KB
    keys = rng.sample(range(256), nodes)
    low = keys[hit] if hit is not None else rng.choice([b for b in range(256) if b not in keys])
    for i, (seg, off) in enumerate(where):
        memory[physical(seg, off)] = keys[i]
        for k in range(1, 5):
            memory[physical(seg, (off + k) & 0xFFFF)] = rng.randrange(256)
        nxt = where[i + 1] if i + 1 < nodes else (where[cycle_to] if cycle_to is not None else (0, 0))
        put_word(memory, seg, off + 5, nxt[1]); put_word(memory, seg, off + 7, nxt[0])
    x = (0xFFFF, 0x0400 + 16 * rng.randrange(0x40)) if a20 else place(slots[nodes])
    head = where[0] if nodes else (0, 0)
    put_word(memory, x[0], x[1] + 0x7F, head[1]); put_word(memory, x[0], x[1] + 0x81, head[0])
    if cursor == "free":
        c = place(slots[nodes + 1])
        for k in range(4):
            memory[physical(c[0], (c[1] + k) & 0xFFFF)] = rng.randrange(256)
    elif cursor == "head":
        c = x[0], (x[1] + 0x7F) & 0xFFFF
    elif cursor == "frame":
        c = ss, (sp - 4) & 0xFFFF
    elif cursor == "args":
        c = ss, (sp + 4) & 0xFFFF
    elif cursor == "node5":
        c = where[0][0], (where[0][1] + 5) & 0xFFFF
    else:
        raise ValueError(cursor)
    key_word = ((rng.randrange(256) if key_high is None else key_high) << 8) | low
    put_frame(memory, ss, sp, (0x7000, 0x7001, c[1], c[0], key_word, x[1], x[0]))
    r = regs or tuple(rng.randrange(0x10000) for _ in range(7))
    cpu = Cpu(*r, sp, CS, ENTRY, rng.randrange(0x10000), rng.randrange(0x10000), ss, flags)
    high = a20 or ss >= 0xF000
    expect = None
    if cursor == "free" and not straddle and not a20 and (cycle_to is None or hit is not None):
        expect = (1, where[hit]) if hit is not None else (0, (0, 0))
    return memory, cpu, high, expect, c


def compare(label, memory, cpu, high, target, function, cap=CAP, budget=200000):
    from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
    original = Uc(UC_ARCH_X86, UC_MODE_16)
    original.mem_map(0, 2 * SIZE)
    original.mem_write(0, bytes(memory)); original.mem_write(SIZE, bytes(memory))
    for (name, _), register in zip(Cpu._fields_, REGISTERS):
        original.reg_write(register, getattr(cpu, name))
    start = physical(cpu.cs, ENTRY); body_end = start + len(target["body"]); last = physical(cpu.cs, RETF_AT)
    returned, stopped, arrivals, previous, visited, edges = [False], [False], [0], [None], set(), set()

    def instruction(emulator, address, size, user):
        del size, user
        if returned[0] or stopped[0]:
            emulator.emu_stop(); return
        here = address - start + ENTRY
        check(start <= address < body_end, f"left the body: {label}")
        if here == LOOP_HEAD:
            arrivals[0] += 1
            if arrivals[0] == cap + 2:
                stopped[0] = True; emulator.emu_stop(); return
        visited.add(here)
        if previous[0] in target["conditional"]:
            edges.add((previous[0], here))
        previous[0] = here
        if address == last:
            returned[0] = True

    def mirror(emulator, access, address, size, value, user):
        del access, user
        for byte in range(size):
            raw = address + byte; at = raw & (SIZE - 1)
            emulator.mem_write(at + SIZE if raw < SIZE else at, bytes([(value >> (8 * byte)) & 0xFF]))

    original.hook_add(UC_HOOK_CODE, instruction)
    if high:
        original.hook_add(UC_HOOK_MEM_WRITE, mirror)
    original.emu_start(start, 2 * SIZE, count=budget)
    check(returned[0] or stopped[0], f"original neither returned nor reached the cap: {label}")
    expected = bytes(original.mem_read(0, SIZE))
    ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
    status = function(ram, SIZE, ctypes.byref(cpu), cap)
    check(status == (0 if returned[0] else 2), f"C status {status}: {label}")
    actual = bytes(ram)
    if actual != expected:
        differences = [i for i, (a, b) in enumerate(zip(actual, expected)) if a != b]
        raise ValueError(f"RAM mismatch {label}: {[(hex(d), actual[d], expected[d]) for d in differences[:6]]} ({len(differences)} bytes)")
    for (name, _), register in zip(Cpu._fields_, REGISTERS):
        got, want = getattr(cpu, name), original.reg_read(register) & 0xFFFF
        check(got == want, f"{name} mismatch {label}: C={got:04x}, original={want:04x}")
    if stopped[0]:
        return None, arrivals[0], ram
    return visited, edges, ram


def cases():
    spec = []
    rng = random.Random(0x2199)
    for i in range(240):
        n = rng.randrange(0, 9)
        hit = rng.choice([None, None] + list(range(n))) if n else None
        cycle = rng.randrange(n) if n and rng.random() < 0.25 and hit is not None else None
        spec.append((f"random-{i}", dict(nodes=n, hit=hit, cycle_to=cycle, flags=rng.choice((0x7202, 0x0202, 0x0ED7, 0x0647, 0x0002)))))
    for n in range(0, 7):
        spec.append((f"miss-{n}", dict(nodes=n)))
        for h in range(n):
            spec.append((f"hit-{n}-{h}", dict(nodes=n, hit=h, key_high=0xFF - h)))
    for cursor in ("head", "frame", "args", "node5"):
        for i in range(12):
            n = 1 + i % 5
            spec.append((f"{cursor}-{i}", dict(nodes=n, hit=(i % n if i % 3 else None), cursor=cursor)))
    spec.append(("frame-empty", dict(nodes=0, cursor="frame")))
    for sp in (0x0002, 0x0004, 0x0010, 0xFFF0, 0xFFF2, 0xFFF4, 0xFFF8, 0xFFFA, 0xFFFC, 0xFFFE):
        spec.append((f"sp-{sp:04x}", dict(nodes=3, hit=sp % 3, ss=0x1800, sp=sp, background=bytes(SIZE))))
        spec.append((f"sp-{sp:04x}-frame", dict(nodes=2, cursor="frame", ss=0x1800, sp=sp)))
    for i in range(6):
        spec.append((f"straddle-{i}", dict(nodes=2 + i % 3, hit=(None if i % 2 else 1 + i % 2), straddle=True)))
        spec.append((f"a20-{i}", dict(nodes=1 + i % 4, hit=(None if i % 2 else 0), a20=True)))
    spec.append(("ss-a20", dict(nodes=3, hit=2, ss=0xFFFF, sp=0x0400)))
    spec.append(("random-background", dict(nodes=4, hit=3, background=bytes(random.Random(9).randbytes(SIZE)))))
    spec.append(("preserved-df-if", dict(nodes=2, flags=0x0603)))
    for n in (1, 2, 3, 5):
        spec.append((f"cycle-miss-{n}", dict(nodes=n, cycle_to=0)))                    # endless in DOS
    spec.append(("cycle-miss-tail", dict(nodes=4, cycle_to=2)))
    return spec


def differential(target, image, function):
    rng = random.Random(0x2199C0DE)
    visited, edges, returning, limited, semantic = set(), set(), 0, [], 0
    for label, options in cases():
        memory, cpu, high, expect, cell = build(image, rng, **options)
        v, e, ram = compare(f"RES_2199 {label}", memory, cpu, high, target, function)
        if v is None:
            check(label.startswith("cycle-miss") or "cycle" in label or options.get("cursor") != "free",
                  f"unexpected LIMIT for {label}")
            limited.append(f"{label}@{e}")
            continue
        visited |= v; edges |= e; returning += 1
        if expect is not None:
            pointer = (struct.unpack_from("<H", bytes(ram), physical(cell[0], cell[1] + 2))[0],
                       struct.unpack_from("<H", bytes(ram), physical(cell[0], cell[1]))[0])
            check(cpu.ax & 0xFF == expect[0] and pointer == expect[1], f"{label}: AL {cpu.ax & 0xFF}, cursor {pointer}, documented {expect}")
            semantic += 1
    check(all(label.startswith("cycle-miss") for label in [x.split("@")[0] for x in limited if x.startswith("cycle")]), "cycle LIMIT set")
    check({x.split("@")[0] for x in limited} >= {"cycle-miss-1", "cycle-miss-2", "cycle-miss-3", "cycle-miss-5", "cycle-miss-tail"},
          f"endless cycles did not all stop: {limited}")
    missing_sites = sorted(set(i.address for i in target["decoded"]) - visited)
    missing_edges = sorted({(s, o) for s, outs in target["conditional"].items() for o in outs} - edges)
    check(not missing_sites and not missing_edges, f"coverage incomplete: {missing_sites} {missing_edges}")
    print(f"RES_00BA_2199: {returning} returning original-byte cases passed, full 1 MiB RAM and all 14 CPU words equal; "
          f"{semantic} with AL and the cursor cell checked against the documented lookup; "
          f"{len(visited)}/{len(target['decoded'])} instruction starts, {len(edges)}/6 conditional outcomes")
    print(f"RES_00BA_2199: {len(limited)} host-cap stops equal the original stopped at the same loop-head arrival: {', '.join(limited)}")
    return dict(returning=returning, semantic=semantic, limit_stops=len(limited), starts=len(visited), outcomes=len(edges))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--unicorn", action="store_true")
    parser.add_argument("--native", action="store_true")
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    target, image = source()
    report = dict(target=dict(sha256=target["sha"], bytes=len(target["body"]), instructions=len(target["decoded"])))
    if args.unicorn or args.native:
        with tempfile.TemporaryDirectory(prefix="poolrad-res2199-") as temp:
            function = library(Path(temp))
            if args.unicorn:
                report["unicorn"] = differential(target, image, function)
            if args.native:
                from resident_2199_cpu_native import native_audit
                report["native"] = native_audit(target, function)
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(json.dumps(report, sort_keys=True, indent=2) + "\n")
    print(json.dumps(report, sort_keys=True))


if __name__ == "__main__":
    main()
