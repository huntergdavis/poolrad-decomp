#!/usr/bin/env python3
"""RES_0709_0768 (the game's text-window printer) against the original bytes with every callee REAL: StrStore, set expansion
0D12, set membership 0DC0, string load 0B0B, 0899:0104, the window clear 0DD3 (and its EGA closure), the word printer 06A7
(glyph 01DF, UpCase, CRT Delay) and the skip-spaces helper 0714 - and, on the window-full path, 0BF5 ("Press any key to
continue": 05EB, 0626, ReadKey 0899:0059), which the C model stops at (STOP_BF5) and resumes after (the two-phase seam).

The whole loader-expanded START.EXE image is loaded at a test base with the loader's own relocation table applied; no byte is
patched. Declared environment, identical on both sides where both sides run: the bus's port environment (every OUT recorded in
order, fail-closed: any IN or interrupt inside the compared segments refuses), the EGA as pr_ega on the C side and flat RAM on
the original's side (compared through the ordered VRAM write-operand stream and the aperture replay, as in the 06A7 and 0DD3
audits). Inside 0BF5 (original only): ReadKey's services (INT 21h 2Ch/25h/35h, ports 40h/43h/61h/C0h, INT 16h under the
#1326 keyboard contract) and the real-byte INT 10h stub. A scripted environment proves register, memory and EGA-model effects;
it proves nothing about real hardware.

Catchers: the unmasked CPU14 + RAM (outside the A000 aperture) + OUT stream + VRAM write stream differential, per segment; an
independent statement of the printer (statement()) for the cursor DS:5E24/5E25, the ordered glyph draws (character, x, y) seen
at 01DF's entry, the clears and the window-full stop; and the CRT Delay loop-back count predicted from DS:49E2, DS:84FC and
DS:4A00. Media stay under ignored resources/."""
import argparse, ctypes, json, random, struct, subprocess, sys, tempfile, time
from hashlib import sha256
from pathlib import Path
from capstone import Cs, CS_ARCH_X86, CS_MODE_16
from res0709_ega_reset_0000_cpu_audit import Cpu, Env, OUT_FN, IN_FN, ROOT, SIZE, REGISTERS, physical, put16, check
import res0899_quit_0000_cpu_audit as Q
import res0802_0716_cpu_audit as A716
CHECKER_ROWS = ('RES_0709_0768',)
ENTRY, END = 0x0768, 0x09E3
DIGEST = 'baedd54c715b714b38db85632ef9dfa9314e01e1749adf29544af9c765576668'
BASE = Q.BASE
TEXT, CRT, IDLE = BASE + 0x0709, BASE + 0x0A40, BASE + 0x0899
STR_SEG, FONT_SEG, SS_ = 0x5000, 0x6000, 0x7000
RETURN_TO = (0x9001, 0x7000)
import res0709_text_06a7_cpu_audit as T06A7
import res0709_clear_0dd3_cpu_audit as C0DD3
SOURCES = list(dict.fromkeys(['resident_0709_printer_0768_cpu', 'resident_0709_skip_spaces_0714_cpu', 'resident_0899_idle_0104_cpu',
                              'system_set_expand_0d12_cpu', 'system_runtime_leaves_cpu', 'system_string_leaves_cpu',
                              'system_strstore_fullstate_0b25'] + list(T06A7.SOURCES) + list(C0DD3.SOURCES)))
FAR_SITES = [0x077E, 0x0848, 0x084D, 0x0880, 0x0885, 0x08E0, 0x08E5, 0x0990, 0x0999]
NEAR_SITES = {0x07E4: 0x0DD3, 0x0955: 0x06A7, 0x0964: 0x0714, 0x0999: 0x0BF5, 0x09B2: 0x0DD3, 0x09B7: 0x06A7, 0x09BE: 0x06A7}
STATUS = {0: 'RETURN', 1: 'ARGUMENT', 2: 'STOP_BF5', 3: 'LIMIT', 4: 'CALLEE'}
DIRTY_FLAGS = (0x0202, 0x0602, 0x0A87, 0x0402, 0x0E57)
DELAY_BACKS = (0x2C1, 0x2C6, 0x2B8)
# Conditional successors no case reaches, each with its argument (fail-closed both ways: an excluded edge that IS reached fails).
EXCLUDED = {
    (0x093B, 0x093D): ('dead code', '0919..0939 recomputes exactly the 08F2..0912 value (no instruction between them writes '
                       'memory or the operands), and 0914 JG was taken, so AX > BX and 093B JNE is always taken; 093D..0952 '
                       '(DEC j; CALL 06A7) is never reached'),
    (0x094A, 0x094C): ('dead code', 'reachable only through 093B falling through (above)'),
    (0x094A, 0x0955): ('dead code', 'reachable only through 093B falling through (above)'),
}
DEAD = set(range(0x093D, 0x0955))


