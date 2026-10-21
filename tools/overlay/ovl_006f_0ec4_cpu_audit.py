#!/usr/bin/env python3
"""OVL_006F_0EC4 (unit 0x6F [0EC4,1262), the monster-record loader) against the original bytes, as a composed closure (VP i20392/i20394).

Every callee runs as its ORIGINAL bytes in the oracle and as its merged C model in pr_u6f_entry6_0ec4_cpu: Str 1366, StrLoad 0B0B, StrConcat 0B98,
Move 1685, GetMem 023F, FreeMem 0254, the block loader 0802:0716 with its whole cluster and its QUIT, the message 0709:0BF5 with ReadKey, QUIT
0899:0000, the idle 0899:0104, and the roll through the stub 00B0:0048 into unit 0x9E +0DE3 with SYSTEM Random.

The whole loader-expanded START.EXE image is loaded at a test base with the loader's relocation table; the unit-0x6F span and the unit-0x9E span are
taken from GAME.OVR, hash-checked, and relocated by the unit's own lcall_segment fixup table at their own load segments. The ONE substitution: the
INT 3Fh overlay-manager dispatch at 00B0:0048 is not executed; its five stub bytes are replaced, identically in both runs, by a far JMP (EAh) to
the loaded unit 0x9E +0DE3. The A000 aperture is an independent Python EGA device on an MMIO window of the oracle; every OUT is forwarded to it.
Declared environment, identical in the oracle and the C callbacks: the 0716 composed audit's DOS file service (here routed by name over three
archives, one per name constant read from the bytes), its keyboard contract (AH=01h peeks type-ahead only; waited keys from a per-case script), the
logging INT 10h stub, ReadKey's services and the EGA ports. Compared, unmasked: CPU14, ALL 1,048,576 RAM bytes, the file-service state and its
ordered log, the ordered environment log, the full EGA state, the ordered EGA OUT events and VRAM write operands, and the end state. Also pinned:
the call sequence and every argument frame the body hands each callee, against an independent Python statement; the final record, chains and rolls
against an independent statement (the LCG, the class rule, the chain walk). Media stay under ignored resources/."""
import argparse, ctypes, json, random, struct, subprocess, sys, tempfile, time
from hashlib import sha256
from pathlib import Path
from capstone import Cs, CS_ARCH_X86, CS_MODE_16
import res0802_0716_composed_audit as M
import res0802_0716_cpu_audit as A716
import res0802_0040_cpu_audit as B
import res0802_0193_cpu_audit as P
import res0709_msgbox_0bf5_cpu_audit as B5
import res0709_glyph_01df_cpu_audit as G
import res0709_ega_reset_0000_cpu_audit as E0
import res0899_readkey_0059_cpu_audit as R
import res0899_quit_0000_cpu_audit as Q
import u33_grid_pair_0d13_audit as DICE
from overlay_inventory import find_units, parse_mz
from u86_procvars_3413_audit import EXE_SHA, OVR_SHA
from res0709_ega_reset_0000_cpu_audit import Cpu, ROOT, SIZE, REGISTERS, physical, put16, check
from res00ba_display_1693_cpu_audit import PythonEga

CHECKER_ROWS = ('OVL_006F_0EC4',)
ENTRY, END, RETF = 0x0EC4, 0x1262, 0x125F
DIGEST = '1160665fef6397a88b922ce115836e5cf4b55f09b5c4db04e765e8a02f08f747'
BASE, STUB_SEG, LOG_SEG, SCRATCH, PSP = Q.BASE, Q.STUB_SEG, B.LOG_SEG, P.SCRATCH, R.PSP
SEG = Q.SEG
CS_OV, DICE_CS, REC_SEG, REC_OFF, FONT_SEG = 0x6400, 0x6C00, 0x7800, 0x0100, 0x7400
RET_TO = (0x9001, 0x7000)
STUB_AT = (BASE + 0xB0, 0x48)
SHADOW = (0x11C0, 0x90)
DIRTY_FLAGS = (0x0202, 0x0602, 0x0A87, 0x0402, 0x0E57)       # the standing dirty-incoming-flags set (VP i20371)
STATUS = {0: 'RETURN', 1: 'ARGUMENT', 2: 'CODE', 3: 'EXIT', 4: 'RUNERROR', 5: 'DIVIDE', 6: 'BREAK', 7: 'STALLED', 8: 'LIMIT', 9: 'SERVICE',
          10: 'CALLEE', 11: 'TRANSFER'}
NON_ARCHITECTURAL = ('ARGUMENT', 'CODE', 'LIMIT', 'SERVICE', 'CALLEE', 'TRANSFER')
SYS = BASE + SEG['system']
KINDS = {'str': 0x1366, 'strload': 0x0B0B, 'strcat': 0x0B98, 'move': 0x1685, 'getmem': 0x023F, 'freemem': 0x0254}
CALLS = {  # the 32 far calls of the span: site -> (kind, entry)
    0x0EE0: ('str', 0x1366), 0x0EF0: ('strload', 0x0B0B), 0x0EFA: ('strcat', 0x0B98), 0x0F04: ('strcat', 0x0B98), 0x0F14: ('strload', 0x0B0B),
    0x0F27: ('load', 0x0716), 0x0F3F: ('strload', 0x0B0B), 0x0F44: ('msg', 0x0BF5), 0x0F49: ('quit', 0x0000), 0x0F5C: ('move', 0x1685),
    0x0FEB: ('roll', 0x0048), 0x1038: ('roll', 0x0048), 0x1076: ('freemem', 0x0254), 0x108B: ('strload', 0x0B0B), 0x1095: ('strcat', 0x0B98),
    0x109F: ('strcat', 0x0B98), 0x10AF: ('strload', 0x0B0B), 0x10C2: ('load', 0x0716), 0x10D9: ('getmem', 0x023F), 0x110F: ('move', 0x1685),
    0x1131: ('getmem', 0x023F), 0x1163: ('freemem', 0x0254), 0x1173: ('strload', 0x0B0B), 0x117D: ('strcat', 0x0B98), 0x1187: ('strcat', 0x0B98),
    0x1197: ('strload', 0x0B0B), 0x11AA: ('load', 0x0716), 0x11C1: ('getmem', 0x023F), 0x11F9: ('move', 0x1685), 0x121B: ('getmem', 0x023F),
    0x124D: ('freemem', 0x0254), 0x1257: ('idle', 0x0104)}
CONST = {0x0EF0: 0x0E98, 0x0F04: 0x0E9C, 0x0F14: 0x0EA0, 0x0F3F: 0x0EA5, 0x108B: 0x0E98, 0x109F: 0x0EBC, 0x10AF: 0x0EA0, 0x1173: 0x0E98,
         0x1187: 0x0EC0, 0x1197: 0x0EA0}      # the string constants each call's source pointer names (code offsets)
SOURCES = list(dict.fromkeys(list(M.SOURCES) + list(R.SOURCES) + [
    'resident_0709_msgbox_0bf5_cpu', 'resident_0709_space_05eb_cpu', 'resident_0709_printer_0626_cpu', 'resident_0709_glyph_01df_cpu',
    'system_strstore_fullstate_0b25', 'system_runtime_leaves_cpu', 'memory_bus', 'ega_hw', 'overlay_u6f_entry6_0ec4_cpu', 'system_freemem_0254_cpu',
    'system_string_move_leaves_cpu', 'resident_0899_idle_0104_cpu', 'overlay_u9e_entry8_fullstate_compose_0de3', 'overlay_u9e_entry8_fullstate_prefix_0de3',
    'overlay_u9e_entry8_fullstate_loop_0e05', 'overlay_u9e_entry8_fullstate_postcall_0e13', 'overlay_u9e_entry8_fullstate_tail_0e1f',
    'overlay_u9e_entry8_system_call_0e0e', 'system_random_fullstate_11c0', 'system_seed_fullstate_120f']))


