#!/usr/bin/env python3
"""RES_0709_0BF5 (the message-line prompt) against the original bytes, as a composed closure: the body, SYSTEM StrStore 0AF8:0B25, the row clear 0709:05EB and
the string printer 0709:0626 with the +01DF glyph writer and SYSTEM UpCase beneath them (the EGA the declared hardware), and ReadKey-with-hotkeys 0899:0059 with ITS
closure (Crt ReadKey and KeyPressed with INT 16h the declared service, the 08AE driver members, SYSTEM leaves, QUIT).

The whole loader-expanded START.EXE image is loaded at a test base with the loader's own relocation table applied; every original byte of the closure runs. The A000
aperture is an independent Python EGA device (planes, latches, indexed registers) on an MMIO window of the oracle; every OUT is forwarded to it. Declared environment,
identical in the oracle and the C callbacks: ReadKey's (INT 21h 25h/35h/2Ch, ports 40h/43h/61h/C0h, INT 16h from a per-case key script, QUIT's INT 10h stub) and the EGA
sequencer/graphics-controller ports 3C4h/3C5h/3CEh/3CFh. Compared, unmasked, after every case: CPU14, ALL 1,048,576 RAM bytes (the aperture's backing bytes included), the
full EGA state, the ordered EGA OUT events (port, value, size, CS:IP), the ordered VRAM write operands, the ordered ReadKey environment events, and the stop. What this pins:
the C model equals the original closure; the frames 0BF5 hands each callee equal an independent Python statement of its call sites; the key and the hotkey state equal the
ReadKey audit's independent statement. What it cannot pin: what a real EGA or BIOS does: that is the native run. Media stays under ignored resources/."""
import argparse, ctypes, json, random, struct, subprocess, sys, tempfile
from hashlib import sha256
from pathlib import Path
from capstone import Cs, CS_ARCH_X86, CS_MODE_16
import res0899_readkey_0059_cpu_audit as R
import res0899_quit_0000_cpu_audit as Q
import res0709_ega_reset_0000_cpu_audit as E0
import res0709_glyph_01df_cpu_audit as G
from res0709_ega_reset_0000_cpu_audit import Cpu, ROOT, SIZE, REGISTERS, physical, put16, check
from res00ba_display_1693_cpu_audit import PythonEga

CHECKER_ROWS = ('RES_0709_0BF5',)
ENTRY, END = 0x0BF5, 0x0C3F
DIGEST = '4a3f940caae95794f257a25f5241bcf3a04c2749b212f5b8d9def5befd63a4cc'
BASE, SS, STUB_SEG = R.BASE, R.SS, R.STUB_SEG
SEG = dict(R.SEGS, vid=0x0709)
FLAGS = R.FLAGS
DIRTY_FLAGS = (0x0202, 0x0602, 0x0A87, 0x0402, 0x0E57)       # the standing dirty-incoming-flags set (see the 0105/0175/0193 audits)
MSG_SEG, FONT_SEG = 0x3000, 0x7400
RETURN_TO = R.RETURN_TO
EGA_PORTS = (0x3C4, 0x3C5, 0x3CE, 0x3CF)
STATUS = {0: 'RETURNED', 1: 'ARGUMENT', 2: 'SPACE', 3: 'PRINTER', 4: 'KEY', 5: 'CALLEE'}
BODY_CALLS = [(0x0C09, 'lcall', '0xaf8, 0xb25'), (0x0C1B, 'call', '0x5eb'), (0x0C2E, 'call', '0x626'), (0x0C31, 'lcall', '0x899, 0x59')]
MEMBERS = [('0709:05EB row clear', 'vid', 0x05EB, 0x0626), ('0709:0626 printer', 'vid', 0x0626, 0x06A7), ('0709:01DF glyph', 'vid', 0x01DF, 0x05EB),
           ('0AF8:0B25 StrStore', 'system', 0x0B25, 0x0B49), ('0AF8:182A UpCase', 'system', 0x182A, 0x183D)]
