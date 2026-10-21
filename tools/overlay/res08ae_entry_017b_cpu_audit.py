#!/usr/bin/env python3
"""RES_08AE +017B original-byte parity: unmasked Unicorn CPU14 + 1 MiB with named callee stops; native DOSBox-X with declared stubs.

The bytes come from the hash-checked loader-expanded START.EXE image. The Unicorn run executes the original at
CODE:017B (or at one of the four return points 0191/0198/019F/01A3) until either the RETF has run or the next
instruction would be a callee (137C, 1742, 17AE, 13B1), then compares all 14 CPU words and the whole 1 MiB RAM with the C
model and the stop status, with no mask. Callees are STOPS, not modeled. For the cases that do not alias the stack an
independent Python reference of the documented effect is compared too. Media stays private.
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

CHECKER_ROWS = ('RES_08AE_017B',)
ENTRY, END, RETF = 0x017B, 0x01A5, 0x01A4
ENTRIES = (0x017B, 0x0191, 0x0198, 0x019F, 0x01A3)               # the function and its four post-call return points
CALL_SITES = {0x018E: 0x137C, 0x0195: 0x1742, 0x019C: 0x17AE, 0x01A0: 0x13B1}
STOPS = {0x137C: 2, 0x1742: 3, 0x17AE: 4, 0x13B1: 5}            # callee entry -> pr_r08ae_017b_status
CODE, DEFAULT_DATA = 0x8800, 0x5000
BODY_SHA = '2cf68891044056f55a1559fb007ce30089c3bf070f44e915cb5e571cd7fa613f'
EXPANDED_SHA = 'a42a57e68d3f7815831821cc9365bb917befda45a58e958544451fc7a878df8e'
FLAGS = (0x0002, 0x00D7, 0x08D7, 0x0402, 0x0893, 0x7202, 0x0ED7)
# Resident far-call sites (9A + relocated 08AE:017B) and the function that contains each, decoded from the nearest far-call-target start.
FAR_CALLERS = {0x45B2: 0x4354, 0x8A31: 0x89E9}
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
    check(sha256(span).hexdigest() == BODY_SHA and len(insns) == 21 and sum(i.size for i in insns) == 42 and
          (insns[-1].address, insns[-1].mnemonic, insns[-1].op_str) == (RETF, 'retf', ''), 'span/return differs')
    check(not any(i.mnemonic in ('int', 'int3', 'into', 'in', 'out', 'lcall', 'ljmp') for i in insns), 'no interrupt, port or far transfer expected')
    calls = {i.address: int(i.op_str, 16) for i in insns if i.mnemonic == 'call' and i.op_str.startswith('0x')}
    check(calls == CALL_SITES and sum(1 for i in insns if i.mnemonic == 'call') == 4, f'call sites differ: {calls}')
    byaddr = {i.address: i for i in insns}
    check(all(byaddr[a - 1].mnemonic == 'push' and byaddr[a - 1].op_str == 'cs' for a in CALL_SITES), 'each call is preceded by PUSH CS')
    rels, _ = relocations(exe, expanded)
    check(not any(physical(0x08AE, a) in rels for a in range(ENTRY, END)), 'the body has no loader relocation')
    conditional = {i.address: (i.address + i.size, int(i.op_str, 16)) for i in insns if i.mnemonic.startswith('j') and i.mnemonic != 'jmp'}
    check(conditional == {0x0189: (0x018B, 0x018D)}, 'conditional jumps')
    check(not any(i.mnemonic in ('jmp', 'call') and not i.op_str.startswith('0x') for i in insns), 'indirect transfer')
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
    check(not direct, f'direct overlay far calls to 08AE:017B appeared: {direct}')
    image = bytes(unpack(exe)[0])
    rels, _ = relocations(exe, image)
    segment = image[0x8AE0:0x8AE0 + 0x10000]
    near = [o for o in range(len(segment) - 2) if segment[o] == 0xE8 and (o + 3 + struct.unpack_from('<h', segment, o + 1)[0]) & 0xFFFF == ENTRY]
    check(not near, f'near calls to 017B appeared: {near}')
    pattern = struct.pack('<HH', ENTRY, 0x08AE)
    far = [i - 1 for i in range(1, len(image) - 3) if image[i:i + 4] == pattern and image[i - 1] == 0x9A and (i + 2) in rels]
    targets = set()                                            # every relocated far-call target is a candidate function start
    for i in range(len(image) - 5):
        if image[i] == 0x9A and (i + 3) in rels:
            off, seg = struct.unpack_from('<HH', image, i + 1)
            if seg * 16 + off < len(image):
                targets.add(seg * 16 + off)
    decoded = {}
    for site in far:
        for start in sorted((t for t in targets if t <= site and site - t < 0x300), reverse=True):
            insns = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(image[start:site + 5], start))
            if insns and insns[-1].address == site and insns[-1].mnemonic == 'lcall' and sum(i.size for i in insns) == site + 5 - start:
                decoded[site] = start
                break
    check(far == sorted(FAR_CALLERS) and decoded == FAR_CALLERS,
          f'resident far-call census differs: raw {[hex(a) for a in far]}, decoded {[(hex(a), hex(f)) for a, f in sorted(decoded.items())]}')
    return dict(overlay_far_call_fixups=0, near_call_sites=[], resident_far_call_sites={f'image+{a:05X}': f'function image+{f:05X}' for a, f in sorted(decoded.items())},
                identity='unresolved', credit_claimed=False)


def library(directory):
    path = directory / '017b.so'
    subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Wpedantic', '-Werror', '-O2', '-fPIC', '-shared', '-I', str(ROOT / 'include'),
                    str(ROOT / 'src/recovered/resident_08ae_entry_017b_cpu.c'), '-o', str(path)], check=True)
    fn = ctypes.CDLL(str(path)).pr_r08ae_entry_017b_cpu
    fn.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t, ctypes.POINTER(Cpu)]
    fn.restype = ctypes.c_int
    return fn


def fixture(span, rng, entry, flag, *, ss=0x4000, sp=0x8000, data=DEFAULT_DATA, flags=2, background=None, stack_words=None, extra=()):
    memory = bytearray([0xA5]) * SIZE if background is None else bytearray(background)
    at = physical(CODE, ENTRY)
    memory[at:at + len(span)] = span
    words = stack_words or (rng.randrange(65536), 0x7000, 0x9000)       # [saved BP or return IP][return CS or beyond]
    for i, w in enumerate(words):
        put16(memory, ss, (sp + 2 * i) & 0xFFFF, w)
    memory[physical(data, 0x5372)] = flag
    for off, value in extra:                                           # (offset in the data segment, byte)
        memory[physical(data, off)] = value
    regs = [rng.randrange(65536) for _ in range(7)]
    cpu = Cpu(*regs, sp, CODE, entry, data, rng.randrange(65536), ss, flags)
    return memory, cpu


def p_flags(a, b):
    """CMP byte a,b flags, written without the C model's formulas."""
    res = (a - b) & 255
    sa, sb = a - 256 * (a >> 7), b - 256 * (b >> 7)
    flags = (1 if a < b else 0) | (4 if bin(res).count('1') % 2 == 0 else 0) | (0x10 if (a & 15) < (b & 15) else 0)
    flags |= (0x40 if res == 0 else 0) | (0x80 if res >> 7 else 0) | (0x800 if not -128 <= sa - sb <= 127 else 0)
    return flags


