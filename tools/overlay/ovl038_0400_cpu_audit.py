#!/usr/bin/env python3
"""OVL_0038_0400 (GAME.OVR unit 38h +0400, the 009E dispatcher's slot-11 handler) against the original bytes, as a composed
closure: the body and RES_00BA_23DB (the party count) run from the original bytes.

The loader-expanded START.EXE (hash-checked) is loaded at base 1000h with the loader's relocation table applied, and unit 38h's
code at its own segment with its fixups applied; no byte is patched. Each case's world (records A and B, B's +108h far
pointer C, the party list 23DB walks, the DS counters) is built from a seed derived from the case NAME, never from run order.
Unicorn: CPU14 + all 1 MiB, unmasked; any interrupt or port access fails the case. The C model runs on the same RAM image.
--native corroborates through ovl040c_015d_cpu_native.py's oracle (every case lies inside its regions).

python3 tools/overlay/ovl038_0400_cpu_audit.py [--compile-only] [--unicorn] [--native] [--output report.json]"""
import argparse, ctypes, hashlib, json, random, struct, subprocess, sys, tempfile
from pathlib import Path
from capstone import Cs, CS_ARCH_X86, CS_MODE_16
from res0709_ega_reset_0000_cpu_audit import Cpu, ROOT, SIZE, REGISTERS, physical, put16, check
from overlay_inventory import find_units, parse_mz
import res0899_quit_0000_cpu_audit as Q

CHECKER_ROWS = ('OVL_0038_0400',)
ENTRY, END = 0x0400, 0x04BA
BASE = 0x1000
DS = BASE + 0x0C7C
SS, OVSEG = 0x7000, 0x5000
RETURN_TO = (0x9F00, 0x0000)                       # (CS, IP) the RETF 0Ah returns to; nothing is loaded there
A_SEG, B_SEG, C_SEG, PARTY_SEG = 0x3000, 0x3400, 0x3800, 0x4000
SOURCES = ['resident_00ba_link_check_2e03_cpu', 'resident_00ba_party_count_23db_cpu', 'overlay_u38_handler_0400_cpu']
STATUS = {0: 'RETURN', 1: 'ARGUMENT', 2: 'CALLEE', 3: 'TRANSFER'}
FLAGS = (0x0002, 0x00D7, 0x08D7, 0x0893, 0x0003, 0x0846)
RES23DB = (0x23DB, 0x2433)


def unit_code():
    exe = (ROOT / 'resources/DOS/POOLRAD/START.EXE').read_bytes(); ovr = (ROOT / 'resources/DOS/POOLRAD/GAME.OVR').read_bytes()
    u = {u['stub_para']: u for u in find_units(exe[parse_mz(exe)['header_bytes']:], ovr)}[0x38]
    code = ovr[u['ovr_offset']:u['ovr_offset'] + u['code_size']]
    check(hashlib.sha256(code).hexdigest() == u['code_sha256'], 'unit 38h code identity')
    check(ENTRY in u['entries'], 'unit 38h exports +0400')
    return code, list(u['fixups'])


def source():
    code, fixups = unit_code()
    body = code[ENTRY:END]
    ins = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(body, ENTRY))
    check(len(ins) == 59 and sum(i.size for i in ins) == len(body), 'decode covers the span')
    check(ins[-1].mnemonic == 'retf' and ins[-1].op_str == '0xa' and [i.mnemonic for i in ins].count('retf') == 1, 'single RETF 0Ah')
    check([(i.address, i.op_str) for i in ins if i.mnemonic == 'lcall'] == [(0x04AC, '0xba, 0x23db')], 'the one far call')
    check(0x04AF in fixups, 'the far-call segment word is a fixup')
    check(not [i for i in ins if i.mnemonic in ('call', 'ret', 'int', 'out', 'in')], 'no near call, RET, INT or port access')
    return code, fixups, body, {i.address: i for i in ins}


def library(directory):
    path = directory / 'u38_0400.so'
    subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Wpedantic', '-Werror', '-O2', '-fPIC', '-shared', '-I', str(ROOT / 'include')] +
                   [str(ROOT / f'src/recovered/{s}.c') for s in SOURCES] + ['-o', str(path)], check=True)
    f = ctypes.CDLL(str(path)).pr_u38_handler_0400_cpu
    f.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t, ctypes.POINTER(Cpu), ctypes.c_size_t, ctypes.POINTER(ctypes.c_int)]
    f.restype = ctypes.c_int
    return f


def seeded(name):
    return random.Random(int.from_bytes(hashlib.sha256(name.encode()).digest()[:8], 'little'))