READKEY_KEYS = ('keys', 'cur', 'saved', 'w', 'arg', 'mode_unused', 'flag49', 'buffered', 'brk', 'tail', 'in61', 'time_start', 'time_step', 'table')


class Ega(ctypes.Structure):
    _fields_ = [('plane', (ctypes.c_uint8 * 65536) * 4), ('latch', ctypes.c_uint8 * 4), ('seq_index', ctypes.c_uint8), ('seq', ctypes.c_uint8 * 5),
                ('gc_index', ctypes.c_uint8), ('gc', ctypes.c_uint8 * 9)]


class Bus(ctypes.Structure):
    _fields_ = [('ram', ctypes.POINTER(ctypes.c_uint8)), ('ega', ctypes.POINTER(Ega)), ('ports', ctypes.POINTER(E0.Env)), ('on_vram_write', G.WRITE_FN), ('ctx', ctypes.c_void_p)]


class SpaceDiag(ctypes.Structure):
    _fields_ = [(n, ctypes.c_int) for n in ('callee', 'status', 'glyph_callee', 'glyph_status')]


class Env0BF5(ctypes.Structure):
    _fields_ = [('key', R.KeyEnv), ('space', ctypes.c_size_t), ('printer', ctypes.c_size_t), ('glyph', ctypes.c_size_t)]


class Diag0BF5(ctypes.Structure):
    _fields_ = [('site', ctypes.c_uint16), ('callee_status', ctypes.c_int), ('space', SpaceDiag), ('printer', SpaceDiag), ('key_status', ctypes.c_int), ('key', R.KeyDiag)]


def source(image, sites):
    b = SEG['vid'] * 16
    body = image[b + ENTRY:b + END]
    check(len(body) == 74 and sha256(body).hexdigest() == DIGEST, '0709:0BF5 body identity')
    ins = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(body, ENTRY))
    check(len(ins) == 38 and sum(i.size for i in ins) == len(body), 'decode covers the span')
    check([(i.address, i.mnemonic, i.op_str) for i in ins if 'call' in i.mnemonic] == BODY_CALLS, 'four callees')
    check([i.address for i in ins if i.mnemonic in ('ret', 'retf')] == [0x0C3C], 'single RETF 6 at 0C3C')
    check(not [i for i in ins if i.mnemonic in ('int', 'in', 'out', 'ljmp') or i.mnemonic.startswith('j')], 'straight line: no jump, interrupt or port instruction')
    check(sorted(s - b for s in sites if b + ENTRY <= s < b + END) == [0x0C0C, 0x0C34], 'the two far-call segment words are the only relocations')
    return body, {i.address: i for i in ins}


def ownership(image):
    from overlay_inventory import find_units, parse_mz
    from u86_procvars_3413_audit import EXE_SHA, OVR_SHA
    exe = (ROOT / 'resources/DOS/POOLRAD/START.EXE').read_bytes(); ovr = (ROOT / 'resources/DOS/POOLRAD/GAME.OVR').read_bytes()
    check(sha256(exe).hexdigest() == EXE_SHA and sha256(ovr).hexdigest() == OVR_SHA, 'media provenance')
    pattern = struct.pack('<HH', ENTRY, SEG['vid'])
    far = [i for i in range(1, len(image) - 3) if image[i:i + 4] == pattern and image[i - 1] in (0x9A, 0xEA)]
    direct = sorted(f'{u["stub_para"]:04X}:{f - 3:04X}' for u in find_units(exe[parse_mz(exe)['header_bytes']:], ovr) for f in u['fixups']
                    if ovr[u['ovr_offset'] + f - 3:u['ovr_offset'] + f - 2] == b'\x9a' and struct.unpack_from('<HH', ovr, u['ovr_offset'] + f - 2) == (ENTRY, SEG['vid']))
    return dict(far_transfers_in_resident_image=[hex(i - 1) for i in far], overlay_far_call_sites=direct)


