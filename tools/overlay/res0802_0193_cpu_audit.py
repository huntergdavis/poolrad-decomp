#!/usr/bin/env python3
"""RES_0802_0193 (the status-line prompt, the last leaf of the t30 display stack) against the original bytes, as a composed closure: the body, SYSTEM StrStore
0AF8:0B25, the block filler 0175, the string writer 0105, the character writer 0040, Dos.Intr, and ReadKey-with-hotkeys 0899:0059 with ITS closure (Crt ReadKey and
KeyPressed with INT 16h the declared service, the 08AE driver members, SYSTEM leaves, QUIT).

The whole loader-expanded START.EXE image is loaded at a test base with the loader's own relocation table applied; every original byte of the closure runs. Declared
environment, identical in every oracle and in the C service callbacks: ReadKey's (INT 21h 25h/35h/2Ch, ports, INT 16h from a per-case key script) and, for INT 10h, the
real-byte LOGGING stub of the 0040 audit (so QUIT's INT 10h call is logged like the display calls). Unicorn: CPU14 + full 1 MiB + stop + the ordered environment event log,
unmasked. What this pins: the C model equals the original closure; the INT 10h register blocks equal an independent Python contract (blank, message, blank); the key and the
hotkey state equal the ReadKey audit's independent statement. What it cannot pin: what the BIOS does with a block: that is run B of res0802_0193_native.py. Media stays under
ignored resources/."""
import argparse, ctypes, json, os, random, struct, subprocess, sys, tempfile
from hashlib import sha256
from pathlib import Path
from capstone import Cs, CS_ARCH_X86, CS_MODE_16
import res0899_readkey_0059_cpu_audit as R
import res0899_quit_0000_cpu_audit as Q
import res0802_0040_cpu_audit as B
from res0709_ega_reset_0000_cpu_audit import Cpu, ROOT, SIZE, REGISTERS, physical, put16, check
CHECKER_ROWS = ('RES_0802_0193',)
ENTRY, END = 0x0193, 0x01ED
DIGEST = '03865df7d3a85b7cb3f9d71f8c0a7fb6a82fb8db3a0beea9568dd3cf92de11fc'
BASE, SS, STUB_SEG, LOG_SEG = R.BASE, R.SS, R.STUB_SEG, B.LOG_SEG
SEG = dict(R.SEGS, text=0x0802)
FLAGS = R.FLAGS
SCRATCH = 0x4100
MSG_SEG = 0x3000
DIRTY_FLAGS = (0x0202, 0x0602, 0x0A87, 0x0402, 0x0E57)       # the standing dirty-incoming-flags set (see the 0105/0175 audits)
RETURN_TO = R.RETURN_TO
STATUS = {0: 'RETURNED', 1: 'ARGUMENT', 2: 'DISPLAY', 3: 'KEY', 4: 'CALLEE'}
MEMBERS = [('0802:0040 writer', 'text', 0x0040, 0x0105), ('0802:0105 string', 'text', 0x0105, 0x0175), ('0802:0175 filler', 'text', 0x0175, 0x0193),
           ('0A32:000B Intr', 'dos', 0x000B, 0x006C), ('0AF8:0B25 StrStore', 'system', 0x0B25, 0x0B49)]


class Diag105(ctypes.Structure):
    _fields_ = [('chars', ctypes.c_uint), ('callee_status', ctypes.c_int), ('intr_status', ctypes.c_int), ('intr_calls', ctypes.c_uint)]


class Diag175(ctypes.Structure):
    _fields_ = [('callee_status', ctypes.c_int), ('intr_status', ctypes.c_int), ('intr_calls', ctypes.c_uint)]


class Env193(ctypes.Structure):
    _fields_ = [('display', B.Env), ('key', R.KeyEnv)]


class Diag193(ctypes.Structure):
    _fields_ = [('site', ctypes.c_uint16), ('callee_status', ctypes.c_int), ('string', Diag105), ('fill', Diag175), ('key_status', ctypes.c_int), ('key', R.KeyDiag)]


