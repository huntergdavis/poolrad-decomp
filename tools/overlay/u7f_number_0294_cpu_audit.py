#!/usr/bin/env python3
"""OVL_007F_0294 (the game's numeric entry line editor) against the original bytes, as a composed closure: the body (GAME.OVR
unit 7Fh, its 17 far-call segment fixups applied), SYSTEM StrStore/Str/string load/concat/char-to-string/Copy/Val, the 0709
printer (0626) and space (05EB) routines drawing through an EGA, and RES_0899_0059 with everything under it.

The whole loader-expanded START.EXE image is loaded at a test base with the loader's own relocations, and the overlay body
is placed at a test segment with its unit fixups relocated the same way; no byte is patched. EGA: an independent Python
device (res00ba_display_1693_cpu_audit.PythonEga) mapped as MMIO at A0000h in Unicorn, the merged C pr_ega on the C side;
planes, latches and registers are compared in full, with the VRAM write operands and every EGA OUT. The declared
environment is RES_0899_0059's (INT 16h, INT 21h 25h/35h/2Ch, the 08AE ports, the INT 10h stub) with the keys delivered in
typing groups: KeyPressed reports a key only within the current group, so each RES_0899_0059 call returns the last key of
one group, as a person typing between calls produces. Media and generated programs stay under ignored resources/."""
import argparse, ctypes, json, random, struct, subprocess, sys, tempfile
from hashlib import sha256
from pathlib import Path
from capstone import Cs, CS_ARCH_X86, CS_MODE_16
from res0709_ega_reset_0000_cpu_audit import Cpu, ROOT, SIZE, REGISTERS, physical, put16, check
from overlay_inventory import find_units, parse_mz
import res0899_quit_0000_cpu_audit as Q
import res0899_readkey_0059_cpu_audit as K
import res0709_printer_0626_cpu_audit as P
from res00ba_display_1693_cpu_audit import PythonEga
CHECKER_ROWS = ('OVL_007F_0294',)
ENTRY, END = 0x0294, 0x0441
DIGEST = '66d7b9df95a5aad97b6d084354e56f72b70122a796018be24e48ab21605aafe8'
BASE, SEG, SS, STUB_SEG, PSP = Q.BASE, Q.SEG, Q.SS, Q.STUB_SEG, K.PSP
OVL, FONT, PROMPT = 0x8000, 0x9000, 0x7800
SOURCES = list(dict.fromkeys(K.SOURCES + ['overlay_u7f_number_0294_cpu', 'system_val_13b1_cpu', 'system_str_1366_cpu',
                                          'system_string_leaves_cpu', 'system_strstore_fullstate_0b25', 'resident_0709_space_05eb_cpu'] + list(P.SOURCES)))
FLAGS = Q.FLAGS
RETURN_TO = (0x9001, 0x7000)
EGA_PORTS = (0x3C4, 0x3C5, 0x3CE, 0x3CF)
KNOWN = {BASE + v for v in K.SEGS.values()} | {BASE + 0x0709, STUB_SEG, OVL}   # 0709: the printer/space/glyph routines
# Conditional sites in the body: (site) -> successors (taken, fall-through). JG +0363 taken is argued infeasible (doc).
CONDITIONALS = {0x0308: (0x030D, 0x030A), 0x030F: (0x0314, 0x0311), 0x0363: (0x036C, 0x0365), 0x0365: (0x0371, 0x0367),
                0x036A: (0x0371, 0x036C), 0x03AD: (0x03F3, 0x03AF), 0x03B3: (0x03F3, 0x03B5), 0x03F7: (0x0402, 0x03F9),
                0x03FD: (0x0402, 0x03FF), 0x0417: (0x0420, 0x0419)}
INFEASIBLE = {(0x0363, 0x036C)}


class Env(ctypes.Structure):
    _fields_ = [('key', K.KeyEnv), ('space_loopbacks', ctypes.c_size_t), ('printer_loopbacks', ctypes.c_size_t),
                ('glyph_loopbacks', ctypes.c_size_t), ('key_limit', ctypes.c_uint)]


class Diag(ctypes.Structure):
    _fields_ = [('site', ctypes.c_uint16), ('status', ctypes.c_int), ('key', K.KeyDiag), ('printer', ctypes.c_int * 4),
                ('space', ctypes.c_int * 4), ('keys', ctypes.c_uint)]


