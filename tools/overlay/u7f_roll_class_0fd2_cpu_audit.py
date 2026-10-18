#!/usr/bin/env python3
"""Original-byte full-state parity for raw-CPU U7F +0FD2 with the U9E entry-8 dice chain composed (through the loaded stub
00B0:0048), plus a DOSBox-X oracle.

The differential places the hash-checked expanded START.EXE image at load segment 0, so the SYSTEM generator runs from its
original bytes, patches the stub to its loaded form, loads units 0x9E and 0x7F, and compares the entire 1 MiB RAM and all
14 CPU words between the original under Unicorn and the C model, with no mask. The original decides each stop: its return,
a site or stub whose live bytes differ at its fetch (CODE), or the chain reaching its SYSTEM call +0E0E once more than the
host's max_system_calls (CALLEE). Media stays private.
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

ENTRY, END, BODY_RETF, SITE = 0x0FD2, 0x1010, 0x100F, 0x0FDE
U9E_SHA = "9a2681e8b24a9a95838a11f2d7cbddea0c253d29e631db37d25fae017a14693b"
SYSTEM_CALL = 0x0E0E
CALLER_CS, DICE_CS, DS, SS, EDGE_SS = 0x8800, 0x4800, 0x1800, 0x2800, 0x2000
RET_CS, RET_IP = 0x9001, 0x7000
OK, CODE, CALLEE = 0, 2, 3
NAMES = {OK: "return", CODE: "CODE", CALLEE: "CALLEE"}
# A 1d20 is 1..20, so the class code's r = 0 edges (JB +0FEB taken, then JB +0FF9 taken) cannot be reached.
UNREACHABLE_EDGES = {(0x0FEB, 0x0FF7), (0x0FF9, 0x1005)}


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
    u7f, u9e = units[0x7F], units[0x9E]
    code7f = ovr[u7f["ovr_offset"]:u7f["ovr_offset"] + u7f["code_size"]]
    code9e = ovr[u9e["ovr_offset"]:u9e["ovr_offset"] + u9e["code_size"]]
    check(sha256(code9e[0x0DE3:0x0E52]).hexdigest() == U9E_SHA and u9e["entries"][8] == 0x0DE3, "unit 0x9E entry 8 differs")
    starts = sorted(f["rel"] for f in u7f["functions"])
    check(END == next(s for s in starts if s > ENTRY), "U7F +0FD2 bounds differ")
    body = code7f[ENTRY:END]
    decoded = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(body, ENTRY))
    calls = [(i.address, i.op_str) for i in decoded if "call" in i.mnemonic]
    conditional = {i.address: (i.address + i.size, int(i.op_str, 16)) for i in decoded if i.mnemonic.startswith("j") and i.mnemonic != "jmp"}
    check(len(body) == 62 and len(decoded) == 27 and sum(i.size for i in decoded) == 62 and decoded[-1].address == BODY_RETF
          and decoded[-1].mnemonic == "retf" and decoded[-1].op_str == "" and calls == [(SITE, "0xb0, 0x48")]
          and SITE + 3 in set(u7f["fixups"]) and len(conditional) == 4, f"U7F +0FD2 extent or calls differ: {calls}")
    target = dict(body=body, decoded=decoded, conditional=conditional, sha=sha256(body).hexdigest(), file_offset=u7f["ovr_offset"] + ENTRY)
    print(f"U7F +0FD2 (62/27, LCALL 00B0:0048 to U9E +0DE3, RETF) GAME file {target['file_offset']:#x} sha {target['sha']}")
    return target, code7f, code9e, image


def library(directory):
    path = directory / "u7f0fd2.so"
    sources = ["overlay_u7f_roll_class_0fd2_cpu.c", "overlay_u9e_entry8_fullstate_compose_0de3.c", "overlay_u9e_entry8_fullstate_prefix_0de3.c",
               "overlay_u9e_entry8_fullstate_loop_0e05.c", "overlay_u9e_entry8_fullstate_postcall_0e13.c", "overlay_u9e_entry8_fullstate_tail_0e1f.c",
               "overlay_u9e_entry8_system_call_0e0e.c", "system_random_fullstate_11c0.c", "system_seed_fullstate_120f.c"]
    subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-O2", "-fPIC", "-shared", "-I", str(ROOT / "include"),
                    *[str(ROOT / "src/recovered" / s) for s in sources], "-o", str(path)], check=True, capture_output=True, text=True)
    function = ctypes.CDLL(str(path)).pr_u7f_roll_class_0fd2_cpu
    function.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t, ctypes.POINTER(Cpu), ctypes.c_uint, ctypes.POINTER(ctypes.c_int)]
    function.restype = ctypes.c_int
    return function


def build(code7f, code9e, image, rng, *, seed=None, sp=0x8000, ss=SS, flags=0x7202, background=None, patch=(), stub_patch=(), base=0):
    memory = bytearray([0xA5]) * SIZE if background is None else bytearray(background)
    memory[0:len(image)] = image
    memory[physical(0x00B0, 0x48):physical(0x00B0, 0x48) + 5] = struct.pack("<BHH", 0xEA, 0x0DE3, DICE_CS)
    at = physical(DICE_CS, 0); memory[at:at + len(code9e)] = code9e
    at = physical(CALLER_CS, base); memory[at:at + len(code7f)] = code7f
    for offset, value in patch:
        memory[physical(CALLER_CS, (base + offset) & 0xFFFF)] = value
    for offset, value in stub_patch:
        memory[physical(0x00B0, offset)] = value
    s = rng.randrange(1 << 32) if seed is None else seed
    put16(memory, DS, 0x4428, s & 0xFFFF); put16(memory, DS, 0x442A, s >> 16)
    frame = physical(ss, sp)
    for i, value in enumerate([RET_IP, RET_CS]):
        put16(memory, ss, (sp + 2 * i) & 0xFFFF, value)
        put16(memory, 0, (frame + 2 * i) & (SIZE - 1), value)
    reg = tuple(rng.randrange(0x10000) for _ in range(7))
    return memory, Cpu(*reg, sp, CALLER_CS, (base + ENTRY) & 0xFFFF, DS, rng.randrange(0x10000), ss, flags), ss >= 0xF000


def compare(label, memory, cpu, high, target, function, cap=256, base=0, budget=3000000):
    from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
    original = Uc(UC_ARCH_X86, UC_MODE_16)
    original.mem_map(0, 2 * SIZE)
    original.mem_write(0, bytes(memory)); original.mem_write(SIZE, bytes(memory))
    for (name, _), register in zip(Cpu._fields_, REGISTERS):
        original.reg_write(register, getattr(cpu, name))
    origin = physical(CALLER_CS, base)
    body = (origin + ENTRY, origin + END)
    ret = physical(RET_CS, RET_IP)
    system_call = physical(DICE_CS, SYSTEM_CALL)
    state = dict(stop=None, previous=None, calls=0)
    visited, edges = set(), set()

    def hook(emulator, address, size, user):
        del size, user
        if address == ret:
            state["stop"] = OK
            return emulator.emu_stop()
        if body[0] <= address < body[1]:
            here = address - origin
            if here == SITE:
                raw = bytes(emulator.mem_read(address, 5))
                stub = physical(raw[3] | raw[4] << 8, 0x0048)
                if raw[:3] != b"\x9a\x48\x00" or bytes(emulator.mem_read(stub, 3)) != b"\xea\xe3\x0d":
                    state["stop"] = CODE
                    return emulator.emu_stop()
            visited.add(here)
            if state["previous"] in target["conditional"] and state["previous"] != here:
                edges.add((state["previous"], here))
            state["previous"] = here
            return None
        state["previous"] = None
        if address == system_call:
            state["calls"] += 1
            if state["calls"] > cap:
                state["stop"] = CALLEE
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
    original.emu_start(body[0], 2 * SIZE, count=budget)
    check(state["stop"] is not None, f"original did not stop: {label}")
    expected = bytes(original.mem_read(0, SIZE))
    ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
    callee = ctypes.c_int(99)
    status = function(ram, SIZE, ctypes.byref(cpu), cap, ctypes.byref(callee))
    check(status == state["stop"], f"C status {status} (callee {callee.value}), original stopped with {NAMES[state['stop']]}: {label}")
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
    for i in range(300):
        spec.append((f"random-{i}", dict(flags=(0x7202, 0x0202, 0x0ED7, 0x0647)[i % 4], sp=rng.randrange(0x0400, 0xF000) & ~1)))
    for sp in (0x0040, 0x0100, 0xFFC0, 0xFFE8):
        spec.append((f"sp-{sp:04x}", dict(sp=sp, ss=EDGE_SS)))
    spec.append(("ss-a20", dict(sp=0x0400, ss=0xFFFF)))
    spec.append(("random-background", dict(background=bytes(random.Random(0x7F0FD2).randbytes(SIZE)))))
    spec.append(("relocated-entry", dict(base=0x2000)))
    spec.append(("preserved-df-if", dict(flags=0x0603)))
    spec.append(("code-site", dict(patch=((SITE + 1, 0x49),))))
    spec.append(("code-stub", dict(stub_patch=((0x49, 0xE4),))))
    spec.append(("system-cap-0", dict(cap=0)))
    return spec


def differential(target, code7f, code9e, image, function):
    rng = random.Random(0x7F0FD2)
    visited, edges, stops = set(), set(), {k: 0 for k in NAMES}
    for label, options in cases(rng):
        cap = options.pop("cap", 256)
        base = options.get("base", 0)
        memory, cpu, high = build(code7f, code9e, image, rng, **options)
        stop, v, e = compare(f"+0FD2 {label}", memory, cpu, high, target, function, cap, base)
        stops[stop] += 1
        visited |= v
        edges |= e
    reachable = {(s, o) for s, outs in target["conditional"].items() for o in outs} - UNREACHABLE_EDGES
    missing = sorted(hex(i.address) for i in target["decoded"] if i.address not in visited)
    missing_edges = sorted((hex(s), hex(o)) for s, o in reachable - edges)
    check(not missing and not missing_edges and not (edges & UNREACHABLE_EDGES), f"coverage incomplete: {missing} {missing_edges}")
    check(stops[CODE] and stops[CALLEE], f"a stop kind was not exercised: {stops}")
    print(f"U7F +0FD2: {sum(stops.values())} original-byte cases passed ({stops[OK]} returns, {stops[CODE]} CODE, {stops[CALLEE]} CALLEE); "
          f"full 1 MiB RAM and all 14 CPU words equal; {len(visited)}/{len(target['decoded'])} instruction starts, "
          f"{len(edges)}/{len(reachable)} reachable conditional outcomes (the two r = 0 edges are unreachable for a 1d20)")
    return dict(stops={NAMES[k]: v for k, v in stops.items()}, starts=len(visited), outcomes=len(edges), reachable_outcomes=len(reachable))


# ---- DOSBox-X synthetic oracle -------------------------------------------------------------------------
CALLER_N, DICE_N, STUB_N, DATA_DS = 0x8800, 0x8000, 0x7F00, 0x1900
MAIN_AT, U9E_BLOB, SYS_BLOB, CASES_AT, CODE_AT = 0x0300, 0x0C80, 0x0D00, 0x0E00, 0x4100
SYSTEM_HOLE = (0x4000, 0x4090)
OUTPUT, HEADER, STACK_TOP = 0xC000, 0xC800, 0xF000
FRAME_LOW, FRAME_SIZE = 0xEF80, 0x80
CASE_SIZE = 6
RECORD_BYTES = 28 + FRAME_SIZE + 4
NATIVE_FLAGS = (0x0002, 0x00D7, 0x08D7, 0x0402, 0x0893, 0x0003)
ENTRY_REGS = dict(ax=0x1234, bx=0x3456, cx=0x5678, dx=0x789A, si=0x9ABC, di=0xBCDE, bp=0x7777)
SYSTEM_FILE = (0xBE36, 0xBEBD)


def native_audit(target, code7f, code9e, image, function):
    del target, image
    directory = ROOT / "resources/oracle-runs/u7f-0fd2-synthetic"
    directory.mkdir(parents=True, exist_ok=True)
    exe = (ROOT / "resources/DOS/POOLRAD/START.EXE").read_bytes()
    system = exe[SYSTEM_FILE[0]:SYSTEM_FILE[1]]
    span = code9e[0x0DE3:0x0E52]
    unit = bytearray(code7f)
    struct.pack_into("<H", unit, SITE + 3, STUB_N)
    rng = random.Random(0x7F0FD2D)
    cases = [dict(seed=rng.randrange(1 << 32), flags=n % len(NATIVE_FLAGS)) for n in range(160)]
    check(CASES_AT + CASE_SIZE * len(cases) <= SYSTEM_HOLE[0] and CODE_AT >= SYSTEM_HOLE[1] and CODE_AT + len(unit) <= OUTPUT
          and SYS_BLOB + len(system) <= CASES_AT and U9E_BLOB + len(span) <= SYS_BLOB, "COM layout overlaps")
    text = [".code16", ".intel_syntax noprefix", ".global _start", "_start:", "    jmp main", f".org {MAIN_AT - 0x100:#x}", "main:",
            "    mov ax,cs", "    mov ds,ax", "    mov es,ax", "    mov ss,ax", f"    mov sp,{STACK_TOP:#x}", "    cld"]
    for segment, offset, label, length in ((CALLER_N, 0, "unit", len(unit)), (DICE_N, 0x0DE3, "dice", len(span)), (0x0AF8, 0x11C0, "system", len(system))):
        text += [f"    mov ax,{segment:#x}", "    mov es,ax", f"    mov si,offset {label}", f"    mov di,{offset:#x}", f"    mov cx,{length:#x}", "    rep movsb"]
    text += [f"    mov ax,{STUB_N:#x}", "    mov es,ax", "    mov byte ptr es:[0x48],0xea", "    mov word ptr es:[0x49],0x0de3",
             f"    mov word ptr es:[0x4b],{DICE_N:#x}", "    push cs", "    pop es",
             f"    mov word ptr [{HEADER:#x}],cs", f"    mov word ptr [{HEADER + 2:#x}],offset returned",
             f"    mov word ptr [{HEADER + 4:#x}],{len(cases)}", f"    mov word ptr [{HEADER + 6:#x}],0",
             "    mov dx,offset filename", "    xor cx,cx", "    mov ah,0x3c", "    int 0x21", "    jc failure",
             "    mov word ptr [handle],ax", "    mov bx,ax", f"    mov dx,{HEADER:#x}", "    mov cx,8", "    mov ah,0x40", "    int 0x21", "    jc failure",
             "again:", "    cli", f"    mov sp,{STACK_TOP:#x}", "    cld", "    push cs", "    pop ds", "    push cs", "    pop es",
             "    mov al,0xa5", f"    mov di,{FRAME_LOW:#x}", f"    mov cx,{FRAME_SIZE}", "    rep stosb",
             "    mov ax,word ptr [index]", f"    mov bx,{CASE_SIZE}", "    mul bx", "    add ax,offset cases", "    mov si,ax",
             f"    mov ax,{DATA_DS:#x}", "    mov es,ax", "    mov ax,word ptr [si]", "    mov word ptr es:[0x4428],ax",
             "    mov ax,word ptr [si+2]", "    mov word ptr es:[0x442a],ax", "    push cs", "    pop es",
             "    mov bl,byte ptr [si+4]", "    xor bh,bh", "    shl bx,1", "    push word ptr [flags_table+bx]",
             f"    mov ax,{DATA_DS:#x}", "    mov ds,ax", "    popf"]
    for name, value in ENTRY_REGS.items():
        text.append(f"    mov {name},{value:#x}")
    text += ["    call dword ptr cs:[target_far]", "returned:"]
    for i, reg in enumerate("ax bx cx dx si di bp sp cs".split()):
        text.append(f"    mov word ptr cs:[{OUTPUT + 2 * i:#x}],{reg}")
    text += [f"    mov word ptr cs:[{OUTPUT + 18:#x}],offset returned", f"    mov word ptr cs:[{OUTPUT + 20:#x}],ds",
             f"    mov word ptr cs:[{OUTPUT + 22:#x}],es", f"    mov word ptr cs:[{OUTPUT + 24:#x}],ss",
             f"    mov sp,{STACK_TOP + 0x800:#x}", "    pushf", "    pop ax", f"    mov word ptr cs:[{OUTPUT + 26:#x}],ax",
             "    cld", "    push cs", "    pop ds", "    push cs", "    pop es", f"    mov di,{OUTPUT + 28:#x}",
             f"    mov si,{FRAME_LOW:#x}", f"    mov cx,{FRAME_SIZE}", "    rep movsb",
             f"    mov ax,{DATA_DS:#x}", "    mov ds,ax", "    mov si,0x4428", "    mov cx,4", "    rep movsb",
             "    push cs", "    pop ds", "    sti",
             "    mov bx,word ptr [handle]", f"    mov dx,{OUTPUT:#x}", f"    mov cx,{RECORD_BYTES}", "    mov ah,0x40", "    int 0x21", "    jc failure",
             f"    cmp ax,{RECORD_BYTES}", "    jne failure",
             "    inc word ptr [index]", "    mov ax,word ptr [index]", f"    cmp ax,{len(cases)}", "    jb again",
             "    mov bx,word ptr [handle]", "    mov ah,0x3e", "    int 0x21", "    mov ax,0x4c00", "    int 0x21",
             "failure:", "    mov ax,0x4c01", "    int 0x21",
             'filename: .asciz "RESULT.BIN"', "handle: .word 0", "index: .word 0", f"target_far: .word {ENTRY:#x}, {CALLER_N:#x}",
             "flags_table: .word " + ",".join(f"{f:#x}" for f in NATIVE_FLAGS)]
    for label, blob, at in (("dice", span, U9E_BLOB), ("system", system, SYS_BLOB)):
        text += [f".org {at - 0x100:#x}", f"{label}:"]
        for i in range(0, len(blob), 16):
            text.append("    .byte " + ",".join(f"{b:#x}" for b in blob[i:i + 16]))
    text += [f".org {CASES_AT - 0x100:#x}", "cases:"]
    for case in cases:
        text.append("    .byte " + ",".join(f"{b:#x}" for b in list(struct.pack("<I", case["seed"])) + [case["flags"], 0]))
    text += [f".org {CODE_AT - 0x100:#x}", "unit:"]
    for i in range(0, len(unit), 16):
        text.append("    .byte " + ",".join(f"{b:#x}" for b in unit[i:i + 16]))
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
    classes = {}
    for n, case in enumerate(cases):
        record = result[8 + n * RECORD_BYTES:8 + (n + 1) * RECORD_BYTES]
        memory = bytearray(SIZE)
        for seg, off, blob in ((CALLER_N, 0, bytes(unit)), (DICE_N, 0x0DE3, span), (0x0AF8, 0x11C0, system)):
            at = physical(seg, off); memory[at:at + len(blob)] = blob
        memory[physical(STUB_N, 0x48):physical(STUB_N, 0x48) + 5] = struct.pack("<BHH", 0xEA, 0x0DE3, DICE_N)
        put16(memory, DATA_DS, 0x4428, case["seed"] & 0xFFFF); put16(memory, DATA_DS, 0x442A, case["seed"] >> 16)
        for i in range(FRAME_SIZE):
            memory[physical(segment, FRAME_LOW + i)] = 0xA5
        sp = STACK_TOP
        for value in (segment, returned):
            sp -= 2; put16(memory, segment, sp, value)
        cpu = Cpu(*ENTRY_REGS.values(), sp, CALLER_N, ENTRY, DATA_DS, segment, segment, NATIVE_FLAGS[case["flags"]])
        ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
        callee = ctypes.c_int(99)
        check(function(ram, SIZE, ctypes.byref(cpu), 256, ctypes.byref(callee)) == 0, f"C status on native case {n}")
        for (name, _), want in zip(Cpu._fields_, struct.unpack_from("<14H", record)):
            check(getattr(cpu, name) == want, f"native case {n} {name}: C={getattr(cpu, name):04x}, DOS={want:04x}")
        got = bytes(ram[physical(segment, FRAME_LOW):physical(segment, FRAME_LOW) + FRAME_SIZE])
        got += bytes(ram[physical(DATA_DS, 0x4428):physical(DATA_DS, 0x4428) + 4])
        check(got == record[28:], f"native case {n} bytes differ")
        classes[cpu.ax & 0xFF] = classes.get(cpu.ax & 0xFF, 0) + 1
    digest = sha256(result).hexdigest()
    print(f"DOSBox-X normal-core/386 synthetic cases passed: {len(cases)} (classes {dict(sorted(classes.items()))}); all 14 CPU words, "
          f"the {FRAME_SIZE}-byte stack window and the seed dword; no mask")
    print(f"native result SHA-256 {digest}; COM segment {segment:04x}; U9E +0DE3 span at {DICE_N:04X}, SYSTEM generator at 0AF8:11C0")
    return dict(cases=len(cases), classes={str(k): v for k, v in sorted(classes.items())}, result_sha256=digest)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--unicorn", action="store_true")
    parser.add_argument("--native", action="store_true")
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    target, code7f, code9e, image = source()
    report = dict(target=dict(sha256=target["sha"], bytes=62, instructions=27, file_offset=target["file_offset"]))
    if args.unicorn or args.native:
        with tempfile.TemporaryDirectory(prefix="poolrad-u7f0fd2-") as temp:
            function = library(Path(temp))
            if args.unicorn:
                report["unicorn"] = differential(target, code7f, code9e, image, function)
            if args.native:
                report["native"] = native_audit(target, code7f, code9e, image, function)
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(json.dumps(report, sort_keys=True, indent=2) + "\n")
    print(json.dumps(report, sort_keys=True))


if __name__ == "__main__":
    main()
