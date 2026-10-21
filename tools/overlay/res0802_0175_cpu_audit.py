#!/usr/bin/env python3
"""RES_0802_0175 (the block filler; side (ii), NON-CREDIT) against the original bytes, as a composed closure: the body, the character writer 0802:0040 and
Dos.Intr 0A32:000B, which transfers to INT 10h. Same method as the 0105 audit: the whole loader-expanded START.EXE image at a test base with the loader's own
relocations applied, a real-byte logging INT 10h stub (the 0040 audit's, scratch word at 4100h), Unicorn CPU14 + full 1 MiB + stop, unmasked, and an
independent Python contract of the INT 10h register blocks. What it cannot pin is what the BIOS does with a block: that is run B of res0802_0175_native.py.
Media stays under ignored resources/."""
import argparse, ctypes, json, os, random, struct, subprocess, sys, tempfile
from hashlib import sha256
from pathlib import Path
from capstone import Cs, CS_ARCH_X86, CS_MODE_16
import res0802_0040_cpu_audit as B
from res0709_ega_reset_0000_cpu_audit import Cpu, ROOT, SIZE, REGISTERS, physical, put16, check
CHECKER_ROWS = ('RES_0802_0175',)
ENTRY, END = 0x0175, 0x0193
DIGEST = '8f590b56a4323310803fe56a823e839504857c6d583789cbf0f69ec34e0b0228'
BASE, SEG, SS, STUB_SEG, LOG_SEG, FLAGS = B.BASE, B.SEG, B.SS, B.STUB_SEG, B.LOG_SEG, B.FLAGS
SCRATCH = 0x4100
# The standing set of DIRTY incoming flag images (a mutant that drops a flag-clearing instruction is invisible under clear flags): IF, DF and OF set, no TF.
DIRTY_FLAGS = (0x0202, 0x0602, 0x0A87, 0x0402, 0x0E57)
STATUS = {0: 'RETURNED', 1: 'ARGUMENT', 2: 'DISPLAY', 3: 'CALLEE'}
MEMBERS = [('0802:0040 writer', 'body', 0x0040, 0x0105), ('0A32:000B Intr', 'dos', 0x000B, 0x006C)]


class DiagFull(ctypes.Structure):                                     # pr_r0802_0175_diagnostic
    _fields_ = [('callee_status', ctypes.c_int), ('intr_status', ctypes.c_int), ('intr_calls', ctypes.c_uint)]


def source(image, sites):
    b = SEG['body'] * 16
    body = image[b + ENTRY:b + END]
    check(sha256(body).hexdigest() == DIGEST, '0802:0175 body identity')
    ins = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(body, ENTRY))
    check(len(ins) == 16 and sum(i.size for i in ins) == len(body), 'decode covers the span')
    check([(i.address, i.mnemonic, i.op_str) for i in ins if 'call' in i.mnemonic] == [(0x18C, 'call', '0x40')], 'one callee: the near CALL 0040')
    check([i.address for i in ins if i.mnemonic in ('ret', 'retf')] == [0x190], 'single RETF 8 at 0190')
    check(not [i for i in ins if i.mnemonic in ('int', 'in', 'out', 'ljmp', 'lcall') or i.mnemonic.startswith('j')], 'no jump, far call, interrupt or port instruction')
    check(not [s for s in sites if b + ENTRY <= s < b + END], 'no loader relocation in the span')
    check([(i.address, i.op_str) for i in ins if i.mnemonic == 'mov' and i.op_str == 'al, 0xdb'] == [(0x188, 'al, 0xdb')], 'the constant block character DBh')
    return body, {i.address: i for i in ins}


def library(directory):
    path = directory / 'r0802_0175.so'
    subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Wpedantic', '-Werror', '-O2', '-fPIC', '-shared', '-I', str(ROOT / 'include')] +
                   [str(ROOT / f'src/recovered/{s}.c') for s in ('resident_0802_0175_cpu', 'resident_0802_0040_cpu', 'dos_intr_000b_cpu')] + ['-o', str(path)], check=True)
    f = ctypes.CDLL(str(path)).pr_r0802_0175
    f.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t, ctypes.POINTER(Cpu), ctypes.POINTER(B.Env), ctypes.POINTER(DiagFull)]
    f.restype = ctypes.c_int
    return f