class Ega(ctypes.Structure):
    _fields_ = [('plane', (ctypes.c_uint8 * 65536) * 4), ('latch', ctypes.c_uint8 * 4), ('seq_index', ctypes.c_uint8),
                ('seq', ctypes.c_uint8 * 5), ('gc_index', ctypes.c_uint8), ('gc', ctypes.c_uint8 * 9)]


WRITE_FN = ctypes.CFUNCTYPE(None, ctypes.c_void_p, ctypes.c_uint16, ctypes.c_uint8)


class Bus(ctypes.Structure):
    _fields_ = [('ram', ctypes.POINTER(ctypes.c_uint8)), ('ega', ctypes.POINTER(Ega)), ('ports', ctypes.POINTER(Env)),
                ('on_vram_write', WRITE_FN), ('ctx', ctypes.c_void_p)]


class Budget(ctypes.Structure):
    _fields_ = [('clear_loopbacks', ctypes.c_size_t), ('clear_rect_loopbacks', ctypes.c_size_t), ('text_loopbacks', ctypes.c_size_t),
                ('text_glyph_loopbacks', ctypes.c_size_t), ('text_delay_loopbacks', ctypes.c_uint64), ('skip_loopbacks', ctypes.c_uint),
                ('word_loopbacks', ctypes.c_size_t), ('scan_loopbacks', ctypes.c_size_t)]


LOOPBACKS = {0x0858: 'scan', 0x089F: 'scan', 0x08F0: 'scan', 0x09C8: 'word'}
STATEMENT_PASSES = 4096                                               # the statement's own budget (passes of its word loop)


class Diag(ctypes.Structure):
    _fields_ = [('site', ctypes.c_uint16), ('callee_status', ctypes.c_int), ('nested', ctypes.c_int * 4)]


def source(image):
    body = image[0x709 * 16 + ENTRY:0x709 * 16 + END]
    check(sha256(body).hexdigest() == DIGEST, '0709:0768 identity')
    ins = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(body, ENTRY))
    check(len(ins) == 246 and sum(i.size for i in ins) == len(body), 'decode covers 0768')
    check([i.address for i in ins if i.mnemonic == 'lcall'] == FAR_SITES, 'the nine far-call sites')
    check({i.address + i.size: int(i.op_str, 16) for i in ins if i.mnemonic == 'call'} == NEAR_SITES, 'the seven near-call sites')
    check([i.address for i in ins if i.mnemonic in ('retf', 'ret')] == [0x09E0], 'one exit')
    check(not [i for i in ins if i.mnemonic in ('in', 'out', 'int', 'insb', 'outsb')], 'no port I/O or interrupt in the body')
    conds = {i.address: (int(i.op_str, 16), i.address + i.size) for i in ins if i.mnemonic.startswith('j') and i.mnemonic != 'jmp'}
    check(len(conds) == 25, f'25 conditional sites, got {len(conds)}')
    return body, {i.address: i for i in ins}, conds


def library(directory):
    path = directory / 'r0768.so'
    subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Wpedantic', '-Werror', '-O2', '-fPIC', '-shared', '-I', str(ROOT / 'include')]
                   + [str(ROOT / f'src/recovered/{n}.c') for n in SOURCES] + ['-o', str(path)], check=True)
    lib = ctypes.CDLL(str(path)); f = lib.pr_r0709_printer_0768_cpu
    f.argtypes = [ctypes.POINTER(Bus), ctypes.c_size_t, ctypes.POINTER(Cpu), ctypes.POINTER(Budget), ctypes.POINTER(Diag)]
    f.restype = ctypes.c_int
    lib.pr_ega_reset.argtypes = [ctypes.POINTER(Ega)]
    return lib


