#!/usr/bin/env python3
"""Original-byte full-state parity for GAME.OVR unit 0x06C [2033,206E) (59 B / 23 instructions).

The body branches on an SS byte: below 6 it reaches the far call at 204Ch
(load-zero fixup 0085:002F, unit 0x77 entry 3), which is OUT OF SCOPE and declared
a boundary, so the differential stops at the call site and compares the state the
model hands over; 6 or above it pushes the active far pointer DS:[5D94]:[5D92],
x 4, y 012h and [bp+8] into the accepted composed resident 00BA:0973 model and
returns through RETF 4. The oracle executes the real expanded START.EXE image and
the real GAME.OVR body bytes; the candidate side composes the C 0973 model (itself
composing the C 1037 formatter and 0709:0626 printer). Compared: CPU14, all RAM
outside A0000..AFFFF, the ordered OUT events and the ordered A0000 write operands,
plus every instruction start and conditional outcome of the 59-byte body. Media
stays under ignored resources/."""
import argparse
import ctypes
import json
import random
import subprocess
import tempfile
from hashlib import sha256
from pathlib import Path

import resident_0973_cpu_audit as R
import res0709_printer_0626_cpu_audit as P
from capstone import CS_ARCH_X86, CS_MODE_16, Cs
from overlay_inventory import find_units, parse_mz
from u86_procvars_3413_audit import Cpu, ROOT, SIZE, EXE_SHA, OVR_SHA, REGISTERS, physical, put16

CHECKER_ROWS = ('OVL_006C_2033',)

LOADED_SEG = 0x0078            # unit 0x06C loaded memory segment (inventory: next descriptor)
RUN_SEG = 0x7800               # the differential runs a relocated copy: the body has no CS-relative
                               # data, and 0x0078:2033 would overlap the resident image held at load 0
ENTRY, END, RETF = 0x2033, 0x206E, 0x206B
CALL_LOW, CALL_HIGH = 0x204C, 0x2065
BODY_SHA = '9d2095e546eea202d4fc5b9e42490735020536df5b63fe1e0dd6a85a715d9add'
RESIDENT_ALLOWED = (0x00BA, 0x0AF8, 0x0709)
NODE_WORD = 0x1000             # [bp+6]: word offset in SS of the node record
RETURN = (0x9001, 0x7000)      # caller frame far address the body returns to

check = R.check


def source():
    exe = (ROOT / 'resources/DOS/POOLRAD/START.EXE').read_bytes()
    ovr = (ROOT / 'resources/DOS/POOLRAD/GAME.OVR').read_bytes()
    check(len(exe) == 65231 and sha256(exe).hexdigest() == EXE_SHA, 'START.EXE provenance differs')
    check(sha256(ovr).hexdigest() == OVR_SHA, 'GAME.OVR provenance differs')
    spec, image = R.source()                     # validates the composed resident spans
    unit = next(u for u in find_units(exe[parse_mz(exe)['header_bytes']:], ovr) if u['stub_para'] == 0x6C)
    check(unit['memory_segment'] == LOADED_SEG, 'unit 0x6C memory segment differs')
    start = unit['ovr_offset'] + ENTRY
    body = ovr[start:start + (END - ENTRY)]
    md = Cs(CS_ARCH_X86, CS_MODE_16)
    ins = list(md.disasm(body, ENTRY))
    check(len(body) == END - ENTRY and sha256(body).hexdigest() == BODY_SHA, 'OVL_006C_2033 extent differs')
    check(len(ins) == 23 and sum(i.size for i in ins) == len(body), 'OVL_006C_2033 instruction count differs')
    check([(i.address, i.mnemonic, i.op_str) for i in ins if 'call' in i.mnemonic] ==
          [(CALL_LOW, 'lcall', '0x85, 0x2f'), (CALL_HIGH, 'lcall', '0xba, 0x973')], 'call sites differ')
    check([(i.address, i.op_str) for i in ins if i.mnemonic.startswith('j')] ==
          [(0x203E, '0x2053'), (0x2051, '0x206a')], 'branch structure differs')
    check([(i.address, i.op_str) for i in ins if i.mnemonic == 'retf'] == [(RETF, '4')], 'return differs')
    check(not [i for i in ins if i.mnemonic in ('in', 'out', 'int')], 'body performs I/O')
    conditional = {i.address: (i.address + i.size, int(i.op_str, 16))
                   for i in ins if i.mnemonic.startswith('j') and i.mnemonic != 'jmp'}
    print(f'OVL_006C_2033 ({len(body)} bytes / {len(ins)} instructions) sha {sha256(body).hexdigest()}')
    return dict(body=body, instructions=len(ins), sha=BODY_SHA, conditional=conditional,
                starts=[i.address for i in ins]), image