STATUS = {0: 'RETURN', 1: 'ARGUMENT', 2: 'EXIT', 3: 'BREAK', 4: 'CALLEE', 5: 'LIMIT', 6: 'TRANSFER'}


def source():
    exe = (ROOT / 'resources/DOS/POOLRAD/START.EXE').read_bytes(); ovr = (ROOT / 'resources/DOS/POOLRAD/GAME.OVR').read_bytes()
    unit = next(u for u in find_units(exe[parse_mz(exe)['header_bytes']:], ovr) if u['stub_para'] == 0x7F)
    body = ovr[unit['ovr_offset'] + ENTRY:unit['ovr_offset'] + END]
    check(sha256(body).hexdigest() == DIGEST, 'U7F +0294 body identity')
    fixups = [f for f in unit['fixups'] if ENTRY <= f < END]
    ins = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(body, ENTRY))
    check(len(ins) == 188 and sum(i.size for i in ins) == len(body), 'decode covers the span')
    calls = [i for i in ins if i.mnemonic == 'lcall']
    check(len(calls) == 17 and [i.address + 3 for i in calls] == fixups, 'each far call carries one unit fixup (its segment word)')
    check([i.address for i in ins if i.mnemonic in ('ret', 'retf')] == [0x043E], 'single RETF 8')
    check({i.address for i in ins if i.mnemonic.startswith('j') and i.mnemonic != 'jmp'} == set(CONDITIONALS), 'conditional sites')
    tail = ovr[unit['ovr_offset'] + END:unit['ovr_offset'] + ENTRY + 440]
    check(tail == b'\nOverloaded', 'the queue span adds the next string, not code')
    return body, fixups, {i.address: i for i in ins}


def library(directory):
    path = directory / 'number.so'
    subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Wpedantic', '-Werror', '-O2', '-fPIC', '-shared', '-I', str(ROOT / 'include')] +
                   [str(ROOT / f'src/recovered/{s}.c') for s in SOURCES] + ['-o', str(path)], check=True)
    lib = ctypes.CDLL(str(path)); f = lib.pr_u7f_number_0294_cpu
    f.argtypes = [ctypes.POINTER(P.Bus), ctypes.c_size_t, ctypes.POINTER(Cpu), ctypes.POINTER(Env), ctypes.POINTER(Diag)]
    f.restype = ctypes.c_int
    lib.pr_ega_reset.argtypes = [ctypes.POINTER(P.Ega)]
    return lib


def build(image_reloc, body, fixups, *, groups=((0x1C0D,),), maxv=100, colour=0x0E, prompt=b'How much? ', flags=2, sp=0xF000,
          tail=b' foo', cpu=None, font_pattern=0):
    m = bytearray([0x5A]) * SIZE
    b = BASE * 16; m[b:b + len(image_reloc)] = image_reloc
    at = physical(OVL, ENTRY); m[at:at + len(body)] = body
    for f in fixups:
        put16(m, OVL, f, (struct.unpack_from('<H', m, physical(OVL, f))[0] + BASE) & 0xFFFF)
    ds = BASE + SEG['data']
    m[physical(ds, 0x5372)] = 1; m[physical(ds, 0x5373)] = 0; put16(m, ds, 0x5374, 1); put16(m, ds, 0x2612, 0)
    m[physical(ds, 0x4A1C)] = 3; m[physical(ds, 0x49F7)] = 0; m[physical(ds, 0x8501)] = 0; m[physical(ds, 0x8502)] = 0
    put16(m, ds, 0x4422, PSP); m[physical(PSP, 0x80)] = len(tail)
    for k, ch in enumerate(tail + b'\r'):
        m[physical(PSP, 0x81 + k)] = ch
    # The glyph path's display state (BSS): mode 1, pages A000h/A200h, the font pointer; its tables are the image's own data.
    m[physical(ds, 0x4A1E)] = 1; put16(m, ds, 0x4A16, 0xA000); put16(m, ds, 0x4A18, 0xA200)
    put16(m, ds, 0x5E20, 0x0000); put16(m, ds, 0x5E22, FONT)
    for j in range(2048):
        m[physical(FONT, j)] = ((j * 37 + (j >> 3) * 13) ^ 0xA6) & 255 if not font_pattern else 0
    s = physical(STUB_SEG, 0); m[s:s + len(Q.INT10_STUB)] = Q.INT10_STUB
    put16(m, 0, 0x40, 0x0000); put16(m, 0, 0x42, STUB_SEG); m[physical(0x0040, 0x0049)] = 0x5A
    m[physical(PROMPT, 0x100)] = len(prompt)
    for k, ch in enumerate(prompt):
        m[physical(PROMPT, 0x101 + k)] = ch
    for j, w in enumerate((RETURN_TO[1], RETURN_TO[0], colour, 0x0100, PROMPT, maxv)):
        put16(m, SS, (sp + 2 * j) & 0xFFFF, w)
    r = cpu or (0x1234, 0x3456, 0x5678, 0x789A, 0x9ABC, 0xBCDE, 0x7777)
    c = Cpu(*r, sp, OVL, ENTRY, ds, 0x1111, SS, flags)
    env = dict(table={8: (0x0123, 0xF000)}, in61=0xB0, time_start=4736147, time_step=7)
    return m, c, env, [list(g) for g in groups]


