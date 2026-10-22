#!/usr/bin/env python3
"""RES_03F1_0000 (the tile-box drawer) against the original bytes, as a composed closure: the body, overlay stub 0428:002A
in its loaded state (JMP FAR to unit 40Ch +015D) and OVL_040C_015D with every one of its callees (see
ovl040c_015d_cpu_audit.py for that closure and its declared environment, reused here unchanged).

The loader-expanded START.EXE is loaded at a test base with the loader's relocation table applied; the unit's code is
loaded at its own segment with its fixups applied, and the stub entry holds what the overlay manager writes when the unit
is resident (EAh, offset 015Dh, the unit's segment). An unloaded entry (INT 3Fh) is a separate case: both sides stop at
the stub with OVERLAY. Unicorn: CPU14 + full 1 MiB + stop + the ordered environment event log, unmasked."""
import argparse, ctypes, json, random, struct, subprocess, sys, tempfile
from hashlib import sha256
from pathlib import Path
from capstone import Cs, CS_ARCH_X86, CS_MODE_16
from res0709_ega_reset_0000_cpu_audit import Cpu, ROOT, SIZE, REGISTERS, physical, put16, check
import ovl040c_015d_cpu_audit as O

Q, K = O.Q, O.K
CHECKER_ROWS = ('RES_03F1_0000',)
SEG03F1, STUBSEG, STUB_ENTRY = 0x03F1, 0x0428, 0x002A
ENTRY, END = 0x0000, 0x0137
BASE, OVSEG, SS = O.BASE, O.OVSEG, O.SS
RETURN_TO = O.RETURN_TO
FLAGS = O.FLAGS
SOURCES = sorted(set(O.SOURCES) | {'resident_03f1_box_0000_cpu'})
STATUS = {0: 'RETURN', 1: 'ARGUMENT', 2: 'CALLEE', 3: 'TRANSFER', 4: 'LIMIT', 5: 'OVERLAY', 6: 'CONTRACT', 7: 'EXIT'}
CALL_SITES = [0x0038, 0x0052, 0x006C, 0x0086, 0x00B9, 0x00D2, 0x010C, 0x0124]
STUB_UNLOADED = bytes.fromhex('cd3f5d0100')


class Env03(ctypes.Structure):
    _fields_ = [('tile', O.Env15), ('max_loopbacks', ctypes.c_size_t)]


class Diag03(ctypes.Structure):
    _fields_ = [('site', ctypes.c_uint16), ('status', ctypes.c_int), ('calls', ctypes.c_uint), ('tile', O.Diag15)]


def source(image):
    body = image[SEG03F1 * 16 + ENTRY:SEG03F1 * 16 + END]
    ins = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(body, ENTRY))
    check(len(ins) == 145 and sum(i.size for i in ins) == len(body), 'decode covers the span')
    check(ins[-1].mnemonic == 'retf' and ins[-1].op_str == '0xa', 'single RETF 0Ah at the end')
    check([(i.address, i.op_str) for i in ins if i.mnemonic == 'lcall'] == [(a, '0x428, 0x2a') for a in CALL_SITES], 'far calls')
    check(not [i for i in ins if i.mnemonic in ('call', 'ret', 'int', 'out', 'in')], 'no near call, RET, INT or port access')
    check(image[STUBSEG * 16 + STUB_ENTRY:STUBSEG * 16 + STUB_ENTRY + 5] == STUB_UNLOADED, 'stub entry 2 is INT 3Fh +015D (unit 40Ch)')
    return body, {i.address: i for i in ins}


def library(directory):
    path = directory / 'res03f1box.so'
    subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Wpedantic', '-Werror', '-O2', '-fPIC', '-shared', '-I', str(ROOT / 'include')] +
                   [str(ROOT / f'src/recovered/{s}.c') for s in SOURCES] + ['-o', str(path)], check=True)
    f = ctypes.CDLL(str(path)).pr_r03f1_box_0000_cpu
    f.argtypes = [ctypes.POINTER(O.Bus), ctypes.c_size_t, ctypes.POINTER(Cpu), ctypes.POINTER(Env03), ctypes.POINTER(Diag03)]
    f.restype = ctypes.c_int
    return f


