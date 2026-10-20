#!/usr/bin/env python3
"""Original-byte full-state parity for U25 +1CC3 (public entry 20) composed with the raw SYSTEM 6-byte Real models,
plus a DOSBox-X oracle.

The Unicorn run executes the original GAME.OVR bytes of +1CC3 and the original SYSTEM bytes (loader-expanded START.EXE
image at segment 0) from the entry to the root return, or to the SYSTEM RunError entry 0AF8:00E2, and compares all 14 CPU
words and the whole 1 MiB RAM with the C model, with no mask. The DOSBox-X oracle runs the same bytes at A000:1CC3 and
9000:<entry>, patching only the RunError entry to a trap. Media stays private.
"""
import argparse
import csv
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
import system_real_cpu_audit as RA

CHECKER_ROWS = ('OVL_0025_1CC3',)
check = RA.check
SYSTEM, RUNERROR = RA.SYSTEM, RA.RUNERROR
CS, SS, RSEG, ROFF = 0x5000, 0x2800, 0x3100, 0x0010
RET_CS, RET_IP = 0x9001, 0x7000
ENTRY, END, RETF = 0x1CC3, 0x1DF0, 0x1DED
OK, CALLEE, CODE = 0, 3, 2
BODY_SHA = 'cfffb7e933808123e58434886f10226994973bdb120d358f09cfcf97f2ea90ca'
FLAG_SET = RA.FLAG_SET
SITES = [(0x1CD0 + 0x2A * k + d, e) for k in range(7) for d, e in ((0, 0x119A), (0xE, 0x1186), (0x13, 0x119E))]
# SYSTEM closure instruction starts no input reaches from this caller, by reason. The audit asserts the unvisited set is
# exactly their union, so a change in either direction fails.
UNREACHABLE = (
    ('general multiply path (+0F5D..+0F68, +0F89..+1001)',
     'the first operand is a converted 16-bit word: +10D9 leaves AH=0 and BX=0 (DX=0 on entry, so BX stays 0 through the shift loop and AH is 0 from MOV AX,00A0h), so +0F55/+0F59 always take the short path',
     lambda a: 0x0F5D <= a <= 0x0F68 or 0x0F89 <= a <= 0x1001),
    ('negative-LongInt arm of +10D9 (+10E5..+10EC)',
     'the caller zeroes DX before every +119A, so the sign test at +10E3 always finds DX non-negative',
     lambda a: a in (0x10E5, 0x10E7, 0x10E9, 0x10EC)),
    ('rounding arm of +1118 (+1155..+115B)',
     'the +119E wrapper sets CH=0 immediately before CALL +1118',
     lambda a: a in (0x1155, 0x1158, 0x115B)),
)
# Words just below a power of two, with the factor mantissa that makes the product round up through every mantissa word
# (found by probing; each reaches the carry-out arm at +1021 through this caller).
CARRY = ((0xFFFE, 0x8001000200), (0xFFFC, 0x8002000800), (0xFFF8, 0x8004002001), (0xFFF0, 0x8008008008), (0xFFE8, 0x800C01201B), (0xFFE0, 0x8010020040))