class Env0EC4(ctypes.Structure):
    _fields_ = [('load', M.Env), ('msg', B5.Env0BF5), ('heap_steps', ctypes.c_uint), ('roll_calls', ctypes.c_uint)]


class Diag0EC4(ctypes.Structure):
    _fields_ = [('site', ctypes.c_uint16), ('callee', ctypes.c_int), ('callee_status', ctypes.c_int), ('calls', ctypes.c_uint), ('rolls', ctypes.c_uint),
                ('load', M.Diag), ('msg', B5.Diag0BF5), ('quit', Q.QuitDiag)]


# ---------------------------------------------------------------------------------------------------- the span, from GAME.OVR
def source():
    exe = (ROOT / 'resources/DOS/POOLRAD/START.EXE').read_bytes(); ovr = (ROOT / 'resources/DOS/POOLRAD/GAME.OVR').read_bytes()
    check(sha256(exe).hexdigest() == EXE_SHA and sha256(ovr).hexdigest() == OVR_SHA, 'media provenance')
    units = {u['stub_para']: u for u in find_units(exe[parse_mz(exe)['header_bytes']:], ovr)}
    u6, u9 = units[0x6F], units[0x9E]
    at = u6['ovr_offset'] + ENTRY
    body = ovr[at:at + END - ENTRY]
    check(len(body) == 926 and sha256(body).hexdigest() == DIGEST, 'unit 0x6F +0EC4 identity')
    ins = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(body, ENTRY))
    check(len(ins) == 341 and sum(i.size for i in ins) == len(body), 'decode covers the span')
    check([(i.address, i.op_str) for i in ins if i.mnemonic == 'lcall'] == [(a, f'{s:#x}, {e:#x}' if e else f'{s:#x}, 0') for a, s, e in sorted(
        (a, {'str': 0xAF8, 'strload': 0xAF8, 'strcat': 0xAF8, 'move': 0xAF8, 'getmem': 0xAF8, 'freemem': 0xAF8, 'load': 0x802, 'msg': 0x709,
             'quit': 0x899, 'idle': 0x899, 'roll': 0xB0}[k], e) for a, (k, e) in CALLS.items())], 'the 32 far calls')
    cond = [i for i in ins if i.mnemonic.startswith('j') and i.mnemonic != 'jmp']
    check(len(cond) == 16 and sorted(i.mnemonic for i in cond) == ['jae'] * 2 + ['jb'] * 2 + ['je'] * 5 + ['jne'] * 7, '16 conditional branches')
    check(sum(1 for i in ins if i.mnemonic == 'jmp') == 5 and [i.address for i in ins if i.mnemonic == 'retf'] == [RETF], '5 jmp and one RETF 6')
    check(not [i for i in ins if i.mnemonic in ('int', 'in', 'out', 'call') or i.mnemonic == 'ljmp'], 'no interrupt, port or near call')
    fix = sorted(f for f in u6['fixups'] if ENTRY <= f < END)
    check(fix == sorted(a + 3 for a in CALLS), 'the 32 segment words are exactly the relocation-backed words')
    check(units[0x9E]['memory_segment'] == 0xB0 and units[0x9E]['memory_segment_sequential_ok'] and u9['entries'][8] == 0x0DE3, 'unit 0x9E stub segment and entry 8')
    check(0x20 + 5 * 8 == 0x48 and struct.unpack_from('<H', body, 0xFEE - ENTRY)[0] == 0xB0 and struct.unpack_from('<H', body, 0x103B - ENTRY)[0] == 0xB0,
          'the roll calls name stub entry 8 of segment 0B0h')
    consts = ovr[u6['ovr_offset'] + 0xE98:u6['ovr_offset'] + ENTRY]
    check(len(consts) == 0x2C and not [f for f in u6['fixups'] if 0xE98 <= f < ENTRY], 'the name constants are plain data, no relocation')
    names = {}
    for key, off in (('c0e98', 0xE98), ('c0e9c', 0xE9C), ('c0ea0', 0xEA0), ('c0ea5', 0xEA5), ('c0ebc', 0xEBC), ('c0ec0', 0xEC0)):
        s = ovr[u6['ovr_offset'] + off:u6['ovr_offset'] + off + 1 + ovr[u6['ovr_offset'] + off]]
        names[key] = bytes(s[1:])
    lo, hi = DICE.U9E_SPAN
    dice = ovr[u9['ovr_offset'] + lo:u9['ovr_offset'] + hi]
    check(sha256(dice).hexdigest() == DICE.U9E_SHA, 'unit 0x9E +0DE3 identity')
    u9_fix = sorted(f for f in u9['fixups'] if lo <= f < hi)
    return dict(consts=consts, consts_sha=sha256(consts).hexdigest(), body=body, decoded={i.address: i for i in ins}, cond=cond, fix=fix, dice=dice, dice_lo=lo, u9_fix=u9_fix, names=names, file_start=at,
                u6_fixups=sorted(u6['fixups']))


def library(directory, override=None):
    path = directory / ('u6f_0ec4_mut.so' if override else 'u6f_0ec4.so')
    subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Wpedantic', '-Werror', '-O2', '-fPIC', '-shared', '-I', str(ROOT / 'include')] +
                   [str(override if override and s == 'overlay_u6f_entry6_0ec4_cpu' else ROOT / f'src/recovered/{s}.c') for s in SOURCES] + ['-o', str(path)], check=True)
    lib = ctypes.CDLL(str(path))
    f = lib.pr_u6f_entry6_0ec4_cpu
    f.argtypes = [ctypes.POINTER(B5.Bus), ctypes.c_size_t, ctypes.POINTER(Cpu), ctypes.POINTER(Env0EC4), ctypes.POINTER(Diag0EC4)]
    f.restype = ctypes.c_int
    lib.pr_ega_reset.argtypes = [ctypes.POINTER(B5.Ega)]
    return f, lib


# ---------------------------------------------------------------------------------------------------- the archives
class Routed(A716.FileSystem):
    """The 0716 audit's DOS file service with one archive per name constant (a name containing the constant's text selects it; None = the file is absent)."""
    def __init__(self, blobs, suffixes, **kw):
        super().__init__(None, **kw)
        self.blobs, self.suffixes, self.hblob = blobs, suffixes, {}

    def call(self, ah, al, bx, cx, dx, read_name=None, write=None):
        if ah == 0x3D:
            key = next((k for k, s in self.suffixes.items() if s in (read_name or '').upper()), None)
            self.blob = self.blobs.get(key)
            r = super().call(ah, al, bx, cx, dx, read_name, write)
            if r[0] == 0:
                self.hblob[r[1]] = self.blob
            return r
        if ah in (0x3F, 0x42) and bx in self.hblob:
            self.blob = self.hblob[bx]
        return super().call(ah, al, bx, cx, dx, read_name, write)


def pack_block(bid, raw):
    return None if raw is None else A716.archive([(bid, raw)])


def record(rng, cls, n=0x11D):
    raw = bytearray(rng.randrange(1, 256) for _ in range(n))
    if n > 0x2E:
        raw[0x2E] = cls
    return bytes(raw)