def build(reloc, stub, *, text=b'HELLO WORLD', window=(1, 1, 20, 5), cursor=(1, 1), clear=0, color=5, gate=0, speed=1,
          calibration=2, mode=1, keys=(0x1C0D,), typeahead=(), sp=0xF000, ss=SS_, flags=2, regs=None, ret=RETURN_TO, writes=(),
          word=65536, scan=65536, period=None):
    m = bytearray([0x5A]) * SIZE
    m[BASE * 16:BASE * 16 + len(reloc)] = reloc
    ds = BASE + Q.SEG['data']
    end = BASE * 16 + len(reloc)
    if end < physical(ds, 0xFFFF):
        m[end:physical(ds, 0xFFFF) + 1] = bytes(physical(ds, 0xFFFF) + 1 - end)   # the data segment's BSS, as DOS leaves it
    s = physical(Q.STUB_SEG, 0); m[s:s + len(stub)] = stub; put16(m, 0, 0x40, 0); put16(m, 0, 0x42, Q.STUB_SEG)
    m[physical(ds, 0x5E24)], m[physical(ds, 0x5E25)] = cursor
    m[physical(ds, 0x4A00)] = gate; m[physical(ds, 0x49E2)] = speed; put16(m, ds, 0x84FC, calibration)
    m[physical(ds, 0x4A1E)] = mode; put16(m, ds, 0x4A16, 0xA000); put16(m, ds, 0x4A18, 0xA200)
    put16(m, ds, 0x5E20, 0x1000); put16(m, ds, 0x5E22, FONT_SEG)
    for j in range(2048):
        m[physical(FONT_SEG, 0x1000 + j)] = ((j * 37 + (j >> 3) * 13) ^ 0xA6) & 255
    m[0x400] = 0x42                                               # the BIOS byte CRT Delay compares (0040:0000), never changed
    m[physical(STR_SEG, 0x100)] = len(text) & 255
    m[physical(STR_SEG, 0x101):physical(STR_SEG, 0x101) + len(text)] = text
    x1, y1, x2, y2 = window
    ss = ds if ss == 'ds' else ss
    for k, w in enumerate((ret[1], ret[0], 0x0100, STR_SEG, 0xA500 | clear, 0xA500 | color, 0xA500 | y2, 0xA500 | x2, 0xA500 | y1,
                           0xA500 | x1)):
        put16(m, ss, (sp + 2 * k) & 0xFFFF, w)
    for seg, off, blob in writes:
        m[physical(seg, off):physical(seg, off) + len(blob)] = blob
    r = regs or (0x1234, 0x3456, 0x5678, 0x789A, 0x9ABC, 0xBCDE, 0x7777)
    return m, Cpu(*r, sp, TEXT, ENTRY, ds, 0x1111, ss, flags), dict(keys=list(keys), typeahead=list(typeahead), ret=ret, word=word,
                                                                    scan=scan, period=period)


