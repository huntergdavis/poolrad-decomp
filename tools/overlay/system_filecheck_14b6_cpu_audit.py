#!/usr/bin/env python3
"""SYSTEM 0AF8:14B6 (Turbo Pascal file-magic check helper, shared by five call sites; raw CPU, runtime coverage, no identity claim)
against the original bytes. The Unicorn run executes the original at CODE:14B6 until the near RET has run (to whatever IP the slot holds), then compares
all 14 CPU words and the whole 1 MiB RAM with the C model, with no mask. Native DOSBox-X replays the original with a declared stub at the return IP. Media stays
private."""
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
from resident_loader_map import unpack, relocations
from u86_procvars_3413_audit import Cpu, ROOT, SIZE, EXE_SHA, OVR_SHA, REGISTERS, physical, put16

CHECKER_ROWS = ('RES_0AF8_14B6',)
SEG = 0x0AF8
ENTRY, END, RETF = 0x14B6, 0x14C5, 0x14C4
ENTRIES = (0x14B6,)
CALL_SITES = {}
STOPS = {0x0100: 2}                                              # native only: the return IP the harness planted, where a JMP FAR stub captures the state
NAMES = {0: 'RETURN'}
CODE, DEFAULT_DATA, DATA_SEG = 0x8800, 0x5000, 0x3000
BODY_SHA = '24d6d6fd5079eeb043c5f3a598502fa40ba66a4e12c919717ccdf847f362d363'
EXPANDED_SHA = 'a42a57e68d3f7815831821cc9365bb917befda45a58e958544451fc7a878df8e'
FLAGS = (0x0002, 0x00D7, 0x08D7, 0x0402, 0x0893, 0x7202, 0x0ED7)
NEAR_CALLERS = {0x147B: 0x1475, 0x1497: 0x1491, 0x14D7: 0x14D1, 0x150D: 0x1507, 0x1569: 0x1563}   # call site -> the entry body that contains it
MASK = SIZE - 1


def check(condition, message):
    if not condition:
        raise ValueError(message)


def source():
    exe = (ROOT / 'resources/DOS/POOLRAD/START.EXE').read_bytes()
    check(len(exe) == 65231 and sha256(exe).hexdigest() == EXE_SHA, 'START.EXE provenance')
    expanded = bytes(unpack(exe)[0])
    check(sha256(expanded).hexdigest() == EXPANDED_SHA, 'expanded image provenance')
    span = expanded[physical(SEG, ENTRY):physical(SEG, END)]
    insns = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(span, ENTRY))
    check(sha256(span).hexdigest() == BODY_SHA and len(insns) == 4 and sum(i.size for i in insns) == 15, 'span differs')
    check([(i.address, i.mnemonic, i.op_str) for i in insns] ==
          [(0x14B6, 'cmp', 'word ptr es:[di + 2], 0xd7b3'), (0x14BC, 'je', '0x14c4'), (0x14BE, 'mov', 'word ptr [0x4426], 0x67'), (0x14C4, 'ret', '')],
          'instructions differ')
    check(not any(i.mnemonic in ('int', 'in', 'out', 'call', 'lcall', 'retf') for i in insns), 'no interrupt, port, call or far return')
    rels, _ = relocations(exe, expanded)
    check(not any(physical(SEG, a) in rels for a in range(ENTRY, END)), 'the body has no loader relocation')
    conditional = {i.address: (i.address + i.size, int(i.op_str, 16)) for i in insns if i.mnemonic.startswith('j') and i.mnemonic != 'jmp'}
    check(conditional == {0x14BC: (0x14BE, 0x14C4)}, 'conditional jumps')
    return span, {i.address for i in insns}, conditional


