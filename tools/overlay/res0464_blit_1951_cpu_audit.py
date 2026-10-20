#!/usr/bin/env python3
"""Resident 0464:1951 bitmap blit (raw CPU) against the original bytes: CPU14 + full 1 MiB, OUT and A000-write streams.

Readiness work, not credit: the body is a graphics primitive; no identity is claimed here.
Original media and generated guest programs stay under ignored resources/."""
import argparse, ctypes, json, random, sys, tempfile
from hashlib import sha256
from pathlib import Path
from capstone import Cs, CS_ARCH_X86, CS_MODE_16
from resident_loader_map import unpack
from res0709_ega_reset_0000_cpu_audit import Cpu, Env, OUT_FN, IN_FN, ROOT, SIZE, REGISTERS, physical, put16, check
CHECKER_ROWS = ('RES_0464_1951',)
SOURCES = ('resident_0464_blit_1951_cpu', 'system_string_move_leaves_cpu', 'system_runtime_leaves_cpu', 'memory_bus', 'ega_hw')
CS, SYS, DS, SS, BMP = 0x8800, 0x6000, 0x5000, 0x2000, 0x7000
ENTRY, END, MOVE, MOVE_END = 0x1951, 0x1B52, 0x1685, 0x16A8
SEGWORDS = (0x19DF, 0x1B3C)
DIGESTS = {'body': 'd64bb18e4503a5679481894c5ddaad44b3c0f703b00d1810afbbba8e9f358de3',
           'move': '1941e7e4eafb9300636ac7ff777605d8ea0b96654893d1198ca3d5f0bfd93972'}
STOPS = {0: 'RETURN', 1: 'ARGUMENT', 2: 'CALLEE', 3: 'PORT', 4: 'TRANSFER', 5: 'LIMIT', 6: 'CONTRACT'}
FLAGS = (0x0002, 0x00D7, 0x08D7, 0x0402, 0x0893, 0x0003)
WRITE_FN = ctypes.CFUNCTYPE(None, ctypes.c_void_p, ctypes.c_uint16, ctypes.c_uint8)
# Composed conditional successors that no case reaches, with the argument and the state that would make them feasible.
EXCLUDED = {}


class Ega(ctypes.Structure):
    _fields_ = [('plane', (ctypes.c_uint8 * 65536) * 4), ('latch', ctypes.c_uint8 * 4), ('seq_index', ctypes.c_uint8),
                ('seq', ctypes.c_uint8 * 5), ('gc_index', ctypes.c_uint8), ('gc', ctypes.c_uint8 * 9)]


class Bus(ctypes.Structure):
    _fields_ = [('ram', ctypes.POINTER(ctypes.c_uint8)), ('ega', ctypes.POINTER(Ega)), ('ports', ctypes.POINTER(Env)),
                ('on_vram_write', WRITE_FN), ('ctx', ctypes.c_void_p)]


class Diagnostic(ctypes.Structure):
    _fields_ = [('move_calls', ctypes.c_int), ('move_status', ctypes.c_int)]


def taken(mnemonic, f):
    cf, zf, sf, of = f & 1, (f >> 6) & 1, (f >> 7) & 1, (f >> 11) & 1
    return bool({'je': zf, 'jne': not zf, 'jb': cf, 'jae': not cf, 'jbe': cf or zf, 'ja': not (cf or zf),
                 'jl': sf != of, 'jge': sf == of, 'jle': zf or sf != of, 'jg': not zf and sf == of}[mnemonic])


def conditional(i):
    return (i.mnemonic.startswith('j') and i.mnemonic != 'jmp') or i.mnemonic.startswith('loop')


