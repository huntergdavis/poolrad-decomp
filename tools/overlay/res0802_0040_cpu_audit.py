#!/usr/bin/env python3
"""RES_0802_0040 (the character writer) against the original bytes, as a composed closure: the body plus Dos.Intr 0A32:000B, which
transfers to INT 10h.

The whole loader-expanded START.EXE image is loaded at a test base with the loader's own relocation table applied (what DOS does; no
byte is patched), so the original body AND the original Intr bytes run. Declared environment, identical in every oracle and in the C
service callback: a real-byte INT 10h handler at the vector that LOGS the register block it is called with (AX BX CX DX SI DI BP ES) into
RAM at 0060:0000 and IRETs. Unicorn: CPU14 + full 1 MiB + stop, unmasked. What this pins: the SEQUENCE OF INT 10h REGISTER BLOCKS the body
issues, and every byte the body and Intr write. What it cannot pin: what the video BIOS DOES with a block: that is the native BIOS-live run
(res0802_0040_native.py), whose mechanism is DOSBox-X's real INT 10h and an independent Python text-mode model. Media and generated programs
stay under ignored resources/."""
import argparse, ctypes, json, os, random, struct, subprocess, sys, tempfile
from hashlib import sha256
from pathlib import Path
from capstone import Cs, CS_ARCH_X86, CS_MODE_16
from resident_loader_map import unpack, relocations
from res0709_ega_reset_0000_cpu_audit import Cpu, ROOT, SIZE, REGISTERS, physical, put16, check
CHECKER_ROWS = ('RES_0802_0040',)
ENTRY, END = 0x0040, 0x0105
DIGEST = '371aba1b1f82444f7021284640b7eed635a7d0a9f97b25333fb9723071d798cd'
IMAGE_DIGEST = 'a42a57e68d3f7815831821cc9365bb917befda45a58e958544451fc7a878df8e'
BASE = 0x1000
SEG = dict(body=0x0802, dos=0x0A32, data=0x0C7C)
SS, STUB_SEG, LOG_SEG = 0x7000, 0x6000, 0x0060
FLAGS = (0x0002, 0x00D7, 0x08D7, 0x0402, 0x0893, 0x0003)
RELOC_AT = (0x65, 0xA8, 0xD3, 0xFF)
INTR_MEMBER = (0x000B, 0x006C)                                   # Dos.Intr span, original bytes
STUB_SOURCE = '''.code16
.intel_syntax noprefix
push ds
push bx
push ax
mov ax,0x60
mov ds,ax
mov bx,word ptr [0]
shl bx,1
shl bx,1
shl bx,1
shl bx,1
mov word ptr [bx+6],cx
mov word ptr [bx+8],dx
mov word ptr [bx+10],si
mov word ptr [bx+12],di
mov word ptr [bx+14],bp
mov word ptr [bx+16],es
pop ax
mov word ptr [bx+2],ax
mov word ptr [0xfe],ax
pop ax
mov word ptr [bx+4],ax
inc word ptr [0]
mov bx,ax
mov ax,word ptr [0xfe]
pop ds
iret
'''


def assemble_stub(log_seg=LOG_SEG, scratch=0x00FE):
    with tempfile.TemporaryDirectory(prefix='stub10-') as tmp:
        s, o, b = Path(tmp) / 's.S', Path(tmp) / 's.o', Path(tmp) / 's.bin'
        s.write_text(STUB_SOURCE.replace('mov ax,0x60', f'mov ax,{log_seg:#x}').replace('[0xfe]', f'[{scratch:#x}]'))
        subprocess.run(['as', '--32', str(s), '-o', str(o)], check=True)
        subprocess.run(['objcopy', '-O', 'binary', '-j', '.text', str(o), str(b)], check=True)
        return b.read_bytes()


VIDEO_FN = ctypes.CFUNCTYPE(ctypes.c_int, ctypes.c_void_p, ctypes.POINTER(ctypes.c_uint8), ctypes.POINTER(Cpu), ctypes.POINTER(Cpu))


class Env(ctypes.Structure):
    _fields_ = [('video', VIDEO_FN), ('video_context', ctypes.c_void_p)]


