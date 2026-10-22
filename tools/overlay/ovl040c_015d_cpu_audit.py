#!/usr/bin/env python3
"""OVL_040C_015D (GAME.OVR unit 40Ch tile placement) against the original bytes, as a composed closure: the body and every
callee's original code (0464:00FF with Dos.Intr, SYSTEM Write 0919 and Writeln 084A, 0899:0059 ReadKey with its Crt,
08AE and SYSTEM members, 0899:0000 QUIT, 0464:08D4, SYSTEM Move 1685 and 0464:1951).

The loader-expanded START.EXE is loaded at a test base with the loader's relocation table applied, and the unit's code is
loaded at its own segment with its 25 segment fixups applied; no byte is patched. The declared environment is
RES_0899_0059's (INT 16h script, INT 21h 25h/35h/2Ch, ports 40h/43h/61h/C0h, the INT 10h stub) plus real-byte Text
driver stubs as Output's InOutFunc and FlushFunc, and the sequencer ports 3C4h/3C5h traced with no EGA attached (A000 is
RAM, as the bus has it without an EGA). Unicorn: CPU14 + full 1 MiB + stop + the ordered environment event log, unmasked.
Media and generated programs stay under ignored resources/."""
import argparse, ctypes, json, random, struct, subprocess, sys, tempfile
from hashlib import sha256
from pathlib import Path
from capstone import Cs, CS_ARCH_X86, CS_MODE_16
from res0709_ega_reset_0000_cpu_audit import Cpu, ROOT, SIZE, REGISTERS, physical, put16, check, Env as PortEnv, OUT_FN, IN_FN
from overlay_inventory import find_units, parse_mz
import res0899_quit_0000_cpu_audit as Q
import res0899_readkey_0059_cpu_audit as K
import res0464_compose_08d4_cpu_audit as C8
import res0464_blit_1951_cpu_audit as B
import system_text_write_cpu_audit as T

CHECKER_ROWS = ('OVL_040C_015D',)
ENTRY, END = 0x015D, 0x02AD
BASE, SEG, SS, STUB_SEG, PSP, CRT = Q.BASE, Q.SEG, Q.SS, Q.STUB_SEG, K.PSP, K.CRT
OVSEG = 0x5000
TILES, MOVE_OBJ, COMP_OBJ, AUX, ROWS = 0x3000, (0x0010, 0x4800), (0x0200, 0x4400), (0x0040, 0x4C00), 0x8000
IN_STUB, FLUSH_STUB = 0x0040, 0x0060
OUTPUT = 0x8608
RETURN_TO = K.RETURN_TO
HALT = Q.HALT
FLAGS = (0x0002, 0x00D7, 0x08D7, 0x0402, 0x0893, 0x0003)
SOURCES = sorted(set(K.SOURCES) | {'system_text_write_cpu', 'resident_0464_video_mode_cpu', 'resident_0464_compose_08d4_cpu',
                                   'resident_0464_blit_1951_cpu', 'overlay_u40c_tile_015d_cpu', 'memory_bus', 'ega_hw'})
STATUS = {0: 'RETURN', 1: 'ARGUMENT', 2: 'CALLEE', 3: 'TRANSFER', 4: 'CONTRACT', 5: 'EXIT'}
CALL_SITES = [(0x01C5, '0x464, 0xff'), (0x01D7, '0xaf8, 0x919'), (0x01DC, '0xaf8, 0x84a'), (0x01E1, '0x899, 0x59'),
              (0x01E9, '0x899, 0'), (0x0249, '0x464, 0x8d4'), (0x028B, '0xaf8, 0x1685'), (0x02A2, '0x464, 0x1951')]
UNREACHABLE_OUTCOMES = {(0x01BE, 0x01EE): 'AX is non-zero and below 11Fh only inside the five bands, which were all taken before'}
SEGS = {v for v in K.SEGS.values()} | {0x0464}


class VideoEnv(ctypes.Structure):
    _fields_ = [('video', Q.VIDEO_FN), ('context', ctypes.c_void_p)]


class Env15(ctypes.Structure):
    _fields_ = [('video', VideoEnv), ('text', T.TextEnv), ('key', K.KeyEnv), ('max_loopbacks', ctypes.c_size_t)]