def source():
    image = bytes(unpack((ROOT / 'resources/DOS/POOLRAD/START.EXE').read_bytes())[0])
    md = Cs(CS_ARCH_X86, CS_MODE_16)
    body = image[physical(0x464, ENTRY):physical(0x464, END)]
    move = image[physical(0xAF8, MOVE):physical(0xAF8, MOVE_END)]
    check(sha256(body).hexdigest() == DIGESTS['body'] and sha256(move).hexdigest() == DIGESTS['move'], 'identity')
    ins = list(md.disasm(body, ENTRY)); mins = list(md.disasm(move, MOVE))
    check(len(ins) == 201 and len(mins) == 16, 'instruction counts')
    rets = [i.address for i in ins if i.mnemonic in ('ret', 'retf', 'iret')]
    check(rets == [0x1B4F] and 0x1B4F + 3 == END, f'first RETF {rets}')
    decoded = {('body', i.address): i for i in ins}; decoded.update({('move', i.address): i for i in mins})
    meta = {'0464:1951': dict(bytes=len(body), instructions=len(ins), sha256=DIGESTS['body'],
                              image_span=[physical(0x464, ENTRY), physical(0x464, END)]),
            '0AF8:1685': dict(bytes=len(move), instructions=len(mins), sha256=DIGESTS['move'])}
    return dict(body=body, move=move), decoded, meta


def library(directory):
    import subprocess
    path = directory / 'blit.so'
    subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Wpedantic', '-Werror', '-O2', '-fPIC', '-shared', '-I', str(ROOT / 'include'),
                    *[str(ROOT / 'src/recovered' / (s + '.c')) for s in SOURCES], '-o', str(path)], check=True)
    lib = ctypes.CDLL(str(path)); f = lib.pr_r0464_blit_1951_cpu
    f.argtypes = [ctypes.POINTER(Bus), ctypes.c_size_t, ctypes.POINTER(Cpu), ctypes.c_size_t, ctypes.POINTER(Diagnostic)]
    f.restype = ctypes.c_int
    lib.pr_ega_reset.argtypes = [ctypes.POINTER(Ega)]
    return lib


def build(code, *, mode=1, rows=2, cols=2, x=1, y=3, bmp=(0x0100, BMP), ss=SS, sp=0x8000, ds=DS, flags=2,
          row_seg=0xB800, writes=(), background=0x5A):
    m = bytearray([background]) * SIZE
    at = physical(CS, ENTRY); m[at:at + len(code['body'])] = code['body']
    for w in SEGWORDS:
        put16(m, CS, w, SYS)
    at = physical(SYS, MOVE); m[at:at + len(code['move'])] = code['move']
    c = Cpu(0x1234, 0x3456, 0x5678, 0x789A, 0x9ABC, 0xBCDE, 0x7777, sp, CS, ENTRY, ds, 0x1111, ss, flags)
    for j, v in enumerate((0x7000, 0x9001, bmp[0], bmp[1], x, y)):
        put16(m, ss, (sp + 2 * j) & 0xFFFF, v)
    m[physical(ds, 0x4A1E)] = mode & 255
    for r in range(64):  # disjoint row tables: 4EC0/51E0 hold a segment per row, 5050 an A000 offset per row
        put16(m, ds, 0x4EC0 + 2 * r, row_seg + r * 0x10); put16(m, ds, 0x51E0 + 2 * r, row_seg + 0x400 + r * 0x10)
        put16(m, ds, 0x5050 + 2 * r, (r * 40) & 0xFFFF)
    for p, v in enumerate((1, 2, 4, 8)):
        m[physical(ds, 0x25F6 + p)] = v
    # bmp is (offset, segment), matching the pushed far pointer [bp+6]/[bp+8]
    put16(m, bmp[1], bmp[0], rows); put16(m, bmp[1], (bmp[0] + 2) & 0xFFFF, cols)
    for k in range(0x400):
        m[physical(bmp[1], (bmp[0] + 0x17 + k) & 0xFFFF)] = (k * 29 + 7) & 255
    for seg, off, blob in writes:
        for j, v in enumerate(blob):
            m[physical(seg, (off + j) & 0xFFFF)] = v
    return m, c