def library(directory):
    path = directory / 'r0709_0bf5.so'
    sources = list(R.SOURCES) + ['resident_0709_msgbox_0bf5_cpu', 'resident_0709_space_05eb_cpu', 'resident_0709_printer_0626_cpu', 'resident_0709_glyph_01df_cpu',
                                 'system_strstore_fullstate_0b25', 'system_runtime_leaves_cpu', 'memory_bus', 'ega_hw']
    subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Wpedantic', '-Werror', '-O2', '-fPIC', '-shared', '-I', str(ROOT / 'include')] +
                   [str(ROOT / f'src/recovered/{s}.c') for s in dict.fromkeys(sources)] + ['-o', str(path)], check=True)
    lib = ctypes.CDLL(str(path))
    f = lib.pr_r0709_msgbox_0bf5
    f.argtypes = [ctypes.POINTER(Bus), ctypes.c_size_t, ctypes.POINTER(Cpu), ctypes.POINTER(Env0BF5), ctypes.POINTER(Diag0BF5)]
    f.restype = ctypes.c_int
    lib.pr_ega_reset.argtypes = [ctypes.POINTER(Ega)]
    return f, lib


def build(image_reloc, o):
    rk = {k: o[k] for k in READKEY_KEYS if k in o and k != 'mode_unused'}
    ss = o.get('ss', SS); sp = o.get('sp', 0xF000)
    m, c, envspec, keys = R.build(image_reloc, base=BASE, ss=ss, sp=sp, flags=o.get('flags', 2), cpu=o.get('cpu'), **rk)
    ds = BASE + SEG['data']
    ssv = ds if ss == 'ds' else ss
    # the +01DF glyph writer's display state (the glyph audit's recipe): mode, pages, font pointer, masks, tables, 8x8 font
    m[physical(ds, 0x4A1E)] = o.get('mode', 1)
    pages = o.get('pages', (0xA000, 0xA200))
    put16(m, ds, 0x4A16, pages[0]); put16(m, ds, 0x4A18, pages[1])
    fo = o.get('fontoff', 0x1000)
    put16(m, ds, 0x5E20, 0 if o.get('nofont') else fo); put16(m, ds, 0x5E22, 0 if o.get('nofont') else FONT_SEG)    # a zero font pointer makes +01DF return without drawing
    for j, v in enumerate(o.get('mask', (1, 2, 4, 8))):
        m[physical(ds, 0x25C9 + j)] = v
    for j, v in enumerate((0, 0x5555, 0xAAAA, 0xFFFF)):
        put16(m, ds, 0x25CD + 2 * j, v)
    for j in range(256):
        m[physical(ds, 0x25D5 + j)] = j % 4
    for j in range(8):
        put16(m, ds, 0x25E5 + 2 * j, 0xC000 >> (2 * j))
    for j in range(2048):
        m[physical(FONT_SEG, (fo + j) & 0xFFFF)] = ((j * 37 + (j >> 3) * 13) ^ 0xA6) & 255
    msg = o.get('msg', b'Insert disk')
    off = o.get('msg_off', 0x0100)
    mseg = o.get('msg_seg', MSG_SEG)
    if o.get('self_copy'):
        mseg, off = ssv, (sp - 2 - 0x29) & 0xFFFF            # the source IS the destination buffer BP-29h
    m[physical(mseg, off)] = len(msg) & 0xFF
    for k, ch in enumerate(msg[:255]):
        m[physical(mseg, (off + 1 + k) & 0xFFFF)] = ch
    put16(m, ssv, (sp + 4) & 0xFFFF, off); put16(m, ssv, (sp + 6) & 0xFFFF, mseg)
    put16(m, ssv, (sp + 8) & 0xFFFF, (o.get('hi', 0xAB) << 8) | o.get('color', 0x0E))
    c.cs, c.ip = BASE + SEG['vid'], ENTRY
    return m, c, envspec, keys


def new_device():
    return PythonEga()