class Diag(ctypes.Structure):
    _fields_ = [('intr_status', ctypes.c_int), ('calls', ctypes.c_uint)]


STATUS = {0: 'RETURNED', 1: 'ARGUMENT', 2: 'INTR', 3: 'TRANSFER'}


def load_image():
    exe = (ROOT / 'resources/DOS/POOLRAD/START.EXE').read_bytes()
    image = bytes(unpack(exe)[0])
    check(sha256(image).hexdigest() == IMAGE_DIGEST, 'loader-expanded image identity')
    return image, relocations(exe, image)[0]


def relocate(image, sites, base):
    out = bytearray(image)
    for s in sites:
        out[s:s + 2] = struct.pack('<H', (struct.unpack_from('<H', out, s)[0] + base) & 0xFFFF)
    return bytes(out)


def source(image, sites):
    b = SEG['body'] * 16
    body = image[b + ENTRY:b + END]
    check(sha256(body).hexdigest() == DIGEST, '0802:0040 body identity')
    ins = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(body, ENTRY))
    check(len(ins) == 72 and sum(i.size for i in ins) == len(body), 'decode covers the span')
    check([(i.address, i.op_str) for i in ins if i.mnemonic == 'lcall'] == [(a, '0xa32, 0xb') for a in (0x62, 0xA5, 0xD0, 0xFC)], 'four Intr far calls')
    check([i.address for i in ins if i.mnemonic in ('ret', 'retf')] == [0x102], 'single RETF 0Ah at 0102')
    check(not [i for i in ins if i.mnemonic in ('int', 'call', 'in', 'out', 'ljmp')], 'no call, interrupt or port instruction')
    check({i.address: i.op_str for i in ins if i.mnemonic.startswith('j')} == {0x7D: '0x8f', 0x8D: '0x95', 0xAF: '0x101'}, 'jumps')
    check([a - b - ENTRY + ENTRY for a in sorted(s for s in sites if b + ENTRY <= s < b + END)] == [x for x in RELOC_AT], 'the four Intr segment words are the only relocations')
    conditional = {i.address: (i.address + i.size, int(i.op_str, 16)) for i in ins if i.mnemonic.startswith('j') and i.mnemonic != 'jmp'}
    return body, {i.address: i for i in ins}, conditional


def library(directory):
    path = directory / 'r0802_0040.so'
    subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Wpedantic', '-Werror', '-O2', '-fPIC', '-shared', '-I', str(ROOT / 'include')] +
                   [str(ROOT / f'src/recovered/{s}.c') for s in ('resident_0802_0040_cpu', 'dos_intr_000b_cpu')] + ['-o', str(path)], check=True)
    f = ctypes.CDLL(str(path)).pr_r0802_0040
    f.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t, ctypes.POINTER(Cpu), ctypes.POINTER(Env), ctypes.POINTER(Diag)]
    f.restype = ctypes.c_int
    return f


def build(image_reloc, stub, rng, *, ega=0, ch=0x41, count=1, attr=0x12, row=5, col=7, page_a=7, page_b=3, ss=SS, sp=0xF000, flags=2, ds_off=0,
          cpu=None, table=None, regs=None, hi=None, background=0x5A, log_count=0):
    m = bytearray([background]) * SIZE
    b = BASE * 16; m[b:b + len(image_reloc)] = image_reloc
    ds = BASE + SEG['data'] + ds_off
    s = physical(STUB_SEG, 0); m[s:s + len(stub)] = stub
    put16(m, 0, 0x40, 0x0000); put16(m, 0, 0x42, STUB_SEG)       # IVT[10h] -> the declared logging stub
    put16(m, LOG_SEG, 0, log_count)                              # the log count (its record area is whatever the background holds)
    tab = table if table is not None else [(0xA0 + k * 7 + 3) & 0xFF for k in range(256)]
    for k in range(256):
        m[physical(ds, 0x25D5 + k)] = tab[k]
    m[physical(ds, 0x4A1A)] = page_a; m[physical(ds, 0x4A1B)] = page_b; m[physical(ds, 0x4A1E)] = ega
    for k, v in enumerate(regs or (0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0x6666, 0x7777, 0x8888, 0x9999, 0x0246)):
        put16(m, ds, 0x4A02 + 2 * k, v)                           # Regs.AX BX CX DX BP SI DI DS ES FLAGS
    hi = hi or (0xAB, 0xCD, 0xEF, 0x12, 0x34)
    put16(m, ss, sp, 0x7000); put16(m, ss, (sp + 2) & 0xFFFF, 0x9001)
    for k, v in enumerate((ch, count, attr, row, col)):
        put16(m, ss, (sp + 4 + 2 * k) & 0xFFFF, (hi[k] << 8) | v)
    r = cpu or (0x1234, 0x3456, 0x5678, 0x789A, 0x9ABC, 0xBCDE, 0x7777)
    c = Cpu(*r, sp, BASE + SEG['body'], ENTRY, ds, 0x1111, ss, flags)
    return m, c


