#!/usr/bin/env python3
"""Resident 0464:08D4 tile compositor (raw CPU) against the original bytes: CPU14 + full 1 MiB, unmasked.

The body and SYSTEM Move 0AF8:1685 run from their original bytes under Unicorn; the generated C model
(resident_0464_compose_08d4_cpu.c, from emit_res0464_compose_08d4.py) runs on a pr_bus with no EGA attached, since the
compositor writes an off-screen object in ordinary memory. Readiness work: identity and credit are the verifier's.
Original media and generated guest programs stay under ignored resources/."""
import argparse, ctypes, json, random, sys, tempfile
from hashlib import sha256
from pathlib import Path
from capstone import Cs, CS_ARCH_X86, CS_MODE_16
from resident_loader_map import unpack
from res0709_ega_reset_0000_cpu_audit import Cpu, ROOT, SIZE, REGISTERS, physical, put16, check
CHECKER_ROWS = ('RES_0464_08D4',)
SOURCES = ('resident_0464_compose_08d4_cpu', 'system_string_move_leaves_cpu', 'system_runtime_leaves_cpu', 'memory_bus', 'ega_hw')
CS, SYS, DS, SS, SRC, DST, AUX = 0x8800, 0x6000, 0x5000, 0x2000, 0x3000, 0x4000, 0x7000
ENTRY, END, MOVE, MOVE_END = 0x08D4, 0x0D71, 0x1685, 0x16A8
SEGWORDS = (0x0B33, 0x0D2D)
DIGESTS = {'body': '9a486dce6109d9a6c2241339b9ae1f088829d4bd5b1a8128a21b1a32de1ec9fe',
           'move': '1941e7e4eafb9300636ac7ff777605d8ea0b96654893d1198ca3d5f0bfd93972'}
STOPS = {0: 'RETURN', 1: 'ARGUMENT', 2: 'CALLEE', 3: 'PORT', 4: 'TRANSFER', 5: 'LIMIT', 6: 'CONTRACT'}
FLAGS = (0x0002, 0x00D7, 0x08D7, 0x0402, 0x0893, 0x0003)
WRITE_FN = ctypes.CFUNCTYPE(None, ctypes.c_void_p, ctypes.c_uint16, ctypes.c_uint8)
# `XOR AX,AX; CMP AX,[BP-18h]; JBE`: zero is never above an unsigned word, so the fall-through is unreachable.
EXCLUDED = {(0x09F4, 0x09F6): 'JBE after XOR AX,AX is always taken', (0x0AD0, 0x0AD2): 'JBE after XOR AX,AX is always taken',
            (0x0BDE, 0x0BE0): 'JBE after XOR AX,AX is always taken', (0x0CCA, 0x0CCC): 'JBE after XOR AX,AX is always taken'}
# The JMP behind each of those JBEs is therefore never fetched.
UNREACHABLE = {0x09F6, 0x0AD2, 0x0BE0, 0x0CCC}


class Bus(ctypes.Structure):
    _fields_ = [('ram', ctypes.POINTER(ctypes.c_uint8)), ('ega', ctypes.c_void_p), ('ports', ctypes.c_void_p),
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
    check(len(ins) == 429 and len(mins) == 16 and sum(i.size for i in ins) == len(body), 'instruction counts')
    rets = [i.address for i in ins if i.mnemonic in ('ret', 'retf', 'iret')]
    check(rets == [0x0D6E] and 0x0D6E + 3 == END, f'single RETF 10h {rets}')
    check([(i.address, i.op_str) for i in ins if i.mnemonic == 'lcall'] == [(0x0B30, '0xaf8, 0x1685'), (0x0D2A, '0xaf8, 0x1685')], 'two Move calls')
    check(not [i for i in ins if i.mnemonic in ('in', 'out', 'int')], 'no port or interrupt')
    decoded = {('body', i.address): i for i in ins}; decoded.update({('move', i.address): i for i in mins})
    meta = {'0464:08D4': dict(bytes=len(body), instructions=len(ins), sha256=DIGESTS['body'],
                              image_span=[physical(0x464, ENTRY), physical(0x464, END)]),
            '0AF8:1685': dict(bytes=len(move), instructions=len(mins), sha256=DIGESTS['move'])}
    return dict(body=body, move=move), decoded, meta


def library(directory):
    import subprocess
    path = directory / 'compose.so'
    subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Wpedantic', '-Werror', '-O2', '-fPIC', '-shared', '-I', str(ROOT / 'include'),
                    *[str(ROOT / 'src/recovered' / (s + '.c')) for s in SOURCES], '-o', str(path)], check=True)
    lib = ctypes.CDLL(str(path)); f = lib.pr_r0464_compose_08d4_cpu
    f.argtypes = [ctypes.POINTER(Bus), ctypes.c_size_t, ctypes.POINTER(Cpu), ctypes.c_size_t, ctypes.POINTER(Diagnostic)]
    f.restype = ctypes.c_int
    return lib