def run_unicorn(name, m, c, envspec, keys, body_ips):
    from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE, UC_HOOK_INTR, UC_HOOK_INSN
    from unicorn import x86_const as RR
    env = Q.Env(**envspec); script = list(keys); device = new_device()
    uc = Uc(UC_ARCH_X86, UC_MODE_16); backing = (ctypes.c_uint8 * SIZE).from_buffer_copy(m)

    def vread(em, off, size, user):
        return sum(device.read((off + j) & 65535) << (8 * j) for j in range(size))

    def vwrite(em, off, size, value, user):
        for j in range(size):
            device.write((off + j) & 65535, (value >> (8 * j)) & 255)
    for origin in (0, SIZE):                     # the aperture is an independent device; all other backing RAM is flat
        uc.mem_map_ptr(origin, 0xA0000, 7, ctypes.addressof(backing)); uc.mmio_map(origin + 0xA0000, 65536, vread, None, vwrite, None)
        uc.mem_map_ptr(origin + 0xB0000, 0x50000, 7, ctypes.addressof(backing) + 0xB0000)
    for (n, _), reg in zip(Cpu._fields_, REGISTERS):
        uc.reg_write(reg, getattr(c, n))
    s = dict(stop=None, steps=0); seen = set(); closure = set(); ega_events = []; frames = {}
    known = {BASE + v for v in SEG.values()} | {STUB_SEG}
    vid = BASE + SEG['vid']; sysc = BASE + SEG['system']; body = BASE + SEG['body']
    sp0 = c.sp

    def code(em, address, size, user):
        cs = uc.reg_read(RR.UC_X86_REG_CS); ip = uc.reg_read(RR.UC_X86_REG_IP)
        if (cs, ip) == (sysc, 0x00E9):
            s['stop'] = 'EXIT'; return uc.emu_stop()
        if (cs, ip) == (BASE + R.CRT, 0x014B):
            s['stop'] = 'BREAK'; return uc.emu_stop()
        if (cs, ip) == RETURN_TO:
            s['stop'] = 'RETURN'; return uc.emu_stop()
        if cs not in known:
            s['stop'] = 'TRANSFER'; return uc.emu_stop()
        closure.add((cs, ip))
        if cs == vid and ENTRY <= ip < END:
            check(ip in body_ips, f'escape inside 0BF5 {name} {ip:04x}'); seen.add(ip)
        # the frames 0BF5 hands each callee, read at the callee's entry as its own bytes run
        for key, cs_, ip_ in (('strstore', sysc, 0x0B25), ('space', vid, 0x05EB), ('printer', vid, 0x0626), ('readkey', body, 0x0059)):
            if (cs, ip) == (cs_, ip_) and key not in frames:
                spv = uc.reg_read(RR.UC_X86_REG_SP); ssv_ = uc.reg_read(RR.UC_X86_REG_SS)
                frames[key] = (spv, tuple(struct.unpack('<HHHHHHHH', bytes(uc.mem_read(physical(ssv_, spv), 16)))))
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
        if port in EGA_PORTS:
            ega_events.append((port, value, size, uc.reg_read(RR.UC_X86_REG_CS), uc.reg_read(RR.UC_X86_REG_IP))); device.out(port, value, size); return
        check(port in (0x40, 0x43, 0x61, 0xC0) and size == 1, f'ENV refusal {name}: OUT {port:#x}')
        env.log.append(('out', port, value))
    uc.hook_add(UC_HOOK_CODE, code); uc.hook_add(UC_HOOK_INTR, intr)
    uc.hook_add(UC_HOOK_INSN, port_in, None, 1, 0, RR.UC_X86_INS_IN)
    uc.hook_add(UC_HOOK_INSN, port_out, None, 1, 0, RR.UC_X86_INS_OUT)
    uc.emu_start(physical(c.cs, c.ip), 2 * SIZE, count=4000000)
    check(s['stop'] in ('RETURN', 'EXIT', 'BREAK', 'TRANSFER'), f'original stop {name}: {s["stop"]}')
    return uc, backing, env, s['stop'], seen, closure, device, ega_events, frames, sp0