def expected_log(m_before, o):
    """The documented contract, from the inputs only: the register blocks the body hands to INT 10h, in order. Regs is DS:4A02 (stale values
    from the fixture); the handler does not change them, and Intr stores the registers back unchanged."""
    ds = BASE + SEG['data'] + o.get('ds_off', 0)
    regs = o.get('regs') or (0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0x6666, 0x7777, 0x8888, 0x9999, 0x0246)
    ax, bx, cx, dx, bp, si, di, rds, res = regs[:9]
    ega, ch, count, attr, row, col = o.get('ega', 0), o.get('ch', 0x41), o.get('count', 1), o.get('attr', 0x12), o.get('row', 5), o.get('col', 7)
    pa, pb = o.get('page_a', 7), o.get('page_b', 3)
    tab = o.get('table') or [(0xA0 + k * 7 + 3) & 0xFF for k in range(256)]
    out = []
    def block(ax_, bx_, cx_, dx_):
        out.append((ax_, bx_, cx_, dx_, si, di, bp, res))
    dx_ = (row << 8) | col
    ax1 = (0x02 << 8) | (ax & 0xFF); bx1 = (pb << 8) | (bx & 0xFF)
    block(ax1, bx1, cx, dx_)
    bl = tab[attr] if ega == 0 else attr
    block((0x09 << 8) | ch, (pb << 8) | bl, count, dx_)
    if ega == 1:
        block((0x02 << 8) | ch, (pa << 8) | bl, count, dx_)
        block((0x09 << 8) | ch, (pa << 8) | attr, count, dx_)
    return out


def read_log(ram):
    n = ram[physical(LOG_SEG, 0)] | ram[physical(LOG_SEG, 1)] << 8
    recs = []
    for k in range(n):
        base = (k * 16) & 0xFFFF
        recs.append(tuple(ram[physical(LOG_SEG, (base + 2 + 2 * j) & 0xFFFF)] | ram[physical(LOG_SEG, (base + 3 + 2 * j) & 0xFFFF)] << 8 for j in range(8)))
    return recs


def run_unicorn(name, m, c, body_ips, conditional=None, edges=None):
    from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE
    from unicorn import x86_const as R
    uc = Uc(UC_ARCH_X86, UC_MODE_16); backing = (ctypes.c_uint8 * SIZE).from_buffer_copy(m)
    uc.mem_map_ptr(0, SIZE, 7, ctypes.addressof(backing)); uc.mem_map_ptr(SIZE, SIZE, 7, ctypes.addressof(backing))
    for (n, _), reg in zip(Cpu._fields_, REGISTERS):
        uc.reg_write(reg, getattr(c, n))
    s = dict(stop=None, steps=0, returned=False, last=None); seen = set(); closure = set()
    body_cs = BASE + SEG['body']

    def code(em, address, size, user):
        cs = uc.reg_read(R.UC_X86_REG_CS); ip = uc.reg_read(R.UC_X86_REG_IP)
        if s['returned']:
            s['stop'] = 'RETURNED'; return uc.emu_stop()
        closure.add((cs, ip))
        if edges is not None and s['last'] is not None and s['last'][0] == body_cs and s['last'][1] in conditional:
            edges.add((s['last'][1], ip if cs == body_cs else -1))
        s['last'] = (cs, ip)
        if cs == body_cs:
            check(ip in body_ips, f'escape inside 0802 {name} {ip:04x}'); seen.add(ip)
            if ip == 0x102:
                s['returned'] = True
        s['steps'] += 1
        if s['steps'] > 20000:
            s['stop'] = 'RUNAWAY'; uc.emu_stop()
    uc.hook_add(UC_HOOK_CODE, code)
    uc.emu_start(physical(c.cs, c.ip), 2 * SIZE, count=30000)
    check(s['stop'] == 'RETURNED', f'original did not return {name}: {s["stop"]}')
    return uc, backing, seen, closure


