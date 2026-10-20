#!/usr/bin/env python3
"""Resident 0464:248E bitmap allocator (raw CPU) against the original bytes: CPU14 + full 1 MiB.

Readiness work, not credit. Heap fixture and GetMem stop handling follow u33_party_attach_12e1_audit.py
(credited composition of the same SYSTEM GetMem). Media stays under ignored resources/."""
import argparse, ctypes, json, sys, tempfile
from hashlib import sha256
from pathlib import Path
from capstone import Cs, CS_ARCH_X86, CS_MODE_16
from resident_loader_map import unpack
from res0709_ega_reset_0000_cpu_audit import Cpu, Env, OUT_FN, IN_FN, ROOT, SIZE, REGISTERS, physical, put16, check
from res0709_text_06a7_cpu_audit import Ega, Bus, WRITE_FN


def conditional(i):
    return (i.mnemonic.startswith('j') and i.mnemonic != 'jmp') or i.mnemonic.startswith('loop')
CHECKER_ROWS = ('RES_0464_248E',)
SOURCES = ('resident_0464_alloc_248e_cpu', 'system_getmem_023f_cpu', 'system_fillchar_bus_16a8_cpu', 'system_runtime_leaves_cpu',
           'memory_bus', 'ega_hw')
CS, SYS, DS, SS, HEAP = 0x8800, 0x6000, 0x5000, 0x2000, 0x3000
ENTRY, END = 0x248E, 0x2595
SYS_SPANS = ((0x23F, 0x254), (0x308, 0x3D3), (0x491, 0x4A2), (0xA9, 0xAE), (0x16A8, 0x16BC))
SEGWORDS = (0x24C3, 0x24E4, 0x2534, 0x255E, 0x2568, 0x258D)
STOPS = {0: 'RETURN', 1: 'ARGUMENT', 2: 'CALLEE', 3: 'PORT', 4: 'TRANSFER', 5: 'LIMIT'}
GETMEM = {0: 'RETURNED', 1: 'ARGUMENT', 2: 'RUNERROR', 3: 'HEAP_ERROR', 4: 'TRANSFER', 5: 'LIMIT'}
DIGEST = '3476a8ae8b3a3c1430c591b9922d493db2e4c475d542f089257464a82ce9d32f'
# Composed conditional outcomes no case reaches: (site, target) -> (kind, argument, feasible state).
EXCLUDED = {}  # body outcomes: none
# GetMem outcomes outside this composition (same argument as the credited u33_party_attach_12e1 review): the only
# modelled HeapError is the default 00A9 (XOR AX,AX; RETF 2), so AL = 0 after +03C3; a custom handler stops the
# model at the LCALL (HEAP_ERROR). Feasible only with a DS:4414 handler returning AL = 1 (nil) or AL >= 2 (retry).
SYSTEM_EXCLUDED = {(0x3C9, 0x3CB): 'HeapError AL >= 1', (0x3CB, 0x383): 'HeapError AL = 1 (nil)',
                   (0x3CB, 0x3CD): 'HeapError AL >= 2 (retry)'}
SYSTEM_EXCLUDED_STARTS = {0x3CB: 'after HeapError AL >= 1', 0x3CD: 'retry', 0x3D0: 'retry'}


class Diagnostic(ctypes.Structure):
    _fields_ = [('getmem_calls', ctypes.c_int), ('getmem_status', ctypes.c_int), ('fill_calls', ctypes.c_int), ('fill_status', ctypes.c_int)]


def source():
    image = bytes(unpack((ROOT / 'resources/DOS/POOLRAD/START.EXE').read_bytes())[0])
    md = Cs(CS_ARCH_X86, CS_MODE_16)
    body = image[physical(0x464, ENTRY):physical(0x464, END)]
    check(sha256(body).hexdigest() == DIGEST, 'body identity')
    ins = list(md.disasm(body, ENTRY))
    check(len(ins) == 95 and [i.address for i in ins if i.mnemonic in ('ret', 'retf', 'iret')] == [0x2592] and 0x2592 + 3 == END,
          'first RETF / count')
    check([i.address + 3 for i in ins if i.mnemonic == 'lcall'] == list(SEGWORDS), 'far-call segment words')
    decoded = {('body', i.address): i for i in ins}
    sysbytes = {}
    for a, b in SYS_SPANS:
        blob = image[physical(0xAF8, a):physical(0xAF8, b)]; sysbytes[a] = blob
        decoded.update({('sys', i.address): i for i in md.disasm(blob, a)})
    meta = {'0464:248E': dict(bytes=len(body), instructions=len(ins), sha256=DIGEST,
                              image_span=[physical(0x464, ENTRY), physical(0x464, END)])}
    return dict(body=body, sys=sysbytes), decoded, meta


