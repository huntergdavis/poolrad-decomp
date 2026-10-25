#!/usr/bin/env python3
"""RES_00BA_1509 (START.EXE resident 00BA [1509,15F7), remove a record from the character's +0C8h list) against the original bytes, as a composed closure:
the body, SYSTEM StrLoad 0AF8:0B0B, the resident prompt 0709:0BF5 with its whole closure (StrStore, the row clear 05EB, the printer 0626 with the +01DF glyph
writer, ReadKey-with-hotkeys 0899:0059 and ITS closure), and SYSTEM FreeMem 0AF8:0254.

The world is the 0BF5 audit's (res0709_msgbox_0bf5_cpu_audit): the loader-expanded START.EXE image at a test base with its relocations, the independent Python
EGA on the A000 aperture, and the declared environment (INT 16h from a per-case key script, INT 21h 25h/35h/2Ch, ports 40h/43h/61h/C0h, QUIT's INT 10h stub, the
EGA ports). This body runs at its own resident segment; a Turbo Pascal heap is set under the program's DS for FreeMem. Compared, unmasked, after every case:
CPU14, ALL 1,048,576 RAM bytes, the full EGA state, the ordered EGA OUT events, the ordered VRAM write operands, the ordered environment events, the stop and
the remaining budget (one unit per instruction of THIS body; a cyclic list never ends, as in DOS). FreeMem's RunError 204 is a named stop at SYSTEM:00E2 in
both runs. Media stay under ignored resources/."""
import argparse, ctypes, json, random, subprocess, sys, tempfile, time
from hashlib import sha256
from pathlib import Path
from capstone import Cs, CS_ARCH_X86, CS_MODE_16
import res0709_msgbox_0bf5_cpu_audit as M
import res0899_readkey_0059_cpu_audit as R
import res0899_quit_0000_cpu_audit as Q
import res0709_ega_reset_0000_cpu_audit as E0
import res0709_glyph_01df_cpu_audit as G
from res0709_ega_reset_0000_cpu_audit import Cpu, ROOT, SIZE, REGISTERS, physical, put16, check
from res00ba_display_1693_cpu_audit import PythonEga

CHECKER_ROWS = ('RES_00BA_1509',)
ENTRY, END = 0x1509, 0x15F7
RES = 0x00BA
BASE = M.BASE
SEG = M.SEG
SP_MIN, SP_MAX = 0x0200, 0xFFF2
BUDGET = 200000
STATUS = {0: 'RETURNED', 1: 'ARGUMENT', 2: 'CODE', 3: 'LIMIT', 4: 'TRANSFER', 5: 'CALLEE', 6: 'KEY', 7: 'RUNERROR'}
CALLS = [(0x1524, 'lcall', '0xaf8, 0xb0b'), (0x1529, 'lcall', '0x709, 0xbf5'), (0x15B3, 'lcall', '0xaf8, 0xb0b'), (0x15B8, 'lcall', '0x709, 0xbf5'),
         (0x15EC, 'lcall', '0xaf8, 0x254')]
RELOCS = [0x1527, 0x152C, 0x15B6, 0x15BB, 0x15EF]
LITERALS = (0x14CE, 0x14E2)
HEAP_ORG, HEAP_TOP, HEAP_END = (0x4000, 0), (0x4800, 0), (0x4C00, 0)
CHAR = (0x5000, 0x0100)
K = R.K


class Env1509(ctypes.Structure):
    _fields_ = [('prompt', M.Env0BF5), ('freemem_steps', ctypes.c_uint)]


class Diag1509(ctypes.Structure):
    _fields_ = [('callee', ctypes.c_int), ('callee_status', ctypes.c_int), ('site', ctypes.c_uint16), ('d0bf5', M.Diag0BF5)]