def run_c(f, lib, m, c, envspec, keys):
    keep = []
    q, env = Q.c_env(envspec, keep)
    ke = R.KeyEnv(q, R.keyboard(env, keys, keep, None), None, 64)
    e = Env0BF5(ke, 65536, 65536, 65536)
    actual = Cpu.from_buffer_copy(c); ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(m)
    ega = Ega(); lib.pr_ega_reset(ctypes.byref(ega))
    for p in range(4):
        ctypes.memset(ctypes.addressof(ega.plane[p]), 0x31 + p * 0x27, 65536)
    events, writes = [], []

    def out(ctx, port, value, size, cs, ip):
        events.append((port, value, size, cs, ip)); return 0

    def write(ctx, off, value):
        writes.append((off, value))
    ports = E0.Env(None, E0.OUT_FN(out), E0.IN_FN())
    keep.extend([ports])
    bus = Bus(ram, ctypes.pointer(ega), ctypes.pointer(ports), G.WRITE_FN(write), None)
    d = Diag0BF5()
    status = f(ctypes.byref(bus), SIZE, ctypes.byref(actual), ctypes.byref(e), ctypes.byref(d))
    stop = 'RETURN' if status == 0 else R.STATUS.get(d.key_status) if status == 4 else STATUS.get(status)
    if status == 0 and (actual.cs, actual.ip) != RETURN_TO:
        stop = 'TRANSFER'           # RETF 6 took a corrupted return frame: the oracle stops at the first foreign fetch
    return stop, status, actual, bytes(ram), env, d, events, writes, bytes(ega)


def run_c_native(m, c, envspec, keys, stub_at, kb_sps):
    """The C side of a native case: no INT 10h stub (the display is the EGA); records the SP of every INT 16h call and returns the four planes."""
    keep = []
    q, env = Q.c_env(envspec, keep, stub_at=stub_at)
    ke = R.KeyEnv(q, R.keyboard(env, keys, keep, kb_sps), None, 64)
    e = Env0BF5(ke, 65536, 65536, 65536)
    actual = Cpu.from_buffer_copy(c); ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(m)
    ega = Ega(); LIB.pr_ega_reset(ctypes.byref(ega))
    for p in range(4):
        ctypes.memset(ctypes.addressof(ega.plane[p]), 0x31 + p * 0x27, 65536)
    ports = E0.Env(None, E0.OUT_FN(lambda *a: 0), E0.IN_FN())
    keep.append(ports)
    bus = Bus(ram, ctypes.pointer(ega), ctypes.pointer(ports), G.WRITE_FN(lambda *a: None), None)
    d = Diag0BF5()
    status = F(ctypes.byref(bus), SIZE, ctypes.byref(actual), ctypes.byref(e), ctypes.byref(d))
    stop = 'RETURN' if status == 0 else R.STATUS.get(d.key_status) if status == 4 else STATUS.get(status)
    return stop, status, actual, bytes(ram), env, bytes(ega)


def expected_frames(o, sp0):
    """The words at each callee's entry, derived from the instruction listing and the case alone (independent of both models):
    [IP, CS] then the pushed arguments. AH of each pushed AX is whatever the caller's AX held: only the low bytes are asserted."""
    bp = (sp0 - 2) & 0xFFFF; buf = (bp - 0x29) & 0xFFFF
    ssv = o.get('ss', SS)
    return dict(strstore=[0x0C0E, None, 0x0028, buf, None, o.get('msg_off', 0x0100) if not o.get('self_copy') else buf, None],
                space=[0x0C1E, None, 0x0028 & 0xFF, 0x0000, 0x0018, 0x0000],
                printer=[0x0C31, None, o.get('color', 0x0E), 0x0018, 0x0000],
                readkey=[0x0C36, None])