def run_unicorn(name, m, c, spec, decoded, conds):
    from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE, UC_HOOK_INTR, UC_HOOK_INSN, UC_HOOK_MEM_WRITE
    from unicorn import x86_const as R
    env = Q.Env(table={8: (0x0123, 0xF000)}, in61=0xB0, time_start=4736147, time_step=7)
    keys, ahead = list(spec['keys']), list(spec['typeahead'])
    uc = Uc(UC_ARCH_X86, UC_MODE_16); backing = (ctypes.c_uint8 * SIZE).from_buffer_copy(m)
    uc.mem_map_ptr(0, SIZE, 7, ctypes.addressof(backing)); uc.mem_map_ptr(SIZE, SIZE, 7, ctypes.addressof(backing))
    for (n, _), reg in zip(Cpu._fields_, REGISTERS):
        uc.reg_write(reg, getattr(c, n))
    s = dict(stop=None, prev=None, pending=False, steps=0, word=0, scan=0); seen, edges, snaps, period = set(), set(), [], {}
    events, writes, draws, clears, delay_backs, bf5_events = [], [], [], [0], [0], []
    known = {BASE + v for v in Q.SEG.values()} | {TEXT, CRT, IDLE, Q.STUB_SEG, BASE + 0x0A40}

    def snap(kind):
        snaps.append(dict(kind=kind, cpu=Cpu(*[uc.reg_read(r) & 0xFFFF for r in REGISTERS]), ram=bytes(backing),
                          events=len(events), writes=len(writes)))

    def code(em, address, size, user):
        cs = uc.reg_read(R.UC_X86_REG_CS); ip = uc.reg_read(R.UC_X86_REG_IP)
        if s['prev'] is not None and cs == TEXT:
            edges.add((s['prev'], ip))
        s['prev'] = None
        if (cs, ip) == spec['ret']:
            snap('RETURN'); s['stop'] = 'RETURN'; return uc.emu_stop()
        check(cs in known, f'escape {name} {cs:04x}:{ip:04x}')
        sp_ = uc.reg_read(R.UC_X86_REG_SP); ss_ = uc.reg_read(R.UC_X86_REG_SS)
        if cs == TEXT and ip == 0x0BF5 and not s['pending']:
            if struct.unpack('<H', bytes(backing[physical(ss_, sp_):physical(ss_, sp_) + 2]))[0] == 0x0999:
                snap('STOP_BF5'); s['pending'] = True
        if cs == TEXT and ip == 0x0999 and s['pending']:
            snap('RESUME'); s['pending'] = False; s['word'] = s['scan'] = 0    # the C model's budgets are fresh per entry
        if cs == TEXT and ip in LOOPBACKS and not s['pending']:
            kind = LOOPBACKS[ip]
            if s[kind] >= spec[kind]:                                  # the arrival the C model refuses: same state, JMP not run
                snap('LIMIT'); snaps[-1]['site'] = ip; s['stop'] = 'LIMIT'; return uc.emu_stop()
            s[kind] += 1
            if spec['period'] and kind == 'word' and s['word'] in spec['period']:
                period[s['word']] = (bytes(Cpu(*[uc.reg_read(r) & 0xFFFF for r in REGISTERS])), bytes(backing), len(events), len(writes))
        if cs == TEXT and ip == 0x01DF and not s['pending']:            # a glyph draw: (char, x, y) from its frame
            w = lambda k: backing[physical(ss_, (sp_ + k) & 0xFFFF)]
            draws.append((w(6), w(14), w(12)))
        if cs == TEXT and ip == 0x0DD3 and not s['pending']:
            clears[0] += 1
        if cs == CRT and ip in DELAY_BACKS and not s['pending']:
            fl = uc.reg_read(R.UC_X86_REG_EFLAGS)
            taken = (uc.reg_read(R.UC_X86_REG_CX) != 1 and bool(fl & 0x40)) if ip == 0x2C6 else not fl & 0x40
            delay_backs[0] += 1 if taken else 0
        if cs == TEXT and ENTRY <= ip < END and not s['pending']:
            check(ip in decoded, f'escape inside 0768 {name} {ip:04x}'); seen.add(ip)
            if ip in conds:
                s['prev'] = ip
        s['steps'] += 1
        if s['steps'] > 20000000:
            s['stop'] = 'RUNAWAY'; uc.emu_stop()

    def intr(em, intno, user):
        ax = uc.reg_read(R.UC_X86_REG_AX); ah, al = ax >> 8, ax & 0xFF; fl = uc.reg_read(R.UC_X86_REG_EFLAGS)
        check(s['pending'], f'ENV refusal {name}: INT {intno:#x} outside 0BF5')
        if intno == 0x16:                                            # the #1326 contract
            check(ah in (0, 1), f'ENV refusal {name}: INT 16h AH {ah:#x}')
            if ah == 1:
                uc.reg_write(R.UC_X86_REG_EFLAGS, (fl & ~0x40) if ahead else (fl | 0x40))
                if ahead:
                    uc.reg_write(R.UC_X86_REG_AX, ahead[0])
                return
            src = ahead if ahead else keys
            check(bool(src), f'ENV refusal {name}: INT 16h with the key script empty'); uc.reg_write(R.UC_X86_REG_AX, src.pop(0)); return
        check(intno == 0x21, f'ENV refusal {name}: INT {intno:#x}')
        if ah == 0x2C:
            cx, dx = env.gettime(); uc.reg_write(R.UC_X86_REG_CX, cx); uc.reg_write(R.UC_X86_REG_DX, dx); return
        check(ah in (0x25, 0x35), f'ENV refusal {name}: INT 21h AH {ah:#x}')
        for k, v in ((2, fl & 0xFFFF), (4, uc.reg_read(R.UC_X86_REG_CS)), (6, uc.reg_read(R.UC_X86_REG_IP))):
            uc.mem_write(physical(uc.reg_read(R.UC_X86_REG_SS), (uc.reg_read(R.UC_X86_REG_SP) - k) & 0xFFFF), struct.pack('<H', v))
        out = env.dos(ah, al, dict(ds=uc.reg_read(R.UC_X86_REG_DS), dx=uc.reg_read(R.UC_X86_REG_DX)))
        for k, v in out.items():
            uc.reg_write(dict(es=R.UC_X86_REG_ES, bx=R.UC_X86_REG_BX)[k], v)

    def port_in(em, port, size, user):
        check(s['pending'] and port == 0x61 and size == 1, f'ENV refusal {name}: IN {port:#x}'); return env.in61

    def port_out(em, port, size, value, user):
        if s['pending']:
            # inside 0BF5 (original only): ReadKey's driver ports and 0626's own EGA programming (SEQ/GC), recorded apart
            check(port in (0x40, 0x43, 0x61, 0xC0) and size == 1 or port in (0x3C4, 0x3C5, 0x3CE, 0x3CF), f'ENV refusal {name}: OUT {port:#x} in 0BF5')
            bf5_events.append((port, value, size)); return
        events.append((port, value, size, uc.reg_read(R.UC_X86_REG_CS), uc.reg_read(R.UC_X86_REG_IP)))

    def mem_write(em, access, address, size, value, user):
        for j in range(size):
            at = (address + j) & (SIZE - 1)
            if 0xA0000 <= at < 0xB0000:
                writes.append((at - 0xA0000, (value >> (8 * j)) & 255))
    uc.hook_add(UC_HOOK_CODE, code); uc.hook_add(UC_HOOK_INTR, intr)
    uc.hook_add(UC_HOOK_INSN, port_in, None, 1, 0, R.UC_X86_INS_IN)
    uc.hook_add(UC_HOOK_INSN, port_out, None, 1, 0, R.UC_X86_INS_OUT)
    uc.hook_add(UC_HOOK_MEM_WRITE, mem_write)
    uc.emu_start(physical(c.cs, c.ip), 2 * SIZE, count=40000000)
    check(s['stop'] in ('RETURN', 'LIMIT'), f'original stop {name}: {s["stop"]}')
    return snaps, events, writes, draws, clears[0], delay_backs[0], seen, edges, period