class Diag15(ctypes.Structure):
    _fields_ = [('site', ctypes.c_uint16), ('status', ctypes.c_int), ('service', ctypes.c_int), ('calls', ctypes.c_uint),
                ('key', K.KeyDiag), ('quit', Q.QuitDiag), ('compose', C8.Diagnostic), ('blit', B.Diagnostic)]


class Bus(ctypes.Structure):
    _fields_ = [('ram', ctypes.POINTER(ctypes.c_uint8)), ('ega', ctypes.c_void_p), ('ports', ctypes.POINTER(PortEnv)),
                ('on_vram_write', C8.WRITE_FN), ('ctx', ctypes.c_void_p)]


def unit_code():
    exe = (ROOT / 'resources/DOS/POOLRAD/START.EXE').read_bytes(); ovr = (ROOT / 'resources/DOS/POOLRAD/GAME.OVR').read_bytes()
    u = {u['stub_para']: u for u in find_units(exe[parse_mz(exe)['header_bytes']:], ovr)}[0x40C]
    code = ovr[u['ovr_offset']:u['ovr_offset'] + u['code_size']]
    check(sha256(code).hexdigest() == u['code_sha256'] and len(code) == 690, 'unit 40Ch code identity')
    check([e for e in u['entries']] == [0x2AD, 0x3B, 0x15D] and ENTRY in u['entries'], 'unit 40Ch entries')
    return code, list(u['fixups'])


def source():
    code, fixups = unit_code()
    body = code[ENTRY:END]
    ins = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(body, ENTRY))
    check(len(ins) == 127 and sum(i.size for i in ins) == len(body), 'decode covers the span')
    check(ins[-1].mnemonic == 'retf' and ins[-1].op_str == '0xa', 'single RETF 0Ah at the end')
    check([(i.address, i.op_str) for i in ins if i.mnemonic == 'lcall'] == CALL_SITES, 'far calls')
    check(all(i.address + 3 in fixups for i in ins if i.mnemonic == 'lcall'), 'every far-call segment word is a fixup')
    check(not [i for i in ins if i.mnemonic in ('call', 'ret', 'int', 'out', 'in')], 'no near call, RET, INT or port access')
    return code, fixups, body, {i.address: i for i in ins}


def library(directory):
    path = directory / 'ovl40c015d.so'
    subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Wpedantic', '-Werror', '-O2', '-fPIC', '-shared', '-I', str(ROOT / 'include')] +
                   [str(ROOT / f'src/recovered/{s}.c') for s in SOURCES] + ['-o', str(path)], check=True)
    f = ctypes.CDLL(str(path)).pr_ovl040c_015d_cpu
    f.argtypes = [ctypes.POINTER(Bus), ctypes.c_size_t, ctypes.POINTER(Cpu), ctypes.POINTER(Env15), ctypes.POINTER(Diag15)]
    f.restype = ctypes.c_int
    return f