def ownership():
    """The helper has no far entry: it is reached only by near CALL. Every E8 in the SYSTEM segment whose target is 14B6, each decoded inside the entry body
    it belongs to (the body's first instruction is the nearest preceding entry listed in NEAR_CALLERS)."""
    exe = (ROOT / 'resources/DOS/POOLRAD/START.EXE').read_bytes()
    image = bytes(unpack(exe)[0])
    segment = image[physical(SEG, 0):physical(SEG, 0) + 0x10000]
    raw = [o for o in range(len(segment) - 2) if segment[o] == 0xE8 and (o + 3 + struct.unpack_from('<h', segment, o + 1)[0]) & 0xFFFF == ENTRY]
    jumps = [o for o in range(len(segment) - 2) if segment[o] == 0xE9 and (o + 3 + struct.unpack_from('<h', segment, o + 1)[0]) & 0xFFFF == ENTRY]
    decoded = {}
    for site, start in NEAR_CALLERS.items():
        run = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(segment[start:site + 3], start))
        if run and run[-1].address == site and run[-1].mnemonic == 'call' and run[-1].op_str == '0x14b6' and sum(i.size for i in run) == site + 3 - start:
            decoded[site] = start
    check(raw == sorted(NEAR_CALLERS) and decoded == NEAR_CALLERS and not jumps,
          f'near-call census differs: raw {[hex(a) for a in raw]}, decoded {[(hex(a), hex(f)) for a, f in sorted(decoded.items())]}, jumps {[hex(a) for a in jumps]}')
    pattern = struct.pack('<HH', ENTRY, SEG)
    rels, _ = relocations(exe, image)
    far = [i - 1 for i in range(1, len(image) - 3) if image[i:i + 4] == pattern and image[i - 1] == 0x9A and (i + 2) in rels]
    check(not far, f'a far call to 14B6 appeared: {far}')
    return dict(near_call_sites={f'{a:04X}': f'body {f:04X}' for a, f in sorted(NEAR_CALLERS.items())}, far_entries=0, identity='unresolved', credit_claimed=False)


def library(directory):
    path = directory / 'filecheck.so'
    subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Wpedantic', '-Werror', '-O2', '-fPIC', '-shared', '-I', str(ROOT / 'include'),
                    str(ROOT / 'src/recovered/system_filecheck_14b6_cpu.c'), '-o', str(path)], check=True)
    fn = ctypes.CDLL(str(path)).pr_system_filecheck_14b6
    fn.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t, ctypes.POINTER(Cpu)]
    fn.restype = ctypes.c_int
    return fn


def fixture(span, rng, magic, *, var=(DATA_SEG, 0x0100), inout=0x1234, ss=0x4000, sp=0x8000, data=DEFAULT_DATA, flags=2, background=None, ret_ip=0x149A, handle=5):
    """A near-called state: [return IP] at SS:SP, ES:DI = the file variable [handle][magic], InOutRes = the WORD DS:4426 (inout None leaves the background)."""
    memory = bytearray([0xA5]) * SIZE if background is None else bytearray(background)
    at = physical(CODE, ENTRY)
    memory[at:at + len(span)] = span
    put16(memory, ss, sp & 0xFFFF, ret_ip)
    put16(memory, var[0], var[1], handle)
    put16(memory, var[0], (var[1] + 2) & 0xFFFF, magic)
    if inout is not None:
        put16(memory, data, 0x4426, inout)
    regs = [rng.randrange(65536) for _ in range(7)]
    regs[5] = var[1]                                                # DI
    cpu = Cpu(*regs, sp, CODE, ENTRY, data, var[0], ss, flags)
    return memory, cpu


def p_cmp_flags(a, b):
    """CMP word a,b flags written without the C model's helper (SF, ZF, PF, CF, AF, OF)."""
    r = (a - b) & 0xFFFF
    sa, sb = a - 65536 * (a >> 15), b - 65536 * (b >> 15)
    return ((1 if a < b else 0) | (4 if bin(r & 255).count('1') % 2 == 0 else 0) | (0x10 if (a & 15) < (b & 15) else 0) |
            (0x40 if r == 0 else 0) | (0x80 if r >> 15 else 0) | (0x800 if not -32768 <= sa - sb <= 32767 else 0))