def run_c(lib, m, c, spec):
    ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(m); ega = Ega(); lib.pr_ega_reset(ctypes.byref(ega))
    for p in range(4):
        ctypes.memset(ctypes.addressof(ega.plane[p]), 0x31 + p * 0x27, 65536)
    events, writes = [], []

    def out(ctx, port, value, size, cs, ip):
        events.append((port, value, size, cs, ip)); return 0

    def write(ctx, off, value):
        writes.append((off, value))
    keep = (OUT_FN(out), WRITE_FN(write))
    env = Env(None, keep[0], IN_FN()); bus = Bus(ram, ctypes.pointer(ega), ctypes.pointer(env), keep[1], None)
    b = Budget(65536, 65536, 65536, 65536, 10 ** 8, 65536, spec['word'], spec['scan']); d = Diag(); cpu = Cpu.from_buffer_copy(c)
    status = STATUS[lib.pr_r0709_printer_0768_cpu(ctypes.byref(bus), SIZE, ctypes.byref(cpu), ctypes.byref(b), ctypes.byref(d))]
    return status, cpu, bytes(ram), events, writes, d


def compare_segment(name, k, status, cpu, ram, events, writes, want, u_events, u_writes, ev_from, wr_from):
    check(status == want['kind'], f'{name} segment {k}: C {status} original {want["kind"]}')
    A716.compare_cpu(f'{name}#{k}', cpu, want['cpu'])
    exp = want['ram']
    check(ram[:0xA0000] + ram[0xB0000:] == exp[:0xA0000] + exp[0xB0000:], f'RAM outside the aperture {name}#{k}')
    check(events == u_events[ev_from:want['events']], f'OUT stream {name}#{k}: C {len(events)} original {want["events"] - ev_from}')
    check(writes == u_writes[wr_from:want['writes']], f'VRAM writes {name}#{k}: C {len(writes)} original {want["writes"] - wr_from}')


