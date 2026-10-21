#!/usr/bin/env python3
"""SYSTEM 0AF8:1491 Close (raw CPU + its check helper 14B6, runtime coverage, no identity claim) against the original bytes.

The bytes come from the hash-checked loader-expanded START.EXE image. The Unicorn run executes the original at CODE:1491 until the
RETF 4 has run (or the helper's RET returned somewhere other than 149A: TRANSFER), with the declared INT 21h AH=3Eh service played by
an interrupt hook, then compares all 14 CPU words, the whole 1 MiB RAM and the stop status with the C model, with no mask. Native
DOSBox-X replays the original with an INT 21h hook stub (declared). Media stays private."""
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

CHECKER_ROWS = ('RES_0AF8_1491',)
SEG = 0x0AF8
ENTRY, END, RETF, HELPER_RET, INT_AT = 0x1491, 0x14C5, 0x14B3, 0x14C4, 0x14A6
ENTRIES = (0x1491,)
NAMES = {0: 'RETURN', 2: 'SERVICE', 3: 'TRANSFER'}
CODE, DEFAULT_DATA, DATA_SEG = 0x8800, 0x5000, 0x3000
BODY_SHA = '83ba3ec7cf3aabfd785f101768657a686f1915e65f5895a82be1d91030dc8858'
EXPANDED_SHA = 'a42a57e68d3f7815831821cc9365bb917befda45a58e958544451fc7a878df8e'
FLAGS = (0x0002, 0x00D7, 0x08D7, 0x0402, 0x0893, 0x7202, 0x0ED7)
OVERLAY_FAR_CALLERS = ['0036:05E5', '006F:08D1', '006F:09C6', '006F:0AB9', '006F:0C4F', '006F:0D8A', '006F:0E88', '006F:18E5', '006F:20BF']
NEAR_CALLERS = {0x1442: 0x141F}                                  # the near CALL inside the shared Reset/Rewrite body at 141F
# Resident far calls (9A + relocated 0AF8:1491) and the function that contains each (decoded from the nearest function start).
FAR_CALLERS = {0x43CA: 0x4354, 0x834A: 0x830B, 0x870F: 0x8707, 0x871E: 0x8707}
SERVICE_FN = ctypes.CFUNCTYPE(ctypes.c_int, ctypes.c_void_p, ctypes.POINTER(Cpu), ctypes.POINTER(Cpu))
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
    check(sha256(span).hexdigest() == BODY_SHA and len(insns) == 17 and sum(i.size for i in insns) == 52, 'span differs')
    check([(i.address, i.mnemonic, i.op_str) for i in insns] ==
          [(0x1491, 'mov', 'bx, sp'), (0x1493, 'les', 'di, ptr ss:[bx + 4]'), (0x1497, 'call', '0x14b6'), (0x149A, 'jne', '0x14b3'),
           (0x149C, 'mov', 'bx, word ptr es:[di]'), (0x149F, 'cmp', 'bx, 4'), (0x14A2, 'jbe', '0x14ad'), (0x14A4, 'mov', 'ah, 0x3e'),
           (0x14A6, 'int', '0x21'), (0x14A8, 'jae', '0x14ad'), (0x14AA, 'mov', 'word ptr [0x4426], ax'),
           (0x14AD, 'mov', 'word ptr es:[di + 2], 0xd7b0'), (0x14B3, 'retf', '4'), (0x14B6, 'cmp', 'word ptr es:[di + 2], 0xd7b3'),
           (0x14BC, 'je', '0x14c4'), (0x14BE, 'mov', 'word ptr [0x4426], 0x67'), (0x14C4, 'ret', '')], 'instructions differ')
    check([i.address for i in insns if i.mnemonic in ('ret', 'retf')] == [RETF, HELPER_RET], 'returns')
    check([i.address for i in insns if i.mnemonic == 'int'] == [INT_AT], 'the only interrupt is the INT 21h at 14A6')
    ah = {i.address: i for i in insns}[0x14A4]
    check(ah.bytes.hex() == 'b43e', 'the AH list is {3Eh}: AH is set only by MOV AH,3Eh immediately before the INT')
    check(not any(i.mnemonic in ('in', 'out', 'lcall', 'ljmp') for i in insns), 'no port or far transfer')
    rels, _ = relocations(exe, expanded)
    check(not any(physical(SEG, a) in rels for a in range(ENTRY, END)), 'the body has no loader relocation')
    conditional = {i.address: (i.address + i.size, int(i.op_str, 16)) for i in insns if i.mnemonic.startswith('j') and i.mnemonic != 'jmp'}
    check(conditional == {0x149A: (0x149C, 0x14B3), 0x14A2: (0x14A4, 0x14AD), 0x14A8: (0x14AA, 0x14AD), 0x14BC: (0x14BE, 0x14C4)}, 'conditional jumps')
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
            if len(call) == 5 and call[0] == 0x9A and struct.unpack_from('<HH', call, 1) == (ENTRY, SEG):
                direct.append(f"{unit['stub_para']:04X}:{fix - 3:04X}")
    check(direct == OVERLAY_FAR_CALLERS, f'overlay far-call census differs: {direct}')
    image = bytes(unpack(exe)[0])
    rels, _ = relocations(exe, image)
    segment = image[physical(SEG, 0):physical(SEG, 0) + 0x10000]
    near = [o for o in range(len(segment) - 2) if segment[o] in (0xE8, 0xE9) and (o + 3 + struct.unpack_from('<h', segment, o + 1)[0]) & 0xFFFF == ENTRY]
    check(near == sorted(NEAR_CALLERS), f'near transfers to 1491: {[hex(a) for a in near]}')
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
    return dict(overlay_far_call_fixups=len(direct), overlay_far_call_sites=direct, near_transfers={f'{a:04X}': f'function {f:04X}' for a, f in NEAR_CALLERS.items()}, resident_far_call_sites={f'image+{a:05X}': f'function image+{f:05X}' for a, f in sorted(decoded.items())},
                identity='unresolved', credit_claimed=False)


