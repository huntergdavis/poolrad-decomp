#!/usr/bin/env python3
"""Original-byte full-state parity for u3B4 +00C7, +00FB and +012F, plus a DOSBox-X synthetic oracle.

The Unicorn differential loads the original unit 0x3B4 code into one 1 MiB image,
runs the original and the C model on a copy, and compares the entire RAM and all
14 CPU words for every argument byte of each function, with random registers,
flags and backgrounds, stacks at both segment ends and across A20. The DOSBox-X
oracle runs the three original routines for all 768 function/argument pairs.
Media stays private.
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

UNIT_BASE, UNIT_SIZE = 0x2DB87, 0x2A5
TARGETS = {
    0x00C7: dict(end=0x00FB, insns=21, retf=0x00F8, conditionals=3, caller=(0x016D, 0x01D9), symbol="pr_u3b4_case_map_00c7",
                 table=((0x08, 0x00), (0x41, 0x00), (0xE8, 0x0F)), default=0x0F),
    0x00FB: dict(end=0x012F, insns=21, retf=0x012C, conditionals=3, caller=(0x016D, 0x01E5), symbol="pr_u3b4_case_map_00fb",
                 table=((0x08, 0x08), (0x41, 0x07), (0xE8, 0x07)), default=0x07),
    0x012F: dict(end=0x016D, insns=25, retf=0x016A, conditionals=4, caller=(0x016D, 0x01F1), symbol="pr_u3b4_case_map_012f",
                 table=((0x0B, 0x08), (0x09, 0x06), (0xDB, 0x06), (0x06, 0x01)), default=0x06),
}
CS, DS, SS = 0x3000, 0x1000, 0x2000


def check(condition, message):
    if not condition:
        raise ValueError(message)


def source():
    exe = (ROOT / "resources/DOS/POOLRAD/START.EXE").read_bytes()
    ovr = (ROOT / "resources/DOS/POOLRAD/GAME.OVR").read_bytes()
    check(len(exe) == 65231 and sha256(exe).hexdigest() == EXE_SHA, "START.EXE provenance differs")
    check(len(ovr) == 206105 and sha256(ovr).hexdigest() == OVR_SHA, "GAME.OVR provenance differs")
    units = {u["stub_para"]: u for u in find_units(exe[parse_mz(exe)["header_bytes"]:], ovr)}
    unit = units[0x3B4]
    check(unit["ovr_offset"] == UNIT_BASE and unit["code_size"] == UNIT_SIZE, "unit 0x3B4 placement differs")
    code = ovr[UNIT_BASE:UNIT_BASE + UNIT_SIZE]
    starts = sorted(f["rel"] for f in unit["functions"])
    decoder = Cs(CS_ARCH_X86, CS_MODE_16)
    near = {}
    for function in starts:
        end = next((x for x in starts if x > function), len(code))
        aligned = {i.address: i for i in decoder.disasm(code[function:end], function)}
        for site, ins in aligned.items():
            if ins.mnemonic == "call" and ins.op_str.startswith("0x"):
                check(aligned[site - 1].op_str == "cs", f"+{site:04X} lacks PUSH CS")
                near.setdefault(int(ins.op_str, 16), set()).add((function, site))
    targets = {}
    for entry, spec in TARGETS.items():
        body = code[entry:spec["end"]]
        decoded = list(decoder.disasm(body, entry))
        check(len(decoded) == spec["insns"] and decoded[-1].address == spec["retf"] and decoded[-1].mnemonic == "retf"
              and decoded[-1].op_str == "4" and spec["end"] in starts and entry in starts, f"+{entry:04X} extent differs")
        check(not any("call" in i.mnemonic or i.mnemonic.startswith("int") for i in decoded), f"+{entry:04X} is not a leaf")
        conditional = {i.address: (i.address + i.size, int(i.op_str, 16)) for i in decoded if i.mnemonic.startswith("j") and i.mnemonic != "jmp"}
        check(len(conditional) == spec["conditionals"], f"+{entry:04X} conditional count differs")
        check(near.get(entry) == {spec["caller"]}, f"+{entry:04X} aligned near callers differ: {near.get(entry)}")
        check(entry not in unit["entries"], f"+{entry:04X} is unexpectedly a public entry")
        targets[entry] = dict(spec, entry=entry, body=body, decoded=decoded, conditional=conditional, sha=sha256(body).hexdigest())
        print(f"u3B4 +{entry:04X} ({len(body)}/{len(decoded)}, leaf, RETF 4) sha {targets[entry]['sha'][:16]}...; "
              f"caller +{spec['caller'][0]:04X}@+{spec['caller'][1]:04X}")
    return targets, code


def library(directory):
    path = directory / "u3b4_case_maps.so"
    subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-O2", "-fPIC", "-shared",
                    "-I", str(ROOT / "include"), str(ROOT / "src/recovered/overlay_u3b4_case_maps.c"),
                    "-o", str(path)], check=True, capture_output=True, text=True)
    lib = ctypes.CDLL(str(path))
    functions = {}
    for entry, spec in TARGETS.items():
        f = getattr(lib, spec["symbol"])
        f.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t, ctypes.POINTER(Cpu)]
        f.restype = ctypes.c_int
        functions[entry] = f
    return functions


def fixture(code, entry, *, seed=1, argument=0x3C00, link=0x6E6E, ss=SS, sp=0x8000, ds=DS, es=0x1111, flags=0x7202,
            background=None, regs=None):
    memory = bytearray([0xA5]) * SIZE if background is None else bytearray(background)
    at = physical(CS, 0); memory[at:at + len(code)] = code
    frame = physical(ss, sp)
    for delta, value in ((0, 0x7000), (2, 0x7001), (4, link), (6, argument)):
        put16(memory, ss, (sp + delta) & 0xFFFF, value)            # the CPU's segment-wrapped view
        if delta < 4:                                              # and the physically contiguous RETF frame
            memory[(frame + delta) & (SIZE - 1)] = value & 0xFF; memory[(frame + delta + 1) & (SIZE - 1)] = value >> 8
    r = regs or (0x5AA5, 0x3456, 0x5678, 0x789A, 0x9ABC, 0xBCDE, 0x7777)
    cpu = Cpu(*r, sp, CS, entry, ds, es, ss, flags)
    return memory, cpu, ss >= 0xF000


def compare(label, memory, cpu, high, target, function, budget=10000):
    from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
    original = Uc(UC_ARCH_X86, UC_MODE_16)
    original.mem_map(0, 2 * SIZE)
    original.mem_write(0, bytes(memory)); original.mem_write(SIZE, bytes(memory))
    for (name, _), register in zip(Cpu._fields_, REGISTERS):
        original.reg_write(register, getattr(cpu, name))
    start = physical(cpu.cs, target["entry"]); body_end = start + len(target["body"]); last = physical(cpu.cs, target["retf"])
    returned, previous, visited, edges = [False], [None], set(), set()

    def instruction(emulator, address, size, user):
        del size, user
        if returned[0]:
            emulator.emu_stop(); return
        here = address - start + target["entry"]
        check(start <= address < body_end, f"left the body: {label}")
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
    check(returned[0], f"original did not return: {label}")
    expected = bytes(original.mem_read(0, SIZE))
    ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
    check(function(ram, SIZE, ctypes.byref(cpu)) == 0, f"C status: {label}")
    actual = bytes(ram)
    if actual != expected:
        differences = [i for i, (a, b) in enumerate(zip(actual, expected)) if a != b]
        raise ValueError(f"RAM mismatch {label}: {[(hex(d), actual[d], expected[d]) for d in differences[:6]]} ({len(differences)} bytes)")
    for (name, _), register in zip(Cpu._fields_, REGISTERS):
        got, want = getattr(cpu, name), original.reg_read(register) & 0xFFFF
        check(got == want, f"{name} mismatch {label}: C={got:04x}, original={want:04x}")
    return visited, edges


def expected_al(target, argument):
    return dict(target["table"]).get(argument & 0xFF, target["default"])


def cases(entry):
    rng = random.Random(0x3B40000 + entry)
    spec = []
    for b in range(256):
        spec.append((f"arg-{b:02x}", dict(argument=(rng.randrange(256) << 8) | b, flags=(0x7202, 0x0ED7, 0x0002, 0x08D7)[b % 4])))
    for i in range(48):
        spec.append((f"random-{i}", dict(argument=rng.randrange(0x10000), link=rng.randrange(0x10000), ds=rng.randrange(0x10000),
                                        es=rng.randrange(0x10000), flags=rng.choice((0x7202, 0x0202, 0x0ED7, 0x0647, 0x0002)),
                                        regs=tuple(rng.randrange(0x10000) for _ in range(7)))))
    hits = [value for value, _ in TARGETS[entry]["table"]]
    for sp in (0x0000, 0x0001, 0x0002, 0x0004, 0x0006, 0x0008, 0xFFF8, 0xFFFA, 0xFFFB, 0xFFFC, 0xFFFE, 0xFFFF):
        spec.append((f"sp-{sp:04x}", dict(argument=hits[sp % len(hits)], ss=0x1800, sp=sp, background=bytes(SIZE))))
    spec.append(("ss-a20", dict(argument=hits[0], ss=0xFFFF, sp=0x0400)))
    spec.append(("ss-a20-wrap", dict(argument=hits[-1], ss=0xFFFF, sp=0xFFFA)))
    spec.append(("random-background", dict(argument=0x0100 | hits[1], background=bytes(random.Random(9).randbytes(SIZE)))))
    spec.append(("preserved-df-if", dict(argument=0x00FF, flags=0x0603)))
    return spec


def differential(targets, code, functions):
    report = {}
    for entry, target in targets.items():
        visited, edges, count = set(), set(), 0
        for label, options in cases(entry):
            memory, cpu, high = fixture(code, entry, **options)
            want = expected_al(target, options["argument"])
            v, e = compare(f"+{entry:04X} {label}", memory, cpu, high, target, functions[entry])
            check(cpu.ax & 0xFF == want, f"+{entry:04X} {label}: AL {cpu.ax & 0xFF:02x}, documented {want:02x}")
            visited |= v; edges |= e; count += 1
        missing_sites = sorted(set(i.address for i in target["decoded"]) - visited)
        missing_edges = sorted({(s, o) for s, outs in target["conditional"].items() for o in outs} - edges)
        check(not missing_sites and not missing_edges, f"+{entry:04X} coverage incomplete: {missing_sites} {missing_edges}")
        print(f"+{entry:04X}: {count} original-byte cases passed, full 1 MiB RAM and all 14 CPU words equal, AL as documented; "
              f"{len(visited)}/{len(target['decoded'])} instruction starts, {len(edges)}/{2 * len(target['conditional'])} conditional outcomes")
        report[f"{entry:04X}"] = dict(returning=count, starts=len(visited), outcomes=len(edges), sha256=target["sha"],
                                      bytes=len(target["body"]), instructions=len(target["decoded"]))
    return report


# ---- DOSBox-X synthetic oracle -------------------------------------------------------------------------
MAIN_AT, UNIT_AT = 0x0300, 0x1000
OUTPUT, HEADER, STACK_TOP = 0x9000, 0x9800, 0xF000
FRAME_LOW, FRAME_SIZE = 0xEFF0, 0x10
RECORD_BYTES = 28 + FRAME_SIZE
FLAGS = (0x0002, 0x00D7, 0x08D7, 0x0402, 0x0893, 0x0003)
ENTRY_REGS = dict(bx=0x3456, cx=0x5678, dx=0x789A, si=0x9ABC, di=0xBCDE, bp=0x7777)
LINK, ARG_HIGH, ENTRY_AH = 0x6E6E, 0x3C, 0x5A
ORDER = (0x00C7, 0x00FB, 0x012F)


def assembly():
    count = 256 * len(ORDER)
    text = [".code16", ".intel_syntax noprefix", ".global _start", "_start:", "    jmp main",
            f".org {MAIN_AT - 0x100:#x}", "main:",
            "    mov ax,cs", "    mov ds,ax", "    mov es,ax", "    mov ss,ax", f"    mov sp,{STACK_TOP:#x}",
            "    mov ax,cs", f"    add ax,{UNIT_AT // 16:#x}"]
    for n in range(len(ORDER)):
        text.append(f"    mov word ptr [targets+{4 * n + 2}],ax")
    text += [f"    mov word ptr [{HEADER:#x}],cs", f"    mov word ptr [{HEADER + 2:#x}],offset returned",
             f"    mov word ptr [{HEADER + 4:#x}],{count}", f"    mov word ptr [{HEADER + 6:#x}],0",
             "    mov dx,offset filename", "    xor cx,cx", "    mov ah,0x3c", "    int 0x21", "    jc failure",
             "    mov word ptr [handle],ax", "    mov bx,ax", f"    mov dx,{HEADER:#x}", "    mov cx,8",
             "    mov ah,0x40", "    int 0x21", "    jc failure",
             "again:", "    cli", f"    mov sp,{STACK_TOP:#x}", "    cld", "    push cs", "    pop es",
             "    mov al,0xa5", f"    mov di,{FRAME_LOW:#x}", f"    mov cx,{FRAME_SIZE}", "    rep stosb",
             "    mov ax,word ptr [index]", "    xor dx,dx", f"    mov cx,{len(FLAGS)}", "    div cx",
             "    mov bx,dx", "    shl bx,1", "    mov ax,word ptr [flags_table+bx]", "    mov word ptr [case_flags],ax",
             "    mov ax,word ptr [index]", "    mov bl,ah", "    xor bh,bh", "    shl bx,1", "    shl bx,1",
             "    mov dx,word ptr [targets+bx]", "    mov word ptr [target_far],dx",
             "    mov dx,word ptr [targets+bx+2]", "    mov word ptr [target_far+2],dx",
             f"    mov ah,{ARG_HIGH:#x}", "    push ax", f"    mov ax,{LINK:#x}", "    push ax",
             "    push word ptr [case_flags]", "    popf", f"    mov ax,{ENTRY_AH << 8 | 0xA5:#x}"]
    for name, value in ENTRY_REGS.items():
        text.append(f"    mov {name},{value:#x}")
    text += ["    call dword ptr [target_far]", "returned:"]
    for i, reg in enumerate("ax bx cx dx si di bp sp cs".split()):
        text.append(f"    mov word ptr [{OUTPUT + 2 * i:#x}],{reg}")
    text += [f"    mov word ptr [{OUTPUT + 18:#x}],offset returned", f"    mov word ptr [{OUTPUT + 20:#x}],ds",
             f"    mov word ptr [{OUTPUT + 22:#x}],es", f"    mov word ptr [{OUTPUT + 24:#x}],ss",
             f"    mov sp,{STACK_TOP + 0x800:#x}", "    pushf", "    pop ax", f"    mov word ptr [{OUTPUT + 26:#x}],ax",
             "    cld", "    push cs", "    pop ds", "    push cs", "    pop es", f"    mov di,{OUTPUT + 28:#x}",
             f"    mov si,{FRAME_LOW:#x}", f"    mov cx,{FRAME_SIZE}", "    rep movsb",
             "    sti", "    mov bx,word ptr [handle]", f"    mov dx,{OUTPUT:#x}", f"    mov cx,{RECORD_BYTES}",
             "    mov ah,0x40", "    int 0x21", "    jc failure", f"    cmp ax,{RECORD_BYTES}", "    jne failure",
             "    inc word ptr [index]", "    mov ax,word ptr [index]", f"    cmp ax,{count}", "    jb again",
             "    mov bx,word ptr [handle]", "    mov ah,0x3e", "    int 0x21", "    mov ax,0x4c00", "    int 0x21",
             "failure:", "    mov ax,0x4c01", "    int 0x21",
             'filename: .asciz "RESULT.BIN"', "handle: .word 0", "index: .word 0", "case_flags: .word 0",
             "target_far: .word 0, 0", "targets: .word " + ", ".join(f"{e:#x}, 0" for e in ORDER),
             "flags_table: .word " + ",".join(f"{f:#x}" for f in FLAGS)]
    return "\n".join(text) + "\n"


def native_audit(code, functions):
    directory = ROOT / "resources/oracle-runs/u3b4-case-maps-synthetic"
    directory.mkdir(parents=True, exist_ok=True)
    asm, obj = directory / "oracle.S", directory / "oracle.o"
    asm.write_text(assembly())
    subprocess.run(["as", "--32", str(asm), "-o", str(obj)], check=True)
    subprocess.run(["ld", "-m", "elf_i386", "-Ttext", "0x100", "--oformat", "binary", str(obj), "-o", str(directory / "main.bin")], check=True)
    assembled = (directory / "main.bin").read_bytes()
    check(0x100 + len(assembled) <= UNIT_AT, "harness reaches the unit 0x3B4 copy")
    image = bytearray(UNIT_AT + UNIT_SIZE - 0x100)
    image[:len(assembled)] = assembled
    image[UNIT_AT - 0x100:UNIT_AT - 0x100 + UNIT_SIZE] = code
    (directory / "ORACLE.COM").write_bytes(bytes(image))
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
    count = 256 * len(ORDER)
    check(len(result) == 8 + count * RECORD_BYTES, f"native output incomplete: {len(result)} bytes")
    segment, returned, recorded, _ = struct.unpack_from("<4H", result)
    check(recorded == count, "native header differs")
    for n in range(count):
        entry, b = ORDER[n >> 8], n & 0xFF
        record = result[8 + n * RECORD_BYTES:8 + (n + 1) * RECORD_BYTES]
        memory = bytearray(SIZE)
        for i in range(FRAME_SIZE):
            memory[physical(segment, FRAME_LOW + i)] = 0xA5
        sp = STACK_TOP
        for value in ((ARG_HIGH << 8) | b, LINK, segment, returned):
            sp -= 2; put16(memory, segment, sp, value)
        cpu = Cpu(ENTRY_AH << 8 | 0xA5, ENTRY_REGS["bx"], ENTRY_REGS["cx"], ENTRY_REGS["dx"], ENTRY_REGS["si"], ENTRY_REGS["di"],
                  ENTRY_REGS["bp"], sp, (segment + UNIT_AT // 16) & 0xFFFF, entry, segment, segment, segment, FLAGS[n % len(FLAGS)])
        ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
        check(functions[entry](ram, SIZE, ctypes.byref(cpu)) == 0, f"C status on native case {n}")
        for (name, _), want in zip(Cpu._fields_, struct.unpack_from("<14H", record)):
            check(getattr(cpu, name) == want, f"native case {n} (+{entry:04X}, b={b:02x}) {name}: C={getattr(cpu, name):04x}, DOS={want:04x}")
        at = physical(segment, FRAME_LOW)
        check(bytes(ram[at:at + FRAME_SIZE]) == record[28:28 + FRAME_SIZE], f"native case {n} frame bytes differ")
    digest = sha256(result).hexdigest()
    print(f"DOSBox-X normal-core/386 synthetic cases passed: {count} (all 256 argument bytes for each routine); "
          f"all 14 CPU words and {FRAME_SIZE} frame bytes; no mask")
    print(f"native result SHA-256 {digest}; COM segment {segment:04x}")
    return dict(cases=count, result_sha256=digest)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--unicorn", action="store_true")
    parser.add_argument("--native", action="store_true")
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    targets, code = source()
    report = dict(targets={f"{e:04X}": dict(sha256=t["sha"], bytes=len(t["body"]), instructions=t["insns"]) for e, t in targets.items()})
    if args.unicorn or args.native:
        with tempfile.TemporaryDirectory(prefix="poolrad-u3b4-") as temp:
            functions = library(Path(temp))
            if args.unicorn:
                report["unicorn"] = differential(targets, code, functions)
            if args.native:
                report["native"] = native_audit(code, functions)
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(json.dumps(report, sort_keys=True, indent=2) + "\n")
    print(json.dumps(report, sort_keys=True))


if __name__ == "__main__":
    main()