def two_phase(name, lib, m, c, spec, snaps, u_events, u_writes):
    cpu, ram, k, ev, wr, seams = c, bytes(m), 0, 0, 0, []
    while True:
        status, actual, ram_c, events, writes, d = run_c(lib, ram, cpu, spec)
        want = snaps[k]
        compare_segment(name, len(seams), status, actual, ram_c, events, writes, want, u_events, u_writes, ev, wr)
        seams.append(status)
        if status == 'LIMIT':
            check(d.site == want['site'] and d.callee_status == -1, f'{name}: LIMIT site {d.site:#x} vs {want["site"]:#x}')
        if status in ('RETURN', 'LIMIT'):
            return seams, ram_c, actual
        nxt = snaps[k + 1]
        check(nxt['kind'] == 'RESUME', f'{name}: no resume after {status}')
        cpu, ram, k, ev, wr = nxt['cpu'], nxt['ram'], k + 2, nxt['events'], nxt['writes']


SEPARATORS = None


def statement(o, sep):
    """Independent restatement of the printer from the decode and 06A7's documented loop (not from the C model): returns the
    final cursor, the ordered draws (char, x, y), the clears and whether the window filled (the 0BF5 path)."""
    x1, y1, x2, y2 = o.get('window', (1, 1, 20, 5)); cx, cy = o.get('cursor', (1, 1)); clear = o.get('clear', 0)
    t = bytes([len(o.get('text', b'HELLO WORLD')) & 255]) + o.get('text', b'HELLO WORLD')[:255]
    draws, clears, filled = [], 0, 0
    if x1 > 0x27 or y1 > 0x18 or x2 > 0x27 or y2 > 0x27:
        return (cx, cy), draws, clears, filled
    if not (x1 <= cx <= x2 and y1 <= cy <= y2):
        cx, cy = x1, y1
    if clear:
        clears += 1; cx, cy = x1, y1
    n = t[0]
    if n == 0:
        return (cx, cy), draws, clears, filled
    i = 1

    def at(k):
        return t[k] if k < len(t) else 0x5A
    member = lambda ch: ch in sep

    def show(i, j, cx):
        for _ in range(256):                                          # 06A7's index is a byte: j = 255 never ends
            if i > j:
                return i, cx
            draws.append((at(i), cx, cy)); i = (i + 1) & 255; cx = (cx + 1) & 255
        return None, cx
    for _ in range(STATEMENT_PASSES):
        j = i
        while j < n and member(at(j)):
            j += 1
        while j < n and not member(at(j)) and at(j) != 0x20:
            j += 1
        if at(j) != 0x20:
            while j + 1 < n and member(at(j + 1)):
                j += 1
        if j - i + cx > x2:
            cx = x1; cy = (cy + 1) & 255
            while i < n and at(i) == 0x20:
                i += 1
            if cy > y2 and i < n:
                cx, cy = x1, y1; filled += 1; clears += 1
                i, cx = show(i, j, cx)
        else:
            i, cx = show(i, j, cx)
        if i is None:
            return None
        if i > n:
            break
    else:
        return None                                                   # does not finish within the statement's budget
    if cx > x2:
        cx = x1; cy = (cy + 1) & 255
    return (cx, cy), draws, clears, filled