def source():
    ctx = RA.source()
    ovr = (ROOT / 'resources/DOS/POOLRAD/GAME.OVR').read_bytes()
    exe = (ROOT / 'resources/DOS/POOLRAD/START.EXE').read_bytes()
    check(sha256(ovr).hexdigest() == OVR_SHA, 'GAME.OVR provenance differs')
    row = [r for r in csv.DictReader(open(ROOT / 'docs/re/overlay_function_ledger.csv')) if r['id'] == '0x25:0x1cc3'][0]
    at = int(row['ovr_offset'])
    body = ovr[at:at + 301]
    check(sha256(body).hexdigest() == BODY_SHA, 'body hash differs')
    md = Cs(CS_ARCH_X86, CS_MODE_16)
    ins = list(md.disasm(body, ENTRY))
    check(len(ins) == 81 and sum(i.size for i in ins) == 301, '1CC3 extent')
    check((ins[-1].address, ins[-1].mnemonic, ins[-1].op_str) == (RETF, 'retf', '0xa'), 'RETF 0Ah')
    check(not any(i.mnemonic.startswith('j') or i.mnemonic == 'call' for i in ins), 'no jumps or near calls')
    check([(i.address, i.op_str) for i in ins if i.mnemonic == 'lcall'] == [(a, f'0xaf8, {e:#x}') for a, e in SITES], 'the 21 SYSTEM call sites')
    units = find_units(exe[parse_mz(exe)['header_bytes']:], ovr)
    unit = next(u for u in units if u['stub_para'] == 0x25)
    check(all(a + 3 in unit['fixups'] for a, _ in SITES), 'segment words are relocation-backed')
    incoming = []
    for u in units:
        for fix in u['fixups']:
            b = ovr[u['ovr_offset'] + fix - 3:u['ovr_offset'] + fix + 2]
            if len(b) == 5 and b[0] == 0x9A and struct.unpack_from('<HH', b, 1) == (0x84, unit['memory_segment']):
                incoming.append((u['stub_para'], fix - 3))
    check(sorted(incoming) == [(0x1C, 0x1E1F), (0x1C, 0x1E6E)], f'incoming far callers {incoming}')
    ctx.update(body=body, decoded_caller={i.address: i for i in ins}, incoming=incoming, info_1cc3=dict(
        bytes=301, instructions=81, sha256=BODY_SHA, file_start=at, entry_stub='2B:0084', incoming=[[f'{u:04X}', f'{s:04X}'] for u, s in incoming]))
    print('U25 +1CC3', json.dumps(ctx['info_1cc3'], sort_keys=True), flush=True)
    return ctx


def library(directory):
    path = directory / 'u25_1cc3.so'
    subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Wpedantic', '-Werror', '-O2', '-fPIC', '-shared', '-I', str(ROOT / 'include'),
                    str(ROOT / 'src/recovered/overlay_u25_entry20_1cc3_cpu.c'), str(ROOT / 'src/recovered/system_real_cpu.c'), '-o', str(path)],
                   check=True, capture_output=True, text=True)
    function = ctypes.CDLL(str(path)).pr_u25_entry20_1cc3_cpu

    class Diagnostic(ctypes.Structure):
        _fields_ = [('callee_status', ctypes.c_int), ('calls', ctypes.c_uint), ('call_site', ctypes.c_uint16)]
    function.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t, ctypes.POINTER(Cpu), ctypes.POINTER(Diagnostic)]
    function.restype = ctypes.c_int
    return function, Diagnostic


def build(ctx, rng, words, factor, *, flags=0x7202, sp=0x8000, ss=SS, record=(RSEG, ROFF), background=None, patch=(), segment=SYSTEM):
    memory = bytearray([0xA5]) * SIZE if background is None else bytearray(background)
    memory[0:len(ctx['image'])] = ctx['image']
    at = physical(CS, ENTRY)
    memory[at:at + 301] = ctx['body']
    for site, _ in SITES:
        put16(memory, CS, site + 3, segment)
    for k, w in enumerate(words):
        put16(memory, record[0], (record[1] + 0x88 + 2 * k) & 0xFFFF, w)
    for i, v in enumerate((RET_IP, RET_CS, record[1], record[0], *factor)):
        put16(memory, ss, (sp + 2 * i) & 0xFFFF, v)
    for seg, off, blob in patch:
        for j, v in enumerate(blob):
            memory[(physical(seg, off) + j) & (SIZE - 1)] = v
    return memory, Cpu(rng.randrange(0x10000), rng.randrange(0x10000), rng.randrange(0x10000), rng.randrange(0x10000), rng.randrange(0x10000),
                       rng.randrange(0x10000), rng.randrange(0x10000), sp, CS, ENTRY, rng.randrange(0x10000), rng.randrange(0x10000), ss, flags)