def build(code, *, mode=0, rows=2, sw=2, dw=40, tile=0x20, item=1, flag=1, aux=True, x=3, y=2, ext='mixed', counter=0x33,
          ss=SS, sp=0x8000, ds=DS, flags=2, src=(0x0100, SRC), dst=(0x0200, DST), auxp=(0x0040, AUX), seed=1, background=0x5A):
    rng = random.Random(seed)
    m = bytearray([background]) * SIZE
    at = physical(CS, ENTRY); m[at:at + len(code['body'])] = code['body']
    for w in SEGWORDS:
        put16(m, CS, w, SYS)
    at = physical(SYS, MOVE); m[at:at + len(code['move'])] = code['move']
    c = Cpu(0x1234, 0x3456, 0x5678, 0x789A, 0x9ABC, 0xBCDE, 0x7777, sp, CS, ENTRY, ds, 0x1111, ss, flags)
    frame = (0x7000, 0x9001, dst[0], dst[1], src[0], src[1], flag, item, y, x)
    for j, v in enumerate(frame):
        put16(m, ss, (sp + 2 * j) & 0xFFFF, v)
    m[physical(ds, 0x4A1E)] = mode & 255
    m[physical(ds, 0x70C7)] = counter
    for r in range(0xA8):  # metadata rows, 168 each and packed: dirty bytes 4A28[], extents 4AD0[], 4C20[], 4D70[]
        m[physical(ds, 0x4A28 + r)] = 0
        lo = {'low': 0x0000, 'high': 0xFFFF}.get(ext, rng.choice((0x0000, 0xFFFF, rng.randrange(0x10000))))
        hi = {'low': 0xFFFF, 'high': 0x0000}.get(ext, rng.choice((0x0000, 0xFFFF, rng.randrange(0x10000))))
        put16(m, ds, 0x4AD0 + 2 * r, 0xEEEE); put16(m, ds, 0x4C20 + 2 * r, lo); put16(m, ds, 0x4D70 + 2 * r, hi)
    put16(m, src[1], src[0], rows); put16(m, src[1], (src[0] + 2) & 0xFFFF, sw)
    put16(m, src[1], (src[0] + 0x11) & 0xFFFF, tile)
    put16(m, src[1], (src[0] + 0x13) & 0xFFFF, auxp[0] if aux else 0); put16(m, src[1], (src[0] + 0x15) & 0xFFFF, auxp[1] if aux else 0)
    for k in range(0x600):
        m[physical(src[1], (src[0] + 0x17 + k) & 0xFFFF)] = (k * 29 + 7) & 255
        m[physical(auxp[1], (auxp[0] + k) & 0xFFFF)] = (k * 53 + 0x91) & 255
    put16(m, dst[1], (dst[0] + 2) & 0xFFFF, dw)
    for k in range(0x1000):
        m[physical(dst[1], (dst[0] + 0x17 + k) & 0xFFFF)] = (k * 13 + 3) & 255
    return m, c


def run_c(lib, m, c, limit):
    ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(m)
    bus = Bus(ram, None, None, WRITE_FN(), None); d = Diagnostic()
    status = lib.pr_r0464_compose_08d4_cpu(ctypes.byref(bus), SIZE, ctypes.byref(c), limit, ctypes.byref(d))
    return status, bytes(ram), d