def build(image_reloc, code, fixups, *, number=0x10, flag=1, byte6=0x01, x=3, y=2, mode=0, null=(), garbage_ptr=False,
          tile=0x20, rows=2, sw=2, dw=40, keys=(0x1E61,), cur=1, saved=0, vmode=0x03, brk=0, tail=b' X', bufsize=0x80,
          r_io=0, r_flush=0, res=0, ss=SS, sp=0xF000, flags=2, cpu=None, fill=0x5A, base=BASE, ovseg=OVSEG, seed=1):
    rng = random.Random(seed)
    m = bytearray([fill]) * SIZE
    b = base * 16; m[b:b + len(image_reloc)] = image_reloc
    unit = bytearray(code)
    for f in fixups:
        struct.pack_into('<H', unit, f, (struct.unpack_from('<H', unit, f)[0] + base) & 0xFFFF)
    o = ovseg * 16; m[o:o + len(unit)] = unit
    ds = base + SEG['data']
    # the five band tile sets (and the stale-[BP-1] entry for the fill byte), each: rows, width, tile size, aux far pointer, data
    for band in list(range(5)) + [fill]:
        seg = TILES + 0x100 * (band % 8)
        ptr = (0, 0) if band in null or (band == fill and not garbage_ptr) else (0x0100, seg)
        put16(m, ds, 0x5E3A + 4 * band, ptr[0]); put16(m, ds, 0x5E3C + 4 * band, ptr[1])
        if ptr[1]:
            put16(m, seg, 0x0100, rows); put16(m, seg, 0x0102, sw); put16(m, seg, 0x0111, tile)
            put16(m, seg, 0x0113, AUX[0]); put16(m, seg, 0x0115, AUX[1])
            for k in range(0x700):
                m[physical(seg, 0x0117 + k)] = (k * 29 + 7 + band) & 255
    put16(m, ds, 0x5E4E, MOVE_OBJ[0]); put16(m, ds, 0x5E50, MOVE_OBJ[1])
    put16(m, MOVE_OBJ[1], MOVE_OBJ[0], rows); put16(m, MOVE_OBJ[1], MOVE_OBJ[0] + 2, sw)
    put16(m, ds, 0x4A20, COMP_OBJ[0]); put16(m, ds, 0x4A22, COMP_OBJ[1]); put16(m, COMP_OBJ[1], COMP_OBJ[0] + 2, dw)
    for k in range(0x600):
        m[physical(AUX[1], AUX[0] + k)] = (k * 53 + 0x91) & 255
    m[physical(ds, 0x4A1E)] = mode & 255
    m[physical(ds, 0x70C7)] = 0x33
    for r in range(0xA8):                       # 08D4 metadata rows: dirty bytes and three extent words
        m[physical(ds, 0x4A28 + r)] = 0
        put16(m, ds, 0x4AD0 + 2 * r, 0xEEEE)
        put16(m, ds, 0x4C20 + 2 * r, rng.choice((0x0000, 0xFFFF, rng.randrange(0x10000))))
        put16(m, ds, 0x4D70 + 2 * r, rng.choice((0x0000, 0xFFFF, rng.randrange(0x10000))))
    for r in range(64):                         # 1951 row tables: 4EC0/51E0 a segment per row, 5050 an A000 offset per row
        put16(m, ds, 0x4EC0 + 2 * r, ROWS + r * 0x10); put16(m, ds, 0x51E0 + 2 * r, ROWS + 0x400 + r * 0x10)
        put16(m, ds, 0x5050 + 2 * r, (r * 40) & 0xFFFF)
    for p, v in enumerate((1, 2, 4, 8)):
        m[physical(ds, 0x25F6 + p)] = v
    # the error branch: Output's TextRec (buffer at +80h, drivers in the stub segment), InOutRes, ReadKey/QUIT state
    for off, v in ((0, 5), (2, 0xD7B2), (4, bufsize), (6, 0), (8, 0), (0x0A, 0), (0x0C, OUTPUT + 0x80), (0x0E, ds),
                   (0x10, 0), (0x12, 0), (0x14, IN_STUB), (0x16, STUB_SEG), (0x18, FLUSH_STUB), (0x1A, STUB_SEG)):
        put16(m, ds, OUTPUT + off, v)
    put16(m, ds, 0x4426, res)
    s0, s1 = T.stub(r_io), T.stub(r_flush)
    at = physical(STUB_SEG, IN_STUB); m[at:at + len(s0)] = s0
    at = physical(STUB_SEG, FLUSH_STUB); m[at:at + len(s1)] = s1
    m[physical(ds, 0x5372)] = cur; m[physical(ds, 0x5373)] = saved
    put16(m, ds, 0x5374, 1); put16(m, ds, 0x2612, 0); m[physical(ds, 0x4A1C)] = vmode
    m[physical(ds, 0x49F7)] = 0; m[physical(ds, 0x8501)] = 0; m[physical(ds, 0x8502)] = brk
    put16(m, ds, 0x4422, PSP)
    m[physical(PSP, 0x80)] = len(tail)
    for k, ch in enumerate(tail + b'\r'):
        m[physical(PSP, 0x81 + k)] = ch
    at = physical(STUB_SEG, 0); m[at:at + len(Q.INT10_STUB)] = Q.INT10_STUB
    put16(m, 0, 0x40, 0x0000); put16(m, 0, 0x42, STUB_SEG)
    m[physical(0x0040, 0x0049)] = 0x5A
    for k, v in enumerate((RETURN_TO[1], RETURN_TO[0], byte6, flag, number, x & 0xFFFF, y & 0xFFFF)):
        put16(m, ss, (sp + 2 * k) & 0xFFFF, v)
    r = cpu or (0x1234, 0x3456, 0x5678, 0x789A, 0x9ABC, 0xBCDE, 0x7777)
    c = Cpu(*r, sp, ovseg, ENTRY, ds, 0x1111, ss, flags)
    env = dict(table={8: (0x0123, 0xF000)}, in61=0xB0, time_start=4736147, time_step=7)
    stubs = {(STUB_SEG, IN_STUB): r_io, (STUB_SEG, FLUSH_STUB): r_flush}
    return m, c, env, list(keys), stubs