def check_frames(name, o, frames, sp0, c0):
    """Compare the low bytes of every argument word to the contract; segment words must equal the live CS / SS / message segment."""
    bp = (sp0 - 2) & 0xFFFF; buf = (bp - 0x29) & 0xFFFF
    got = {k: v[1] for k, v in frames.items()}
    exp = expected_frames(o, sp0)
    ssv = BASE + SEG['data'] if o.get('ss') == 'ds' else o.get('ss', SS)
    msg_seg = ssv if o.get('self_copy') else o.get('msg_seg', MSG_SEG)
    if 'strstore' in got:
        g = got['strstore']
        check(g[0] == 0x0C0E and g[1] == BASE + SEG['vid'] and g[2] == 0x0028 and g[3] == buf and g[4] == ssv and g[5] == (exp['strstore'][5]) and g[6] == msg_seg, f'{name}: StrStore frame {g}')
    if 'space' in got:
        g = got['space']
        check(g[0] == 0x0C1E and g[1] == BASE + SEG['vid'] and [x & 0xFF for x in g[2:6]] == [0x28, 0x00, 0x18, 0x00], f'{name}: 05EB frame {g}')
    if 'printer' in got:
        g = got['printer']
        check(g[0] == 0x0C31 and g[1] == BASE + SEG['vid'] and g[2] == buf and g[3] == ssv and (g[4] & 0xFF) == o.get('color', 0x0E) & 0xFF and (g[5] & 0xFF) == 0x18 and (g[6] & 0xFF) == 0, f'{name}: 0626 frame {g}')   # [SP+4..] = last pushed first: offset, segment, colour, y, x
    if 'readkey' in got:
        g = got['readkey']
        check(g[0] == 0x0C36 and g[1] == BASE + SEG['vid'], f'{name}: ReadKey frame {g}')


def compare(name, state, body_ips, f, lib, o):
    m, c, envspec, keys = state
    uc, backing, uenv, ustop, seen, closure, device, uega, frames, sp0 = run_unicorn(name, m, c, envspec, keys, body_ips)
    stop, status, actual, ram, cenv, d, cega, cwrites, cstate = run_c(f, lib, m, c, envspec, keys)
    check(stop == ustop, f'C stop {name}: {stop} (status {STATUS.get(status)}, site {d.site:04x}, key {d.key_status}) original {ustop}')
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
    check_frames(name, o, frames, sp0, c)
    return ustop, seen, closure, actual, ram, d, device, uega, frames


K = R.K
F = LIB = None
IMAGE_LEN = 0