def reference(before, cpu_in, entry):
    """Documented effect, independent of the C model: returns (status, ax, bp, sp, ip, cs, flags&0x8D5, byte at DS:5373)."""
    data, ss, sp, bp, ax = cpu_in.ds, cpu_in.ss, cpu_in.sp, cpu_in.bp, cpu_in.ax
    flags = cpu_in.flags & 0x8D5
    w = lambda i: before[physical(ss, sp + 2 * i)] | (before[physical(ss, sp + 2 * i + 1)] << 8)
    flagbyte = before[physical(data, 0x5372)]
    out5373 = before[physical(data, 0x5373)]
    if entry == 0x017B:
        al = flagbyte
        ax = (ax & 0xFF00) | al
        out5373 = al
        flags = p_flags(al, 2)
        if al == 2:
            return 0, ax, bp, (sp + 4) & 0xFFFF, w(0), w(1), flags, out5373       # PUSH BP/POP BP cancel; RETF pops IP, CS
        return 2, ax, (sp - 2) & 0xFFFF, (sp - 6) & 0xFFFF, 0x137C, cpu_in.cs, flags, out5373
    if entry == 0x0191:
        return 3, 0, bp, (sp - 6) & 0xFFFF, 0x1742, cpu_in.cs, 0x44, out5373
    if entry == 0x0198:
        return 4, 0, bp, (sp - 6) & 0xFFFF, 0x17AE, cpu_in.cs, 0x44, out5373
    if entry == 0x019F:
        return 5, ax, bp, (sp - 4) & 0xFFFF, 0x13B1, cpu_in.cs, flags, out5373
    return 0, ax, w(0), (sp + 6) & 0xFFFF, w(1), w(2), flags, out5373