def run_unicorn(name, m, c, envspec, keys, body_ips, base=BASE, body=None, extra=()):
    """body: (segment, entry, end) of the routine whose starts and edges are recorded (default: 015D at CS)."""
    from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE, UC_HOOK_INTR, UC_HOOK_INSN
    from unicorn import x86_const as R
    env = Q.Env(**envspec); script = list(keys)
    uc = Uc(UC_ARCH_X86, UC_MODE_16); backing = (ctypes.c_uint8 * SIZE).from_buffer_copy(m)
    uc.mem_map_ptr(0, SIZE, 7, ctypes.addressof(backing)); uc.mem_map_ptr(SIZE, SIZE, 7, ctypes.addressof(backing))
    for (n, _), reg in zip(Cpu._fields_, REGISTERS):
        uc.reg_write(reg, getattr(c, n))
    s = dict(stop=None, steps=0, prev=None); seen = set(); edges = set()
    ovseg, entry, end = body or (c.cs, ENTRY, END)
    known = {base + v for v in SEGS} | {STUB_SEG, ovseg, c.cs} | set(extra)

    def code(em, address, size, user):
        cs = uc.reg_read(R.UC_X86_REG_CS); ip = uc.reg_read(R.UC_X86_REG_IP)
        if s['prev'] is not None and cs == ovseg:
            edges.add((s['prev'], ip))
        s['prev'] = None
        if (cs, ip) == (base + SEG['system'], 0x00E9):
            s['stop'] = 'EXIT'; return uc.emu_stop()
        if (cs, ip) == (base + CRT, 0x014B):
            s['stop'] = 'BREAK'; return uc.emu_stop()
        if (cs, ip) == RETURN_TO:
            s['stop'] = 'RETURN'; return uc.emu_stop()
        if cs not in known:
            s['stop'] = 'TRANSFER'; return uc.emu_stop()
        if bytes(uc.mem_read(address, 2)) == b'\xcd\x3f':      # an unloaded overlay stub: stop before the overlay manager
            s['stop'] = 'OVERLAY'; return uc.emu_stop()
        if cs == ovseg and entry <= ip < end:
            check(ip in body_ips, f'escape inside the body {name} {ip:04x}'); seen.add(ip)
            if body_ips[ip].mnemonic.startswith('j') and body_ips[ip].mnemonic != 'jmp':
                s['prev'] = ip
        s['steps'] += 1
        if s['steps'] > 3000000:
            s['stop'] = 'RUNAWAY'; uc.emu_stop()

    def intr(em, intno, user):
        ax = uc.reg_read(R.UC_X86_REG_AX); ah, al = ax >> 8, ax & 0xFF
        if intno == 0x16:
            check(ah in (0x00, 0x01), f'ENV refusal {name}: INT 16h AH {ah:#x}')
            fl = uc.reg_read(R.UC_X86_REG_EFLAGS)
            if ah == 0x01:
                env.log.append(('16', 1, script[0] if script else None))
                if script:
                    uc.reg_write(R.UC_X86_REG_AX, script[0]); uc.reg_write(R.UC_X86_REG_EFLAGS, fl & ~0x40)
                else:
                    uc.reg_write(R.UC_X86_REG_EFLAGS, fl | 0x40)
                return
            check(bool(script), f'ENV refusal {name}: INT 16h AH 00h with the script empty')
            k = script.pop(0); env.log.append(('16', 0, k)); uc.reg_write(R.UC_X86_REG_AX, k); return
        check(intno == 0x21, f'ENV refusal {name}: INT {intno:#x}')
        if ah == 0x2C:
            cx, dx = env.gettime(); uc.reg_write(R.UC_X86_REG_CX, cx); uc.reg_write(R.UC_X86_REG_DX, dx); return
        check(ah in (0x25, 0x35), f'ENV refusal {name}: INT 21h AH {ah:#x}')
        sp = uc.reg_read(R.UC_X86_REG_SP); ss_ = uc.reg_read(R.UC_X86_REG_SS)
        fl = uc.reg_read(R.UC_X86_REG_EFLAGS) & 0xFFFF
        for k, v in ((2, fl), (4, uc.reg_read(R.UC_X86_REG_CS)), (6, uc.reg_read(R.UC_X86_REG_IP))):
            uc.mem_write(physical(ss_, (sp - k) & 0xFFFF), struct.pack('<H', v))
        out = env.dos(ah, al, dict(ds=uc.reg_read(R.UC_X86_REG_DS), dx=uc.reg_read(R.UC_X86_REG_DX)))
        for k, v in out.items():
            uc.reg_write(dict(es=R.UC_X86_REG_ES, bx=R.UC_X86_REG_BX)[k], v)

    def port_in(em, port, size, user):
        check(port == 0x61 and size == 1, f'ENV refusal {name}: IN {port:#x}')
        env.log.append(('in', port, env.in61)); return env.in61

    def port_out(em, port, size, value, user):
        check(port in (0x40, 0x43, 0x61, 0xC0, 0x3C4, 0x3C5), f'ENV refusal {name}: OUT {port:#x}')
        env.log.append(('out', port, value))
    uc.hook_add(UC_HOOK_CODE, code); uc.hook_add(UC_HOOK_INTR, intr)
    uc.hook_add(UC_HOOK_INSN, port_in, None, 1, 0, R.UC_X86_INS_IN)
    uc.hook_add(UC_HOOK_INSN, port_out, None, 1, 0, R.UC_X86_INS_OUT)
    uc.emu_start(physical(c.cs, c.ip), 2 * SIZE, count=3500000)
    check(s['stop'] in ('RETURN', 'EXIT', 'BREAK', 'TRANSFER', 'OVERLAY'), f'original stop {name}: {s["stop"]}')
    return uc, backing, env, s['stop'], seen, edges