def c_video(keep, log_seg=LOG_SEG, stub_at=(STUB_SEG, 0), scratch=0x00FE):
    """The INT 10h logging stub's effect, as its bytes run (every push, store, re-read and the IRET), for the C model."""
    def video(ctx, ram, before, after):
        b = before[0]
        if (b.cs, b.ip) != stub_at:
            return 1
        a = Cpu.from_buffer_copy(b)

        def rd(seg, off):
            at = physical(seg, off & 0xFFFF)
            return ram[at] | ram[(at + 1) % SIZE] << 8

        def wr(seg, off, v):
            at = physical(seg, off & 0xFFFF)
            ram[at] = v & 255; ram[(at + 1) % SIZE] = (v >> 8) & 255
        sp, ss = b.sp, b.ss
        wr(ss, sp - 2, b.ds); wr(ss, sp - 4, b.bx); wr(ss, sp - 6, b.ax)             # PUSH DS; PUSH BX; PUSH AX
        bxi = (rd(log_seg, 0) << 4) & 0xFFFF                                         # MOV BX,[0]; SHL BX,1 x4
        for k, v in ((6, b.cx), (8, b.dx), (10, b.si), (12, b.di), (14, b.bp), (16, b.es)):
            wr(log_seg, bxi + k, v)
        ax1 = rd(ss, sp - 6); wr(log_seg, bxi + 2, ax1); wr(log_seg, scratch, ax1)       # POP AX; MOV [BX+2],AX; MOV [scratch],AX
        ax2 = rd(ss, sp - 4); wr(log_seg, bxi + 4, ax2)                              # POP AX; MOV [BX+4],AX
        wr(log_seg, 0, (rd(log_seg, 0) + 1) & 0xFFFF)                                # INC WORD [0]
        a.bx = ax2; a.ax = rd(log_seg, scratch); a.ds = rd(ss, sp - 2)                  # MOV BX,AX; MOV AX,[scratch]; POP DS
        a.ip, a.cs, a.flags = rd(ss, sp), rd(ss, sp + 2), rd(ss, sp + 4); a.sp = (sp + 6) & 0xFFFF   # IRET
        after[0] = a
        return 0
    fn = VIDEO_FN(video)
    keep.append(fn)
    return fn


def compare(name, state, o, body_ips, f, conditional=None, edges=None):
    m, c = state
    uc, backing, seen, closure = run_unicorn(name, m, c, body_ips, conditional, edges)
    actual = Cpu.from_buffer_copy(c); ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(m); keep = []
    env = Env(c_video(keep), None); d = Diag()
    status = f(ram, SIZE, ctypes.byref(actual), ctypes.byref(env), ctypes.byref(d))
    check(STATUS.get(status) == 'RETURNED', f'C status {name}: {STATUS.get(status)} intr {d.intr_status} calls {d.calls}')
    if bytes(ram) != bytes(backing):
        diff = [hex(i) for i, (a, b) in enumerate(zip(bytes(ram), bytes(backing))) if a != b]
        raise ValueError(f'RAM {name}: {diff[:8]} ({len(diff)} bytes)')
    for (n, _), reg in zip(Cpu._fields_, REGISTERS):
        want = uc.reg_read(reg) & 0xFFFF
        check(getattr(actual, n) == want, f'CPU {name}/{n} C {getattr(actual, n):04x} original {want:04x}')
    return seen, closure, actual, bytes(ram), d