def compare(memory, cpu, fn, label, conditional, ref_entry=None):
    from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE
    from unicorn.x86_const import UC_X86_REG_CS
    oracle = Uc(UC_ARCH_X86, UC_MODE_16)
    shared = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
    for base in (0, SIZE):                                          # two mappings of the same bytes give the A20-off alias
        oracle.mem_map_ptr(base, SIZE, 7, ctypes.addressof(shared))
    for (name, _), reg in zip(Cpu._fields_, REGISTERS):
        oracle.reg_write(reg, getattr(cpu, name))
    entry = cpu.ip
    before = bytes(memory) if ref_entry is not None else None
    cpu_in = Cpu.from_buffer_copy(cpu)
    seen, edges = set(), set()
    state = dict(returned=False, stop=None, stopped=False, previous=None)

    def hook(uc, address, size, user):
        cs = uc.reg_read(UC_X86_REG_CS)
        ip = (address - (cs << 4)) & 0xFFFFF
        if state['returned']:
            state['stopped'] = True
            uc.emu_stop()
            return
        if cs == CODE and ip in STOPS and state['previous'] is not None and state['previous'] in CALL_SITES:
            state['stop'] = STOPS[ip]
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
    oracle.emu_start(physical(CODE, entry), 2 * SIZE, count=200)
    check(state['stopped'], f'{label}: neither RETURN nor a callee stop')
    want_status = 0 if state['stop'] is None else state['stop']
    ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
    got = fn(ram, SIZE, ctypes.byref(cpu))
    check(got == want_status, f'{label}: C status {got}, original {want_status}')
    for (name, _), reg in zip(Cpu._fields_, REGISTERS):
        want = oracle.reg_read(reg) & 65535
        check(getattr(cpu, name) == want, f'{label}: {name}: C={getattr(cpu, name):04x} oracle={want:04x}')
    if bytes(ram) != bytes(shared):
        diff = [i for i, (a, b) in enumerate(zip(bytes(ram), bytes(shared))) if a != b]
        raise ValueError(f'{label}: full RAM differs at {[hex(d) for d in diff[:8]]} ({len(diff)} bytes)')
    if ref_entry is not None:
        st, ax, bp, sp, ip, cs, fl, b5373 = reference(before, cpu_in, entry)
        final = Cpu.from_buffer_copy(cpu)
        check((st, final.ax, final.bp, final.sp, final.ip, final.cs, final.flags & 0x8D5, bytes(shared)[physical(cpu_in.ds, 0x5373)]) ==
              (st, ax, bp, sp, ip, cs, fl, b5373) and got == st, f'{label}: independent reference differs from the original')
    return seen, edges, got