def source(image, sites):
    b = RES * 16
    body = bytes(image[b + ENTRY:b + END])
    ins = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(body, ENTRY))
    check(len(body) == 238 and len(ins) == 83 and sum(i.size for i in ins) == len(body), 'decode covers [1509,15F7): 238 bytes / 83 instructions')
    check([(i.address, i.mnemonic, i.op_str) for i in ins if 'call' in i.mnemonic] == CALLS, 'five far calls: StrLoad and the prompt twice, FreeMem once')
    check([(i.address, i.op_str) for i in ins if i.mnemonic in ('ret', 'retf')] == [(0x15F4, '8')], 'the only return is RETF 8 at 15F4')
    check(sorted(s - b for s in sites if b + ENTRY <= s < b + END) == RELOCS, 'the five far-call segment words are the only relocations')
    check(not [i for i in ins if i.mnemonic in ('int', 'in', 'out')], 'no interrupt or port instruction')
    for lit in LITERALS:
        check(not [s for s in sites if b + lit <= s < b + lit + 1 + image[b + lit]], f'the literal at CS:{lit:04X} has no relocation')
    meta = dict(span='00BA:[1509,15F7)', bytes=len(body), instructions=len(ins), sha256=sha256(body).hexdigest(), image_sha256=Q.IMAGE_DIGEST,
                literals={f'{lit:04X}': dict(length=image[b + lit], sha256=sha256(bytes(image[b + lit:b + lit + 1 + image[b + lit]])).hexdigest()) for lit in LITERALS})
    return body, {i.address: i for i in ins}, meta


def library(directory, override=None):
    path = Path(directory) / ('r1509_mut.so' if override else 'r1509.so')
    sources = list(R.SOURCES) + ['resident_0709_msgbox_0bf5_cpu', 'resident_0709_space_05eb_cpu', 'resident_0709_printer_0626_cpu', 'resident_0709_glyph_01df_cpu',
                                 'system_strstore_fullstate_0b25', 'system_runtime_leaves_cpu', 'system_string_leaves_cpu', 'system_freemem_0254_cpu',
                                 'resident_00ba_sidecar_remove_1509_cpu', 'memory_bus', 'ega_hw']
    subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Wpedantic', '-Werror', '-O2', '-fPIC', '-shared', '-I', str(ROOT / 'include')] +
                   [str((override or {}).get(s, ROOT / f'src/recovered/{s}.c')) for s in dict.fromkeys(sources)] + ['-o', str(path)], check=True, capture_output=True)
    lib = ctypes.CDLL(str(path))
    f = lib.pr_res_00ba_sidecar_remove_1509_cpu
    f.argtypes = [ctypes.POINTER(M.Bus), ctypes.c_size_t, ctypes.POINTER(Cpu), ctypes.POINTER(ctypes.c_size_t), ctypes.POINTER(Env1509), ctypes.POINTER(Diag1509)]
    f.restype = ctypes.c_int
    lib.pr_ega_reset.argtypes = [ctypes.POINTER(M.Ega)]
    return f, lib


def node_at(k):
    """The k-th record's heap address: 40h-byte slots from HeapOrg + 1 paragraph, offset 0 (a valid FreeMem block of 3Fh bytes)."""
    return (HEAP_ORG[0] + 1 + 4 * k, 0x0000)


def build(image_reloc, o):
    """The 0BF5 world (keys, hotkeys, display state), then this body's entry: the frame (record, character), the heap, the character's list."""
    mo = {k: v for k, v in o.items() if k in ('keys', 'cur', 'saved', 'w', 'flag49', 'buffered', 'brk', 'tail', 'in61', 'mode', 'pages', 'nofont', 'flags', 'cpu')}
    mo['sp'] = o.get('sp', 0xF000)
    m, c, envspec, keys = M.build(image_reloc, mo)
    ds = BASE + SEG['data']; ss = c.ss; sp = c.sp
    put16(m, ds, 0x4400, HEAP_ORG[1]); put16(m, ds, 0x4402, HEAP_ORG[0]); put16(m, ds, 0x4404, HEAP_TOP[1]); put16(m, ds, 0x4406, HEAP_TOP[0])
    put16(m, ds, 0x4408, HEAP_END[1]); put16(m, ds, 0x440A, HEAP_END[0]); put16(m, ds, 0x440C, HEAP_TOP[1]); put16(m, ds, 0x440E, HEAP_TOP[0])
    put16(m, ds, 0x4410, 0); put16(m, ds, 0x4412, 0)
    places = o.get('places') or [node_at(k) for k in range(o.get('nodes', 3))]
    for k, (seg, off) in enumerate(places):
        for j in range(0x40):
            m[physical(seg, (off + j) & 0xFFFF)] = (0x30 + 3 * j + k) & 0xFF
    order = o.get('order', list(range(len(places))))
    for i, k in enumerate(order):
        nxt = places[order[i + 1]] if i + 1 < len(order) else (o.get('tail_link') or (0, 0))
        seg, off = places[k]; put16(m, seg, (off + 0x2A) & 0xFFFF, nxt[1]); put16(m, seg, (off + 0x2C) & 0xFFFF, nxt[0])
    char = o.get('char', CHAR)
    head = places[order[0]] if order else (0, 0)
    put16(m, char[0], (char[1] + 0xC8) & 0xFFFF, head[1]); put16(m, char[0], (char[1] + 0xCA) & 0xFFFF, head[0])
    target = o.get('target_at') or (places[o['target']] if o.get('target') is not None else (0, 0))
    for k, w in enumerate((target[1], target[0], char[1], char[0])):
        put16(m, ss, (sp + 4 + 2 * k) & 0xFFFF, w)
    for seg, off, blob in o.get('writes', ()):
        for j, v in enumerate(blob):
            m[physical(seg, (off + j) & 0xFFFF)] = v
    c.cs, c.ip = BASE + RES, ENTRY
    info = dict(places=places, order=order, char=char, target=target, sp=sp, ss=ss, ds=ds, budget=o.get('budget', BUDGET),
                plain=not o.get('writes') and o.get('tail_link') is None and o.get('char') is None)
    return m, c, envspec, keys, info