def default_regs():
    return (0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0x6666, 0x7777, 0x8888, 0x9999, 0x0246)


def build(image_reloc, stub, rng, *, ega=0, col=0, row=24, attr=0, count=40, page_a=7, page_b=3, ss=SS, sp=0xF000, flags=2, cpu=None, table=None, regs=None,
          hi=None, log_count=0, background=0x5A):
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
    hi = hi or (0xAB, 0xCD, 0xEF, 0x12)
    put16(m, ss, sp, 0x7000); put16(m, ss, (sp + 2) & 0xFFFF, 0x9001)
    for k, v in enumerate((count, attr, row, col)):                    # [SP+4] count, [SP+6] attribute, [SP+8] row, [SP+0Ah] column
        put16(m, ss, (sp + 4 + 2 * k) & 0xFFFF, (hi[k] << 8) | v)
    r = cpu or (0x1234, 0x3456, 0x5678, 0x789A, 0x9ABC, 0xBCDE, 0x7777)
    c = Cpu(*r, sp, BASE + SEG['body'], ENTRY, ds, 0x1111, ss, flags)
    return m, c


def contract(o):
    """The documented behaviour from the inputs only: the INT 10h register blocks of one 0040 call with character DBh and the forwarded count."""
    regs = o.get('regs') or default_regs()
    ax, bx, cx, dx, bp, si, di, rds, res = regs[:9]
    ega, attr, row, col, count = o.get('ega', 0), o.get('attr', 0), o.get('row', 24), o.get('col', 0), o.get('count', 40)
    pa, pb = o.get('page_a', 7), o.get('page_b', 3)
    tab = o.get('table') or [(0xA0 + k * 7 + 3) & 0xFF for k in range(256)]
    out = []

    def block(ax_, bx_, cx_, dx_):
        out.append((ax_, bx_, cx_, dx_, si, di, bp, res))
    dx = (row << 8) | col
    ax = (0x02 << 8) | (ax & 0xFF); bx = (pb << 8) | (bx & 0xFF)
    block(ax, bx, cx, dx)
    bl = tab[attr] if ega == 0 else attr
    ax = (0x09 << 8) | 0xDB; bx = (pb << 8) | bl; cx = count
    block(ax, bx, cx, dx)
    if ega == 1:
        ax = (0x02 << 8) | 0xDB; bx = (pa << 8) | bl
        block(ax, bx, cx, dx)
        ax = (0x09 << 8) | 0xDB; bx = (pa << 8) | attr
        block(ax, bx, cx, dx)
    return out


def run_unicorn(name, m, c, body_ips):
    from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE
    from unicorn import x86_const as R
    uc = Uc(UC_ARCH_X86, UC_MODE_16); backing = (ctypes.c_uint8 * SIZE).from_buffer_copy(m)
    uc.mem_map_ptr(0, SIZE, 7, ctypes.addressof(backing)); uc.mem_map_ptr(SIZE, SIZE, 7, ctypes.addressof(backing))
    for (n, _), reg in zip(Cpu._fields_, REGISTERS):
        uc.reg_write(reg, getattr(c, n))
    s = dict(stop=None, steps=0, returned=False); closure = set(); body_cs = BASE + SEG['body']
    allowed = set(body_ips) | {i.address for i in Cs(CS_ARCH_X86, CS_MODE_16).disasm(m[(body_cs << 4) + 0x40:(body_cs << 4) + 0x105], 0x40)}

    def code(em, address, size, user):
        cs = uc.reg_read(R.UC_X86_REG_CS); ip = uc.reg_read(R.UC_X86_REG_IP)
        if s['returned']:
            s['stop'] = 'RETURNED'; return uc.emu_stop()
        closure.add((cs, ip))
        if cs == body_cs:
            check(ip in allowed, f'escape inside 0802 {name} {ip:04x}')
            if ip == 0x190:
                s['returned'] = True
        s['steps'] += 1
        if s['steps'] > 20000:
            s['stop'] = 'RUNAWAY'; uc.emu_stop()
    uc.hook_add(UC_HOOK_CODE, code)
    uc.emu_start(physical(c.cs, c.ip), 2 * SIZE, count=30000)
    check(s['stop'] == 'RETURNED', f'original did not return {name}: {s["stop"]}')
    return uc, backing, closure