def differential(span, starts, conditional, fn):
    rng = random.Random(0x08AE017B)
    cases = []                                                      # (label, entry, flag, kwargs, plain)
    for entry in ENTRIES:
        for flag in (0, 1, 2, 3, 0x7F, 0x80, 0x82, 0xFF):
            for n, fl in enumerate(FLAGS):
                cases.append((f'e{entry:04x}-f{flag:02x}-fl{n}', entry, flag, dict(flags=fl), True))
        for sp in (0x0006, 0x0007, 0xFFFA, 0xFFF9, 0x0100):
            cases.append((f'e{entry:04x}-sp{sp:04x}', entry, 2, dict(sp=sp), True))
        for flag in (0, 2):                                         # the stack lives on DS:5372/5373: pushes and the store alias
            for sp in range(0x5360, 0x5386):
                cases.append((f'e{entry:04x}-stack-in-data-{sp:04x}-f{flag:x}', entry, flag, dict(ss=DEFAULT_DATA, sp=sp, flags=0x0202), False))
        for data in (0x0000, 0xFFFF, 0x1234, CODE):
            cases.append((f'e{entry:04x}-ds{data:04x}', entry, 2 if data != CODE else 0, dict(data=data), data not in (0xFFFF,)))
        cases.append((f'e{entry:04x}-a20-stack', entry, 0, dict(ss=0xFFFF, sp=0x0013), False))
        cases.append((f'e{entry:04x}-random-ram', entry, 0, dict(background=rng.randbytes(SIZE)), False))
        cases.append((f'e{entry:04x}-ret-frame-on-5372', entry, 0, dict(ss=DEFAULT_DATA, sp=0x536E, stack_words=(0x1111, 0x2222, 0x3333, 0x4444), flags=0x0ED7), False))
    seen, edges, statuses, plain = set(), set(), {}, 0
    for label, entry, flag, kw, is_plain in cases:
        memory, cpu = fixture(span, rng, entry, flag, **kw)
        s, e, status = compare(memory, cpu, fn, label, conditional, entry if is_plain and kw.get('ss', 0x4000) != kw.get('data', DEFAULT_DATA) else None)
        plain += 1 if is_plain and kw.get('ss', 0x4000) != kw.get('data', DEFAULT_DATA) else 0
        statuses[status] = statuses.get(status, 0) + 1
        seen |= s
        edges |= e
    check(seen == starts, f'starts missing: {sorted(starts - seen)}')
    wanted = {(a, b) for a, pair in conditional.items() for b in pair}
    check(edges >= wanted, f'outcomes missing: {sorted(wanted - edges)}')
    check(set(statuses) == {0, 2, 3, 4, 5}, f'stops reached {sorted(statuses)}')
    memory, base = fixture(span, rng, ENTRY, 0)
    rejects = 0
    for sp, ip, size in ((0x0005, ENTRY, SIZE), (0xFFFB, ENTRY, SIZE), (0x8000, ENTRY + 1, SIZE), (0x8000, 0x1A4, SIZE), (0x8000, ENTRY, SIZE - 1)):
        cpu = Cpu.from_buffer_copy(base)
        cpu.sp, cpu.ip = sp, ip
        ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
        before = bytes(cpu)
        check(fn(ram, size, ctypes.byref(cpu)) == 1 and bytes(cpu) == before and bytes(ram) == memory, 'ARGUMENT mutation')
        rejects += 1
    names = {0: 'RETURN', 2: 'CALL_137C', 3: 'CALL_1742', 4: 'CALL_17AE', 5: 'CALL_13B1'}
    return dict(cases=len(cases), reference_checked_cases=plain, argument_cases=rejects, starts=len(seen), starts_total=len(starts),
                conditional_outcomes=len(edges & wanted), conditional_outcomes_total=len(wanted), calls=0, cpu_words=14, ram_bytes=SIZE,
                masked_ram_bytes=0, stops={names[k]: v for k, v in sorted(statuses.items())} | {'ARGUMENT': rejects})


# ---- DOSBox-X synthetic oracle (callee stubs DECLARED) -------------------------------------------------
CALLER_N, DATA_N = 0x8800, 0x5000
MAIN_AT, WRAP_AT, CASES_AT = 0x0300, 0x2600, 0x2700
OUTPUT, HEADER, STACK_TOP = 0xC000, 0xD800, 0xF000
WINDOW_LOW, WINDOW_SIZE = STACK_TOP - 0x100, 0x100
NATIVE_CASES, CASE_SIZE = 48, 0x100
RECORD_BYTES = 28 + WINDOW_SIZE + 0x10000
NATIVE_FLAGS = (0x0002, 0x00D7, 0x08D7, 0x0402, 0x0893, 0x0003)
ENTRY_REGS = dict(ax=0x1234, bx=0x3456, cx=0x5678, dx=0x789A, si=0x9ABC, di=0xBCDE, bp=0x7777)
NATIVE_FLAG_BYTES = (0, 1, 2, 3, 0x7F, 0x80, 0x82, 0xFF, 0xA5)


def native_cases():
    cases = []
    for n in range(NATIVE_CASES):
        entry_index = n // 9 if n < 45 else 0
        flag = NATIVE_FLAG_BYTES[n % 9] if n < 45 else 2
        cases.append(dict(entry=entry_index, extra=1 if entry_index else 0, flags=n % len(NATIVE_FLAGS), writes=[(0x5372, flag | 0xA500)]))
    return cases


