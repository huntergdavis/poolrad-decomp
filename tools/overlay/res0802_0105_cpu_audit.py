#!/usr/bin/env python3
"""RES_0802_0105 (the status-line string writer) against the original bytes, as a composed closure: the body, SYSTEM StrStore 0AF8:0B25, the
character writer 0802:0040 and Dos.Intr 0A32:000B, which transfers to INT 10h.

The whole loader-expanded START.EXE image is loaded at a test base with the loader's own relocation table applied; every original byte of the closure
runs. Declared environment, identical in every oracle and in the C service callback: the real-byte logging INT 10h stub of the 0040 audit. Unicorn:
CPU14 + full 1 MiB + stop, unmasked. The shared pieces (image loading, the stub, the stub's C emulation, the log reader) are the 0040 audit's. What this
pins: the C model equals the original closure; and the INT 10h register blocks equal an independent Python contract. What it cannot pin: what the BIOS does
with a block: that is run B of res0802_0105_native.py (real DOSBox-X INT 10h, text mode, independent text model). Media stays under ignored resources/."""
import argparse, ctypes, json, os, random, struct, subprocess, sys, tempfile
from hashlib import sha256
from pathlib import Path
from capstone import Cs, CS_ARCH_X86, CS_MODE_16
import res0802_0040_cpu_audit as B
from res0709_ega_reset_0000_cpu_audit import Cpu, ROOT, SIZE, REGISTERS, physical, put16, check
CHECKER_ROWS = ('RES_0802_0105',)
ENTRY, END = 0x0105, 0x0175
DIGEST = '3c9775696f677e8b4711764021cf10172ab0bfb86e4aac681f88c3533a57b842'
BASE, SEG, SS, STUB_SEG, LOG_SEG, FLAGS = B.BASE, B.SEG, B.SS, B.STUB_SEG, B.LOG_SEG, B.FLAGS
SEG = dict(SEG, system=0x0AF8)
MSG_SEG = 0x3000
SCRATCH = 0x4100            # the stub scratch word: past the longest log (4 records per character x 255 = 16,322 bytes), inside the log segment, clear of the image
STATUS = {0: 'RETURNED', 1: 'ARGUMENT', 2: 'DISPLAY', 3: 'CALLEE'}
MEMBERS = [('0802:0040 writer', 'body', 0x0040, 0x0105), ('0A32:000B Intr', 'dos', 0x000B, 0x006C), ('0AF8:0B25 StrStore', 'system', 0x0B25, 0x0B49)]


class Diag(ctypes.Structure):
    _fields_ = [('chars', ctypes.c_uint), ('callee_status', ctypes.c_int), ('intr_status', ctypes.c_int), ('intr_calls', ctypes.c_uint)]


def source(image, sites):
    b = SEG['body'] * 16
    body = image[b + ENTRY:b + END]
    check(sha256(body).hexdigest() == DIGEST, '0802:0105 body identity')
    ins = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(body, ENTRY))
    check(len(ins) == 44 and sum(i.size for i in ins) == len(body), 'decode covers the span')
    check([(i.address, i.mnemonic, i.op_str) for i in ins if 'call' in i.mnemonic] == [(0x11B, 'lcall', '0xaf8, 0xb25'), (0x15F, 'call', '0x40')], 'two callees')
    check([i.address for i in ins if i.mnemonic in ('ret', 'retf')] == [0x172], 'single RETF 0Ah at 0172')
    check(not [i for i in ins if i.mnemonic in ('int', 'in', 'out', 'ljmp')], 'no interrupt or port instruction')
    check({i.address: i.op_str for i in ins if i.mnemonic.startswith('j')} == {0x136: '0x16f', 0x13C: '0x142', 0x16D: '0x13e'}, 'jumps')
    check([a - b for a in sorted(s for s in sites if b + ENTRY <= s < b + END)] == [0x11E], 'the StrStore segment word is the only relocation')
    conditional = {i.address: (i.address + i.size, int(i.op_str, 16)) for i in ins if i.mnemonic.startswith('j') and i.mnemonic != 'jmp'}
    return body, {i.address: i for i in ins}, conditional