def library(directory):
    path = directory / 'close.so'
    subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Wpedantic', '-Werror', '-O2', '-fPIC', '-shared', '-I', str(ROOT / 'include'),
                    str(ROOT / 'src/recovered/system_close_1491_cpu.c'), '-o', str(path)], check=True)
    fn = ctypes.CDLL(str(path)).pr_system_close_1491
    fn.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t, ctypes.POINTER(Cpu), SERVICE_FN, ctypes.c_void_p]
    fn.restype = ctypes.c_int
    return fn


def fixture(span, rng, handle, magic, *, var=(DATA_SEG, 0x0100), inout=0, ss=0x4000, sp=0x8000, data=DEFAULT_DATA, flags=2, background=None,
            stack_words=None, svc=(0x3E11, 0)):
    """[return IP][return CS][var offset][var segment] at SS:SP; the variable is [handle][magic] at var; InOutRes is the WORD DS:4426
    (inout None leaves it as the background has it, for the aliasing cases)."""
    memory = bytearray([0xA5]) * SIZE if background is None else bytearray(background)
    at = physical(CODE, ENTRY)
    memory[at:at + len(span)] = span
    for i, word in enumerate(stack_words or (0x7000, 0x9000, var[1], var[0])):
        put16(memory, ss, (sp + 2 * i) & 0xFFFF, word)
    put16(memory, var[0], var[1], handle)
    put16(memory, var[0], (var[1] + 2) & 0xFFFF, magic)
    if inout is not None:
        put16(memory, data, 0x4426, inout)
    cpu = Cpu(*[rng.randrange(65536) for _ in range(7)], sp, CODE, ENTRY, data, rng.randrange(65536), ss, flags)
    return memory, cpu, svc


def p_cmp_flags(a, b):
    """CMP word a,b flags written without the C model's helper (SF, ZF, PF, CF, AF, OF)."""
    r = (a - b) & 0xFFFF
    sa, sb = a - 65536 * (a >> 15), b - 65536 * (b >> 15)
    return ((1 if a < b else 0) | (4 if bin(r & 255).count('1') % 2 == 0 else 0) | (0x10 if (a & 15) < (b & 15) else 0) |
            (0x40 if r == 0 else 0) | (0x80 if r >> 15 else 0) | (0x800 if not -32768 <= sa - sb <= 32767 else 0))