def run_c(lib, m, c, limit, refuse):
    ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(m); ega = Ega(); lib.pr_ega_reset(ctypes.byref(ega))
    for p in range(4):
        ctypes.memset(ctypes.addressof(ega.plane[p]), 0x31 + p * 0x27, 65536)
    events, stores = [], []

    def out(ctx, port, value, size, cs, ip):
        if refuse is not None and len(events) == refuse:
            return 1
        events.append((port, value, size, cs, ip)); return 0

    def write(ctx, off, value):
        stores.append((off, value))
    env = Env(None, OUT_FN(out), IN_FN()); bus = Bus(ram, ctypes.pointer(ega), ctypes.pointer(env), WRITE_FN(write), None)
    d = Diagnostic()
    status = lib.pr_r0464_blit_1951_cpu(ctypes.byref(bus), SIZE, ctypes.byref(c), limit, ctypes.byref(d))
    return status, bytes(ram), events, stores, d


def compare(name, state, code, decoded, lib, limit=1 << 20, refuse=None):
    from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE, UC_HOOK_MEM_WRITE, UC_HOOK_INSN
    from unicorn import x86_const as R
    m, c = state; uc = Uc(UC_ARCH_X86, UC_MODE_16); backing = (ctypes.c_uint8 * SIZE).from_buffer_copy(m)
    uc.mem_map_ptr(0, SIZE, 7, ctypes.addressof(backing)); uc.mem_map_ptr(SIZE, SIZE, 7, ctypes.addressof(backing))
    for (n, _), r in zip(Cpu._fields_, REGISTERS):
        uc.reg_write(r, getattr(c, n))
    s = dict(stop=None, loops=0, last=None, call=None, ret=False, moves=0)
    seen, edges, events, writes = set(), set(), [], []

    def stop(v):
        s['stop'] = v; uc.emu_stop()

    def hook(em, address, size, user):
        cs = uc.reg_read(R.UC_X86_REG_CS); ip = uc.reg_read(R.UC_X86_REG_IP)
        key = ('body' if cs == CS else 'move' if cs == SYS else '?', ip)
        if s['ret']:
            s['ret'] = False
            if s['call'] is None:
                return stop(0)
            if (cs, ip) != s['call']:
                return stop(4)
            s['call'] = None
        check(key in decoded, f'escape {name} {cs:04x}:{ip:04x}')
        ins = decoded[key]
        if s['last'] is not None:
            edges.add((s['last'], key))
        if key[0] == 'body' and (ins.mnemonic == 'jmp' or conditional(ins)) and int(ins.op_str, 16) <= ip:
            if ins.mnemonic == 'jmp' or taken(ins.mnemonic, uc.reg_read(R.UC_X86_REG_EFLAGS)):
                if s['loops'] == limit:
                    return stop(5)
                s['loops'] += 1
        if key[0] == 'body' and ins.mnemonic == 'out' and refuse is not None and len(events) == refuse:
            return stop(3)
        seen.add(key); s['last'] = key if conditional(ins) else None
        if key[0] == 'body' and ins.mnemonic == 'lcall':
            s['call'] = (CS, ip + ins.size); s['moves'] += 1
        if ins.mnemonic == 'retf':
            s['ret'] = True

    def out(em, port, size, value, user):
        events.append((port, value, size, uc.reg_read(R.UC_X86_REG_CS), uc.reg_read(R.UC_X86_REG_IP)))

    def write(em, access, address, size, value, user):
        for j in range(size):
            at = (address + j) & (SIZE - 1)
            if 0xA0000 <= at < 0xB0000:
                writes.append((at - 0xA0000, (value >> (8 * j)) & 255))
    uc.hook_add(UC_HOOK_CODE, hook); uc.hook_add(UC_HOOK_INSN, out, None, 1, 0, R.UC_X86_INS_OUT)
    uc.hook_add(UC_HOOK_MEM_WRITE, write)
    uc.emu_start(physical(c.cs, c.ip), 2 * SIZE, count=5_000_000)
    check(s['stop'] is not None, f'instruction ceiling {name}')
    actual = Cpu.from_buffer_copy(c)
    status, ram, mine, stores, d = run_c(lib, m, actual, limit, refuse)
    check(status == s['stop'], f'status {name}: C {STOPS.get(status)} original {STOPS.get(s["stop"])}')
    check(mine == events, f'OUTs {name}: {mine[:4]} / {events[:4]}')
    check(stores == writes, f'A000 writes {name}: {len(stores)}/{len(writes)}')
    expected = bytes(backing)
    check(ram[:0xA0000] + ram[0xB0000:] == expected[:0xA0000] + expected[0xB0000:], f'RAM {name}')
    replay = bytearray(m[0xA0000:0xB0000])
    for off, v in writes:
        replay[off] = v
    check(expected[0xA0000:0xB0000] == bytes(replay) and ram[0xA0000:0xB0000] == bytes(m[0xA0000:0xB0000]), f'aperture {name}')
    for (n, _), r in zip(Cpu._fields_, REGISTERS):
        want = uc.reg_read(r) & 0xFFFF
        check(getattr(actual, n) == want, f'CPU {name}/{n} C {getattr(actual, n):04x} original {want:04x}')
    check(d.move_calls == s['moves'] or status in (2, 4, 6), f'move count {name}: {d.move_calls}/{s["moves"]}')
    return STOPS[status], seen, edges, len(events), len(writes), actual