def source(image, sites):
    b = SEG['text'] * 16
    body = image[b + ENTRY:b + END]
    check(sha256(body).hexdigest() == DIGEST, '0802:0193 body identity')
    ins = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(body, ENTRY))
    check(len(ins) == 48 and sum(i.size for i in ins) == len(body), 'decode covers the span')
    check([(i.address, i.mnemonic, i.op_str) for i in ins if 'call' in i.mnemonic] ==
          [(0x1A7, 'lcall', '0xaf8, 0xb25'), (0x1B9, 'call', '0x175'), (0x1CC, 'call', '0x105'), (0x1CF, 'lcall', '0x899, 0x59'), (0x1E4, 'call', '0x175')], 'five callees')
    check([i.address for i in ins if i.mnemonic in ('ret', 'retf')] == [0x1EA], 'single RETF 6 at 01EA')
    check(not [i for i in ins if i.mnemonic in ('int', 'in', 'out', 'ljmp') or i.mnemonic.startswith('j')], 'straight line: no jump, interrupt or port instruction')
    check([a - b for a in sorted(s for s in sites if b + ENTRY <= s < b + END)] == [0x1AA, 0x1D2], 'the two far-call segment words are the only relocations')
    return body, {i.address: i for i in ins}


def ownership(image):
    from overlay_inventory import find_units, parse_mz
    from u86_procvars_3413_audit import EXE_SHA, OVR_SHA
    exe = (ROOT / 'resources/DOS/POOLRAD/START.EXE').read_bytes(); ovr = (ROOT / 'resources/DOS/POOLRAD/GAME.OVR').read_bytes()
    check(sha256(exe).hexdigest() == EXE_SHA and sha256(ovr).hexdigest() == OVR_SHA, 'media provenance')
    seg = image[SEG['text'] * 16:SEG['text'] * 16 + 0x10000]
    near = [o for o in range(len(seg) - 2) if seg[o] in (0xE8, 0xE9) and (o + 3 + struct.unpack_from('<h', seg, o + 1)[0]) & 0xFFFF == ENTRY]
    check(near == [0x2AA, 0x84A] and all(seg[o] == 0xE8 for o in near), f'near transfers into 0193: {[hex(o) for o in near]}')
    pattern = struct.pack('<HH', ENTRY, SEG['text'])
    check(not [i for i in range(1, len(image) - 3) if image[i:i + 4] == pattern and image[i - 1] in (0x9A, 0xEA)], 'a far transfer into 0802:0193 appeared')
    direct = [1 for u in find_units(exe[parse_mz(exe)['header_bytes']:], ovr) for f in u['fixups']
              if ovr[u['ovr_offset'] + f - 3:u['ovr_offset'] + f - 2] == b'\x9a' and struct.unpack_from('<HH', ovr, u['ovr_offset'] + f - 2) == (ENTRY, SEG['text'])]
    check(not direct, 'an overlay far call into 0802:0193 appeared')
    return dict(near_call_sites=['0802:02AA (025D)', '0802:084A (0716)'], far_entries=0, overlay_callers=0)


def library(directory):
    path = directory / 'r0802_0193.so'
    sources = list(R.SOURCES) + ['resident_0802_0193_cpu', 'resident_0802_0175_cpu', 'resident_0802_0105_cpu', 'resident_0802_0040_cpu', 'system_strstore_fullstate_0b25']
    subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Wpedantic', '-Werror', '-O2', '-fPIC', '-shared', '-I', str(ROOT / 'include')] +
                   [str(ROOT / f'src/recovered/{s}.c') for s in dict.fromkeys(sources)] + ['-o', str(path)], check=True)
    f = ctypes.CDLL(str(path)).pr_r0802_0193
    f.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t, ctypes.POINTER(Cpu), ctypes.POINTER(Env193), ctypes.POINTER(Diag193)]
    f.restype = ctypes.c_int
    return f


READKEY_KEYS = ('keys', 'cur', 'saved', 'w', 'arg', 'mode', 'flag49', 'buffered', 'brk', 'tail', 'in61', 'time_start', 'time_step', 'table')
DISPLAY_KEYS = ('msg', 'ega', 'attr', 'page_a', 'page_b', 'regs', 'hi', 'log_count', 'display_table')