def library(directory):
    import subprocess
    path = directory / 'alloc.so'
    subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Wpedantic', '-Werror', '-O2', '-fPIC', '-shared', '-I', str(ROOT / 'include'),
                    *[str(ROOT / 'src/recovered' / (s + '.c')) for s in SOURCES], '-o', str(path)], check=True)
    lib = ctypes.CDLL(str(path)); f = lib.pr_r0464_alloc_248e_cpu
    f.argtypes = [ctypes.POINTER(Bus), ctypes.c_size_t, ctypes.POINTER(Cpu), ctypes.c_size_t, ctypes.c_uint, ctypes.POINTER(Diagnostic)]
    f.restype = ctypes.c_int
    lib.pr_ega_reset.argtypes = [ctypes.POINTER(Ega)]
    return lib


def build(code, *, mode=0, bit=1, mult=3, w0=4, w1=5, pvar=(DS, 0x9000), heap_mode='top', custom_heap=False, blocks=None, end=None, ss=SS, sp=0x8000,
          flags=2, writes=(), background=0x5A):
    m = bytearray([background]) * SIZE

    def word(seg, off, v):
        put16(m, seg, off & 0xFFFF, v)
    at = physical(CS, ENTRY); m[at:at + len(code['body'])] = code['body']
    for w in SEGWORDS:
        word(CS, w, SYS)
    for a, blob in code['sys'].items():
        at = physical(SYS, a); m[at:at + len(blob)] = blob
    c = Cpu(0x1234, 0x3456, 0x5678, 0x789A, 0x9ABC, 0xBCDE, 0x7777, sp, CS, ENTRY, DS, 0x1111, ss, flags)
    for j, v in enumerate((0x7000, 0x9001, pvar[1], pvar[0], bit, mult, w1, w0)):
        word(ss, sp + 2 * j, v)
    m[physical(DS, 0x4A1E)] = mode
    # Heap exactly as u33_party_attach_12e1_audit.fixture builds it (HeapOrg/HeapPtr/HeapEnd/FreeList/HeapError).
    top = (HEAP + 0x300, 0)
    explicit_blocks, explicit_end = blocks, end
    end = (HEAP + 0xFF0, 0); blocks = []
    if heap_mode == 'exact': blocks = [(HEAP + 0x10, 0, 1, 8)]
    elif heap_mode == 'split': blocks = [(HEAP + 0x10, 0, 0x80, 0)]
    elif heap_mode == 'split-carry': blocks = [(HEAP + 0x10, 8, 0x80, 0)]
    elif heap_mode == 'skip': blocks = [(HEAP + 0x10, 0, 0, 8), (HEAP + 0x20, 0, 1, 0), (HEAP + 0x30, 0, 0x80, 8)]
    elif heap_mode == 'fail': end = top
    elif heap_mode == 'cyclic': blocks = [(HEAP + 0x10, 0, 0, 8)]
    elif heap_mode == 'tight': end = (top[0] + 2, 0)
    elif heap_mode != 'top': raise ValueError(heap_mode)
    if explicit_blocks is not None:
        blocks = [(HEAP + so, off, para, gran) for so, off, para, gran in explicit_blocks]
    if explicit_end is not None:
        end = (top[0] + explicit_end[0], explicit_end[1])
    for off, value in ((0x4400, 0), (0x4402, HEAP), (0x4404, top[1]), (0x4406, top[0]), (0x4408, end[1]), (0x440A, end[0]),
                       (0x4414, 0x1234 if custom_heap else 0xA9), (0x4416, SYS)):
        word(DS, off, value)
    chain = blocks + [(top[0], top[1], 0, 0)]; word(DS, 0x440C, chain[0][1]); word(DS, 0x440E, chain[0][0])
    for (seg, off, paragraphs, granule), nxt in zip(chain, chain[1:]):
        if heap_mode == 'cyclic':
            nxt = chain[0]
        word(seg, off, nxt[1]); word(seg, off + 2, nxt[0]); word(seg, off + 4, granule); word(seg, off + 6, paragraphs)
    for seg, off, blob in writes:
        for j, v in enumerate(blob):
            m[physical(seg, (off + j) & 0xFFFF)] = v
    return m, c