def compare(name, state, decoded, lib, limit=1 << 20):
    from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE
    from unicorn import x86_const as R
    m, c = state; uc = Uc(UC_ARCH_X86, UC_MODE_16); backing = (ctypes.c_uint8 * SIZE).from_buffer_copy(m)
    uc.mem_map_ptr(0, SIZE, 7, ctypes.addressof(backing)); uc.mem_map_ptr(SIZE, SIZE, 7, ctypes.addressof(backing))
    for (n, _), r in zip(Cpu._fields_, REGISTERS):
        uc.reg_write(r, getattr(c, n))
    s = dict(stop=None, loops=0, last=None, call=None, ret=False, moves=0)
    seen, edges = set(), set()

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
        seen.add(key); s['last'] = key if conditional(ins) else None
        if key[0] == 'body' and ins.mnemonic == 'lcall':
            s['call'] = (CS, ip + ins.size); s['moves'] += 1
        if ins.mnemonic == 'retf':
            s['ret'] = True
    uc.hook_add(UC_HOOK_CODE, hook)
    uc.emu_start(physical(c.cs, c.ip), 2 * SIZE, count=20_000_000)
    check(s['stop'] is not None, f'instruction ceiling {name}')
    actual = Cpu.from_buffer_copy(c)
    status, ram, d = run_c(lib, m, actual, limit)
    check(status == s['stop'], f'status {name}: C {STOPS.get(status)} original {STOPS.get(s["stop"])}')
    if ram != bytes(backing):
        diff = [hex(i) for i, (a, b) in enumerate(zip(ram, bytes(backing))) if a != b]
        raise ValueError(f'RAM {name}: {diff[:8]} ({len(diff)} bytes)')
    for (n, _), r in zip(Cpu._fields_, REGISTERS):
        want = uc.reg_read(r) & 0xFFFF
        check(getattr(actual, n) == want, f'CPU {name}/{n} C {getattr(actual, n):04x} original {want:04x}')
    check(d.move_calls == s['moves'] or status in (2, 4, 6), f'move count {name}: {d.move_calls}/{s["moves"]}')
    return STOPS[status], seen, edges, s['moves'], actual


def cases(quick=False):
    out = []
    for mode in (0, 1, 2):
        for flag, aux in ((1, True), (0, True), (1, False), (2, True)):
            for rows, sw in ((1, 1), (2, 2), (3, 3)):
                out.append((f'm{mode}-f{flag}-a{int(aux)}-r{rows}w{sw}', dict(mode=mode, flag=flag, aux=aux, rows=rows, sw=sw)))
        out += [(f'm{mode}-ext-{e}-f{fl}', dict(mode=mode, ext=e, flag=fl)) for e in ('low', 'high') for fl in (0, 1)]
        # Move copies forward when its source offset is at or above its destination offset, backward otherwise.
        out += [(f'm{mode}-move-forward', dict(mode=mode, flag=0, x=0, y=0, src=(0x9000, SRC), dst=(0x0000, DST)))]
        out += [(f'm{mode}-move-backward', dict(mode=mode, flag=0, src=(0x0010, SRC), dst=(0x0400, DST))),
                (f'm{mode}-move-overlap', dict(mode=mode, flag=0, rows=2, src=(0x0100, DST), dst=(0x0104, DST)))]
        out += [(f'm{mode}-wide-neg', dict(mode=mode, sw=0x20, rows=1)), (f'm{mode}-sw0', dict(mode=mode, sw=0, rows=2)),
                (f'm{mode}-item0', dict(mode=mode, item=0)), (f'm{mode}-item3', dict(mode=mode, item=3, tile=0x40)),
                (f'm{mode}-x0y0', dict(mode=mode, x=0, y=0)), (f'm{mode}-xneg', dict(mode=mode, x=0xFFFF)),
                (f'm{mode}-limit0', dict(mode=mode, limit=0)), (f'm{mode}-limit2', dict(mode=mode, limit=2)),
                (f'm{mode}-rows0-limit', dict(mode=mode, rows=0, limit=40)),
                (f'm{mode}-a20-stack', dict(mode=mode, ss=0xFFFF, sp=0x0400)), (f'm{mode}-ss-ds', dict(mode=mode, ss=DS, sp=0xE000))]
        out += [(f'm{mode}-f{f:x}', dict(mode=mode, flags=f)) for f in FLAGS]
    out += [(f'mode{v}', dict(mode=v)) for v in (3, 4, 0x80, 0xFF)]
    if quick:
        return out
    rng = random.Random(0x046408D4)
    for k in range(60):
        out.append((f'random{k}', dict(mode=rng.choice((0, 1, 2, 3)), flag=rng.randrange(4), aux=rng.random() < 0.7, rows=rng.randrange(1, 5),
                                       sw=rng.randrange(0, 7), dw=rng.randrange(8, 81), tile=rng.choice((0x10, 0x20, 0x30)),
                                       item=rng.randrange(4), x=rng.randrange(0, 20), y=rng.randrange(0, 12), counter=rng.randrange(256),
                                       flags=rng.choice(FLAGS), seed=k, sp=rng.randrange(0x0800, 0xF000) & ~1)))
    return out