def build(image_reloc, stub, o):
    rk = {k: o[k] for k in READKEY_KEYS if k in o}
    ss = o.get('ss', SS); sp = o.get('sp', 0xF000)
    m, c, envspec, keys = R.build(image_reloc, base=BASE, ss=ss, sp=sp, flags=o.get('flags', 2), cpu=o.get('cpu'), **rk)
    ds = BASE + SEG['data']
    ssv = ds if ss == 'ds' else ss
    s = physical(STUB_SEG, 0); m[s:s + len(stub)] = stub
    put16(m, LOG_SEG, 0, o.get('log_count', 0))
    tab = o.get('display_table') or [(0xA0 + k * 7 + 3) & 0xFF for k in range(256)]
    for k in range(256):
        m[physical(ds, 0x25D5 + k)] = tab[k]
    m[physical(ds, 0x4A1A)] = o.get('page_a', 7); m[physical(ds, 0x4A1B)] = o.get('page_b', 3); m[physical(ds, 0x4A1E)] = o.get('ega', 0)
    for k, v in enumerate(o.get('regs') or (0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0x6666, 0x7777, 0x8888, 0x9999, 0x0246)):
        put16(m, ds, 0x4A02 + 2 * k, v)
    msg = o.get('msg', b'Insert disk')
    m[physical(MSG_SEG, 0x100)] = len(msg) & 0xFF
    for k, ch in enumerate(msg[:255]):
        m[physical(MSG_SEG, 0x101 + k)] = ch
    put16(m, ssv, (sp + 4) & 0xFFFF, 0x0100); put16(m, ssv, (sp + 6) & 0xFFFF, MSG_SEG)
    hi = (o.get('hi') or (0xAB,))[0]
    put16(m, ssv, (sp + 8) & 0xFFFF, (hi << 8) | o.get('attr', 0x0E))
    c.cs, c.ip = BASE + SEG['text'], ENTRY
    return m, c, envspec, keys


def blocks_for(o):
    """The documented INT 10h register blocks of a RETURNING prompt: blank row 24 (0175), the message (0105), blank again, with Regs carried. Independent of both models."""
    regs = o.get('regs') or (0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0x6666, 0x7777, 0x8888, 0x9999, 0x0246)
    st = dict(ax=regs[0], bx=regs[1], cx=regs[2], dx=regs[3]); bp, si, di, es = regs[4], regs[5], regs[6], regs[8]
    ega, attr_arg = o.get('ega', 0), o.get('attr', 0x0E)
    pa, pb = o.get('page_a', 7), o.get('page_b', 3)
    tab = o.get('display_table') or [(0xA0 + k * 7 + 3) & 0xFF for k in range(256)]
    out = []

    def put(ch, count, row, col, attr):
        st['dx'] = (row << 8) | col
        st['ax'] = (0x02 << 8) | (st['ax'] & 0xFF); st['bx'] = (pb << 8) | (st['bx'] & 0xFF)
        out.append((st['ax'], st['bx'], st['cx'], st['dx'], si, di, bp, es))
        bl = tab[attr] if ega == 0 else attr
        st['ax'] = (0x09 << 8) | ch; st['bx'] = (pb << 8) | bl; st['cx'] = count
        out.append((st['ax'], st['bx'], st['cx'], st['dx'], si, di, bp, es))
        if ega == 1:
            st['ax'] = (0x02 << 8) | ch; st['bx'] = (pa << 8) | bl
            out.append((st['ax'], st['bx'], st['cx'], st['dx'], si, di, bp, es))
            st['ax'] = (0x09 << 8) | ch; st['bx'] = (pa << 8) | attr
            out.append((st['ax'], st['bx'], st['cx'], st['dx'], si, di, bp, es))
    msg = o.get('msg', b'Insert disk')[:40]
    put(0xDB, 0x28, 0x18, 0, 0)
    for i, ch in enumerate(msg):
        put(ch, 1, 0x18, i, attr_arg)
    put(0xDB, 0x28, 0x18, 0, 0)
    return out