class Keys:
    """Key groups: AH=00h takes the next key of the current group, moving to the next group when it is empty; AH=01h reports
    a key only while the current group has one."""
    def __init__(self, groups, log):
        self.groups = [list(g) for g in groups]; self.log = log

    def pending(self):
        return self.groups[0][0] if self.groups and self.groups[0] else None

    def take(self):
        while self.groups and not self.groups[0]:
            self.groups.pop(0)
        if not self.groups:
            return None
        return self.groups[0].pop(0)


def run_unicorn(name, m, c, envspec, groups, body_ips):
    from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE, UC_HOOK_INTR, UC_HOOK_INSN
    from unicorn import x86_const as R
    env = Q.Env(**envspec); keys = Keys(groups, env.log); device = PythonEga(); ega_events = []
    uc = Uc(UC_ARCH_X86, UC_MODE_16); backing = (ctypes.c_uint8 * SIZE).from_buffer_copy(m)

    def vread(em, off, size, user):
        return sum(device.read((off + j) & 65535) << (8 * j) for j in range(size))

    def vwrite(em, off, size, value, user):
        for j in range(size):
            device.write((off + j) & 65535, (value >> (8 * j)) & 255)
    for base in (0, SIZE):
        uc.mem_map_ptr(base, 0xA0000, 7, ctypes.addressof(backing)); uc.mmio_map(base + 0xA0000, 65536, vread, None, vwrite, None)
        uc.mem_map_ptr(base + 0xB0000, 0x50000, 7, ctypes.addressof(backing) + 0xB0000)
    for (n, _), reg in zip(Cpu._fields_, REGISTERS):
        uc.reg_write(reg, getattr(c, n))
    s = dict(stop=None, steps=0, prev=None); seen = set(); edges = set(); closure = set()

    def code(em, address, size, user):
        cs = uc.reg_read(R.UC_X86_REG_CS); ip = uc.reg_read(R.UC_X86_REG_IP)
        if s['prev'] in CONDITIONALS and cs == OVL:
            edges.add((s['prev'], ip))
        s['prev'] = None
        if (cs, ip) == (BASE + SEG['system'], 0x00E9):
            s['stop'] = 'EXIT'; return uc.emu_stop()
        if (cs, ip) == (BASE + K.CRT, 0x014B):
            s['stop'] = 'BREAK'; return uc.emu_stop()
        if (cs, ip) == RETURN_TO:
            s['stop'] = 'RETURN'; return uc.emu_stop()
        if cs not in KNOWN:
            s['stop'] = 'TRANSFER'; return uc.emu_stop()
        closure.add((cs, ip))
        if cs == OVL:
            check(ip in body_ips, f'escape inside 0294 {name} {ip:04x}'); seen.add(ip); s['prev'] = ip
        s['steps'] += 1
        if s['steps'] > 3000000:
            s['stop'] = 'RUNAWAY'; uc.emu_stop()

    def intr(em, intno, user):
        ax = uc.reg_read(R.UC_X86_REG_AX); ah, al = ax >> 8, ax & 0xFF
        if intno == 0x16:
            check(ah in (0x00, 0x01), f'ENV refusal {name}: INT 16h AH {ah:#x}')
            fl = uc.reg_read(R.UC_X86_REG_EFLAGS)
            if ah == 0x01:
                k = keys.pending(); env.log.append(('16', 1, k))
                if k is not None:
                    uc.reg_write(R.UC_X86_REG_AX, k); uc.reg_write(R.UC_X86_REG_EFLAGS, fl & ~0x40)
                else:
                    uc.reg_write(R.UC_X86_REG_EFLAGS, fl | 0x40)
                return
            k = keys.take(); check(k is not None, f'ENV refusal {name}: no key left'); env.log.append(('16', 0, k))
            uc.reg_write(R.UC_X86_REG_AX, k); return
        check(intno == 0x21, f'ENV refusal {name}: INT {intno:#x}')
        if ah == 0x2C:
            cx, dx = env.gettime(); uc.reg_write(R.UC_X86_REG_CX, cx); uc.reg_write(R.UC_X86_REG_DX, dx); return
        check(ah in (0x25, 0x35), f'ENV refusal {name}: INT 21h AH {ah:#x}')
        sp = uc.reg_read(R.UC_X86_REG_SP); ss_ = uc.reg_read(R.UC_X86_REG_SS); fl = uc.reg_read(R.UC_X86_REG_EFLAGS) & 0xFFFF
        for k, v in ((2, fl), (4, uc.reg_read(R.UC_X86_REG_CS)), (6, uc.reg_read(R.UC_X86_REG_IP))):
            uc.mem_write(physical(ss_, (sp - k) & 0xFFFF), struct.pack('<H', v))
        out = env.dos(ah, al, dict(ds=uc.reg_read(R.UC_X86_REG_DS), dx=uc.reg_read(R.UC_X86_REG_DX)))
        for k, v in out.items():
            uc.reg_write(dict(es=R.UC_X86_REG_ES, bx=R.UC_X86_REG_BX)[k], v)

    def port_in(em, port, size, user):
        check(port == 0x61 and size == 1, f'ENV refusal {name}: IN {port:#x}')
        env.log.append(('in', port, env.in61)); return env.in61

    def port_out(em, port, size, value, user):
        if port in EGA_PORTS or (size == 2 and port in (0x3C4, 0x3CE)):
            ega_events.append((port, value, size, uc.reg_read(R.UC_X86_REG_CS), uc.reg_read(R.UC_X86_REG_IP))); device.out(port, value, size)
            return
        check(port in (0x40, 0x43, 0x61, 0xC0) and size == 1, f'ENV refusal {name}: OUT {port:#x}')
        env.log.append(('out', port, value))
    uc.hook_add(UC_HOOK_CODE, code); uc.hook_add(UC_HOOK_INTR, intr)
    uc.hook_add(UC_HOOK_INSN, port_in, None, 1, 0, R.UC_X86_INS_IN)
    uc.hook_add(UC_HOOK_INSN, port_out, None, 1, 0, R.UC_X86_INS_OUT)
    uc.emu_start(physical(c.cs, c.ip), 2 * SIZE, count=4000000)
    check(s['stop'] in ('RETURN', 'EXIT', 'BREAK', 'TRANSFER'), f'original stop {name}: {s["stop"]}')
    return uc, backing, env, s['stop'], seen, edges, closure, device, ega_events