def build(image_reloc, code, fixups, *, style=0, left=2, top=1, right=6, bottom=4, high=0x0000, loaded=True, ovseg=OVSEG,
          sp=0xF000, flags=2, cpu=None, **o):
    m, c, env, keys, stubs = O.build(image_reloc, code, fixups, sp=sp, flags=flags, cpu=cpu, ovseg=ovseg, **o)
    at = physical(BASE + STUBSEG, STUB_ENTRY)
    m[at:at + 5] = (bytes([0xEA]) + struct.pack('<HH', 0x015D, ovseg)) if loaded else STUB_UNLOADED
    for k, v in enumerate((RETURN_TO[1], RETURN_TO[0], style, bottom, right, top, left)):
        put16(m, SS, (sp + 2 * k) & 0xFFFF, (v & 0xFF) | (high if k >= 2 else v & 0xFF00))
    c.cs, c.ip = BASE + SEG03F1, ENTRY
    return m, c, env, keys, stubs


def run_c(f, m, c, envspec, keys, stubs, max_loopbacks=1 << 20):
    keep = []
    q, env = Q.c_env(envspec, keep)
    ke = K.KeyEnv(q, K.keyboard(env, keys, keep), None, 64)
    text = O.T.c_driver(stubs, False, keep)

    def pout(ctx, port, value, size, cs, ip):
        env.log.append(('out', port, value)); return 0 if port in (0x3C4, 0x3C5) else 1

    def pin(ctx, port, size, cs, ip, value):
        return 1
    fo, fi = O.OUT_FN(pout), O.IN_FN(pin); keep += [fo, fi]
    ports = O.PortEnv(None, fo, fi)
    e = Env03(O.Env15(O.VideoEnv(q.video, None), text, ke, 1 << 20), max_loopbacks)
    actual = Cpu.from_buffer_copy(c); ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(m); d = Diag03()
    bus = O.Bus(ram, None, ctypes.pointer(ports), O.C8.WRITE_FN(), None)
    status = STATUS.get(f(ctypes.byref(bus), SIZE, ctypes.byref(actual), ctypes.byref(e), ctypes.byref(d)))
    return status, actual, bytes(ram), env, d


def compare(name, state, body_ips, f, ovseg):
    m, c, envspec, keys, stubs = state
    uc, backing, uenv, ustop, seen, edges = O.run_unicorn(name, m, c, envspec, keys, body_ips,
                                                          body=(BASE + SEG03F1, ENTRY, END), extra=(BASE + STUBSEG, ovseg))
    status, actual, ram, cenv, d = run_c(f, m, c, envspec, keys, stubs)
    check(status == ('EXIT' if ustop == 'BREAK' else ustop), f'C status {name}: {status} original {ustop} (site {d.site:04x} {d.status})')
    check(cenv.log == uenv.log, f'environment events {name}: C {cenv.log[:6]} original {uenv.log[:6]}')
    if ram != bytes(backing):
        diff = [hex(i) for i, (a, b) in enumerate(zip(ram, bytes(backing))) if a != b]
        raise ValueError(f'RAM {name}: {diff[:8]} ({len(diff)} bytes)')
    for (n, _), reg in zip(Cpu._fields_, REGISTERS):
        w = uc.reg_read(reg) & 0xFFFF
        check(getattr(actual, n) == w, f'CPU {name}/{n} C {getattr(actual, n):04x} original {w:04x}')
    return ustop, seen, edges, d