def cases(quick=False):
    out = [(f'mode{m}', dict(mode=m)) for m in (0, 1, 2, 3, 255)]
    out += [('null', dict(bmp=(0, 0))), ('null-seg-only', dict(bmp=(0, 0x7000))), ('null-off-only', dict(bmp=(0x0100, 0)))]
    for m in (0, 1, 2):
        out += [(f'm{m}-rows0', dict(mode=m, rows=0)), (f'm{m}-cols0', dict(mode=m, cols=0)),
                (f'm{m}-rows1cols1', dict(mode=m, rows=1, cols=1)), (f'm{m}-wide', dict(mode=m, rows=3, cols=3)),
                (f'm{m}-xneg', dict(mode=m, x=0xFFFF)), (f'm{m}-xwrap', dict(mode=m, x=0x1FFF, rows=3)),
                (f'm{m}-yneg', dict(mode=m, y=0xFFFF)), (f'm{m}-limit0', dict(mode=m, limit=0)),
                (f'm{m}-limit1', dict(mode=m, limit=1)), (f'm{m}-limit3', dict(mode=m, limit=3))]
        for f in FLAGS:
            out.append((f'm{m}-f{f:x}', dict(mode=m, flags=f)))
        for sp in (0x0030, 0x0031, 0x8001, 0xFFF4):
            out.append((f'm{m}-sp{sp:x}', dict(mode=m, sp=sp)))
        out += [(f'm{m}-ss-ds', dict(mode=m, ss=DS)), (f'm{m}-a20-stack', dict(mode=m, ss=0xFFFF, sp=0x0400)),
                (f'm{m}-bmp-wrap', dict(mode=m, bmp=(0xFFF0, BMP)))]
    if quick:
        return out
    for n in range(12):  # mode 1: refuse each OUT position of a 1x2 blit (1 index OUT, 8 plane OUTs, 1 final)
        out.append((f'm1-port{n}', dict(mode=1, rows=1, cols=2, refuse=n)))
    # In-contract aliases (mode 0): the row segment equals SS, so Move's destination lands on the stack.
    # The body's running source offset [bp-10h] and loop counter [bp-4] sit below BP; Move's own return frame
    # sits at entry SP - 2Ah. y selects the destination offset 2*y.
    sp = 0x8000; bp = sp - 2
    # Row 0 (x = 0) so the row table yields SS itself. Each alias case asserts its outcome.
    out += [('alias-locals', dict(mode=0, rows=2, cols=4, x=0, sp=sp, row_seg=SS, y=(bp - 0x10) // 2, expect='RETURN',
                                  overlap=(bp - 0x1A, bp))),
            # Saved BP and the body's own return IP only (4 bytes at BP): the outer RETF goes elsewhere.
            ('alias-own-return', dict(mode=0, rows=1, cols=2, x=0, sp=sp, row_seg=SS, y=bp // 2, expect='RETURN',
                                      overlap=(bp, bp + 4), returns_elsewhere=True)),
            # The row loop counter [bp-4] (and [bp-2]): the loop no longer meets its end, so the budget stops it.
            ('alias-counter', dict(mode=0, rows=1, cols=2, x=0, sp=sp, row_seg=SS, y=(bp - 4) // 2, expect='LIMIT',
                                   overlap=(bp - 4, bp), limit=6)),
            ('alias-move-frame', dict(mode=0, rows=1, cols=2, x=0, sp=sp, row_seg=SS, y=(sp - 0x2A) // 2, expect='TRANSFER')),
            ('alias-move-frame-m2', dict(mode=2, rows=1, cols=1, x=0, sp=sp, row_seg=SS - 0x400, y=(sp - 0x2A) // 4, expect='TRANSFER'))]
    return out


def differential(code, decoded, lib, quick=False):
    seen, edges, stops, outs, writes, per = set(), set(), {}, 0, 0, {}
    specs = cases(quick)
    for name, options in specs:
        options = dict(options); limit = options.pop('limit', 1 << 20); refuse = options.pop('refuse', None)
        expect = options.pop('expect', None); overlap = options.pop('overlap', None)
        elsewhere = options.pop('returns_elsewhere', False)
        state = build(code, **options)
        if overlap is not None:  # the first Move's destination must really meet the stated stack range
            y = options['y']; n = (2 if options.get('mode', 1) == 0 else 4) * options.get('cols', 2)
            d0 = (2 if options.get('mode', 1) == 0 else 4) * y
            check(d0 < overlap[1] and overlap[0] < d0 + n, f'fixture {name}: destination [{d0:04X},+{n}) misses {overlap}')
        status, v, e, o, w, final = compare(name, state, code, decoded, lib, limit, refuse)
        if expect is not None:
            check(status == expect, f'fixture {name} expected {expect}, got {status}')
        if elsewhere:
            check((final.cs, final.ip) != (0x9001, 0x7000), f'fixture {name}: outer return was not redirected')
        print(f'  {name}: {status}', flush=True) if name.startswith('alias') else None
        seen |= v; edges |= e; stops[status] = stops.get(status, 0) + 1; outs += o; writes += w; per[name] = status
    body = {k for k in decoded if k[0] == 'body'}
    branches = {(k, (k[0], t)) for k, i in decoded.items() if conditional(i)
                for t in (int(i.op_str, 16), i.address + i.size)}
    missing = sorted((a[1], b[1]) for a, b in branches - edges)
    if not quick:
        check(body <= seen, f'body starts missing: {sorted(k[1] for k in body - seen)}')
        check(set(missing) == set(EXCLUDED), f'outcomes not covered/excluded: {[f"{a:04X}->{b:04X}" for a, b in missing]}')
    return dict(cases=len(specs), stops=dict(sorted(stops.items())), non_return={k: v for k, v in per.items() if v != 'RETURN'},
                body_starts=len(body & seen), body_starts_total=len(body), closure_starts=len(seen), closure_total=len(decoded),
                outcomes=len(branches & edges), outcomes_total=len(branches),
                missing_outcomes=[f'{a:04X}->{b:04X}' for a, b in missing], out_events=outs, a000_write_operands=writes,
                cpu_words=14, ordinary_ram_bytes=SIZE - 65536, aperture_bytes_replayed=65536, ram_bytes=SIZE, flag_masks=0)


def main():
    p = argparse.ArgumentParser()
    for flag in ('--compile-only', '--quick', '--unicorn', '--native'):
        p.add_argument(flag, action='store_true')
    p.add_argument('--output', type=Path); a = p.parse_args()
    with tempfile.TemporaryDirectory(prefix='blit1951-') as tmp:
        lib = library(Path(tmp))
        if a.compile_only:
            print('CHECKER_BUILD_OK'); return
        code, decoded, meta = source(); report = dict(target=meta)
        if a.unicorn:
            report['unicorn'] = differential(code, decoded, lib, a.quick)
        if a.native:
            from res0464_blit_1951_cpu_native import native
            report['native'] = native(code, lib)
    if a.output:
        a.output.write_text(json.dumps(report, sort_keys=True, indent=2) + '\n')
    print(json.dumps(report, sort_keys=True))


if __name__ == '__main__':
    sys.modules['res0464_blit_1951_cpu_audit'] = sys.modules[__name__]
    main()
