#!/usr/bin/env python3
"""RES_00BA_16EC (START.EXE resident 00BA [16EC,17E3), the window message) against the original bytes, as a composed closure: the body, SYSTEM
StrStore 0B25, 03F1:0137 (clear), the near 1816 (display), 0709:0768 (printer) with 0709:0BF5 (prompt) behind its CALL 0BF5, 03F1:0000 (box)
through overlay stub 0428:002A into OVL_040C_015D and its whole closure, 0709:0DBE (CRT Delay) and the near 17E3 (clear).

The world joins the tile world of ovl040c_015d_cpu_audit (the loader-expanded START.EXE at a test base with its relocations, unit 40Ch at its own
segment with its fixups, the tile sets, the 08D4/1951 tables, Output's TextRec, ReadKey/QUIT state) with the display state of the 0BF5/0768
worlds (pages, font, the attribute and mask tables, the text cursor, gate, speed and Delay calibration) and the independent Python EGA on the
A0000..AFFFF aperture. Declared environment, as in those audits: INT 16h from a per-case key script, INT 21h 25h/35h/2Ch, ports 40h/43h/61h/C0h,
QUIT's INT 10h stub, the EGA ports. Compared after every case, unmasked: CPU14, ALL 1,048,576 RAM bytes, the full EGA state, the ordered EGA OUT
events, the ordered VRAM write operands, the ordered environment events, the stop, and the remaining budget (one unit per instruction of THIS body).
FLAGS are exact except where the last FLAGS writer is a non-zero shift or a word DIV/MUL inside a callee: there the declared pinned split applies
(tools/overlay/pinned_shift_flags.py; counted). Every case's world is built from its own name (PM i22687). Media stay under ignored resources/."""
import argparse, ctypes, json, random, struct, subprocess, sys, tempfile, time
from hashlib import sha256
from pathlib import Path
from capstone import Cs, CS_ARCH_X86, CS_MODE_16
from pinned_shift_flags import LastFlagWriter, declared_flags_split, split_used, SPLIT_BY_RULE
from res0709_ega_reset_0000_cpu_audit import Cpu, ROOT, SIZE, REGISTERS, physical, put16, check
import ovl040c_015d_cpu_audit as O
import res03f1_box_0000_cpu_audit as BX
import res0709_msgbox_0bf5_cpu_audit as M
import res0709_printer_0768_cpu_audit as P
import res00ba_display_1693_cpu_audit as D1693
import res00ba_display_1816_cpu_audit as D1816
import res00ba_clear_17e3_cpu_audit as C17E3
import res0709_ega_reset_0000_cpu_audit as E0
import res0709_glyph_01df_cpu_audit as G

Q, K, R = O.Q, O.K, M.R
CHECKER_ROWS = ('RES_00BA_16EC',)
SEG16EC, ENTRY, END = 0x00BA, 0x16EC, 0x17E3
BASE, OVSEG, SS = O.BASE, O.OVSEG, O.SS
STUBSEG, STUB_ENTRY = BX.STUBSEG, BX.STUB_ENTRY
RETURN_TO = O.RETURN_TO
SP_MIN, SP_MAX = 0x0800, 0xFFF0
BUDGET = 1000
MSG = (0x8800, 0x0100)            # the message (a Pascal string) and the 1816 string, outside every other world segment
TITLE = (0x8800, 0x0400)
FONT_SEG, FONT_OFF = M.FONT_SEG, 0x1000
STATUS = {0: 'RETURNED', 1: 'ARGUMENT', 2: 'LIMIT', 3: 'CODE', 4: 'TRANSFER', 5: 'CALLEE', 6: 'OVERLAY', 7: 'EXIT', 8: 'KEY'}
FAR_SITES = [(0x1702, 0xAF8, 0x0B25), (0x171B, 0x3F1, 0x0137), (0x1750, 0x709, 0x0768), (0x177B, 0x3F1, 0x0000), (0x178E, 0x3F1, 0x0137),
             (0x17C9, 0x709, 0x0768), (0x17D4, 0x709, 0x0DBE)]