def cases():
    out = [('style0-small', dict()), ('style1', dict(style=1)), ('style2', dict(style=2)),
           ('style3-err', dict(style=3)), ('style-ff-err', dict(style=0xFF, keys=(0x1F73,))),
           ('full-screen', dict(left=0, top=0, right=39, bottom=23)), ('large', dict(left=1, top=1, right=38, bottom=22)), ('single-cell', dict(left=5, top=5, right=5, bottom=5)),
           ('adjacent', dict(left=5, top=5, right=6, bottom=6)), ('inverted', dict(left=9, top=7, right=3, bottom=2)),
           ('wrap-7f', dict(left=0x7D, top=0x7E, right=0x7F, bottom=0x7F)), ('wrap-80', dict(left=0x80, top=0x80, right=0x81, bottom=0x81)),
           ('high-bytes', dict(high=0xA500)), ('unloaded', dict(loaded=False)),
           ('mode1', dict(mode=1, left=1, top=1, right=4, bottom=3)), ('mode2', dict(mode=2)), ('mode3', dict(mode=3)),
           ('band4-null', dict(null=(4,))), ('ovseg-high', dict(ovseg=0x9800)), ('err-break', dict(style=3, brk=1))]
    out += [(f'flags{f:x}', dict(flags=f)) for f in FLAGS]
    rng = random.Random(0x03F1)
    for k in range(30):
        left, top = rng.randrange(0, 30), rng.randrange(0, 20)
        out.append((f'random{k}', dict(style=rng.choice((0, 0, 1, 2, 2, 3)), left=left, top=top, right=left + rng.randrange(-2, 9),
                                       bottom=top + rng.randrange(-2, 6), high=rng.randrange(65536) & 0xFF00, mode=rng.choice((0, 0, 2, 3)),
                                       flags=rng.choice(FLAGS), cpu=tuple(rng.randrange(65536) for _ in range(7)),
                                       fill=rng.choice((0x5A, 0x00, rng.randrange(256))), seed=k)))
    return out


def native_cases():
    """Resident styles in modes 0 and 2 whose adjusted edges stay in 0..7Eh: a -1 edge indexes 1951's row table at -1 and
    sends its Move to an arbitrary segment, which is plain RAM under Unicorn but DOS and the oracle itself under DOSBox.
    The random cases are Unicorn-only: a Unicorn write trace showed eight of them storing outside the oracle's loaded
    regions (the interrupt table at 0024h..007Fh, or high conventional memory), which hangs or reboots DOSBox."""
    def inside(o):
        return all(1 <= o.get(k, d) <= 0x7D for k, d in (('left', 2), ('top', 1), ('right', 6), ('bottom', 4)))
    return [(n, o) for n, o in cases() if o.get('style', 0) < 3 and o.get('loaded', True) and o.get('mode', 0) in (0, 2)
            and 'ovseg' not in o and inside(o) and not n.startswith('random')]


def differential(image_reloc, code, fixups, body_ips, f):
    stops, seen, edges, calls = {}, set(), set(), 0
    for name, o in cases():
        state = build(image_reloc, code, fixups, **o)
        ustop, s, e, d = compare(name, state, body_ips, f, o.get('ovseg', OVSEG))
        stops[ustop] = stops.get(ustop, 0) + 1; seen |= s; edges |= e; calls += d.calls
    conds = {a: (int(i.op_str, 16), a + i.size) for a, i in body_ips.items() if i.mnemonic.startswith('j') and i.mnemonic != 'jmp'}
    outcomes = {(a, t) for a, ts in conds.items() for t in ts}
    check(set(body_ips) <= seen, f'body starts not reached: {[hex(a) for a in sorted(set(body_ips) - seen)]}')
    check(outcomes <= edges, f'outcomes not reached: {sorted(outcomes - edges)}')
    return dict(cases=len(cases()), stops=stops, starts=f'{len(seen)}/{len(body_ips)}', outcomes=f'{len(outcomes & edges)}/{len(outcomes)}',
                tile_calls=calls, cpu_words=14, ram_bytes=SIZE, flag_masks=0)


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
        code, fixups, _, _ = O.source()
        body, body_ips = source(image)
        report = dict(image_sha256=Q.IMAGE_DIGEST, load_base=f'{BASE:04X}', relocations=len(sites),
                      body=dict(span=f'03F1:[{ENTRY:04X},{END:04X})', bytes=len(body), instructions=len(body_ips), sha256=sha256(body).hexdigest()))
        if a.unicorn:
            report['unicorn'] = differential(image_reloc, code, fixups, body_ips, f)
        if a.native:
            import ovl040c_015d_cpu_native as N
            regions = N.REGIONS + (((BASE + STUBSEG) * 16, 0x40),)
            report['native'] = N.native(sys.modules[__name__], code, fixups, body_ips, f, regions=regions,
                                        directory=ROOT / 'resources/res03f1-box/native')
    text = json.dumps(report, indent=1, sort_keys=True)
    if a.output:
        a.output.parent.mkdir(parents=True, exist_ok=True); a.output.write_text(text + '\n')
    print(json.dumps(report, sort_keys=True))


if __name__ == '__main__':
    main()