def run_c(lib, m, c, envspec, groups, key_limit=64):
    keep = []
    q, env = Q.c_env(envspec, keep)
    keys = Keys(groups, env.log)

    def kb(ctx, before, after):
        bb = before[0]; ah = bb.ax >> 8; a = Cpu.from_buffer_copy(bb); a.ip = (bb.ip + 2) & 0xFFFF
        if ah == 0x01:
            k = keys.pending(); env.log.append(('16', 1, k))
            if k is not None:
                a.ax = k; a.flags = bb.flags & ~0x40
            else:
                a.flags = bb.flags | 0x40
            after[0] = a; return 0
        if ah == 0x00:
            k = keys.take()
            if k is None:
                return 1
            env.log.append(('16', 0, k)); a.ax = k; after[0] = a; return 0
        return 1
    kfn = K.KB_FN(kb); keep.append(kfn)
    e = Env(K.KeyEnv(q, kfn, None, 64), 65536, 65536, 65536, key_limit)
    ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(m); ega = P.Ega(); lib.pr_ega_reset(ctypes.byref(ega))
    for p in range(4):
        ctypes.memset(ctypes.addressof(ega.plane[p]), 0x31 + p * 0x27, 65536)
    events, writes = [], []

    def out(ctx, port, value, size, cs, ip):
        events.append((port, value, size, cs, ip)); return 0

    def write(ctx, off, value):
        writes.append((off, value))
    penv = P.Env(None, P.OUT_FN(out), P.IN_FN()); wfn = P.WRITE_FN(write); keep += [penv, wfn]
    bus = P.Bus(ram, ctypes.pointer(ega), ctypes.pointer(penv), wfn, None)
    actual = Cpu.from_buffer_copy(c); d = Diag()
    status = STATUS.get(lib.pr_u7f_number_0294_cpu(ctypes.byref(bus), SIZE, ctypes.byref(actual), ctypes.byref(e), ctypes.byref(d)))
    return status, actual, bytes(ram), env, d, events, writes, bytes(ega)