def cases():
    rng = random.Random(0x07090BF5)
    pr = lambda n: bytes(rng.randrange(1, 256) for _ in range(n))
    out = []
    a = (K['a'],)
    for mode in (0, 1, 2, 3):
        for n in (0, 1, 2, 11, 39, 40, 41, 60, 255):
            out.append((f'mode{mode}-len{n}', dict(msg=pr(n), mode=mode, color=rng.choice((0x00, 0x0E, 0x0F, 0x05, rng.randrange(256))), keys=a)))
    for color in range(16):
        out.append((f'color{color:02x}', dict(msg=b'Press a key', color=color, mode=color % 4, keys=a)))
    for keys_, label in (((K['a'], K['x'], K['a'], K['x']), 'drain-3'), ((K['a'], K['f1'], K['a']), 'drain-ext'), ((K['f1'],), 'ext-first'), ((K['zero'],), 'zero-first'),
                         ((K['zero'], K['a']), 'zero-then'), ((), 'buffered')):
        out.append((label, dict(keys=keys_, buffered=0x48 if label == 'buffered' else 0, msg=b'Type-ahead', mode=rng.choice((0, 1, 2, 3)))))
    for cur in (0, 1, 2):                                                      # ctrl-S: the sound toggle runs the 08AE driver inside the prompt
        for saved in (0, 1, 5):
            out.append((f'ctrl-s-c{cur}-s{saved}', dict(keys=(K['s'],), cur=cur, saved=saved, w=1, msg=b'Sound', mode=rng.choice((0, 1)))))
    out += [(f'ctrl-d-49f7-{v:x}', dict(keys=(K['d'],), flag49=v, msg=b'Toggle')) for v in (0, 1, 0xFF)]
    for tail in (b' STING', b'', b' sting'):                                   # ctrl-C: with ParamStr 'STING' the prompt EXITs through QUIT
        out.append((f'ctrl-c-tail{tail!r}', dict(keys=(K['c'],), tail=tail, msg=b'Quit?', cur=2, saved=1)))
    out += [('break-a', dict(keys=a, brk=1, msg=b'Break')), ('break-drain', dict(keys=(K['a'], K['x']), brk=1, msg=b'Break'))]
    for fl in FLAGS + DIRTY_FLAGS:
        out.append((f'flags{fl:04x}', dict(flags=fl, msg=b'Flags', keys=a, mode=rng.choice((0, 1, 2, 3)))))
    for pa, pb in ((0xA000, 0xA200), (0xA200, 0xA000), (0xA000, 0xA000), (0xA400, 0xA600)):
        out.append((f'pages{pa:04x}{pb:04x}', dict(pages=(pa, pb), mode=1, msg=b'Pg', keys=a)))
    out += [('nofont', dict(nofont=True, msg=b'No font', keys=a)), ('nofont-mode3', dict(nofont=True, mode=3, msg=b'No font', keys=a))]
    out += [('sp-min', dict(sp=0x016A, msg=b'ab', keys=a)), ('sp-max', dict(sp=0xFFF6, msg=b'ab', keys=a)), ('sp-odd', dict(sp=0xF001, msg=b'ab', keys=a)),
            ('ss-ds', dict(ss='ds', sp=0xE000, msg=b'ab', keys=a)), ('self-copy', dict(self_copy=True, msg=b'Overlap', keys=a)),
            ('self-copy-long', dict(self_copy=True, msg=pr(42), keys=a)), ('msg-wrap', dict(msg_off=0xFFF0, msg=pr(30), keys=a))]
    for k in range(24):
        n = rng.choice((0, 1, 5, 40, rng.randrange(256)))
        out.append((f'random{k}', dict(msg=pr(n), mode=rng.choice((0, 1, 2, 3)), color=rng.randrange(256), keys=tuple(rng.choice(list(K.values()) + [rng.randrange(1, 65536)]) for _ in range(rng.randrange(1, 4))),
                                       flags=rng.choice(FLAGS + DIRTY_FLAGS), cpu=tuple(rng.randrange(65536) for _ in range(7)), hi=rng.randrange(256),
                                       cur=rng.choice((0, 1, 2)), saved=rng.choice((0, 1, 5)), w=rng.choice((0, 1)), in61=rng.randrange(256),
                                       pages=rng.choice(((0xA000, 0xA200), (0xA400, 0xA000))), fontoff=rng.choice((0x1000, 0x2000, 0x0010)))))
    return out


