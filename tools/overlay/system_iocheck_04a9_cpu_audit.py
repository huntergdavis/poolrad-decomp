#!/usr/bin/env python3
"""SYSTEM 0AF8:04A9 (Turbo Pascal I/O check; raw CPU, runtime coverage, no identity claim) against the original bytes.

The bytes come from the hash-checked loader-expanded START.EXE image. The Unicorn run executes the original at CODE:04A9 until
either the RETF has run or the JMP to 00E2 (RunError) has executed (the TERMINAL stop: nothing at 00E2 runs), then compares all
14 CPU words, the whole 1 MiB RAM and the stop status with the C model, with no mask. The checker also asserts, from the
bytes, that no path from 00E2 returns to the caller. Native DOSBox-X replays the original with a declared stub at 00E2. Media
stays private."""
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

CHECKER_ROWS = ('RES_0AF8_04A9',)
SEG = 0x0AF8
ENTRY, END, RETF = 0x04A9, 0x04B7, 0x04B0
ENTRIES = (0x04A9,)
CALL_SITES = {0x04B4: 0x00E2}                                    # the JMP at 04B4 is the terminal transfer
STOPS = {0x00E2: 2}                                              # RunError entry -> pr_system_iocheck_04a9 status RUNERROR
NAMES = {0: 'RETURN', 2: 'RUNERROR'}
CODE, DEFAULT_DATA = 0x8800, 0x5000
BODY_SHA = 'ed42ecda2f0160a0b18cabfa9716d9676927298a45a01bb362328f2241302924'
EXPANDED_SHA = 'a42a57e68d3f7815831821cc9365bb917befda45a58e958544451fc7a878df8e'
FLAGS = (0x0002, 0x00D7, 0x08D7, 0x0402, 0x0893, 0x7202, 0x0ED7)
RUNERROR_REACHABLE = 87                                          # instructions reachable from 00E2 inside the SYSTEM segment
# Resident far calls (9A + relocated 0AF8:04A9) and the function that contains each (decoded from the nearest function start).
FAR_CALLERS = {0x86B2: 0x85ED, 0x86CF: 0x85ED, 0x86EF: 0x85ED, 0x8714: 0x8707, 0x8723: 0x8707, 0x87CC: 0x8736, 0x87EA: 0x8736,
               0x8808: 0x8736, 0x8826: 0x8736, 0x8885: 0x8736, 0x890D: 0x8736, 0x892B: 0x8736}
MASK = SIZE - 1


def check(condition, message):
    if not condition:
        raise ValueError(message)