def run_unicorn(name, m, c, envspec, keys, body_ips, budget):
    """M.run_unicorn's oracle with this body's segment admitted, a budget on this body's instructions, and FreeMem's RunError entry as a stop."""
    from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE, UC_HOOK_INTR, UC_HOOK_INSN
    from unicorn import x86_const as RR
    env = Q.Env(**envspec); script = list(keys); device = M.new_device()
    uc = Uc(UC_ARCH_X86, UC_MODE_16); backing = (ctypes.c_uint8 * SIZE).from_buffer_copy(m)

    def vread(em, off, size, user):
        return sum(device.read((off + j) & 65535) << (8 * j) for j in range(size))

    def vwrite(em, off, size, value, user):
        for j in range(size):
            device.write((off + j) & 65535, (value >> (8 * j)) & 255)
    for origin in (0, SIZE):
        uc.mem_map_ptr(origin, 0xA0000, 7, ctypes.addressof(backing)); uc.mmio_map(origin + 0xA0000, 65536, vread, None, vwrite, None)
        uc.mem_map_ptr(origin + 0xB0000, 0x50000, 7, ctypes.addressof(backing) + 0xB0000)
    for (n, _), reg in zip(Cpu._fields_, REGISTERS):
        uc.reg_write(reg, getattr(c, n))
    s = dict(stop=None, steps=0, left=budget, prev=None); seen = set(); closure = set(); ega_events = []; edges = set()
    known = {BASE + v for v in SEG.values()} | {M.STUB_SEG, BASE + RES}
    sysc = BASE + SEG['system']; here = BASE + RES

    def code(em, address, size, user):
        cs = uc.reg_read(RR.UC_X86_REG_CS); ip = uc.reg_read(RR.UC_X86_REG_IP)
        if (cs, ip) == (sysc, 0x00E9):
            s['stop'] = 'EXIT'; return uc.emu_stop()
        if (cs, ip) == (sysc, 0x00E2):
            s['stop'] = 'RUNERROR'; return uc.emu_stop()
        if (cs, ip) == (BASE + R.CRT, 0x014B):
            s['stop'] = 'BREAK'; return uc.emu_stop()
        if (cs, ip) == M.RETURN_TO:
            s['stop'] = 'RETURN'; return uc.emu_stop()
        if cs not in known:
            s['stop'] = 'TRANSFER'; return uc.emu_stop()
        closure.add((cs, ip))
        if cs == here and ENTRY <= ip < END:
            check(ip in body_ips, f'escape inside 1509 {name} {ip:04x}')
            if s['left'] == 0:
                s['stop'] = 'LIMIT'; return uc.emu_stop()
            s['left'] -= 1; seen.add(ip)
            if s['prev'] is not None:
                edges.add((s['prev'], ip))
            s['prev'] = ip
        s['steps'] += 1
        if s['steps'] > 3000000:
            s['stop'] = 'RUNAWAY'; uc.emu_stop()

    def intr(em, intno, user):
        ax = uc.reg_read(RR.UC_X86_REG_AX); ah, al = ax >> 8, ax & 0xFF
        if intno == 0x16:
            check(ah in (0x00, 0x01), f'ENV refusal {name}: INT 16h AH {ah:#x}')
            fl = uc.reg_read(RR.UC_X86_REG_EFLAGS)
            if ah == 0x01:
                env.log.append(('16', 1, script[0] if script else None))
                if script:
                    uc.reg_write(RR.UC_X86_REG_AX, script[0]); uc.reg_write(RR.UC_X86_REG_EFLAGS, fl & ~0x40)
                else:
                    uc.reg_write(RR.UC_X86_REG_EFLAGS, fl | 0x40)
                return
            check(bool(script), f'ENV refusal {name}: INT 16h AH 00h with the script empty')
            k = script.pop(0); env.log.append(('16', 0, k)); uc.reg_write(RR.UC_X86_REG_AX, k); return
        check(intno == 0x21, f'ENV refusal {name}: INT {intno:#x}')
        if ah == 0x2C:
            cx, dx = env.gettime(); uc.reg_write(RR.UC_X86_REG_CX, cx); uc.reg_write(RR.UC_X86_REG_DX, dx); return
        check(ah in (0x25, 0x35), f'ENV refusal {name}: INT 21h AH {ah:#x}')
        import struct
        sp = uc.reg_read(RR.UC_X86_REG_SP); ss_ = uc.reg_read(RR.UC_X86_REG_SS)
        fl = uc.reg_read(RR.UC_X86_REG_EFLAGS) & 0xFFFF
        for k, v in ((2, fl), (4, uc.reg_read(RR.UC_X86_REG_CS)), (6, uc.reg_read(RR.UC_X86_REG_IP))):
            uc.mem_write(physical(ss_, (sp - k) & 0xFFFF), struct.pack('<H', v))
        out = env.dos(ah, al, dict(ds=uc.reg_read(RR.UC_X86_REG_DS), dx=uc.reg_read(RR.UC_X86_REG_DX)))
        for k, v in out.items():
            uc.reg_write(dict(es=RR.UC_X86_REG_ES, bx=RR.UC_X86_REG_BX)[k], v)

    def port_in(em, port, size, user):
        check(port == 0x61 and size == 1, f'ENV refusal {name}: IN {port:#x}')
        env.log.append(('in', port, env.in61)); return env.in61

    def port_out(em, port, size, value, user):
        if port in M.EGA_PORTS:
            ega_events.append((port, value, size, uc.reg_read(RR.UC_X86_REG_CS), uc.reg_read(RR.UC_X86_REG_IP))); device.out(port, value, size); return
        check(port in (0x40, 0x43, 0x61, 0xC0) and size == 1, f'ENV refusal {name}: OUT {port:#x}')
        env.log.append(('out', port, value))
    uc.hook_add(UC_HOOK_CODE, code); uc.hook_add(UC_HOOK_INTR, intr)
    uc.hook_add(UC_HOOK_INSN, port_in, None, 1, 0, RR.UC_X86_INS_IN)
    uc.hook_add(UC_HOOK_INSN, port_out, None, 1, 0, RR.UC_X86_INS_OUT)
    uc.emu_start(physical(c.cs, c.ip), 2 * SIZE, count=6000000)
    check(s['stop'] in ('RETURN', 'EXIT', 'BREAK', 'TRANSFER', 'LIMIT', 'RUNERROR'), f'original stop {name}: {s["stop"]}')
    return uc, backing, env, s['stop'], seen, closure, device, ega_events, edges, s['left']