NEAR_SITES = [(0x1731, 0x1816), (0x17A8, 0x1816), (0x17DA, 0x17E3)]
SOURCES = list(dict.fromkeys(['resident_00ba_window_16ec_cpu', 'system_strstore_fullstate_0b25', 'memory_bus', 'ega_hw'] + list(BX.SOURCES) +
                             list(P.SOURCES) + list(D1693.SOURCES) + list(D1816.SOURCES) + list(C17E3.SOURCES) + list(R.SOURCES) +
                             ['resident_0709_msgbox_0bf5_cpu', 'resident_0709_space_05eb_cpu', 'resident_0709_printer_0626_cpu', 'resident_0709_glyph_01df_cpu',
                              'system_strstore_fullstate_0b25', 'system_runtime_leaves_cpu', 'system_string_leaves_cpu']))


class Env16(ctypes.Structure):
    _fields_ = [('clear_loopbacks', ctypes.c_size_t), ('clear_rect_loopbacks', ctypes.c_size_t),
                ('display_printer_loopbacks', ctypes.c_size_t), ('display_glyph_loopbacks', ctypes.c_size_t),
                ('printer', P.Budget), ('prompt', M.Env0BF5), ('prompt_rounds', ctypes.c_uint), ('box', BX.Env03),
                ('delay_loopbacks', ctypes.c_uint64), ('clear17e3_loopbacks', ctypes.c_size_t), ('clear17e3_rect_loopbacks', ctypes.c_size_t)]


class Diag16(ctypes.Structure):
    _fields_ = [('site', ctypes.c_uint16), ('callee', ctypes.c_int), ('callee_status', ctypes.c_int), ('prompt_rounds', ctypes.c_uint),
                ('printer', P.Diag), ('prompt', M.Diag0BF5), ('box', BX.Diag03)]


def source(image, sites):
    b = SEG16EC * 16
    body = bytes(image[b + ENTRY:b + END])
    ins = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(body, ENTRY))
    check(len(body) == 247 and len(ins) == 116 and sum(i.size for i in ins) == 247, 'decode covers [16EC,17E3): 247 bytes / 116 instructions')
    check([(i.address, int(i.op_str.split(',')[0], 16), int(i.op_str.split(',')[1], 16)) for i in ins if i.mnemonic == 'lcall'] == FAR_SITES, 'the seven far calls')
    check([(i.address, int(i.op_str, 16)) for i in ins if i.mnemonic == 'call'] == NEAR_SITES, 'the three near calls (each after PUSH CS)')
    check([(i.address, i.op_str) for i in ins if i.mnemonic in ('ret', 'retf')] == [(0x17E0, '0xc')], 'the only return is RETF 0Ch at 17E0')
    check(sorted(s - b for s in sites if b + ENTRY <= s < b + END) == [a + 3 for a, _, _ in FAR_SITES], 'the seven far-call segment words are the only relocations')
    check(not [i for i in ins if i.mnemonic in ('int', 'in', 'out')], 'no interrupt or port instruction')
    text_code = bytes(image[0x0709 * 16 + 0x06A7:0x0709 * 16 + 0x0714])
    text_ins = {i.address: i for i in Cs(CS_ARCH_X86, CS_MODE_16).disasm(text_code, 0x06A7)}
    check(text_ins[0x06B5].mnemonic == 'cmp' and text_ins[0x06B5].op_str.replace(' ', '') == 'al,byteptrss:[di-0x104]', '06A7 compares byte index against byte end')
    check(text_ins[0x06BA].mnemonic == 'ja' and int(text_ins[0x06BA].op_str, 16) == 0x0710, '06A7 exits only for unsigned index>end')
    increments = [i.address for i in text_ins.values() if i.mnemonic == 'inc' and i.op_str.replace(' ', '') == 'byteptrss:[di-0x103]']
    check(len(increments) == 1, '06A7 increments the index as a byte')
    check(text_ins[0x070E].mnemonic == 'jmp' and int(text_ins[0x070E].op_str, 16) == 0x06AA, '06A7 loop-back')
    return body, {i.address: i for i in ins}, dict(span='00BA:[16EC,17E3)', bytes=247, instructions=116, sha256=sha256(body).hexdigest(), image_sha256=Q.IMAGE_DIGEST,
        text_boundary=dict(body_sha256=sha256(text_code).hexdigest(), compare='0709:06B5', unsigned_exit='0709:06BA', byte_increment=increments[0], loop_back='0709:070E', index_bits=8,
                           reason='For unchanged end=255 no unsigned byte index is greater than end; increment wraps modulo256.'))


