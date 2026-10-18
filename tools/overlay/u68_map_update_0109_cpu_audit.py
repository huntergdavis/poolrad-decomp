#!/usr/bin/env python3
"""Original-byte full-state parity for the raw-CPU U68 +0109 map update, plus a DOSBox-X oracle.

The differential places the hash-checked expanded START.EXE image at load segment 0, loads unit 0x68, and compares the
entire 1 MiB RAM and all 14 CPU words between the original under Unicorn and the C model, with no mask, after its
RETF 6: in-range and out-of-range signed x/y bytes, every selector byte, map pointers that wrap past FFFF at +300h,
SS across A20 and DF set. Every case keeps its stack frame apart from the map. Media stays private.
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

CALLER_CS, ENTRY, END, BODY_RETF = 0x8800, 0x0109, 0x0236, 0x0233
DS, SS = 0x1800, 0x2800
RET_CS, RET_IP = 0x9001, 0x7000
FLAGS = (0x0602, 0x0202, 0x0ED7, 0x0647, 0x72D7, 0x0403, 0x0213)


def check(condition, message):
    if not condition:
        raise ValueError(message)


def source():
    exe = (ROOT / "resources/DOS/POOLRAD/START.EXE").read_bytes()
    ovr = (ROOT / "resources/DOS/POOLRAD/GAME.OVR").read_bytes()
    check(sha256(exe).hexdigest() == EXE_SHA and len(ovr) == 206105 and sha256(ovr).hexdigest() == OVR_SHA, "media provenance differs")
    image = bytes(unpack(exe)[0])
    u = next(u for u in find_units(exe[parse_mz(exe)["header_bytes"]:], ovr) if u["stub_para"] == 0x68)
    code = ovr[u["ovr_offset"]:u["ovr_offset"] + u["code_size"]]
    insns = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(code[ENTRY:END], ENTRY))
    check(len(insns) == 125 and sum(i.size for i in insns) == END - ENTRY and insns[-1].address == BODY_RETF and insns[-1].op_str == "6"
          and not any("call" in i.mnemonic for i in insns), "U68 +0109 differs")
    conditional = {i.address: (i.address + i.size, int(i.op_str, 16)) for i in insns if i.mnemonic.startswith("j") and i.mnemonic != "jmp"}
    sha = sha256(code[ENTRY:END]).hexdigest()
    print(f"U68 +0109 (301/125, call-free, RETF 6) GAME file {u['ovr_offset'] + ENTRY:#x} sha {sha}")
    return dict(conditional=conditional, starts=[i.address for i in insns], decoded={i.address: i for i in insns}, sha=sha,
                file_offset=u["ovr_offset"] + ENTRY), code, image


def library(directory):
    path = directory / "u680109.so"
    subprocess.run(["cc", "-std=c11", "-O2", "-fPIC", "-shared", "-I", str(ROOT / "include"),
                    str(ROOT / "src/recovered/overlay_u68_map_update_0109_cpu.c"), "-o", str(path)], check=True, capture_output=True, text=True)
    function = ctypes.CDLL(str(path)).pr_u68_map_update_0109_cpu
    function.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t, ctypes.POINTER(Cpu)]
    function.restype = ctypes.c_int
    return function


def build(code, image, rng, *, x, y, s, base, sp=0x4000, ss=SS, flags=0x0602, background=None):
    memory = bytearray([0xA5]) * SIZE if background is None else bytearray(background)
    memory[0:len(image)] = image
    at = physical(CALLER_CS, 0); memory[at:at + len(code)] = code
    put16(memory, DS, 0x6A5C, base[1]); put16(memory, DS, 0x6A5E, base[0])
    for i in range(0x100):
        memory[physical(base[0], (base[1] + 0x300 + i) & 0xFFFF)] = rng.randrange(256)
    frame = physical(ss, sp)
    for i, value in enumerate((RET_IP, RET_CS, (rng.randrange(256) << 8) | s, (rng.randrange(256) << 8) | y, (rng.randrange(256) << 8) | x)):
        put16(memory, ss, (sp + 2 * i) & 0xFFFF, value)
        put16(memory, 0, (frame + 2 * i) & (SIZE - 1), value)
    reg = tuple(rng.randrange(0x10000) for _ in range(7))
    return memory, Cpu(*reg, sp, CALLER_CS, ENTRY, DS, rng.randrange(0x10000), ss, flags), ss >= 0xF000


def compare(label, memory, cpu, high, target, function):
    from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
    from unicorn.x86_const import UC_X86_REG_CS, UC_X86_REG_IP
    original = Uc(UC_ARCH_X86, UC_MODE_16)
    original.mem_map(0, 2 * SIZE)
    original.mem_write(0, bytes(memory)); original.mem_write(SIZE, bytes(memory))
    for (name, _), register in zip(Cpu._fields_, REGISTERS):
        original.reg_write(register, getattr(cpu, name))
    state = dict(done=False, returned=False, unexpected=None, previous=None)
    visited, edges = set(), set()

    def hook(emulator, address, size, user):
        del address, size, user
        cs, ip = emulator.reg_read(UC_X86_REG_CS), emulator.reg_read(UC_X86_REG_IP)
        if state["returned"]:
            state["done"] = True
            return emulator.emu_stop()
        if cs != CALLER_CS or ip not in target["decoded"]:
            state["unexpected"] = (cs, ip)
            return emulator.emu_stop()
        visited.add(ip)
        if state["previous"] in target["conditional"] and state["previous"] != ip:
            edges.add((state["previous"], ip))
        state["previous"] = ip
        state["returned"] = ip == BODY_RETF
        return None

    def mirror(emulator, access, address, size, value, user):
        del access, user
        for byte in range(size):
            raw = address + byte; at = raw & (SIZE - 1)
            emulator.mem_write(at + SIZE if raw < SIZE else at, bytes([(value >> (8 * byte)) & 0xFF]))

    original.hook_add(UC_HOOK_CODE, hook)
    if high:
        original.hook_add(UC_HOOK_MEM_WRITE, mirror)
    original.emu_start(physical(CALLER_CS, ENTRY), 2 * SIZE, count=10000)
    check(state["unexpected"] is None and state["done"], f"original did not return cleanly ({state['unexpected']}): {label}")
    expected = bytes(original.mem_read(0, SIZE))
    ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
    check(function(ram, SIZE, ctypes.byref(cpu)) == 0, f"C status: {label}")
    actual = bytes(ram)
    if actual != expected:
        differences = [i for i, (a, b) in enumerate(zip(actual, expected)) if a != b]
        raise ValueError(f"RAM mismatch {label}: {[(hex(d), actual[d], expected[d]) for d in differences[:6]]} ({len(differences)} bytes)")
    for (name, _), register in zip(Cpu._fields_, REGISTERS):
        mine, want = getattr(cpu, name), original.reg_read(register) & 0xFFFF
        check(mine == want, f"{name} mismatch {label}: C={mine:04x}, original={want:04x}")
    return visited, edges


def cases(rng):
    spec = []
    coords = (0, 1, 7, 14, 15, 16, 0x7F, 0x80, 0xFE, 0xFF)
    for i in range(240):
        base = (rng.randrange(0x3000, 0x7000), rng.choice((rng.randrange(0x0000, 0xF000), rng.randrange(0xFC00, 0x10000))))
        spec.append((f"random-{i}", dict(x=rng.choice(coords + (rng.randrange(16),) * 4), y=rng.choice(coords + (rng.randrange(16),) * 4),
                                         s=rng.choice((0, 2, 4, 6, 0, 2, 4, 6, 1, 3, 5, 7, 8, 0xFF, rng.randrange(256))),
                                         base=base, flags=FLAGS[i % len(FLAGS)], sp=rng.randrange(0x0400, 0x7000) & ~1)))
    for s in (0, 2, 4, 6):
        spec.append((f"corner-{s}", dict(x=15, y=15, s=s, base=(0x4000, 0xFD00))))
    spec.append(("ss-a20", dict(x=3, y=4, s=6, base=(0x4000, 0x0100), sp=0x0400, ss=0xFFFF)))
    spec.append(("df-set", dict(x=3, y=4, s=2, base=(0x4000, 0x0100), flags=0x0603)))
    spec.append(("random-background", dict(x=9, y=2, s=4, base=(0x4000, 0x0100), background=bytes(random.Random(0x680109).randbytes(SIZE)))))
    return spec


def differential(target, code, image, function):
    rng = random.Random(0x680109)
    visited, edges, count = set(), set(), 0
    for label, options in cases(rng):
        memory, cpu, high = build(code, image, rng, **options)
        v, e = compare(f"+0109 {label}", memory, cpu, high, target, function)
        visited |= v; edges |= e; count += 1
    reachable = {(s, o) for s, outs in target["conditional"].items() for o in outs}
    check(set(target["starts"]) <= visited and reachable <= edges, f"coverage incomplete: {sorted(set(target['starts']) - visited)} {sorted(reachable - edges)}")
    print(f"U68 +0109: {count} original-byte cases; full 1 MiB RAM and all 14 CPU words equal after RETF 6, no mask; "
          f"{len(visited)}/{len(target['starts'])} instruction starts, {len(edges & reachable)}/{len(reachable)} conditional outcomes")
    return dict(cases=count, starts=len(visited), outcomes=len(edges & reachable))


# ---- DOSBox-X synthetic oracle -------------------------------------------------------------------------
CALLER_N, DATA_N = 0x8800, 0x5000
MAIN_AT, CASES_AT, CODE_AT = 0x0300, 0x0C00, 0x2000
OUTPUT, HEADER, STACK_TOP = 0xC000, 0xD800, 0xF000
WINDOW_LOW, WINDOW_SIZE = STACK_TOP - 0x40, 0x40
NATIVE_CASES, CASE_SIZE = 128, 12
RECORD_BYTES = 28 + WINDOW_SIZE + 0x10000
NATIVE_FLAGS = (0x0002, 0x00D7, 0x08D7, 0x0402, 0x0893, 0x0003)
ENTRY_REGS = dict(ax=0x1234, bx=0x3456, cx=0x5678, dx=0x789A, si=0x9ABC, di=0xBCDE, bp=0x7777)


def native_audit(code, function):
    directory = ROOT / "resources/oracle-runs/u68-0109-synthetic"
    directory.mkdir(parents=True, exist_ok=True)
    unit = bytes(code)
    rng = random.Random(0x680109D)
    cases = [dict(flags=n % len(NATIVE_FLAGS), x=rng.choice((0, 5, 15, 16, 0xFF, rng.randrange(16))), y=rng.choice((0, 9, 15, 16, 0x80, rng.randrange(16))),
                  s=rng.choice((0, 2, 4, 6, 1, 7, 0xFF)), base=rng.choice((rng.randrange(0x7000, 0xC000), 0xFD80)), fill=rng.randrange(256))
             for n in range(NATIVE_CASES)]
    table = b"".join(struct.pack("<BBBBHH4x", c["flags"], c["x"], c["y"], c["s"], c["base"], c["fill"]) for c in cases)
    check(len(table) == CASE_SIZE * len(cases), "native case size differs from the COM stride")
    check(CASES_AT + len(table) <= CODE_AT and CODE_AT + len(unit) <= OUTPUT, "COM layout overlaps")
    t = [".code16", ".intel_syntax noprefix", ".global _start", "_start:", "    jmp main", f".org {MAIN_AT - 0x100:#x}", "main:",
         "    mov ax,cs", "    mov ds,ax", "    mov es,ax", "    mov ss,ax", f"    mov sp,{STACK_TOP:#x}", "    cld",
         f"    mov ax,{CALLER_N:#x}", "    mov es,ax", "    mov si,offset unit", "    xor di,di", f"    mov cx,{len(unit):#x}", "    rep movsb",
         "    push cs", "    pop es", f"    mov word ptr [{HEADER:#x}],cs", f"    mov word ptr [{HEADER + 2:#x}],offset returned",
         f"    mov word ptr [{HEADER + 4:#x}],{len(cases)}", f"    mov word ptr [{HEADER + 6:#x}],0",
         "    mov dx,offset filename", "    xor cx,cx", "    mov ah,0x3c", "    int 0x21", "    jc failure",
         "    mov word ptr [handle],ax", "    mov bx,ax", f"    mov dx,{HEADER:#x}", "    mov cx,8", "    mov ah,0x40", "    int 0x21", "    jc failure",
         "again:", "    cli", f"    mov sp,{STACK_TOP:#x}", "    cld", "    push cs", "    pop ds", "    push cs", "    pop es",
         "    mov al,0xa5", f"    mov di,{WINDOW_LOW:#x}", f"    mov cx,{WINDOW_SIZE:#x}", "    rep stosb",
         "    mov ax,word ptr [index]", f"    mov bx,{CASE_SIZE}", "    mul bx", "    add ax,offset cases", "    mov bx,ax",
         f"    mov ax,{DATA_N:#x}", "    mov es,ax", "    xor di,di", "    mov cx,0x8000", "    mov ax,0xa5a5", "    rep stosw",
         "    mov di,word ptr [bx+4]", "    mov word ptr es:[0x6a5c],di", f"    mov word ptr es:[0x6a5e],{DATA_N:#x}",
         "    add di,0x300", "    mov al,byte ptr [bx+6]", "    mov cx,0x100", "fill_loop:", "    stosb", "    add al,0x3b", "    loop fill_loop",
         "    xor ah,ah", "    mov al,byte ptr [bx+1]", "    or ax,0x5a00", "    push ax", "    mov al,byte ptr [bx+2]", "    push ax",
         "    mov al,byte ptr [bx+3]", "    push ax",
         "    xor ah,ah", "    mov al,byte ptr [bx]", "    shl ax,1", "    mov si,ax",
         "    push word ptr [flags_table+si]", "    push cs", "    pop es", f"    mov ax,{DATA_N:#x}", "    mov ds,ax"]
    for reg, value in ENTRY_REGS.items():
        t.append(f"    mov {reg},{value:#x}")
    t += ["    popf", "    call dword ptr cs:[target_far]", "returned:"]
    for i, reg in enumerate("ax bx cx dx si di bp sp cs".split()):
        t.append(f"    mov word ptr cs:[{OUTPUT + 2 * i:#x}],{reg}")
    t += [f"    mov word ptr cs:[{OUTPUT + 18:#x}],offset returned", f"    mov word ptr cs:[{OUTPUT + 20:#x}],ds",
          f"    mov word ptr cs:[{OUTPUT + 22:#x}],es", f"    mov word ptr cs:[{OUTPUT + 24:#x}],ss",
          f"    mov sp,{STACK_TOP + 0x800:#x}", "    pushf", "    pop ax", f"    mov word ptr cs:[{OUTPUT + 26:#x}],ax",
          "    cld", "    push cs", "    pop ds", "    push cs", "    pop es", f"    mov di,{OUTPUT + 28:#x}",
          f"    mov si,{WINDOW_LOW:#x}", f"    mov cx,{WINDOW_SIZE:#x}", "    rep movsb", "    sti",
          "    mov bx,word ptr [handle]", f"    mov dx,{OUTPUT:#x}", f"    mov cx,{28 + WINDOW_SIZE}", "    mov ah,0x40", "    int 0x21", "    jc failure",
          f"    mov ax,{DATA_N:#x}", "    mov ds,ax", "    xor dx,dx", "    mov cx,0x8000", "    mov ah,0x40", "    int 0x21", "    jc failure",
          "    mov dx,0x8000", "    mov cx,0x8000", "    mov ah,0x40", "    int 0x21", "    jc failure", "    push cs", "    pop ds",
          "    inc word ptr [index]", "    mov ax,word ptr [index]", f"    cmp ax,{len(cases)}", "    jb again",
          "    mov bx,word ptr [handle]", "    mov ah,0x3e", "    int 0x21", "    mov ax,0x4c00", "    int 0x21",
          "failure:", "    mov ax,0x4c01", "    int 0x21",
          'filename: .asciz "RESULT.BIN"', "handle: .word 0", "index: .word 0", f"target_far: .word {ENTRY:#x}, {CALLER_N:#x}",
          "flags_table: .word " + ",".join(f"{f:#x}" for f in NATIVE_FLAGS)]
    for label, blob, where in (("cases", table, CASES_AT), ("unit", unit, CODE_AT)):
        t += [f".org {where - 0x100:#x}", f"{label}:"]
        for i in range(0, len(blob), 16):
            t.append("    .byte " + ",".join(f"{b:#x}" for b in blob[i:i + 16]))
    asm, obj = directory / "oracle.S", directory / "oracle.o"
    asm.write_text("\n".join(t) + "\n")
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
    stores = 0
    for n, case in enumerate(cases):
        record = result[8 + n * RECORD_BYTES:8 + (n + 1) * RECORD_BYTES]
        memory = bytearray(SIZE)
        at = physical(CALLER_N, 0); memory[at:at + len(unit)] = unit
        data = bytearray([0xA5]) * 0x10000
        data[0x6A5C:0x6A60] = struct.pack("<HH", case["base"], DATA_N)
        value = case["fill"]
        for i in range(0x100):
            data[(case["base"] + 0x300 + i) & 0xFFFF] = value; value = (value + 0x3B) & 0xFF
        at = physical(DATA_N, 0); memory[at:at + 0x10000] = data
        for i in range(WINDOW_SIZE):
            memory[physical(segment, WINDOW_LOW + i)] = 0xA5
        sp = STACK_TOP
        for word in (0x5A00 | case["x"], 0x5A00 | case["y"], 0x5A00 | case["s"], segment, returned):
            sp -= 2; put16(memory, segment, sp, word)
        cpu = Cpu(*ENTRY_REGS.values(), sp, CALLER_N, ENTRY, DATA_N, segment, segment, NATIVE_FLAGS[case["flags"]])
        ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
        check(function(ram, SIZE, ctypes.byref(cpu)) == 0, f"C status on native case {n}")
        for (reg, _), want in zip(Cpu._fields_, struct.unpack_from("<14H", record)):
            check(getattr(cpu, reg) == want, f"native case {n} {reg}: C={getattr(cpu, reg):04x}, DOS={want:04x}")
        got = bytes(ram[physical(segment, WINDOW_LOW):physical(segment, WINDOW_LOW) + WINDOW_SIZE]) + bytes(ram[physical(DATA_N, 0):physical(DATA_N, 0) + 0x10000])
        if got != record[28:]:
            differences = [i for i, (a, b) in enumerate(zip(got, record[28:])) if a != b]
            raise ValueError(f"native case {n} bytes differ at {[hex(d) for d in differences[:8]]} ({len(differences)} bytes)")
        stores += got[WINDOW_SIZE:] != bytes(data)
    digest = sha256(result).hexdigest()
    print(f"DOSBox-X normal-core/386 synthetic cases passed: {len(cases)} ({stores} storing); all 14 CPU words, the stack window and the "
          f"whole 64 KiB data segment; no mask")
    print(f"native result SHA-256 {digest}; COM segment {segment:04x}; U68 at {CALLER_N:04X}")
    return dict(cases=len(cases), stores=stores, result_sha256=digest)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--unicorn", action="store_true")
    parser.add_argument("--native", action="store_true")
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    target, code, image = source()
    report = dict(target=dict(sha256=target["sha"], bytes=301, instructions=125, file_offset=target["file_offset"]))
    if args.unicorn or args.native:
        with tempfile.TemporaryDirectory(prefix="poolrad-u680109-") as temp:
            function = library(Path(temp))
            if args.unicorn:
                report["unicorn"] = differential(target, code, image, function)
            if args.native:
                report["native"] = native_audit(code, function)
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(json.dumps(report, sort_keys=True, indent=2) + "\n")
    print(json.dumps(report, sort_keys=True))


if __name__ == "__main__":
    main()
