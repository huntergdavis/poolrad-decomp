#!/usr/bin/env python3
"""Original-byte full-state parity for u33 +02BA and +0377 with the +0221 writer composed.

Both bodies stamp glyphs through +0221 at positions relative to their caller's
frame (the static link at [SP+4]). The Unicorn differential loads the original
unit 0x33 code into one 1 MiB image, runs the original and the composed C on a
copy, and compares the entire RAM and all 14 CPU words. The DOSBox-X synthetic
oracle runs the same bodies from a COM whose unit 0x33 copy keeps every
PUSH CS/CALL displacement. Media stays private.
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

U33_BASE = 0xC5D1
CALLER_CS, DS, SS = 0x3000, 0x1000, 0x2000
TARGETS = {
    "02ba": dict(entry=0x02BA, end=0x0377, insns=86, retf=0x0374, conditionals=5, symbol="pr_u33_stamp_block_02ba",
                 calls=[0x02E1, 0x0315, 0x0327, 0x033C, 0x035E, 0x036E], callers={(0x0810, 0x087E)}),
    "0377": dict(entry=0x0377, end=0x040F, insns=82, retf=0x040C, conditionals=1, symbol="pr_u33_stamp_square_0377",
                 calls=[0x0394, 0x03A4, 0x03B4, 0x03C4, 0x03D6, 0x03E6, 0x03F6, 0x0406], callers={(0x0810, 0x0883)}),
}


def check(condition, message):
    if not condition:
        raise ValueError(message)


def source():
    exe = (ROOT / "resources/DOS/POOLRAD/START.EXE").read_bytes()
    ovr = (ROOT / "resources/DOS/POOLRAD/GAME.OVR").read_bytes()
    check(len(exe) == 65231 and sha256(exe).hexdigest() == EXE_SHA, "START.EXE provenance differs")
    check(len(ovr) == 206105 and sha256(ovr).hexdigest() == OVR_SHA, "GAME.OVR provenance differs")
    units = {u["stub_para"]: u for u in find_units(exe[parse_mz(exe)["header_bytes"]:], ovr)}
    u33 = units[0x33]
    check(u33["ovr_offset"] == U33_BASE, "unit 0x33 placement differs")
    code = ovr[U33_BASE:U33_BASE + u33["code_size"]]
    starts = sorted(f["rel"] for f in u33["functions"])
    decoder = Cs(CS_ARCH_X86, CS_MODE_16)
    bodies = {}
    for key, target in TARGETS.items():
        body = code[target["entry"]:target["end"]]
        decoded = list(decoder.disasm(body, target["entry"]))
        check(len(decoded) == target["insns"] and decoded[-1].address == target["retf"] and decoded[-1].mnemonic == "retf"
              and int(decoded[-1].op_str, 16) == 2 and decoded[-1].address + decoded[-1].size == target["end"]
              and target["end"] in starts, f"+{key} extent differs")
        calls = [i for i in decoded if "call" in i.mnemonic or i.mnemonic.startswith("int")]
        check([i.address for i in calls] == target["calls"] and all(i.op_str == "0x221" for i in calls), f"+{key} calls differ")
        check(all(decoded[decoded.index(i) - 1].op_str == "cs" for i in calls), f"+{key} near call lacks PUSH CS")
        conditional = {i.address: (i.address + i.size, int(i.op_str, 16)) for i in decoded
                       if i.mnemonic.startswith("j") and i.mnemonic != "jmp"}
        check(len(conditional) == target["conditionals"], f"+{key} conditional count differs")
        sites = {i.address for i in decoded}
        check(all(int(i.op_str, 16) in sites for i in decoded if i.mnemonic.startswith("j")), f"+{key} branch leaves the body")
        near = set()
        for function in starts:
            end = next((x for x in starts if x > function), len(code))
            aligned = {i.address: i for i in decoder.disasm(code[function:end], function)}
            for site, ins in aligned.items():
                if ins.mnemonic == "call" and ins.op_str.startswith("0x") and int(ins.op_str, 16) == target["entry"]:
                    check(aligned[site - 1].op_str == "cs", f"+{key} caller lacks PUSH CS")
                    near.add((function, site))
        check(near == target["callers"], f"+{key} aligned near callers differ: {sorted(near)}")
        bodies[key] = dict(body=body, decoded=decoded, conditional=conditional, sha=sha256(body).hexdigest(), **target)
    writer = code[0x0221:0x02BA]
    print(f"u33 +02BA (189/86, six CALL +0221) sha {bodies['02ba']['sha'][:16]}...; +0377 (152/82, eight CALL +0221) "
          f"sha {bodies['0377']['sha'][:16]}...; +0221 writer {sha256(writer).hexdigest()[:16]}...")
    print("aligned PUSH CS/CALL callers:", {k: sorted(f"+{f:04X}@+{s:04X}" for f, s in v["callers"]) for k, v in bodies.items()})
    return bodies, code


def library(directory):
    path = directory / "u33.so"
    subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-O2", "-fPIC", "-shared",
                    "-I", str(ROOT / "include"),
                    *[str(ROOT / "src/recovered" / s) for s in ("overlay_u33_stamp_block_02ba.c", "overlay_u33_stamp_square_0377.c",
                                                               "overlay_u33_clipped_writer_0221.c")],
                    "-o", str(path)], check=True, capture_output=True, text=True)
    lib = ctypes.CDLL(str(path))
    functions = {}
    for key, target in TARGETS.items():
        f = getattr(lib, target["symbol"])
        f.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t, ctypes.POINTER(Cpu)]
        f.restype = ctypes.c_int
        functions[key] = f
    return functions


def fixture(code, key, *, seed=1, link=0x8100, link_bytes=None, ss=SS, sp=0x8000, ds=DS, es=0x1111, flags=0x7202,
            grid_at=(0x5000, 0x0000), background=None, high_byte=0x5A, regs=None):
    rng = random.Random(seed)
    memory = bytearray([0xA5]) * SIZE if background is None else bytearray(background)
    memory[physical(CALLER_CS, 0):physical(CALLER_CS, 0) + len(code)] = code
    put16(memory, ds, 0x6716, grid_at[1]); put16(memory, ds, 0x6718, grid_at[0])
    for i in range(0x4F0):
        memory[physical(grid_at[0], (grid_at[1] + i) & 0xFFFF)] = rng.randrange(256)
    values = link_bytes if link_bytes is not None else [rng.randrange(256) for _ in range(8)]
    for i, value in enumerate(values):                        # SS:[L-8] .. SS:[L-1]
        memory[physical(ss, (link - 8 + i) & 0xFFFF)] = value & 0xFF
    words = (0x7000, 0x7001, link)
    frame = physical(ss, sp)
    for i, value in enumerate(words):
        put16(memory, ss, (sp + 2 * i) & 0xFFFF, value)
        memory[(frame + 2 * i) & (SIZE - 1)] = value & 0xFF
        memory[(frame + 2 * i + 1) & (SIZE - 1)] = value >> 8
    r = regs or (0x1234, 0x3456, 0x5678, (high_byte << 8) | 0x9A, 0x9ABC, 0xBCDE, 0x7777)
    cpu = Cpu(r[0] & 0x00FF | (high_byte << 8), r[1], r[2], r[3], r[4], r[5], r[6], sp, CALLER_CS, TARGETS[key]["entry"],
              ds, es, ss, flags)
    high = any(seg >= 0xF000 for seg in (ss, ds, grid_at[0]))
    return memory, cpu, high


def compare(label, memory, cpu, high, bodies, key, function, budget=200000):
    from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
    target = bodies[key]
    original = Uc(UC_ARCH_X86, UC_MODE_16)
    original.mem_map(0, 2 * SIZE)
    original.mem_write(0, bytes(memory)); original.mem_write(SIZE, bytes(memory))
    for (name, _), register in zip(Cpu._fields_, REGISTERS):
        original.reg_write(register, getattr(cpu, name))
    start = physical(cpu.cs, target["entry"]); body_end = start + len(target["body"]); last = physical(cpu.cs, target["retf"])
    writer = (physical(CALLER_CS, 0x0221), physical(CALLER_CS, 0x02BA))
    returned, previous, visited, edges = [False], [None], set(), set()

    def instruction(emulator, address, size, user):
        del size, user
        if returned[0]:
            emulator.emu_stop(); return
        linear = address & (2 * SIZE - 1)
        if start <= linear < body_end:
            here = linear - start + target["entry"]
            visited.add(here)
            if previous[0] in target["conditional"]:
                edges.add((previous[0], here))
            previous[0] = here
            if linear == last:
                returned[0] = True
            return
        previous[0] = None
        check(writer[0] <= linear < writer[1], f"left the legal code: {label} at {linear:#x}")

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
    rng = random.Random({"02ba": 0x3302BA, "0377": 0x330377}[key])
    spec = []
    for i in range(120):
        link = [rng.randrange(256) for _ in range(8)]
        link[0] = rng.choice((1, 3, 0, 2, 0xFF, rng.randrange(256)))        # L-8
        link[1] = rng.choice((1, 0, 2, 0xFF, rng.randrange(256)))           # L-7
        link[6] = rng.choice((0, 1, 2, 0xFF, 0xFE, 4, 9, rng.randrange(256)))  # L-2
        link[7] = rng.choice((0, 1, 2, 0xFF, 0xFE, 4, 7, rng.randrange(256)))  # L-1
        spec.append((f"random-{i}", dict(seed=rng.randrange(1 << 30), link_bytes=link, flags=rng.choice((0x7202, 0x0202, 0x0ED7, 0x0647)),
                                        es=rng.randrange(0x10000), high_byte=rng.randrange(256),
                                        regs=tuple(rng.randrange(0x10000) for _ in range(7)))))
    for selector in (1, 3, 0, 2):
        for column_y in ((0, 0), (1, 1), (0xFF, 0xFF), (9, 7), (0xF8, 4)):
            link = [selector, 1 if selector == 1 else 2, 0, 0, 0, 0, column_y[0], column_y[1]]
            spec.append((f"selector-{selector}-{column_y[0]:02x}{column_y[1]:02x}", dict(seed=selector * 7 + column_y[0], link_bytes=link)))
    base = dict(seed=77, link_bytes=[1, 1, 0, 0, 0, 0, 0, 0])
    for link in (0x0004, 0x0001, 0xFFFF, 0x0008):
        spec.append((f"link-{link:04x}", dict(base, link=link)))
    for sp in (0x0000, 0x0010, 0x0040, 0xFFF0, 0xFFFC, 0xFFFE):
        spec.append((f"sp-{sp:04x}", dict(base, sp=sp, ss=0x1800, link=(sp + 0x100) & 0xFFFF, background=bytes(SIZE))))
    spec.append(("ss-a20", dict(base, ss=0xFFFF, sp=0x0400, link=0x0500)))
    spec.append(("ds-a20", dict(base, ds=0xFFFF)))
    spec.append(("grid-a20", dict(base, grid_at=(0xFFFF, 0x0010))))
    spec.append(("grid-offset-wrap", dict(base, grid_at=(0x5000, 0xFE00))))
    # The first stamp of either body lands on the caller's column byte [L-2] (column 0Ah + 15h, row 0Ah, fx = fy = 0
    # gives cell 0Ah*32h + 15h + 7), so every later position follows the rewritten byte.
    for key_column, row in (("02ba", 12), ("0377", 10)):
        column = 0x15 + (0 if key_column == "02ba" else 3)
        spec.append((f"grid-over-link-{key_column}", dict(base, link=0x8100, grid_at=(SS, (0x8100 - 2 - (row * 50 + column + 7)) & 0xFFFF))))
    spec.append(("random-background", dict(base, background=bytes(random.Random(5).randbytes(SIZE)))))
    spec.append(("preserved-df", dict(base, flags=0x0603)))
    return spec


def differential(bodies, code, key, functions):
    visited, edges, count = set(), set(), 0
    target = bodies[key]
    for label, options in cases(key):
        memory, cpu, high = fixture(code, key, **options)
        v, e = compare(f"+{key} {label}", memory, cpu, high, bodies, key, functions[key])
        visited |= v; edges |= e; count += 1
    missing_sites = sorted(set(i.address for i in target["decoded"]) - visited)
    missing_edges = sorted({(s, o) for s, outs in target["conditional"].items() for o in outs} - edges)
    check(not missing_sites and not missing_edges, f"+{key} coverage incomplete: {missing_sites} {missing_edges}")
    print(f"+{key}: {count} original-byte cases passed, full 1 MiB RAM and all 14 CPU words equal; "
          f"{len(visited)}/{len(target['decoded'])} instruction starts, {len(edges)}/{2 * len(target['conditional'])} conditional outcomes")
    return dict(returning=count, starts=len(visited), outcomes=len(edges))


# ---- DOSBox-X synthetic oracle -------------------------------------------------------------------------
MAIN_AT, FIXTURES_AT, U33_AT, GRID = 0x0300, 0x0C00, 0x1000, 0x8000
OUTPUT, HEADER, STACK_TOP = 0xA000, 0xA800, 0xF000
LINK = STACK_TOP + 0x20
WINDOW = (STACK_TOP - 0x60, 0x80)                 # [EFA0, F020): frames below SP and the link bytes above
GRID_BYTES = 0x4F0
FIXTURE_SIZE = 12
RECORD_BYTES = 28 + WINDOW[1] + GRID_BYTES
FLAGS = (0x0002, 0x00D7, 0x08D7, 0x0402, 0x0893, 0x0003)
ENTRY_REGS = dict(ax=0x1234, bx=0x3456, cx=0x5678, dx=0x789A, si=0x9ABC, di=0xBCDE, bp=0x7777)


def native_cases():
    rng = random.Random(0x3302BB)
    cases = []
    for i in range(80):
        which = i % 2
        link = [rng.choice((1, 3, 0, 2)), rng.choice((1, 0, 2)), *[rng.randrange(256) for _ in range(4)],
                rng.choice((0, 1, 0xFF, 2, 4)), rng.choice((0, 1, 0xFF, 2, 7))]
        cases.append(dict(which=which, link=link, grid_seed=rng.randrange(256), flags=i % len(FLAGS)))
    return cases


def assembly(cases):
    text = [".code16", ".intel_syntax noprefix", ".global _start", "_start:", "    jmp main",
            f".org {MAIN_AT - 0x100:#x}", "main:",
            "    mov ax,cs", "    mov ds,ax", "    mov es,ax", "    mov ss,ax", f"    mov sp,{STACK_TOP:#x}",
            f"    mov word ptr [0x6716],{GRID:#x}", "    mov word ptr [0x6718],cs",
            f"    mov word ptr [{HEADER:#x}],cs", f"    mov word ptr [{HEADER + 2:#x}],offset returned",
            f"    mov word ptr [{HEADER + 4:#x}],{len(cases)}", "    mov word ptr [" + f"{HEADER + 6:#x}" + "],0",
            "    mov dx,offset filename", "    xor cx,cx", "    mov ah,0x3c", "    int 0x21", "    jc failure",
            "    mov word ptr [handle],ax", "    mov bx,ax", f"    mov dx,{HEADER:#x}", "    mov cx,8",
            "    mov ah,0x40", "    int 0x21", "    jc failure",
            "again:", "    cli", f"    mov sp,{STACK_TOP:#x}", "    cld", "    push cs", "    pop es",
            "    mov al,0xa5", f"    mov di,{WINDOW[0]:#x}", f"    mov cx,{WINDOW[1]}", "    rep stosb",
            "    mov ax,word ptr [index]", f"    mov bx,{FIXTURE_SIZE}", "    mul bx", "    add ax,offset fixtures", "    mov si,ax",
            # the caller frame bytes SS:[L-8..L-1]
            "    push si", "    add si,1", f"    mov di,{LINK - 8:#x}", "    mov cx,8", "    rep movsb", "    pop si",
            # the grid pattern: grid_seed + 7*i
            "    mov al,byte ptr [si+9]", f"    mov di,{GRID:#x}", f"    mov cx,{GRID_BYTES:#x}", "grid_loop:",
            "    mov byte ptr [di],al", "    add al,7", "    inc di", "    loop grid_loop",
            "    mov ax,offset target_02ba", "    cmp byte ptr [si],0", "    je chosen", "    mov ax,offset target_0377",
            "chosen:", "    mov bx,ax", "    mov ax,word ptr [bx]", "    mov word ptr [target],ax",
            "    mov bl,byte ptr [si+10]", "    xor bh,bh", "    shl bx,1", "    push word ptr [flags_table+bx]", "    popf"]
    for name, value in ENTRY_REGS.items():
        text.append(f"    mov {name},{value:#x}")
    text += [f"    push {LINK:#x}", "    push cs", "    call word ptr [target]", "returned:"]
    for i, reg in enumerate("ax bx cx dx si di bp sp cs".split()):
        text.append(f"    mov word ptr [{OUTPUT + 2 * i:#x}],{reg}")
    text += [f"    mov word ptr [{OUTPUT + 18:#x}],offset returned", f"    mov word ptr [{OUTPUT + 20:#x}],ds",
             f"    mov word ptr [{OUTPUT + 22:#x}],es", f"    mov word ptr [{OUTPUT + 24:#x}],ss",
             f"    mov sp,{STACK_TOP + 0x800:#x}", "    pushf", "    pop ax", f"    mov word ptr [{OUTPUT + 26:#x}],ax",
             "    cld", "    push cs", "    pop ds", "    push cs", "    pop es",
             f"    mov si,{WINDOW[0]:#x}", f"    mov di,{OUTPUT + 28:#x}", f"    mov cx,{WINDOW[1]}", "    rep movsb",
             f"    mov si,{GRID:#x}", f"    mov cx,{GRID_BYTES:#x}", "    rep movsb",
             "    sti", "    mov bx,word ptr [handle]", f"    mov dx,{OUTPUT:#x}", f"    mov cx,{RECORD_BYTES}",
             "    mov ah,0x40", "    int 0x21", "    jc failure", f"    cmp ax,{RECORD_BYTES}", "    jne failure",
             "    inc word ptr [index]", "    mov ax,word ptr [index]", f"    cmp ax,{len(cases)}", "    jb again",
             "    mov bx,word ptr [handle]", "    mov ah,0x3e", "    int 0x21", "    mov ax,0x4c00", "    int 0x21",
             "failure:", "    mov ax,0x4c01", "    int 0x21",
             'filename: .asciz "RESULT.BIN"', "handle: .word 0", "index: .word 0", "target: .word 0",
             f"target_02ba: .word {U33_AT + 0x02BA:#x}", f"target_0377: .word {U33_AT + 0x0377:#x}",
             "flags_table: .word " + ",".join(f"{f:#x}" for f in FLAGS),
             f".org {FIXTURES_AT - 0x100:#x}", "fixtures:"]
    for case in cases:
        row = [case["which"], *case["link"], case["grid_seed"], case["flags"], 0]
        check(len(row) == FIXTURE_SIZE, "fixture row size")
        text.append("    .byte " + ",".join(str(v) for v in row))
    return "\n".join(text) + "\n"


def native_audit(code, functions):
    directory = ROOT / "resources/oracle-runs/u33-02ba-0377-synthetic"
    directory.mkdir(parents=True, exist_ok=True)
    cases = native_cases()
    asm, obj = directory / "oracle.S", directory / "oracle.o"
    asm.write_text(assembly(cases))
    subprocess.run(["as", "--32", str(asm), "-o", str(obj)], check=True)
    subprocess.run(["ld", "-m", "elf_i386", "-Ttext", "0x100", "--oformat", "binary", str(obj), "-o", str(directory / "main.bin")], check=True)
    assembled = (directory / "main.bin").read_bytes()
    check(0x100 + len(assembled) <= U33_AT, "harness reaches the unit 0x33 copy")
    image = bytearray(U33_AT + len(code) - 0x100)
    image[:len(assembled)] = assembled
    image[U33_AT - 0x100:] = code
    check(not (U33_AT <= 0x6716 < U33_AT + len(code)) and U33_AT + len(code) <= GRID, "COM layout overlaps a global")
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
                       env=environment, stdout=log, stderr=subprocess.STDOUT, check=True, timeout=180)
    result = result_path.read_bytes()
    check(len(result) == 8 + len(cases) * RECORD_BYTES, f"native output incomplete: {len(result)} bytes")
    segment, returned, count, _ = struct.unpack_from("<4H", result)
    check(count == len(cases), "native header differs")
    totals = {"02ba": 0, "0377": 0}
    for n, case in enumerate(cases):
        key = "02ba" if case["which"] == 0 else "0377"
        record = result[8 + n * RECORD_BYTES:8 + (n + 1) * RECORD_BYTES]
        memory = bytearray([0xA5]) * SIZE
        base = physical(segment, 0x100)
        memory[base:base + len(image)] = image
        put16(memory, segment, 0x6716, GRID); put16(memory, segment, 0x6718, segment)
        for i, value in enumerate(case["link"]):
            memory[physical(segment, LINK - 8 + i)] = value
        for i in range(GRID_BYTES):
            memory[physical(segment, GRID + i)] = (case["grid_seed"] + 7 * i) & 0xFF
        sp = STACK_TOP
        for value in (LINK, segment, returned):
            sp -= 2; put16(memory, segment, sp, value)
        cpu = Cpu(ENTRY_REGS["ax"], ENTRY_REGS["bx"], ENTRY_REGS["cx"], ENTRY_REGS["dx"], ENTRY_REGS["si"], ENTRY_REGS["di"],
                  ENTRY_REGS["bp"], sp, segment, U33_AT + TARGETS[key]["entry"], segment, segment, segment, FLAGS[case["flags"]])
        ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
        check(functions[key](ram, SIZE, ctypes.byref(cpu)) == 0, f"C status on native case {n}")
        actual = bytes(ram)
        for (name, _), want in zip(Cpu._fields_, struct.unpack_from("<14H", record)):
            check(getattr(cpu, name) == want, f"native case {n} (+{key}) {name}: C={getattr(cpu, name):04x}, DOS={want:04x}")
        offset = 28
        for start, length in (WINDOW, (GRID, GRID_BYTES)):
            at = physical(segment, start)
            if actual[at:at + length] != record[offset:offset + length]:
                diff = [i for i in range(length) if actual[at + i] != record[offset + i]]
                raise ValueError(f"native case {n} (+{key}) bytes at {start:#x}+{diff[:6]} differ")
            offset += length
        totals[key] += 1
    digest = sha256(result).hexdigest()
    print(f"DOSBox-X normal-core/386 synthetic cases passed: {totals}; all 14 CPU words, {WINDOW[1]} frame/link bytes and "
          f"{GRID_BYTES} grid bytes; no mask")
    print(f"native result SHA-256 {digest}; COM segment {segment:04x}")
    return dict(cases=totals, result_sha256=digest)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--unicorn", action="store_true")
    parser.add_argument("--native", action="store_true")
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    bodies, code = source()
    report = dict(targets={k: dict(sha256=v["sha"], bytes=len(v["body"]), instructions=v["insns"]) for k, v in bodies.items()})
    if args.unicorn or args.native:
        with tempfile.TemporaryDirectory(prefix="poolrad-u33-stamps-") as temp:
            functions = library(Path(temp))
            if args.unicorn:
                report["unicorn_cases"] = {k: differential(bodies, code, k, functions) for k in bodies}
            if args.native:
                report["native"] = native_audit(code, functions)
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(json.dumps(report, sort_keys=True, indent=2) + "\n")
    print(json.dumps(report, sort_keys=True))


if __name__ == "__main__":
    main()
