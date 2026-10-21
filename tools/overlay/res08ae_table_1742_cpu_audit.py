#!/usr/bin/env python3
"""RES_08AE +1742 original-byte parity: unmasked Unicorn CPU14 + 1 MiB; native DOSBox-X corroboration.

The bytes come from the hash-checked loader-expanded START.EXE image. The Unicorn run executes the original at
CODE:1742 (the relocated segment immediate at CODE:174B names a data segment) until the RETF has run, then compares
all 14 CPU words and the whole 1 MiB RAM with the C model, with no mask. For the cases that do not alias the stack,
an independent Python reference of the documented table effect (the offsets of the merged resident_8ae_table_1742
model) is also compared with the original's final RAM. Media stays private.
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
from resident_loader_map import unpack, relocations
from u86_procvars_3413_audit import Cpu, ROOT, SIZE, EXE_SHA, OVR_SHA, REGISTERS, physical, put16

CHECKER_ROWS = ('RES_08AE_1742',)
ENTRY, END, RETF = 0x1742, 0x17AE, 0x17AB
CODE, DEFAULT_DATA = 0x8800, 0x5000
BODY_SHA = '6128c5126be22f692937ccbb5e5eb2b07c85081a04f086e9bb84579486258484'
EXPANDED_SHA = 'a42a57e68d3f7815831821cc9365bb917befda45a58e958544451fc7a878df8e'
FLAGS = (0x0002, 0x00D7, 0x08D7, 0x0402, 0x0893, 0x7202, 0x0ED7)
NEAR_CALLERS = [0x107, 0x132, 0x144, 0x170]       # 00DC's near calls to this entry (census asserted below)
MASK = SIZE - 1


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
    check(sha256(span).hexdigest() == BODY_SHA and len(insns) == 43 and sum(i.size for i in insns) == 108 and
          (insns[-1].address, insns[-1].mnemonic, insns[-1].op_str) == (RETF, 'retf', '2'), 'span/return differs')
    check(sum(1 for i in insns if i.mnemonic in ('call', 'lcall', 'int', 'in', 'out')) == 0, 'the body must make no call and no port access')
    rels, _ = relocations(exe, expanded)
    check(physical(0x08AE, 0x174B) in rels and not any(physical(0x08AE, a) in rels for a in range(ENTRY, END) if a != 0x174B),
          'exactly one loader relocation, the DS segment immediate at 174B')
    conditional = {i.address: (i.address + i.size, int(i.op_str, 16)) for i in insns if i.mnemonic.startswith('j') and i.mnemonic != 'jmp'}
    check(conditional == {0x1770: (0x1772, 0x1794), 0x177C: (0x177E, 0x1775), 0x179E: (0x17A0, 0x1769)}, 'conditional jumps')
    return span, {i.address for i in insns}, conditional


def ownership():
    from overlay_inventory import find_units, parse_mz
    exe = (ROOT / 'resources/DOS/POOLRAD/START.EXE').read_bytes()
    ovr = (ROOT / 'resources/DOS/POOLRAD/GAME.OVR').read_bytes()
    check(sha256(ovr).hexdigest() == OVR_SHA, 'GAME.OVR provenance')
    direct = []
    for unit in find_units(exe[parse_mz(exe)['header_bytes']:], ovr):
        for fix in unit['fixups']:
            at = unit['ovr_offset'] + fix - 3
            call = ovr[at:at + 5]
            if len(call) == 5 and call[0] == 0x9A and struct.unpack_from('<HH', call, 1) == (ENTRY, 0x08AE):
                direct.append(f"{unit['stub_para']:04X}:{fix - 3:04X}")
    check(not direct, f'direct overlay far calls to 08AE:1742 appeared: {direct}')
    image = bytes(unpack(exe)[0])
    callers = [i.address for i in Cs(CS_ARCH_X86, CS_MODE_16).disasm(image[0x8BBC:0x8C5B], 0xDC) if i.mnemonic == 'call' and i.op_str == '0x1742']
    check(callers == NEAR_CALLERS, f'00DC near callers {[hex(a) for a in callers]}')
    return dict(overlay_far_call_fixups=0, resident_00dc_near_sites=[f'{a:04X}' for a in callers], identity='unresolved', credit_claimed=False)


def library(directory):
    path = directory / '1742.so'
    subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Wpedantic', '-Werror', '-O2', '-fPIC', '-shared', '-I', str(ROOT / 'include'),
                    str(ROOT / 'src/recovered/resident_08ae_table_1742_cpu.c'), '-o', str(path)], check=True)
    fn = ctypes.CDLL(str(path)).pr_r08ae_table_1742_cpu
    fn.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t, ctypes.POINTER(Cpu)]
    fn.restype = ctypes.c_int
    return fn


def fixture(span, rng, arg, *, ss=0x4000, sp=0x8000, data=DEFAULT_DATA, flags=2, background=None, writes=(), bytes_at=None, bytes_next=None):
    memory = bytearray([0xA5]) * SIZE if background is None else bytearray(background)
    at = physical(CODE, ENTRY)
    memory[at:at + len(span)] = span
    put16(memory, CODE, 0x174B, data)
    for off, value in ((0, 0x7000), (2, 0x9000), (4, arg)):
        put16(memory, ss, (sp + off) & 0xFFFF, value)
    for off, value in writes:                                      # (offset in the data segment, word)
        put16(memory, data, off & 0xFFFF, value)
    for at_byte in (bytes_at, bytes_next):                         # absolute (physical address, byte) pairs
        if at_byte is not None:
            memory[at_byte[0] & MASK] = at_byte[1]
    cpu = Cpu(*(rng.randrange(65536) for _ in range(7)), sp, CODE, ENTRY, rng.randrange(65536), rng.randrange(65536), ss, flags)
    return memory, cpu


def reference(before, data, arg):
    """The documented effect on the data segment, written independently of the C model's control flow (the offsets
    come from the merged resident_8ae_table_1742 model): returns the expected final RAM."""
    mem = bytearray(before)
    base = data << 4

    def p(o):
        return (base + (o & 0xFFFF)) & MASK

    def rw(o):
        return mem[p(o)] | (mem[(p(o) + 1) & MASK] << 8)

    def ww(o, v):
        mem[p(o)] = v & 0xFF
        mem[(p(o) + 1) & MASK] = (v >> 8) & 0xFF
    ww(0x1FF, arg)
    si, di = (8 * arg) & 0xFFFF, 0x22D
    mem[p(0x1D8)], mem[p(0x1D9)] = 1, 4
    for row in range(4):
        value = rw(si + 0x55F)
        if value:
            for k in range(0x2E, -2, -2):
                ww(di + k, 0)
            ww(di + 2, value)
            mem[p(0x209 + (4 - mem[p(0x1D9)]))] = rw(0x1FF) & 0xFF
            ww(di, 1)
        di, si = (di + 0x30) & 0xFFFF, (si + 2) & 0xFFFF
        mem[p(0x1D9)] = (mem[p(0x1D9)] - 1) & 0xFF
    mem[p(0x1D8)] = 0
    return bytes(mem)


def compare(memory, cpu, fn, label, conditional, ref=None):
    from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE
    oracle = Uc(UC_ARCH_X86, UC_MODE_16)
    shared = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
    for base in (0, SIZE):                                          # two mappings of the same bytes give the A20-off alias
        oracle.mem_map_ptr(base, SIZE, 7, ctypes.addressof(shared))
    for (name, _), reg in zip(Cpu._fields_, REGISTERS):
        oracle.reg_write(reg, getattr(cpu, name))
    seen, edges = set(), set()
    state = dict(returned=False, stopped=False, previous=None)

    def hook(uc, address, size, user):
        if state['returned']:
            state['stopped'] = True
            uc.emu_stop()
            return
        ip = address - (CODE << 4)
        check(ENTRY <= ip < END, f'{label}: escaped before RETF: {ip:x}')
        if state['previous'] in conditional:
            edges.add((state['previous'], ip))
        seen.add(ip)
        state['previous'] = ip
        state['returned'] = ip == RETF

    oracle.hook_add(UC_HOOK_CODE, hook)
    oracle.emu_start(physical(CODE, ENTRY), 2 * SIZE, count=5000)
    check(state['stopped'], f'{label}: missing RETURN stop')
    ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
    check(fn(ram, SIZE, ctypes.byref(cpu)) == 0, f'{label}: C status')
    for (name, _), reg in zip(Cpu._fields_, REGISTERS):
        want = oracle.reg_read(reg) & 65535
        check(getattr(cpu, name) == want, f'{label}: {name}: C={getattr(cpu, name):04x} oracle={want:04x}')
    if bytes(ram) != bytes(shared):
        diff = [i for i, (a, b) in enumerate(zip(bytes(ram), bytes(shared))) if a != b]
        raise ValueError(f'{label}: full RAM differs at {[hex(d) for d in diff[:8]]} ({len(diff)} bytes)')
    if ref is not None:                    # the reference models the data segment; the stack of these cases is elsewhere
        data, expected = ref
        lo = data << 4
        check(expected[lo:lo + 0x10000] == bytes(shared)[lo:lo + 0x10000], f'{label}: independent reference differs from the original')
    return seen, edges


def alias_arguments():
    """Arguments whose row-i source word lands on a chosen data address t: 8*A = t - 055F - 2*i (mod 65536), solvable
    when the right side is a multiple of 8; A and A + 8192*j give the same table, so the high bits vary."""
    out = []
    for target in (0x01D8, 0x01D9, 0x01FF, 0x0209, 0x022D, 0x022F, 0x0231, 0x025B, 0x025D, 0x025F, 0x028D, 0x02BD, 0x02BF, 0x02EB):
        for row in range(4):
            need = (target - 0x055F - 2 * row) & 0xFFFF
            if need % 8 == 0:
                out.append((target, row, need // 8))
    return out


def differential(span, starts, conditional, fn):
    rng = random.Random(0x08AE1742)
    cases = []                                                      # (label, arg, kwargs, plain)
    args = (0, 1, 2, 5, 0x00FF, 0x0100, 0x0A02, 0x1234, 0x1FA0, 0x1FFF, 0x2000, 0x7FFF, 0x8000, 0xFFFF)
    for arg in args:
        for mask in range(16):
            writes = [((0x55F + 8 * arg + 2 * i) & 0xFFFF, (rng.randrange(1, 65536) if mask >> i & 1 else 0)) for i in range(4)]
            for o in range(0x22D, 0x2ED, 2):
                writes.append((o, rng.randrange(65536)))
            writes.append((0x1FF, 0x5A5A))
            cases.append((f'arg-{arg:04x}-mask-{mask:x}', arg, dict(flags=FLAGS[mask % len(FLAGS)], writes=writes), True))
    for target, row, base in alias_arguments():
        for j in (0, 3, 7):
            arg = base + 8192 * j
            writes = [(0x227, 0x5555), (0x229, 0), (0x22B, 0), (0x22D, 0), (0x1D8, 0), (target, rng.randrange(65536))]
            cases.append((f'alias-t{target:04x}-r{row}-j{j}', arg, dict(writes=writes, flags=0x0ED7), True))
    # Row 0's source word at DS:FFFF (A = 1F54h) straddles into the next paragraph; the byte at DS:10000h is the first byte
    # of the segment that follows, so the high half of the word is outside the 64 KiB data segment.
    for j in (0, 1, 7):
        for low, high in ((0x34, 0x12), (0x00, 0x00), (0x00, 0x80), (0xFF, 0xFF)):
            arg = 0x1F54 + 8192 * j
            memory_writes = [(0x055F + 8 * 0x1F54 + 2 * i & 0xFFFF, 0x1111 * (i + 1)) for i in range(1, 4)]
            cases.append((f'straddle-ffff-j{j}-{low:02x}{high:02x}', arg, dict(writes=memory_writes, flags=0x0ED7,
                          bytes_at=((DEFAULT_DATA << 4) + 0xFFFF, low), bytes_next=((DEFAULT_DATA << 4) + 0x10000, high)), True))
    for sp in range(0x1C0, 0x300, 2):                               # the stack lives in the data segment
        for arg in (2, 0x0A02):
            writes = [((0x55F + 8 * arg + 2 * i) & 0xFFFF, 0x1234 + i) for i in range(4)]
            cases.append((f'stack-in-data-{sp:04x}-{arg:04x}', arg, dict(ss=DEFAULT_DATA, sp=sp, writes=writes, flags=0x0202), False))
    for sp in (0x0C, 0x0D, 0xFFFA, 0xFFF9):
        cases.append((f'sp-{sp:04x}', 0x0102, dict(sp=sp), False))
    cases += [('a20-stack', 0x0002, dict(ss=0xFFFF, sp=0x0013), False), ('a20-data', 0x0003, dict(data=0xFFFF), False),
              ('data-at-code', 0x0004, dict(data=CODE), False),
              ('random-ram', 0x9876, dict(background=rng.randbytes(SIZE)), False)]
    seen, edges, plain = set(), set(), 0
    for label, arg, kw, is_plain in cases:
        memory, cpu = fixture(span, rng, arg, **kw)
        ref = None
        if is_plain:
            data = kw.get('data', DEFAULT_DATA)
            ref = (data, reference(bytes(memory), data, arg))
            plain += 1
        s, e = compare(memory, cpu, fn, label, conditional, ref)
        seen |= s
        edges |= e
    check(seen == starts, f'starts missing: {sorted(starts - seen)}')
    wanted = {(a, b) for a, pair in conditional.items() for b in pair}
    check(edges >= wanted, f'outcomes missing: {sorted(wanted - edges)}')
    memory, base = fixture(span, rng, 0x1234)
    rejects = 0
    for sp, ip, size in ((0x000B, ENTRY, SIZE), (0xFFFB, ENTRY, SIZE), (0x8000, ENTRY + 1, SIZE), (0x8000, ENTRY, SIZE - 1)):
        cpu = Cpu.from_buffer_copy(base)
        cpu.sp, cpu.ip = sp, ip
        ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
        before = bytes(cpu)
        check(fn(ram, size, ctypes.byref(cpu)) == 1 and bytes(cpu) == before and bytes(ram) == memory, 'ARGUMENT mutation')
        rejects += 1
    return dict(return_cases=len(cases), reference_checked_cases=plain, argument_cases=rejects, starts=len(seen), starts_total=len(starts),
                conditional_outcomes=len(edges & wanted), conditional_outcomes_total=len(wanted), calls=0, cpu_words=14, ram_bytes=SIZE,
                masked_ram_bytes=0, stops={'RETURN': len(cases), 'ARGUMENT': rejects})


# ---- DOSBox-X synthetic oracle -------------------------------------------------------------------------
CALLER_N, DATA_N = 0x8800, 0x5000
MAIN_AT, WRAP_AT, CASES_AT = 0x0300, 0x2600, 0x2700
OUTPUT, HEADER, STACK_TOP = 0xC000, 0xD800, 0xF000
WINDOW_LOW, WINDOW_SIZE = STACK_TOP - 0x100, 0x100
NATIVE_CASES, CASE_SIZE = 48, 0x200
RECORD_BYTES = 28 + WINDOW_SIZE + 0x10000
NATIVE_FLAGS = (0x0002, 0x00D7, 0x08D7, 0x0402, 0x0893, 0x0003)
ENTRY_REGS = dict(ax=0x1234, bx=0x3456, cx=0x5678, dx=0x789A, si=0x9ABC, di=0xBCDE, bp=0x7777)


def native_cases():
    rng = random.Random(0x08AE1742D)
    cases = []
    args = (0, 1, 2, 5, 0x00FF, 0x0100, 0x0A02, 0x1234, 0x1FA0, 0x7FFF, 0xFFFF)
    for n in range(NATIVE_CASES - 8):
        arg = args[n % len(args)]
        mask = (n * 7 + 3) % 16
        writes = [((0x55F + 8 * arg + 2 * i) & 0xFFFF, (rng.randrange(1, 65536) if mask >> i & 1 else 0)) for i in range(4)]
        writes += [(o, 0x7777) for o in range(0x22D, 0x2ED, 2)]
        cases.append(dict(arg=arg, writes=writes, flags=n % len(NATIVE_FLAGS)))
    for target, row, base in alias_arguments()[:8]:
        cases.append(dict(arg=base + 8192 * (len(cases) % 8), writes=[(0x227, 0x5555), (0x229, 0), (0x22B, 0), (0x22D, 0), (target, 0x2222)],
                          flags=len(cases) % len(NATIVE_FLAGS)))
    return cases[:NATIVE_CASES]


def native_audit(span, function):
    span = bytearray(span)
    struct.pack_into('<H', span, 0x174B - ENTRY, DATA_N)
    span = bytes(span)
    directory = ROOT / 'resources/oracle-runs/res08ae-1742-synthetic'
    directory.mkdir(parents=True, exist_ok=True)
    cases = native_cases()
    table = b''.join((struct.pack('<BBHHHH', c['flags'], 0, len(c['writes']), c['arg'], 0, 0) +
                      b''.join(struct.pack('<HH', o, v) for o, v in c['writes'])).ljust(CASE_SIZE, b'\0') for c in cases)
    check(all(len(c['writes']) * 4 + 10 <= CASE_SIZE for c in cases), 'case rows overflow')
    check(CASES_AT + len(table) <= OUTPUT, 'COM layout overlaps')
    t = ['.code16', '.intel_syntax noprefix', '.global _start', '_start:', '    jmp main', f'.org {MAIN_AT - 0x100:#x}', 'main:',
         '    mov ax,cs', '    mov ds,ax', '    mov es,ax', '    mov ss,ax', f'    mov sp,{STACK_TOP:#x}', '    cld',
         f'    mov ax,{CALLER_N:#x}', '    mov es,ax', '    mov si,offset wrap', f'    mov di,{ENTRY:#x}', f'    mov cx,{len(span):#x}', '    rep movsb',
         '    push cs', '    pop es', f'    mov word ptr [{HEADER:#x}],cs', f'    mov word ptr [{HEADER + 2:#x}],offset returned',
         f'    mov word ptr [{HEADER + 4:#x}],{NATIVE_CASES}', f'    mov word ptr [{HEADER + 6:#x}],0',
         '    mov dx,offset filename', '    xor cx,cx', '    mov ah,0x3c', '    int 0x21', '    jc failure',
         '    mov word ptr [handle],ax', '    mov bx,ax', f'    mov dx,{HEADER:#x}', '    mov cx,8', '    mov ah,0x40', '    int 0x21', '    jc failure',
         'again:', '    cli', f'    mov sp,{STACK_TOP:#x}', '    cld', '    push cs', '    pop ds', '    push cs', '    pop es',
         '    mov al,0xa5', f'    mov di,{WINDOW_LOW:#x}', f'    mov cx,{WINDOW_SIZE:#x}', '    rep stosb',
         '    mov ax,word ptr [index]', f'    mov bx,{CASE_SIZE}', '    mul bx', '    add ax,offset cases', '    mov bx,ax',
         f'    mov ax,{DATA_N:#x}', '    mov es,ax', '    xor di,di', '    mov cx,0x8000', '    mov ax,0xa5a5', '    rep stosw',
         '    mov cx,word ptr [bx+2]', '    jcxz written', '    lea si,[bx+10]', 'write_loop:', '    mov di,word ptr [si]', '    mov ax,word ptr [si+2]',
         '    mov word ptr es:[di],ax', '    add si,4', '    loop write_loop', 'written:',
         '    push word ptr [bx+4]',
         '    xor ah,ah', '    mov al,byte ptr [bx]', '    shl ax,1', '    mov si,ax',
         '    push word ptr [flags_table+si]', '    push cs', '    pop es', f'    mov ax,{DATA_N:#x}', '    mov ds,ax']
    for reg, value in ENTRY_REGS.items():
        t.append(f'    mov {reg},{value:#x}')
    t += ['    popf', '    call dword ptr cs:[target_far]', 'returned:']
    for i, reg in enumerate('ax bx cx dx si di bp sp cs'.split()):
        t.append(f'    mov word ptr cs:[{OUTPUT + 2 * i:#x}],{reg}')
    t += [f'    mov word ptr cs:[{OUTPUT + 18:#x}],offset returned', f'    mov word ptr cs:[{OUTPUT + 20:#x}],ds',
          f'    mov word ptr cs:[{OUTPUT + 22:#x}],es', f'    mov word ptr cs:[{OUTPUT + 24:#x}],ss',
          f'    mov sp,{STACK_TOP + 0x800:#x}', '    pushf', '    pop ax', f'    mov word ptr cs:[{OUTPUT + 26:#x}],ax',
          '    cld', '    push cs', '    pop ds', '    push cs', '    pop es', f'    mov di,{OUTPUT + 28:#x}',
          f'    mov si,{WINDOW_LOW:#x}', f'    mov cx,{WINDOW_SIZE:#x}', '    rep movsb', '    sti',
          '    mov bx,word ptr [handle]', f'    mov dx,{OUTPUT:#x}', f'    mov cx,{28 + WINDOW_SIZE}', '    mov ah,0x40', '    int 0x21', '    jc failure',
          f'    mov ax,{DATA_N:#x}', '    mov ds,ax', '    xor dx,dx', '    mov cx,0x8000', '    mov ah,0x40', '    int 0x21', '    jc failure',
          '    mov dx,0x8000', '    mov cx,0x8000', '    mov ah,0x40', '    int 0x21', '    jc failure', '    push cs', '    pop ds',
          '    inc word ptr [index]', '    mov ax,word ptr [index]', f'    cmp ax,{NATIVE_CASES}', '    jb again',
          '    mov bx,word ptr [handle]', '    mov ah,0x3e', '    int 0x21', '    mov ax,0x4c00', '    int 0x21',
          'failure:', '    mov ax,0x4c01', '    int 0x21',
          'filename: .asciz "RESULT.BIN"', 'handle: .word 0', 'index: .word 0', f'target_far: .word {ENTRY:#x}, {CALLER_N:#x}',
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
        for value in (case['arg'], segment, returned):
            sp -= 2
            put16(memory, segment, sp, value)
        cpu = Cpu(*ENTRY_REGS.values(), sp, CALLER_N, ENTRY, DATA_N, segment, segment, NATIVE_FLAGS[case['flags']])
        ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
        check(function(ram, SIZE, ctypes.byref(cpu)) == 0, f'C status on native case {n}')
        for (reg, _), want in zip(Cpu._fields_, struct.unpack_from('<14H', record)):
            check(getattr(cpu, reg) == want, f'native case {n} {reg}: C={getattr(cpu, reg):04x}, DOS={want:04x}')
        got = bytes(ram[physical(segment, WINDOW_LOW):physical(segment, WINDOW_LOW) + WINDOW_SIZE]) + bytes(ram[physical(DATA_N, 0):physical(DATA_N, 0) + 0x10000])
        if got != record[28:]:
            differences = [i for i, (x, y) in enumerate(zip(got, record[28:])) if x != y]
            raise ValueError(f'native case {n} bytes differ at {[hex(d) for d in differences[:8]]} ({len(differences)} bytes)')
    probe = subprocess.run(['dosbox-x', '--version'], capture_output=True, text=True)
    version = [l.split(',')[0].strip() for l in (probe.stdout + probe.stderr).splitlines() if l.strip().startswith('DOSBox-X version')][:1]
    binary = subprocess.run(['which', 'dosbox-x'], capture_output=True, text=True).stdout.strip()
    digest = sha256(result).hexdigest()
    report = dict(dosbox_x=dict(path=binary, sha256=sha256(Path(binary).read_bytes()).hexdigest(), version=version[0] if version else '',
                                pinned_source='5a9c7c53481c2e933948039c33256776653f4faf (not used: declared substitution)'),
                  cases=NATIVE_CASES, result_sha256=digest, cpu_words=14, ram_bytes=WINDOW_SIZE + 65536,
                  masked_ram_bytes=SIZE - WINDOW_SIZE - 65536, synthetic_frames=True, callee_stubs=0, com_segment=f'{segment:04x}')
    print('1742 native parity', json.dumps(report, sort_keys=True), flush=True)
    return report


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--compile-only', action='store_true')
    p.add_argument('--unicorn', action='store_true')
    p.add_argument('--native', action='store_true')
    p.add_argument('--output', type=Path)
    a = p.parse_args()
    report = {}
    with tempfile.TemporaryDirectory(prefix='poolrad-1742-') as tmp:
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