def compare(name, state, o, body_ips, f):
    m, c = state
    uc, backing, closure = run_unicorn(name, m, c, body_ips)
    actual = Cpu.from_buffer_copy(c); ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(m); keep = []
    env = B.Env(B.c_video(keep, scratch=SCRATCH), None); d = DiagFull()
    status = f(ram, SIZE, ctypes.byref(actual), ctypes.byref(env), ctypes.byref(d))
    check(STATUS.get(status) == 'RETURNED', f'C status {name}: {STATUS.get(status)} callee {d.callee_status} intr {d.intr_status}')
    if bytes(ram) != bytes(backing):
        diff = [hex(i) for i, (a, b) in enumerate(zip(bytes(ram), bytes(backing))) if a != b]
        raise ValueError(f'RAM {name}: {diff[:8]} ({len(diff)} bytes)')
    for (n, _), reg in zip(Cpu._fields_, REGISTERS):
        want = uc.reg_read(reg) & 0xFFFF
        check(getattr(actual, n) == want, f'CPU {name}/{n} C {getattr(actual, n):04x} original {want:04x}')
    return closure, actual, bytes(ram), d


def cases():
    rng = random.Random(0x08020175)
    out = []
    for ega in (0, 1, 2, 0xFF):
        out.append((f'zero-call-ega{ega:02x}', dict(ega=ega), True))                 # the 0193 call: column 0, row 24, attribute 0, 40 blocks
        for attr in (0x00, 0x01, 0x0F, 0x12, 0x7F, 0x80, 0xFF):
            out.append((f'attr{attr:02x}-ega{ega:02x}', dict(ega=ega, attr=attr, col=rng.randrange(80), row=rng.randrange(25), count=1 + rng.randrange(40)), True))
    for attr in range(256):
        out.append((f'tab-ega0-attr{attr:02x}', dict(ega=0, attr=attr), True))
    for count in (0, 1, 2, 0x28, 0x4F, 0x50, 0xFF):
        for ega in (0, 1):
            out.append((f'count{count:02x}-ega{ega}', dict(count=count, ega=ega), True))
    for row, col in ((0, 0), (24, 39), (24, 79), (0xFF, 0xFF), (25, 80)):
        out.append((f'rc{row:02x}{col:02x}', dict(row=row, col=col, ega=1), True))
    for pa, pb in ((0, 0), (7, 3), (3, 7), (0xFF, 0), (1, 1)):
        out.append((f'pages{pa:02x}{pb:02x}', dict(page_a=pa, page_b=pb, ega=1), True))
    out += [(f'flags{fl:04x}-ega{e}', dict(flags=fl, ega=e), True) for fl in FLAGS + DIRTY_FLAGS for e in (0, 1)]
    out += [(f'hi{n}', dict(hi=tuple(rng.randrange(256) for _ in range(4)), ega=rng.choice((0, 1, 2))), True) for n in range(6)]
    out += [('sp-min', dict(sp=0x0040), False), ('sp-min-plus-1', dict(sp=0x0041), False), ('sp-max', dict(sp=0xFFF4), False), ('sp-odd', dict(sp=0xF001), False),
            ('log-count-15', dict(log_count=15, ega=1), False), ('log-count-ffff', dict(log_count=0xFFFF, ega=1), False)]
    ds_ss = BASE + SEG['data']
    out += [(f'alias-slots-on-regs-{sp:04x}', dict(ss=ds_ss, sp=sp, ega=e), False) for sp in (0x4A00, 0x4A02, 0x49FE, 0x4A04) for e in (0, 1)]
    out += [(f'alias-attr-slot-on-table-{sp:04x}', dict(ss=ds_ss, sp=sp, ega=0), False) for sp in (0x25CF, 0x25D3, 0x2600)]
    for k in range(20):
        out.append((f'random{k}', dict(ega=rng.choice((0, 1, 2, rng.randrange(256))), col=rng.randrange(256), row=rng.randrange(256), attr=rng.randrange(256), count=rng.randrange(256),
                                       page_a=rng.randrange(256), page_b=rng.randrange(256), flags=rng.choice(FLAGS + DIRTY_FLAGS), cpu=tuple(rng.randrange(65536) for _ in range(7)),
                                       regs=tuple(rng.randrange(65536) for _ in range(10)), hi=tuple(rng.randrange(256) for _ in range(4)),
                                       table=[rng.randrange(256) for _ in range(256)]), True))
    return out