def run_unicorn(name, m, c, envspec, keys, body_ips):
    from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE, UC_HOOK_INTR, UC_HOOK_INSN
    from unicorn import x86_const as RR
    env = Q.Env(**envspec); script = list(keys)
    uc = Uc(UC_ARCH_X86, UC_MODE_16); backing = (ctypes.c_uint8 * SIZE).from_buffer_copy(m)
    uc.mem_map_ptr(0, SIZE, 7, ctypes.addressof(backing)); uc.mem_map_ptr(SIZE, SIZE, 7, ctypes.addressof(backing))
    for (n, _), reg in zip(Cpu._fields_, REGISTERS):
        uc.reg_write(reg, getattr(c, n))
    s = dict(stop=None, steps=0); seen = set(); closure = set()
    known = {BASE + v for v in SEG.values()} | {STUB_SEG}
    text_cs = BASE + SEG['text']

    def code(em, address, size, user):
        cs = uc.reg_read(RR.UC_X86_REG_CS); ip = uc.reg_read(RR.UC_X86_REG_IP)
        if (cs, ip) == (BASE + SEG['system'], 0x00E9):
            s['stop'] = 'EXIT'; return uc.emu_stop()
        if (cs, ip) == (BASE + R.CRT, 0x014B):
            s['stop'] = 'BREAK'; return uc.emu_stop()
        if (cs, ip) == RETURN_TO:
            s['stop'] = 'RETURN'; return uc.emu_stop()
        if cs not in known:
            s['stop'] = 'TRANSFER'; return uc.emu_stop()
        closure.add((cs, ip))
        if cs == text_cs and ENTRY <= ip < END:
            check(ip in body_ips, f'escape inside 0193 {name} {ip:04x}'); seen.add(ip)
        s['steps'] += 1
        if s['steps'] > 900000:
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
        check(port in (0x40, 0x43, 0x61, 0xC0) and size == 1, f'ENV refusal {name}: OUT {port:#x}')
        env.log.append(('out', port, value))
    uc.hook_add(UC_HOOK_CODE, code); uc.hook_add(UC_HOOK_INTR, intr)
    uc.hook_add(UC_HOOK_INSN, port_in, None, 1, 0, RR.UC_X86_INS_IN)
    uc.hook_add(UC_HOOK_INSN, port_out, None, 1, 0, RR.UC_X86_INS_OUT)
    uc.emu_start(physical(c.cs, c.ip), 2 * SIZE, count=1000000)
    check(s['stop'] in ('RETURN', 'EXIT', 'BREAK', 'TRANSFER'), f'original stop {name}: {s["stop"]}')
    return uc, backing, env, s['stop'], seen, closure


def run_c(f, m, c, envspec, keys, stub_at=(STUB_SEG, 0), log_seg=LOG_SEG, kb_sps=None):
    keep = []
    q, env = Q.c_env(envspec, keep, stub_at=stub_at)
    video = B.c_video(keep, log_seg, stub_at, scratch=SCRATCH)
    q.video = video
    ke = R.KeyEnv(q, R.keyboard(env, keys, keep, kb_sps), None, 64)
    e = Env193(B.Env(video, None), ke)
    actual = Cpu.from_buffer_copy(c); ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(m); d = Diag193()
    status = f(ram, SIZE, ctypes.byref(actual), ctypes.byref(e), ctypes.byref(d))
    stop = 'RETURN' if status == 0 else R.STATUS.get(d.key_status) if status == 3 else STATUS.get(status)
    return stop, status, actual, bytes(ram), env, d


def compare(name, state, body_ips, f):
    m, c, envspec, keys = state
    uc, backing, uenv, ustop, seen, closure = run_unicorn(name, m, c, envspec, keys, body_ips)
    stop, status, actual, ram, cenv, d = run_c(f, m, c, envspec, keys)
    check(stop == ustop, f'C stop {name}: {stop} (status {STATUS.get(status)}, site {d.site:04x}, key {d.key_status}, callee {d.key.callee:04x}) original {ustop}')
    check(cenv.log == uenv.log, f'environment events {name}: C {cenv.log[:6]} original {uenv.log[:6]}')
    if ram != bytes(backing):
        diff = [hex(i) for i, (a, b) in enumerate(zip(ram, bytes(backing))) if a != b]
        raise ValueError(f'RAM {name}: {diff[:8]} ({len(diff)} bytes)')
    for (n, _), reg in zip(Cpu._fields_, REGISTERS):
        want = uc.reg_read(reg) & 0xFFFF
        check(getattr(actual, n) == want, f'CPU {name}/{n} C {getattr(actual, n):04x} original {want:04x}')
    return ustop, seen, closure, actual, ram, d