def differential(code, decoded, lib, quick=False):
    seen, edges, stops, per, moves = set(), set(), {}, {}, 0
    specs = cases(quick)
    for name, options in specs:
        options = dict(options); limit = options.pop('limit', 1 << 20)
        status, v, e, mv, final = compare(name, build(code, **options), decoded, lib, limit)
        seen |= v; edges |= e; stops[status] = stops.get(status, 0) + 1; per[name] = status; moves += mv
    body = {k for k in decoded if k[0] == 'body'}
    branches = {(k, (k[0], t)) for k, i in decoded.items() if conditional(i) for t in (int(i.op_str, 16), i.address + i.size)}
    missing = sorted((a[1], b[1]) for a, b in branches - edges)
    if not quick:
        check({k[1] for k in body - seen} == UNREACHABLE, f'body starts missing: {sorted(hex(k[1]) for k in body - seen)}')
        check(set(missing) == set(EXCLUDED), f'outcomes not covered/excluded: {[f"{a:04X}->{b:04X}" for a, b in missing]}')
    return dict(cases=len(specs), stops=dict(sorted(stops.items())), non_return={k: v for k, v in per.items() if v != 'RETURN'},
                body_starts=len(body & seen), body_starts_total=len(body), closure_starts=len(seen), closure_total=len(decoded),
                outcomes=len(branches & edges), outcomes_total=len(branches),
                excluded_outcomes={f'{a:04X}->{b:04X}': r for (a, b), r in EXCLUDED.items()}, move_calls=moves,
                unreachable_starts=[f'{a:04X}' for a in sorted(UNREACHABLE)],
                cpu_words=14, ram_bytes=SIZE, flag_masks=0)


def main():
    p = argparse.ArgumentParser()
    for flag in ('--compile-only', '--quick', '--unicorn', '--native'):
        p.add_argument(flag, action='store_true')
    p.add_argument('--output', type=Path); a = p.parse_args()
    with tempfile.TemporaryDirectory(prefix='compose08d4-') as tmp:
        lib = library(Path(tmp))
        if a.compile_only:
            print('CHECKER_BUILD_OK'); return
        code, decoded, meta = source(); report = dict(target=meta)
        if a.unicorn:
            report['unicorn'] = differential(code, decoded, lib, a.quick)
        if a.native:
            from res0464_compose_08d4_cpu_native import native
            report['native'] = native(code, lib)
    if a.output:
        a.output.parent.mkdir(parents=True, exist_ok=True); a.output.write_text(json.dumps(report, sort_keys=True, indent=2) + '\n')
    print(json.dumps(report, sort_keys=True))


if __name__ == '__main__':
    sys.modules['res0464_compose_08d4_cpu_audit'] = sys.modules[__name__]
    main()