def cases():
    rng = random.Random(0x07090768)
    out = [('hello', dict()), ('cursor-outside', dict(cursor=(30, 9))), ('cursor-inside', dict(cursor=(4, 3))), ('cursor-below', dict(cursor=(4, 9))),
           ('last-char-overflow', dict(window=(1, 1, 4, 1), text=b'ABCD E')),
           ('clear', dict(clear=1)), ('clear-flag-ff', dict(clear=0xFF)), ('empty', dict(text=b'')), ('empty-clear', dict(text=b'', clear=1)),
           ('bad-x1', dict(window=(0x28, 1, 0x28, 5))), ('bad-y1', dict(window=(1, 0x19, 20, 0x19))), ('bad-x2', dict(window=(1, 1, 0x28, 5))),
           ('bad-y2', dict(window=(1, 1, 20, 0x28))), ('max-window', dict(window=(0, 0, 0x27, 0x18), text=b'EDGE OF THE SCREEN')),
           ('wrap-two-lines', dict(window=(2, 2, 10, 6), text=b'THE QUICK BROWN FOX JUMPS')),
           ('separators', dict(window=(1, 1, 12, 6), text=b'HALT! WHO GOES THERE? FRIEND, OR FOE; SPEAK:')),
           ('leading-spaces', dict(window=(1, 1, 8, 6), text=b'   SPACED   OUT   ')),
           ('long-word', dict(window=(1, 1, 6, 6), text=b'ABCDEFGHIJKLMNOP')),
           ('exact-fit', dict(window=(1, 1, 5, 4), text=b'ABCDE FGHIJ')),
           ('end-past-x2', dict(window=(1, 1, 5, 4), text=b'ABCDE')),
           ('window-full', dict(window=(1, 1, 8, 2), text=b'ONE TWO THREE FOUR FIVE SIX', keys=(0x1C0D,) * 4)),
           ('window-full-twice', dict(window=(1, 1, 6, 1), text=b'AAAA BBBB CCCC DDDD', keys=(0x1C0D,) * 8)),
           ('delay-on', dict(gate=1, speed=1, calibration=2, text=b'DELAY')), ('delay-speed-3', dict(gate=1, speed=3, calibration=1, text=b'SLOW')),
           ('delay-speed-0', dict(gate=1, speed=0, text=b'ZERO')), ('delay-gate-off', dict(gate=0, speed=5, text=b'FAST')),
           ('mode-0', dict(mode=0)), ('mode-2', dict(mode=2)),
           # declared budgets of this body's own loops: the C LIMIT against the original stopped at the same JMP arrival
           ('word-budget', dict(window=(2, 2, 10, 6), text=b'THE QUICK BROWN FOX JUMPS', word=2)),
           ('scan-budget-0858', dict(text=b'!!!AB', scan=1)), ('scan-budget-089f', dict(window=(1, 1, 6, 6), text=b'ABCDEFGHIJKLMNOP', scan=3)),
           ('scan-budget-08f0', dict(text=b'AB!!!!', scan=3)),
           ('word-budget-after-bf5', dict(window=(1, 1, 6, 1), text=b'AAAA BBBB CCCC DDDD', keys=(0x1C0D,) * 8, word=1)),
           # x1 > x2 passes the argument checks; a one-byte last word then never fits and the row advances forever
           ('x1-gt-x2-hang', dict(window=(10, 1, 5, 5), text=b'A', word=600, period=(300, 556)))]
    out += [(f'dirty-{fl:04x}', dict(flags=fl, window=(2, 2, 10, 6), text=b'DIRTY FLAGS WRAP')) for fl in DIRTY_FLAGS]
    out += [(f'flags-{fl:04x}', dict(flags=fl)) for fl in Q.FLAGS]
    out += [(f'sp-{sp:04x}', dict(sp=sp)) for sp in (0x0300, 0x8001, 0xFFEB, 0xFFEC)]
    for k in range(8):
        words = [bytes(rng.choice(b'ABCDEFGHIJ!,.-:;?') for _ in range(rng.randrange(1, 7))) for _ in range(rng.randrange(2, 9))]
        x1 = rng.randrange(0, 20); y1 = rng.randrange(0, 10)
        out.append((f'random{k}', dict(text=b' '.join(words), window=(x1, y1, x1 + rng.randrange(3, 15), y1 + rng.randrange(2, 8)),
                                       cursor=(rng.randrange(0, 40), rng.randrange(0, 25)), clear=rng.choice((0, 1)), keys=(0x1C0D,) * 8,
                                       flags=rng.choice(Q.FLAGS), regs=tuple(rng.randrange(65536) for _ in range(7)))))
    return out


def separators(reloc):
    ds = BASE + Q.SEG['data']; at = physical(ds, 0x0D16) - BASE * 16
    data = reloc[at:at + 4]; return {32 + 8 * k + b for k in range(4) for b in range(8) if data[k] >> b & 1}