def reference(handle, magic, inout, svc, sp, bx_in):
    """The documented behavior: returns (interrupts, InOutRes, magic, BX, AX or None)."""
    if magic != 0xD7B3:
        return 0, 0x0067, magic, sp, None
    ints = 1 if handle > 4 else 0
    res = svc[0] if ints and svc[1] else inout
    return ints, res, 0xD7B0, handle, (svc[0] if ints else None)


def compare(memory, cpu, fn, label, conditional, svc, ref=None):
    from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE, UC_HOOK_INTR
    from unicorn.x86_const import UC_X86_REG_CS, UC_X86_REG_AX, UC_X86_REG_EFLAGS
    oracle = Uc(UC_ARCH_X86, UC_MODE_16)
    shared = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
    for base in (0, SIZE):                                          # two mappings of the same bytes give the A20-off alias
        oracle.mem_map_ptr(base, SIZE, 7, ctypes.addressof(shared))
    for (name, _), reg in zip(Cpu._fields_, REGISTERS):
        oracle.reg_write(reg, getattr(cpu, name))
    cpu_in = Cpu.from_buffer_copy(cpu)
    seen, edges = set(), set()
    state = dict(returned=False, stop=None, stopped=False, previous=None, ints=0)

    def hook(uc, address, size, user):
        cs = uc.reg_read(UC_X86_REG_CS)
        ip = (address - (cs << 4)) & 0xFFFFF
        if state['returned']:
            state['stopped'] = True
            uc.emu_stop()
            return
        if state['previous'] == HELPER_RET and not (cs == CODE and ip == 0x149A):       # the helper's RET went somewhere else
            state['stop'] = 3
            state['stopped'] = True
            uc.emu_stop()
            return
        check(cs == CODE and ENTRY <= ip < END, f'{label}: escaped: {cs:x}:{ip:x}')
        if state['previous'] in conditional:
            edges.add((state['previous'], ip))
        seen.add(ip)
        state['previous'] = ip
        state['returned'] = ip == RETF

    def intr(uc, intno, user):                                      # the declared service: AX and CF are DOS's whole answer
        check(intno == 0x21 and (uc.reg_read(UC_X86_REG_AX) >> 8) == 0x3E, f'{label}: unexpected interrupt {intno:x}')
        state['ints'] += 1
        uc.reg_write(UC_X86_REG_AX, svc[0])
        uc.reg_write(UC_X86_REG_EFLAGS, (uc.reg_read(UC_X86_REG_EFLAGS) & ~1) | svc[1])

    oracle.hook_add(UC_HOOK_CODE, hook)
    oracle.hook_add(UC_HOOK_INTR, intr)
    oracle.emu_start(physical(CODE, ENTRY), 2 * SIZE, count=200)
    check(state['stopped'], f'{label}: neither RETURN nor TRANSFER')
    want_status = 0 if state['stop'] is None else state['stop']
    ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
    calls = []

    def service(context, before, after):
        calls.append(Cpu.from_buffer_copy(before[0]))
        after[0] = Cpu.from_buffer_copy(before[0])
        after[0].ip = 0x14A8
        after[0].ax = svc[0]
        after[0].flags = (before[0].flags & ~1) | svc[1]
        return 0
    got = fn(ram, SIZE, ctypes.byref(cpu), SERVICE_FN(service), None)
    check(got == want_status, f'{label}: C status {NAMES.get(got, got)}, original {NAMES[want_status]}')
    check(len(calls) == state['ints'] and all(c.ip == INT_AT and (c.ax >> 8) == 0x3E for c in calls), f'{label}: INT 21h count {len(calls)} vs original {state["ints"]}')
    for (name, _), reg in zip(Cpu._fields_, REGISTERS):
        want = oracle.reg_read(reg) & 65535
        check(getattr(cpu, name) == want, f'{label}: {name}: C={getattr(cpu, name):04x} oracle={want:04x}')
    if bytes(ram) != bytes(shared):
        diff = [i for i, (a, b) in enumerate(zip(bytes(ram), bytes(shared))) if a != b]
        raise ValueError(f'{label}: full RAM differs at {[hex(d) for d in diff[:8]]} ({len(diff)} bytes)')
    if ref is not None:                                            # the documented behavior, on non-aliasing states
        handle, magic, inout, var = ref
        ints, res, mag, bx, ax = reference(handle, magic, inout, svc, cpu_in.sp, cpu_in.bx)
        mem = bytes(shared)
        w = lambda seg, off: mem[physical(seg, off)] | (mem[physical(seg, off) + 1] << 8)
        final = Cpu.from_buffer_copy(cpu)
        check(got == 0 and state['ints'] == ints, f'{label}: interrupts {state["ints"]}, expected {ints}')
        check(w(cpu_in.ds, 0x4426) == res and w(var[0], (var[1] + 2) & 0xFFFF) == mag and final.bx == bx and (ax is None or final.ax == ax),
              f'{label}: InOutRes {w(cpu_in.ds, 0x4426):04x} magic {w(var[0], (var[1] + 2) & 0xFFFF):04x} BX {final.bx:04x}; expected {res:04x} {mag:04x} {bx:04x}')
        check((final.ip, final.cs, final.sp) == (w(cpu_in.ss, cpu_in.sp), w(cpu_in.ss, cpu_in.sp + 2), (cpu_in.sp + 8) & 0xFFFF), f'{label}: RETF 4 frame differs')
        if magic != 0xD7B3:
            check(final.flags & 0x08D5 == p_cmp_flags(magic, 0xD7B3), f'{label}: CMP flags differ')
    return seen, edges, got, state['ints']