def differential(image_reloc, decoded, f, lib):
    seen, closure, stops = set(), set(), {}
    specs = cases(); ds = BASE + SEG['data']; writes = 0; outs = 0; returns = 0
    for name, o in specs:
        state = build(image_reloc, o)
        stop, v, cl, final, ram, d, device, uega, frames = compare(name, state, set(decoded), f, lib, o)
        seen |= v; closure |= cl; stops[stop] = stops.get(stop, 0) + 1; writes += len(device.writes); outs += len(uega)
        want = R.expected(dict(o, keys=o.get('keys', (0x1E61,))))
        if o.get('brk') is None and o.get('ss') is None and o.get('cpu') is None and o.get('sp') is None:
            check(stop == want['stop'], f'{name}: independent key stop {want["stop"]}, got {stop}')
            if stop == 'RETURN':
                check((ram[physical(ds, 0x5372)], ram[physical(ds, 0x5373)], ram[physical(ds, 0x49F7)]) == (want['cur'], want['saved'], want['flag49']), f'{name}: hotkey state')
        if stop == 'RETURN':
            returns += 1
            sp0 = o.get('sp', 0xF000); ssv = ds if o.get('ss') == 'ds' else o.get('ss', SS)
            msg = o.get('msg', b'Insert disk'); n = min(len(msg) & 0xFF, 40)
            at = physical(ssv, ((sp0 - 2 - 0x29) & 0xFFFF))
            if not o.get('self_copy'):
                check(ram[at] == n and bytes(ram[at + 1:at + 1 + n]) == msg[:n], f'{name}: the Pascal copy at BP-29h')
            if o.get('cpu') is None and o.get('sp') is None and o.get('ss') is None:
                check(ram[physical(ssv, (sp0 - 2 - 0x2A) & 0xFFFF)] == final.ax & 0xFF == (R.expected(dict(o, keys=o.get('keys', (0x1E61,))))['key'] & 0xFF), f'{name}: [BP-2Ah] is the key')
    check(seen == set(decoded), f'0BF5 body starts missing {sorted(set(decoded) - seen)}')
    starts = {'0709:0BF5 body': f'{len(seen)}/{len(decoded)}'}
    for label, seg, lo, hi in MEMBERS:
        base = SEG[seg] * 16
        dec = {i.address for i in Cs(CS_ARCH_X86, CS_MODE_16).disasm(image_reloc[base + lo:base + hi], lo)}
        ran = {ip for cs, ip in closure if cs == BASE + SEG[seg] and lo <= ip < hi}
        starts[label] = f'{len(ran)}/{len(dec)}' + (' (unexecuted: ' + ', '.join(f'{x:04X}' for x in sorted(dec - ran)) + ')' if dec - ran else '')
    rk_dec = {i.address for i in Cs(CS_ARCH_X86, CS_MODE_16).disasm(image_reloc[SEG['body'] * 16 + R.ENTRY:SEG['body'] * 16 + R.END], R.ENTRY)}
    rk_ran = {ip for cs, ip in closure if cs == BASE + SEG['body'] and R.ENTRY <= ip < R.END}
    check(rk_ran <= rk_dec, 'ReadKey executed starts off the decode')
    starts['0899:0059 ReadKey (its own audit owns its coverage)'] = f'{len(rk_ran)}/{len(rk_dec)}'
    return dict(cases=len(specs), stops=dict(sorted(stops.items())), returning_cases=returns, closure_member_starts=starts, conditional_outcomes_total=0,
                ega_out_events=outs, vram_write_operands=writes, dirty_flag_images=[f'{x:04X}' for x in DIRTY_FLAGS], cpu_words=14, ram_bytes=SIZE,
                ram_masked_bytes=0, ega_state_bytes=len(PythonEga().packed()), flag_masks=0)


def main():
    p = argparse.ArgumentParser()
    for flag in ('--compile-only', '--unicorn', '--native'):
        p.add_argument(flag, action='store_true')
    p.add_argument('--output', type=Path); a = p.parse_args()
    with tempfile.TemporaryDirectory(prefix='r0709-0bf5-') as tmp:
        f, lib = library(Path(tmp))
        if a.compile_only:
            print('CHECKER_BUILD_OK'); return
        image, sites = Q.load_image()
        body, decoded = source(image, sites)
        ownership_report = ownership(image)
        image_reloc = Q.relocate(image, sites, BASE)
        report = dict(target=dict(span='0709:[0BF5,0C3F)', bytes=len(body), instructions=len(decoded), sha256=DIGEST, image_sha256=Q.IMAGE_DIGEST, load_base=f'{BASE:04X}'),
                      ownership=ownership_report)
        if a.unicorn:
            report['unicorn'] = differential(image_reloc, decoded, f, lib)
        if a.native:
            import res0709_msgbox_0bf5_native
            report['native'] = res0709_msgbox_0bf5_native.native(image, sites, decoded, f, lib, sys.modules[__name__])
    if a.output:
        a.output.write_text(json.dumps(report, sort_keys=True, indent=2) + '\n')
    print(json.dumps(report, sort_keys=True))


if __name__ == '__main__':
    main()