def expected(groups, maxv):
    """Independent statement of the editor's semantics (not either model): each group yields its last key."""
    def val(t):
        t2 = t.lstrip(' ')
        if not t2 or not t2.isdigit():
            return 0
        v = int(t2)
        return v if v < 2 ** 31 else 0
    maxtext = str(maxv).rjust(5)
    text = ''
    for g in groups:
        k = g[-1] & 0xFF
        if 0x30 <= k <= 0x39:
            t = (text + chr(k))[:6]
            text = maxtext if val(t) > maxv else t
        elif k == 8 and text:
            text = text[:-1]
        if k in (0x0D, 0x1B):
            return 0 if k == 0x1B else val(text) & 0xFFFF
    return None


def compare(name, state, body_ips, lib):
    m, c, envspec, groups = state
    uc, backing, uenv, ustop, seen, edges, closure, device, ega_events = run_unicorn(name, m, c, envspec, groups, body_ips)
    status, actual, ram, cenv, d, events, writes, ega = run_c(lib, m, c, envspec, groups)
    check(status == ustop, f'C status {name}: {status} original {ustop} (site {d.site:04x}/{d.status})')
    check(cenv.log == uenv.log, f'environment events {name}')
    check(events == ega_events, f'EGA OUTs {name}: C {len(events)} original {len(ega_events)}')
    check(writes == device.writes, f'VRAM write operands {name}')
    check(ega == device.packed(), f'EGA planes/latches/registers {name}')
    if ram != bytes(backing):
        diff = [hex(i) for i, (a, bb) in enumerate(zip(ram, bytes(backing))) if a != bb]
        raise ValueError(f'RAM {name}: {diff[:8]} ({len(diff)} bytes)')
    for (n, _), reg in zip(Cpu._fields_, REGISTERS):
        want = uc.reg_read(reg) & 0xFFFF
        check(getattr(actual, n) == want, f'CPU {name}/{n} C {getattr(actual, n):04x} original {want:04x}')
    return ustop, seen, edges, closure, actual, len(writes)


def g(*keys):
    return tuple((k,) if isinstance(k, int) else tuple(k) for k in keys)


ENTER, ESC, BS = 0x1C0D, 0x011B, 0x0E08
D = {str(i): 0x0230 + i for i in range(10)}


def cases():
    rng = random.Random(0x007F0294)
    out = [('enter-empty', dict(groups=g(ENTER))), ('esc-empty', dict(groups=g(ESC))),
           ('42', dict(groups=g(D['4'], D['2'], ENTER))), ('42-esc', dict(groups=g(D['4'], D['2'], ESC))),
           ('bs', dict(groups=g(D['4'], D['2'], BS, D['7'], ENTER))), ('bs-empty', dict(groups=g(BS, D['5'], ENTER))),
           ('clamp', dict(groups=g(D['9'], D['9'], D['9'], ENTER), maxv=100)), ('exact-max', dict(groups=g(D['1'], D['0'], D['0'], ENTER), maxv=100)),
           ('clamp-then-digit', dict(groups=g(D['5'], D['0'], D['0'], D['1'], ENTER), maxv=300)), ('max0', dict(groups=g(D['3'], ENTER), maxv=0)),
           ('max65535', dict(groups=g(*[D[x] for x in '65536'], ENTER), maxv=65535)), ('six-digits', dict(groups=g(*[D[x] for x in '1234567'], ENTER), maxv=65535)),
           ('leading-zeros', dict(groups=g(D['0'], D['0'], D['7'], ENTER))), ('letters-ignored', dict(groups=g(0x1E61, D['3'], 0x2D78, ENTER))),
           ('drain-group', dict(groups=(((D['1'], D['2'], D['3']),) + g(ENTER)))),       # one call drains three keys: only '3' arrives
           ('ctrl-s', dict(groups=g(0x1F13, D['8'], ENTER))), ('enter-in-drain', dict(groups=((D['5'], ENTER),))),
           ('empty-prompt', dict(prompt=b'', groups=g(D['9'], ENTER))), ('long-prompt', dict(prompt=b'X' * 45, groups=g(D['1'], ENTER))),
           ('sting-quit', dict(tail=b' STING', groups=g(D['1'], 0x2E03))), ('sting-ctrl-d', dict(tail=b' STING', groups=g(0x2004, D['2'], ENTER)))]
    out += [(f'flags{fl:x}', dict(flags=fl, groups=g(D['6'], BS, D['1'], ENTER))) for fl in FLAGS]
    out += [('sp-low', dict(sp=0x0400, groups=g(D['2'], ENTER))), ('sp-odd', dict(sp=0xF001, groups=g(D['2'], ENTER)))]
    for k in range(20):
        pool = [D[x] for x in '0123456789'] + [BS, 0x1E61]
        keys = [rng.choice(pool) for _ in range(rng.randrange(0, 9))] + [rng.choice((ENTER, ESC))]
        out.append((f'random{k}', dict(groups=g(*keys), maxv=rng.choice((0, 9, 100, 999, 12345, 65535, rng.randrange(65536))),
                                       colour=rng.randrange(16), flags=rng.choice(FLAGS), cpu=tuple(rng.randrange(65536) for _ in range(7)))))
    return out