def run_c(lib, m, c, limit, cap):
    ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(m); ega = Ega(); lib.pr_ega_reset(ctypes.byref(ega))
    stores = []
    env = Env(None, OUT_FN(lambda *a: 0), IN_FN())
    bus = Bus(ram, ctypes.pointer(ega), ctypes.pointer(env), WRITE_FN(lambda ctx, o, v: stores.append((o, v))), None)
    d = Diagnostic()
    status = lib.pr_r0464_alloc_248e_cpu(ctypes.byref(bus), SIZE, ctypes.byref(c), limit, cap, ctypes.byref(d))
    return status, bytes(ram), stores, d


def compare(name, state, code, decoded, lib, limit=1 << 20, cap=64):
    from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
    from unicorn import x86_const as R
    m, c = state; uc = Uc(UC_ARCH_X86, UC_MODE_16); backing = (ctypes.c_uint8 * SIZE).from_buffer_copy(m)
    uc.mem_map_ptr(0, SIZE, 7, ctypes.addressof(backing)); uc.mem_map_ptr(SIZE, SIZE, 7, ctypes.addressof(backing))
    for (n, _), r in zip(Cpu._fields_, REGISTERS):
        uc.reg_write(r, getattr(c, n))
    s = dict(stop=None, calls=[], pending=None, last=None, prev=None, walk=0, getmem=-1, gm_calls=0, fill_calls=0, path=set())
    seen, edges, writes = set(), set(), []

    def word(seg, off):
        at = physical(seg, off & 0xFFFF); return backing[at] | backing[(at + 1) & (SIZE - 1)] << 8

    def stop(v):
        s['stop'] = v; uc.emu_stop()

    def hook(em, address, size, user):
        cs = uc.reg_read(R.UC_X86_REG_CS); ip = uc.reg_read(R.UC_X86_REG_IP)
        if s['pending'] is not None:
            target, expected, getmem_ret = s['pending']; s['pending'] = None
            check((cs, ip) == target, f'{name}: return differs')
            if getmem_ret:
                s['getmem'] = 0
            if expected is None:
                return stop(0)
            if target != expected:
                return stop(4)
        key = ('body' if cs == CS else 'sys' if cs == SYS else '?', ip)
        if key == ('sys', 0xE2):
            s['getmem'] = 2; return stop(2)
        if key == ('sys', 0x23F):
            s['walk'] = 0
        if key[0] == 'sys' and ip in (0x3B7, 0x3C3) and (word(DS, 0x4414), word(DS, 0x4416)) != (0xA9, SYS):
            s['getmem'] = 3; return stop(2)
        if key == ('sys', 0x319) and s['last'] in (('sys', 0x32C), ('sys', 0x334)):
            s['walk'] += 1
            if s['walk'] > cap:
                s['getmem'] = 5; return stop(2)
        check(key in decoded, f'escape {name} {cs:04x}:{ip:04x}')
        ins = decoded[key]
        if s['prev'] is not None:
            edges.add((s['prev'], key))
        seen.add(key); s['path'].add(key)
        s['prev'] = key if conditional(ins) else None
        if ins.mnemonic in ('call', 'lcall'):
            s['calls'].append((cs, (ip + ins.size) & 0xFFFF))
            if key[0] == 'body':
                s['gm_calls' if ins.op_str.endswith('0x23f') else 'fill_calls'] += 1
        if ins.mnemonic in ('ret', 'retf'):
            sp = uc.reg_read(R.UC_X86_REG_SP); ss = uc.reg_read(R.UC_X86_REG_SS)
            target = (word(ss, sp + 2) if ins.mnemonic == 'retf' else cs, word(ss, sp))
            s['pending'] = (target, s['calls'].pop() if s['calls'] else None, key == ('sys', 0x24B))
        s['last'] = key

    def write(em, access, address, size, value, user):
        for j in range(size):
            at = (address + j) & (SIZE - 1)
            if 0xA0000 <= at < 0xB0000:
                writes.append((at - 0xA0000, (value >> (8 * j)) & 255))
    uc.hook_add(UC_HOOK_CODE, hook); uc.hook_add(UC_HOOK_MEM_WRITE, write)
    uc.emu_start(physical(c.cs, c.ip), 2 * SIZE, count=2_000_000)
    check(s['stop'] is not None, f'instruction ceiling {name}')
    actual = Cpu.from_buffer_copy(c)
    status, ram, stores, d = run_c(lib, m, actual, limit, cap)
    check(status == s['stop'], f'status {name}: C {STOPS.get(status)} original {STOPS.get(s["stop"])}')
    if status == 2:
        check(d.getmem_status == s['getmem'] or (s['getmem'] == -1 and d.fill_status != 0), f'callee {name}: {d.getmem_status}/{s["getmem"]}')
    check(stores == writes, f'A000 writes {name}')
    expected = bytes(backing)
    check(ram[:0xA0000] + ram[0xB0000:] == expected[:0xA0000] + expected[0xB0000:], f'RAM {name}')
    replay = bytearray(m[0xA0000:0xB0000])
    for off, v in writes:
        replay[off] = v
    check(expected[0xA0000:0xB0000] == bytes(replay) and ram[0xA0000:0xB0000] == bytes(m[0xA0000:0xB0000]), f'aperture {name}')
    split = None
    for (n, _), r in zip(Cpu._fields_, REGISTERS):
        want = uc.reg_read(r) & 0xFFFF; got = getattr(actual, n)
        if n == 'flags' and got != want and ('body', 0x2555) in s['path'] and status == 0:
            # Declared split: SHR +2555's architecturally undefined AF reaches RETURN through FillChar (no flag writes).
            # The model follows DOSBox-X (AF := 1); Unicorn clears it. AF must be the only difference.
            check(got ^ want == 0x10 and got & 0x10, f'split {name}: C {got:04x} Unicorn {want:04x}')
            split = dict(case=name, model_flags=f'{got:04X}', unicorn_flags=f'{want:04X}', differing='AF')
            continue
        check(got == want, f'CPU {name}/{n} C {got:04x} original {want:04x}')
    check((d.getmem_calls, d.fill_calls) == (s['gm_calls'], s['fill_calls']),
          f'call counts {name}: C {d.getmem_calls}/{d.fill_calls} original {s["gm_calls"]}/{s["fill_calls"]}')
    detail = STOPS[status] + (f'/getmem/{GETMEM[d.getmem_status]}' if status == 2 else '')
    return detail, seen, edges, split