K = R.K


def cases():
    rng = random.Random(0x08020193)
    pr = lambda n: bytes(rng.randrange(1, 256) for _ in range(n))
    out = []
    a = (K['a'],)
    for ega in (0, 1, 2, 0xFF):
        for n in (0, 1, 2, 11, 40, 41, 60, 255):
            out.append((f'ega{ega:02x}-len{n}', dict(msg=pr(n), ega=ega, attr=rng.choice((0x00, 0x0E, 0xFF, rng.randrange(256))), keys=a), True))
    for keys_, label in (((K['a'], K['x'], K['a'], K['x']), 'drain-3'), ((K['a'], K['f1'], K['a']), 'drain-ext'), ((K['f1'],), 'ext-first'), ((K['zero'],), 'zero-first'),
                         ((K['zero'], K['a']), 'zero-then'), ((), 'buffered')):
        out.append((label, dict(keys=keys_, buffered=0x48 if label == 'buffered' else 0, msg=b'Type-ahead', ega=rng.choice((0, 1))), True))
    for cur in (0, 1, 2):                                                      # ctrl-S: the sound toggle runs the 08AE driver inside the prompt
        for saved in (0, 1, 5):
            out.append((f'ctrl-s-c{cur}-s{saved}', dict(keys=(K['s'],), cur=cur, saved=saved, w=1, msg=b'Sound', ega=0), True))
    out += [(f'ctrl-d-49f7-{v:x}', dict(keys=(K['d'],), flag49=v, msg=b'Toggle'), True) for v in (0, 1, 0xFF)]
    for tail in (b' STING', b'', b' sting'):
        out.append((f'ctrl-c-tail{tail!r}', dict(keys=(K['c'],), tail=tail, msg=b'Quit?', cur=2, saved=1), tail != b' STING'))
    out += [('break-a', dict(keys=a, brk=1, msg=b'Break'), False), ('break-drain', dict(keys=(K['a'], K['x']), brk=1, msg=b'Break'), False)]
    for fl in FLAGS + DIRTY_FLAGS:
        out.append((f'flags{fl:04x}', dict(flags=fl, msg=b'Flags', keys=a, ega=rng.choice((0, 1))), True))
    out += [(f'attr{at:02x}', dict(attr=at, msg=b'Attr', keys=a, ega=e), True) for at in (0x00, 0x01, 0x0F, 0x7F, 0x80, 0xFF) for e in (0, 1)]
    for pa, pb in ((0, 0), (7, 3), (3, 7), (0xFF, 0)):
        out.append((f'pages{pa:02x}{pb:02x}', dict(page_a=pa, page_b=pb, ega=1, msg=b'Pg', keys=a), True))
    out += [('sp-min', dict(sp=0x0200, msg=b'ab', keys=a), False), ('sp-odd', dict(sp=0xF001, msg=b'ab', keys=a), False), ('ss-ds', dict(ss='ds', sp=0xE000, msg=b'ab', keys=a), False),
            ('log-count-15', dict(log_count=15, ega=1, msg=b'ab', keys=a), False)]
    for k in range(16):
        n = rng.choice((0, 1, 5, 40, rng.randrange(256)))
        out.append((f'random{k}', dict(msg=pr(n), ega=rng.choice((0, 1, 2)), attr=rng.randrange(256), keys=tuple(rng.choice(list(K.values()) + [rng.randrange(1, 65536)]) for _ in range(rng.randrange(1, 4))),
                                       flags=rng.choice(FLAGS + DIRTY_FLAGS), cpu=tuple(rng.randrange(65536) for _ in range(7)), page_a=rng.randrange(256), page_b=rng.randrange(256),
                                       regs=tuple(rng.randrange(65536) for _ in range(10)), cur=rng.choice((0, 1, 2)), saved=rng.choice((0, 1, 5)), w=rng.choice((0, 1)),
                                       in61=rng.randrange(256), display_table=[rng.randrange(256) for _ in range(256)]), False))
    return out