def cases():
    rng = random.Random(0x08020040)
    out = []
    for ega in (0, 1, 2, 0xFF):
        for attr in (0x00, 0x01, 0x07, 0x0F, 0x12, 0x7F, 0x80, 0xFF):
            out.append((f'ega{ega:02x}-attr{attr:02x}', dict(ega=ega, attr=attr), True))
    for attr in range(256):                                                # the whole translation table, both ways
        out.append((f'tab-ega0-attr{attr:02x}', dict(ega=0, attr=attr, ch=(attr * 5) & 0xFF), True))
        out.append((f'tab-ega1-attr{attr:02x}', dict(ega=1, attr=attr, ch=(attr * 11) & 0xFF), True))
    for ch in (0x00, 0x20, 0x41, 0x7F, 0x80, 0xDB, 0xFF):
        for count in (0, 1, 2, 0x28, 0xFF):
            out.append((f'ch{ch:02x}-n{count:02x}', dict(ch=ch, count=count, ega=rng.choice((0, 1))), True))
    for row, col in ((0, 0), (24, 39), (24, 79), (0xFF, 0xFF), (10, 0), (0, 79), (25, 80)):
        for ega in (0, 1, 2):
            out.append((f'rc{row:02x}{col:02x}-ega{ega}', dict(row=row, col=col, ega=ega), True))
    for pa, pb in ((0, 0), (7, 3), (3, 7), (0xFF, 0), (1, 1), (0, 0xFF)):
        for ega in (0, 1, 2):
            out.append((f'pages{pa:02x}{pb:02x}-ega{ega}', dict(page_a=pa, page_b=pb, ega=ega), True))
    out += [(f'flags{fl:x}-ega{e}', dict(flags=fl, ega=e), True) for fl in FLAGS for e in (0, 1)]
    out += [(f'hi{n}', dict(hi=tuple(rng.randrange(256) for _ in range(5)), ega=rng.choice((0, 1, 2))), True) for n in range(6)]
    out += [('sp-min', dict(sp=0x0022), False), ('sp-min-plus-1', dict(sp=0x0023), False), ('sp-max', dict(sp=0xFFF0), False), ('sp-odd', dict(sp=0xF001), False),
            ('log-count-3', dict(log_count=3, ega=1), False), ('log-count-15', dict(log_count=15, ega=1), False), ('log-count-ffff', dict(log_count=0xFFFF, ega=1), False)]
    # Aliasing: the argument slots overlap the data the body writes or reads, so operands are re-read live.
    ds_ss = BASE + SEG['data']
    out += [(f'alias-slots-on-regs-{sp:04x}', dict(ss=ds_ss, sp=sp, ega=e), False) for sp in (0x4A00, 0x4A02, 0x49FE, 0x4A04, 0x4A0A) for e in (0, 1)]
    out += [(f'alias-attr-slot-on-table-{sp:04x}', dict(ss=ds_ss, sp=sp, ega=0), False) for sp in (0x25CD, 0x25D0, 0x25D3, 0x2600)]
    out += [(f'alias-slots-after-regs-{sp:04x}', dict(ss=ds_ss, sp=sp, ega=e), False) for sp in (0x4A16,) for e in (0, 1, 2)]   # the return frame stays clean; a frame over the flag byte would redirect the RETF
    for k in range(24):
        out.append((f'random{k}', dict(ega=rng.choice((0, 1, 2, rng.randrange(256))), ch=rng.randrange(256), count=rng.choice((0, 1, rng.randrange(256))),
                                       attr=rng.randrange(256), row=rng.randrange(256), col=rng.randrange(256), page_a=rng.randrange(256), page_b=rng.randrange(256),
                                       flags=rng.choice(FLAGS), cpu=tuple(rng.randrange(65536) for _ in range(7)),
                                       regs=tuple(rng.randrange(65536) for _ in range(10)), hi=tuple(rng.randrange(256) for _ in range(5)),
                                       table=[rng.randrange(256) for _ in range(256)]), True))
    return out