def differential(image_reloc, body, fixups, decoded, lib):
    seen, edges, closure, stops = set(), set(), set(), {}; specs = cases(); checked = writes = 0
    for name, o in specs:
        stop, v, e, cl, final, w = compare(name, build(image_reloc, body, fixups, **o), set(decoded), lib)
        seen |= v; edges |= e; closure |= cl; stops[stop] = stops.get(stop, 0) + 1; writes += w
        if stop == 'RETURN':
            want = expected(o['groups'], o.get('maxv', 100))
            check(want is not None and final.ax == want, f'{name}: independent result {want}, got {final.ax}')
            checked += 1
    check(seen == set(decoded), f'body starts missing {sorted(hex(a) for a in set(decoded) - seen)}')
    want = {(a, t) for a, ts in CONDITIONALS.items() for t in ts} - INFEASIBLE
    check(edges == want, f'outcomes missing {sorted((hex(a), hex(t)) for a, t in want - edges)} extra {sorted(edges - want)}')
    per = {}
    for label, seg, start, end in K.MEMBERS:
        base = K.SEGS[seg] * 16
        dec = {i.address for i in Cs(CS_ARCH_X86, CS_MODE_16).disasm(image_reloc[base + start:base + end], start)}
        ran = {ip for cs, ip in closure if cs == BASE + K.SEGS[seg] and start <= ip < end}
        per[label] = f'{len(ran)}/{len(dec)}'
    return dict(cases=len(specs), stops=stops, body_starts=len(seen), body_starts_total=len(decoded),
                conditional_outcomes=len(edges), conditional_outcomes_total=len(want), infeasible=['+0363 JG taken'],
                independent_result_checks=checked, vram_writes=writes, closure_member_starts=per, cpu_words=14, ram_bytes=SIZE,
                ega='planes, latches and registers compared in full', flag_masks=0)


def main():
    p = argparse.ArgumentParser()
    for flag in ('--compile-only', '--unicorn', '--native'):
        p.add_argument(flag, action='store_true')
    p.add_argument('--output', type=Path); a = p.parse_args()
    with tempfile.TemporaryDirectory(prefix='u7f0294-') as tmp:
        lib = library(Path(tmp))
        if a.compile_only:
            print('CHECKER_BUILD_OK'); return
        image, sites = Q.load_image()
        body, fixups, decoded = source()
        image_reloc = Q.relocate(image, sites, BASE)
        report = dict(target=dict(unit='0x7F', span='+[0294,0441)', bytes=len(body), instructions=len(decoded), sha256=DIGEST,
                                  fixups=[f'{f:04X}' for f in fixups], queue_extent_note='440 = 429 code + 11 bytes of the next string'))
        if a.unicorn:
            report['unicorn'] = differential(image_reloc, body, fixups, decoded, lib)
        if a.native:
            import u7f_number_0294_native
            report['native'] = u7f_number_0294_native.native(image, sites, body, fixups, lib, sys.modules[__name__])
    if a.output:
        a.output.write_text(json.dumps(report, sort_keys=True, indent=2) + '\n')
    print(json.dumps(report, sort_keys=True))


if __name__ == '__main__':
    main()
