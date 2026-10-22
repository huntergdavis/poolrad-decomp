#!/usr/bin/env python3
"""Original-byte full-state parity for raw-CPU RES_00BA_26C2 and u86 +0729 (entry 3), plus a DOSBox-X oracle.

The Unicorn differential places the hash-checked expanded START.EXE image at load
segment 0, so RES_00BA_26C2 runs from its original bytes at 00BA:26C2, both when
called directly and from +0729's unrelocated LCALL 00BA:26C2, and loads unit 0x86.
Every case compares the entire 1 MiB RAM and all 14 CPU words. The DOSBox-X oracle
relocates +0729's far-call segment word loader-style to a resident paragraph holding
the routine's 103 bytes. Media stays private.
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

RES_AT, RES_END, RES_IP = 0x3262, 0x32C9, 0x26C2
U86_ENTRY, U86_END = 0x0729, 0x07D6
CALLER_CS, DS, SS = 0x3800, 0x1800, 0x2800
RECORD = (0x6000, 0x0100)


def check(condition, message):
    if not condition:
        raise ValueError(message)


def source():
    exe = (ROOT / "resources/DOS/POOLRAD/START.EXE").read_bytes()
    ovr = (ROOT / "resources/DOS/POOLRAD/GAME.OVR").read_bytes()
    check(len(exe) == 65231 and sha256(exe).hexdigest() == EXE_SHA, "START.EXE provenance differs")
    check(len(ovr) == 206105 and sha256(ovr).hexdigest() == OVR_SHA, "GAME.OVR provenance differs")
    image = bytes(unpack(exe)[0])
    unit = {u["stub_para"]: u for u in find_units(exe[parse_mz(exe)["header_bytes"]:], ovr)}[0x86]
    code = ovr[unit["ovr_offset"]:unit["ovr_offset"] + unit["code_size"]]
    decoder = Cs(CS_ARCH_X86, CS_MODE_16)
    res = image[RES_AT:RES_END]
    res_decoded = list(decoder.disasm(res, RES_IP))
    check(res[:3] == b"\x55\x89\xe5" and len(res_decoded) == 38 and res_decoded[-1].mnemonic == "retf" and res_decoded[-1].op_str == "2"
          and sum(i.size for i in res_decoded) == len(res), "RES_00BA_26C2 extent differs")
    check(not any("call" in i.mnemonic for i in res_decoded), "RES_00BA_26C2 is not a leaf")
    starts = sorted(f["rel"] for f in unit["functions"])
    check(U86_END == next(s for s in starts if s > U86_ENTRY) and unit["entries"][3] == U86_ENTRY, "u86 +0729 bounds or entry slot differ")
    body = code[U86_ENTRY:U86_END]
    decoded = list(decoder.disasm(body, U86_ENTRY))
    calls = [(i.address, i.op_str) for i in decoded if "call" in i.mnemonic]
    check(len(decoded) == 69 and decoded[-1].mnemonic == "retf" and decoded[-1].op_str == "2" and calls == [(0x073A, "0xba, 0x26c2")]
          and (0x073A + 3) in set(unit["fixups"]), "u86 +0729 extent or call differs")
    targets = {}
    for key, b, dec, entry in (("26C2", res, res_decoded, RES_IP), ("0729", body, decoded, U86_ENTRY)):
        conditional = {i.address: (i.address + i.size, int(i.op_str, 16)) for i in dec if i.mnemonic.startswith("j") and i.mnemonic != "jmp"}
        targets[key] = dict(body=b, decoded=dec, conditional=conditional, sha=sha256(b).hexdigest(), entry=entry)
        print(f"{'RES_00BA_26C2' if key == '26C2' else 'u86 +0729 (entry 3)'}: {len(b)}/{len(dec)}, {len(conditional)} conditionals, sha {targets[key]['sha'][:16]}...")
    return targets, code, image, unit


CHECKER_ROWS = ('OVL_0086_0729',)


def library(directory):
    path = directory / "r26c2.so"
    sources = ["resident_00ba_class_value_26c2_cpu.c", "overlay_u86_entry3_0729.c"]
    subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-O2", "-fPIC", "-shared",
                    "-I", str(ROOT / "include"), *[str(ROOT / "src/recovered" / s) for s in sources],
                    "-o", str(path)], check=True, capture_output=True, text=True)
    lib = ctypes.CDLL(str(path))
    functions = {}
    for key, name in (("26C2", "pr_res_00ba_26c2_cpu"), ("0729", "pr_u86_entry3_0729")):
        f = getattr(lib, name)
        f.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t, ctypes.POINTER(Cpu)]
        f.restype = ctypes.c_int
        functions[key] = f
    return functions


def world(rng, key, *, k=None, cls=None, flag=None, t2=None, t3=None, t6=None, sp=0x8000, ss=SS, ds=DS, flags=0x7202,
          regs=None, background=None):
    """Data the two routines read, as (segment, offset) -> byte, plus the argument word."""
    data = {}
    k = rng.randrange(256) if k is None else k
    for i in range(0x1010):                                         # the whole 16-byte-entry table at DS:31FA
        data[(ds, 0x31FA + i)] = rng.randrange(256)
    entry = 0x31FA + 16 * k
    data[(ds, entry)] = rng.choice((0, 1, 2, 3, 0xFF)) if cls is None else cls
    for offset, value in ((2, t2), (3, t3), (6, t6)):
        if value is not None:
            data[(ds, entry + offset)] = value
    data[(ds, 0x6DBF)] = rng.choice((0, 0, 1, 0x80)) if flag is None else flag
    put = {(ds, 0x5D92): RECORD[1], (ds, 0x5D94): RECORD[0]}
    for field in (0x96, 0x9B):
        data[(RECORD[0], RECORD[1] + field)] = rng.randrange(256)
    return data, put, (rng.randrange(256) << 8) | k


def fixture(code, image, data, words, *, entry_cs, entry_ip, sp, ss, ds, flags, regs, background):
    memory = bytearray([0xA5]) * SIZE if background is None else bytearray(background)
    memory[0:len(image)] = image
    at = physical(CALLER_CS, 0); memory[at:at + len(code)] = code
    (bytes_, words_map) = data
    for (seg, off), value in bytes_.items():
        memory[physical(seg, off)] = value
    for (seg, off), value in words_map.items():
        put16(memory, seg, off, value)
    frame = physical(ss, sp)
    for i, value in enumerate(words):
        put16(memory, ss, (sp + 2 * i) & 0xFFFF, value)
        memory[(frame + 2 * i) & (SIZE - 1)] = value & 0xFF
        memory[(frame + 2 * i + 1) & (SIZE - 1)] = value >> 8
    return memory, Cpu(*regs, sp, entry_cs, entry_ip, ds, 0x1111, ss, flags), ss >= 0xF000 or ds >= 0xF000


def compare(label, memory, cpu, high, target, function, budget=100000):
    from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
    original = Uc(UC_ARCH_X86, UC_MODE_16)
    original.mem_map(0, 2 * SIZE)
    original.mem_write(0, bytes(memory)); original.mem_write(SIZE, bytes(memory))
    for (name, _), register in zip(Cpu._fields_, REGISTERS):
        original.reg_write(register, getattr(cpu, name))
    start = physical(cpu.cs, target["entry"]); body_end = start + len(target["body"]); ret = physical(0x7001, 0x7000)
    previous, visited, edges, returned = [None], set(), set(), [False]

    def instruction(emulator, address, size, user):
        del emulator, size, user
        here = address - start + target["entry"]
        visited.add(here)
        if previous[0] in target["conditional"]:
            edges.add((previous[0], here))
        previous[0] = here

    def outside(emulator, address, size, user):
        del size, user
        previous[0] = None
        if address == ret:
            returned[0] = True; emulator.emu_stop()

    def mirror(emulator, access, address, size, value, user):
        del access, user
        for byte in range(size):
            raw = address + byte; at = raw & (SIZE - 1)
            emulator.mem_write(at + SIZE if raw < SIZE else at, bytes([(value >> (8 * byte)) & 0xFF]))

    original.hook_add(UC_HOOK_CODE, instruction, begin=start, end=body_end - 1)
    original.hook_add(UC_HOOK_CODE, outside, begin=0, end=start - 1)
    original.hook_add(UC_HOOK_CODE, outside, begin=body_end, end=2 * SIZE - 1)
    if high:
        original.hook_add(UC_HOOK_MEM_WRITE, mirror)
    original.emu_start(start, 2 * SIZE, count=budget)
    check(returned[0], f"original did not return: {label}")
    expected = bytes(original.mem_read(0, SIZE))
    ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
    status = function(ram, SIZE, ctypes.byref(cpu))
    check(status == 0, f"C status {status}: {label}")
    actual = bytes(ram)
    if actual != expected:
        differences = [i for i, (a, b) in enumerate(zip(actual, expected)) if a != b]
        raise ValueError(f"RAM mismatch {label}: {[(hex(d), actual[d], expected[d]) for d in differences[:6]]} ({len(differences)} bytes)")
    for (name, _), register in zip(Cpu._fields_, REGISTERS):
        got, want = getattr(cpu, name), original.reg_read(register) & 0xFFFF
        check(got == want, f"{name} mismatch {label}: C={got:04x}, original={want:04x}")
    return visited, edges


def cases(key):
    rng = random.Random(0x26C2 if key == "26C2" else 0x0729)
    spec = []
    for k in range(256):
        spec.append((f"k-{k:02x}", dict(k=k, cls=(0, 1, 2, 3, 0xFF)[k % 5], flag=(0, 1)[(k >> 3) & 1])))
    for i in range(96):
        spec.append((f"random-{i}", dict(flags=(0x7202, 0x0202, 0x0ED7, 0x0647)[i % 4])))
    if key == "0729":
        for i in range(40):                                         # aim [BP-2] at 0 and FFh on both paths
            flag = i % 2; target = (0, 0xFF)[(i >> 1) % 2]; t3 = rng.randrange(256)
            t2 = (target - 6 * t3) & 0xFF if flag else None
            spec.append((f"edge-{i}", dict(flag=flag, t3=t3 if flag else 0, t2=t2 if flag else target, t6=(0, 5)[(i >> 2) % 2], cls=3)))
    for sp in (0x0002, 0x0004, 0x0010, 0xFFF0, 0xFFF4, 0xFFF8, 0xFFFC, 0xFFFE):
        spec.append((f"sp-{sp:04x}", dict(sp=sp, ss=0x1C00, background=bytes(SIZE))))
    spec.append(("ss-a20", dict(ss=0xFFFF, sp=0x0400)))
    spec.append(("random-background", dict(background=bytes(random.Random(9).randbytes(SIZE)))))
    spec.append(("preserved-df-if", dict(flags=0x0603)))
    return spec


def differential(targets, code, image, functions):
    report = {}
    for key, target in targets.items():
        rng = random.Random(0x26C20729 + int(key, 16))
        visited, edges, count = set(), set(), 0
        for label, options in cases(key):
            opts = dict(options)
            sp, ss, flags, background = opts.pop("sp", 0x8000), opts.pop("ss", SS), opts.pop("flags", 0x7202), opts.pop("background", None)
            regs = tuple(rng.randrange(0x10000) for _ in range(7))
            data = world(rng, key, **opts)
            entry_cs = 0x00BA if key == "26C2" else CALLER_CS
            memory, cpu, high = fixture(code, image, data[:2], [0x7000, 0x7001, data[2]], entry_cs=entry_cs, entry_ip=target["entry"],
                                        sp=sp, ss=ss, ds=DS, flags=flags, regs=regs, background=background)
            v, e = compare(f"{key} {label}", memory, cpu, high, target, functions[key])
            visited |= v; edges |= e; count += 1
        missing_sites = sorted(hex(s) for s in set(i.address for i in target["decoded"]) - visited)
        missing_edges = sorted((hex(s), hex(o)) for s, outs in target["conditional"].items() for o in outs if (s, o) not in edges)
        check(not missing_sites and not missing_edges, f"{key} coverage incomplete: {missing_sites} {missing_edges}")
        print(f"{key}: {count} original-byte cases passed, full 1 MiB RAM and all 14 CPU words equal; "
              f"{len(visited)}/{len(target['decoded'])} instruction starts, {len(edges)}/{2 * len(target['conditional'])} conditional outcomes")
        report[key] = dict(returning=count, starts=len(visited), outcomes=len(edges), sha256=target["sha"], bytes=len(target["body"]),
                           instructions=len(target["decoded"]))
    return report


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
    targets, code, image, unit = source()
    report = dict(targets={k: dict(sha256=t["sha"], bytes=len(t["body"]), instructions=len(t["decoded"])) for k, t in targets.items()})
    if args.unicorn or args.native:
        with tempfile.TemporaryDirectory(prefix="poolrad-r26c2-") as temp:
            functions = library(Path(temp))
            if args.unicorn:
                report["unicorn"] = differential(targets, code, image, functions)
            if args.native:
                from res26c2_u86_0729_native import native_audit
                report["native"] = native_audit(targets, code, image, functions)
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(json.dumps(report, sort_keys=True, indent=2) + "\n")
    print(json.dumps(report, sort_keys=True))


if __name__ == "__main__":
    main()