def library(directory):
    path = directory / 'r0802_0105.so'
    subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Wpedantic', '-Werror', '-O2', '-fPIC', '-shared', '-I', str(ROOT / 'include')] +
                   [str(ROOT / f'src/recovered/{s}.c') for s in ('resident_0802_0105_cpu', 'resident_0802_0040_cpu', 'dos_intr_000b_cpu', 'system_strstore_fullstate_0b25')] +
                   ['-o', str(path)], check=True)
    f = ctypes.CDLL(str(path)).pr_r0802_0105
    f.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t, ctypes.POINTER(Cpu), ctypes.POINTER(B.Env), ctypes.POINTER(DiagFull)]
    f.restype = ctypes.c_int
    return f


class DiagFull(ctypes.Structure):                                     # pr_r0802_0105_diagnostic
    _fields_ = [('chars', ctypes.c_uint), ('callee_status', ctypes.c_int), ('intr_status', ctypes.c_int), ('intr_calls', ctypes.c_uint)]


def default_regs():
    return (0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0x6666, 0x7777, 0x8888, 0x9999, 0x0246)


def build(image_reloc, stub, rng, *, msg=b'Hi', ega=0, attr=0x12, row=24, col=3, page_a=7, page_b=3, ss=SS, sp=0xF000, flags=2, cpu=None, table=None,
          regs=None, hi=None, msg_at=(MSG_SEG, 0x0100), msg_len=None, log_count=0, background=0x5A):
    m = bytearray([background]) * SIZE
    b = BASE * 16; m[b:b + len(image_reloc)] = image_reloc
    ds = BASE + SEG['data']
    s = physical(STUB_SEG, 0); m[s:s + len(stub)] = stub
    put16(m, 0, 0x40, 0x0000); put16(m, 0, 0x42, STUB_SEG)
    put16(m, LOG_SEG, 0, log_count)
    tab = table if table is not None else [(0xA0 + k * 7 + 3) & 0xFF for k in range(256)]
    for k in range(256):
        m[physical(ds, 0x25D5 + k)] = tab[k]
    m[physical(ds, 0x4A1A)] = page_a; m[physical(ds, 0x4A1B)] = page_b; m[physical(ds, 0x4A1E)] = ega
    for k, v in enumerate(regs or default_regs()):
        put16(m, ds, 0x4A02 + 2 * k, v)
    if msg_at is not None:
        n = len(msg) if msg_len is None else msg_len
        m[physical(msg_at[0], msg_at[1])] = n
        for k, ch in enumerate(msg):
            m[physical(msg_at[0], (msg_at[1] + 1 + k) & 0xFFFF)] = ch
    hi = hi or (0xAB, 0xCD, 0xEF)
    put16(m, ss, sp, 0x7000); put16(m, ss, (sp + 2) & 0xFFFF, 0x9001)
    put16(m, ss, (sp + 4) & 0xFFFF, (msg_at or (0, 0))[1]); put16(m, ss, (sp + 6) & 0xFFFF, (msg_at or (0, 0))[0])
    for k, v in enumerate((attr, row, col)):
        put16(m, ss, (sp + 8 + 2 * k) & 0xFFFF, (hi[k] << 8) | v)
    r = cpu or (0x1234, 0x3456, 0x5678, 0x789A, 0x9ABC, 0xBCDE, 0x7777)
    c = Cpu(*r, sp, BASE + SEG['body'], ENTRY, ds, 0x1111, ss, flags)
    return m, c


def contract(o):
    """The documented behaviour from the inputs only: the INT 10h register blocks, per character, with Regs carried from call to call (the handler
    changes nothing, Intr stores the registers back unchanged, and 0040 sets AH, AL, BH, BL, CX, DX only as listed)."""
    regs = o.get('regs') or default_regs()
    ax, bx, cx, dx, bp, si, di, rds, res = regs[:9]
    msg = o.get('msg', b'Hi'); n = len(msg) if o.get('msg_len') is None else o['msg_len']
    msg = msg[:n] if n <= len(msg) else msg + bytes(n - len(msg))
    ega, attr, row, col = o.get('ega', 0), o.get('attr', 0x12), o.get('row', 24), o.get('col', 3)
    pa, pb = o.get('page_a', 7), o.get('page_b', 3)
    tab = o.get('table') or [(0xA0 + k * 7 + 3) & 0xFF for k in range(256)]
    out = []

    def block(ax_, bx_, cx_, dx_):
        out.append((ax_, bx_, cx_, dx_, si, di, bp, res))
    for ch in msg:
        dx = (row << 8) | col
        ax = (0x02 << 8) | (ax & 0xFF); bx = (pb << 8) | (bx & 0xFF)
        block(ax, bx, cx, dx)
        bl = tab[attr] if ega == 0 else attr
        ax = (0x09 << 8) | ch; bx = (pb << 8) | bl; cx = 1
        block(ax, bx, cx, dx)
        if ega == 1:
            ax = (0x02 << 8) | (ax & 0xFF); bx = (pa << 8) | (bx & 0xFF)
            block(ax, bx, cx, dx)
            ax = (0x09 << 8) | ch; bx = (pa << 8) | attr
            block(ax, bx, cx, dx)
        col = (col + 1) & 0xFF
    return out