def build(ctx, reloc, stub, o):
    """Memory and CPU for one case: the 0716-composed substrate (image, DS, heap, INT 10h stub, file state), then the unit spans and the stub."""
    blobs = {k: pack_block(o.get('id', 7), o.get(k)) if o.get(k) is not None else (A716.archive([(0xEE, b'xx')]) if o.get(k + '_idmiss') else None)
             for k in ('b1', 'b2', 'b3')}
    sp = o.get('sp', 0xF000); ss = o.get('ss', Q.SS)
    L = {**dict(base=BASE, heap=A716.HEAP, cs_ov=CS_OV, dice=DICE_CS, rec=REC_SEG, font=FONT_SEG, ret=RET_TO), **o.get('L', {})}
    base, cs_ov, dice_cs, rec_seg, font_seg, ret = L['base'], L['cs_ov'], L['dice'], L['rec'], L['font'], L['ret']
    m, c, spec = M.build(reloc, stub, dict(blob=None, block_id=0, flag557c=o.get('f557c', 1), heap_paras=o.get('heap_paras', 0x2000), flags=o.get('flags', 2),
                                           sp=sp, regs=o.get('regs'), ss=ss, base=base, heap=L['heap'], ret=ret, ds_name=o.get('ds_name', b'ECL'), first_handle=o.get('first_handle', 5), keys=o.get('keys', (0x1C0D,)), typeahead=o.get('typeahead', ()),
                                           appear_after=o.get('appear_after'), gone_from=o.get('gone_from')))
    ds = base + SEG['data']; ssv = ds if ss == 'ds' else ss
    # the unit spans: relocated by the unit's own fixup table at their load segments
    body = bytearray(ctx['body'])
    for f in ctx['fix']:
        struct.pack_into('<H', body, f - ENTRY, (struct.unpack_from('<H', body, f - ENTRY)[0] + base) & 0xFFFF)
    at = physical(cs_ov, ENTRY); m[at:at + len(body)] = body
    at = physical(cs_ov, 0xE98); m[at:at + len(ctx['consts'])] = ctx['consts']                    # the six name constants the string loads read
    dice = bytearray(ctx['dice'])
    for f in ctx['u9_fix']:
        struct.pack_into('<H', dice, f - ctx['dice_lo'], (struct.unpack_from('<H', dice, f - ctx['dice_lo'])[0] + base) & 0xFFFF)
    at = physical(dice_cs, ctx['dice_lo']); m[at:at + len(dice)] = dice
    # DECLARED: the credited 0DE3 closure model (overlay_u9e_entry8_system_call_0e0e.c) fixes the SYSTEM segment at 0AF8h instead of reading the call's
    # relocated segment word, so with the image at BASE its SYSTEM reads (Random 11C0.., seed 120F.., multiplier 1245) land at 0AF8:xxxx. A byte copy of
    # exactly those SYSTEM bytes sits there in both runs; the oracle executes the relocated original at BASE+0AF8 and never touches the copy.
    src = physical(base + SEG['system'], SHADOW[0]); dst = physical(0x0AF8, SHADOW[0]); m[dst:dst + SHADOW[1]] = m[src:src + SHADOW[1]]
    at = physical(base + 0xB0, 0x48); m[at:at + 5] = struct.pack('<BHH', 0xEA, 0x0DE3, dice_cs)          # the declared trampoline
    # DS: block index digit, flag, RNG seed, ReadKey and glyph state
    m[physical(ds, 0x5376)] = o.get('digit', 3)
    seed = o.get('seed', 0x12345678)
    put16(m, ds, 0x4428, seed & 0xFFFF); put16(m, ds, 0x442A, seed >> 16)
    m[physical(ds, 0x5372)] = o.get('cur', 1); m[physical(ds, 0x5373)] = o.get('saved', 0)
    put16(m, ds, 0x5374, o.get('w', 1)); put16(m, ds, 0x2612, o.get('arg', 0)); m[physical(ds, 0x4A1C)] = o.get('vmode', 3)
    m[physical(ds, 0x49F7)] = o.get('flag49', 0); m[physical(ds, 0x8501)] = o.get('buffered', 0); m[physical(ds, 0x8502)] = o.get('brk', 0)
    put16(m, ds, 0x4422, PSP)
    tail = o.get('tail', b' STING')
    m[physical(PSP, 0x80)] = len(tail)
    for k, ch in enumerate(tail + b'\r'):
        m[physical(PSP, 0x81 + k)] = ch
    m[physical(0x0040, 0x0049)] = 0x5A
    m[physical(ds, 0x4A1E)] = o.get('mode', 1)
    pages = o.get('pages', (0xA000, 0xA200)); put16(m, ds, 0x4A16, pages[0]); put16(m, ds, 0x4A18, pages[1])
    fo = o.get('fontoff', 0x1000); put16(m, ds, 0x5E20, fo); put16(m, ds, 0x5E22, font_seg)
    for j, v in enumerate((1, 2, 4, 8)):
        m[physical(ds, 0x25C9 + j)] = v
    for j, v in enumerate((0, 0x5555, 0xAAAA, 0xFFFF)):
        put16(m, ds, 0x25CD + 2 * j, v)
    for j in range(256):
        m[physical(ds, 0x25D5 + j)] = j % 4
    for j in range(8):
        put16(m, ds, 0x25E5 + 2 * j, 0xC000 >> (2 * j))
    for j in range(2048):
        m[physical(font_seg, (fo + j) & 0xFFFF)] = ((j * 37 + (j >> 3) * 13) ^ 0xA6) & 255
    # the record R and the entry frame: [SP] IP, [SP+2] CS, [SP+4]/[SP+6] R, [SP+0Ah] id word
    rec = o.get('rec', (rec_seg, REC_OFF))
    for j in range(0x240):
        m[physical(rec[0], (rec[1] + j) & 0xFFFF)] = (j * 29 + 0x17) & 0xFF if o.get('rec_junk', True) else 0
    idw = (o.get('hi', 0xAB) << 8) | o.get('id', 7)
    for k, w in enumerate((ret[1], ret[0], rec[1], rec[0], idw)):
        put16(m, ssv, (sp + 2 * k) & 0xFFFF, w)
    c.cs, c.ip = cs_ov, ENTRY
    spec.update(blobs=blobs, ret=ret, L=L, rec=rec, ssv=ssv, sp=sp, base_ip=ENTRY)
    suffixes = {k: ctx['names'][n].decode('latin-1').upper() for k, n in (('b1', 'c0e9c'), ('b2', 'c0ebc'), ('b3', 'c0ec0'))}
    spec['suffixes'] = suffixes
    return m, c, spec


def file_service(spec):
    return Routed(spec['blobs'], spec['suffixes'], first_handle=spec['first_handle'], appear_after=spec['appear_after'], gone_from=spec['gone_from'],
                  seek_fail=spec['seek_fail'])


