#!/usr/bin/env python3
"""Original-byte full-state parity for u38 +2915 and entries 89/90/91 (+2976/+2984/+2992), raw CPU.

The bodies compose RES_00BA_26C2, the U9E entry-8 dice chain (through the loaded stub
00B0:0048) and the U38 +0000 helper. The differential places the hash-checked
expanded START.EXE image at load segment 0, so SYSTEM and RES_00BA_26C2 run from
their original bytes, patches the stub to its loaded form, loads units 0x9E and 0x38,
and compares the entire 1 MiB RAM and all 14 CPU words. Media stays private.
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

U9E_SHA = "9a2681e8b24a9a95838a11f2d7cbddea0c253d29e631db37d25fae017a14693b"
TARGETS = {
    0x2915: dict(end=0x2976, insns=42, pops=2, conditionals=3, slot=None, symbol="pr_u38_roll_gate_2915_cpu",
                 calls=[(0x291F, "lcall", "0xba, 0x26c2"), (0x295F, "lcall", "0xb0, 0x48"), (0x296D, "call", "0")], words=1),
    0x2976: dict(end=0x2984, insns=8, pops=0x0A, conditionals=0, slot=89, symbol="pr_u38_entry89_2976_cpu",
                 calls=[(0x297D, "call", "0x2915")], words=5),
    0x2984: dict(end=0x2992, insns=8, pops=0x0A, conditionals=0, slot=90, symbol="pr_u38_entry90_2984_cpu",
                 calls=[(0x298B, "call", "0x2915")], words=5),
    0x2992: dict(end=0x29B6, insns=19, pops=0x0A, conditionals=1, slot=91, symbol="pr_u38_entry91_2992_cpu",
                 calls=[(0x299B, "lcall", "0xb0, 0x48"), (0x29A8, "call", "0"), (0x29AF, "call", "0")], words=5),
}
CALLER_CS, DICE_CS, DS, SS = 0x3800, 0x4800, 0x1800, 0x2800
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
    units = {u["stub_para"]: u for u in find_units(exe[parse_mz(exe)["header_bytes"]:], ovr)}
    u38, u9e = units[0x38], units[0x9E]
    code38 = ovr[u38["ovr_offset"]:u38["ovr_offset"] + u38["code_size"]]
    code9e = ovr[u9e["ovr_offset"]:u9e["ovr_offset"] + u9e["code_size"]]
    check(sha256(code9e[0x0DE3:0x0E52]).hexdigest() == U9E_SHA and u9e["entries"][8] == 0x0DE3, "unit 0x9E entry 8 differs")
    check(sha256(image[0x3262:0x32C9]).hexdigest() == "5e191d3962386c67108f916bc38655e0a2231e352e215aea8ca1983d48f389c9",
          "RES_00BA_26C2 differs")
    check(sha256(code38[0:0x1F]).hexdigest() == "f62ad6b7474059693853b623f564cfd5c9298cd50e4628732b49b97eb6265240", "u38 helper differs")
    starts = sorted(f["rel"] for f in u38["functions"])
    fixups = set(u38["fixups"])
    decoder = Cs(CS_ARCH_X86, CS_MODE_16)
    targets = {}
    for entry, spec in TARGETS.items():
        check(spec["end"] == next(s for s in starts if s > entry), f"+{entry:04X} bounds differ")
        body = code38[entry:spec["end"]]
        decoded = list(decoder.disasm(body, entry))
        calls = [(i.address, i.mnemonic, i.op_str) for i in decoded if "call" in i.mnemonic]
        check(len(decoded) == spec["insns"] and decoded[-1].mnemonic == "retf" and int(decoded[-1].op_str, 16) == spec["pops"]
              and calls == spec["calls"], f"+{entry:04X} extent or calls differ: {calls}")
        check(all(a + 3 in fixups for a, m, _ in calls if m == "lcall"), f"+{entry:04X} far call not fixup-backed")
        check(spec["slot"] is None and entry not in u38["entries"] or u38["entries"][spec["slot"]] == entry, f"+{entry:04X} entry slot differs")
        conditional = {i.address: (i.address + i.size, int(i.op_str, 16)) for i in decoded if i.mnemonic.startswith("j") and i.mnemonic != "jmp"}
        check(len(conditional) == spec["conditionals"], f"+{entry:04X} conditional count differs")
        targets[entry] = dict(spec, entry=entry, body=body, decoded=decoded, conditional=conditional, sha=sha256(body).hexdigest())
        print(f"u38 +{entry:04X} ({len(body)}/{len(decoded)}, RETF {spec['pops']:X}h) sha {targets[entry]['sha'][:16]}...")
    return targets, code38, code9e, image


def library(directory):
    path = directory / "u38r.so"
    sources = ["overlay_u38_roll_gate_2915_cpu.c", "overlay_u38_helper_cluster_cpu.c", "resident_00ba_class_value_26c2_cpu.c",
               "overlay_u9e_entry8_fullstate_compose_0de3.c", "overlay_u9e_entry8_fullstate_prefix_0de3.c",
               "overlay_u9e_entry8_fullstate_loop_0e05.c", "overlay_u9e_entry8_fullstate_postcall_0e13.c",
               "overlay_u9e_entry8_fullstate_tail_0e1f.c", "overlay_u9e_entry8_system_call_0e0e.c",
               "system_random_fullstate_11c0.c", "system_seed_fullstate_120f.c"]
    subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-O2", "-fPIC", "-shared",
                    "-I", str(ROOT / "include"), *[str(ROOT / "src/recovered" / s) for s in sources],
                    "-o", str(path)], check=True, capture_output=True, text=True)
    lib = ctypes.CDLL(str(path))
    functions = {}
    for entry, spec in TARGETS.items():
        f = getattr(lib, spec["symbol"])
        f.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t, ctypes.POINTER(Cpu), ctypes.c_uint, ctypes.POINTER(ctypes.c_int)]
        f.restype = ctypes.c_int
        functions[entry] = f
    return functions


def build(code38, code9e, image, rng, entry, *, key=None, b6817=None, b6819=None, k=None, cls=None, flag=None, seed=None,
          sp=0x8000, ss=SS, flags=0x7202, regs=None, background=None):
    memory = bytearray([0xA5]) * SIZE if background is None else bytearray(background)
    memory[0:len(image)] = image
    memory[physical(0x00B0, 0x48):physical(0x00B0, 0x48) + 5] = struct.pack("<BHH", 0xEA, 0x0DE3, DICE_CS)
    at = physical(DICE_CS, 0); memory[at:at + len(code9e)] = code9e
    at = physical(CALLER_CS, 0); memory[at:at + len(code38)] = code38
    k = rng.randrange(256) if k is None else k
    for i in range(0x1010):
        memory[physical(DS, 0x31FA + i)] = rng.randrange(256)
    memory[physical(DS, 0x31FA + 16 * k)] = rng.choice((0, 1, 2, 3)) if cls is None else cls
    memory[physical(DS, 0x681B)] = k
    memory[physical(DS, 0x6817)] = rng.choice((0, 0, 1, 0x34, 0x35, 0x0B)) if b6817 is None else b6817
    memory[physical(DS, 0x6818)] = rng.randrange(256)
    memory[physical(DS, 0x6819)] = rng.randrange(256) if b6819 is None else b6819
    memory[physical(DS, 0x6DBF)] = rng.choice((0, 0, 1)) if flag is None else flag
    s = rng.randrange(1 << 32) if seed is None else seed
    put16(memory, DS, 0x4428, s & 0xFFFF); put16(memory, DS, 0x442A, s >> 16)
    put16(memory, DS, 0x5D92, RECORD[1]); put16(memory, DS, 0x5D94, RECORD[0])
    for field in (0x96, 0x9B):
        memory[physical(RECORD[0], RECORD[1] + field)] = rng.randrange(256)
    words = [0x7000, 0x7001]
    if TARGETS[entry]["words"] == 1:
        words.append((rng.randrange(256) << 8) | (rng.randrange(256) if key is None else key))
    else:
        words += [rng.randrange(0x10000) for _ in range(5)]
    frame = physical(ss, sp)
    for i, value in enumerate(words):
        put16(memory, ss, (sp + 2 * i) & 0xFFFF, value)
        if i < 2:
            memory[(frame + 2 * i) & (SIZE - 1)] = value & 0xFF
            memory[(frame + 2 * i + 1) & (SIZE - 1)] = value >> 8
    reg = regs or tuple(rng.randrange(0x10000) for _ in range(7))
    return memory, Cpu(*reg, sp, CALLER_CS, entry, DS, rng.randrange(0x10000), ss, flags), ss >= 0xF000


def compare(label, memory, cpu, high, target, function, budget=3000000):
    from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
    original = Uc(UC_ARCH_X86, UC_MODE_16)
    original.mem_map(0, 2 * SIZE)
    original.mem_write(0, bytes(memory)); original.mem_write(SIZE, bytes(memory))
    for (name, _), register in zip(Cpu._fields_, REGISTERS):
        original.reg_write(register, getattr(cpu, name))
    base = physical(cpu.cs, 0)
    start, body_end, ret = base + target["entry"], base + target["end"], physical(0x7001, 0x7000)
    previous, visited, edges, returned = [None], set(), set(), [False]

    def instruction(emulator, address, size, user):
        del emulator, size, user
        here = address - base
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
        check(got == want, f"{name} mismatch {label}: C={got:04x}, original={want:04x}")
    return visited, edges


def cases(entry):
    spec = []
    for i in range(160):
        spec.append((f"random-{i}", dict(flags=(0x7202, 0x0202, 0x0ED7, 0x0647)[i % 4])))
    for i in range(48):                                             # both gate tests and both roll outcomes
        spec.append((f"gate-{i}", dict(b6817=(0, 0, 0x34)[i % 3], b6819=(0x00, 0x08, 0xF7, 0xFF)[(i // 3) % 4], cls=(0, 1, 2, 3)[i % 4],
                                     key=(0, 0x32, 0x0F, 0x80, 0xFF)[i % 5])))
    for sp in (0x0040, 0x0100, 0xFFC0, 0xFFE8):
        spec.append((f"sp-{sp:04x}", dict(sp=sp, b6817=1)))
    spec.append(("random-background", dict(b6817=1, background=bytes(random.Random(9).randbytes(SIZE)))))
    spec.append(("preserved-df-if", dict(b6817=1, flags=0x0603)))
    return spec


def differential(targets, code38, code9e, image, functions):
    report = {}
    for entry, target in targets.items():
        rng = random.Random(0x382915 + entry)
        visited, edges, count = set(), set(), 0
        for label, options in cases(entry):
            memory, cpu, high = build(code38, code9e, image, rng, entry, **options)
            v, e = compare(f"+{entry:04X} {label}", memory, cpu, high, target, functions[entry])
            visited |= v; edges |= e; count += 1
        missing_sites = sorted(hex(s) for s in set(i.address for i in target["decoded"]) - visited)
        missing_edges = sorted((hex(s), hex(o)) for s, outs in target["conditional"].items() for o in outs if (s, o) not in edges)
        check(not missing_sites and not missing_edges, f"+{entry:04X} coverage incomplete: {missing_sites} {missing_edges}")
        print(f"+{entry:04X}: {count} original-byte cases passed, full 1 MiB RAM and all 14 CPU words equal; "
              f"{len(set(i.address for i in target['decoded']) & visited)}/{len(target['decoded'])} instruction starts, "
              f"{len(edges)}/{2 * len(target['conditional'])} conditional outcomes")
        report[f"{entry:04X}"] = dict(returning=count, starts=len(target["decoded"]), outcomes=len(edges), sha256=target["sha"],
                                      bytes=len(target["body"]), instructions=len(target["decoded"]))
    return report


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--unicorn", action="store_true")
    parser.add_argument("--native", action="store_true")
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    targets, code38, code9e, image = source()
    report = dict(targets={f"{e:04X}": dict(sha256=t["sha"], bytes=len(t["body"]), instructions=len(t["decoded"])) for e, t in targets.items()})
    if args.unicorn or args.native:
        with tempfile.TemporaryDirectory(prefix="poolrad-u38r-") as temp:
            functions = library(Path(temp))
            if args.unicorn:
                report["unicorn"] = differential(targets, code38, code9e, image, functions)
            if args.native:
                from u38_roll_gate_2915_native import native_audit
                report["native"] = native_audit(targets, code38, code9e, image, functions)
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(json.dumps(report, sort_keys=True, indent=2) + "\n")
    print(json.dumps(report, sort_keys=True))


if __name__ == "__main__":
    main()