def compare(label, ctx, memory, cpu, function, diag_type, record, budget=2000000):
    from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
    from unicorn.x86_const import UC_X86_REG_CS, UC_X86_REG_IP
    original = Uc(UC_ARCH_X86, UC_MODE_16)
    original.mem_map(0, 2 * SIZE)
    original.mem_write(0, bytes(memory))
    original.mem_write(SIZE, bytes(memory))
    for (field, _), register in zip(Cpu._fields_, REGISTERS):
        original.reg_write(register, getattr(cpu, field))
    decoded, caller, conditional = ctx['decoded'], ctx['decoded_caller'], ctx['conditional']
    protected = [(physical(SYSTEM, a), physical(SYSTEM, a) + i.size) for a, i in decoded.items()]
    protected.append((physical(CS, ENTRY), physical(CS, ENTRY) + 301))
    site_entry = dict(SITES)
    state = dict(stop=None, unexpected=None, root_return=False, shift=False)
    visited_caller, visited_system = set(), set()

    def hook(emulator, address, size, user):
        cs, ip = emulator.reg_read(UC_X86_REG_CS), emulator.reg_read(UC_X86_REG_IP)
        if state['root_return']:                  # the RETF has run; where it landed is compared as CS:IP below
            state['stop'] = OK
            return emulator.emu_stop()
        if (cs, ip) == (SYSTEM, RUNERROR):
            state['stop'] = CALLEE
            return emulator.emu_stop()
        if cs == CS and ip in caller:
            visited_caller.add(ip)
            if ip == RETF:
                state['root_return'] = True
            if ip in site_entry and bytes(emulator.mem_read(physical(cs, ip), 3)) != b'\x9a' + struct.pack('<H', site_entry[ip]):
                state['stop'] = CODE                # a reached LCALL that is not 9A <entry>: stopped at the site, nothing pushed
                return emulator.emu_stop()
            mnemonic = caller[ip].mnemonic
        elif cs == SYSTEM and ip in decoded:
            visited_system.add(ip)
            mnemonic = decoded[ip].mnemonic
        else:
            state['unexpected'] = (cs, ip)
            return emulator.emu_stop()
        if mnemonic in RA.SHIFTS:
            state['shift'] = True
        elif mnemonic in RA.AF_WRITERS:
            state['shift'] = False

    def mirror(emulator, access, address, size, value, user):
        for byte in range(size):
            raw = address + byte
            at = raw & (SIZE - 1)
            check(not any(lo <= at < hi for lo, hi in protected), f'immutable code written: {label} {at:x}')
            emulator.mem_write(at + SIZE if raw < SIZE else at, bytes([(value >> (8 * byte)) & 0xFF]))

    original.hook_add(UC_HOOK_CODE, hook)
    original.hook_add(UC_HOOK_MEM_WRITE, mirror)
    original.emu_start(physical(CS, ENTRY), 2 * SIZE, count=budget)
    check(state['unexpected'] is None and state['stop'] is not None, f'original escaped or stalled: {label} {state}')
    expected = bytes(original.mem_read(0, SIZE))
    ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
    diag = diag_type()
    status = function(ram, SIZE, ctypes.byref(cpu), ctypes.byref(diag))
    check(status == state['stop'], f"C status {status}, original {state['stop']}: {label}")
    actual = bytes(ram)
    if actual != expected:
        differences = [i for i, (a, b) in enumerate(zip(actual, expected)) if a != b]
        raise ValueError(f'RAM mismatch {label}: {[(hex(d), actual[d], expected[d]) for d in differences[:6]]} ({len(differences)} bytes)')
    split = 0
    for (field, _), register in zip(Cpu._fields_, REGISTERS):
        mine, want = getattr(cpu, field), original.reg_read(register) & 0xFFFF
        if field == 'flags' and mine != want and mine ^ want == 0x10 and mine & 0x10 and state['shift']:
            split = 1                              # the declared AF-after-shift split (see system_real_cpu_audit)
            continue
        check(mine == want, f'{field} mismatch {label}: C={mine:04x}, original={want:04x}')
    final = [struct.unpack_from('<H', expected, physical(record[0], (record[1] + 0x88 + 2 * k) & 0xFFFF))[0] for k in range(7)]
    return state['stop'], visited_caller, visited_system, diag, final, split