def run_c(f, lib, m, c, envspec, keys, budget):
    keep = []
    q, env = Q.c_env(envspec, keep)
    ke = R.KeyEnv(q, R.keyboard(env, keys, keep, None), None, 64)
    e = Env1509(M.Env0BF5(ke, 65536, 65536, 65536), 65536)
    actual = Cpu.from_buffer_copy(c); ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(m)
    ega = M.Ega(); lib.pr_ega_reset(ctypes.byref(ega))
    for p in range(4):
        ctypes.memset(ctypes.addressof(ega.plane[p]), 0x31 + p * 0x27, 65536)
    events, writes = [], []

    def out(ctx, port, value, size, cs, ip):
        events.append((port, value, size, cs, ip)); return 0

    def write(ctx, off, value):
        writes.append((off, value))
    ports = E0.Env(None, E0.OUT_FN(out), E0.IN_FN()); keep.extend([ports])
    bus = M.Bus(ram, ctypes.pointer(ega), ctypes.pointer(ports), G.WRITE_FN(write), None)
    d = Diag1509(); left = ctypes.c_size_t(budget)
    status = f(ctypes.byref(bus), SIZE, ctypes.byref(actual), ctypes.byref(left), ctypes.byref(e), ctypes.byref(d))
    name = STATUS.get(status)
    stop = {'RETURNED': 'RETURN', 'LIMIT': 'LIMIT', 'RUNERROR': 'RUNERROR', 'TRANSFER': 'TRANSFER'}.get(name, name)
    if name == 'KEY':
        stop = R.STATUS.get(d.d0bf5.key_status)
    if name == 'RETURNED' and (actual.cs, actual.ip) != M.RETURN_TO:
        stop = 'TRANSFER'
    return stop, status, actual, bytes(ram), env, d, events, writes, bytes(ega), left.value