def library(directory, override=None):
    path = Path(directory) / ('r16ec_mut.so' if override else 'r16ec.so')
    subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Wpedantic', '-Werror', '-O2', '-fPIC', '-shared', '-I', str(ROOT / 'include')] +
                   [str((override or {}).get(s, ROOT / f'src/recovered/{s}.c')) for s in SOURCES] + ['-o', str(path)], check=True, capture_output=True)
    lib = ctypes.CDLL(str(path))
    f = lib.pr_res_00ba_window_16ec_cpu
    f.argtypes = [ctypes.POINTER(M.Bus), ctypes.c_size_t, ctypes.POINTER(Cpu), ctypes.POINTER(ctypes.c_size_t), ctypes.POINTER(Env16), ctypes.POINTER(Diag16)]
    f.restype = ctypes.c_int
    lib.pr_ega_reset.argtypes = [ctypes.POINTER(M.Ega)]
    return f, lib


def build(name, image_reloc, code, fixups, *, mode5=True, flag6dda=0, wait=0, row=3, msg=b'Hello', title=b'Ti', words=None, keys=(0x1E61,),
          loaded=True, sp=0xF000, flags=0x0202, regs=None, gmode=1, speed=0, calibration=2, cursor=(1, 1), gate=0, msg_at=MSG, title_at=TITLE,
          writes=(), budget=BUDGET, high=0xA500, text_loopbacks=65536):
    """The tile world, the display state, then this case's mode byte, DS:6DDA, message, frame and registers, all from the case's name."""
    rng = random.Random(f'RES_00BA_16EC/{name}')
    m, c, envspec, kl, stubs = O.build(image_reloc, code, fixups, keys=keys, mode=gmode, sp=sp, flags=flags, seed=rng.randrange(1 << 30))
    ds = BASE + Q.SEG['data']
    at = physical(BASE + STUBSEG, STUB_ENTRY)
    m[at:at + 5] = (bytes([0xEA]) + struct.pack('<HH', 0x015D, OVSEG)) if loaded else BX.STUB_UNLOADED
    put16(m, ds, 0x4A16, 0xA000); put16(m, ds, 0x4A18, 0xA200)                             # the display state of the 0BF5/0768 worlds
    put16(m, ds, 0x5E20, FONT_OFF); put16(m, ds, 0x5E22, FONT_SEG)
    for j in range(2048):
        m[physical(FONT_SEG, FONT_OFF + j)] = ((j * 37 + (j >> 3) * 13) ^ 0xA6) & 255
    for j, v in enumerate((1, 2, 4, 8)):
        m[physical(ds, 0x25C9 + j)] = v
    for j, v in enumerate((0, 0x5555, 0xAAAA, 0xFFFF)):
        put16(m, ds, 0x25CD + 2 * j, v)
    for j in range(256):
        m[physical(ds, 0x25D5 + j)] = j % 4
    for j in range(8):
        put16(m, ds, 0x25E5 + 2 * j, 0xC000 >> (2 * j))
    m[physical(ds, 0x5E24)], m[physical(ds, 0x5E25)] = cursor                                 # the 0768 world's cursor, gate, speed, Delay calibration
    m[physical(ds, 0x4A00)] = gate; m[physical(ds, 0x49E2)] = speed; put16(m, ds, 0x84FC, calibration); m[0x400] = 0x42
    m[physical(ds, 0x49F3)] = 5 if mode5 else rng.choice((3, 4, 0, 6))                        # 16EC's own DS reads
    m[physical(ds, 0x6DDA)] = flag6dda
    for (seg, off), s in ((msg_at, msg), (title_at, title)):
        if s is not None:
            for j, v in enumerate(bytes([len(s) & 0xFF]) + s):
                m[physical(seg, (off + j) & 0xFFFF)] = v
    w = words or (title_at[1], title_at[0])
    frame = (RETURN_TO[1], RETURN_TO[0], high | wait, high | row, msg_at[1], msg_at[0], w[0], w[1])
    for k, v in enumerate(frame):
        put16(m, SS, (sp + 2 * k) & 0xFFFF, v)
    for seg, off, blob in writes:
        for j, v in enumerate(blob):
            m[physical(seg, (off + j) & 0xFFFF)] = v
    r = regs or tuple(rng.randrange(65536) for _ in range(7))
    c = Cpu(r[0], r[1], r[2], r[3], r[4], r[5], r[6], sp, BASE + SEG16EC, ENTRY, ds, 0x1111, SS, (flags | 0x0002) & ~0x0100)
    return m, c, envspec, kl, stubs, dict(name=name, budget=budget, sp=sp, mode5=mode5, wait=wait, row=row, msg=msg, plain=not writes, text_loopbacks=text_loopbacks)


