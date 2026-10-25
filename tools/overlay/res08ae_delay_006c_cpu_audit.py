#!/usr/bin/env python3
"""RES_08AE +006C original-byte parity: unmasked Unicorn CPU14 + 1 MiB with a declared
time-source stub at CS:0000 (the original RES_08AE_0000 is NOT executed); synthetic native run."""
from native_emulator_provenance import describe_dosbox
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
from resident_loader_map import unpack, relocations
from u86_procvars_3413_audit import Cpu, ROOT, SIZE, EXE_SHA, REGISTERS, physical, put16

CHECKER_ROWS = ('RES_08AE_006C',)
ENTRY, END, CODE = 0x006C, 0x00DC, 0x8800
BODY_SHA = 'f03f1ed3f490c274e18fa550c29af9b1bb41112ad6cb741781582721fed52446'
EXPANDED_SHA = 'a42a57e68d3f7815831821cc9365bb917befda45a58e958544451fc7a878df8e'
FLAGS = (0x0002, 0x00D7, 0x08D7, 0x0402, 0x0893, 0x7202, 0x0ED7, 0x0003, 0x0046)
CALL_SITES = (0x0073, 0x008A)
CONDITIONALS = {0x00AA: (0x00B2, 0x00AC), 0x00AC: (0x00C4, 0x00AE), 0x00B0: (0x00C4, 0x00B2),
                0x00CD: (0x00D6, 0x00CF), 0x00CF: (0x0089, 0x00D1), 0x00D4: (0x0089, 0x00D6)}
STATUS = {0: 'RETURN', 1: 'ARGUMENT', 2: 'CODE', 3: 'TIME', 4: 'LIMIT'}
HOOK = ctypes.CFUNCTYPE(ctypes.c_int, ctypes.c_void_p, ctypes.POINTER(ctypes.c_uint8), ctypes.POINTER(Cpu))
M32 = 0xFFFFFFFF

def check(condition, message):
    if not condition:
        raise ValueError(message)

def source():
    exe = (ROOT / 'resources/DOS/POOLRAD/START.EXE').read_bytes()
    check(len(exe) == 65231 and sha256(exe).hexdigest() == EXE_SHA, 'START.EXE provenance')
    expanded = bytes(unpack(exe)[0])
    check(sha256(expanded).hexdigest() == EXPANDED_SHA, 'expanded image provenance')
    span = expanded[physical(0x08AE, ENTRY):physical(0x08AE, END)]
    insns = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(span, ENTRY))
    check(sha256(span).hexdigest() == BODY_SHA and len(insns) == 46 and
          sum(i.size for i in insns) == 112 and
          (insns[-1].address, insns[-1].mnemonic, insns[-1].op_str) == (0x00D9, 'retf', '2'), 'span/return differs')
    # RETF is the first and only return, and every branch target lies inside the span.
    check([i.address for i in insns if i.mnemonic in ('ret', 'retf')] == [0x00D9], 'unexpected return')
    for i in insns:
        if i.mnemonic.startswith('j'):
            check(ENTRY <= int(i.op_str, 16) < END, 'branch leaves span')
    calls = [(i.address, i.mnemonic, i.op_str) for i in insns if i.mnemonic == 'call']
    check(calls == [(0x0073, 'call', '0'), (0x008A, 'call', '0')], 'call sites differ')
    check(all(span[a - ENTRY - 1] == 0x0E for a in CALL_SITES), 'PUSH CS before each call')
    rels, _ = relocations(exe, expanded)
    lo, hi = physical(0x08AE, ENTRY), physical(0x08AE, END)
    check(not any(lo <= r < hi for r in rels), 'loader relocation inside the span')
    return span, {i.address for i in insns}

def library(directory):
    path = directory / '006c.so'
    subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Werror', '-O2', '-fPIC', '-shared',
                    '-I', str(ROOT / 'include'), str(ROOT / 'src/recovered/resident_08ae_delay_006c_cpu.c'),
                    '-o', str(path)], check=True)
    fn = ctypes.CDLL(str(path)).pr_r08ae_delay_006c_cpu
    fn.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t, ctypes.POINTER(Cpu), HOOK,
                   ctypes.c_void_p, ctypes.c_uint]
    fn.restype = ctypes.c_int
    return fn