def differential(image_reloc, stub, decoded, f):
    seen, closure, stops = set(), set(), {}
    specs = cases(); ds = BASE + SEG['data']; plain = 0; chars = {}
    for name, o, independent in specs:
        state = build(image_reloc, stub, o)
        stop, v, cl, final, ram, d = compare(name, state, set(decoded), f)
        seen |= v; closure |= cl; stops[stop] = stops.get(stop, 0) + 1
        want = R.expected(o)
        if o.get('brk') is None and o.get('ss') is None and o.get('cpu') is None and o.get('sp') is None:
            check(stop == want['stop'], f'{name}: independent key stop {want["stop"]}, got {stop}')
            if stop == 'RETURN':
                check((ram[physical(ds, 0x5372)], ram[physical(ds, 0x5373)], ram[physical(ds, 0x49F7)]) == (want['cur'], want['saved'], want['flag49']), f'{name}: hotkey state')
                check(ram[physical(SS, 0xF000 - 2 - 0x2A)] == want['key'] & 0xFF, f'{name}: [BP-2Ah] is the key')
        if independent and stop == 'RETURN':
            got, wantblocks = B.read_log(ram), blocks_for(o)
            check(got == wantblocks, f'{name}: INT 10h blocks differ from the contract ({len(got)} vs {len(wantblocks)})')
            plain += 1
        chars[d.string.chars] = chars.get(d.string.chars, 0) + 1
    check(seen == set(decoded), f'0193 body starts missing {sorted(set(decoded) - seen)}')
    starts = {'0802:0193 body': f'{len(seen)}/{len(decoded)}'}
    for label, seg, lo, hi in MEMBERS:
        base = SEG[seg] * 16
        dec = {i.address for i in Cs(CS_ARCH_X86, CS_MODE_16).disasm(image_reloc[base + lo:base + hi], lo)}
        ran = {ip for cs, ip in closure if cs == BASE + SEG[seg] and lo <= ip < hi}
        check(ran == dec, f'{label}: executed starts {len(ran)} of {len(dec)} decoded')
        starts[label] = f'{len(ran)}/{len(dec)}'
    rk_dec = {i.address for i in Cs(CS_ARCH_X86, CS_MODE_16).disasm(image_reloc[SEG['body'] * 16 + R.ENTRY:SEG['body'] * 16 + R.END], R.ENTRY)}
    rk_ran = {ip for cs, ip in closure if cs == BASE + SEG['body'] and R.ENTRY <= ip < R.END}
    check(rk_ran <= rk_dec, 'ReadKey executed starts off the decode')
    starts['0899:0059 ReadKey (its own audit owns its coverage)'] = f'{len(rk_ran)}/{len(rk_dec)}'
    return dict(cases=len(specs), stops=dict(sorted(stops.items())), independent_contract_cases=plain, closure_member_starts=starts, conditional_outcomes_total=0,
                dirty_flag_images=[f'{x:04X}' for x in DIRTY_FLAGS], cpu_words=14, ram_bytes=SIZE, flag_masks=0)


def main():
    p = argparse.ArgumentParser()
    for flag in ('--compile-only', '--unicorn', '--native'):
        p.add_argument(flag, action='store_true')
    p.add_argument('--output', type=Path); a = p.parse_args()
    with tempfile.TemporaryDirectory(prefix='r0802-0193-') as tmp:
        f = library(Path(tmp))
        if a.compile_only:
            print('CHECKER_BUILD_OK'); return
        image, sites = R.Q.load_image()
        body, decoded = source(image, sites)
        ownership_report = ownership(image)
        stub = B.assemble_stub(scratch=SCRATCH)
        image_reloc = Q.relocate(image, sites, BASE)
        report = dict(target=dict(span='0802:[0193,01ED)', bytes=len(body), instructions=len(decoded), sha256=DIGEST, image_sha256=Q.IMAGE_DIGEST, load_base=f'{BASE:04X}',
                                  int10_stub_sha256=sha256(stub).hexdigest()), ownership=ownership_report)
        if a.unicorn:
            report['unicorn'] = differential(image_reloc, stub, decoded, f)
        if a.native:
            import res0802_0193_native
            report['native'] = res0802_0193_native.native(image, sites, decoded, f, sys.modules[__name__], stub)
    if a.output:
        a.output.write_text(json.dumps(report, sort_keys=True, indent=2) + '\n')
    print(json.dumps(report, sort_keys=True))


if __name__ == '__main__':
    main()