def run_unicorn(name, m, c, envspec, keys, body_ips, budget, text_loopbacks=65536):
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
    s = dict(stop=None, steps=0, left=budget, prev=None, text_loops=0); seen = set(); edges = set(); ega_events = []; writer = LastFlagWriter()
    here = BASE + SEG16EC; sysc = BASE + Q.SEG['system']; crt = BASE + O.CRT

    def code(em, address, size, user):
        cs = uc.reg_read(RR.UC_X86_REG_CS); ip = uc.reg_read(RR.UC_X86_REG_IP)
        writer.finish(uc)
        if (cs, ip) == (BASE + 0x0709, 0x06A7):
            s['text_loops'] = 0
        if (cs, ip) == (BASE + 0x0709, 0x070E):
            if s['text_loops'] == text_loopbacks:
                s['stop'] = 'LIMIT'; return uc.emu_stop()
            s['text_loops'] += 1
        if (cs, ip) == (sysc, 0x00E9):
            s['stop'] = 'EXIT'; return uc.emu_stop()
        if (cs, ip) == (crt, 0x014B):
            s['stop'] = 'BREAK'; return uc.emu_stop()
        if (cs, ip) == RETURN_TO:
            s['stop'] = 'RETURNED'; return uc.emu_stop()
        if not (BASE <= cs < BASE + 0x10C0 or cs in (OVSEG, O.STUB_SEG)):
            s['stop'] = 'TRANSFER'; return uc.emu_stop()
        if bytes(uc.mem_read(address, 2)) == b'\xcd\x3f':
            s['stop'] = 'OVERLAY'; return uc.emu_stop()
        if cs == here and ENTRY <= ip < END:
            check(ip in body_ips, f'escape inside 16EC {name} {ip:04x}')
            if s['left'] == 0:
                s['stop'] = 'LIMIT'; return uc.emu_stop()
            s['left'] -= 1; seen.add(ip)
            if s['prev'] is not None:
                edges.add((s['prev'], ip))
            s['prev'] = ip
        s['steps'] += 1
        if s['steps'] > 6000000:
            s['stop'] = 'RUNAWAY'; return uc.emu_stop()
        writer.step(uc, address)

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
        if port in M.EGA_PORTS:
            ega_events.append((port, value, size, uc.reg_read(RR.UC_X86_REG_CS), uc.reg_read(RR.UC_X86_REG_IP))); device.out(port, value, size); return
        check(port in (0x40, 0x43, 0x61, 0xC0) and size == 1, f'ENV refusal {name}: OUT {port:#x}')
        env.log.append(('out', port, value))
    uc.hook_add(UC_HOOK_CODE, code); uc.hook_add(UC_HOOK_INTR, intr)
    uc.hook_add(UC_HOOK_INSN, port_in, None, 1, 0, RR.UC_X86_INS_IN)
    uc.hook_add(UC_HOOK_INSN, port_out, None, 1, 0, RR.UC_X86_INS_OUT)
    uc.emu_start(physical(c.cs, c.ip), 2 * SIZE, count=7000000)
    writer.finish(uc)
    check(s['stop'] in ('RETURNED', 'EXIT', 'BREAK', 'TRANSFER', 'LIMIT', 'OVERLAY'), f'original stop {name}: {s["stop"]}, at {uc.reg_read(RR.UC_X86_REG_CS):04x}:{uc.reg_read(RR.UC_X86_REG_IP):04x}, steps {s["steps"]}, last environment {env.log[-3:]}')
    return uc, backing, env, s['stop'], seen, edges, device, ega_events, s['left'], writer