def compare(memory, cpu, fn, label, conditional, ref=None):
    from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE
    from unicorn.x86_const import UC_X86_REG_CS
    oracle = Uc(UC_ARCH_X86, UC_MODE_16)
    shared = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
    for base in (0, SIZE):                                          # two mappings of the same bytes give the A20-off alias
        oracle.mem_map_ptr(base, SIZE, 7, ctypes.addressof(shared))
    for (name, _), reg in zip(Cpu._fields_, REGISTERS):
        oracle.reg_write(reg, getattr(cpu, name))
    cpu_in = Cpu.from_buffer_copy(cpu)
    seen, edges = set(), set()
    state = dict(returned=False, stopped=False, previous=None)

    def hook(uc, address, size, user):
        cs = uc.reg_read(UC_X86_REG_CS)
        ip = (address - (cs << 4)) & 0xFFFFF
        if state['returned']:                                       # the RET has run: the next instruction is wherever the slot pointed
            state['stopped'] = True
            uc.emu_stop()
            return
        check(cs == CODE and ENTRY <= ip < END, f'{label}: escaped: {cs:x}:{ip:x}')
        if state['previous'] in conditional:
            edges.add((state['previous'], ip))
        seen.add(ip)
        state['previous'] = ip
        state['returned'] = ip == RETF

    oracle.hook_add(UC_HOOK_CODE, hook)
    oracle.emu_start(physical(CODE, ENTRY), 2 * SIZE, count=20)
    check(state['stopped'], f'{label}: the RET did not run')
    ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
    got = fn(ram, SIZE, ctypes.byref(cpu))
    check(got == 0, f'{label}: C status {got}')
    for (name, _), reg in zip(Cpu._fields_, REGISTERS):
        want = oracle.reg_read(reg) & 65535
        check(getattr(cpu, name) == want, f'{label}: {name}: C={getattr(cpu, name):04x} oracle={want:04x}')
    if bytes(ram) != bytes(shared):
        diff = [i for i, (a, b) in enumerate(zip(bytes(ram), bytes(shared))) if a != b]
        raise ValueError(f'{label}: full RAM differs at {[hex(d) for d in diff[:8]]} ({len(diff)} bytes)')
    if ref is not None:                                            # the documented contract, on non-aliasing states
        magic, inout, ret_ip = ref
        mem = bytes(shared)
        final = Cpu.from_buffer_copy(cpu)
        res = mem[physical(cpu_in.ds, 0x4426)] | (mem[physical(cpu_in.ds, 0x4427)] << 8)
        want_res = inout if magic == 0xD7B3 else 0x0067
        check(res == want_res, f'{label}: InOutRes {res:04x}, expected {want_res:04x}')
        check((final.ip, final.sp, final.cs) == (ret_ip, (cpu_in.sp + 2) & 0xFFFF, cpu_in.cs), f'{label}: RET did not pop the planted IP')
        check(final.flags & 0x08D5 == p_cmp_flags(magic, 0xD7B3), f'{label}: flags {final.flags & 0x8D5:04x} != {p_cmp_flags(magic, 0xD7B3):04x}')
    return seen, edges, got