def run_unicorn(name, m, c, body_ips, edges, conditional):
    from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE
    from unicorn import x86_const as R
    uc = Uc(UC_ARCH_X86, UC_MODE_16); backing = (ctypes.c_uint8 * SIZE).from_buffer_copy(m)
    uc.mem_map_ptr(0, SIZE, 7, ctypes.addressof(backing)); uc.mem_map_ptr(SIZE, SIZE, 7, ctypes.addressof(backing))
    for (n, _), reg in zip(Cpu._fields_, REGISTERS):
        uc.reg_write(reg, getattr(c, n))
    s = dict(stop=None, steps=0, returned=False, last=None); closure = set(); body_cs = BASE + SEG['body']
    allowed = set(body_ips) | {i.address for i in Cs(CS_ARCH_X86, CS_MODE_16).disasm(m[(body_cs << 4) + 0x40:(body_cs << 4) + 0x105], 0x40)}

    def code(em, address, size, user):
        cs = uc.reg_read(R.UC_X86_REG_CS); ip = uc.reg_read(R.UC_X86_REG_IP)
        if s['returned']:
            s['stop'] = 'RETURNED'; return uc.emu_stop()
        closure.add((cs, ip))
        if s['last'] is not None and s['last'][0] == body_cs and s['last'][1] in conditional:
            edges.add((s['last'][1], ip if cs == body_cs else -1))
        s['last'] = (cs, ip)
        if cs == body_cs:
            check(ip in allowed, f'escape inside 0802 {name} {ip:04x}')
            if ip == 0x172:
                s['returned'] = True
        s['steps'] += 1
        if s['steps'] > 400000:
            s['stop'] = 'RUNAWAY'; uc.emu_stop()
    uc.hook_add(UC_HOOK_CODE, code)
    uc.emu_start(physical(c.cs, c.ip), 2 * SIZE, count=450000)
    check(s['stop'] == 'RETURNED', f'original did not return {name}: {s["stop"]}')
    return uc, backing, closure


def compare(name, state, o, body_ips, f, edges, conditional):
    m, c = state
    uc, backing, closure = run_unicorn(name, m, c, body_ips, edges, conditional)
    actual = Cpu.from_buffer_copy(c); ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(m); keep = []
    env = B.Env(B.c_video(keep, scratch=SCRATCH), None); d = DiagFull()
    status = f(ram, SIZE, ctypes.byref(actual), ctypes.byref(env), ctypes.byref(d))
    check(STATUS.get(status) == 'RETURNED', f'C status {name}: {STATUS.get(status)} chars {d.chars} callee {d.callee_status} intr {d.intr_status}')
    if bytes(ram) != bytes(backing):
        diff = [hex(i) for i, (a, b) in enumerate(zip(bytes(ram), bytes(backing))) if a != b]
        raise ValueError(f'RAM {name}: {diff[:8]} ({len(diff)} bytes)')
    for (n, _), reg in zip(Cpu._fields_, REGISTERS):
        want = uc.reg_read(reg) & 0xFFFF
        check(getattr(actual, n) == want, f'CPU {name}/{n} C {getattr(actual, n):04x} original {want:04x}')
    return closure, actual, bytes(ram), d