def run_c(f, lib, m, c, envspec, keys, stubs, budget, text_loopbacks=65536):
    keep = []
    q, env = Q.c_env(envspec, keep)
    ke = K.KeyEnv(q, K.keyboard(env, keys, keep), None, 64)
    text = O.T.c_driver(stubs, False, keep)
    box = BX.Env03(O.Env15(O.VideoEnv(q.video, None), text, ke, 1 << 20), 1 << 20)
    e = Env16(65536, 65536, 65536, 65536, P.Budget(65536, 65536, text_loopbacks, 65536, 10 ** 8, 65536, 65536, 65536), M.Env0BF5(ke, 65536, 65536, 65536), 64,
              box, 10 ** 8, 65536, 65536)
    actual = Cpu.from_buffer_copy(c); ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(m)
    ega = M.Ega(); lib.pr_ega_reset(ctypes.byref(ega))
    for p in range(4):
        ctypes.memset(ctypes.addressof(ega.plane[p]), 0x31 + p * 0x27, 65536)
    events, writes = [], []

    def out(ctx, port, value, size, cs, ip):
        events.append((port, value, size, cs, ip)); return 0

    def write(ctx, off, value):
        writes.append((off, value))
    ports = E0.Env(None, E0.OUT_FN(out), E0.IN_FN()); keep.append(ports)
    bus = M.Bus(ram, ctypes.pointer(ega), ctypes.pointer(ports), G.WRITE_FN(write), None)
    d = Diag16(); left = ctypes.c_size_t(budget)
    status = STATUS[f(ctypes.byref(bus), SIZE, ctypes.byref(actual), ctypes.byref(left), ctypes.byref(e), ctypes.byref(d))]
    return status, actual, bytes(ram), env, d, events, writes, bytes(ega), left.value


def compare(state, body_ips, f, lib):
    m, c, envspec, keys, stubs, info = state
    name = info['name']
    uc, backing, uenv, ustop, seen, edges, device, uega, uleft, writer = run_unicorn(name, m, c, envspec, keys, body_ips, info['budget'], info['text_loopbacks'])
    status, actual, ram, cenv, d, cega, cwrites, cstate, cleft = run_c(f, lib, m, c, envspec, keys, stubs, info['budget'], info['text_loopbacks'])
    want = {'BREAK': 'KEY', 'EXIT': ('EXIT', 'KEY')}.get(ustop, ustop)
    check(status == want or (isinstance(want, tuple) and status in want), f'{name}: C {status} (callee {d.callee}/{d.callee_status} site {d.site:04x}) original {ustop}')
    check(cleft == uleft, f'{name}: budget left C {cleft} original {uleft}')
    check(cenv.log == uenv.log, f'{name}: environment events C {cenv.log[:6]} original {uenv.log[:6]}')
    check(cega == uega, f'{name}: EGA OUT events C {len(cega)} original {len(uega)}')
    check(cwrites == device.writes, f'{name}: VRAM write operands {len(cwrites)}/{len(device.writes)}')
    check(cstate == device.packed(), f'{name}: EGA state')
    if ram != bytes(backing):
        diff = [hex(i) for i, (a, b) in enumerate(zip(ram, bytes(backing))) if a != b]
        raise ValueError(f'{name}: RAM {diff[:8]} ({len(diff)} bytes)')
    for (n, _), reg in zip(Cpu._fields_, REGISTERS):
        want_v = uc.reg_read(reg) & 0xFFFF
        if n == 'flags':
            problem = declared_flags_split(actual.flags, want_v, writer)
            check(problem is None, f'{name}: {problem}'); split_used(actual.flags, want_v, writer); continue
        check(getattr(actual, n) == want_v, f'{name}: CPU {n} C {getattr(actual, n):04x} original {want_v:04x}')
    if ustop == 'RETURNED':
        check((actual.cs, actual.ip, actual.sp, actual.bp) == (RETURN_TO[0], RETURN_TO[1], (c.sp + 0x10) & 0xFFFF, c.bp),
              f'{name}: independent RETF 0Ch frame rule')
    if name == 'boundary-254':
        check(ustop == status == 'RETURNED', '254-byte boundary must return')
    if name == 'boundary-255':
        check(ustop == status == 'LIMIT' and actual.cs == BASE + 0x0709 and actual.ip == 0x070E,
              '255-byte boundary must stop before06A7 loop-back070E')
    return ustop, seen, edges, ram, d