def library(directory):
    path = directory / 'u6c.so'
    sources = ['overlay_u6c_2033_cpu.c', 'resident_00ba_0973_cpu.c',
               'resident_00ba_byte_string_1037_cpu.c', 'system_str_1366_cpu.c'] + \
              [name + '.c' for name in P.SOURCES]
    built = subprocess.run(['cc', '-std=c11', '-O2', '-fPIC', '-shared', '-I', str(ROOT / 'include'),
                            *[str(ROOT / 'src/recovered' / s) for s in sources], '-o', str(path)],
                          capture_output=True, text=True)
    check(built.returncode == 0, built.stderr[-2000:])
    lib = ctypes.CDLL(str(path))
    lib.pr_ega_reset.argtypes = [ctypes.POINTER(P.Ega)]
    f = lib.pr_ovl_006c_2033_cpu
    f.argtypes = [ctypes.POINTER(P.Bus), ctypes.c_size_t, ctypes.POINTER(Cpu), ctypes.c_void_p, ctypes.c_void_p]
    f.restype = ctypes.c_int
    return lib, f


def build(name, *, first=0x10, arg2=0, node_word=NODE_WORD, sp=0x8000, flags=0x0202,
          node_seg=R.NODE_SEG, node_off=R.NODE_OFF):
    spec, image = SOURCE
    m = bytearray([0xA5]) * SIZE
    m[0:len(image)] = image
    body = spec['body']
    m[physical(RUN_SEG, ENTRY):physical(RUN_SEG, ENTRY) + len(body)] = body
    # the composed resident world, exactly as the accepted 0973 differential sets it
    put16(m, R.DS, 0x4A16, 0xA000); put16(m, R.DS, 0x4A18, 0xA200)
    put16(m, R.DS, 0x5E20, R.FONT_OFF); put16(m, R.DS, 0x5E22, R.FONT_SEG)
    for j in range(2048):
        m[physical(R.FONT_SEG, R.FONT_OFF + j)] = ((j * 37 + (j >> 3) * 13) ^ 0xA6) & 255
    for j, v in enumerate((1, 2, 4, 8)):
        m[physical(R.DS, 0x25C9 + j)] = v
    for j, v in enumerate((0, 0x5555, 0xAAAA, 0xFFFF)):
        put16(m, R.DS, 0x25CD + 2 * j, v)
    for j in range(256):
        m[physical(R.DS, 0x25D5 + j)] = j % 4
    for j in range(8):
        put16(m, R.DS, 0x25E5 + 2 * j, 0xC000 >> (2 * j))
    m[physical(R.DS, 0x5E24)], m[physical(R.DS, 0x5E25)] = 1, 1
    m[physical(R.DS, 0x4A00)] = 0; m[physical(R.DS, 0x49E2)] = 0
    put16(m, R.DS, 0x84FC, 2); m[0x400] = 0x42
    m[physical(R.DS, 0x4A1E)] = 1
    # the active far pointer the else branch hands to 0973
    put16(m, R.DS, 0x5D92, node_off); put16(m, R.DS, 0x5D94, node_seg)
    # the node bytes 0973 reads, and the byte this body compares
    m[physical(node_seg, (node_off + 0x11B) & 0xFFFF)] = rng_byte(name, 0x11B)
    m[physical(node_seg, (node_off + 0x32) & 0xFFFF)] = rng_byte(name, 0x32)
    m[physical(R.SS, (node_word - 0x0D) & 0xFFFF)] = first
    # the caller frame: far return, then two argument words
    put16(m, R.SS, sp, RETURN[0])
    put16(m, R.SS, (sp + 2) & 0xFFFF, RETURN[1])
    put16(m, R.SS, (sp + 4) & 0xFFFF, node_word)
    put16(m, R.SS, (sp + 6) & 0xFFFF, 0xA500 | arg2)
    c = Cpu(0x1234, 0x3456, 0x5678, 0x789A, 0x9ABC, 0xBCDE, 0x7777, sp, RUN_SEG, ENTRY,
            R.DS, 0x1111, R.SS, (flags | 0x0002) & ~0x0100)
    return m, c