def terminal_from_runerror(image):
    """Static proof obligation for 'never returns': walk every path from 00E2 inside the SYSTEM segment. The only RETF is the
    ExitProc dispatch (it pushes ES:BX under a CS:0123 frame, so the exit procedure returns to 0123, not to our caller), the only
    RET is a local helper reached by CALL, and the process ends at INT 21h AH=4Ch."""
    base = physical(SEG, 0)
    md = Cs(CS_ARCH_X86, CS_MODE_16)
    ins = {i.address: i for i in md.disasm(image[base + 0xE2:base + 0x200], 0xE2)}
    seen, work, rets, ints = set(), [0xE2], [], []
    while work:
        a = work.pop()
        while a not in seen:
            check(a in ins, f'undecoded address {a:04x} reachable from 00E2')
            seen.add(a)
            i = ins[a]
            if i.mnemonic in ('ret', 'retf', 'iret'):
                rets.append((a, i.mnemonic))
                break
            if i.mnemonic == 'int':
                ints.append((a, i.op_str))
            if i.mnemonic == 'jmp':
                check(i.op_str.startswith('0x'), 'indirect jump reachable from 00E2')
                a = int(i.op_str, 16)
                continue
            if i.mnemonic.startswith('j') or i.mnemonic.startswith('loop'):
                check(i.op_str.startswith('0x'), 'indirect branch reachable from 00E2')
                work.append(int(i.op_str, 16))
            if i.mnemonic == 'call':                                # callees are the Halt boundary's helpers; they return to this flow
                check(i.op_str.startswith('0x'), 'indirect call reachable from 00E2')
            a += i.size
    check(rets == [(0x013F, 'retf'), (0x01B2, 'ret')], f'returns reachable from RunError: {[(hex(a), m) for a, m in rets]}')
    check([(i.address, i.mnemonic, i.op_str) for i in (ins[0x138], ins[0x13B], ins[0x13C], ins[0x13D], ins[0x13E])] ==
          [(0x138, 'mov', 'ax, 0x123'), (0x13B, 'push', 'cs'), (0x13C, 'push', 'ax'), (0x13D, 'push', 'es'), (0x13E, 'push', 'bx')],
          'the one RETF is not the ExitProc dispatch frame')
    check(ints == [(0x0164, '0x21'), (0x01A3, '0x21')], f'interrupts reachable from RunError: {ints}')
    check(ins[0x1A1].mnemonic == 'mov' and ins[0x1A1].op_str == 'ah, 0x4c', 'the final INT 21h is not AH=4Ch')
    check(ins[0x15F].mnemonic == 'mov' and ins[0x15F].op_str == 'ah, 0x25', 'the INT 21h at 0164 is not the AH=25h vector restore')
    check(len(seen) == RUNERROR_REACHABLE, f'reachable instruction count {len(seen)}')
    return dict(reachable_instructions=len(seen), returns=[f'{a:04X} {m}' for a, m in rets], interrupts=[f'{a:04X} {o}' for a, o in ints])


def source():
    exe = (ROOT / 'resources/DOS/POOLRAD/START.EXE').read_bytes()
    check(len(exe) == 65231 and sha256(exe).hexdigest() == EXE_SHA, 'START.EXE provenance')
    expanded = bytes(unpack(exe)[0])
    check(sha256(expanded).hexdigest() == EXPANDED_SHA, 'expanded image provenance')
    span = expanded[physical(SEG, ENTRY):physical(SEG, END)]
    insns = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(span, ENTRY))
    check(sha256(span).hexdigest() == BODY_SHA and len(insns) == 5 and sum(i.size for i in insns) == 14, 'span differs')
    check([(i.address, i.mnemonic, i.op_str) for i in insns] ==
          [(0x4A9, 'cmp', 'word ptr [0x4426], 0'), (0x4AE, 'jne', '0x4b1'), (0x4B0, 'retf', ''), (0x4B1, 'mov', 'ax, word ptr [0x4426]'),
           (0x4B4, 'jmp', '0xe2')], 'instructions differ')
    check([i.address for i in insns if i.mnemonic == 'retf'] == [RETF], 'exactly one return')
    rels, _ = relocations(exe, expanded)
    check(not any(physical(SEG, a) in rels for a in range(ENTRY, END)), 'the body has no loader relocation')
    conditional = {i.address: (i.address + i.size, int(i.op_str, 16)) for i in insns if i.mnemonic.startswith('j') and i.mnemonic != 'jmp'}
    check(conditional == {0x04AE: (0x04B0, 0x04B1)}, 'conditional jumps')
    terminal = terminal_from_runerror(expanded)
    return span, {i.address for i in insns}, conditional, terminal


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
            if len(call) == 5 and call[0] == 0x9A and struct.unpack_from('<HH', call, 1) == (ENTRY, SEG):
                direct.append(f"{unit['stub_para']:04X}:{fix - 3:04X}")
    check(not direct, f'direct overlay far calls to 0AF8:04A9 appeared: {direct}')
    image = bytes(unpack(exe)[0])
    rels, _ = relocations(exe, image)
    segment = image[physical(SEG, 0):physical(SEG, 0) + 0x10000]
    near = [o for o in range(len(segment) - 2) if segment[o] in (0xE8, 0xE9) and (o + 3 + struct.unpack_from('<h', segment, o + 1)[0]) & 0xFFFF == ENTRY]
    check(not near, f'near transfers to 04A9 appeared: {near}')
    pattern = struct.pack('<HH', ENTRY, SEG)
    far = [i - 1 for i in range(1, len(image) - 3) if image[i:i + 4] == pattern and image[i - 1] == 0x9A and (i + 2) in rels]
    targets = set()                                            # every relocated far-call target and every PUSH BP; MOV BP,SP is a candidate start
    for i in range(len(image) - 5):
        if image[i] == 0x9A and (i + 3) in rels:
            off, seg = struct.unpack_from('<HH', image, i + 1)
            if seg * 16 + off < len(image):
                targets.add(seg * 16 + off)
    targets |= {i for i in range(len(image) - 2) if image[i:i + 3] == b'\x55\x89\xe5'}
    decoded = {}
    for site in far:
        for start in sorted((t for t in targets if t <= site and site - t < 0x300), reverse=True):
            insns = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(image[start:site + 5], start))
            if insns and insns[-1].address == site and insns[-1].mnemonic == 'lcall' and sum(i.size for i in insns) == site + 5 - start:
                decoded[site] = start
                break
    check(far == sorted(FAR_CALLERS) and decoded == FAR_CALLERS,
          f'resident far-call census differs: raw {[hex(a) for a in far]}, decoded {[(hex(a), hex(f)) for a, f in sorted(decoded.items())]}')
    return dict(overlay_far_call_fixups=0, near_transfers=[], resident_far_call_sites={f'image+{a:05X}': f'function image+{f:05X}' for a, f in sorted(decoded.items())},
                identity='unresolved', credit_claimed=False)