def statement(state, stop, ram):
    """Without the C model, on plain RETURNED cases: the local copy of the message (StrStore, at most FFh) at [BP-100h], the colour byte at
    [BP-101h] on the second branch, with the return-frame rule checked separately in compare()."""
    m, c, envspec, keys, stubs, info = state
    if stop != 'RETURNED' or not info['plain'] or info['msg'] is None:
        return 0
    bp = (info['sp'] - 2) & 0xFFFF
    s = info['msg'][:255]
    got = bytes(ram[physical(SS, (bp - 0x100 + j) & 0xFFFF)] for j in range(1 + len(s)))
    check(got == bytes([len(s)]) + s, f'{info["name"]}: the local copy at [BP-100h] is {got[:8].hex()}..., rule {(bytes([len(s)]) + s)[:8].hex()}...')
    if not info['mode5']:
        col = ram[physical(SS, (bp - 0x101) & 0xFFFF)]
        want = 0x12 if m[physical(BASE + Q.SEG['data'], 0x6DDA)] else 0x11
        check(col == want, f'{info["name"]}: colour byte {col:02x}, rule {want:02x}')
    return 1


def cases(quick=False):
    out = []

    def add(name, **kw):
        out.append((name, kw))
    for mode5 in (True, False):
        for f6 in ((0, 1) if not mode5 else (0,)):
            for wait in (0, 1):
                tag = f'{"m5" if mode5 else "m3"}-f{f6}-w{wait}'
                add(f'{tag}', mode5=mode5, flag6dda=f6, wait=wait)
                for row in (0, 1, 7, 0x10):
                    add(f'{tag}-row{row:02x}', mode5=mode5, flag6dda=f6, wait=wait, row=row)
                for msg, label in ((b'', 'empty'), (b'A', 'one'), (b'x' * 38, 'line'), (b'word ' * 20, 'wrap'), (b'y' * 255, 'max')):
                    add(f'{tag}-{label}', mode5=mode5, flag6dda=f6, wait=wait, msg=msg, text_loopbacks=256 if label == 'max' else 65536)
    add('boundary-254', msg=b'y' * 254, text_loopbacks=256)
    add('boundary-255', msg=b'y' * 255, text_loopbacks=256)
    for gm in (0, 2, 3):
        add(f'gmode{gm}-m5', gmode=gm); add(f'gmode{gm}-box', mode5=False, gmode=gm)
    add('box-unloaded-stub', mode5=False, loaded=False)
    add('m5-speed2-wait', wait=1, speed=2)
    for fl in (0x0002, 0x0202, 0x0ED7, 0x08D7, 0x0003, 0x0A87):
        add(f'flags-{fl:04x}-m5', flags=fl); add(f'flags-{fl:04x}-box', flags=fl, mode5=False)
    for sp in (SP_MIN, SP_MIN + 1, 0x7FFF, 0xF001, SP_MAX - 1, SP_MAX):
        add(f'sp-{sp:04x}', sp=sp); add(f'sp-{sp:04x}-box', sp=sp, mode5=False)
    for k in (-0x100, -0xF0, -0x02, 0x04):                                   # the message inside the frame: StrStore reads it while writing [BP-100h]
        add(f'msg-in-frame{k:+04x}', msg=None, msg_at=(SS, (0xF000 - 2 + k) & 0xFFFF))
    for b in range(0, 130, 1 if not quick else 4):
        add(f'budget-m5-{b}', budget=b); add(f'budget-box-{b}', budget=b, mode5=False, wait=1)
    for k in range(6 if quick else 30):
        rng = random.Random(f'RES_00BA_16EC/random{k}')
        add(f'random{k}', mode5=rng.random() < 0.5, flag6dda=rng.choice((0, 1)), wait=rng.choice((0, 0, 1)), row=rng.randrange(0x14),
            msg=bytes(rng.randrange(0x20, 0x7F) for _ in range(rng.choice((0, 3, 20, 60)))), gmode=rng.choice((0, 1, 2, 3)),
            flags=rng.randrange(65536) & 0x0ED7, sp=rng.randrange(SP_MIN, SP_MAX + 1), regs=tuple(rng.randrange(65536) for _ in range(7)))
    return out