def rng_byte(name, offset):
    return random.Random(f'OVL_006C_2033/{name}/{offset:04X}').randrange(256)


def run_original(name, m, c, spec):
    from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE, UC_HOOK_INSN, UC_HOOK_MEM_WRITE
    from unicorn import x86_const as Reg
    uc = Uc(UC_ARCH_X86, UC_MODE_16)
    backing = (ctypes.c_uint8 * SIZE).from_buffer_copy(m)
    uc.mem_map_ptr(0, SIZE, 7, ctypes.addressof(backing))
    uc.mem_map_ptr(SIZE, SIZE, 7, ctypes.addressof(backing))
    for (n, _), r in zip(Cpu._fields_, REGISTERS):
        uc.reg_write(r, getattr(c, n))
    st = dict(stop=None, returned=False, boundary=False, error=None)
    seen, edges, events, writes = set(), set(), [], []
    prev = [None]

    def stop(value):
        st['stop'] = value
        uc.emu_stop()

    def hook(em, address, size, user):
        cs, ip = uc.reg_read(Reg.UC_X86_REG_CS), uc.reg_read(Reg.UC_X86_REG_IP)
        if (cs, ip) == (RETURN[1], RETURN[0]):
            st['returned'] = True
            return stop(0)
        if cs not in (RUN_SEG,) + RESIDENT_ALLOWED:
            st['error'] = (cs, ip)
            return stop(99)
        if cs == RUN_SEG and ip == CALL_LOW:
            # stop BEFORE the far call: the hand-over state is the model's boundary.
            # The call-site instruction itself is reached, so count it as visited.
            seen.add(ip)
            st['boundary'] = True
            return stop(1)
        if cs == RUN_SEG and ip in spec['starts']:
            if prev[0] in spec['conditional'] and prev[0] != ip:
                edges.add((prev[0], ip))
            seen.add(ip)
            prev[0] = ip if spec['conditional'] else None
        else:
            prev[0] = None
        return None

    def out(em, port, size, value, user):
        events.append((port, value, size))

    def write(em, access, address, size, value, user):
        for j in range(size):
            at = (address + j) & (SIZE - 1)
            if 0xA0000 <= at < 0xB0000:
                writes.append((at - 0xA0000, (value >> (8 * j)) & 255))

    uc.hook_add(UC_HOOK_CODE, hook)
    uc.hook_add(UC_HOOK_INSN, out, None, 1, 0, Reg.UC_X86_INS_OUT)
    uc.hook_add(UC_HOOK_MEM_WRITE, write)
    uc.emu_start(physical(c.cs, c.ip), 2 * SIZE, count=3_000_000)
    check(st['error'] is None and st['stop'] is not None, f'original did not stop cleanly: {name} {st}')
    final = {n: uc.reg_read(r) & 0xFFFF for (n, _), r in zip(Cpu._fields_, REGISTERS)}
    return bytes(backing), final, seen, edges, events, writes, st


def run_c(f, lib, m, c):
    ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(m)
    ega = P.Ega()
    lib.pr_ega_reset(ctypes.byref(ega))
    for p in range(4):
        ctypes.memset(ctypes.addressof(ega.plane[p]), 0x31 + p * 0x27, 65536)
    events, writes = [], []

    def out(ctx, port, value, size, cs, ip):
        events.append((port, value, size))
        return 0

    def wr(ctx, off, value):
        writes.append((off, value))

    env = P.Env(None, P.OUT_FN(out), P.IN_FN())
    bus = P.Bus(ram, ctypes.pointer(ega), ctypes.pointer(env), P.WRITE_FN(wr), None)
    c2 = Cpu(*[getattr(c, n) for n, _ in Cpu._fields_])
    status = f(ctypes.byref(bus), SIZE, ctypes.byref(c2), None, None)
    return status, bytes(ram), events, writes, c2