def cases(quick=False):
    out = []
    for mode in (0, 1, 2):
        for bit in (0, 1):
            out.append((f'm{mode}-b{bit}', dict(mode=mode, bit=bit)))
    for hm in ('exact', 'split', 'split-carry', 'skip', 'fail', 'tight'):
        out.append((f'heap-{hm}', dict(heap_mode=hm, bit=1)))
    out += [('heap-cyclic', dict(heap_mode='cyclic', bit=1, cap=3)), ('heap-custom', dict(heap_mode='fail', custom_heap=True, bit=1)),
            ('bit-other', dict(bit=0xFE)), ('bit-hi', dict(bit=0xFF01)),
            ('mult0', dict(mult=0, bit=1, expect='RETURN')), ('mult-hi-byte', dict(mult=0x1203)), ('w-zero', dict(w0=0, w1=0)),
            ('mul-overflow', dict(w0=0x2000, w1=0x0040, mult=1)), # The first size is (2*w0*w1[*2]*mult + 17h) mod 64K: always odd, never 0, so the first GetMem cannot take its nil
            # path; a large request fails (RunError). The nil path is the second call with mult 0 (case mult0).
            ('size-large-fail', dict(mode=0, w0=0xFFE9, w1=1, mult=1, bit=0, expect='CALLEE/getmem/RUNERROR')),
            ('m1-quarter', dict(mode=1, bit=1, w0=3, w1=7, mult=5)),
            # Mode 0, w0 = w1 = 1: request (size+7)>>4 paragraphs / (size+7)&8 granule = (2,8) for mult 5, (2,0) for mult 3;
            # the defaults request (9,0). Free blocks are (segment offset from HEAP, offset, paragraphs, granule).
            ('gm-para-equal-bigger', dict(bit=0, w0=1, w1=1, mult=5, blocks=[(0x10, 0, 2, 0)])),
            ('gm-para-equal-fit', dict(bit=0, w0=1, w1=1, mult=5, blocks=[(0x10, 0, 2, 8)])),
            ('gm-para-equal-split', dict(bit=0, w0=1, w1=1, mult=3, blocks=[(0x10, 0, 2, 8)])),
            ('gm-top-exact', dict(bit=0, end=(9, 0))),
            ('gm-top-granule-fail', dict(bit=0, w0=1, w1=1, mult=5, end=(2, 0)))]
    if quick:
        return out
    for f in (0x0002, 0x00D7, 0x08D7, 0x0402, 0x0893, 0x0003):
        out.append((f'flags{f:x}', dict(flags=f, bit=1, mode=1)))
    for sp in (0x0040, 0x0041, 0x8001, 0xFFF0):
        out.append((f'sp{sp:x}', dict(sp=sp)))
    out += [('ss-ds', dict(ss=DS, sp=0x8800)), ('a20-stack', dict(ss=0xFFFF, sp=0x0400)),
            # P is HeapPtr itself: storing the first block pointer moves the heap top before the second GetMem.
            ('alias-p-heapptr', dict(pvar=(DS, 0x4404), bit=1, expect='RETURN')),
            # P is on the caller's stack argument area: the stored pointer overwrites the body's own arguments.
            ('alias-p-args', dict(pvar=(SS, 0x8004), bit=1))]
    return out