def build(image_reloc, code, fixups, *, name='case', sel=1, a3=0x40, b10e=0x01, b84=0xB3, b10f=0x00, party=2, c_is=None,
          a_off=0x0100, b_off=0x0200, c_off=0x0300, a_seg=A_SEG, b_seg=B_SEG, c_seg=C_SEG, sp=0xF000, flags=0x0002,
          cpu=None, fill=None, ovseg=OVSEG, members=None):
    """party: number of characters in the DS:5D96 list (B is not in it unless c_is/members say so). members: a list of
    (+10Dh byte, +10Eh byte) for the party characters. c_is: 'B' or 'A' points B+108h at that record instead of C."""
    rng = seeded(name)
    fillb = rng.randrange(256) if fill is None else fill
    m = bytearray([fillb]) * SIZE
    m[BASE * 16:BASE * 16 + len(image_reloc)] = image_reloc
    unit = bytearray(code)
    for f in fixups:
        struct.pack_into('<H', unit, f, (struct.unpack_from('<H', unit, f)[0] + BASE) & 0xFFFF)
    m[ovseg * 16:ovseg * 16 + len(unit)] = unit
    for seg, off, n in ((a_seg, a_off, 0x20), (b_seg, b_off, 0x120), (c_seg, c_off, 0x20)):
        for k in range(n):
            m[physical(seg, off + k)] = rng.randrange(256)
    m[physical(a_seg, a_off + 3)] = a3
    m[physical(b_seg, b_off + 0x84)] = b84; m[physical(b_seg, b_off + 0x10E)] = b10e; m[physical(b_seg, b_off + 0x10F)] = b10f
    target = {'B': (b_off, b_seg), 'A': (a_off, a_seg)}.get(c_is, (c_off, c_seg))
    put16(m, b_seg, b_off + 0x108, target[0]); put16(m, b_seg, b_off + 0x10A, target[1])
    members = members if members is not None else [(rng.choice((0, 1, 7)), rng.choice((0, 1, 0xFF, 0x7F, 0x80, rng.randrange(256))))
                                                    for _ in range(party)]
    links = [(PARTY_SEG + 0x20 * k, 0x0010) for k in range(len(members))]
    head = links[0] if links else (0, 0)
    put16(m, DS, 0x5D96, head[1]); put16(m, DS, 0x5D98, head[0])
    for k, ((seg, off), (b10d, idx)) in enumerate(zip(links, members)):
        nxt = links[k + 1] if k + 1 < len(links) else (0, 0)
        m[physical(seg, off + 0x10D)] = b10d; m[physical(seg, off + 0x10E)] = idx
        put16(m, seg, off + 0x104, nxt[1]); put16(m, seg, off + 0x106, nxt[0])
    for k in range(-128, 130):
        m[physical(DS, 0x6814 + k)] = rng.randrange(256)
    m[physical(DS, 0x6825)] = rng.randrange(256)
    for k, v in enumerate((RETURN_TO[1], RETURN_TO[0], sel, a_off, a_seg, b_off, b_seg)):
        put16(m, SS, (sp + 2 * k) & 0xFFFF, v & 0xFFFF)
    r = cpu or tuple(rng.randrange(65536) for _ in range(7))
    c = Cpu(*r, sp, ovseg, ENTRY, DS, rng.randrange(65536), SS, flags)
    return m, c, dict(name=name), [], {}