def differential(span, starts, conditional, fn):
    rng = random.Random(0x08AF14B6)
    cases = []                                                      # (label, magic, kwargs, plain)
    magics = (0xD7B3, 0xD7B0, 0xD7B1, 0xD7B2, 0xD7B4, 0x0000, 0xFFFF, 0x7FFF, 0x8000, 0x5000, 0x00B3, 0xD700, 0xD7B3 ^ 0x0100, 0xD7B3 ^ 0x8000)
    n = 0
    for magic in magics:
        for inout in (0, 0x1234):
            for ret_ip in (0x149A, 0x0000, 0xFFFF):
                cases.append((f'm{magic:04x}-i{inout:04x}-r{ret_ip:04x}', magic, dict(inout=inout, ret_ip=ret_ip, flags=FLAGS[n % len(FLAGS)]), True))
                n += 1
    for di in (0xFFFC, 0xFFFD, 0xFFFE, 0xFFFF):                       # the magic word's offset wraps inside the segment, the word at FFFF straddles
        for magic in (0xD7B3, 0xD7B1):
            cases.append((f'di{di:04x}-m{magic:04x}', magic, dict(var=(DATA_SEG, di)), False))
    for sp in (0x0000, 0x0001, 0x0100, 0xFFFB, 0xFFFC):
        for magic in (0xD7B3, 0xD7B1):
            cases.append((f'sp{sp:04x}-m{magic:04x}', magic, dict(sp=sp), False))
    for data in (0x0000, 0xFFFF, 0x1234, CODE):
        for magic in (0xD7B3, 0xD7B1):
            cases.append((f'ds{data:04x}-m{magic:04x}', magic, dict(data=data), data != 0xFFFF))
    for es in (0xFFFF, DEFAULT_DATA, 0x4000):                         # A20-aliased variable, variable in the data segment, variable on the stack segment
        for magic in (0xD7B3, 0xD7B1):
            cases.append((f'es{es:04x}-m{magic:04x}', magic, dict(var=(es, 0x0100)), False))
    cases.append(('a20-stack', 0xD7B1, dict(ss=0xFFFF, sp=0x0013), False))
    for magic in (0xD7B3, 0xD7B1):
        cases.append((f'random-ram-m{magic:04x}', magic, dict(background=rng.randbytes(SIZE)), False))
    for sp in range(0x4420, 0x4430):                                 # SS == DS: the RET slot overlaps InOutRes, so `MOV [4426],67h` redirects the return
        for magic in (0xD7B3, 0xD7B1):
            cases.append((f'ret-slot-on-inoutres-{sp:04x}-m{magic:04x}', magic, dict(ss=DEFAULT_DATA, sp=sp, inout=None, flags=0x0ED7), False))
    for k in range(4):                                               # the RET slot overlaps the file variable's magic word itself (SS == ES)
        cases.append((f'ret-slot-on-magic-{k}', 0xD7B1, dict(ss=DATA_SEG, sp=0x0102 - k, var=(DATA_SEG, 0x0100)), False))
    seen, edges, plain = set(), set(), 0
    for label, magic, kw, is_plain in cases:
        memory, cpu = fixture(span, rng, magic, **kw)
        independent = is_plain and kw.get('ss', 0x4000) != kw.get('data', DEFAULT_DATA)
        s_, e_, _ = compare(memory, cpu, fn, label, conditional, (magic, kw.get('inout', 0x1234), kw.get('ret_ip', 0x149A)) if independent else None)
        plain += 1 if independent else 0
        seen |= s_
        edges |= e_
    check(seen == starts, f'starts missing: {sorted(starts - seen)}')
    wanted = {(a, b) for a, pair in conditional.items() for b in pair}
    check(edges >= wanted, f'outcomes missing: {sorted(wanted - edges)}')
    memory, base = fixture(span, rng, 0xD7B3)
    rejects = 0
    for sp, ip, size in ((0xFFFD, ENTRY, SIZE), (0xFFFF, ENTRY, SIZE), (0x8000, ENTRY + 1, SIZE), (0x8000, RETF, SIZE), (0x8000, ENTRY, SIZE - 1)):
        cpu = Cpu.from_buffer_copy(base)
        cpu.sp, cpu.ip = sp, ip
        ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
        before = bytes(cpu)
        check(fn(ram, size, ctypes.byref(cpu)) == 1 and bytes(cpu) == before and bytes(ram) == memory, 'ARGUMENT mutation')
        rejects += 1
    return dict(cases=len(cases), reference_checked_cases=plain, argument_cases=rejects, starts=len(seen), starts_total=len(starts),
                conditional_outcomes=len(edges & wanted), conditional_outcomes_total=len(wanted), calls=0, cpu_words=14, ram_bytes=SIZE,
                masked_ram_bytes=0, stops={'RETURN': len(cases), 'ARGUMENT': rejects})


