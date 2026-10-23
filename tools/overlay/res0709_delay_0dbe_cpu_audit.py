#!/usr/bin/env python3
"""Original-byte full-state parity for the raw-CPU CRT Delay 0A40:029C and the raw-CPU resident 0709:0DBE composing it.

The differential loads the original START.EXE spans 0A40:029C..02C8 and 0709:0DBE..0DD2 at their own addresses in one
1 MiB image and compares the entire RAM and all 14 CPU words between the original under Unicorn and the C models, with
no mask. Delay cases enter 029C after a far call; wrapper cases enter 0DBE. The original decides each stop: the fetch
after the entered routine's final RETF (return), the wrapper's LCALL fetched with other bytes (CODE), or a fetch of a
loop target after more taken backward jumps than the host cap (LIMIT, which the wrapper reports as CALLEE). Any fetch
outside the two spans is an error. Media stays private.
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
from resident_loader_map import unpack
from u86_procvars_3413_audit import Cpu, ROOT, SIZE, EXE_SHA, REGISTERS, physical, put16
from pinned_shift_flags import LastFlagWriter, SPLIT_BY_RULE, declared_flags_split, split_used
SPLIT_ARRIVALS = [0]

A40, DELAY, DELAY_END, DELAY_RETF = 0x0A40, 0x029C, 0x02C9, 0x02BA
R709, WRAP, WRAP_END, WRAP_RETF, SITE = 0x0709, 0x0DBE, 0x0DD3, 0x0DD2, 0x0DCC
LOOPS = {0x02C1: 0x02C0, 0x02C6: 0x02BD, 0x02B8: 0x02B0}
DS, SS = 0x3000, 0x4000
RET_CS, RET_IP = 0x9001, 0x7000
OK, CODE, CALLEE, LIMIT = 0, 2, 3, 5
NAMES = {OK: "return", CODE: "CODE", CALLEE: "CALLEE", LIMIT: "LIMIT"}
FLAGS = (0x0602, 0x0202, 0x0ED7, 0x0647, 0x72D7, 0x0403, 0x0213)


def check(condition, message):
    if not condition:
        raise ValueError(message)


def source():
    exe = (ROOT / "resources/DOS/POOLRAD/START.EXE").read_bytes()
    check(len(exe) == 65231 and sha256(exe).hexdigest() == EXE_SHA, "START.EXE provenance differs")
    image = bytes(unpack(exe)[0])
    md = Cs(CS_ARCH_X86, CS_MODE_16)
    delay = image[physical(A40, DELAY):physical(A40, DELAY_END)]
    wrap = image[physical(R709, WRAP):physical(R709, WRAP_END)]
    d = list(md.disasm(delay, DELAY)); w = list(md.disasm(wrap, WRAP))
    by = {i.address: i for i in d}
    check(len(d) == 19 and sum(i.size for i in d) == len(delay) and (by[DELAY_RETF].mnemonic, by[DELAY_RETF].op_str) == ("retf", "2")
          and d[-1].mnemonic == "ret" and all(by[a].mnemonic in ("jne", "loope") and int(by[a].op_str, 16) == t for a, t in LOOPS.items()),
          "A40 Delay differs")
    check(len(w) == 10 and sum(i.size for i in w) == len(wrap) and w[-1].address == WRAP_RETF and w[-1].mnemonic == "retf"
          and [(i.address, i.op_str) for i in w if "call" in i.mnemonic] == [(SITE, "0xa40, 0x29c")], "0709:0DBE differs")
    conditional = {(A40, i.address): (i.address + i.size, int(i.op_str, 16)) for i in d
                   if (i.mnemonic.startswith("j") and i.mnemonic != "jmp") or i.mnemonic.startswith("loop")}
    target = dict(conditional=conditional, starts=[(A40, i.address) for i in d] + [(R709, i.address) for i in w],
                  delay_sha=sha256(delay).hexdigest(), wrap_sha=sha256(wrap).hexdigest())
    print(f"A40 Delay 029C..02C8 (45/19, RETF 2 at 02BA) sha {target['delay_sha']}; 0709:0DBE..0DD2 (21/10, LCALL 0A40:029C, RETF) "
          f"sha {target['wrap_sha']}")
    return target, delay, wrap


CHECKER_ROWS = ('RES_0709_0DBE',)


def library(directory):
    path = directory / "delay.so"
    sources = ["resident_a40_delay_029c_cpu.c", "resident_0709_delay_wrapper_0dbe_cpu.c"]
    built = subprocess.run(["cc", "-std=c11", "-O2", "-fPIC", "-shared", "-I", str(ROOT / "include"),
                            *[str(ROOT / "src/recovered" / s) for s in sources], "-o", str(path)], capture_output=True, text=True)
    check(built.returncode == 0, built.stderr[-2000:])
    lib = ctypes.CDLL(str(path))
    delay, wrap = lib.pr_a40_delay_029c_cpu, lib.pr_r0709_delay_wrapper_0dbe_cpu
    delay.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t, ctypes.POINTER(Cpu), ctypes.c_uint64]
    wrap.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t, ctypes.POINTER(Cpu), ctypes.c_uint64, ctypes.POINTER(ctypes.c_int)]
    delay.restype = wrap.restype = ctypes.c_int
    return delay, wrap


def build(delay, wrap, rng, *, mode, ms=1, count=1, factor=1, bios=None, ds=DS, ss=SS, sp=0x8000, flags=0x7202, patch=(), background=None):
    memory = bytearray([0xA5]) * SIZE if background is None else bytearray(background)
    at = physical(A40, DELAY); memory[at:at + len(delay)] = delay
    at = physical(R709, WRAP); memory[at:at + len(wrap)] = wrap
    for offset, value in patch:
        memory[physical(R709, offset)] = value
    memory[0x400] = rng.randrange(256) if bios is None else bios
    put16(memory, ds, 0x84FC, count)
    memory[physical(ds, 0x49E2)] = factor
    frame = physical(ss, sp)
    words = (RET_IP, RET_CS, ms) if mode == "delay" else (RET_IP, RET_CS)
    for i, value in enumerate(words):
        put16(memory, ss, (sp + 2 * i) & 0xFFFF, value)
        put16(memory, 0, (frame + 2 * i) & (SIZE - 1), value)
    cs, ip = (A40, DELAY) if mode == "delay" else (R709, WRAP)
    cpu = Cpu(*(rng.randrange(0x10000) for _ in range(7)), sp, cs, ip, ds, rng.randrange(0x10000), ss, flags)
    return memory, cpu, ss >= 0xF000


def compare(label, mode, memory, cpu, high, target, functions, cap):
    from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
    original = Uc(UC_ARCH_X86, UC_MODE_16)
    original.mem_map(0, 2 * SIZE)
    original.mem_write(0, bytes(memory)); original.mem_write(SIZE, bytes(memory))
    for (name, _), register in zip(Cpu._fields_, REGISTERS):
        original.reg_write(register, getattr(cpu, name))
    spans = [(A40, physical(A40, DELAY), physical(A40, DELAY_END), DELAY)] + ([(R709, physical(R709, WRAP), physical(R709, WRAP_END), WRAP)]
                                                                            if mode == "wrapper" else [])
    final = (A40, DELAY_RETF) if mode == "delay" else (R709, WRAP_RETF)
    state = dict(stop=None, returned=False, previous=None, loopbacks=0, illegal=None)
    visited, edges = set(), set()
    writer = LastFlagWriter()  # declared splits for the pinned flag rules, see pinned_shift_flags

    def hook(emulator, address, size, user):
        del size, user
        if state["returned"]:
            state["stop"] = OK
            return emulator.emu_stop()
        linear = address & (2 * SIZE - 1)
        here = next(((seg, linear - lo + base) for seg, lo, hi, base in spans if lo <= linear < hi), None)
        if here is None:
            state["illegal"] = linear
            return emulator.emu_stop()
        previous = state["previous"]
        if previous is not None and previous[0] == A40 and here[0] == A40 and LOOPS.get(previous[1]) == here[1]:
            state["loopbacks"] += 1
            if state["loopbacks"] > cap:
                state["stop"] = LIMIT
                return emulator.emu_stop()
        if here == (R709, SITE) and bytes(emulator.mem_read(address, 3)) != b"\x9a\x9c\x02":
            state["stop"] = CODE
            return emulator.emu_stop()
        writer.step(emulator, address)
        visited.add(here)
        if previous in target["conditional"] and previous != here:
            edges.add((previous, here))
        state["previous"] = here
        state["returned"] = here == final
        return None

    def mirror(emulator, access, address, size, value, user):
        del access, user
        for byte in range(size):
            raw = address + byte; at = raw & (SIZE - 1)
            emulator.mem_write(at + SIZE if raw < SIZE else at, bytes([(value >> (8 * byte)) & 0xFF]))

    original.hook_add(UC_HOOK_CODE, hook)
    if high:
        original.hook_add(UC_HOOK_MEM_WRITE, mirror)
    original.emu_start(physical(cpu.cs, cpu.ip), 2 * SIZE, count=20000000)
    writer.finish(original)
    check(state["illegal"] is None and state["stop"] is not None, f"original did not stop cleanly ({state['illegal']}): {label}")
    expected = bytes(original.mem_read(0, SIZE))
    ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
    if mode == "delay":
        status = functions[0](ram, SIZE, ctypes.byref(cpu), cap)
        want = {OK: 0, LIMIT: 2}[state["stop"]]
    else:
        callee = ctypes.c_int(99)
        status = functions[1](ram, SIZE, ctypes.byref(cpu), cap, ctypes.byref(callee))
        want = {OK: 0, CODE: 2, LIMIT: 3}[state["stop"]]
        check(state["stop"] != LIMIT or callee.value == 2, f"callee status {callee.value} on a delay LIMIT: {label}")
    check(status == want, f"C status {status}, original {NAMES[state['stop']]}: {label}")
    actual = bytes(ram)
    if actual != expected:
        differences = [i for i, (x, y) in enumerate(zip(actual, expected)) if x != y]
        raise ValueError(f"RAM mismatch {label}: {[(hex(d), actual[d], expected[d]) for d in differences[:6]]} ({len(differences)} bytes)")
    for (name, _), register in zip(Cpu._fields_, REGISTERS):
        mine, value = getattr(cpu, name), original.reg_read(register) & 0xFFFF
        if name == "flags":
            problem = declared_flags_split(mine, value, writer)
            check(problem is None, f"flags mismatch {label}: {problem} ({NAMES[state['stop']]})")
            SPLIT_ARRIVALS[0] += split_used(mine, value, writer)
            continue
        check(mine == value, f"{name} mismatch {label}: C={mine:04x}, original={value:04x} ({NAMES[state['stop']]})")
    return state["stop"], visited, edges, state["loopbacks"]


def cases(rng):
    spec = []
    for i in range(100):
        spec.append((f"random-delay-{i}", dict(mode="delay", ms=rng.choice((0, 1, 2, 3, rng.randrange(1, 7))), count=rng.choice((1, 2, 3, 5, rng.randrange(1, 13))),
                                               flags=FLAGS[i % len(FLAGS)], sp=rng.randrange(0x0800, 0xF000) & ~1)))
    for i in range(40):
        spec.append((f"random-wrapper-{i}", dict(mode="wrapper", factor=rng.choice((0, 1, 2)), count=rng.choice((1, 2, 3)),
                                                 flags=FLAGS[i % len(FLAGS)], sp=rng.randrange(0x0800, 0xF000) & ~1)))
    spec.append(("count-zero", dict(mode="delay", ms=1, count=0, cap=300000)))
    spec.append(("alias-bios", dict(mode="delay", ms=2, count=3, bios=0x5A, ss=0x0000, sp=0x0402)))
    spec.append(("alias-count", dict(mode="delay", ms=2, count=3, ds=0x4000, ss=0x4000, sp=0x84FE, cap=10000)))
    for cap in (0, 3, 11):
        spec.append((f"limit-{cap}", dict(mode="delay", ms=2, count=3, cap=cap)))
    spec.append(("delay-ss-a20", dict(mode="delay", ms=2, count=2, ss=0xFFFF, sp=0x0800)))
    spec.append(("delay-df-set", dict(mode="delay", ms=1, count=2, flags=0x0603)))
    spec.append(("factor-ff", dict(mode="wrapper", factor=0xFF, count=1, cap=300000)))
    spec.append(("wrapper-code", dict(mode="wrapper", factor=2, patch=((SITE + 1, 0x9D),))))
    spec.append(("wrapper-callee-limit", dict(mode="wrapper", factor=1, count=1, cap=100)))
    spec.append(("wrapper-ss-a20", dict(mode="wrapper", factor=1, count=2, ss=0xFFFF, sp=0x0800)))
    spec.append(("random-background", dict(mode="wrapper", factor=1, count=2, background=bytes(random.Random(0x0DBE029C).randbytes(SIZE)))))
    return spec


def differential(target, delay, wrap, functions):
    rng = random.Random(0x0DBE029C)
    visited, edges, stops, most = set(), set(), {k: 0 for k in NAMES}, 0
    for label, options in cases(rng):
        cap = options.pop("cap", 100000)
        mode = options["mode"]
        memory, cpu, high = build(delay, wrap, rng, **options)
        stop, v, e, loopbacks = compare(label, mode, memory, cpu, high, target, functions, cap)
        if not label.startswith("random"):
            print(f"  {label}: {NAMES[stop]} after {loopbacks} loop-backs")
        stops[stop] += 1
        most = max(most, loopbacks)
        visited |= v; edges |= e
    reachable = {(s, (s[0], o)) for s, outs in target["conditional"].items() for o in outs}
    missing = sorted(f"{s:04X}:{a:04X}" for s, a in target["starts"] if (s, a) not in visited)
    print(f"Declared pinned flag splits used at {SPLIT_ARRIVALS[0]} stops {SPLIT_BY_RULE} (MULW: SF/AF kept, ZF from AX, PF over DX:AX)")
    print(f"Delay 029C + 0709:0DBE: {sum(stops.values())} original-byte cases ({', '.join(f'{v} {NAMES[k]}' for k, v in stops.items())}); "
          f"full 1 MiB RAM and 13 CPU words equal, FLAGS through the declared split above (no mask); {len(visited)}/{len(target['starts'])} instruction starts, "
          f"{len(edges & reachable)}/{len(reachable)} conditional outcomes; up to {most} loop-backs in one case")
    check(not missing and reachable <= edges, f"coverage incomplete: {missing} {sorted(reachable - edges)}")
    check(stops[OK] and stops[CODE] and stops[LIMIT], f"a stop kind was not exercised: {stops}")
    return dict(stops={NAMES[k]: v for k, v in stops.items()}, starts=len(visited), outcomes=len(edges & reachable), max_loopbacks=most)


# ---- DOSBox-X synthetic oracle ---------------------------------------------------------------------------
# The wrapper's LCALL names paragraph 0A40 directly: with the COM at segment 0814, 0A40:029C is COM offset 255Ch, so the
# Delay bytes are placed there in the COM image. The wrapper span is copied to its own offset in another segment. The
# harness records the live BIOS byte 0040:0000 for the replay; interrupts stay off around the call.
CALLER_N, DATA_N = 0x8800, 0x5000
MAIN_AT, DELAY_AT, WRAP_AT, CASES_AT = 0x0300, 0x255C, 0x2600, 0x2700
OUTPUT, HEADER, STACK_TOP = 0xC000, 0xD800, 0xF000
WINDOW_LOW, WINDOW_SIZE = STACK_TOP - 0x100, 0x100
NATIVE_CASES, CASE_SIZE = 32, 0x20
RECORD_BYTES = 28 + WINDOW_SIZE + 0x10000
NATIVE_FLAGS = (0x0002, 0x00D7, 0x08D7, 0x0402, 0x0893, 0x0003)
ENTRY_REGS = dict(ax=0x1234, bx=0x3456, cx=0x5678, dx=0x789A, si=0x9ABC, di=0xBCDE, bp=0x7777)


def native_memory(delay, wrap, data, bios):
    memory = bytearray(SIZE)
    at = physical(A40, DELAY); memory[at:at + len(delay)] = delay
    at = physical(CALLER_N, WRAP); memory[at:at + len(wrap)] = wrap
    at = physical(DATA_N, 0); memory[at:at + 0x10000] = data
    memory[0x400] = bios
    return memory


def native_audit(delay, wrap, functions):
    import os
    directory = ROOT / "resources/oracle-runs/res0709-delay-0dbe-synthetic"
    directory.mkdir(parents=True, exist_ok=True)
    check(physical(0x0814, DELAY_AT) == physical(A40, DELAY), "the Delay hole is not at 0A40:029C")
    rng = random.Random(0x0DBE029CD)
    cases = []
    for n in range(NATIVE_CASES):
        data = bytearray([0xA5]) * 0x10000
        factor, count = rng.choice((0, 1, 2, 3)), rng.randrange(1, 9)
        struct.pack_into("<H", data, 0x84FC, count)
        data[0x49E2] = factor
        writes = [(o, data[o] | (data[o + 1] << 8)) for o in range(0, 0x10000, 2) if data[o] != 0xA5 or data[o + 1] != 0xA5]
        check(4 + 4 * len(writes) <= CASE_SIZE, f"native case needs {len(writes)} writes")
        cases.append(dict(flags=n % len(NATIVE_FLAGS), data=data, writes=writes))
    table = b"".join((struct.pack("<BBH", c["flags"], 0, len(c["writes"])) + b"".join(struct.pack("<HH", o, v) for o, v in c["writes"]))
                     .ljust(CASE_SIZE, b"\0") for c in cases)
    check(MAIN_AT < DELAY_AT and DELAY_AT + len(delay) <= WRAP_AT and WRAP_AT + len(wrap) <= CASES_AT and CASES_AT + len(table) <= OUTPUT,
          "COM layout overlaps")
    t = [".code16", ".intel_syntax noprefix", ".global _start", "_start:", "    jmp main", f".org {MAIN_AT - 0x100:#x}", "main:",
         "    mov ax,cs", "    mov ds,ax", "    mov es,ax", "    mov ss,ax", f"    mov sp,{STACK_TOP:#x}", "    cld",
         f"    mov ax,{CALLER_N:#x}", "    mov es,ax", "    mov si,offset wrap", f"    mov di,{WRAP:#x}", f"    mov cx,{len(wrap):#x}", "    rep movsb",
         "    push cs", "    pop es", f"    mov word ptr [{HEADER:#x}],cs", f"    mov word ptr [{HEADER + 2:#x}],offset returned",
         f"    mov word ptr [{HEADER + 4:#x}],{len(cases)}", "    mov ax,0x40", "    mov es,ax", "    xor ah,ah", "    mov al,byte ptr es:[0]",
         f"    mov word ptr [{HEADER + 6:#x}],ax", "    push cs", "    pop es",
         "    mov dx,offset filename", "    xor cx,cx", "    mov ah,0x3c", "    int 0x21", "    jc failure",
         "    mov word ptr [handle],ax", "    mov bx,ax", f"    mov dx,{HEADER:#x}", "    mov cx,8", "    mov ah,0x40", "    int 0x21", "    jc failure",
         "again:", "    cli", f"    mov sp,{STACK_TOP:#x}", "    cld", "    push cs", "    pop ds", "    push cs", "    pop es",
         "    mov al,0xa5", f"    mov di,{WINDOW_LOW:#x}", f"    mov cx,{WINDOW_SIZE:#x}", "    rep stosb",
         "    mov ax,word ptr [index]", f"    mov bx,{CASE_SIZE}", "    mul bx", "    add ax,offset cases", "    mov bx,ax",
         f"    mov ax,{DATA_N:#x}", "    mov es,ax", "    xor di,di", "    mov cx,0x8000", "    mov ax,0xa5a5", "    rep stosw",
         "    mov cx,word ptr [bx+2]", "    jcxz written", "    lea si,[bx+4]", "write_loop:", "    mov di,word ptr [si]", "    mov ax,word ptr [si+2]",
         "    mov word ptr es:[di],ax", "    add si,4", "    loop write_loop", "written:",
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
          'filename: .asciz "RESULT.BIN"', "handle: .word 0", "index: .word 0", f"target_far: .word {WRAP:#x}, {CALLER_N:#x}",
          "flags_table: .word " + ",".join(f"{f:#x}" for f in NATIVE_FLAGS)]
    for label, blob, where in (("delay", delay, DELAY_AT), ("wrap", wrap, WRAP_AT), ("cases", table, CASES_AT)):
        t += [f".org {where - 0x100:#x}", f"{label}:"]
        for i in range(0, len(blob), 16):
            t.append("    .byte " + ",".join(f"{x:#x}" for x in blob[i:i + 16]))
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
    segment, returned, count, bios = struct.unpack_from("<4H", result)
    check(count == len(cases) and segment == 0x0814, f"native header differs or COM segment {segment:04x} is not 0814")
    delayed = 0
    for n, case in enumerate(cases):
        record = result[8 + n * RECORD_BYTES:8 + (n + 1) * RECORD_BYTES]
        memory = native_memory(delay, wrap, case["data"], bios & 0xFF)
        for i in range(WINDOW_SIZE):
            memory[physical(segment, WINDOW_LOW + i)] = 0xA5
        sp = STACK_TOP
        for value in (segment, returned):
            sp -= 2; put16(memory, segment, sp, value)
        cpu = Cpu(*ENTRY_REGS.values(), sp, CALLER_N, WRAP, DATA_N, segment, segment, NATIVE_FLAGS[case["flags"]])
        ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
        check(functions[1](ram, SIZE, ctypes.byref(cpu), 1 << 32, None) == OK, f"C status on native case {n}")
        for (reg, _), want in zip(Cpu._fields_, struct.unpack_from("<14H", record)):
            check(getattr(cpu, reg) == want, f"native case {n} {reg}: C={getattr(cpu, reg):04x}, DOS={want:04x}")
        got = bytes(ram[physical(segment, WINDOW_LOW):physical(segment, WINDOW_LOW) + WINDOW_SIZE]) + bytes(ram[physical(DATA_N, 0):physical(DATA_N, 0) + 0x10000])
        if got != record[28:]:
            differences = [i for i, (x, y) in enumerate(zip(got, record[28:])) if x != y]
            raise ValueError(f"native case {n} bytes differ at {[hex(d) for d in differences[:8]]} ({len(differences)} bytes)")
        delayed += case["data"][0x49E2] != 0
    digest = sha256(result).hexdigest()
    print(f"DOSBox-X normal-core/386 synthetic cases passed: {len(cases)} ({delayed} with a nonzero delay); all 14 CPU words, the stack window "
          f"and the whole 64 KiB data segment; no mask; live BIOS byte 0040:0000 = {bios & 0xFF:02X}h")
    print(f"native result SHA-256 {digest}; COM segment {segment:04x}; Delay at 0A40:029C (COM 255Ch), wrapper at {CALLER_N:04X}:{WRAP:04X}")
    return dict(cases=len(cases), delayed=delayed, result_sha256=digest)


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
    target, delay, wrap = source()
    report = dict(delay=dict(sha256=target["delay_sha"], bytes=len(delay), instructions=19),
                  wrapper=dict(sha256=target["wrap_sha"], bytes=len(wrap), instructions=10))
    if args.unicorn or args.native:
        with tempfile.TemporaryDirectory(prefix="poolrad-delay-") as temp:
            functions = library(Path(temp))
            if args.unicorn:
                report["unicorn"] = differential(target, delay, wrap, functions)
            if args.native:
                report["native"] = native_audit(delay, wrap, functions)
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(json.dumps(report, sort_keys=True, indent=2) + "\n")
    print(json.dumps(report, sort_keys=True))


if __name__ == "__main__":
    main()