def differential(image_reloc, code, fixups, body_ips, f, lib, quick=False, only=None, keep_going=False):
    t0 = time.time(); stops = {}; stated = 0; seen = set(); edges = set(); failures = []
    for k in SPLIT_BY_RULE:
        SPLIT_BY_RULE[k] = 0
    cs = [c for c in cases(quick) if only is None or c[0] in only]
    check(bool(cs), 'no cases selected')
    if only is not None:
        check(set(only) <= {name for name, _ in cs}, 'unknown case names')
    for index, (name, kw) in enumerate(cs):
        if index % 10 == 0:
            print(f'[{index + 1}/{len(cs)}] {name}: START', file=sys.stderr, flush=True)
        state = build(name, image_reloc, code, fixups, **kw)
        try:
            stop, s, e, ram, d = compare(state, body_ips, f, lib)
            stated += statement(state, stop, ram)
        except (ValueError, AssertionError) as error:
            if not keep_going:
                raise
            failures.append(dict(case=name, error=str(error)))
            print(f'FAIL {name}: {error}', file=sys.stderr, flush=True)
            continue
        stops[stop] = stops.get(stop, 0) + 1; seen |= s; edges |= e
    conds = {a: (int(i.op_str, 16), a + i.size) for a, i in body_ips.items() if i.mnemonic.startswith('j') and i.mnemonic != 'jmp'}
    outcomes = {(a, t) for a, ts in conds.items() for t in ts}
    check(bool(failures) or only is not None or seen == set(body_ips), f'starts not reached {sorted(hex(a) for a in set(body_ips) - seen)}')
    check(bool(failures) or only is not None or outcomes <= edges, f'outcomes not reached {sorted(outcomes - edges)}')
    return dict(cases=len(cs), compared=len(cs)-len(failures), failures=failures, stops=dict(sorted(stops.items())), starts=f'{len(seen & set(body_ips))}/{len(body_ips)}', outcomes=f'{len(outcomes & edges)}/{len(outcomes)}',
                independent_statements=stated, cpu_words=14, ram_bytes=SIZE, flag_masks=0, declared_splits=dict(SPLIT_BY_RULE), seconds=round(time.time() - t0, 1))


MUTANTS = (
    ('mode selector 5 becomes 4', 'rd8(ram, c->ds, 0x49F3u), 0x05u', 'rd8(ram, c->ds, 0x49F3u), 0x04u'),
    ('colour 11 becomes 10', 'c->bp + (-257)), 0x11u)', 'c->bp + (-257)), 0x10u)'),
    ('message copy cap zero', 'c->ax = 0x00FFu;', 'c->ax = 0x0000u;'),
    ('wait branch always skips', 'if ((c->flags & 0x0040u)) { c->ip = 0x17DDu;', 'if (1) { c->ip = 0x17DDu;'),
    ('return drops two argument bytes', 'c->sp + 0x0Cu', 'c->sp + 0x0Au'),
    ('instruction budget unchanged', '--*budget; } while (0)', '(void)budget; } while (0)'),
)
MUTATION_CASES = ('m5-f0-w0', 'm5-f0-w1', 'm3-f0-w0', 'm3-f1-w0', 'budget-m5-0', 'budget-m5-3', 'budget-m5-11', 'budget-m5-12')