# ---- DOSBox-X synthetic oracle (callee stubs DECLARED) -------------------------------------------------
CALLER_N, DATA_N = 0x8800, 0x5000
MAIN_AT, WRAP_AT, CASES_AT = 0x0300, 0x2600, 0x2700
OUTPUT, HEADER, STACK_TOP = 0xC000, 0xD800, 0xF000
WINDOW_LOW, WINDOW_SIZE = STACK_TOP - 0x100, 0x100
NATIVE_CASES, CASE_SIZE = 48, 0x100
RECORD_BYTES = 28 + WINDOW_SIZE + 0x10000
NATIVE_FLAGS = (0x0002, 0x00D7, 0x08D7, 0x0402, 0x0893, 0x0003)
ENTRY_REGS = dict(ax=0x1234, bx=0x3456, cx=0x5678, dx=0x789A, si=0x9ABC, di=0x0100, bp=0x7777)


def native_cases():
    """48 synthetic runs: InOutRes values 0 (RETURN) and non-zero (RUNERROR stop), under six flag images."""
    magics = (0xD7B3, 0xD7B0, 0xD7B1, 0xD7B2, 0xD7B4, 0x0000, 0xFFFF, 0x7FFF, 0x8000, 0x5000, 0xD7B3, 0xD700)
    return [dict(entry=0, arg=0, flags=n % len(NATIVE_FLAGS), writes=[(0x0100, 5), (0x0102, magics[n % len(magics)]), (0x4426, 0x1234)]) for n in range(NATIVE_CASES)]