def compare(name, state, body_ips, f, lib):
    m, c, envspec, keys, info = state
    uc, backing, uenv, ustop, seen, closure, device, uega, edges, uleft = run_unicorn(name, m, c, envspec, keys, body_ips, info['budget'])
    stop, status, actual, ram, cenv, d, cega, cwrites, cstate, cleft = run_c(f, lib, m, c, envspec, keys, info['budget'])
    check(stop == ustop, f'C stop {name}: {stop} (status {STATUS.get(status)}, site {d.site:04x}, callee {d.callee}/{d.callee_status}) original {ustop}')
    check(cleft == uleft, f'budget left {name}: C {cleft} original {uleft}')
    check(cenv.log == uenv.log, f'environment events {name}: C {cenv.log[:6]} original {uenv.log[:6]}')
    check(cega == uega, f'EGA OUT events {name}: C {len(cega)} original {len(uega)}')
    check(cwrites == device.writes, f'VRAM write operands {name}: {len(cwrites)}/{len(device.writes)}')
    check(cstate == device.packed(), f'EGA state {name}')
    if ram != bytes(backing):
        diff = [hex(i) for i, (a, b) in enumerate(zip(ram, bytes(backing))) if a != b]
        raise ValueError(f'RAM {name}: {diff[:8]} ({len(diff)} bytes)')
    for (n, _), reg in zip(Cpu._fields_, REGISTERS):
        want = uc.reg_read(reg) & 0xFFFF
        check(getattr(actual, n) == want, f'CPU {name}/{n} C {getattr(actual, n):04x} original {want:04x}')
    return ustop, seen, closure, edges, ram, device, uega


def statement(name, state, stop, ram):
    """Without the C model, on plain cases that returned: which message (if any) the body loaded, the list after the unlink, the record's link, and the
    freed block (FreeList or HeapPtr moved)."""
    m, c, envspec, keys, info = state
    if stop != 'RETURN' or not info['plain']:
        return 0
    word = lambda seg, off: ram[physical(seg, off & 0xFFFF)] | (ram[physical(seg, (off + 1) & 0xFFFF)] << 8)
    bp = (info['sp'] - 2) & 0xFFFF
    target, places, order, char = info['target'], info['places'], info['order'], info['char']
    head = word(char[0], char[1] + 0xC8) | (word(char[0], char[1] + 0xCA) << 16)
    if target == (0, 0):
        check(ram[physical(info['ss'], (bp - 0x18) & 0xFFFF)] == m[physical(BASE + RES, LITERALS[0])], f'{name}: null record, but the first message was not loaded')
        return 1
    k = places.index(target) if target in places else None
    if k is None or k not in order:
        check(ram[physical(info['ss'], (bp - 0x2B) & 0xFFFF)] == m[physical(BASE + RES, LITERALS[1])], f'{name}: absent record, but the second message was not loaded')
        return 1
    rest = [places[j] for j in order if j != k]
    want_head = (rest[0][0] << 16 | rest[0][1]) if rest else 0
    check(head == want_head, f'{name}: head {head:08x}, rule {want_head:08x}')
    for i, p in enumerate(rest):
        link = word(p[0], p[1] + 0x2A) | (word(p[0], p[1] + 0x2C) << 16)
        want = (rest[i + 1][0] << 16 | rest[i + 1][1]) if i + 1 < len(rest) else 0
        check(link == want, f'{name}: link of {p} {link:08x}, rule {want:08x}')
    check(word(target[0], target[1] + 0x2A) == 0 and word(target[0], target[1] + 0x2C) == 0, f'{name}: the removed record still links')
    return 1