# ---------------------------------------------------------------------------------------------------- the original
def run_unicorn(name, ctx, m, c, spec, stop_at=None):
    from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE, UC_HOOK_INTR, UC_HOOK_INSN
    from unicorn import x86_const as RR
    env = Q.Env(**M.ENVSPEC); fs = file_service(spec); script = list(spec['keys']); ahead = list(spec['typeahead'])
    device = PythonEga(); ega_events = []
    base = spec['base']; cs0802 = base + 0x0802; body899 = BASE + SEG['body']
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
    s = dict(stop=None, steps=0, arrivals=0, snap=None, after=None, last=None)
    seen, edges, calls = set(), set(), []
    known = {base + v for v in SEG.values()} | {cs0802, STUB_SEG, base + R.CRT, base + 0x0709, CS_OV, DICE_CS, STUB_AT[0]}
    entries = {(SYS, e): k for k, e in KINDS.items()}
    entries.update({(cs0802, 0x0716): 'load', (base + 0x0709, 0x0BF5): 'msg', (body899, 0x0000): 'quit', (body899, 0x0104): 'idle', (DICE_CS, 0x0DE3): 'roll'})
    bp0 = (spec['sp'] - 2) & 0xFFFF

    def cpu_now():
        return Cpu(*[uc.reg_read(r) & 0xFFFF for r in REGISTERS])

    def code(em, address, size, user):
        cs = uc.reg_read(RR.UC_X86_REG_CS); ip = uc.reg_read(RR.UC_X86_REG_IP)
        if (cs, ip) == spec['ret']:
            s['stop'] = 'RETURN'; return uc.emu_stop()
        if (cs, ip) == (SYS, 0x00E2):
            s['stop'] = 'RUNERROR'; return uc.emu_stop()
        if (cs, ip) == (SYS, 0x00E9):
            s['stop'] = 'EXIT'; return uc.emu_stop()
        if (cs, ip) == (base + R.CRT, 0x014B):
            s['stop'] = 'BREAK'; return uc.emu_stop()
        check(cs in known, f'escape {name} {cs:04x}:{ip:04x}')
        if cs == CS_OV:
            check(ENTRY <= ip < END and ip in ctx['decoded'], f'{name}: fetch inside the span off the decode {ip:04x}')
            if s['last'] is not None and s['last'] in ctx['cond_at']:
                edges.add((s['last'], ip))
            seen.add(ip); s['last'] = ip
        else:
            s['last'] = None
        key = entries.get((cs, ip))
        if key is not None:
            spv = uc.reg_read(RR.UC_X86_REG_SP); ssv_ = uc.reg_read(RR.UC_X86_REG_SS)
            words = struct.unpack('<12H', bytes(uc.mem_read(physical(ssv_, spv), 24)))
            if words[1] == CS_OV:
                calls.append((words[0] - 5, key, words, spv, None,
                              tuple(struct.unpack('<HHH', bytes(uc.mem_read(physical(ssv_, (bp0 - 6) & 0xFFFF), 6))))))
        if stop_at is not None and (cs, ip) == (cs0802, 0x051F):
            s['arrivals'] += 1
            if s['arrivals'] == stop_at:
                s['snap'] = (cpu_now(), bytes(backing), bytes(device.packed()), len(device.writes))
            elif s['arrivals'] > stop_at:
                check(bytes(cpu_now()) == bytes(s['snap'][0]), f'{name}: CPU changed at 050F arrival {s["arrivals"]}')
                if s['arrivals'] == stop_at + M.STALL_EXTRA:
                    s['after'] = bytes(backing); s['stop'] = 'STALLED'; return uc.emu_stop()
        s['steps'] += 1
        if s['steps'] > 90000000:
            s['stop'] = 'RUNAWAY'; uc.emu_stop()

    def intr(em, intno, user):
        ax = uc.reg_read(RR.UC_X86_REG_AX); ah, al = ax >> 8, ax & 0xFF; fl = uc.reg_read(RR.UC_X86_REG_EFLAGS)
        if intno == 0:
            s['stop'] = 'DIVIDE'; return uc.emu_stop()
        if intno == 0x16:
            check(ah in (0x00, 0x01), f'ENV refusal {name}: INT 16h AH {ah:#x}')
            if ah == 0x01:
                env.log.append(('16', 1, ahead[0] if ahead else None))
                if ahead:
                    uc.reg_write(RR.UC_X86_REG_AX, ahead[0]); uc.reg_write(RR.UC_X86_REG_EFLAGS, fl & ~0x40)
                else:
                    uc.reg_write(RR.UC_X86_REG_EFLAGS, fl | 0x40)
                return
            src = ahead if ahead else script
            check(bool(src), f'ENV refusal {name}: INT 16h AH 00h with the script empty')
            k = src.pop(0); env.log.append(('16', 0, k)); uc.reg_write(RR.UC_X86_REG_AX, k); return
        check(intno == 0x21, f'ENV refusal {name}: INT {intno:#x}')
        if ah == 0x2C:
            cx, dx = env.gettime(); uc.reg_write(RR.UC_X86_REG_CX, cx); uc.reg_write(RR.UC_X86_REG_DX, dx); return
        if ah in (0x25, 0x35):
            sp = uc.reg_read(RR.UC_X86_REG_SP); ss_ = uc.reg_read(RR.UC_X86_REG_SS)
            for k, v in ((2, fl & 0xFFFF), (4, uc.reg_read(RR.UC_X86_REG_CS)), (6, uc.reg_read(RR.UC_X86_REG_IP))):
                uc.mem_write(physical(ss_, (sp - k) & 0xFFFF), struct.pack('<H', v))
            out = env.dos(ah, al, dict(ds=uc.reg_read(RR.UC_X86_REG_DS), dx=uc.reg_read(RR.UC_X86_REG_DX)))
            for k, v in out.items():
                uc.reg_write(dict(es=RR.UC_X86_REG_ES, bx=RR.UC_X86_REG_BX)[k], v)
            return
        check(ah in (0x3D, 0x3E, 0x3F, 0x42), f'ENV refusal {name}: INT 21h AH {ah:#x}')
        ds_, dx_ = uc.reg_read(RR.UC_X86_REG_DS), uc.reg_read(RR.UC_X86_REG_DX)
        nm = bytes(backing[physical(ds_, dx_):physical(ds_, dx_) + 80]).split(b'\0')[0].decode('latin-1') if ah == 0x3D else None
        at = physical(ds_, dx_)

        def write(chunk):
            for k, v in enumerate(chunk):
                backing[(at + k) % SIZE] = v
        cf, ax_out, dx_out = fs.call(ah, al, uc.reg_read(RR.UC_X86_REG_BX), uc.reg_read(RR.UC_X86_REG_CX), dx_, nm, write)
        uc.reg_write(RR.UC_X86_REG_AX, ax_out)
        if dx_out is not None:
            uc.reg_write(RR.UC_X86_REG_DX, dx_out)
        uc.reg_write(RR.UC_X86_REG_EFLAGS, (fl | 1) if cf else (fl & ~1))

    def port_in(em, port, size, user):
        check(port == 0x61 and size == 1, f'ENV refusal {name}: IN {port:#x}')
        env.log.append(('in', port, env.in61)); return env.in61

    def port_out(em, port, size, value, user):
        if port in B5.EGA_PORTS:
            ega_events.append((port, value, size, uc.reg_read(RR.UC_X86_REG_CS), uc.reg_read(RR.UC_X86_REG_IP))); device.out(port, value, size); return
        check(port in (0x40, 0x43, 0x61, 0xC0) and size == 1, f'ENV refusal {name}: OUT {port:#x}')
        env.log.append(('out', port, value))
    uc.hook_add(UC_HOOK_CODE, code); uc.hook_add(UC_HOOK_INTR, intr)
    uc.hook_add(UC_HOOK_INSN, port_in, None, 1, 0, RR.UC_X86_INS_IN)
    uc.hook_add(UC_HOOK_INSN, port_out, None, 1, 0, RR.UC_X86_INS_OUT)
    uc.emu_start(physical(c.cs, c.ip), 2 * SIZE, count=200000000)
    check(s['stop'] in ('RETURN', 'RUNERROR', 'EXIT', 'BREAK', 'DIVIDE', 'STALLED'), f'original stop {name}: {s["stop"]}')
    out = dict(stop=s['stop'], fs=fs, env=env, device=device, ega=ega_events, seen=seen, edges=edges, calls=calls)
    if s['stop'] == 'STALLED':
        check(s['after'] == s['snap'][1], f'{name}: RAM changed across {M.STALL_EXTRA} further 050F iterations')
        return dict(out, cpu=s['snap'][0], ram=s['snap'][1], ega_packed=s['snap'][2], writes=device.writes[:s['snap'][3]])
    return dict(out, cpu=cpu_now(), ram=bytes(backing), ega_packed=bytes(device.packed()), writes=list(device.writes))