def native_audit(span, function):
    directory = ROOT / 'resources/oracle-runs/system-filecheck-14b6-synthetic'
    directory.mkdir(parents=True, exist_ok=True)
    cases = native_cases()
    table = b''.join((struct.pack('<BBHHHH', c['flags'], 0, len(c['writes']), c['entry'], c['arg'], 0) +
                      b''.join(struct.pack('<HH', o, v) for o, v in c['writes'])).ljust(CASE_SIZE, b'\0') for c in cases)
    check(CASES_AT + len(table) <= OUTPUT, 'COM layout overlaps')
    stubs = sorted(STOPS)
    t = ['.code16', '.intel_syntax noprefix', '.global _start', '_start:', '    jmp main', f'.org {MAIN_AT - 0x100:#x}', 'main:',
         '    mov ax,cs', '    mov ds,ax', '    mov es,ax', '    mov ss,ax', f'    mov sp,{STACK_TOP:#x}', '    cld',
         f'    mov ax,{CALLER_N:#x}', '    mov es,ax', '    mov si,offset wrap', f'    mov di,{ENTRY:#x}', f'    mov cx,{len(span):#x}', '    rep movsb']
    for n, target in enumerate(stubs):                                  # DECLARED stubs: JMP FAR into the harness at each callee entry
        t += [f'    mov byte ptr es:[{target:#x}],0xea', f'    mov word ptr es:[{target + 1:#x}],offset stub{n}', f'    mov word ptr es:[{target + 3:#x}],cs']
    t += ['    push cs', '    pop es', f'    mov word ptr [{HEADER:#x}],cs', f'    mov word ptr [{HEADER + 2:#x}],offset returned',
          f'    mov word ptr [{HEADER + 4:#x}],{NATIVE_CASES}', f'    mov word ptr [{HEADER + 6:#x}],0',
          '    mov dx,offset filename', '    xor cx,cx', '    mov ah,0x3c', '    int 0x21', '    jc failure',
          '    mov word ptr [handle],ax', '    mov bx,ax', f'    mov dx,{HEADER:#x}', '    mov cx,8', '    mov ah,0x40', '    int 0x21', '    jc failure',
          'again:', '    cli', f'    mov sp,{STACK_TOP:#x}', '    cld', '    push cs', '    pop ds', '    push cs', '    pop es',
          '    mov al,0xa5', f'    mov di,{WINDOW_LOW:#x}', f'    mov cx,{WINDOW_SIZE:#x}', '    rep stosb',
          '    mov ax,word ptr [index]', f'    mov bx,{CASE_SIZE}', '    mul bx', '    add ax,offset cases', '    mov bx,ax',
          f'    mov ax,{DATA_N:#x}', '    mov es,ax', '    xor di,di', '    mov cx,0x8000', '    mov ax,0xa5a5', '    rep stosw',
          '    mov cx,word ptr [bx+2]', '    jcxz written', '    lea si,[bx+10]', 'write_loop:', '    mov di,word ptr [si]', '    mov ax,word ptr [si+2]',
          '    mov word ptr es:[di],ax', '    add si,4', '    loop write_loop', 'written:',
          '    mov ax,word ptr [bx+4]', '    mov word ptr [entry_index],ax', '    shl ax,1', '    mov si,ax', '    mov ax,word ptr [entry_table+si]', '    mov word ptr [target_far],ax',
          '    push 0x0100', '    cmp word ptr [entry_index],0', '    je no_extra', '    mov ax,0x7777', '    push ax', 'no_extra:',
          '    xor ah,ah', '    mov al,byte ptr [bx]', '    shl ax,1', '    mov si,ax',
          '    push word ptr [flags_table+si]', f'    mov ax,{DATA_N:#x}', '    mov es,ax', '    mov ds,ax']
    for reg, value in ENTRY_REGS.items():
        t.append(f'    mov {reg},{value:#x}')
    t += ['    cmp word ptr cs:[entry_index],0', '    je plain_entry', '    mov bp,sp', '    add bp,2', 'plain_entry:', '    popf', '    jmp dword ptr cs:[target_far]']

    def capture(label, cs_value, ip_value):
        out = [f'{label}:']
        for i, reg in enumerate('ax bx cx dx si di bp sp'.split()):
            out.append(f'    mov word ptr cs:[{OUTPUT + 2 * i:#x}],{reg}')
        out += [f'    mov word ptr cs:[{OUTPUT + 16:#x}],{cs_value}', f'    mov word ptr cs:[{OUTPUT + 18:#x}],{ip_value}',
                f'    mov word ptr cs:[{OUTPUT + 20:#x}],ds', f'    mov word ptr cs:[{OUTPUT + 22:#x}],es', f'    mov word ptr cs:[{OUTPUT + 24:#x}],ss',
                f'    mov sp,{STACK_TOP + 0x800:#x}', '    pushf', '    pop ax', f'    mov word ptr cs:[{OUTPUT + 26:#x}],ax', '    jmp finish']
        return out
    t += capture('returned', 'cs', 'offset returned')
    for n, target in enumerate(stubs):
        t += capture(f'stub{n}', f'{CALLER_N:#x}', f'{target:#x}')
    t += ['finish:', '    cld', '    push cs', '    pop ds', '    push cs', '    pop es', f'    mov di,{OUTPUT + 28:#x}',
          f'    mov si,{WINDOW_LOW:#x}', f'    mov cx,{WINDOW_SIZE:#x}', '    rep movsb', '    sti',
          '    mov bx,word ptr [handle]', f'    mov dx,{OUTPUT:#x}', f'    mov cx,{28 + WINDOW_SIZE}', '    mov ah,0x40', '    int 0x21', '    jc failure',
          f'    mov ax,{DATA_N:#x}', '    mov ds,ax', '    xor dx,dx', '    mov cx,0x8000', '    mov ah,0x40', '    int 0x21', '    jc failure',
          '    mov dx,0x8000', '    mov cx,0x8000', '    mov ah,0x40', '    int 0x21', '    jc failure', '    push cs', '    pop ds',
          '    inc word ptr [index]', '    mov ax,word ptr [index]', f'    cmp ax,{NATIVE_CASES}', '    jb again',
          '    mov bx,word ptr [handle]', '    mov ah,0x3e', '    int 0x21', '    mov ax,0x4c00', '    int 0x21',
          'failure:', '    mov ax,0x4c01', '    int 0x21',
          'filename: .asciz "RESULT.BIN"', 'handle: .word 0', 'index: .word 0', 'entry_index: .word 0', f'target_far: .word {ENTRY:#x}, {CALLER_N:#x}',
          'entry_table: .word ' + ','.join(f'{e:#x}' for e in ENTRIES),
          'flags_table: .word ' + ','.join(f'{f:#x}' for f in NATIVE_FLAGS)]
    for label, blob, where in (('wrap', span, WRAP_AT), ('cases', table, CASES_AT)):
        t += [f'.org {where - 0x100:#x}', f'{label}:']
        for i in range(0, len(blob), 16):
            t.append('    .byte ' + ','.join(f'{x:#x}' for x in blob[i:i + 16]))
    asm, obj = directory / 'oracle.S', directory / 'oracle.o'
    asm.write_text('\n'.join(t) + '\n')
    subprocess.run(['as', '--32', str(asm), '-o', str(obj)], check=True)
    subprocess.run(['ld', '-m', 'elf_i386', '-Ttext', '0x100', '--oformat', 'binary', str(obj), '-o', str(directory / 'ORACLE.COM')], check=True)
    config = directory / 'dosbox.conf'
    config.write_text('[sdl]\noutput=surface\n[dosbox]\nmemsize=16\n[cpu]\ncore=normal\ncputype=386\ncycles=max\n'
                      '[mixer]\nnosound=true\n[midi]\nmididevice=none\n'
                      '[autoexec]\nmount c ' + str(directory.resolve()) + '\nc:\nORACLE.COM\nexit\n')
    result_path = directory / 'RESULT.BIN'
    if result_path.exists():
        result_path.unlink()
    try:
        with (directory / 'dosbox.log').open('w') as log:
            subprocess.run(['dosbox-x', '-conf', str(config), '-fastlaunch', '-exit', '-noconsole'],
                           env=dict(os.environ, SDL_VIDEODRIVER='dummy', SDL_AUDIODRIVER='dummy'), stdout=log, stderr=subprocess.STDOUT, check=True, timeout=300)
    finally:                                         # a spinning oracle can write gigabytes of log: keep the head only
        log_path = directory / 'dosbox.log'
        if log_path.exists() and log_path.stat().st_size > (50 << 20):
            with log_path.open('r+b') as log:
                log.truncate(1 << 20)
    result = result_path.read_bytes()
    check(len(result) == 8 + NATIVE_CASES * RECORD_BYTES, f'native output incomplete: {len(result)} bytes')
    segment, returned, count, _ = struct.unpack_from('<4H', result)
    check(count == NATIVE_CASES and segment == 0x0814, f'native header differs or COM segment {segment:04x} is not 0814')
    seen_status = {}
    for n, case in enumerate(cases):
        record = result[8 + n * RECORD_BYTES:8 + (n + 1) * RECORD_BYTES]
        memory = bytearray(SIZE)
        at = physical(CALLER_N, ENTRY)
        memory[at:at + len(span)] = span
        data = bytearray([0xA5]) * 0x10000
        for o, v in case['writes']:
            struct.pack_into('<H', data, o, v)
        memory[physical(DATA_N, 0):physical(DATA_N, 0) + 0x10000] = data
        for i in range(WINDOW_SIZE):
            memory[physical(segment, WINDOW_LOW + i)] = 0xA5
        sp = STACK_TOP
        for value in (0x0100,):                                      # the near-call frame: the return IP, where a JMP FAR stub captures the state
            sp -= 2
            put16(memory, segment, sp, value)
        put16(memory, segment, sp - 2, NATIVE_FLAGS[case['flags']])       # the harness' PUSHed flag word stays below the entry SP
        regs = list(ENTRY_REGS.values())
        cpu = Cpu(*regs, sp, CALLER_N, ENTRIES[case['entry']], DATA_N, DATA_N, segment, NATIVE_FLAGS[case['flags']])
        ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
        status = function(ram, SIZE, ctypes.byref(cpu))
        check(status in NAMES, f'C status {status} on native case {n}')
        seen_status[status] = seen_status.get(status, 0) + 1
        for (reg, _), want in zip(Cpu._fields_, struct.unpack_from('<14H', record)):
            check(getattr(cpu, reg) == want, f'native case {n} {reg}: C={getattr(cpu, reg):04x}, DOS={want:04x}')
        got = bytes(ram[physical(segment, WINDOW_LOW):physical(segment, WINDOW_LOW) + WINDOW_SIZE]) + bytes(ram[physical(DATA_N, 0):physical(DATA_N, 0) + 0x10000])
        if got != record[28:]:
            differences = [i for i, (x, y) in enumerate(zip(got, record[28:])) if x != y]
            raise ValueError(f'native case {n} bytes differ at {[hex(d) for d in differences[:8]]} ({len(differences)} bytes)')
    check(set(seen_status) == set(NAMES), f'native stops reached {sorted(seen_status)}')
    probe = subprocess.run(['dosbox-x', '--version'], capture_output=True, text=True)
    version = [l.split(',')[0].strip() for l in (probe.stdout + probe.stderr).splitlines() if l.strip().startswith('DOSBox-X version')][:1]
    binary = subprocess.run(['which', 'dosbox-x'], capture_output=True, text=True).stdout.strip()
    digest = sha256(result).hexdigest()
    names = NAMES
    report = dict(dosbox_x=dict(path=binary, sha256=sha256(Path(binary).read_bytes()).hexdigest(), version=version[0] if version else '',
                                pinned_source='5a9c7c53481c2e933948039c33256776653f4faf (not used: declared substitution)'),
                  cases=NATIVE_CASES, result_sha256=digest, cpu_words=14, ram_bytes=WINDOW_SIZE + 65536,
                  masked_ram_bytes=SIZE - WINDOW_SIZE - 65536, synthetic_frames=True, callee_stubs=len(stubs),
                  callee_stub_kind='JMP FAR into the capture harness at the planted return IP 0100',
                  stops={names[k]: v for k, v in sorted(seen_status.items())}, com_segment=f'{segment:04x}')
    print('14B6 native parity', json.dumps(report, sort_keys=True), flush=True)
    return report


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--compile-only', action='store_true')
    p.add_argument('--unicorn', action='store_true')
    p.add_argument('--native', action='store_true')
    p.add_argument('--output', type=Path)
    a = p.parse_args()
    report = {}
    with tempfile.TemporaryDirectory(prefix='poolrad-filecheck-') as tmp:
        fn = library(Path(tmp))
        if a.compile_only:
            print('CHECKER_BUILD_OK')
            return
        span, starts, conditional = source()
        report['target'] = dict(bytes=len(span), starts=len(starts), sha256=BODY_SHA, expanded_sha256=EXPANDED_SHA)
        report['ownership'] = ownership()
        if a.unicorn:
            report['unicorn'] = differential(span, starts, conditional, fn)
            print('Original parity', json.dumps(report['unicorn'], sort_keys=True), flush=True)
        if a.native:
            report['native'] = native_audit(span, fn)
    if a.output:
        a.output.parent.mkdir(parents=True, exist_ok=True)
        a.output.write_text(json.dumps(report, indent=2, sort_keys=True) + '\n')


if __name__ == '__main__':
    main()