def cases(quick=False):
    a = (K['a'],)
    out = []

    def add(name, **kw):
        kw.setdefault('keys', a); out.append((name, kw))
    add('null-record', target=None)
    add('null-record-mode3', target=None, mode=3)
    add('absent-empty-list', nodes=0, target_at=node_at(5))
    add('absent-from-list', nodes=3, target_at=node_at(6))
    for t in range(3):
        add(f'remove-{("head", "middle", "tail")[t]}', nodes=3, target=t)
    add('remove-only', nodes=1, target=0)
    add('remove-from-five-reordered', nodes=5, order=[3, 0, 4, 1, 2], target=1)
    add('runerror-offset-4', nodes=2, places=[(HEAP_ORG[0] + 1, 0x0004), node_at(1)], target=0)
    add('runerror-outside-heap', nodes=2, places=[(0x4D00, 0x0000), node_at(1)], target=0)
    add('freemem-top-block', nodes=3, places=[node_at(0), node_at(1), (HEAP_TOP[0] - 4, 0x0000)], target=2)
    S, S2 = node_at(0)[0], node_at(1)[0]                                                          # equal segments, offsets 0 and 8 (both valid FreeMem blocks)
    add('same-seg-head', places=[(S, 0), (S, 8)], target=1)
    add('same-seg-walk', places=[(S2, 0), (S, 0), (S, 8)], target=2)
    add('remove-linked-to-offset-8', places=[(S2, 0), (S, 8)], target=0)                    # the record's link offset word is 8: clearing it is visible
    add('same-seg-absent', places=[(S, 0), (S2, 0)], target_at=(S, 8))
    add('cycle-absent', nodes=2, tail_link=node_at(0), target_at=node_at(7), budget=500)
    add('cycle-found-late', nodes=3, tail_link=node_at(0), target=2)
    for keys_, label in (((K['x'],), 'key-x'), ((K['a'], K['x']), 'two-keys'), ((), 'buffered')):
        add(f'absent-{label}', nodes=2, target_at=node_at(6), keys=keys_, buffered=0x48 if label == 'buffered' else 0)
    for tail in (b' STING', b''):
        add(f'ctrl-c-tail{tail!r}', target=None, keys=(K['c'],), tail=tail, cur=2, saved=1)
    add('break', target=None, brk=1)
    for fl in M.FLAGS + M.DIRTY_FLAGS:
        add(f'flags{fl:04x}', flags=fl, nodes=3, target=1); add(f'flags{fl:04x}-null', flags=fl, target=None)
    for sp in (SP_MIN, SP_MIN + 1, 0xF001, SP_MAX - 1, SP_MAX):
        add(f'sp-{sp:04x}', sp=sp, nodes=2, target=1); add(f'sp-{sp:04x}-null', sp=sp, target=None)
    for k in (-2, 0, 2):                                                                        # the character's head (+0C8h) on the frame
        add(f'char-on-frame{k:+d}', nodes=2, target=1, char=(0x7000, (0xF000 + k - 0xC8) & 0xFFFF))
    for b in list(range(0, 40, 2 if quick else 1)) + [60, 80, 100]:
        add(f'budget-{b}', budget=b, nodes=3, target=2)
    for k in range(6 if quick else 24):
        rng = random.Random(f'RES_00BA_1509/random{k}')                                        # each world from its own name, never from run order
        n = rng.randrange(0, 5)
        order = list(range(n)); rng.shuffle(order)
        tgt = rng.choice([None] + list(range(n)) + ['absent'])
        kw = dict(nodes=n, order=order, mode=rng.choice((0, 1, 2, 3)), flags=rng.choice(M.FLAGS + M.DIRTY_FLAGS), cpu=tuple(rng.randrange(65536) for _ in range(7)))
        if tgt == 'absent':
            kw['target_at'] = node_at(9)
        else:
            kw['target'] = tgt
        add(f'random{k}', **kw)
    return out