def native_audit(span, function):
    directory = ROOT / 'resources/oracle-runs/res08ae-017b-synthetic'
    directory.mkdir(parents=True, exist_ok=True)
    cases = native_cases()
    table = b''.join((struct.pack('<BBHHHH', c['flags'], 0, len(c['writes']), c['entry'], c['extra'], 0) +
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
          '    mov ax,word ptr [bx+4]', '    shl ax,1', '    mov si,ax', '    mov ax,word ptr [entry_table+si]', '    mov word ptr [target_far],ax',
          '    push cs', '    mov ax,offset returned', '    push ax', '    cmp word ptr [bx+6],0', '    je no_extra', '    mov ax,0x7777', '    push ax', 'no_extra:',
          '    xor ah,ah', '    mov al,byte ptr [bx]', '    shl ax,1', '    mov si,ax',
          '    push word ptr [flags_table+si]', '    mov ax,cs', '    mov es,ax', f'    mov ax,{DATA_N:#x}', '    mov ds,ax']
    for reg, value in ENTRY_REGS.items():
        t.append(f'    mov {reg},{value:#x}')
    t += ['    popf', '    jmp dword ptr cs:[target_far]']

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
          'filename: .asciz "RESULT.BIN"', 'handle: .word 0', 'index: .word 0', f'target_far: .word {ENTRY:#x}, {CALLER_N:#x}',
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
        for value in (segment, returned) + ((0x7777,) if case['extra'] else ()):
            sp -= 2
            put16(memory, segment, sp, value)
        put16(memory, segment, sp - 2, NATIVE_FLAGS[case['flags']])       # the harness' PUSHed flag word stays below the entry SP
        cpu = Cpu(*ENTRY_REGS.values(), sp, CALLER_N, ENTRIES[case['entry']], DATA_N, segment, segment, NATIVE_FLAGS[case['flags']])
        ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
        status = function(ram, SIZE, ctypes.byref(cpu))
        check(status in (0, 2, 3, 4, 5), f'C status {status} on native case {n}')
        seen_status[status] = seen_status.get(status, 0) + 1
        for (reg, _), want in zip(Cpu._fields_, struct.unpack_from('<14H', record)):
            check(getattr(cpu, reg) == want, f'native case {n} {reg}: C={getattr(cpu, reg):04x}, DOS={want:04x}')
        got = bytes(ram[physical(segment, WINDOW_LOW):physical(segment, WINDOW_LOW) + WINDOW_SIZE]) + bytes(ram[physical(DATA_N, 0):physical(DATA_N, 0) + 0x10000])
        if got != record[28:]:
            differences = [i for i, (x, y) in enumerate(zip(got, record[28:])) if x != y]
            raise ValueError(f'native case {n} bytes differ at {[hex(d) for d in differences[:8]]} ({len(differences)} bytes)')
    check(set(seen_status) == {0, 2, 3, 4, 5}, f'native stops reached {sorted(seen_status)}')
    probe = subprocess.run(['dosbox-x', '--version'], capture_output=True, text=True)
    version = [l.split(',')[0].strip() for l in (probe.stdout + probe.stderr).splitlines() if l.strip().startswith('DOSBox-X version')][:1]
    binary = subprocess.run(['which', 'dosbox-x'], capture_output=True, text=True).stdout.strip()
    digest = sha256(result).hexdigest()
    names = {0: 'RETURN', 2: 'CALL_137C', 3: 'CALL_1742', 4: 'CALL_17AE', 5: 'CALL_13B1'}
    report = dict(dosbox_x=dict(path=binary, sha256=sha256(Path(binary).read_bytes()).hexdigest(), version=version[0] if version else '',
                                pinned_source='5a9c7c53481c2e933948039c33256776653f4faf (not used: declared substitution)'),
                  cases=NATIVE_CASES, result_sha256=digest, cpu_words=14, ram_bytes=WINDOW_SIZE + 65536,
                  masked_ram_bytes=SIZE - WINDOW_SIZE - 65536, synthetic_frames=True, callee_stubs=len(stubs),
                  callee_stub_kind='JMP FAR into the capture harness at each callee entry (137C, 1742, 17AE, 13B1); the callees are not executed',
                  stops={names[k]: v for k, v in sorted(seen_status.items())}, com_segment=f'{segment:04x}')
    print('017B native parity', json.dumps(report, sort_keys=True), flush=True)
    return report


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--compile-only', action='store_true')
    p.add_argument('--unicorn', action='store_true')
    p.add_argument('--native', action='store_true')
    p.add_argument('--output', type=Path)
    a = p.parse_args()
    report = {}
    with tempfile.TemporaryDirectory(prefix='poolrad-017b-') as tmp:
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