def fixture(span, rng, arg, *, ss=0x4000, sp=0x8000, cs=CODE, flags=2, background=None):
    memory = bytearray([0xA5]) * SIZE if background is None else bytearray(background)
    at = physical(cs, ENTRY)
    memory[at:at + len(span)] = span
    memory[physical(cs, 0)] = 0xCB          # declared stub: RETF stands in for RES_08AE_0000
    for off, value in ((0, 0x7000), (2, 0x9000), (4, arg)):
        put16(memory, ss, sp + off, value)
    cpu = Cpu(*(rng.randrange(65536) for _ in range(7)), sp, cs, ENTRY,
              rng.randrange(65536), rng.randrange(65536), ss, flags)
    return memory, cpu

def entry(v, rng, flags=None, pool=None):
    """One stub reading: DX:AX time plus the callee-owned BX, CX, DI and flags."""
    return (v & 0xFFFF, (v >> 16) & 0xFFFF, rng.randrange(65536), rng.randrange(65536), rng.randrange(65536),
            rng.choice(FLAGS if pool is None else pool) if flags is None else flags)

def compare(memory, cpu, script, fn, label, edges):
    from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE
    oracle = Uc(UC_ARCH_X86, UC_MODE_16)
    shared = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
    for base in (0, SIZE):               # two mappings of one backing give A20-off aliases
        oracle.mem_map_ptr(base, SIZE, 7, ctypes.addressof(shared))
    for (name, _), reg in zip(Cpu._fields_, REGISTERS):
        oracle.reg_write(reg, getattr(cpu, name))
    cs = cpu.cs
    seen, state = set(), dict(returned=False, stopped=None, used=0, last=None)
    def hook(uc, address, size, user):
        if state['returned']:
            state['stopped'] = 'RETURN'
            uc.emu_stop()
            return
        ip = address - (cs << 4)
        if ip == 0:                       # the declared time-source stub
            if state['used'] == len(script):
                state['stopped'] = 'TIME'
                uc.emu_stop()
                return
            for reg, value in zip(REGISTERS[:6], ()):
                pass
            ax, dx, bx, cx, di, flags = script[state['used']]
            state['used'] += 1
            uc.reg_write(REGISTERS[0], ax); uc.reg_write(REGISTERS[3], dx)
            uc.reg_write(REGISTERS[1], bx); uc.reg_write(REGISTERS[2], cx)
            uc.reg_write(REGISTERS[5], di); uc.reg_write(REGISTERS[13], flags)
            state['last'] = None
            return
        check(ENTRY <= ip < END, f'{label}: escaped before RETF: {ip:x}')
        if state['last'] in CONDITIONALS:
            edges.add((state['last'], ip))
        state['last'] = ip
        seen.add(ip)
        state['returned'] = ip == 0x00D9
    oracle.hook_add(UC_HOOK_CODE, hook)
    oracle.emu_start(physical(cs, ENTRY), 2 * SIZE, count=200000)
    check(state['stopped'] is not None, f'{label}: missing stop')
    ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
    calls = dict(n=0)
    def time(_ctx, _ram, c):
        if calls['n'] == len(script):
            return 1
        ax, dx, bx, cx, di, flags = script[calls['n']]
        calls['n'] += 1
        c.contents.ax, c.contents.dx, c.contents.bx = ax, dx, bx
        c.contents.cx, c.contents.di, c.contents.flags = cx, di, flags
        return 0
    status = fn(ram, SIZE, ctypes.byref(cpu), HOOK(time), None, 0xFFFFFFFF)
    check(STATUS[status] == state['stopped'], f'{label}: stop C={STATUS[status]} oracle={state["stopped"]}')
    check(calls['n'] == state['used'], f'{label}: time calls C={calls["n"]} oracle={state["used"]}')
    for (name, _), reg in zip(Cpu._fields_, REGISTERS):
        want = oracle.reg_read(reg) & 65535
        check(getattr(cpu, name) == want, f'{label}: {name}: C={getattr(cpu,name):04x} oracle={want:04x}')
    check(bytes(ram) == bytes(shared), f'{label}: full RAM differs')
    return seen, state['stopped'], state['used']