def mutations(image_reloc, code, fixups, body_ips, f, lib, directory):
    # A compiler failure or a failing baseline never counts as a killed mutation.
    baseline = differential(image_reloc, code, fixups, body_ips, f, lib, only=MUTATION_CASES)
    print(f'Mutation baseline PASS: {baseline["compared"]} cases', file=sys.stderr, flush=True)
    baseline_path = Path(directory) / 'mutation-baseline.json'
    baseline_path.write_text(json.dumps(baseline, sort_keys=True, indent=2) + '\n')
    original = (ROOT / 'src/recovered/resident_00ba_window_16ec_cpu.c').read_text()
    results = []
    for index, (name, old, new) in enumerate(MUTANTS):
        check(original.count(old) == 1, f'mutation must change exactly one site: {name}')
        work = Path(directory) / f'mutant-{index}'; work.mkdir(exist_ok=True)
        candidate = work / 'candidate.c'; candidate.write_text(original.replace(old, new))
        try:
            mf, ml = library(work, {'resident_00ba_window_16ec_cpu': candidate})
        except subprocess.CalledProcessError as error:
            results.append(dict(name=name, verdict='COMPILE-ERROR', reason=error.stderr.decode(errors='replace')))
            continue
        try:
            differential(image_reloc, code, fixups, body_ips, mf, ml, only=MUTATION_CASES)
        except (ValueError, AssertionError) as error:
            results.append(dict(name=name, verdict='CAUGHT', reason=str(error)))
        else:
            results.append(dict(name=name, verdict='SURVIVED'))
        print(f'Mutation {name}: {results[-1]["verdict"]}', file=sys.stderr, flush=True)
    return dict(baseline=baseline, results=results, total=len(results), caught=sum(r['verdict']=='CAUGHT' for r in results))


def main():
    p = argparse.ArgumentParser(description=__doc__)
    for flag in ('--compile-only', '--unicorn', '--quick', '--mutate', '--native', '--keep-going'):
        p.add_argument(flag, action='store_true')
    p.add_argument('--only', nargs='+'); p.add_argument('--output', type=Path); a = p.parse_args()
    if a.native:
        p.error('native 16EC oracle is not implemented; no native parity claim')
    if not (a.compile_only or a.unicorn or a.mutate):
        p.error('select --compile-only, --unicorn, or --mutate')
    with tempfile.TemporaryDirectory(prefix='r16ec-') as tmp:
        f, lib = library(tmp)
        if a.compile_only:
            print('CHECKER_BUILD_OK'); return
        image, sites = Q.load_image()
        body, body_ips, meta = source(image, sites)
        image_reloc = Q.relocate(image, sites, BASE)
        code, fixups, _, _ = O.source()
        report = dict(target=meta)
        if a.unicorn:
            report['unicorn'] = differential(image_reloc, code, fixups, body_ips, f, lib, a.quick, a.only, a.keep_going)
        if a.mutate:
            mutation_dir = a.output.parent / (a.output.stem + '-artifacts') if a.output else Path(tmp) / 'mutations'
            mutation_dir.mkdir(parents=True, exist_ok=True)
            report['mutations'] = mutations(image_reloc, code, fixups, body_ips, f, lib, mutation_dir)
    if a.output:
        a.output.parent.mkdir(parents=True, exist_ok=True); a.output.write_text(json.dumps(report, sort_keys=True, indent=2) + '\n')
    print(json.dumps(report, sort_keys=True))
    if report.get('unicorn', {}).get('failures') or (a.mutate and report['mutations']['caught'] != report['mutations']['total']):
        raise SystemExit(1)


if __name__ == '__main__':
    sys.modules['res00ba_window_16ec_cpu_audit'] = sys.modules[__name__]
    main()