def cases():
    rng = random.Random(0x08020105)
    out = []
    pr = lambda n: bytes(rng.randrange(256) for _ in range(n))
    for ega in (0, 1, 2, 0xFF):
        for n in (0, 1, 2, 3, 10, 40):
            out.append((f'ega{ega:02x}-len{n}', dict(msg=pr(n), ega=ega, attr=rng.randrange(256), row=rng.choice((0, 24, 10)), col=rng.choice((0, 39, 5))), True))
    out.append(('len255-ega0', dict(msg=pr(255), ega=0), True)); out.append(('len255-ega1', dict(msg=pr(255), ega=1, col=250), True))
    out.append(('len255-ega2-allbytes', dict(msg=bytes(range(255)), ega=2, attr=0xFF, row=0xFF, col=0xFE), True))
    for col in (0xFD, 0xFE, 0xFF, 0x00):
        out.append((f'col-wrap-{col:02x}', dict(msg=b'wrap!', col=col, ega=rng.choice((0, 1, 2))), True))
    for attr in (0x00, 0x01, 0x0F, 0x12, 0x7F, 0x80, 0xFF):
        for ega in (0, 1):
            out.append((f'attr{attr:02x}-ega{ega}', dict(msg=b'Attr', attr=attr, ega=ega), True))
    for row, col in ((0, 0), (24, 39), (0xFF, 0xFF), (25, 80)):
        out.append((f'rc{row:02x}{col:02x}', dict(msg=b'Pos', row=row, col=col, ega=1), True))
    for pa, pb in ((0, 0), (7, 3), (3, 7), (0xFF, 0), (1, 1)):
        out.append((f'pages{pa:02x}{pb:02x}', dict(msg=b'Pg', page_a=pa, page_b=pb, ega=1), True))
    out += [(f'flags{fl:x}', dict(msg=b'Fl', flags=fl, ega=rng.choice((0, 1))), True) for fl in FLAGS]
    out += [(f'ah-nonzero-flags{fl:04x}', dict(msg=b'ABC', flags=fl, ega=e), True) for fl in (0x0202, 0x0602, 0x0A87, 0x0402, 0x0E57) for e in (0, 1)]   # AH after Intr is the flags' high byte: XOR AH,AH matters
    out += [(f'hi{n}', dict(msg=pr(4), hi=tuple(rng.randrange(256) for _ in range(3)), ega=rng.choice((0, 1, 2))), True) for n in range(6)]
    out += [('sp-min', dict(msg=b'ab', sp=0x0140), False), ('sp-min-plus-1', dict(msg=b'ab', sp=0x0141), False), ('sp-max', dict(msg=b'ab', sp=0xFFF0), False),
            ('sp-odd', dict(msg=b'ab', sp=0xF001), False), ('log-count-15', dict(msg=b'ab', log_count=15, ega=1), False), ('log-count-ffff', dict(msg=b'ab', log_count=0xFFFF, ega=1), False),
            ('msg-in-data-seg', dict(msg=b'DataSeg', msg_at=(BASE + SEG['data'], 0x3000)), False), ('msg-high-offset', dict(msg=b'wrapFFFF', msg_at=(MSG_SEG, 0xFFFC)), False),
            ('msg-a20', dict(msg=b'A20', msg_at=(0xFFFF, 0x0020)), False)]
    # Aliasing: the message source overlaps its own copy at BP-100h (entry SP 0F000h, BP = EFFEh, copy at EEFEh).
    for off in (0xEEFE, 0xEEFF, 0xEEFD, 0xEF10):
        out.append((f'alias-msg-on-copy-{off:04x}', dict(msg=b'Overlap', msg_at=(SS, off), ega=rng.choice((0, 1))), False))
    for k in range(20):
        n = rng.choice((0, 1, 2, 5, 40, rng.randrange(256)))
        out.append((f'random{k}', dict(msg=pr(n), ega=rng.choice((0, 1, 2, rng.randrange(256))), attr=rng.randrange(256), row=rng.randrange(256), col=rng.randrange(256),
                                       page_a=rng.randrange(256), page_b=rng.randrange(256), flags=rng.choice(FLAGS), cpu=tuple(rng.randrange(65536) for _ in range(7)),
                                       regs=tuple(rng.randrange(65536) for _ in range(10)), hi=tuple(rng.randrange(256) for _ in range(3)),
                                       table=[rng.randrange(256) for _ in range(256)]), True))
    return out