def differential(code, decoded, lib, quick=False):
    seen, edges, stops, per, splits = set(), set(), {}, {}, []
    specs = cases(quick)
    for name, options in specs:
        options = dict(options); cap = options.pop('cap', 64); expect = options.pop('expect', None)
        status, v, e, split = compare(name, build(code, **options), code, decoded, lib, 1 << 20, cap)
        if expect is not None:
            check(status == expect, f'fixture {name} expected {expect}, got {status}')
        seen |= v; edges |= e; stops[status] = stops.get(status, 0) + 1; per[name] = status
        if split:
            splits.append(split)
    body = {k for k in decoded if k[0] == 'body'}
    branches = {(k, (k[0], t)) for k, i in decoded.items() if conditional(i) for t in (int(i.op_str, 16), i.address + i.size)}
    body_branches = {b for b in branches if b[0][0] == 'body'}
    missing = sorted((a[1], b[1]) for a, b in body_branches - edges)
    sys_missing = sorted((a[1], b[1]) for a, b in (branches - body_branches) - edges)
    sys_start_missing = sorted(k[1] for k in decoded if k[0] == 'sys' and k not in seen)
    if not quick:
        check(set(sys_missing) == set(SYSTEM_EXCLUDED), f'GetMem outcomes not covered/excluded: {[f"{a:04X}->{b:04X}" for a, b in sys_missing]}')
        check(set(sys_start_missing) == set(SYSTEM_EXCLUDED_STARTS), f'GetMem starts not covered/excluded: {[f"{a:04X}" for a in sys_start_missing]}')
        check(body <= seen, f'body starts missing: {sorted(k[1] for k in body - seen)}')
        check(set(missing) == set(EXCLUDED), f'body outcomes not covered/excluded: {[f"{a:04X}->{b:04X}" for a, b in missing]}')
    return dict(cases=len(specs), stops=dict(sorted(stops.items())), non_return={k: v for k, v in per.items() if v != 'RETURN'},
                oracle_splits=splits, body_starts=len(body & seen), body_starts_total=len(body),
                body_outcomes=len(body_branches & edges), body_outcomes_total=len(body_branches),
                closure_starts=len(seen), closure_total=len(decoded), closure_outcomes=len(branches & edges),
                closure_outcomes_total=len(branches),
                system_missing_outcomes=[f'{a:04X}->{b:04X}' for a, b in sys_missing], system_missing_starts=[f'{a:04X}' for a in sys_start_missing],
                cpu_words=14, ram_bytes=SIZE, flag_masks=0)


def main():
    p = argparse.ArgumentParser()
    for flag in ('--compile-only', '--quick', '--unicorn', '--native'):
        p.add_argument(flag, action='store_true')
    p.add_argument('--output', type=Path); a = p.parse_args()
    with tempfile.TemporaryDirectory(prefix='alloc248e-') as tmp:
        lib = library(Path(tmp))
        if a.compile_only:
            print('CHECKER_BUILD_OK'); return
        code, decoded, meta = source(); report = dict(target=meta)
        if a.unicorn:
            report['unicorn'] = differential(code, decoded, lib, a.quick)
        if a.native:
            from res0464_alloc_248e_cpu_native import native
            report['native'] = native(code, lib)
    if a.output:
        a.output.write_text(json.dumps(report, sort_keys=True, indent=2) + '\n')
    print(json.dumps(report, sort_keys=True))


if __name__ == '__main__':
    sys.modules['res0464_alloc_248e_cpu_audit'] = sys.modules[__name__]
    main()