def service_refusals(span, fn):
    """The service refuses or answers with a non-continuation: SERVICE, stopped at the INT, nothing after it applied."""
    rng = random.Random(0x14910001)
    done = 0
    for mode in range(5):
        memory, cpu, _ = fixture(span, rng, 6, 0xD7B3, inout=0x1234)
        ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)

        def service(context, before, after, mode=mode):
            if mode == 0:
                return 1
            after[0] = Cpu.from_buffer_copy(before[0])
            after[0].ip = 0x14A7 if mode == 1 else 0x14A8
            if mode == 2:
                after[0].bx ^= 1
            if mode == 3:
                after[0].flags ^= 0x40
            if mode == 4:
                after[0].es ^= 1
            return 0
        got = fn(ram, SIZE, ctypes.byref(cpu), SERVICE_FN(service), None)
        check(got == 2 and cpu.ip == INT_AT and (cpu.ax >> 8) == 0x3E and cpu.bx == 6, f'refusal {mode}: status {got}')
        check(ram[physical(DATA_SEG, 0x0102)] | (ram[physical(DATA_SEG, 0x0103)] << 8) == 0xD7B3 and
              ram[physical(DEFAULT_DATA, 0x4426)] | (ram[physical(DEFAULT_DATA, 0x4427)] << 8) == 0x1234, f'refusal {mode}: state after the INT applied')
        done += 1
    return done


