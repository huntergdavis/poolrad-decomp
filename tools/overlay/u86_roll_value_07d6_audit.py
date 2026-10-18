#!/usr/bin/env python3
"""Original-byte full-state parity for u86 +07D6 with the U9E dice chain and RES_00BA_26C2 composed.

The differential places the hash-checked expanded START.EXE image at load segment 0,
so the original SYSTEM generator at 0AF8:11C0 and RES_00BA_26C2 at 00BA:26C2 run
live. It patches the overlay stub 00B0:0048 to its loaded form (EA to unit 0x9E
+0DE3), loads unit 0x9E and unit 0x86, and compares the entire 1 MiB RAM and all
14 CPU words between the original under Unicorn and the composed C model.
Media stays private.
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

ENTRY, END = 0x07D6, 0x08C3
SITES = [0x07E9, 0x0809, 0x0820, 0x0846, 0x085D]
U9E_SHA = "9a2681e8b24a9a95838a11f2d7cbddea0c253d29e631db37d25fae017a14693b"
SYSTEM_FILE = (0xBE36, 0xBEBD)
CALLER_CS, DICE_CS, DS, SS = 0x3800, 0x4800, 0x1800, 0x2800
RECORD = (0x6000, 0x0100)
MUL_FINAL = {0x28, 0x3F}          # MUL's flags reach the RETF: SF, PF and AF are architecturally undefined
UNDEFINED_AFTER_MUL = 0x0094


def check(condition, message):
    if not condition:
        raise ValueError(message)


def source():
    exe = (ROOT / "resources/DOS/POOLRAD/START.EXE").read_bytes()
    ovr = (ROOT / "resources/DOS/POOLRAD/GAME.OVR").read_bytes()
    check(len(exe) == 65231 and sha256(exe).hexdigest() == EXE_SHA, "START.EXE provenance differs")
    check(len(ovr) == 206105 and sha256(ovr).hexdigest() == OVR_SHA, "GAME.OVR provenance differs")
    image = bytes(unpack(exe)[0])
    check(image[0xAF80 + 0x11C0:0xAF80 + 0x11C0 + (SYSTEM_FILE[1] - SYSTEM_FILE[0])] == exe[SYSTEM_FILE[0]:SYSTEM_FILE[1]],
          "expanded SYSTEM 0AF8:11C0..1246 differs from the START.EXE bytes the merged dice chain pins")
    units = {u["stub_para"]: u for u in find_units(exe[parse_mz(exe)["header_bytes"]:], ovr)}
    u86, u9e = units[0x86], units[0x9E]
    check(u9e["memory_segment"] == 0x00B0 and u9e["entries"][8] == 0x0DE3, "unit 0x9E entry 8 differs")
    code86 = ovr[u86["ovr_offset"]:u86["ovr_offset"] + u86["code_size"]]
    code9e = ovr[u9e["ovr_offset"]:u9e["ovr_offset"] + u9e["code_size"]]
    check(sha256(code9e[0x0DE3:0x0E52]).hexdigest() == U9E_SHA, "unit 0x9E +0DE3..+0E51 differs")
    starts = sorted(f["rel"] for f in u86["functions"])
    check(END == next(s for s in starts if s > ENTRY), "u86 +07D6 bounds differ")
    body = code86[ENTRY:END]
    decoded = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(body, ENTRY))
    calls = [(i.address, i.op_str) for i in decoded if "call" in i.mnemonic]
    check(len(decoded) == 99 and len(body) == 237 and decoded[-1].mnemonic == "retf" and decoded[-1].op_str == "2"
          and calls == [(s, "0xb0, 0x48") for s in SITES] + [(0x0880, "0xba, 0x26c2")], f"u86 +07D6 extent or calls differ: {calls}")
    fixups = set(u86["fixups"])
    check(all(s + 3 in fixups for s, _ in calls), "a far-call segment word is not fixup-backed")
    conditional = {i.address: (i.address + i.size, int(i.op_str, 16)) for i in decoded if i.mnemonic.startswith("j") and i.mnemonic != "jmp"}
    check(len(conditional) == 7, "u86 +07D6 conditional count differs")
    target = dict(body=body, decoded=decoded, conditional=conditional, sha=sha256(body).hexdigest())
    print(f"u86 +07D6 ({len(body)}/{len(decoded)}, 5 x LCALL 00B0:0048 to u9E +0DE3, LCALL 00BA:26C2) sha {target['sha'][:16]}...")
    return target, code86, code9e, image


def library(directory):
    path = directory / "u86.so"
    sources = ["overlay_u86_roll_value_07d6.c", "resident_00ba_class_value_26c2_cpu.c",
               "overlay_u9e_entry8_fullstate_compose_0de3.c", "overlay_u9e_entry8_fullstate_prefix_0de3.c",
               "overlay_u9e_entry8_fullstate_loop_0e05.c", "overlay_u9e_entry8_fullstate_postcall_0e13.c",
               "overlay_u9e_entry8_fullstate_tail_0e1f.c", "overlay_u9e_entry8_system_call_0e0e.c",
               "system_random_fullstate_11c0.c", "system_seed_fullstate_120f.c"]
    subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-O2", "-fPIC", "-shared",
                    "-I", str(ROOT / "include"), *[str(ROOT / "src/recovered" / s) for s in sources],
                    "-o", str(path)], check=True, capture_output=True, text=True)
    f = ctypes.CDLL(str(path)).pr_u86_roll_value_07d6
    f.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t, ctypes.POINTER(Cpu), ctypes.c_uint, ctypes.POINTER(ctypes.c_int)]
    f.restype = ctypes.c_int
    return f


def build(code86, code9e, image, rng, *, k=None, mode=None, cls=None, flag=None, seed=None, sp=0x8000, ss=SS, flags=0x7202,
          regs=None, background=None):
    memory = bytearray([0xA5]) * SIZE if background is None else bytearray(background)
    memory[0:len(image)] = image
    memory[physical(0x00B0, 0x48):physical(0x00B0, 0x48) + 5] = struct.pack("<BHH", 0xEA, 0x0DE3, DICE_CS)
    at = physical(DICE_CS, 0); memory[at:at + len(code9e)] = code9e
    at = physical(CALLER_CS, 0); memory[at:at + len(code86)] = code86
    k = rng.choice((0x28, 0x39, 0x3B, 0x3D, 0x3F, 0x43, rng.randrange(256))) if k is None else k
    for i in range(0x1010):
        memory[physical(DS, 0x31FA + i)] = rng.randrange(256)
    memory[physical(DS, 0x31FA + 16 * k)] = rng.choice((0, 1, 2, 3)) if cls is None else cls
    memory[physical(DS, 0x49F3)] = rng.choice((5, 5, 0, 1, 4, 6)) if mode is None else mode
    memory[physical(DS, 0x6DBF)] = rng.choice((0, 0, 1)) if flag is None else flag
    s = rng.randrange(1 << 32) if seed is None else seed
    put16(memory, DS, 0x4428, s & 0xFFFF); put16(memory, DS, 0x442A, s >> 16)
    put16(memory, DS, 0x5D92, RECORD[1]); put16(memory, DS, 0x5D94, RECORD[0])
    words = [0x7000, 0x7001, (rng.randrange(256) << 8) | k]
    frame = physical(ss, sp)
    for i, value in enumerate(words):
        put16(memory, ss, (sp + 2 * i) & 0xFFFF, value)
        memory[(frame + 2 * i) & (SIZE - 1)] = value & 0xFF
        memory[(frame + 2 * i + 1) & (SIZE - 1)] = value >> 8
    reg = regs or tuple(rng.randrange(0x10000) for _ in range(7))
    return memory, Cpu(*reg, sp, CALLER_CS, ENTRY, DS, rng.randrange(0x10000), ss, flags), k, ss >= 0xF000


def compare(label, memory, cpu, high, target, function, mask=0, budget=3000000):
    from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
    original = Uc(UC_ARCH_X86, UC_MODE_16)
    original.mem_map(0, 2 * SIZE)
    original.mem_write(0, bytes(memory)); original.mem_write(SIZE, bytes(memory))
    for (name, _), register in zip(Cpu._fields_, REGISTERS):
        original.reg_write(register, getattr(cpu, name))
    start = physical(cpu.cs, ENTRY); body_end = start + len(target["body"]); ret = physical(0x7001, 0x7000)
    previous, visited, edges, returned = [None], set(), set(), [False]

    def instruction(emulator, address, size, user):
        del emulator, size, user
        here = address - start + ENTRY
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
    callee = ctypes.c_int(0)
    status = function(ram, SIZE, ctypes.byref(cpu), 256, ctypes.byref(callee))
    check(status == 0, f"C status {status} (callee {callee.value}): {label}")
    actual = bytes(ram)
    if actual != expected:
        differences = [i for i, (a, b) in enumerate(zip(actual, expected)) if a != b]
        raise ValueError(f"RAM mismatch {label}: {[(hex(d), actual[d], expected[d]) for d in differences[:6]]} ({len(differences)} bytes)")
    for (name, _), register in zip(Cpu._fields_, REGISTERS):
        got, want = getattr(cpu, name), original.reg_read(register) & 0xFFFF
        if name == "flags":
            got, want = got & ~mask, want & ~mask
        check(got == want, f"{name} mismatch {label}: C={got:04x}, original={want:04x}")
    return visited, edges


def cases():
    spec = []
    for k in (0x28, 0x39, 0x3B, 0x3D, 0x3F, 0x43):
        for i in range(24):
            spec.append((f"k{k:02x}-{i}", dict(k=k, mode=(5, 0, 6)[i % 3] if k == 0x3F else None)))
    for k in range(256):
        if k not in (0x28, 0x39, 0x3B, 0x3D, 0x3F, 0x43):
            spec.append((f"table-{k:02x}", dict(k=k, cls=(0, 1, 2, 3)[k % 4], flag=(0, 1)[(k >> 2) % 2])))
    for i in range(80):
        spec.append((f"random-{i}", dict(flags=(0x7202, 0x0202, 0x0ED7, 0x0647)[i % 4])))
    for sp in (0x0040, 0x0100, 0xFFC0, 0xFFE8):
        spec.append((f"sp-{sp:04x}", dict(sp=sp, k=(0x28, 0x3F, 0x77, 0x39)[sp % 4])))
    spec.append(("random-background", dict(k=0x3B, background=bytes(random.Random(9).randbytes(SIZE)))))
    spec.append(("preserved-df-if", dict(k=0x28, flags=0x0603)))
    return spec


def differential(target, code86, code9e, image, function):
    rng = random.Random(0x8607D6)
    visited, edges, count, masked = set(), set(), 0, 0
    for label, options in cases():
        memory, cpu, k, high = build(code86, code9e, image, rng, **options)
        mask = UNDEFINED_AFTER_MUL if k in MUL_FINAL else 0
        v, e = compare(f"+07D6 {label}", memory, cpu, high, target, function, mask)
        visited |= v; edges |= e; count += 1; masked += bool(mask)
    missing_sites = sorted(hex(s) for s in set(i.address for i in target["decoded"]) - visited)
    missing_edges = sorted((hex(s), hex(o)) for s, outs in target["conditional"].items() for o in outs if (s, o) not in edges)
    check(not missing_sites and not missing_edges, f"+07D6 coverage incomplete: {missing_sites} {missing_edges}")
    print(f"+07D6: {count} original-byte cases passed, full 1 MiB RAM and all 14 CPU words equal; {masked} MUL-final cases mask only "
          f"the undefined SF/PF/AF (DOSBox-X profile, checked unmasked natively); "
          f"{len(visited)}/{len(target['decoded'])} instruction starts, {len(edges)}/{2 * len(target['conditional'])} conditional outcomes")
    return dict(returning=count, mul_final_masked=masked, starts=len(visited), outcomes=len(edges))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--unicorn", action="store_true")
    parser.add_argument("--native", action="store_true")
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    target, code86, code9e, image = source()
    report = dict(target=dict(sha256=target["sha"], bytes=len(target["body"]), instructions=len(target["decoded"])))
    if args.unicorn or args.native:
        with tempfile.TemporaryDirectory(prefix="poolrad-u86-07d6-") as temp:
            function = library(Path(temp))
            if args.unicorn:
                report["unicorn"] = differential(target, code86, code9e, image, function)
            if args.native:
                from u86_roll_value_07d6_native import native_audit
                report["native"] = native_audit(target, code86, code9e, image, function)
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(json.dumps(report, sort_keys=True, indent=2) + "\n")
    print(json.dumps(report, sort_keys=True))


if __name__ == "__main__":
    main()