def differential(image_reloc, body_ips, f, lib, quick=False, only=None):
    t0 = time.time(); seen, edges, stops, stated = set(), set(), {}, 0
    cs = [c for c in cases(quick) if only is None or c[0] in only]
    for index, (name, o) in enumerate(cs):
        if index % 10 == 0:
            print(f'[{index + 1}/{len(cs)}] {name}: START', file=sys.stderr, flush=True)
        state = build(image_reloc, o)
        stop, v, cl, e, ram, device, uega = compare(name, state, set(body_ips), f, lib)
        seen |= v; edges |= e; stops[stop] = stops.get(stop, 0) + 1
        stated += statement(name, state, stop, ram)
    conds = {a: (int(i.op_str, 16), a + i.size) for a, i in body_ips.items() if i.mnemonic.startswith('j') and i.mnemonic != 'jmp'}
    outcomes = {(a, t) for a, ts in conds.items() for t in ts}
    check(only is not None or seen == set(body_ips), f'starts not reached: {sorted(hex(a) for a in set(body_ips) - seen)}')
    check(only is not None or outcomes <= edges, f'outcomes not reached: {sorted(outcomes - edges)}')
    return dict(cases=len(cs), stops=dict(sorted(stops.items())), starts=f'{len(seen)}/{len(body_ips)}', outcomes=f'{len(outcomes & edges)}/{len(outcomes)}',
                independent_statements=stated, cpu_words=14, ram_bytes=SIZE, flag_masks=0, seconds=round(time.time() - t0, 1))


MUTANT_CASES = ('null-record', 'absent-from-list', 'remove-head', 'remove-middle', 'remove-tail', 'remove-from-five-reordered', 'same-seg-head', 'same-seg-walk', 'remove-linked-to-offset-8', 'runerror-offset-4',
                'cycle-absent', 'absent-key-x', 'ctrl-c-tailb\' STING\'', 'flags0a87', 'flags0a87-null', 'sp-0200', 'budget-4', 'budget-20')
MUTANTS = (
    ('the null test reads one word', [("STEP(); c->ax = (uint16_t)(c->ax | rd16(ram, c->ss, (uint16_t)(c->bp + (8)))); logic(c, c->ax, 16u); c->ip = 0x1515u;",
                                        "STEP(); logic(c, c->ax, 16u); c->ip = 0x1515u;")]),
    ('the first message is at 14CF', [("c->di = 0x14CEu;", "c->di = 0x14CFu;")]),
    ('the colour is 0Fh', [("STEP(); c->ax = (uint16_t)((c->ax & 0xFF00u) | ((uint16_t)(uint8_t)(0x0Eu) << 0u)); c->ip = 0x1519u;",
                            "STEP(); c->ax = (uint16_t)((c->ax & 0xFF00u) | ((uint16_t)(uint8_t)(0x0Fu) << 0u)); c->ip = 0x1519u;")]),
    ('the list head is +0C6h', [("STEP(); c->ax = brd16(bus, c->es, (uint16_t)(c->di + 0x00C8u)); c->ip = 0x1539u;", "STEP(); c->ax = brd16(bus, c->es, (uint16_t)(c->di + 0x00C6u)); c->ip = 0x1539u;")]),
    ('the link is +28h in the walk', [("STEP(); c->ax = brd16(bus, c->es, (uint16_t)(c->di + 0x002Au)); c->ip = 0x157Du;", "STEP(); c->ax = brd16(bus, c->es, (uint16_t)(c->di + 0x0028u)); c->ip = 0x157Du;")]),
    ('the record link is not cleared', [("STEP(); bwr16(bus, c->es, (uint16_t)(c->di + 0x002Au), c->ax); c->ip = 0x15DEu;", "STEP(); c->ip = 0x15DEu;")]),
    ('FreeMem size 40h', [("STEP(); c->ax = 0x003Fu; c->ip = 0x15EBu;", "STEP(); c->ax = 0x0040u; c->ip = 0x15EBu;")]),
    ('RETF 6', [("c->sp = (uint16_t)(c->sp + 8u);", "c->sp = (uint16_t)(c->sp + 6u);")]),
    ('the walk compare skips the high word', [("STEP(); if (!(c->flags & 0x0040u)) { c->ip = 0x158Bu; goto L_158B; } c->ip = 0x1586u;", "STEP(); if (0) { c->ip = 0x158Bu; goto L_158B; } c->ip = 0x1586u;")]),
    ('the prompt KEY stop read as CALLEE', [("if (s == PR_R0709_0BF5_KEY) { d->callee_status = (int)s; return PR_RES_1509_KEY; }", "(void)0;")]),
    ('FreeMem RunError read as CALLEE', [("if (s == PR_SYSTEM_FREEMEM_RUNERROR) { d->callee_status = (int)s; return PR_RES_1509_RUNERROR; }", "(void)0;")]),
    ('the walk is not charged to the budget', [("L_156E: STEP();", "L_156E: ;")]),
)