def compare(name, m, c, spec, lib, f):
    expected, final, seen, edges, events, writes, st = run_original(name, m, c, spec)
    status, actual, aevents, awrites, c2 = run_c(f, lib, m, c)
    want = 3 if st['boundary'] else 0            # PR_U6C_2033_BOUNDARY / RETURNED
    check(status == want, f'C status {status}, oracle boundary={st["boundary"]}: {name}')
    diffs = [i for i in range(SIZE) if not (0xA0000 <= i < 0xB0000) and actual[i] != expected[i]]
    if diffs:
        raise ValueError(f'RAM mismatch {name}: {[(hex(d), actual[d], expected[d]) for d in diffs[:16]]} '
                         f'({len(diffs)} bytes) CPU={ {n: (getattr(c2, n), final[n]) for n, _ in Cpu._fields_ if getattr(c2, n) != final[n]} }')
    for n, _ in Cpu._fields_:
        check(getattr(c2, n) == final[n], f'CPU mismatch {name}/{n}: {getattr(c2, n):04x}/{final[n]:04x}')
    check(aevents == events, f'OUT mismatch {name}: {aevents[:4]} / {events[:4]}')
    check(awrites == writes, f'VRAM mismatch {name}: {awrites[:4]} / {writes[:4]} ({len(awrites)}/{len(writes)})')
    return seen, edges, len(events), len(writes), want == 3


def cases(quick=False):
    out = []
    for first in (0, 1, 5, 6, 7, 0x80, 0xFF):
        for arg2 in (0, 0x12):
            out.append((f'first{first:02x}-arg{arg2:02x}', dict(first=first, arg2=arg2)))
    out += [('arg-ff', dict(first=0x10, arg2=0xFF)),
            ('arg-01', dict(first=0x10, arg2=0x01)),
            ('node-seg-7000', dict(first=0x10, node_seg=0x7000)),
            ('node-off-wrap', dict(first=0x10, node_off=0xFFF0)),
            ('node-word-wrap', dict(first=0x10, node_word=0x0005)),
            ('sp-8100', dict(first=0x10, sp=0x8100)),
            ('sp-0f00', dict(first=0x10, sp=0x0F00))]
    if quick:
        out = out[:8]
    return out


SOURCE = None


def main():
    global SOURCE
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--compile-only', action='store_true', help='build and bind the candidate library only; no media')
    parser.add_argument('--unicorn', action='store_true')
    parser.add_argument('--quick', action='store_true')
    parser.add_argument('--output', type=Path)
    args = parser.parse_args()
    with tempfile.TemporaryDirectory(prefix='poolrad-006c-') as folder:
        lib, f = library(Path(folder))
        if args.compile_only:
            print('CHECKER_BUILD_OK')
            return
        spec, image = source()
        SOURCE = (spec, image)
        report = dict(target='OVL_006C_2033', bytes=len(spec['body']), instructions=spec['instructions'],
                      sha256=spec['sha'], cases=0, ram_bytes=SIZE - 65536, cpu_words=14)
        seen, edges, outs, vram, boundary = set(), set(), 0, 0, 0
        for name, opts in cases(args.quick):
            m, c = build(name, **opts)
            s, e, o, w, is_boundary = compare(name, m, c, spec, lib, f)
            seen |= s; edges |= e; outs += o; vram += w; report['cases'] += 1
            boundary += int(is_boundary)
        outcomes = {(a, b) for a, dests in spec['conditional'].items() for b in dests}
        uncovered_starts = [f'{ip:04X}' for ip in sorted(set(spec['starts']) - seen)]
        uncovered_outcomes = [f'{a:04X}->{b:04X}' for a, b in sorted(outcomes - edges)]
        # 2051 is the jmp after the declared boundary call: with the 0085:002F callee
        # out of scope it is unreachable by construction, and that is the only permitted gap.
        check(uncovered_starts == ['2051'], f'unexpected uncovered starts: {uncovered_starts}')
        check(not uncovered_outcomes, f'uncovered conditional outcomes: {uncovered_outcomes}')
        report.update(caller_starts=len(seen), caller_starts_total=len(spec['starts']), caller_outcomes=len(edges),
                      uncovered_starts=uncovered_starts,
                      uncovered_starts_expected=['2051'],
                      uncovered_outcomes=uncovered_outcomes,
                      out_events=outs, vram_write_operands=vram, boundary_cases=boundary)
        for row in CHECKER_ROWS:
            print(f'ROW_EXECUTED {row} PASS')
        text = json.dumps(report, indent=1, sort_keys=True)
        if args.output:
            args.output.parent.mkdir(parents=True, exist_ok=True)
            args.output.write_text(text + '\n')
        print(text)


if __name__ == '__main__':
    main()