def factor_words(parts):
    ax, bx, dx = RA.pack_real(parts)
    return (ax, bx, dx)


def cases(rng):
    spec = []
    flag = lambda i: FLAG_SET[i % 6]
    n = 0
    word_sets = {'edges': [0, 1, 2, 0x7FFF, 0x8000, 0xFFFF, 0x00FF], 'random': None, 'zeros': [0] * 7, 'ones': [1] * 7}

    def words(kind):
        return [rng.getrandbits(16) for _ in range(7)] if kind == 'random' else list(word_sets[kind])
    for e in RA.EXPONENTS:
        for sign in (0, 1):
            for kind, mant in (('rand', None), ('ones', (1 << 39) - 1), ('zero', 0)):
                for ws in ('edges', 'random'):
                    spec.append((f'factor-{e:02x}-{sign}-{kind}-{ws}', words(ws), factor_words(RA.real(rng, e, sign, mant)), flag(n))); n += 1
    for name, parts in (('one', (0x81, 0, 0, 0)), ('two', (0x82, 0, 0, 0)), ('half', (0x80, 0, 0, 0)), ('minus-one', (0x81, 0, 0, 0x8000)),
                        ('zero', (0, 0, 0, 0)), ('huge', (0xFF, 0xFF, 0xFFFF, 0x7FFF)), ('tiny', (1, 0, 0, 0))):
        for ws in ('edges', 'random', 'ones'):
            spec.append((f'special-{name}-{ws}', words(ws), factor_words(parts), flag(n))); n += 1
    for i in range(1500):
        spec.append((f'random-{i}', words('random'), factor_words(RA.real(rng)), flag(n))); n += 1
    # The record aliases the frame: field k lands on a frame word, so the pointer or factor changes mid-run.
    for word, mantissa in CARRY:
        for exponent in (0x81, 0x80, 0xFE, 0xFF):
            for sign in (0, 1):
                spec.append((f'carry-{word:04x}-{exponent:02x}-{sign}', [word] * 7, factor_words(RA.real(rng, exponent, sign, mantissa & ((1 << 39) - 1))), flag(n))); n += 1
    for j in range(-6, 16):
        off = (0x8000 + 4 + 2 * j - 0x88) & 0xFFFF
        spec.append((f'alias-frame-{j}', [3, 5, 9, 17, 33, 65, 129], factor_words((0x82, 0, 0, 0)), flag(n), dict(record=(SS, off)))); n += 1
    for sp in (0x0046, 0x0048, 0x0100, 0xFFF0, 0xFFF2):
        spec.append((f'sp-{sp:04x}', [7, 8, 9, 10, 11, 12, 13], factor_words((0x82, 0, 0, 0)), flag(n), dict(sp=sp, ss=0x1C00, record=(RSEG, ROFF)))); n += 1
    spec.append(('random-background', words('random'), factor_words(RA.real(rng)), 0x0202, dict(background=bytes(random.Random(0x1CC3).randbytes(SIZE)))))
    for site, entry in ((0x1CD0, 0x119A), (0x1CDE, 0x1186), (0x1CE3, 0x119E), (0x1CD0 + 0x2A * 6 + 0x13, 0x119E)):
        spec.append((f'bad-opcode-{site:04x}', words('edges'), factor_words((0x82, 0, 0, 0)), flag(n), dict(patch=((CS, site, b'\x90'),)))); n += 1
        spec.append((f'bad-offset-{site:04x}', words('edges'), factor_words((0x82, 0, 0, 0)), flag(n), dict(patch=((CS, site + 1, struct.pack('<H', entry ^ 1)),)))); n += 1
    return spec