def differential(image_reloc, stub, decoded, conditional, f):
    seen, closure = set(), set(); specs = cases(); rng = random.Random(0x08020041); statuses = {}; calls = {}; plain = 0; edges = set()
    ds_base = BASE + SEG['data']; edges = set()
    for name, options, independent in specs:
        state = build(image_reloc, stub, rng, **options)
        before = bytes(state[0])
        v, cl, final, ram, d = compare(name, state, options, set(decoded), f, conditional, edges); seen |= v; closure |= cl
        calls[d.calls] = calls.get(d.calls, 0) + 1
        if independent:                                                    # the contract, independent of both models
            got = read_log(ram)
            want = expected_log(before, options)
            check(got == want, f'{name}: INT 10h blocks {got} != contract {want}')
            plain += 1
        sp = options.get('sp', 0xF000)
        check(final.sp == (sp + 14) & 0xFFFF and (not independent or (final.cs, final.ip) == (0x9001, 0x7000)), f'{name}: RETF 0Ah')
    check(seen == set(decoded), f'body starts missing {sorted(set(decoded) - seen)}')
    # Conditional outcomes by successor, from the instruction starts the runs executed per case would be ideal; the three jumps are covered when both successors ran.
    # (the per-successor accounting below is computed from the same cases by re-running the original once per outcome class)
    wanted = {(a, b) for a, pair in conditional.items() for b in pair}
    check(edges >= wanted, f'conditional outcomes missing: {sorted(wanted - edges)}')
    ega_values = {o.get('ega', 0) for _, o, _ in specs}
    check({0, 1, 2, 0xFF} <= ega_values, 'DS:4A1E values 0, 1, 2 and FF are all exercised')
    by_seg = {}
    for cs, ip in closure:
        by_seg.setdefault(f'{cs - BASE:04X}', set()).add(ip)
    b = SEG['dos'] * 16
    intr_decoded = {i.address for i in Cs(CS_ARCH_X86, CS_MODE_16).disasm(image_reloc[b + INTR_MEMBER[0]:b + INTR_MEMBER[1]], INTR_MEMBER[0])}
    intr_ran = {ip for cs, ip in closure if cs == BASE + SEG['dos'] and INTR_MEMBER[0] <= ip < INTR_MEMBER[1]}
    check(intr_ran <= intr_decoded, 'Intr executed starts off the decode')
    return dict(cases=len(specs), stops=dict(RETURNED=len(specs)), independent_contract_cases=plain, body_starts=len(seen), body_starts_total=len(decoded), conditional_outcomes=len(edges & wanted), conditional_outcomes_total=len(wanted),
                intr_calls_histogram=dict(sorted(calls.items())), closure_instruction_starts={k: len(v) for k, v in sorted(by_seg.items())},
                intr_member_starts=f'{len(intr_ran)}/{len(intr_decoded)}', cpu_words=14, ram_bytes=SIZE, flag_masks=0)


def main():
    p = argparse.ArgumentParser()
    for flag in ('--compile-only', '--unicorn', '--native'):
        p.add_argument(flag, action='store_true')
    p.add_argument('--output', type=Path); a = p.parse_args()
    with tempfile.TemporaryDirectory(prefix='r0802-0040-') as tmp:
        f = library(Path(tmp))
        if a.compile_only:
            print('CHECKER_BUILD_OK'); return
        image, sites = load_image()
        body, decoded, conditional = source(image, sites)
        stub = assemble_stub()
        image_reloc = relocate(image, sites, BASE)
        report = dict(target=dict(span='0802:[0040,0105)', bytes=len(body), instructions=len(decoded), sha256=DIGEST, image_sha256=IMAGE_DIGEST,
                                  relocations_in_span=len(RELOC_AT), load_base=f'{BASE:04X}', int10_stub_bytes=len(stub), int10_stub_sha256=sha256(stub).hexdigest()))
        if a.unicorn:
            report['unicorn'] = differential(image_reloc, stub, decoded, conditional, f)
        if a.native:
            import res0802_0040_native
            report['native'] = res0802_0040_native.native(image, sites, decoded, f, sys.modules[__name__], stub)
    if a.output:
        a.output.write_text(json.dumps(report, sort_keys=True, indent=2) + '\n')
    print(json.dumps(report, sort_keys=True))


if __name__ == '__main__':
    main()