def library(directory):
    path = directory / 'iocheck.so'
    subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Wpedantic', '-Werror', '-O2', '-fPIC', '-shared', '-I', str(ROOT / 'include'),
                    str(ROOT / 'src/recovered/system_iocheck_04a9_cpu.c'), '-o', str(path)], check=True)
    fn = ctypes.CDLL(str(path)).pr_system_iocheck_04a9
    fn.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t, ctypes.POINTER(Cpu)]
    fn.restype = ctypes.c_int
    return fn


def fixture(span, rng, value, *, ss=0x4000, sp=0x8000, data=DEFAULT_DATA, flags=2, background=None, stack_words=None):
    """One state: [return IP][return CS] at SS:SP and the WORD InOutRes at DS:4426."""
    memory = bytearray([0xA5]) * SIZE if background is None else bytearray(background)
    at = physical(CODE, ENTRY)
    memory[at:at + len(span)] = span
    for i, word in enumerate(stack_words or (0x7000, 0x9000)):
        put16(memory, ss, (sp + 2 * i) & 0xFFFF, word)
    put16(memory, data, 0x4426, value)
    cpu = Cpu(*[rng.randrange(65536) for _ in range(7)], sp, CODE, ENTRY, data, rng.randrange(65536), ss, flags)
    return memory, cpu


def p_flags(value):
    """CMP word value,0 flags, written without the C model's helpers."""
    return (4 if bin(value & 255).count('1') % 2 == 0 else 0) | (0x40 if value == 0 else 0) | (0x80 if value >> 15 else 0)