def differential(reloc, stub, decoded, conds, lib):
    sep = separators(reloc)
    seen, edges, stops, checked, delay_checked, cs_, limits, hangs = set(), set(), {}, 0, 0, cases(), {}, []
    for index, (name, o) in enumerate(cs_):
        t0 = time.time(); print(f'[{index + 1}/{len(cs_)}] {name}: start', file=sys.stderr, flush=True)
        m, c, spec = build(reloc, stub, **o)
        snaps, ev, wr, draws, clears, backs, v, e, period = run_unicorn(name, m, c, spec, decoded, conds); seen |= v; edges |= e
        seams, ram, cpu = two_phase(name, lib, m, c, spec, snaps, ev, wr)
        stops[seams[-1]] = stops.get(seams[-1], 0) + 1
        if seams[-1] == 'LIMIT':
            limits[name] = f'{snaps[-1]["site"]:04X}'
        if spec['period']:
            # the original's full state (CPU14, 1 MiB, OUT and VRAM-write counts) at two 09C8 arrivals 256 passes apart: equal
            # state and no input inside the loop (any IN/INT outside 0BF5 refuses) means it never returns
            a, b = (period[k] for k in spec['period'])
            check(a[:2] == b[:2], f'{name}: the state at 09C8 arrivals {spec["period"]} differs')
            check(statement(o, sep) is None, f'{name}: the statement finishes but the original cycles')
            hangs.append(dict(case=name, arrivals=list(spec['period']), outs_between=b[2] - a[2], vram_writes_between=b[3] - a[3]))
        elif seams[-1] == 'LIMIT':
            pass                                                     # a budget case: the differential is the catcher
        elif not name.startswith(('random', 'sp-', 'flags-', 'dirty-')) or name.startswith('dirty-'):
            cur, want_draws, want_clears, filled = statement(o, sep)
            ds = BASE + Q.SEG['data']
            got = (ram[physical(ds, 0x5E24)], ram[physical(ds, 0x5E25)])
            check(got == cur, f'{name}: cursor {got} vs statement {cur}')
            check(draws == want_draws, f'{name}: draws {draws[:4]}.. ({len(draws)}) vs {want_draws[:4]}.. ({len(want_draws)})')
            check(clears == want_clears and seams.count('STOP_BF5') == filled, f'{name}: clears/fills {clears}/{seams.count("STOP_BF5")} vs {want_clears}/{filled}')
            checked += 1
            ds_ = lambda off: m[physical(ds, off)]
            if ds_(0x4A00):
                ms = (ds_(0x49E2) * 10) & 0xFFFF; cal = (m[physical(ds, 0x84FC)] | m[physical(ds, 0x84FD)] << 8) or 0x10000
                want_backs = len(draws) * (4 * cal * ms - 1) if ms else 0
                check(backs == want_backs, f'{name}: delay loop-backs {backs} vs predicted {want_backs}')
                delay_checked += 1
            else:
                check(backs == 0, f'{name}: delay ran with the gate off ({backs})')
        print(f'[{index + 1}/{len(cs_)}] {name}: {"/".join(seams)}, {len(draws)} draws, {time.time() - t0:.1f}s', file=sys.stderr, flush=True)
    want_edges = {(a, t) for a, ts in conds.items() for t in ts}
    check(seen == set(decoded) - DEAD, f'starts missing {sorted(hex(a) for a in set(decoded) - DEAD - seen)}')
    check(not (seen & DEAD), 'an excluded start was reached')
    missing = want_edges - edges
    check(missing == set(EXCLUDED), f'outcomes missing {sorted(missing - set(EXCLUDED))}; excluded but reached {sorted(set(EXCLUDED) - missing)}')
    return dict(cases=len(cs_), final_stops=stops, starts=len(seen), starts_total=len(decoded), starts_excluded=len(DEAD & set(decoded)),
                conditional_outcomes=len(edges & want_edges), conditional_outcomes_total=len(want_edges), excluded_outcomes=len(EXCLUDED),
                independent_checks=checked, delay_checks=delay_checked, separators=''.join(chr(x) for x in sorted(sep)),
                limit_sites=limits, proven_hangs=hangs,
                cpu_words=14, ram_bytes=SIZE, flag_masks=0)


def main():
    p = argparse.ArgumentParser()
    for flag in ('--compile-only', '--unicorn'):
        p.add_argument(flag, action='store_true')
    p.add_argument('--output', type=Path); a = p.parse_args()
    with tempfile.TemporaryDirectory(prefix='r0768-') as tmp:
        lib = library(Path(tmp))
        if a.compile_only:
            print('CHECKER_BUILD_OK'); return
        image, sites = Q.load_image()
        body, decoded, conds = source(image)
        report = dict(target=dict(span='0709:[0768,09E3)', bytes=len(body), instructions=len(decoded), sha256=DIGEST,
                                  image_sha256=Q.IMAGE_DIGEST, relocations=len(sites)))
        if a.unicorn:
            report['unicorn'] = differential(Q.relocate(image, sites, BASE), Q.INT10_STUB, decoded, conds, lib)
    if a.output:
        a.output.write_text(json.dumps(report, sort_keys=True, indent=2) + '\n')
    print(json.dumps(report, sort_keys=True))


if __name__ == '__main__':
    main()