def run_unicorn(name, m, c, body_ips, ovseg):
    from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE, UC_HOOK_INTR, UC_HOOK_INSN
    from unicorn import x86_const as R
    uc = Uc(UC_ARCH_X86, UC_MODE_16); backing = (ctypes.c_uint8 * SIZE).from_buffer_copy(m)
    uc.mem_map_ptr(0, SIZE, 7, ctypes.addressof(backing)); uc.mem_map_ptr(SIZE, SIZE, 7, ctypes.addressof(backing))
    for (n, _), reg in zip(Cpu._fields_, REGISTERS):
        uc.reg_write(reg, getattr(c, n))
    s = dict(stop=None, prev=None, steps=0); seen, edges = set(), set()

    def code_hook(em, address, size, user):
        cs = uc.reg_read(R.UC_X86_REG_CS); ip = uc.reg_read(R.UC_X86_REG_IP)
        if s['prev'] is not None and cs == ovseg:
            edges.add((s['prev'], ip))
        s['prev'] = None
        if (cs, ip) == RETURN_TO:
            s['stop'] = 'RETURN'; return uc.emu_stop()
        if cs == ovseg and ENTRY <= ip < END:
            check(ip in body_ips, f'escape inside the body {name} {ip:04x}'); seen.add(ip)
            if body_ips[ip].mnemonic.startswith('j') and body_ips[ip].mnemonic != 'jmp':
                s['prev'] = ip
        elif not (cs == BASE + 0x00BA and RES23DB[0] <= ip < RES23DB[1]):
            s['stop'] = 'TRANSFER'; return uc.emu_stop()
        s['steps'] += 1
        if s['steps'] > 200000:
            s['stop'] = 'RUNAWAY'; uc.emu_stop()

    def refuse(*args):
        raise ValueError(f'ENV refusal {name}: interrupt or port access')
    uc.hook_add(UC_HOOK_CODE, code_hook); uc.hook_add(UC_HOOK_INTR, refuse)
    uc.hook_add(UC_HOOK_INSN, refuse, None, 1, 0, R.UC_X86_INS_IN); uc.hook_add(UC_HOOK_INSN, refuse, None, 1, 0, R.UC_X86_INS_OUT)
    uc.emu_start(physical(c.cs, c.ip), 2 * SIZE, count=250000)
    check(s['stop'] == 'RETURN', f'original stop {name}: {s["stop"]}')
    return uc, backing, s['stop'], seen, edges


def run_c(f, m, c, envspec, keys, stubs, budget=1 << 20):
    actual = Cpu.from_buffer_copy(c); ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(m); cs = ctypes.c_int(-2)
    status = STATUS.get(f(ram, SIZE, ctypes.byref(actual), budget, ctypes.byref(cs)))
    return status, actual, bytes(ram), None, cs.value


def compare(name, state, body_ips, f, ovseg):
    m, c, envspec, keys, stubs = state
    uc, backing, ustop, seen, edges = run_unicorn(name, m, c, body_ips, ovseg)
    status, actual, ram, _, callee = run_c(f, m, c, envspec, keys, stubs)
    check(status == ustop, f'C status {name}: {status} original {ustop} (23DB status {callee})')
    if ram != bytes(backing):
        diff = [hex(i) for i, (a, b) in enumerate(zip(ram, bytes(backing))) if a != b]
        raise ValueError(f'RAM {name}: {diff[:8]} ({len(diff)} bytes)')
    for (n, _), reg in zip(Cpu._fields_, REGISTERS):
        w = uc.reg_read(reg) & 0xFFFF
        check(getattr(actual, n) == w, f'CPU {name}/{n} C {getattr(actual, n):04x} original {w:04x}')
    return ustop, seen, edges, callee


def cases():
    out = []
    for sel in (1, 0xFF):
        for a3 in (0x00, 0x40, 0xBF, 0xFF):
            for b84 in (0xB3, 0xB2, 0x00):
                out.append((f'sel{sel:02x}-a{a3:02x}-b84{b84:02x}', dict(sel=sel, a3=a3, b84=b84)))
    for a3 in (0x00, 0x1F, 0x20, 0x3F, 0x5F, 0x60, 0x9F, 0xC0, 0xDF, 0xFF):
        for b10e in (0x00, 0x01, 0x02, 0x03, 0xFF, 0xFE, 0x7F, 0x80):
            out.append((f'sel00-a{a3:02x}-e{b10e:02x}', dict(sel=0, a3=a3, b10e=b10e)))
    for b84 in (0x00, 0x7E, 0x7F, 0x80, 0xB3, 0xFF):
        out.append((f'sel00-b84{b84:02x}', dict(sel=0, a3=0x01, b84=b84)))
    for n in (0, 1, 3, 6):
        out.append((f'party{n}', dict(sel=0, a3=0x04, party=n)))
    out += [('party-indices', dict(sel=0, a3=0x10, members=[(1, 0x00), (1, 0x01), (1, 0xFF), (1, 0x80), (1, 0x7F), (0, 0x05)])),
            ('party-none-counted', dict(sel=0, a3=0x10, members=[(0, 0x00), (0, 0x01)])),
            ('c-is-b', dict(sel=0, a3=0x02, c_is='B')), ('c-is-a', dict(sel=0, a3=0x02, c_is='A')),
            ('a-is-b', dict(sel=0, a3=0x02, a_off=0x0200, a_seg=B_SEG)), ('a-is-b-sel', dict(sel=1, a3=0x40, a_off=0x0200, a_seg=B_SEG)),
            ('a-on-b84', dict(sel=0, a3=0x7C, a_off=0x0281, a_seg=B_SEG)), ('a-on-b10e', dict(sel=0, a3=0x05, a_off=0x030B, a_seg=B_SEG)),
            ('sel-high-byte', dict(sel=0x5A00, a3=0x00)), ('sel-low-only', dict(sel=0x0100, a3=0x00)),
            ('wrap-b', dict(sel=0, a3=0x00, b_off=0xFF00)), ('wrap-a', dict(sel=0, a3=0x00, a_off=0xFFFE)),
            ('wrap-c', dict(sel=0, a3=0x00, c_off=0xFFF6)), ('wrap-b-sel', dict(sel=1, a3=0x40, b_off=0xFF00)),
            ('ovseg-high', dict(sel=0, a3=0x00, ovseg=0x9800)), ('sp-low', dict(sel=0, a3=0x00, sp=0x0100)),
            ('sp-high', dict(sel=1, a3=0x40, sp=0xFFEE))]
    out += [(f'flags{f:x}', dict(sel=k & 1, a3=0x40 * (k & 1), flags=f)) for k, f in enumerate(FLAGS)]
    for k in range(60):
        rng = seeded(f'random{k}')
        out.append((f'random{k}', dict(sel=rng.choice((0, 0, 1, 0x80, 0x5A00)), a3=rng.randrange(256), b10e=rng.randrange(256),
                                       b84=rng.choice((0x00, 0x7F, 0x80, 0xB3, rng.randrange(256))), b10f=rng.randrange(256),
                                       party=rng.randrange(0, 5), c_is=rng.choice((None, None, 'A', 'B')),
                                       flags=rng.choice(FLAGS), cpu=tuple(rng.randrange(65536) for _ in range(7)))))
    return [(n, dict(o, name=n)) for n, o in out]