def script_for(rng, n, kind, pool=None):
    """Readings after the first, crafted per kind so each branch successor is reached."""
    t0 = rng.randrange(1 << 32) if kind != 'zero' else 0
    target = (t0 + n) & M32
    first = entry(t0, rng, pool=pool)
    r = []
    if kind == 'ascend':                  # climbs to the target in steps, then reaches it
        cur = t0
        while True:
            cur = (cur + rng.randrange(1, max(2, n // 3 + 2))) & M32
            r.append(entry(cur, rng, pool=pool))
            if ((cur - target) & M32) < 0x80000000 or len(r) > 40:
                break
    elif kind == 'exact':
        r.append(entry(target, rng, pool=pool))
    elif kind == 'over':
        r.append(entry((target + rng.randrange(1, 70000)) & M32, rng, pool=pool))
    elif kind == 'wrap':                  # clock far below the target: the guard fires
        r.append(entry((target - n - 1 - rng.randrange(0, 70000)) & M32, rng, pool=pool))
    elif kind == 'wrap-edge':             # exactly n and n+1 below the target
        r.append(entry((target - n) & M32, rng, pool=pool)); r.append(entry((target - n - 1) & M32, rng, pool=pool))
    elif kind == 'hi-sign':               # signed high-word compares: hi < 0, hi == 0, hi > 0
        for hi in (0x8000, 0xFFFF, 0, 1, 0x7FFF):
            r.append(entry(((hi << 16) | rng.randrange(65536)) & M32, rng, pool=pool))
        r.append(entry(target, rng, pool=pool))
    elif kind == 'zero':                  # first reading 0, then the target
        r.append(entry(target, rng, pool=pool))
    elif kind == 'random':
        for _ in range(rng.randrange(0, 12)):
            r.append(entry(rng.randrange(1 << 32), rng, pool=pool))
    return [first] + r

def differential(span, starts, fn):
    rng = random.Random(0x08AE006C)
    cases = []
    kinds = ('ascend', 'exact', 'over', 'wrap', 'wrap-edge', 'hi-sign', 'random', 'zero')
    args = (0, 1, 2, 0x7FFF, 0x8000, 0xFFFE, 0xFFFF)
    for i in range(480):
        kind = kinds[i % len(kinds)]
        n = args[(i // len(kinds)) % len(args)] if i % 3 else rng.randrange(65536)
        cases.append((f'{kind}-{i}', n, script_for(rng, n, kind), dict(flags=FLAGS[i % len(FLAGS)])))
    cases += [(f'sp-boundary-{sp:x}', 0x0123, script_for(rng, 0x0123, 'exact'), dict(sp=sp))
              for sp in (0x000E, 0x000F, 0xFFF9, 0xFFFA)]
    cases += [('a20-stack', 0x0042, script_for(rng, 0x0042, 'ascend'), dict(ss=0xFFFF, sp=0x0013)),
              ('a20-high-stack', 0x0042, script_for(rng, 0x0042, 'exact'), dict(ss=0xF000, sp=0xFFFA)),
              ('code-segment-ffff', 0x0042, script_for(rng, 0x0042, 'exact'), dict(cs=0xFFFF)),
              ('random-ram', 0x9876, script_for(rng, 0x9876, 'wrap'), dict(background=rng.randbytes(SIZE)))]
    seen, edges, stops, calls = set(), set(), {}, 0
    for label, n, script, kw in cases:
        memory, cpu = fixture(span, rng, n, **kw)
        s, stop, used = compare(memory, cpu, script, fn, label, edges)
        seen |= s; stops[stop] = stops.get(stop, 0) + 1; calls += used
    check(seen == starts, f'starts missing: {sorted(starts - seen)}')
    wanted = {(a, b) for a, targets in CONDITIONALS.items() for b in targets}
    check(edges == wanted, f'successor edges missing: {sorted(wanted - edges)}')
    check(stops.get('RETURN') and stops.get('TIME'), f'outcomes {stops}')
    # Code, argument and limit stops are API stops that do not execute original instructions beyond the guard.
    memory, base = fixture(span, rng, 0x0010)
    script = script_for(rng, 0x0010, 'exact')
    rejects = 0
    for sp, ip, size in ((0x000D, ENTRY, SIZE), (0xFFFB, ENTRY, SIZE), (0x8000, ENTRY + 1, SIZE), (0x8000, ENTRY, SIZE - 1)):
        cpu = Cpu.from_buffer_copy(base); cpu.sp = sp; cpu.ip = ip
        ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
        before = bytes(cpu)
        check(fn(ram, size, ctypes.byref(cpu), HOOK(lambda *_: 1), None, 100) == 1 and bytes(cpu) == before and bytes(ram) == memory, 'ARGUMENT mutation')
        rejects += 1
    codes = 0
    for site in CALL_SITES:
        for byte_at, value in ((site, 0x90), (site - 1, 0x90), (site + 1, 0x00)):
            mem = bytearray(memory); mem[physical(CODE, byte_at)] = value
            cpu = Cpu.from_buffer_copy(base); ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(mem)
            if site == 0x008A:            # reach the second site through a first, valid reading
                it = iter(script)
                hk = HOOK(lambda _c, _r, c: (setattr(c.contents, 'ax', 1), setattr(c.contents, 'dx', 0), 0)[2])
            else:
                hk = HOOK(lambda *_: 1)
            st = fn(ram, SIZE, ctypes.byref(cpu), hk, None, 1000)
            check(st == 2 and cpu.ip == site - 1 and cpu.sp == 0x8000 - 2 - 8, 'CODE stop state')
            codes += 1
    limit = Cpu.from_buffer_copy(base); ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
    check(fn(ram, SIZE, ctypes.byref(limit), HOOK(lambda *_: 1), None, 3) == 4 and limit.ip == 0x0072, 'LIMIT stop')
    return dict(cases=len(cases), starts=len(seen), starts_total=len(starts), conditional_edges=len(edges),
                conditional_edges_total=len(wanted), stub_calls=calls, cpu_words=14, ram_bytes=SIZE, masked_ram_bytes=0,
                stops=stops, argument_cases=rejects, code_cases=codes, limit_cases=1)

CALLER_N = 0x8800
MAIN_AT, STUB_AT, WRAP_AT, CASES_AT = 0x0300, 0x2680, 0x2600, 0x2700
OUTPUT, HEADER, STACK_TOP = 0xC000, 0xD800, 0xF000
WINDOW_LOW, WINDOW_SIZE = STACK_TOP - 0x100, 0x100
NATIVE_CASES, CASE_SIZE = 32, 0x100
RECORD_BYTES = 28 + WINDOW_SIZE
NATIVE_FLAGS = (0x0002, 0x00D7, 0x08D7, 0x0402, 0x0893, 0x0003)
ENTRY_REGS = dict(ax=0x1234, bx=0x3456, cx=0x5678, dx=0x789A, si=0x9ABC, di=0xBCDE, bp=0x7777)
SCRIPT_AT, POINTER_AT = 0x0400, 0x03FE

def native_cases(span, fn):
    rng = random.Random(0x08AE006CD)
    kinds = ('ascend', 'exact', 'over', 'wrap', 'wrap-edge', 'hi-sign', 'zero')
    args = (0, 1, 2, 0x7FFF, 0x8000, 0xFFFF)
    cases = []
    while len(cases) < NATIVE_CASES:
        i = len(cases)
        kind = kinds[i % len(kinds)]
        n = args[(i // len(kinds)) % len(args)] if i % 3 else rng.randrange(65536)
        script = script_for(rng, n, kind, NATIVE_FLAGS)
        if len(script) > 20:
            continue
        cases.append(dict(flags=i % len(NATIVE_FLAGS), n=n, script=script, kind=kind))
    return cases

def native_audit(span, function):
    import os
    r = dict(entry=ENTRY)
    cases = native_cases(span, function)
    directory = ROOT / "resources/oracle-runs/res08ae-006c-synthetic"
    directory.mkdir(parents=True, exist_ok=True)
    stub = ["    mov bx,word ptr cs:[%#x]" % POINTER_AT, "    add word ptr cs:[%#x],12" % POINTER_AT,
            "    mov di,sp", "    mov ax,word ptr ss:[di-2]", "    mov dx,word ptr cs:[bx+10]",
            "    push dx", "    popf", "    mov word ptr ss:[di-2],ax",     # POPF scratch word restored: no stack residue
            "    mov ax,word ptr cs:[bx]", "    mov dx,word ptr cs:[bx+2]", "    mov cx,word ptr cs:[bx+6]",
            "    mov di,word ptr cs:[bx+8]", "    mov bx,word ptr cs:[bx+4]", "    retf"]
    table = b""
    for c in cases:
        record = struct.pack("<BBHHH", c["flags"], 0, len(c["script"]), c["n"], 0)
        for e in c["script"]:
            record += struct.pack("<6H", *e)
        check(len(record) <= CASE_SIZE, "case record too large")
        table += record.ljust(CASE_SIZE, b"\0")
    check(CASES_AT + len(table) <= OUTPUT, "COM layout overlaps")
    t = [".code16", ".intel_syntax noprefix", ".global _start", "_start:", "    jmp main", f".org {MAIN_AT - 0x100:#x}", "main:",
         "    mov ax,cs", "    mov ds,ax", "    mov es,ax", "    mov ss,ax", f"    mov sp,{STACK_TOP:#x}", "    cld",
         f"    mov ax,{CALLER_N:#x}", "    mov es,ax", "    mov si,offset wrap", f"    mov di,{ENTRY:#x}", f"    mov cx,{len(span):#x}", "    rep movsb",
         "    mov si,offset stub", "    xor di,di", "    mov cx,0x40", "    rep movsb",
         "    push cs", "    pop es", f"    mov word ptr [{HEADER:#x}],cs", f"    mov word ptr [{HEADER + 2:#x}],offset returned",
         f"    mov word ptr [{HEADER + 4:#x}],{NATIVE_CASES}", f"    mov word ptr [{HEADER + 6:#x}],0",
         "    mov dx,offset filename", "    xor cx,cx", "    mov ah,0x3c", "    int 0x21", "    jc failure",
         "    mov word ptr [handle],ax", "    mov bx,ax", f"    mov dx,{HEADER:#x}", "    mov cx,8", "    mov ah,0x40", "    int 0x21", "    jc failure",
         "again:", "    cli", f"    mov sp,{STACK_TOP:#x}", "    cld", "    push cs", "    pop ds", "    push cs", "    pop es",
         "    mov al,0xa5", f"    mov di,{WINDOW_LOW:#x}", f"    mov cx,{WINDOW_SIZE:#x}", "    rep stosb",
         "    mov ax,word ptr [index]", f"    mov bx,{CASE_SIZE}", "    mul bx", "    add ax,offset cases", "    mov bx,ax",
         f"    mov ax,{CALLER_N:#x}", "    mov es,ax", f"    mov di,{SCRIPT_AT:#x}", "    lea si,[bx+8]",
         "    mov ax,word ptr [bx+2]", "    mov cx,6", "    mul cx", "    mov cx,ax", "    rep movsw",
         f"    mov word ptr es:[{POINTER_AT:#x}],{SCRIPT_AT:#x}",
         "    push word ptr [bx+4]", "    xor ah,ah", "    mov al,byte ptr [bx]", "    shl ax,1", "    mov si,ax",
         "    push word ptr [flags_table+si]", "    push cs", "    pop es", "    push cs", "    pop ds"]
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
          "    inc word ptr [index]", "    mov ax,word ptr [index]", f"    cmp ax,{NATIVE_CASES}", "    jb again",
          "    mov bx,word ptr [handle]", "    mov ah,0x3e", "    int 0x21", "    mov ax,0x4c00", "    int 0x21",
          "failure:", "    mov ax,0x4c01", "    int 0x21",
          'filename: .asciz "RESULT.BIN"', "handle: .word 0", "index: .word 0", f"target_far: .word {r['entry']:#x}, {CALLER_N:#x}",
          "flags_table: .word " + ",".join(f"{f:#x}" for f in NATIVE_FLAGS)]
    for label, blob, where in (("wrap", span, WRAP_AT), ("stub", None, STUB_AT), ("cases", table, CASES_AT)):
        t += [f".org {where - 0x100:#x}", f"{label}:"]
        if blob is None:
            t += stub + ["    .fill 0x40-(.-stub),1,0"]
            continue
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
    emulator = describe_dosbox("dosbox-x")
    with (directory / "dosbox.log").open("w") as log:
        subprocess.run([emulator["path"], "-conf", str(config), "-fastlaunch", "-exit", "-noconsole"],
                       env=environment, stdout=log, stderr=subprocess.STDOUT, check=True, timeout=600)
    result = result_path.read_bytes()
    check(len(result) == 8 + NATIVE_CASES * RECORD_BYTES, f"native output incomplete: {len(result)} bytes")
    segment, returned, count, _ = struct.unpack_from("<4H", result)
    check(count == NATIVE_CASES and segment == 0x0814, f"native header differs or COM segment {segment:04x} is not 0814")
    equal_loops = 0
    for n, case in enumerate(cases):
        record = result[8 + n * RECORD_BYTES:8 + (n + 1) * RECORD_BYTES]
        memory = bytearray(SIZE)
        at = physical(CALLER_N, ENTRY); memory[at:at + len(span)] = span
        memory[physical(CALLER_N, 0)] = 0xCB
        for i in range(WINDOW_SIZE):
            memory[physical(segment, WINDOW_LOW + i)] = 0xA5
        sp = STACK_TOP
        for value in (case["n"], segment, returned):
            sp -= 2; put16(memory, segment, sp, value)
        cpu = Cpu(*ENTRY_REGS.values(), sp, CALLER_N, ENTRY, segment, segment, segment, NATIVE_FLAGS[case["flags"]])
        ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
        used = dict(k=0)
        def time(_ctx, _ram, c, script=case["script"], used=used):
            if used["k"] == len(script):
                return 1
            ax, dx, bx, cx, di, flags = script[used["k"]]
            used["k"] += 1
            c.contents.ax, c.contents.dx, c.contents.bx = ax, dx, bx
            c.contents.cx, c.contents.di, c.contents.flags = cx, di, flags
            return 0
        status = function(ram, SIZE, ctypes.byref(cpu), HOOK(time), None, 0xFFFFFFFF)
        check(status == 0, f"C status on native case {n}: {STATUS[status]}")
        for (reg, _), want in zip(Cpu._fields_, struct.unpack_from("<14H", record)):
            check(getattr(cpu, reg) == want, f"native case {n} {reg}: C={getattr(cpu, reg):04x}, DOS={want:04x}")
        got = bytes(ram[physical(segment, WINDOW_LOW):physical(segment, WINDOW_LOW) + WINDOW_SIZE])
        check(got == record[28:], f"native case {n} stack window differs")
        equal_loops += used["k"] > 2
    digest = sha256(result).hexdigest()
    print(f"006C DOSBox-X normal-core/386 synthetic cases passed: {NATIVE_CASES}; all 14 CPU words and the 256-byte stack window; "
          f"result SHA-256 {digest}")
    return dict(dosbox_x=emulator, cases=NATIVE_CASES, result_sha256=digest, cpu_words=14, ram_bytes_compared=WINDOW_SIZE,
                ram_bytes_not_compared=SIZE - WINDOW_SIZE, synthetic_frames=True, callee_stubs=1,
                cases_with_loop_iterations=equal_loops, dosbox_x_sha256=emulator["sha256"],
                pinned_build_used=False)

def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--compile-only', action='store_true')
    p.add_argument('--unicorn', action='store_true')
    p.add_argument('--native', action='store_true')
    p.add_argument('--output', type=Path)
    a = p.parse_args()
    report = {}
    with tempfile.TemporaryDirectory(prefix='poolrad-006c-') as tmp:
        fn = library(Path(tmp))
        if a.compile_only:
            print('CHECKER_BUILD_OK')
            return
        span, starts = source()
        report['target'] = dict(bytes=len(span), starts=len(starts), sha256=BODY_SHA, expanded_sha256=EXPANDED_SHA)
        if a.unicorn:
            report['unicorn'] = differential(span, starts, fn)
        if a.native:
            report['native'] = native_audit(span, fn)
    if a.output:
        a.output.parent.mkdir(parents=True, exist_ok=True)
        a.output.write_text(json.dumps(report, indent=2, sort_keys=True) + '\n')
    print(json.dumps(report, sort_keys=True))

if __name__ == '__main__':
    main()