def differential(image_reloc, stub, decoded, conditional, f):
    seen_edges, closure = set(), set(); specs = cases(); rng = random.Random(0x08020106); plain = 0; chars_hist = {}
    for name, options, independent in specs:
        state = build(image_reloc, stub, rng, **options)
        cl, final, ram, d = compare(name, state, options, set(decoded), f, seen_edges, conditional); closure |= cl
        chars_hist[d.chars] = chars_hist.get(d.chars, 0) + 1
        if independent:
            got, want = B.read_log(ram), contract(options)
            bad = next((k for k in range(min(len(got), len(want))) if got[k] != want[k]), None)
            check(got == want, f'{name}: INT 10h blocks differ from the contract ({len(got)} vs {len(want)}); first at {bad}: original {got[bad] if bad is not None else None} contract {want[bad] if bad is not None else None}')
            plain += 1
        sp = options.get('sp', 0xF000)
        check(final.sp == (sp + 14) & 0xFFFF and (not independent or (final.cs, final.ip) == (0x9001, 0x7000)), f'{name}: RETF 0Ah')
    wanted = {(a, b) for a, pair in conditional.items() for b in pair}
    check(seen_edges >= wanted, f'conditional outcomes missing: {sorted(wanted - seen_edges)}')
    body_cs = BASE + SEG['body']
    starts = {}
    for label, seg, lo, hi in [('0802:0105 body', 'body', ENTRY, END)] + MEMBERS:
        base = SEG[seg] * 16
        dec = {i.address for i in Cs(CS_ARCH_X86, CS_MODE_16).disasm(image_reloc[base + lo:base + hi], lo)}
        ran = {ip for cs, ip in closure if cs == BASE + SEG[seg] and lo <= ip < hi}
        if seg == 'system':          # StrStore's clamp MOV AL,CL runs only when the source length exceeds the maximum; 0105 passes 255, a length byte never does
            clamp = {i.address for i in Cs(CS_ARCH_X86, CS_MODE_16).disasm(image_reloc[base + lo:base + hi], lo) if i.mnemonic == 'mov' and i.op_str == 'al, cl'}
            check(len(clamp) == 1 and ran == dec - clamp, f'{label}: executed starts {len(ran)} of {len(dec)}; the only start not run must be the clamp {sorted(clamp)}')
            dec = dec - clamp
        else:
            check(ran == dec, f'{label}: executed starts {len(ran)} of {len(dec)} decoded')
        starts[label] = f'{len(ran)}/{len(dec)}' + (' (+1 clamp, unreachable with max 255)' if seg == 'system' else '')
    return dict(cases=len(specs), stops=dict(RETURNED=len(specs)), independent_contract_cases=plain, conditional_outcomes=len(seen_edges & wanted), conditional_outcomes_total=len(wanted),
                closure_member_starts=starts, characters_histogram={str(k): v for k, v in sorted(chars_hist.items())}, cpu_words=14, ram_bytes=SIZE, flag_masks=0)


def main():
    p = argparse.ArgumentParser()
    for flag in ('--compile-only', '--unicorn', '--native'):
        p.add_argument(flag, action='store_true')
    p.add_argument('--output', type=Path); a = p.parse_args()
    with tempfile.TemporaryDirectory(prefix='r0802-0105-') as tmp:
        f = library(Path(tmp))
        if a.compile_only:
            print('CHECKER_BUILD_OK'); return
        image, sites = B.load_image()
        body, decoded, conditional = source(image, sites)
        stub = B.assemble_stub(scratch=SCRATCH)
        image_reloc = B.relocate(image, sites, BASE)
        report = dict(target=dict(span='0802:[0105,0175)', bytes=len(body), instructions=len(decoded), sha256=DIGEST, image_sha256=B.IMAGE_DIGEST, load_base=f'{BASE:04X}',
                                  int10_stub_sha256=sha256(stub).hexdigest()))
        if a.unicorn:
            report['unicorn'] = differential(image_reloc, stub, decoded, conditional, f)
        if a.native:
            import res0802_0105_native
            report['native'] = res0802_0105_native.native(image, sites, decoded, f, sys.modules[__name__], stub)
    if a.output:
        a.output.write_text(json.dumps(report, sort_keys=True, indent=2) + '\n')
    print(json.dumps(report, sort_keys=True))


if __name__ == '__main__':
    main()