SMOKE = ('sel01-a40-b84b3', 'sel01-a00-b84b2', 'sel00-a20-e00', 'sel00-a1f-e01', 'sel00-a00-eff', 'sel00-b84ff', 'sel00-b847f',
         'party-indices', 'c-is-b', 'a-is-b', 'sel-high-byte', 'wrap-b', 'flags8d7', 'random3')


def native_cases():
    """Everything except the cases that move code or the stack outside the oracle's regions (ovseg-high, sp-low, sp-high)."""
    return [(n, o) for n, o in cases() if n not in ('ovseg-high', 'sp-low', 'sp-high')]


def differential(image_reloc, code, fixups, body_ips, f, only=None):
    stops, seen, edges, ran = {}, set(), set(), 0
    for name, o in cases():
        if only is not None and name not in only:
            continue
        state = build(image_reloc, code, fixups, **o)
        ustop, s, e, _ = compare(name, state, body_ips, f, o.get('ovseg', OVSEG))
        stops[ustop] = stops.get(ustop, 0) + 1; seen |= s; edges |= e; ran += 1
    report = dict(cases=ran, stops=stops, cpu_words=14, ram_bytes=SIZE, flag_masks=0)
    if only is not None:
        return report
    conds = {a: (int(i.op_str, 16), a + i.size) for a, i in body_ips.items() if i.mnemonic.startswith('j') and i.mnemonic != 'jmp'}
    outcomes = {(a, t) for a, ts in conds.items() for t in ts}
    check(set(body_ips) <= seen, f'body starts not reached: {[hex(a) for a in sorted(set(body_ips) - seen)]}')
    check(outcomes <= edges, f'outcomes not reached: {sorted((hex(a), hex(t)) for a, t in outcomes - edges)}')
    report.update(starts=f'{len(seen)}/{len(body_ips)}', outcomes=f'{len(outcomes & edges)}/{len(outcomes)}')
    return report


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
        code, fixups, body, body_ips = source()
        report = dict(image_sha256=Q.IMAGE_DIGEST, load_base=f'{BASE:04X}', relocations=len(sites),
                      body=dict(span=f'unit 38h +[{ENTRY:04X},{END:04X})', bytes=len(body), instructions=len(body_ips),
                                sha256=hashlib.sha256(body).hexdigest()))
        if a.unicorn:
            report['unicorn'] = differential(image_reloc, code, fixups, body_ips, f)
        if a.native:
            import ovl040c_015d_cpu_native as N
            report['native'] = N.native(sys.modules[__name__], code, fixups, body_ips, f, directory=ROOT / 'resources/u38-0400/native')
    text = json.dumps(report, indent=1, sort_keys=True)
    if a.output:
        a.output.parent.mkdir(parents=True, exist_ok=True); a.output.write_text(text + '\n')
    print(json.dumps(report, sort_keys=True))


if __name__ == '__main__':
    sys.modules.setdefault('ovl038_0400_cpu_audit', sys.modules[__name__])
    main()