def differential(ctx, function, diag_type):
    rng = random.Random(0x1CC31CC3)
    caller_seen, system_seen = set(), set()
    stops, named, splits = {}, {}, 0
    for entry in cases(rng):
        label, words, factor, flags = entry[:4]
        options = entry[4] if len(entry) > 4 else {}
        memory, cpu = build(ctx, rng, words, factor, flags=flags, **options)
        record = options.get('record', (RSEG, ROFF))
        stop, vc, vs, diag, final, split = compare(label, ctx, memory, cpu, function, diag_type, record)
        splits += split
        caller_seen |= vc
        system_seen |= vs
        key = 'return' if stop == OK else 'RunError' if stop == CALLEE else 'CODE'
        if stop == CODE:
            key = 'CODE'
        stops[key] = stops.get(key, 0) + 1
        if not label.startswith(('factor-', 'random-')):
            named[label] = key
        # Independent arithmetic oracle on the original's final RAM: a word times a known Real factor, truncated to a LongInt,
        # keeps its low 16 bits. Factor 1.0, 2.0, 0.5, -1.0 and 0 are exact, so every stored word is known.
        exact = {'special-one-': lambda w: w, 'special-two-': lambda w: (2 * w) & 0xFFFF, 'special-half-': lambda w: w // 2,
                 'special-minus-one-': lambda w: (-w) & 0xFFFF, 'special-zero-': lambda w: 0}
        for prefix, function_of in exact.items():
            if label.startswith(prefix):
                check(key == 'return' and final == [function_of(w) for w in words], f'{label}: {final} from {words}')
    caller = set(ctx['decoded_caller'])
    missing_caller = sorted(caller - caller_seen)
    wanted = set()
    for name in RA.ENTRIES:
        wanted |= ctx['closure'][name]
    missing_system = sorted(wanted - system_seen)
    check(not missing_caller, f'caller coverage: {[hex(a) for a in missing_caller]}')
    report = dict(cases=sum(stops.values()), stops=stops, cpu_words=14, ram_bytes=SIZE, masked_bits=0, af_after_shift_splits=splits,
                  caller_starts=len(caller_seen), caller_total=len(caller), caller_outcomes=0, caller_outcomes_total=0,
                  system_starts=len(system_seen), system_total=len(wanted), system_missing=[hex(a) for a in missing_system], named_stops=named)
    print('Original parity', json.dumps(report, sort_keys=True), flush=True)
    declared = sorted(a for a in wanted if any(predicate(a) for _, _, predicate in UNREACHABLE))
    check(missing_system == declared, f'system closure coverage: unvisited {[hex(a) for a in missing_system]} vs declared {[hex(a) for a in declared]}')
    report['excluded'] = {name: dict(starts=sum(1 for a in wanted if predicate(a)), reason=why) for name, why, predicate in UNREACHABLE}
    return report


# ---- DOSBox-X synthetic oracle -------------------------------------------------------------------------
SYSTEM_N, OVL_N, REC_N = 0x9000, 0x3000, 0x5000   # conventional memory: A000-AFFF is video RAM under DOS
MAIN_AT, BLOB_AT, BODY_AT, CASES_AT = 0x0300, 0x0900, 0x1900, 0x2A00
BLOB_LOW, BLOB_HIGH = RA.BLOB_LOW, RA.BLOB_HIGH
OUTPUT, HEADER, STACK_TOP = 0xC000, 0xC800, 0xF000
FRAME_LOW, FRAME_SIZE = 0xEFC0, 0x40
RECORD_LOW, RECORD_SIZE = ROFF + 0x80, 0x20
CASE_SIZE = 22
RECORD_BYTES = 30 + FRAME_SIZE + RECORD_SIZE
NATIVE_FLAGS = RA.NATIVE_FLAGS