def run_c(f, m, c, envspec, keys, stubs, refuse_text=False, max_loopbacks=1 << 20):
    keep = []
    q, env = Q.c_env(envspec, keep)
    ke = K.KeyEnv(q, K.keyboard(env, keys, keep), None, 64)
    text = T.c_driver(stubs, refuse_text, keep)

    def pout(ctx, port, value, size, cs, ip):
        env.log.append(('out', port, value)); return 0 if port in (0x3C4, 0x3C5) else 1

    def pin(ctx, port, size, cs, ip, value):
        return 1
    fo, fi = OUT_FN(pout), IN_FN(pin); keep += [fo, fi]
    ports = PortEnv(None, fo, fi)
    e = Env15(VideoEnv(q.video, None), text, ke, max_loopbacks)
    actual = Cpu.from_buffer_copy(c); ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(m); d = Diag15()
    bus = Bus(ram, None, ctypes.pointer(ports), C8.WRITE_FN(), None)
    status = STATUS.get(f(ctypes.byref(bus), SIZE, ctypes.byref(actual), ctypes.byref(e), ctypes.byref(d)))
    return status, actual, bytes(ram), env, d


def compare(name, state, body_ips, f):
    m, c, envspec, keys, stubs = state
    uc, backing, uenv, ustop, seen, edges = run_unicorn(name, m, c, envspec, keys, body_ips)
    status, actual, ram, cenv, d = run_c(f, m, c, envspec, keys, stubs)
    want = 'EXIT' if ustop == 'BREAK' else ustop
    check(status == want, f'C status {name}: {status} original {ustop} (site {d.site:04x} status {d.status})')
    if ustop == 'BREAK':
        check(d.site == 0x01E1 and d.status == 3, f'BREAK reported through ReadKey {name}')
    check(cenv.log == uenv.log, f'environment events {name}: C {cenv.log[:6]} original {uenv.log[:6]}')
    if ram != bytes(backing):
        diff = [hex(i) for i, (a, b) in enumerate(zip(ram, bytes(backing))) if a != b]
        raise ValueError(f'RAM {name}: {diff[:8]} ({len(diff)} bytes)')
    for (n, _), reg in zip(Cpu._fields_, REGISTERS):
        w = uc.reg_read(reg) & 0xFFFF
        check(getattr(actual, n) == w, f'CPU {name}/{n} C {getattr(actual, n):04x} original {w:04x}')
    return ustop, seen, edges, d