def differential(span, starts, conditional, fn):
    rng = random.Random(0x08AF1491)
    cases = []                                                      # (label, handle, magic, kwargs, plain)
    handles = (0, 1, 2, 3, 4, 5, 6, 0x00FF, 0x7FFF, 0x8000, 0xFFFF)
    magics = (0xD7B3, 0xD7B0, 0xD7B1, 0xD7B2, 0x0000, 0xFFFF, 0xD7B4)
    services = ((0x3E11, 0), (0x0006, 1), (0xFFFF, 1), (0x0000, 0))
    n = 0
    for handle in handles:
        for magic in magics:
            for si, svc in enumerate(services):
                if magic != 0xD7B3 and si:                          # the service is never reached on a bad magic
                    continue
                for inout in (0, 0x1234):
                    cases.append((f'h{handle:04x}-m{magic:04x}-s{si}-i{inout:04x}', handle, magic, dict(flags=FLAGS[n % len(FLAGS)], svc=svc, inout=inout), True))
                    n += 1
    for sp in (0x0002, 0x0003, 0x0100, 0xFFF8, 0xFFF7):
        for handle, magic in ((5, 0xD7B3), (2, 0xD7B3), (9, 0xD7B0)):
            cases.append((f'sp{sp:04x}-h{handle}-m{magic:04x}', handle, magic, dict(sp=sp), False))
    for di in (0xFFFC, 0xFFFD, 0xFFFE, 0xFFFF):                       # the magic word's offset wraps inside the segment, the handle word straddles FFFF
        for handle, magic in ((5, 0xD7B3), (3, 0xD7B3), (5, 0xD7B1)):
            cases.append((f'var-off-{di:04x}-h{handle}-m{magic:04x}', handle, magic, dict(var=(DATA_SEG, di)), False))
    for data in (0x0000, 0xFFFF, 0x1234, CODE):
        for handle, magic in ((5, 0xD7B3), (9, 0xD7B2)):
            cases.append((f'ds{data:04x}-h{handle}-m{magic:04x}', handle, magic, dict(data=data), data != 0xFFFF))
    cases.append(('a20-stack', 5, 0xD7B3, dict(ss=0xFFFF, sp=0x0013), False))
    cases.append(('a20-var', 5, 0xD7B3, dict(var=(0xFFFF, 0x0020)), False))
    cases.append(('var-on-stack', 5, 0xD7B3, dict(var=(0x4000, 0x8010)), False))
    for handle, magic in ((5, 0xD7B3), (2, 0xD7B3), (9, 0xD7B0), (0, 0xD7B3)):
        cases.append((f'random-ram-h{handle}', handle, magic, dict(background=rng.randbytes(SIZE)), False))
    for di in (0x4420, 0x4422, 0x4424, 0x4425, 0x4426, 0x4428):       # the variable overlaps InOutRes: the handle or the magic IS InOutRes
        for handle, magic, svc in ((5, 0xD7B3, (0x0006, 1)), (5, 0xD7B3, (0x3E11, 0)), (2, 0xD7B3, (0, 0))):
            cases.append((f'var-on-inoutres-{di:04x}-h{handle}-s{svc[1]}', handle, magic, dict(var=(DEFAULT_DATA, di), inout=None, svc=svc, flags=0x0ED7), False))
    for sp in range(0x4420, 0x4436):                                 # SS == DS: the helper's return slot, the frame and InOutRes alias (TRANSFER included)
        for magic in (0xD7B3, 0xD7B1):
            cases.append((f'stack-in-data-{sp:04x}-m{magic:04x}', 5, magic, dict(ss=DEFAULT_DATA, sp=sp, flags=0x0202), False))
    seen, edges, statuses, plain, ints = set(), set(), {}, 0, 0
    for label, handle, magic, kw, is_plain in cases:
        memory, cpu, svc = fixture(span, rng, handle, magic, **kw)
        independent = is_plain and kw.get('ss', 0x4000) != kw.get('data', DEFAULT_DATA)
        var = kw.get('var', (DATA_SEG, 0x0100))
        s, e, status, n_ints = compare(memory, cpu, fn, label, conditional, svc, (handle, magic, kw.get('inout', 0), var) if independent else None)
        plain += 1 if independent else 0
        statuses[status] = statuses.get(status, 0) + 1
        ints += n_ints
        seen |= s
        edges |= e
    check(seen == starts, f'starts missing: {sorted(starts - seen)}')
    wanted = {(a, b) for a, pair in conditional.items() for b in pair}
    check(edges >= wanted, f'outcomes missing: {sorted(wanted - edges)}')
    check(set(statuses) == {0, 3}, f'stops reached {sorted(statuses)}')
    refusals = service_refusals(span, fn)
    memory, base, _ = fixture(span, rng, 5, 0xD7B3)
    rejects = 0
    for sp, ip, size in ((0x0001, ENTRY, SIZE), (0xFFF9, ENTRY, SIZE), (0xFFFF, ENTRY, SIZE), (0x8000, ENTRY + 1, SIZE), (0x8000, ENTRY, SIZE - 1)):
        cpu = Cpu.from_buffer_copy(base)
        cpu.sp, cpu.ip = sp, ip
        ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
        before = bytes(cpu)
        check(fn(ram, size, ctypes.byref(cpu), SERVICE_FN(lambda c, b, a: 1), None) == 1 and bytes(cpu) == before and bytes(ram) == memory, 'ARGUMENT mutation')
        rejects += 1
    return dict(cases=len(cases), reference_checked_cases=plain, argument_cases=rejects, service_refusal_cases=refusals, starts=len(seen), starts_total=len(starts),
                conditional_outcomes=len(edges & wanted), conditional_outcomes_total=len(wanted), interrupts_issued=ints, calls=0, cpu_words=14, ram_bytes=SIZE,
                masked_ram_bytes=0, stops={'RETURN': statuses.get(0, 0), 'TRANSFER': statuses.get(3, 0), 'SERVICE': refusals, 'ARGUMENT': rejects})


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
    """48 synthetic runs: 16 (handle, magic, DOS answer) combinations under three flag images."""
    base = [(5, 0xD7B3, 0x3E11, 0), (5, 0xD7B3, 0x0006, 1), (6, 0xD7B3, 0xFFFF, 1), (0x7FFF, 0xD7B3, 0x0000, 0), (0xFFFF, 0xD7B3, 0x0005, 1),
            (4, 0xD7B3, 0x0000, 0), (3, 0xD7B3, 0x0000, 0), (0, 0xD7B3, 0x0000, 0), (1, 0xD7B3, 0x0000, 0), (2, 0xD7B3, 0x0000, 0),
            (9, 0xD7B0, 0x0000, 0), (9, 0xD7B1, 0x0000, 0), (9, 0xD7B2, 0x0000, 0), (9, 0x0000, 0x0000, 0), (9, 0xFFFF, 0x0000, 0), (5, 0xD7B3, 0x0102, 1)]
    return [dict(ax=ax, cf=cf, flags=(n // len(base) * 2 + n) % len(NATIVE_FLAGS),
                 writes=[(0x0100, h), (0x0102, m), (0x4426, 0x1234)]) for n, (h, m, ax, cf) in enumerate(base * 3)][:NATIVE_CASES]


def native_audit(span, function):
    directory = ROOT / 'resources/oracle-runs/system-close-1491-synthetic'
    directory.mkdir(parents=True, exist_ok=True)
    cases = native_cases()
    table = b''.join((struct.pack('<BBHHHH', c['flags'], 0, len(c['writes']), c['ax'], c['cf'], 0) +
                      b''.join(struct.pack('<HH', o, v) for o, v in c['writes'])).ljust(CASE_SIZE, b'\0') for c in cases)
    check(CASES_AT + len(table) <= OUTPUT, 'COM layout overlaps')
    stubs = []
    t = ['.code16', '.intel_syntax noprefix', '.global _start', '_start:', '    jmp main', f'.org {MAIN_AT - 0x100:#x}', 'main:',
         '    mov ax,cs', '    mov ds,ax', '    mov es,ax', '    mov ss,ax', f'    mov sp,{STACK_TOP:#x}', '    cld',
         f'    mov ax,{CALLER_N:#x}', '    mov es,ax', '    mov si,offset wrap', f'    mov di,{ENTRY:#x}', f'    mov cx,{len(span):#x}', '    rep movsb',
         '    xor ax,ax', '    mov es,ax', '    mov ax,word ptr es:[0x84]', '    mov word ptr [old21],ax', '    mov ax,word ptr es:[0x86]', '    mov word ptr [old21+2],ax',
         '    mov word ptr es:[0x84],offset int21', '    mov word ptr es:[0x86],cs']
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
          '    mov ax,word ptr [bx+4]', '    mov word ptr [case_ax],ax', '    mov ax,word ptr [bx+6]', '    mov word ptr [case_cf],ax',
          f'    push {DATA_N:#x}', '    push 0x0100', '    push cs', '    mov ax,offset returned', '    push ax',
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
          '    xor ax,ax', '    mov es,ax', '    mov ax,word ptr [old21]', '    mov word ptr es:[0x84],ax', '    mov ax,word ptr [old21+2]', '    mov word ptr es:[0x86],ax',
          '    mov bx,word ptr [handle]', '    mov ah,0x3e', '    int 0x21', '    mov ax,0x4c00', '    int 0x21',
          'failure:', '    mov ax,0x4c01', '    int 0x21',
          # Declared stub: AH=3Eh answers with the case's AX and CF (stacked flags image) and IRETs; every other function chains to DOS.
          'int21:', '    cmp ah,0x3e', '    jne chain', '    push bp', '    mov bp,sp', '    mov ax,word ptr cs:[case_ax]', '    and word ptr [bp+6],0xfffe',
          '    cmp word ptr cs:[case_cf],0', '    je no_cf', '    or word ptr [bp+6],1', 'no_cf:', '    pop bp', '    iret', 'chain:', '    jmp dword ptr cs:[old21]',
          'filename: .asciz "RESULT.BIN"', 'handle: .word 0', 'index: .word 0', 'old21: .word 0,0', 'case_ax: .word 0', 'case_cf: .word 0', f'target_far: .word {ENTRY:#x}, {CALLER_N:#x}',
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
        for value in (DATA_N, 0x0100, segment, returned):
            sp -= 2
            put16(memory, segment, sp, value)
        put16(memory, segment, sp - 2, NATIVE_FLAGS[case['flags']])       # the harness' PUSHed flag word stays below the entry SP
        regs = list(ENTRY_REGS.values())
        cpu = Cpu(*regs, sp, CALLER_N, ENTRY, DATA_N, segment, segment, NATIVE_FLAGS[case['flags']])
        ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
        entry_sp, calls = sp, []

        def service(context, before, after, case=case):
            calls.append(1)
            after[0] = Cpu.from_buffer_copy(before[0])
            after[0].ip = 0x14A8
            after[0].ax = case['ax']
            after[0].flags = (before[0].flags & ~1) | case['cf']
            return 0
        status = function(ram, SIZE, ctypes.byref(cpu), SERVICE_FN(service), None)
        check(status == 0, f'C status {status} on native case {n}')
        seen_status[('INT' if calls else 'NOINT')] = seen_status.get('INT' if calls else 'NOINT', 0) + 1
        if calls:                                                    # DOS-side: the INT frame it pushed below the INT-time SP (declared limit)
            for k in range(1, 7):
                ram[physical(segment, entry_sp - k)] = record[28 + (entry_sp - k - WINDOW_LOW)]
        for (reg, _), want in zip(Cpu._fields_, struct.unpack_from('<14H', record)):
            check(getattr(cpu, reg) == want, f'native case {n} {reg}: C={getattr(cpu, reg):04x}, DOS={want:04x}')
        got = bytes(ram[physical(segment, WINDOW_LOW):physical(segment, WINDOW_LOW) + WINDOW_SIZE]) + bytes(ram[physical(DATA_N, 0):physical(DATA_N, 0) + 0x10000])
        if got != record[28:]:
            differences = [i for i, (x, y) in enumerate(zip(got, record[28:])) if x != y]
            raise ValueError(f'native case {n} bytes differ at {[hex(d) for d in differences[:8]]} ({len(differences)} bytes)')
    check(set(seen_status) == {'INT', 'NOINT'}, f'native paths reached {sorted(seen_status)}')
    probe = subprocess.run(['dosbox-x', '--version'], capture_output=True, text=True)
    version = [l.split(',')[0].strip() for l in (probe.stdout + probe.stderr).splitlines() if l.strip().startswith('DOSBox-X version')][:1]
    binary = subprocess.run(['which', 'dosbox-x'], capture_output=True, text=True).stdout.strip()
    digest = sha256(result).hexdigest()
    report = dict(dosbox_x=dict(path=binary, sha256=sha256(Path(binary).read_bytes()).hexdigest(), version=version[0] if version else '',
                                pinned_source='5a9c7c53481c2e933948039c33256776653f4faf (not used: declared substitution)'),
                  cases=NATIVE_CASES, result_sha256=digest, cpu_words=14, ram_bytes=WINDOW_SIZE + 65536,
                  masked_ram_bytes=SIZE - WINDOW_SIZE - 65536, synthetic_frames=True, callee_stubs=0, dos_service_stub='INT 21h AH=3Eh hook answering the case AX/CF by IRET; other functions chain to DOS', int_frame_bytes_excluded=6,
                  paths=seen_status, com_segment=f'{segment:04x}')
    print('1491 native parity', json.dumps(report, sort_keys=True), flush=True)
    return report


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--compile-only', action='store_true')
    p.add_argument('--unicorn', action='store_true')
    p.add_argument('--native', action='store_true')
    p.add_argument('--output', type=Path)
    a = p.parse_args()
    report = {}
    with tempfile.TemporaryDirectory(prefix='poolrad-close-') as tmp:
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