def native_cases():
    pool = [c for c in cases(random.Random(0x1CC31CC3)) if len(c) == 4]
    spread = lambda items, k: items[::max(1, len(items) // k)][:k]
    chosen = spread([c for c in pool if c[0].startswith('factor-')], 110) + spread([c for c in pool if c[0].startswith('special-')], 21) + \
        spread([c for c in pool if c[0].startswith('random-')], 60)
    return [dict(label=c[0], words=c[1], factor=c[2], flags=n % len(NATIVE_FLAGS)) for n, c in enumerate(chosen)]


def native_audit(ctx, function, diag_type):
    directory = ROOT / 'resources/oracle-runs/u25-1cc3-synthetic'
    directory.mkdir(parents=True, exist_ok=True)
    rows = native_cases()
    blob = ctx['image'][SYSTEM * 16 + BLOB_LOW:SYSTEM * 16 + BLOB_HIGH]
    body = bytearray(ctx['body'])
    for site, _ in SITES:
        struct.pack_into('<H', body, site + 3 - ENTRY, SYSTEM_N)
    body = bytes(body)
    text = ['.code16', '.intel_syntax noprefix', '.global _start', '_start:', '    jmp main', f'.org {MAIN_AT - 0x100:#x}', 'main:',
            '    mov ax,cs', '    mov ds,ax', '    mov es,ax', '    mov ss,ax', f'    mov sp,{STACK_TOP:#x}', '    cld',
            f'    mov ax,{SYSTEM_N:#x}', '    mov es,ax', '    mov si,offset blob', f'    mov di,{BLOB_LOW:#x}', f'    mov cx,{len(blob)}', '    rep movsb',
            f'    mov byte ptr es:[{RUNERROR:#x}],0xea', f'    mov word ptr es:[{RUNERROR + 1:#x}],offset trap', f'    mov word ptr es:[{RUNERROR + 3:#x}],cs',
            f'    mov ax,{OVL_N:#x}', '    mov es,ax', '    mov si,offset body', f'    mov di,{ENTRY:#x}', f'    mov cx,{len(body)}', '    rep movsb',
            '    mov ax,cs', '    mov es,ax',
            f'    mov word ptr [{HEADER:#x}],cs', f'    mov word ptr [{HEADER + 2:#x}],offset returned',
            f'    mov word ptr [{HEADER + 4:#x}],{len(rows)}', f'    mov word ptr [{HEADER + 6:#x}],0',
            '    mov dx,offset filename', '    xor cx,cx', '    mov ah,0x3c', '    int 0x21', '    jc failure',
            '    mov word ptr [handle],ax', '    mov bx,ax', f'    mov dx,{HEADER:#x}', '    mov cx,8', '    mov ah,0x40', '    int 0x21', '    jc failure',
            'again:', '    cli', f'    mov sp,{STACK_TOP:#x}', '    cld', '    mov ax,cs', '    mov ds,ax', '    mov es,ax',
            '    mov al,0xa5', f'    mov di,{FRAME_LOW:#x}', f'    mov cx,{FRAME_SIZE}', '    rep stosb',
            '    mov ax,word ptr [index]', f'    mov bx,{CASE_SIZE}', '    mul bx', '    add ax,offset cases', '    mov si,ax',
            f'    mov ax,{REC_N:#x}', '    mov es,ax', '    mov al,0xa5', f'    mov di,{RECORD_LOW:#x}', f'    mov cx,{RECORD_SIZE}', '    rep stosb',
            '    push si', f'    add si,2', f'    mov di,{ROFF + 0x88:#x}', '    mov cx,7', '    rep movsw', '    pop si',
            '    mov ax,cs', '    mov es,ax',
            '    push word ptr [si+20]', '    push word ptr [si+18]', '    push word ptr [si+16]', f'    push {REC_N:#x}', f'    push {ROFF:#x}',
            '    mov bl,byte ptr [si+1]', '    xor bh,bh', '    shl bx,1', '    push word ptr [flags_table+bx]', '    popf',
            '    mov ax,0x1111', '    mov bx,0x2222', '    mov cx,0x3333', '    mov dx,0x4444', '    mov si,0x5555', '    mov di,0x6666', '    mov bp,0x7777',
            '    call dword ptr cs:[target]', 'returned:', '    mov word ptr cs:[status],0', '    jmp record', 'trap:', '    mov word ptr cs:[status],1', 'record:']
    for i, reg in enumerate('ax bx cx dx si di bp sp'.split()):
        text.append(f'    mov word ptr cs:[{OUTPUT + 2 * i:#x}],{reg}')
    text += [f'    mov word ptr cs:[{OUTPUT + 20:#x}],ds', f'    mov word ptr cs:[{OUTPUT + 22:#x}],es', f'    mov word ptr cs:[{OUTPUT + 24:#x}],ss',
             f'    mov sp,{STACK_TOP + 0x800:#x}', '    pushf', '    pop ax', f'    mov word ptr cs:[{OUTPUT + 26:#x}],ax',
             '    cmp word ptr cs:[status],0', '    jne trapped',
             f'    mov word ptr cs:[{OUTPUT + 16:#x}],cs', f'    mov word ptr cs:[{OUTPUT + 18:#x}],offset returned', '    jmp stamp', 'trapped:',
             f'    mov word ptr cs:[{OUTPUT + 16:#x}],{SYSTEM_N:#x}', f'    mov word ptr cs:[{OUTPUT + 18:#x}],{RUNERROR:#x}', 'stamp:',
             '    mov ax,word ptr cs:[status]', f'    mov word ptr cs:[{OUTPUT + 28:#x}],ax',
             '    cld', '    mov ax,cs', '    mov es,ax', '    mov ds,ax', f'    mov di,{OUTPUT + 30:#x}',
             f'    mov si,{FRAME_LOW:#x}', f'    mov cx,{FRAME_SIZE}', '    rep movsb',
             f'    mov ax,{REC_N:#x}', '    mov ds,ax', f'    mov si,{RECORD_LOW:#x}', f'    mov cx,{RECORD_SIZE}', '    rep movsb',
             '    mov ax,cs', '    mov ds,ax', '    mov es,ax', '    sti',
             '    mov bx,word ptr [handle]', f'    mov dx,{OUTPUT:#x}', f'    mov cx,{RECORD_BYTES}', '    mov ah,0x40', '    int 0x21', '    jc failure',
             f'    cmp ax,{RECORD_BYTES}', '    jne failure',
             '    inc word ptr [index]', '    mov ax,word ptr [index]', f'    cmp ax,{len(rows)}', '    jb again',
             '    mov bx,word ptr [handle]', '    mov ah,0x3e', '    int 0x21', '    mov ax,0x4c00', '    int 0x21',
             'failure:', '    mov ax,0x4c01', '    int 0x21',
             'filename: .asciz "RESULT.BIN"', 'handle: .word 0', 'index: .word 0', 'status: .word 0',
             f'target: .word {ENTRY:#x},{OVL_N:#x}',
             'flags_table: .word ' + ','.join(f'{f:#x}' for f in NATIVE_FLAGS),
             f'.org {BLOB_AT - 0x100:#x}', 'blob:', '    .byte ' + ','.join(f'{b:#x}' for b in blob),
             f'.org {BODY_AT - 0x100:#x}', 'body:', '    .byte ' + ','.join(f'{b:#x}' for b in body),
             f'.org {CASES_AT - 0x100:#x}', 'cases:']
    for row in rows:
        data = struct.pack('<BB7H3H', 0, row['flags'], *row['words'], *row['factor'])
        text.append('    .byte ' + ','.join(f'{b:#x}' for b in data))
    check(CASES_AT + CASE_SIZE * len(rows) <= OUTPUT, 'COM layout overlaps')
    asm, obj = directory / 'oracle.S', directory / 'oracle.o'
    asm.write_text('\n'.join(text) + '\n')
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
    finally:                                       # a spinning oracle can write gigabytes of log: keep the head only
        log_path = directory / 'dosbox.log'
        if log_path.exists() and log_path.stat().st_size > (50 << 20):
            with log_path.open('r+b') as log:
                log.truncate(1 << 20)
    result = result_path.read_bytes()
    check(len(result) == 8 + len(rows) * RECORD_BYTES, f'native output incomplete: {len(result)} bytes')
    segment, returned, count, _ = struct.unpack_from('<4H', result)
    check(count == len(rows), 'native header differs')
    stops = {}
    for n, row in enumerate(rows):
        record = result[8 + n * RECORD_BYTES:8 + (n + 1) * RECORD_BYTES]
        memory = bytearray(SIZE)
        for site, _ in SITES:
            at = physical(OVL_N, site)
            memory[at:at + 5] = b'\x9a' + struct.pack('<HH', {0: 0x119A, 0xE: 0x1186, 0x13: 0x119E}[(site - 0x1CD0) % 0x2A], SYSTEM_N)
        for i in range(FRAME_SIZE):
            memory[physical(segment, FRAME_LOW + i)] = 0xA5
        for i in range(RECORD_SIZE):
            memory[physical(REC_N, RECORD_LOW + i)] = 0xA5
        for k, w in enumerate(row['words']):
            put16(memory, REC_N, ROFF + 0x88 + 2 * k, w)
        sp = STACK_TOP - 10
        for i, v in enumerate((ROFF, REC_N, *row['factor'])):
            put16(memory, segment, sp + 2 * i, v)
        sp -= 4
        for i, v in enumerate((returned, segment)):
            put16(memory, segment, sp + 2 * i, v)
        cpu = Cpu(0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0x6666, 0x7777, sp, OVL_N, ENTRY, segment, segment, segment, NATIVE_FLAGS[row['flags']])
        ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
        diag = diag_type()
        label = f"native case {n} ({row['label']})"
        status = function(ram, SIZE, ctypes.byref(cpu), ctypes.byref(diag))
        want_status = struct.unpack_from('<H', record, 28)[0]
        check(status in (OK, CALLEE) and status == (CALLEE if want_status else OK), f'C status {status} vs DOS {want_status} on {label}')
        for (field, _), want in zip(Cpu._fields_, struct.unpack_from('<14H', record)):
            check(getattr(cpu, field) == want, f'{label} {field}: C={getattr(cpu, field):04x}, DOS={want:04x}')
        got = bytes(ram[physical(segment, FRAME_LOW):physical(segment, FRAME_LOW) + FRAME_SIZE])
        check(got == record[30:30 + FRAME_SIZE], f'{label} stack bytes differ')
        got = bytes(ram[physical(REC_N, RECORD_LOW):physical(REC_N, RECORD_LOW) + RECORD_SIZE])
        check(got == record[30 + FRAME_SIZE:], f'{label} record bytes differ')
        key = 'RunError' if status else 'return'
        stops[key] = stops.get(key, 0) + 1
    probe = subprocess.run(['dosbox-x', '--version'], capture_output=True, text=True)
    version = [l.split(',')[0].strip() for l in (probe.stdout + probe.stderr).splitlines() if l.strip().startswith('DOSBox-X version')][:1]
    binary = subprocess.run(['which', 'dosbox-x'], capture_output=True, text=True).stdout.strip()
    digest = sha256(result).hexdigest()
    report = dict(dosbox_x=dict(path=binary, sha256=sha256(Path(binary).read_bytes()).hexdigest(), version=version[0] if version else '',
                                pinned_source='5a9c7c53481c2e933948039c33256776653f4faf (not used: declared substitution)'),
                  cases=len(rows), stops=stops, cpu_words=14, masked_bits=0, stack_bytes=FRAME_SIZE, record_bytes=RECORD_SIZE,
                  result_sha256=digest, com_segment=f'{segment:04x}')
    print('Native parity', json.dumps(report, sort_keys=True), flush=True)
    return report


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--compile-only', action='store_true')
    parser.add_argument('--unicorn', action='store_true')
    parser.add_argument('--native', action='store_true')
    parser.add_argument('--output', type=Path)
    args = parser.parse_args()
    with tempfile.TemporaryDirectory(prefix='poolrad-1cc3-') as temp:
        function, diag_type = library(Path(temp))
        if args.compile_only:
            print('CHECKER_BUILD_OK')
            return
        ctx = source()
        report = dict(target=ctx['info_1cc3'], system=ctx['info'])
        if args.unicorn:
            report['unicorn'] = differential(ctx, function, diag_type)
        if args.native:
            report['native'] = native_audit(ctx, function, diag_type)
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(json.dumps(report, sort_keys=True, indent=2) + '\n')


if __name__ == '__main__':
    main()