BANDS = ((0x0001, 0), (0x002D, 0), (0x002E, 1), (0x0073, 1), (0x0074, 2), (0x00B9, 2), (0x00BA, 3), (0x00FF, 3), (0x0100, 4), (0x011E, 4))
K_ = K.K


def cases():
    out = []
    for number, band in BANDS:
        for flag in (1, 0):
            out.append((f'n{number:x}-f{flag}', dict(number=number, flag=flag)))
    for mode in (0, 1, 2, 3):
        out.append((f'move-mode{mode}', dict(number=0x30, flag=0, mode=mode)))
    for mode in (0, 1, 2):
        out.append((f'comp-mode{mode}', dict(number=0x75, flag=1, mode=mode, byte6=0)))
    for band in range(5):
        out.append((f'null{band}', dict(number=BANDS[2 * band][0], null=(band,), flag=band & 1)))
    out += [('stale-null-8000', dict(number=0x8000)), ('stale-null-ffff', dict(number=0xFFFF, flag=0)),
            ('stale-move', dict(number=0x8000, flag=0, garbage_ptr=True)), ('stale-comp', dict(number=0xC000, flag=1, garbage_ptr=True)),
            ('flag-high-byte', dict(number=0x40, flag=0x0100)), ('neg-xy-move', dict(number=0x50, flag=0, x=0xFFFE, y=0x0081)),
            ('neg-xy-comp', dict(number=0x50, flag=1, x=0x00FF, y=0xFF80)), ('wide-tile', dict(number=0x12, flag=0, tile=0x60, rows=3, sw=4))]
    for number in (0x0000, 0x011F, 0x7FFF):
        out.append((f'err{number:x}', dict(number=number)))
    out += [('err-sting-03', dict(number=0, keys=(K_['c'] & 0xFF00 | 0x03,), tail=b' STING')),
            ('err-sound', dict(number=0x200, keys=(0x1F13,), cur=1)), ('err-sound-on', dict(number=0x200, keys=(0x1F13,), cur=2, saved=0)),
            ('err-extended', dict(number=0x7FFF, keys=(K_['f1'],))), ('err-drain', dict(number=0x1000, keys=(K_['a'], K_['s'], K_['d']))),
            ('err-break', dict(number=0x0000, brk=1)), ('err-smallbuf', dict(number=0x0000, bufsize=3)),
            ('err-io-result', dict(number=0x0000, bufsize=2, r_io=0x65)), ('err-flush-result', dict(number=0x0000, r_flush=0x66)),
            ('err-inoutres', dict(number=0x0000, res=0x67)), ('err-vmode', dict(number=0x0000, vmode=0x0D))]
    out += [(f'flags{f:x}', dict(number=0x33, flag=k & 1, flags=f)) for k, f in enumerate(FLAGS)]
    out += [(f'err-flags{f:x}', dict(number=0x0000, flags=f)) for f in FLAGS[1:3]]
    out += [('ss-ds', dict(number=0x22, flag=0, ss='ds', sp=0xE000)), ('ovseg-high', dict(number=0x101, flag=1, ovseg=0x9800)),
            ('sp-odd', dict(number=0x55, flag=0, sp=0xEFFF))]
    rng = random.Random(0x40C015D)
    for k in range(40):
        err = rng.random() < 0.2
        number = rng.choice((0, 0x11F, 0x7FFF)) if err else rng.choice([n for n, _ in BANDS] + [rng.randrange(1, 0x11F) for _ in range(4)])
        out.append((f'random{k}', dict(number=number, flag=rng.choice((0, 1, 0x0101)), byte6=rng.randrange(256), mode=rng.choice((0, 0, 1, 2, 3)),
                                       x=rng.randrange(0, 12), y=rng.randrange(0, 12), tile=rng.choice((0x10, 0x20, 0x40)),
                                       rows=rng.randrange(1, 4), sw=rng.randrange(1, 5), null=tuple(b for b in range(5) if rng.random() < 0.1),
                                       keys=(rng.choice((K_['a'], K_['x'], K_['zero'] | 0x3B00)),), flags=rng.choice(FLAGS),
                                       cpu=tuple(rng.randrange(65536) for _ in range(7)), fill=rng.choice((0x5A, 0x00, 0xFF, rng.randrange(256))),
                                       seed=k)))
    return out