def differential(image_reloc, stub, decoded, f):
    closure = set(); specs = cases(); rng = random.Random(0x08020176); plain = 0; calls = {}
    for name, options, independent in specs:
        state = build(image_reloc, stub, rng, **options)
        cl, final, ram, d = compare(name, state, options, set(decoded), f); closure |= cl
        calls[d.intr_calls] = calls.get(d.intr_calls, 0) + 1
        if independent:
            got, want = B.read_log(ram), contract(options)
            check(got == want, f'{name}: INT 10h blocks differ from the contract: original {got} contract {want}')
            plain += 1
        sp = options.get('sp', 0xF000)
        check(final.sp == (sp + 12) & 0xFFFF and (not independent or (final.cs, final.ip) == (0x9001, 0x7000)), f'{name}: RETF 8')
    starts = {}
    for label, seg, lo, hi in [('0802:0175 body', 'body', ENTRY, END)] + MEMBERS:
        base = SEG[seg] * 16
        dec = {i.address for i in Cs(CS_ARCH_X86, CS_MODE_16).disasm(image_reloc[base + lo:base + hi], lo)}
        ran = {ip for cs, ip in closure if cs == BASE + SEG[seg] and lo <= ip < hi}
        check(ran == dec, f'{label}: executed starts {len(ran)} of {len(dec)} decoded')
        starts[label] = f'{len(ran)}/{len(dec)}'
    return dict(cases=len(specs), stops=dict(RETURNED=len(specs)), independent_contract_cases=plain, closure_member_starts=starts, conditional_outcomes_total=0,
                intr_calls_histogram={str(k): v for k, v in sorted(calls.items())}, dirty_flag_images=[f'{x:04X}' for x in DIRTY_FLAGS], cpu_words=14, ram_bytes=SIZE, flag_masks=0)


def main():
    p = argparse.ArgumentParser()
    for flag in ('--compile-only', '--unicorn', '--native'):
        p.add_argument(flag, action='store_true')
    p.add_argument('--output', type=Path); a = p.parse_args()
    with tempfile.TemporaryDirectory(prefix='r0802-0175-') as tmp:
        f = library(Path(tmp))
        if a.compile_only:
            print('CHECKER_BUILD_OK'); return
        image, sites = B.load_image()
        body, decoded = source(image, sites)
        stub = B.assemble_stub(scratch=SCRATCH)
        image_reloc = B.relocate(image, sites, BASE)
        report = dict(target=dict(span='0802:[0175,0193)', bytes=len(body), instructions=len(decoded), sha256=DIGEST, image_sha256=B.IMAGE_DIGEST, load_base=f'{BASE:04X}',
                                  int10_stub_sha256=sha256(stub).hexdigest(), class_='side (ii) non-credit; no identity'))
        if a.unicorn:
            report['unicorn'] = differential(image_reloc, stub, decoded, f)
        if a.native:
            import res0802_0175_native
            report['native'] = res0802_0175_native.native(image, sites, decoded, f, sys.modules[__name__], stub)
    if a.output:
        a.output.write_text(json.dumps(report, sort_keys=True, indent=2) + '\n')
    print(json.dumps(report, sort_keys=True))


if __name__ == '__main__':
    main()