def compare(memory, cpu, fn, label, conditional, value=None):
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
    state = dict(returned=False, stop=None, stopped=False, previous=None)

    def hook(uc, address, size, user):
        cs = uc.reg_read(UC_X86_REG_CS)
        ip = (address - (cs << 4)) & 0xFFFFF
        if state['returned']:
            state['stopped'] = True
            uc.emu_stop()
            return
        if cs == CODE and ip in STOPS and state['previous'] in CALL_SITES:
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
    oracle.emu_start(physical(CODE, ENTRY), 2 * SIZE, count=50)
    check(state['stopped'], f'{label}: neither RETURN nor the RunError transfer')
    want_status = 0 if state['stop'] is None else state['stop']
    ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
    got = fn(ram, SIZE, ctypes.byref(cpu))
    check(got == want_status, f'{label}: C status {NAMES.get(got, got)}, original {NAMES[want_status]}')
    for (name, _), reg in zip(Cpu._fields_, REGISTERS):
        want = oracle.reg_read(reg) & 65535
        check(getattr(cpu, name) == want, f'{label}: {name}: C={getattr(cpu, name):04x} oracle={want:04x}')
    if bytes(ram) != bytes(shared):
        diff = [i for i, (a, b) in enumerate(zip(bytes(ram), bytes(shared))) if a != b]
        raise ValueError(f'{label}: full RAM differs at {[hex(d) for d in diff[:8]]} ({len(diff)} bytes)')
    if value is not None:                                          # independent: RETURN iff InOutRes == 0, and a non-zero value NEVER returns
        final = Cpu.from_buffer_copy(cpu)
        mem = bytes(shared)
        word = lambda i: mem[physical(cpu_in.ss, cpu_in.sp + 2 * i)] | (mem[physical(cpu_in.ss, cpu_in.sp + 2 * i + 1)] << 8)
        if value == 0:
            check(got == 0 and (final.ip, final.cs, final.sp) == (word(0), word(1), (cpu_in.sp + 4) & 0xFFFF), f'{label}: RETURN frame differs')
        else:
            check(got == 2 and final.ip == 0x00E2 and final.ax == value and final.sp == cpu_in.sp and final.cs == cpu_in.cs, f'{label}: a non-zero InOutRes did not stop at RunError')
        check(final.flags & 0x08D5 == p_flags(value), f'{label}: flags {final.flags & 0x8D5:04x} != {p_flags(value):04x}')
    return seen, edges, got


def differential(span, starts, conditional, fn):
    rng = random.Random(0x08AE04A9)
    cases = []                                                      # (label, value, kwargs, plain)
    values = sorted({0, 1, 2, 3, 0x67, 0x68, 0x7F, 0x80, 0xFF, 0x100, 0x101, 0x1FF, 0x200, 0x7FFF, 0x8000, 0x8001, 0xFFFF, 0xFF00, 0xAAAA, 0x5555}
                    | {1 << b for b in range(16)} | {rng.randrange(1, 65536) for _ in range(80)})
    for n, value in enumerate(values):
        cases.append((f'v{value:04x}', value, dict(flags=FLAGS[n % len(FLAGS)]), True))
    for value in (0, 5):
        for sp in (0x0000, 0x0001, 0x0100, 0xFFFA, 0xFFFB, 0xFFFC):
            cases.append((f'v{value:04x}-sp{sp:04x}', value, dict(sp=sp), False))
        for data in (0x0000, 0xFFFF, 0x1234, CODE):
            cases.append((f'v{value:04x}-ds{data:04x}', value, dict(data=data), data != 0xFFFF))
        cases.append((f'v{value:04x}-a20-stack', value, dict(ss=0xFFFF, sp=0x0013), False))
        cases.append((f'v{value:04x}-random-ram', value, dict(background=rng.randbytes(SIZE)), False))
        for sp in range(0x4420, 0x442C, 2):                          # the frame overlaps InOutRes itself
            cases.append((f'v{value:04x}-stack-on-inoutres-{sp:04x}', value, dict(ss=DEFAULT_DATA, sp=sp, flags=0x0ED7), False))
    seen, edges, statuses, plain = set(), set(), {}, 0
    for label, value, kw, is_plain in cases:
        memory, cpu = fixture(span, rng, value, **kw)
        independent = is_plain and kw.get('ss', 0x4000) != kw.get('data', DEFAULT_DATA)
        s, e, status = compare(memory, cpu, fn, label, conditional, value if independent else None)
        plain += 1 if independent else 0
        statuses[status] = statuses.get(status, 0) + 1
        seen |= s
        edges |= e
    check(seen == starts, f'starts missing: {sorted(starts - seen)}')
    wanted = {(a, b) for a, pair in conditional.items() for b in pair}
    check(edges >= wanted, f'outcomes missing: {sorted(wanted - edges)}')
    check(set(statuses) == set(NAMES), f'stops reached {sorted(statuses)}')
    memory, base = fixture(span, rng, 0)
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
                masked_ram_bytes=0, stops={NAMES[k]: v for k, v in sorted(statuses.items())} | {'ARGUMENT': rejects})