# ---------------------------------------------------------------------------------------------------- the C model
def run_c(f, lib, m, c, spec):
    keep = []
    fs = file_service(spec)
    q, env = Q.c_env(M.ENVSPEC, keep, stub_at=(STUB_SEG, 0))
    video = B.c_video(keep, LOG_SEG, (STUB_SEG, 0), scratch=SCRATCH)
    q.video = video
    script, ahead = list(spec['keys']), list(spec['typeahead'])

    def kb(ctx, before, after):
        b = before[0]; ah = b.ax >> 8
        a = Cpu.from_buffer_copy(b); a.ip = (b.ip + 2) & 0xFFFF
        if ah == 0x01:
            env.log.append(('16', 1, ahead[0] if ahead else None))
            if ahead:
                a.ax = ahead[0]; a.flags = b.flags & ~0x40
            else:
                a.flags = b.flags | 0x40
            after[0] = a; return 0
        src = ahead if ahead else script
        if ah == 0x00 and src:
            k = src.pop(0); env.log.append(('16', 0, k)); a.ax = k; after[0] = a; return 0
        return 1
    kbfn = R.KB_FN(kb); keep.append(kbfn)
    ke = R.KeyEnv(q, kbfn, None, 64)

    def service(ctx, before, after):
        b = before[0]; nm = None
        if b.ax >> 8 == 0x3D:
            r = ctypes.cast(ctx, ctypes.POINTER(ctypes.c_uint8)); at = physical(b.ds, b.dx)
            nm = bytes(r[at:at + 80]).split(b'\0')[0].decode('latin-1')
        cf, ax, _ = fs.call(b.ax >> 8, b.ax & 0xFF, b.bx, 0, b.dx, nm)
        a = Cpu.from_buffer_copy(b); a.ax = ax; a.flags = (b.flags | 1) if cf else (b.flags & ~1); a.ip = (b.ip + 2) & 0xFFFF
        after[0] = a; return 0

    def read(ctx, before, after, buffer, length):
        b = before[0]

        def write(chunk):
            for k, v in enumerate(chunk[:length]):
                buffer[k] = v
        cf, ax, _ = fs.call(b.ax >> 8, b.ax & 0xFF, b.bx, b.cx, b.dx, None, write)
        a = Cpu.from_buffer_copy(b); a.ax = ax; a.flags = (b.flags | 1) if cf else (b.flags & ~1); a.ip = (b.ip + 2) & 0xFFFF
        after[0] = a; return 0

    def lseek(ctx, before, after):
        b = before[0]
        cf, ax, dx = fs.call(b.ax >> 8, b.ax & 0xFF, b.bx, b.cx, b.dx)
        a = Cpu.from_buffer_copy(b); a.ax = ax
        if dx is not None:
            a.dx = dx
        a.flags = (b.flags | 1) if cf else (b.flags & ~1); a.ip = (b.ip + 2) & 0xFFFF
        after[0] = a; return 0
    int_sps = []

    def tap(fn):
        def wrapped(ctx, before, after, *rest):
            int_sps.append((before[0].ss, before[0].sp))
            return fn(ctx, before, after, *rest)
        return wrapped
    fns = (M.SERVICE_FN(tap(service)), A716.READ_FN(tap(read)), A716.SEEK_FN(tap(lseek))); keep += fns
    ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(m); cpu = Cpu.from_buffer_copy(c); d = Diag0EC4()
    ctx = ctypes.cast(ram, ctypes.c_void_p)
    load = M.Env(A716.Env716(fns[1], None, fns[2], None, fns[2], None, 100000), M.FileEnv(fns[0], ctx, fns[0], ctx), P.Env193(B.Env(video, None), ke),
                 100000, 1000000, spec['retries'], spec['max_seams'], spec['max_disk_prompts'])
    e = Env0EC4(load, B5.Env0BF5(ke, 65536, 65536, 65536), 100000, 256)
    ega = B5.Ega(); lib.pr_ega_reset(ctypes.byref(ega))
    for p in range(4):
        ctypes.memset(ctypes.addressof(ega.plane[p]), 0x31 + p * 0x27, 65536)
    events, writes = [], []

    def out(ctx_, port, value, size, cs, ip):
        events.append((port, value, size, cs, ip)); return 0

    def write(ctx_, off, value):
        writes.append((off, value))
    ports = E0.Env(None, E0.OUT_FN(out), E0.IN_FN()); keep.append(ports)
    bus = B5.Bus(ram, ctypes.pointer(ega), ctypes.pointer(ports), G.WRITE_FN(write), None)
    status = STATUS[f(ctypes.byref(bus), SIZE, ctypes.byref(cpu), ctypes.byref(e), ctypes.byref(d))]
    return dict(status=status, int_sps=int_sps, cpu=cpu, ram=bytes(ram), fs=fs, env=env, diag=d, events=events, writes=writes, ega=bytes(ega))


# ---------------------------------------------------------------------------------------------------- independent statements
def lcg_rolls(seed, n=5):
    out = []
    for _ in range(n):
        seed = (seed * 0x08088405 + 1) & 0xFFFFFFFF
        out.append((seed >> 16) % 6 + 1)
    return out, seed