def mutation_check(image_reloc, body_ips):
    out = []; src = (ROOT / 'src/recovered/resident_00ba_sidecar_remove_1509_cpu.c').read_text()
    with tempfile.TemporaryDirectory(prefix='r1509-base-') as tmp:                              # the unmutated model must pass the same subset (PM i22687)
        bf, blib = library(tmp)
        baseline = differential(image_reloc, body_ips, bf, blib, quick=True, only=MUTANT_CASES)
    check(baseline['cases'] == len(MUTANT_CASES), f'baseline ran {baseline["cases"]} of {len(MUTANT_CASES)} mutation cases')
    for k, (name, edits) in enumerate(MUTANTS):
        t0 = time.time(); print(f'[mutant {k + 1}/{len(MUTANTS)}] START {name}', file=sys.stderr, flush=True)
        text = src
        for old, new in edits:
            check(text.count(old) == 1, f'mutant text occurs once: {name}: {old[:60]}'); text = text.replace(old, new, 1)
        with tempfile.TemporaryDirectory(prefix='r1509-mut-') as tmp:
            path = Path(tmp) / 'mutant.c'; path.write_text(text)
            try:
                mf, mlib = library(tmp, override={'resident_00ba_sidecar_remove_1509_cpu': path})
            except subprocess.CalledProcessError:
                out.append((name, 'COMPILE-ERROR')); continue
            try:
                differential(image_reloc, body_ips, mf, mlib, quick=True, only=MUTANT_CASES)
                out.append((name, 'SURVIVED the differential'))
            except (ValueError, AssertionError) as e:
                out.append((name, 'CAUGHT: ' + str(e)[:100]))
        print(f'[mutant {k + 1}/{len(MUTANTS)}] {out[-1][1][:80]} ({time.time() - t0:.0f}s)', file=sys.stderr, flush=True)
    d = sum(1 for _, r in out if r.startswith('CAUGHT:'))
    return dict(baseline=dict(cases=baseline['cases'], stops=baseline['stops'], mismatches=0), total=len(out), caught=d, results=out, survivors=[n for n, r in out if not r.startswith('CAUGHT:')], cases=list(MUTANT_CASES))


def main():
    p = argparse.ArgumentParser(description=__doc__)
    for flag in ('--compile-only', '--unicorn', '--quick', '--mutate', '--native'):
        p.add_argument(flag, action='store_true')
    p.add_argument('--output', type=Path); a = p.parse_args()
    with tempfile.TemporaryDirectory(prefix='r1509-') as tmp:
        f, lib = library(Path(tmp))
        if a.compile_only:
            print('CHECKER_BUILD_OK'); return
        image, sites = Q.load_image()
        body, body_ips, meta = source(image, sites)
        image_reloc = Q.relocate(image, sites, BASE)
        report = dict(target=meta)
        if a.unicorn:
            report['unicorn'] = differential(image_reloc, body_ips, f, lib, a.quick)
        if a.mutate:
            report['mutation'] = mutation_check(image_reloc, body_ips)
        if a.native:
            from res00ba_sidecar_remove_1509_native import native
            report['native'] = native(image, sites, body_ips, f, lib)
    if a.output:
        a.output.parent.mkdir(parents=True, exist_ok=True); a.output.write_text(json.dumps(report, sort_keys=True, indent=2) + '\n')
    print(json.dumps(report, sort_keys=True))


if __name__ == '__main__':
    sys.modules['res00ba_sidecar_remove_1509_cpu_audit'] = sys.modules[__name__]
    main()