# ---- DOSBox-X synthetic oracle (callee stubs DECLARED) -------------------------------------------------
CALLER_N, DATA_N = 0x8800, 0x5000
MAIN_AT, WRAP_AT, CASES_AT = 0x0300, 0x2600, 0x2700
OUTPUT, HEADER, STACK_TOP = 0xC000, 0xD800, 0xF000
WINDOW_LOW, WINDOW_SIZE = STACK_TOP - 0x100, 0x100
NATIVE_CASES, CASE_SIZE = 48, 0x100
RECORD_BYTES = 28 + WINDOW_SIZE + 0x10000
NATIVE_FLAGS = (0x0002, 0x00D7, 0x08D7, 0x0402, 0x0893, 0x0003)
ENTRY_REGS = dict(ax=0x1234, bx=0x3456, cx=0x5678, dx=0x789A, si=0x9ABC, di=0xBCDE, bp=0x7777)


def native_cases():
    """48 synthetic runs: InOutRes values 0 (RETURN) and non-zero (RUNERROR stop), under six flag images."""
    values = (0, 1, 2, 0x67, 0x0100, 0x8000, 0, 0xFFFF, 0x7FFF, 3, 0x00FF, 0x0200)
    return [dict(entry=0, arg=0, flags=n % len(NATIVE_FLAGS), writes=[(0x4426, values[n % len(values)])]) for n in range(NATIVE_CASES)]


def native_audit(span, function):
    directory = ROOT / 'resources/oracle-runs/system-iocheck-04a9-synthetic'
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
          '    push cs', '    mov ax,offset returned', '    push ax', '    cmp word ptr [entry_index],0', '    je no_extra', '    mov ax,0x7777', '    push ax', 'no_extra:',
          '    xor ah,ah', '    mov al,byte ptr [bx]', '    shl ax,1', '    mov si,ax',
          '    push word ptr [flags_table+si]', '    mov ax,cs', '    mov es,ax', f'    mov ax,{DATA_N:#x}', '    mov ds,ax']
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
        for value in (segment, returned):
            sp -= 2
            put16(memory, segment, sp, value)
        put16(memory, segment, sp - 2, NATIVE_FLAGS[case['flags']])       # the harness' PUSHed flag word stays below the entry SP
        regs = list(ENTRY_REGS.values())
        cpu = Cpu(*regs, sp, CALLER_N, ENTRIES[case['entry']], DATA_N, segment, segment, NATIVE_FLAGS[case['flags']])
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
                  callee_stub_kind='JMP FAR into the capture harness at 00E2 (RunError); it is not executed',
                  stops={names[k]: v for k, v in sorted(seen_status.items())}, com_segment=f'{segment:04x}')
    print('04A9 native parity', json.dumps(report, sort_keys=True), flush=True)
    return report


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--compile-only', action='store_true')
    p.add_argument('--unicorn', action='store_true')
    p.add_argument('--native', action='store_true')
    p.add_argument('--output', type=Path)
    a = p.parse_args()
    report = {}
    with tempfile.TemporaryDirectory(prefix='poolrad-iocheck-') as tmp:
        fn = library(Path(tmp))
        if a.compile_only:
            print('CHECKER_BUILD_OK')
            return
        span, starts, conditional, terminal = source()
        report['target'] = dict(bytes=len(span), starts=len(starts), sha256=BODY_SHA, expanded_sha256=EXPANDED_SHA)
        report['ownership'] = ownership()
        report['terminal_from_00e2'] = terminal
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