def expected_calls(o, sizes, bp):
    """The call sequence and the argument words of every call the body makes, derived from the instruction listing and the case alone.
    Entries: (site, kind, checks) where checks maps a frame word index to its expected value (None = not asserted)."""
    ss, cs = o['ssv'], CS_OV
    seq = []
    seq.append((0x0EE0, 'str', {2: 1, 3: (bp - 0x12) & 0xFFFF, 4: ss, 5: 0, 6: o.get('digit', 3), 7: 0}))

    def names(sites, suffix_site):
        seq.append((sites[0], 'strload', {2: 0x0E98, 3: cs, 4: (bp - 0x114) & 0xFFFF, 5: ss}))
        seq.append((sites[1], 'strcat', {2: (bp - 0x12) & 0xFFFF, 3: ss, 4: (bp - 0x114) & 0xFFFF, 5: ss}))
        seq.append((sites[2], 'strcat', {2: suffix_site, 3: cs, 4: (bp - 0x114) & 0xFFFF, 5: ss}))
        seq.append((sites[3], 'strload', {2: 0x0EA0, 3: cs, 4: (bp - 0x119) & 0xFFFF, 5: ss}))
        seq.append((sites[4], 'load', {2: (bp - 6) & 0xFFFF, 3: ss, 4: (bp - 2) & 0xFFFF, 5: ss, 6: o.get('id', 7), 7: (bp - 0x119) & 0xFFFF, 8: ss, 9: (bp - 0x114) & 0xFFFF, 10: ss}))
    names((0x0EF0, 0x0EFA, 0x0F04, 0x0F14, 0x0F27), 0x0E9C)
    if sizes[0] == 0:
        seq.append((0x0F3F, 'strload', {2: 0x0EA5, 3: cs, 4: (bp - 0x2B) & 0xFFFF, 5: ss}))
        seq.append((0x0F44, 'msg', {2: (bp - 0x2B) & 0xFFFF, 3: ss}))
        seq.append((0x0F49, 'quit', {}))
        return seq
    seq.append((0x0F5C, 'move', {2: 0x11D, 3: o['rec'][1], 4: o['rec'][0]}))
    seq += [(0x0FEB, 'roll', {})] * 3 + [(0x1038, 'roll', {})] * 2
    seq.append((0x1076, 'freemem', {2: sizes[0]}))
    names((0x108B, 0x1095, 0x109F, 0x10AF, 0x10C2), 0x0EBC)
    if sizes[1]:
        n = -(-sizes[1] // 9)
        seq.append((0x10D9, 'getmem', {2: 9}))
        for k in range(n):
            seq.append((0x110F, 'move', {2: 9, 'src': 9 * k, 'dst': 'cell'}))
            if (k + 1) * 9 < sizes[1]:
                seq.append((0x1131, 'getmem', {2: 9}))
        seq.append((0x1163, 'freemem', {2: sizes[1]}))
    names((0x1173, 0x117D, 0x1187, 0x1197, 0x11AA), 0x0EC0)
    if sizes[2]:
        n = -(-sizes[2] // 0x3F)
        seq.append((0x11C1, 'getmem', {2: 0x3F}))
        for k in range(n):
            seq.append((0x11F9, 'move', {2: 0x3F, 'src': 0x3F * k, 'dst': 'cell'}))
            if (k + 1) * 0x3F < sizes[2]:
                seq.append((0x121B, 'getmem', {2: 0x3F}))
        seq.append((0x124D, 'freemem', {2: sizes[2]}))
    seq.append((0x1257, 'idle', {}))
    return seq


def check_calls(name, o, sizes, calls, stop, bp):
    exp = expected_calls(o, sizes, bp)
    got = [(site, kind) for site, kind, *_ in calls]
    want = [(site, kind) for site, kind, _ in exp]
    if stop == 'RETURN':
        check(got == want, f'{name}: call sequence {got[:6]}.. vs {want[:6]}.. ({len(got)}/{len(want)})')
    else:
        check(got == want[:len(got)], f'{name}: stop {stop}: call sequence is not a prefix of the statement: got {[(hex(a_), b_) for a_, b_ in got]} want {[(hex(a_), b_) for a_, b_ in want[:len(got)+2]]}')
    for (site, kind, words, spv, _, local), (esite, ekind, chk) in zip(calls, exp):
        # `words` is the callee's entry stack: [IP, CS, args...]; the original return IP is site + 5 in the body segment
        check(words[0] == (site + 5) & 0xFFFF and words[1] == CS_OV, f'{name}: return frame at {site:04x}')
        for k, v in chk.items():
            if isinstance(k, str):
                continue
            if v is not None:
                mask = 0xFF if (kind in ('load', 'msg') and k == 6) or (kind == 'msg' and k == 4) else 0xFFFF
                check((words[k] & mask) == v, f'{name}: {ekind}@{site:04x} word {k} = {words[k]:04x}, expected {v:04x}')
        if kind == 'roll':
            check((words[2] & 0xFF, words[3] & 0xFF) == (6, 1), f'{name}: roll frame {words[2]:04x} {words[3]:04x}')
        if kind == 'msg':
            check((words[4] & 0xFF) == 0x0F, f'{name}: message colour {words[4]:04x}')
        boff, bseg, bsize = local              # the block the last 0716 call left in [BP-6]/[BP-4], its size in [BP-2]
        if kind == 'freemem':
            check((words[3], words[4], words[2]) == (boff, bseg, bsize), f'{name}: FreeMem frame at {site:04x}')
        if kind == 'move' and 'src' in chk:
            check((words[5], words[6]) == ((boff + chk['src']) & 0xFFFF, bseg), f'{name}: Move source at {site:04x}')


def expected_record(o, ram, sizes):
    """Independent statement of the final record, the rolls and the chains for a run that returned."""
    seg, off = o['rec']
    at = lambda s, f: physical(s, f & 0xFFFF)
    rw = lambda s, f: ram[at(s, f)] | ram[at(s, f + 1)] << 8
    b1 = o['b1']; ds = BASE + SEG['data']
    rolls, final = lcg_rolls(o.get('seed', 0x12345678))
    cls = b1[0x2E] if len(b1) > 0x2E else None
    exp = {}
    if len(b1) >= 0x11D:
        for j in range(0x11D):
            exp[j] = b1[j]
        for j in (0xC8, 0xCA, 0x7F, 0x81, 0x104, 0x106, 0x108, 0x10A):
            exp[j] = 0; exp[j + 1] = 0
        exp[0xC0] = 1 if cls in (1, 3, 5) else 2 if cls in (2, 4, 6, 7) else b1[0xC0]
        for i, r in zip((1, 2, 3, 5, 6), rolls):
            exp[0xC0 + i] = (((r + 8) << 4) + r) & 0xFF
        exp[0xC4] = 0xC4
        for j in (0x7F, 0x80, 0x81, 0x82, 0xC8, 0xC9, 0xCA, 0xCB):
            exp.pop(j, None)
        for j, v in exp.items():
            check(ram[at(seg, off + j)] == v, f'record byte +{j:#x}: {ram[at(seg, off + j)]:02x} expected {v:02x}')
    for field, link, unit, raw in ((0x7F, 5, 9, o.get('b2')), (0xC8, 0x2A, 0x3F, o.get('b3'))):
        ptr = (rw(seg, off + field), rw(seg, off + field + 2))
        if not raw:
            check(ptr == (0, 0), f'chain +{field:#x} must be empty')
            continue
        cells = -(-len(raw) // unit); here = ptr
        for k in range(cells):
            check(here != (0, 0), f'chain +{field:#x} ends early at {k}')
            if (k + 1) * unit <= len(raw) and unit - 4 >= 0:
                lim = unit - 4 if True else unit
                for j in range(min(lim, len(raw) - k * unit)):
                    if j < link:
                        check(ram[at(here[1], here[0] + j)] == raw[k * unit + j], f'chain +{field:#x} cell {k} byte {j}')
            here = (rw(here[1], here[0] + link), rw(here[1], here[0] + link + 2))
        check(here == (0, 0), f'chain +{field:#x} must end in nil after {cells} cells')
    check(ram[at(ds, 0x557C)] == 0, 'DS:[557C] cleared at the end')
    check((ram[at(ds, 0x4428)] | ram[at(ds, 0x4429)] << 8 | ram[at(ds, 0x442A)] << 16 | ram[at(ds, 0x442B)] << 24) == final if len(b1) >= 0x11D else True,
          'RandSeed advanced by exactly the five draws')


# ---------------------------------------------------------------------------------------------------- cases
K = R.K
ONLY = []


def cases(rng=None):
    rng = rng or random.Random(0x6F0EC4)
    out = []

    def add(name, **kw):
        out.append((name, kw))
    rec = lambda cls=3, n=0x11D: record(rng, cls, n)
    raw = lambda n: bytes(rng.choice((0, 0, 7, rng.randrange(256))) for _ in range(n))
    std = dict(b1=rec(), b2=raw(40), b3=raw(130))
    add('all-three', **std)
    for cls in range(0, 16):
        add(f'class-{cls:02x}', b1=rec(cls), b2=raw(9), b3=raw(63), seed=rng.randrange(2 ** 32))
    for cls in (0x20, 0x7F, 0x80, 0xFE, 0xFF):
        add(f'class-{cls:02x}', b1=rec(cls), b2=raw(10), b3=raw(64), seed=rng.randrange(2 ** 32))
    for n2 in (0, 1, 8, 9, 10, 17, 18, 19, 27, 300):
        add(f'chain2-{n2}', b1=rec(), b2=raw(n2) if n2 else None, b2_idmiss=n2 == 0, b3=raw(5), seed=rng.randrange(2 ** 32))
    for n3 in (0, 1, 62, 63, 64, 126, 127, 128, 400):
        add(f'chain3-{n3}', b1=rec(), b2=raw(3), b3=raw(n3) if n3 else None, b3_idmiss=n3 == 0, seed=rng.randrange(2 ** 32))
    add('block2-file-absent', b1=rec(), b2=None, b3=raw(70)); add('block3-file-absent', b1=rec(), b2=raw(70), b3=None)
    add('blocks23-absent', b1=rec(), b2=None, b3=None)
    for n1 in (0x11C, 0x11D, 0x11E, 0x200, 0x2F, 0x2E, 0x1):
        add(f'record-{n1:x}', b1=record(rng, rng.choice((1, 2, 9)), n1), b2=raw(20), b3=raw(80))
    for digit in (0, 1, 9, 10, 99, 100, 255):
        add(f'digit-{digit}', digit=digit, **std)
    add('block1-id-missing', b1=None, b1_idmiss=True, b2=raw(5), b3=raw(5), keys=(K['a'],))
    add('block1-file-absent', b1=None, b2=raw(5), b3=raw(5), keys=(K['a'],))
    add('block1-absent-prompt-then-ctrl-c', b1=None, b2=raw(5), b3=raw(5), f557c=0, keys=(0x1C0D, K['c']), appear_after=99, cur=2, saved=1)
    add('block1-absent-prompt-then-appears', b1=rec(), b2=raw(5), b3=raw(5), f557c=0, appear_after=2, keys=(0x1C0D, 0x1C0D))
    add('block1-absent-prompt-never', b1=None, b2=raw(5), b3=raw(5), f557c=0, keys=(0x1C0D, 0x1C0D, K['c']))
    for n in range(0, 3):
        add(f'msg-hotkey-{n}', b1=None, b1_idmiss=True, keys=((K['a'], K['s'], K['d'])[n],) + (K['a'],), cur=n, saved=n * 2, w=1)
    add('msg-ctrl-c-sting', b1=None, b1_idmiss=True, keys=(K['c'],), cur=2, saved=1)
    add('msg-ctrl-c-nosting', b1=None, b1_idmiss=True, keys=(K['c'], K['a']), tail=b'')
    add('msg-ext-key', b1=None, b1_idmiss=True, keys=(K['f1'], K['a']))
    add('msg-break', b1=None, b1_idmiss=True, keys=(K['a'],), brk=1)
    add('msg-typeahead', b1=None, b1_idmiss=True, keys=(K['a'], K['x']), typeahead=(K['x'], K['a']))
    for mode in (0, 1, 2, 3):
        add(f'msg-mode{mode}', b1=None, b1_idmiss=True, keys=(K['a'],), mode=mode, vmode=mode)
    for pa, pb in ((0xA000, 0xA200), (0xA200, 0xA000), (0xA400, 0xA600)):
        add(f'msg-pages{pa:04x}', b1=None, b1_idmiss=True, keys=(K['a'],), pages=(pa, pb), mode=1)
    add('stall-in-block1', id=40, b1=None, b2=raw(3), b3=raw(3), keys=(K['a'],), stall=True)
    for hp in (0x0002, 0x0010, 0x0020, 0x0030, 0x0040, 0x0050, 0x0060, 0x0080, 0x00C0, 0x0100, 0x0140, 0x0200):
        add(f'heap-{hp:04x}', b1=rec(), b2=raw(200), b3=raw(500), heap_paras=hp, seed=rng.randrange(2 ** 32))
    add('record-in-heap', rec=(0x3000, 0x0000), b1=rec(), b2=raw(30), b3=raw(100))
    add('record-in-heap-mid', rec=(0x3040, 0x0010), b1=rec(), b2=raw(90), b3=raw(200))
    add('record-on-stack', rec=(Q.SS, 0xEA00), b1=rec(), b2=raw(20), b3=raw(70))
    add('record-wrap', rec=(REC_SEG, 0xFF00), b1=rec(), b2=raw(20), b3=raw(70))
    add('sp-min', sp=0x09F4, **std); add('sp-max', sp=0xFFF4, **std); add('sp-odd', sp=0xF001, **std)
    add('ss-ds', ss='ds', sp=0xE000, **std)
    for fl in Q.FLAGS + DIRTY_FLAGS:
        add(f'flags{fl:04x}', flags=fl, **std)
    for fl in DIRTY_FLAGS:
        add(f'dirty-msg-{fl:04x}', flags=fl, b1=None, b1_idmiss=True, keys=(K['a'],))
    for seed in (0, 1, 0xFFFFFFFF, 0x80000000):
        add(f'seed-{seed:08x}', seed=seed, **std)
    for k in range(20):
        add(f'random{k}', b1=rec(rng.randrange(256), rng.choice((0x11D, 0x11D, 0x200, rng.randrange(1, 0x300)))), b2=raw(rng.choice((0, 1, 9, 50, 400))) or None,
            b3=raw(rng.choice((0, 1, 63, 90, 700))) or None, regs=tuple(rng.randrange(65536) for _ in range(7)), flags=rng.choice(Q.FLAGS + DIRTY_FLAGS),
            seed=rng.randrange(2 ** 32), digit=rng.randrange(256), hi=rng.randrange(256), id=rng.randrange(1, 255), keys=(K['a'],))
    return out


def normalise(o):
    """Defaults the cases share: the block id matches the archives; the stall case swaps in the 050F fixed-point archive."""
    o = dict(o)
    o.setdefault('id', 7)
    for k in ('b1', 'b2', 'b3'):
        o.setdefault(k, None)
    return o


def sizes_of(o):
    """The block sizes 0716 hands back, from the case alone: the unpacked length of the id row, 0 when the file or the row is missing."""
    out = []
    for k in ('b1', 'b2', 'b3'):
        out.append(len(o[k]) if o[k] else 0)
    return out


def differential(ctx, reloc, stub, f, lib):
    specs = [x for x in cases() if not ONLY or any(t in x[0] for t in ONLY)]; stats = dict(stops={}, calls=0, rolls=0, cases=len(specs)); seen, edges = set(), set()
    ctx['cond_at'] = {i.address: i for i in ctx['cond']}
    returned = 0; non_arch = []
    for index, (name, o0) in enumerate(specs):
        o = normalise(o0); t0 = time.time()
        if o.get('stall'):
            o['b1'] = None
        m, c, spec = build(ctx, reloc, stub, o)
        if o.get('stall'):
            spec['blobs']['b1'] = M.stall_archive(o['id']); o['b1'] = None
        r = run_c(f, lib, m, c, spec)
        st, d = r['status'], r['diag']
        if st in NON_ARCHITECTURAL:
            non_arch.append((name, st, f'{d.site:04x}', d.callee, d.callee_status, f'{d.load.seam:04x}', d.load.leaf_status))
        check(st not in ('CODE', 'ARGUMENT'), f'{name}: C {st} at site {d.site:04x}')
        check(st != 'CALLEE' and st != 'TRANSFER', f'{name}: C {st} at site {d.site:04x} callee {d.callee} status {d.callee_status} (load seam {d.load.seam:04x} leaf {d.load.leaf_status} nested {d.load.nested:04x}; msg {d.msg.callee_status}/{d.msg.key_status})')
        u = run_unicorn(name, ctx, m, c, spec, stop_at=d.load.decode_arrivals if st == 'STALLED' else None)
        check(st == u['stop'], f'{name}: C {st} original {u["stop"]}')
        A716.compare_cpu(name, r['cpu'], u['cpu'])
        if r['ram'] != u['ram']:
            dd = [i for i in range(SIZE) if r['ram'][i] != u['ram'][i]]
            print(name, 'seed0', hex(o.get('seed', 0x12345678)), 'expected rolls', lcg_rolls(o.get('seed', 0x12345678)), [(hex(i), hex(r['ram'][i]), hex(u['ram'][i])) for i in dd[:12]], file=sys.stderr)
        A716.compare_ram(name, r['ram'], u['ram'])
        check(r['fs'].state() == u['fs'].state(), f'{name}: file state'); check(r['fs'].log == u['fs'].log, f'{name}: file log')
        check(r['env'].log == u['env'].log, f'{name}: environment log')
        check(r['events'] == u['ega'], f'{name}: EGA OUT events {len(r["events"])}/{len(u["ega"])}')
        check(r['writes'] == u['writes'], f'{name}: VRAM write operands {len(r["writes"])}/{len(u["writes"])}')
        check(r['ega'] == u['ega_packed'], f'{name}: EGA state')
        sizes = sizes_of(o) if not o.get('stall') else [0, 0, 0]
        bp = (spec['sp'] - 2) & 0xFFFF
        unstable = o['rec'][0] in (0x3000, 0x3040, Q.SS) if o.get('rec') else False       # the record aliases the heap or the live stack frames
        if not (o.get('stall') or unstable):
            ostub = dict(o, ssv=spec['ssv'], rec=spec['rec'])
            check_calls(name, ostub, sizes, u['calls'], u['stop'], bp)
            if u['stop'] == 'RETURN':
                expected_record(ostub, u['ram'], sizes)
        if st == 'RETURN':
            returned += 1
        stats['stops'][st] = stats['stops'].get(st, 0) + 1; stats['calls'] += d.calls; stats['rolls'] += d.rolls
        seen |= u['seen']; edges |= u['edges']
        print(f'[{index + 1}/{len(specs)}] {name}: {st}, {d.calls} calls, {time.time() - t0:.1f}s', file=sys.stderr, flush=True)
    if ONLY:
        return dict(only=ONLY, cases=len(specs), end_states=stats['stops'])
    check(seen == set(ctx['decoded']), f'body starts missing {sorted(set(ctx["decoded"]) - seen)}')
    outcomes = {(i.address, t) for i in ctx['cond'] for t in (i.address + i.size, int(i.op_str, 16))}
    missing = sorted(outcomes - edges)
    return dict(cases=len(specs), end_states=dict(sorted(stats['stops'].items())), non_architectural_stops=len(non_arch), non_architectural=non_arch,
                far_calls_composed=stats['calls'], rolls_composed=stats['rolls'], starts=f'{len(seen)}/{len(ctx["decoded"])}',
                conditional_outcomes=f'{len(outcomes) - len(missing)}/{len(outcomes)}', conditional_outcomes_missing=[f'{a:04X}->{b:04X}' for a, b in missing],
                dirty_flag_images=[f'{x:04X}' for x in DIRTY_FLAGS], cpu_words=14, ram_bytes=SIZE, flag_masks=0)


MUTANT_CASES = ('all-three', 'class-02', 'class-80', 'chain2-10', 'chain3-64', 'blocks23-absent', 'msg-hotkey-1', 'record-11c', 'digit-10', 'class-07')
MUTANTS = (   # (name, text in the model, replacement): each must occur exactly once
    ('class byte 1 for {1,3,5} written as 2', "wr8(ram, c->es, (uint16_t)(c->di + 0xC0u), 1u); }", "wr8(ram, c->es, (uint16_t)(c->di + 0xC0u), 2u); }"),
    ('+104 cleared at +105', "{0x104u, 0x106u}", "{0x105u, 0x106u}"),
    ('first roll loop runs to index 4', "for (idx_end = 3u,", "for (idx_end = 4u,"),
    ('+C4 stores C5h', "(uint16_t)(c->di + 0xC4u), 0xC4u);", "(uint16_t)(c->di + 0xC4u), 0xC5u);"),
    ('block 2 link at +6', "unit_chain(&m, 9u, 0x7Fu, 5u,", "unit_chain(&m, 9u, 0x7Fu, 6u,"),
    ('block 3 unit 3Eh', "unit_chain(&m, 0x3Fu, 0xC8u, 0x2Au,", "unit_chain(&m, 0x3Eu, 0xC8u, 0x2Au,"),
    ('DS:[557C] set to 0 after block 1', "wr8(ram, c->ds, 0x557Cu, 1u);", "wr8(ram, c->ds, 0x557Cu, 0u);"),
    ('DS:[557C] left at 1', "wr8(ram, c->ds, 0x557Cu, 0u);", "wr8(ram, c->ds, 0x557Cu, 1u);"),
    ('Move of the record copies 11Ch', "c->ax = 0x011Du; push(&m, c->ax);", "c->ax = 0x011Cu; push(&m, c->ax);"),
    ('message colour 0Eh', "set_al(c, 0x0Fu); push(&m, c->ax);", "set_al(c, 0x0Eu); push(&m, c->ax);"),
    ('Str maximum length 2', "c->ax = 1; push(&m, c->ax);", "c->ax = 2; push(&m, c->ax);"),
    ('second roll loop ends at 5', "++k, idx_end = 6u)", "++k, idx_end = 5u)"),
    ('class 7 not in the {2,4,6,7} set', "sub_flags(c, al, 7u, 0x80u); if (c->flags & 0x40u) two = 1;", "sub_flags(c, al, 8u, 0x80u); if (c->flags & 0x40u) two = 1;"),
    ('INC byte flags dropped (flags dead before the next callee: expected EQUIVALENT)', "inc8_flags(c, bp8(&m, -0x13)); ", "(void)inc8_flags; "),
)


def mutation_check(ctx, reloc, stub):
    src = (ROOT / 'src/recovered/overlay_u6f_entry6_0ec4_cpu.c').read_text()
    out = []
    ONLY[:] = list(MUTANT_CASES)
    for name, old, new in MUTANTS:
        check(src.count(old) == 1, f'mutant text occurs once: {name}')
        with tempfile.TemporaryDirectory(prefix='u6f-0ec4-mut-') as tmp:
            path = Path(tmp) / 'mutant.c'; path.write_text(src.replace(old, new))
            mf, mlib = library(Path(tmp), override=path)
            try:
                differential(ctx, reloc, stub, mf, mlib)
                out.append((name, 'SURVIVED'))
            except (ValueError, AssertionError) as e:
                out.append((name, 'CAUGHT: ' + str(e)[:80]))
    ONLY[:] = []
    return out


def main():
    p = argparse.ArgumentParser()
    for flag in ('--compile-only', '--unicorn', '--native', '--mutate'):
        p.add_argument(flag, action='store_true')
    p.add_argument('--output', type=Path); p.add_argument('--only', nargs='*', default=[]); a = p.parse_args()
    ONLY[:] = a.only
    with tempfile.TemporaryDirectory(prefix='u6f-0ec4-') as tmp:
        f, lib = library(Path(tmp))
        if a.compile_only:
            print('CHECKER_BUILD_OK'); return
        image, sites = Q.load_image()
        ctx = source()
        stub = B.assemble_stub(scratch=SCRATCH)
        reloc = Q.relocate(image, sites, BASE)
        report = dict(target=dict(span='unit 0x6F [0EC4,1262)', bytes=len(ctx['body']), instructions=len(ctx['decoded']), sha256=DIGEST, file_offset=ctx['file_start'], shadow_system_copy=f'0AF8:{SHADOW[0]:04X}+{SHADOW[1]:X}', constants_span='0E98+2C', constants_sha256=ctx['consts_sha'],
                                  image_sha256=Q.IMAGE_DIGEST, load_base=f'{BASE:04X}', relocated_words=len(ctx['fix']), stub_segment=f'{STUB_AT[0] - BASE:04X}',
                                  stub_offset=f'{STUB_AT[1]:04X}', stub_original_bytes=reloc[physical(*STUB_AT):physical(*STUB_AT) + 5].hex(),
                                  unit9e_span=f'{ctx["dice_lo"]:04X}+{len(ctx["dice"]):X}', int10_stub_sha256=sha256(stub).hexdigest()))
        if a.unicorn:
            report['unicorn'] = differential(ctx, reloc, stub, f, lib)
        if a.mutate:
            results = mutation_check(ctx, reloc, stub)
            report['mutation'] = dict(cases=list(MUTANT_CASES), results=results, caught=sum(1 for _, r in results if r.startswith('CAUGHT')), total=len(results))
        if a.native:
            import ovl_006f_0ec4_native
            report['native'] = ovl_006f_0ec4_native.native(ctx, f, lib, sys.modules[__name__])
    if a.output:
        a.output.write_text(json.dumps(report, sort_keys=True, indent=2) + '\n')
    print(json.dumps(report, sort_keys=True))


if __name__ == '__main__':
    main()