def differential(image_reloc, code, fixups, body_ips, f):
    stops, seen, edges, calls = {}, set(), set(), [0] * 8
    for name, o in cases():
        o = dict(o)
        if o.get('ss') == 'ds':
            o['ss'] = BASE + SEG['data']
        state = build(image_reloc, code, fixups, **o)
        ustop, s, e, d = compare(name, state, body_ips, f)
        stops[ustop] = stops.get(ustop, 0) + 1; seen |= s; edges |= e
        for k, (site, _) in enumerate(CALL_SITES):
            calls[k] += site in s
    conds = {a: (int(i.op_str, 16), a + i.size) for a, i in body_ips.items() if i.mnemonic.startswith('j') and i.mnemonic != 'jmp'}
    outcomes = {(a, t) for a, ts in conds.items() for t in ts}
    missing_starts = sorted(set(body_ips) - seen)
    missing = sorted(outcomes - edges - set(UNREACHABLE_OUTCOMES))
    check(not missing_starts, f'body starts not reached: {[hex(a) for a in missing_starts]}')
    check(not missing, f'outcomes not reached: {[(hex(a), hex(t)) for a, t in missing]}')
    check(not (edges & set(UNREACHABLE_OUTCOMES)), 'a declared-unreachable outcome was reached')
    return dict(cases=len(cases()), stops=stops, starts=f'{len(seen)}/{len(body_ips)}',
                outcomes=f'{len(edges)}/{len(outcomes)} ({len(UNREACHABLE_OUTCOMES)} unreachable declared)',
                call_sites_reached={f'{site:04X}': n for (site, _), n in zip(CALL_SITES, calls)}, cpu_words=14, ram_bytes=SIZE, flag_masks=0)


def main():
    p = argparse.ArgumentParser(description=__doc__)
    for flag in ('--compile-only', '--unicorn', '--native'):
        p.add_argument(flag, action='store_true')
    p.add_argument('--output', type=Path); a = p.parse_args()
    with tempfile.TemporaryDirectory() as tmp:
        f = library(Path(tmp))
        if a.compile_only:
            print('CHECKER_BUILD_OK'); return
        image, sites = Q.load_image()
        image_reloc = Q.relocate(image, sites, BASE)
        report = dict(image_sha256=Q.IMAGE_DIGEST, load_base=f'{BASE:04X}', relocations=len(sites))
        code, fixups, body, body_ips = source()
        report.update(body=dict(span=f'unit 40Ch +[{ENTRY:04X},{END:04X})', bytes=len(body), instructions=len(body_ips),
                                sha256=sha256(body).hexdigest(), fixups=len(fixups)))
        if a.unicorn:
            report['unicorn'] = differential(image_reloc, code, fixups, body_ips, f)
        if a.native:
            import ovl040c_015d_cpu_native as N
            report['native'] = N.native(sys.modules[__name__], code, fixups, body_ips, f)
    text = json.dumps(report, indent=1, sort_keys=True)
    if a.output:
        a.output.parent.mkdir(parents=True, exist_ok=True); a.output.write_text(text + '\n')
    print(json.dumps(report, sort_keys=True))


if __name__ == '__main__':
    main()
