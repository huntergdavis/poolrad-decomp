#!/usr/bin/env python3
"""RES_0802_050F (the ECL block decompressor) against the original bytes with its two callees REAL (Move 0AF8:1685, FillChar
0AF8:16A8).

The whole loader-expanded START.EXE image is loaded at a test base with the loader's own relocation table applied; no byte is
patched and no DOS service is involved. Each case is compared unmasked on CPU14 and the full 1 MiB at the model's stop.

Catchers, per claim (each names what it shares with the model):
- full state: the Unicorn differential (shares nothing but the input image and RAM);
- decoded bytes on the REAL corpus: the portable DAX reader src/formats/dax.c (pr_dax_rle), a separate implementation that does
  not share this model's code; it is lenient on 7Fh/80h (it reads them as 128-byte runs), so it is used only on valid data;
- synthetic streams: an independent restatement of the decode below (expected()), including the stall and the 16-bit wraps;
- STALLED (the original never returns): Unicorn runs 1,000 further loop iterations from the model's stop and must see the same
  CPU at every arrival, unchanged RAM, and no return;
- native (DOSBox-X): the C model against the real CPU on the 29 corpus blocks and synthetic returning streams.
Media stay under ignored resources/."""
import argparse, ctypes, json, os, random, struct, subprocess, sys, tempfile, time
from hashlib import sha256
from pathlib import Path
from capstone import Cs, CS_ARCH_X86, CS_MODE_16
from res0709_ega_reset_0000_cpu_audit import Cpu, ROOT, SIZE, REGISTERS, physical, put16, check
import res0899_quit_0000_cpu_audit as Q
import res0802_0716_cpu_audit as A716
CHECKER_ROWS = ('RES_0802_050F',)
ENTRY, END = 0x050F, 0x05CB
DIGEST = '98eb2079e2b8e02ee5cccbbfae1080308ee04142ea1d806328edfcf78e28c9b6'
BASE = Q.BASE
SOURCES = ['resident_0802_050f_cpu', 'system_runtime_leaves_cpu', 'system_string_move_leaves_cpu']
FAR_SITES = [0x055C, 0x05A7]
CONDITIONALS = {0x0535: (0x0574, 0x0537), 0x0539: (0x0574, 0x053B), 0x0576: (0x05B7, 0x0578), 0x057A: (0x05B7, 0x057C),
                0x05C0: (0x05C5, 0x05C2)}
STATUS = {0: 'RETURN', 1: 'ARGUMENT', 2: 'STALLED', 3: 'BUDGET', 4: 'CALLEE'}
PSEG, VSEG, OSEG, SS_, SP_ = 0x5000, 0x8000, 0x7000, 0x6000, 0xF000
LOOP = 0x051F
STALL_EXTRA = 1000


class Diag(ctypes.Structure):
    _fields_ = [('site', ctypes.c_uint16), ('callee_status', ctypes.c_int), ('arrivals', ctypes.c_uint32)]


def source(image):
    body = image[0x802 * 16 + ENTRY:0x802 * 16 + END]
    check(sha256(body).hexdigest() == DIGEST, '0802:050F identity')
    ins = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(body, ENTRY))
    check(len(ins) == 83 and sum(i.size for i in ins) == len(body), 'decode covers 050F')
    check([i.address for i in ins if i.mnemonic == 'lcall'] == FAR_SITES, 'the two far-call sites')
    check(not [i for i in ins if i.mnemonic == 'call'], 'no near calls')
    check({i.address for i in ins if i.mnemonic.startswith('j') and i.mnemonic != 'jmp'} == set(CONDITIONALS), 'conditional sites')
    return body, {i.address: i for i in ins}


def library(directory):
    path = directory / 'r050f.so'
    subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Wpedantic', '-Werror', '-O2', '-fPIC', '-shared', '-I', str(ROOT / 'include')]
                   + [str(ROOT / f'src/recovered/{n}.c') for n in SOURCES] + [str(ROOT / 'src/formats/dax.c'), '-o', str(path)], check=True)
    lib = ctypes.CDLL(str(path))
    f = lib.pr_r0802_050f
    f.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t, ctypes.POINTER(Cpu), ctypes.c_uint32, ctypes.POINTER(Diag)]
    f.restype = ctypes.c_int
    lib.pr_dax_rle.argtypes = [ctypes.c_char_p, ctypes.c_size_t, ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t]
    lib.pr_dax_rle.restype = ctypes.c_int
    return f, lib


def dax_reader(lib, packed, unpacked):
    """The portable reader's decode (separate implementation); returns bytes or None if it rejects the stream."""
    out = (ctypes.c_uint8 * max(unpacked, 1))()
    return bytes(out[:unpacked]) if lib.pr_dax_rle(packed, len(packed), out, unpacked) == 0 else None


def build(reloc, *, packed, out_len=0x400, psrc=(0, PSEG), pdst=(0, OSEG), vars_at=(VSEG, 0), size=None, extra=(), ss=SS_,
          sp=SP_, flags=2, regs=None, base=BASE, ret=(0x9001, 0x7000), fill=0x5A):
    """packed bytes at psrc; output buffer at pdst prefilled; pointer variables at vars_at: +0 packed far pointer, +4 output far
    pointer, +8 packed-size word, +0Ah output-size word. extra: (seg, off, bytes) writes applied last (aliasing cases)."""
    m = bytearray([0x5A]) * SIZE
    m[base * 16:base * 16 + len(reloc)] = reloc
    for k in range(out_len):
        m[physical(pdst[1], (pdst[0] + k) & 0xFFFF)] = fill
    for k, b in enumerate(packed):
        m[physical(psrc[1], (psrc[0] + k) & 0xFFFF)] = b
    vs, vo = vars_at
    put16(m, vs, vo, psrc[0]); put16(m, vs, vo + 2, psrc[1]); put16(m, vs, vo + 4, pdst[0]); put16(m, vs, vo + 6, pdst[1])
    put16(m, vs, vo + 8, len(packed) if size is None else size); put16(m, vs, vo + 10, out_len & 0xFFFF)
    for s_, o_, data in extra:
        for k, b in enumerate(data):
            m[physical(s_, (o_ + k) & 0xFFFF)] = b
    for k, w in enumerate((ret[1], ret[0], vo + 10, vs, vo + 8, vs, vo + 4, vs, vo, vs)):
        put16(m, ss, (sp + 2 * k) & 0xFFFF, w)
    r = regs or (0x1234, 0x3456, 0x5678, 0x789A, 0x9ABC, 0xBCDE, 0x7777)
    return m, Cpu(*r, sp, base + 0x0802, ENTRY, base + Q.SEG['data'], 0x1111, ss, flags)


def run_c(f, cpu, ram_bytes, budget):
    ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(ram_bytes); c = Cpu.from_buffer_copy(cpu); d = Diag()
    status = STATUS[f(ram, SIZE, ctypes.byref(c), budget, ctypes.byref(d))]
    return status, c, bytes(ram), d


def run_unicorn(name, m, c, decoded, base, ret, stop_at=None, extra=0):
    """Run the original. stop_at=k snapshots at the k-th arrival at 051Fh; extra then runs that many more arrivals and requires
    each to show the same CPU (and RAM unchanged at the end)."""
    from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE, UC_HOOK_INTR
    uc = Uc(UC_ARCH_X86, UC_MODE_16); backing = (ctypes.c_uint8 * SIZE).from_buffer_copy(m)
    uc.mem_map_ptr(0, SIZE, 7, ctypes.addressof(backing)); uc.mem_map_ptr(SIZE, SIZE, 7, ctypes.addressof(backing))
    for (n, _), reg in zip(Cpu._fields_, REGISTERS):
        uc.reg_write(reg, getattr(c, n))
    from unicorn import x86_const as R
    cs0802 = base + 0x0802
    s = dict(stop=None, prev=None, arrivals=0, snap=None, after=None); seen, edges = set(), set()

    def cpu_now():
        return Cpu(*[uc.reg_read(r) & 0xFFFF for r in REGISTERS])

    def code(em, address, size, user):
        cs = uc.reg_read(R.UC_X86_REG_CS); ip = uc.reg_read(R.UC_X86_REG_IP)
        if s['prev'] is not None and cs == cs0802:
            edges.add((s['prev'], ip))
        s['prev'] = None
        if (cs, ip) == ret:
            s['stop'] = 'RETURN'; s['snap'] = (cpu_now(), bytes(backing)); return uc.emu_stop()
        check(cs in (cs0802, base + Q.SEG['system']), f'escape {name} {cs:04x}:{ip:04x}')
        if cs == cs0802:
            check(ENTRY <= ip < END and ip in decoded, f'escape inside 050F {name} {ip:04x}'); seen.add(ip)
            if ip in CONDITIONALS:
                s['prev'] = ip
            if ip == LOOP:
                s['arrivals'] += 1
                if stop_at is not None and s['arrivals'] == stop_at:
                    s['snap'] = (cpu_now(), bytes(backing))
                    if not extra:
                        s['stop'] = 'AT'; return uc.emu_stop()
                elif stop_at is not None and s['arrivals'] > stop_at:
                    check(bytes(cpu_now()) == bytes(s['snap'][0]), f'{name}: CPU changed at arrival {s["arrivals"]}')
                    if s['arrivals'] == stop_at + extra:
                        s['after'] = bytes(backing); s['stop'] = 'AT'; return uc.emu_stop()

    def intr(em, intno, user):
        check(False, f'ENV refusal {name}: INT {intno:#x}')
    uc.hook_add(UC_HOOK_CODE, code); uc.hook_add(UC_HOOK_INTR, intr)
    uc.emu_start(physical(c.cs, c.ip), 2 * SIZE, count=50000000)
    check(s['stop'] is not None, f'original did not stop {name}')
    return s, seen, edges


def expected(packed, size, out_len, fill=0x5A, limit=100000):
    """Independent restatement of the decode for flat, non-aliasing buffers: ('RETURN', output, in, out) or ('STALLED', ...)."""
    out = bytearray([fill]) * 0x10000; i = o = 0
    for _ in range(limit):
        c = packed[i] if i < len(packed) else 0x5A
        if c <= 0x7E:
            n = c + 1
            for k in range(n):
                out[(o + k) & 0xFFFF] = packed[(i + 1 + k) & 0xFFFF] if (i + 1 + k) & 0xFFFF < len(packed) else 0x5A
            i = (i + n + 1) & 0xFFFF; o = (o + n) & 0xFFFF
        elif c >= 0x81:
            n = 0x100 - c; v = packed[i + 1] if i + 1 < len(packed) else 0x5A
            for k in range(n):
                out[(o + k) & 0xFFFF] = v
            i = (i + 2) & 0xFFFF; o = (o + n) & 0xFFFF
        elif i < size:
            return 'STALLED', bytes(out[:out_len]), i, o
        if i >= size:
            return 'RETURN', bytes(out[:out_len]), i, o
    return 'LIMIT', None, i, o                                  # the statement's own cap: never hang the audit


def synthetic(rng):
    lit = lambda data: bytes([len(data) - 1]) + bytes(data)
    rep = lambda n, v: bytes([0x100 - n, v])
    out = [('literal-00', dict(packed=lit([0xAB]))), ('literal-7e', dict(packed=lit(range(0x7F)))),
           ('repeat-81', dict(packed=rep(0x7F, 0x11))), ('repeat-ff', dict(packed=rep(1, 0x22))),
           ('mixed', dict(packed=lit(b'HELLO') + rep(10, 0x20) + lit(b'X') + rep(3, 0xEE))),
           ('size-zero-reads-one-control', dict(packed=lit(b'AB'), size=0)),
           ('size-zero-7f-returns', dict(packed=b'\x7f', size=0)), ('size-zero-80-returns', dict(packed=b'\x80', size=0)),
           ('stall-7f', dict(packed=lit(b'AB') + b'\x7f\x00')), ('stall-80', dict(packed=b'\x80\x00')),
           ('stall-7f-after-repeat', dict(packed=rep(5, 1) + b'\x7f')),
           ('size-mid-run', dict(packed=lit(b'ABCDEF') + lit(b'GH'), size=3)),     # in passes the size inside a run
           ('size-odd-overshoot', dict(packed=rep(4, 9) + rep(4, 8), size=3)),
           # 16-bit wraps: the output index past FFFFh, the packed pointer's offset, the output pointer's offset
           ('out-index-wraps', dict(packed=rep(0x7F, 0x33) * 520 + lit(b'WRAP'), out_len=0x10000)),
           ('packed-offset-wraps', dict(packed=lit(b'0123456789') + rep(6, 0x44), psrc=(0xFFF8, PSEG))),
           ('output-offset-wraps', dict(packed=lit(b'abcdefgh') + rep(9, 0x55), pdst=(0xFFFA, OSEG))),
           # Move direction: same segment, source offset below the destination (backward) and above (forward)
           ('move-overlap-backward', dict(packed=lit(b'OVERLAP!'), psrc=(0x0100, OSEG), pdst=(0x0102, OSEG))),
           ('move-overlap-forward', dict(packed=lit(b'OVERLAP!'), psrc=(0x0102, OSEG), pdst=(0x0100, OSEG))),
           ('move-far-alias', dict(packed=lit(b'ALIASED'), psrc=(0x0010, OSEG + 1), pdst=(0x0012, OSEG))),
           # the output overwrites the packed-size word / the packed pointer variable before the next reload
           ('output-hits-size-word', dict(packed=lit(b'\x01\x00') + rep(4, 0x41), pdst=(0x0008, VSEG), out_len=0)),
           ('output-hits-pointer', dict(packed=lit(b'\x00\x01') + rep(4, 0x42), pdst=(0x0000, VSEG), out_len=0)),
           # a packed size near FFFFh lets in wrap below it: no fixed point, bounded by the declared budget
           ('budget-wrap-cycle', dict(packed=rep(0x7F, 0x66) * 40, size=0xFFFF, budget=300))]
    out += [(f'flags{fl:x}', dict(packed=lit(b'F') + rep(3, 7), flags=fl)) for fl in Q.FLAGS]
    out += [(f'sp{sp:x}', dict(packed=lit(b'S') + rep(2, 1), sp=sp)) for sp in (0x0016, 0x0100, 0x8001, 0xFFEB, 0xFFEC)]
    out += [('ss-is-output', dict(packed=lit(b'Q') + rep(2, 3), ss=OSEG, sp=0xF000))]
    for k in range(10):
        stream = b''
        while len(stream) < rng.randrange(8, 200):
            if rng.random() < 0.5:
                stream += lit(bytes(rng.randrange(256) for _ in range(rng.randrange(1, 0x7F))))
            else:
                stream += rep(rng.randrange(1, 0x80), rng.randrange(256))
        out.append((f'random{k}', dict(packed=stream, flags=rng.choice(Q.FLAGS), regs=tuple(rng.randrange(65536) for _ in range(7)))))
    return out


def corpus():
    rows = []
    for k in range(1, 9):
        blob = (ROOT / f'resources/DOS/POOLRAD/ECL{k}.DAX').read_bytes()
        entries, h = A716.parse_dax(blob)
        for bid, off, unpacked, packed_len in entries:
            rows.append((f'ecl{k}-id{bid}', blob[2 + h + off:2 + h + off + packed_len], unpacked))
    return rows


def differential(reloc, decoded, f, lib):
    seen, edges, stops, checked, dax_equal, stall_iters, image_untouched = set(), set(), {}, 0, 0, 0, 0
    cases = [(n, dict(packed=p, out_len=u), u) for n, p, u in corpus()] + [(n, o, None) for n, o in synthetic(random.Random(0x0802050F))]
    for index, (name, o, unpacked) in enumerate(cases):
        o = dict(o); budget = o.pop('budget', 1000000); t0 = time.time()
        m, c = build(reloc, **o)
        status, cpu, ram, d = run_c(f, c, bytes(m), budget)
        print(f'[{index + 1}/{len(cases)}] {name}: start (C {status} at arrival {d.arrivals})', file=sys.stderr, flush=True)
        if status == 'RETURN':
            s, v, e = run_unicorn(name, m, c, decoded, BASE, (0x9001, 0x7000))
            check(s['stop'] == 'RETURN', f'{name}: original {s["stop"]}')
        else:
            check(status in ('STALLED', 'BUDGET'), f'{name}: C {status}')
            s, v, e = run_unicorn(name, m, c, decoded, BASE, (0x9001, 0x7000), stop_at=d.arrivals,
                                  extra=STALL_EXTRA if status == 'STALLED' else 0)
            check(s['stop'] == 'AT', f'{name}: original {s["stop"]} before arrival {d.arrivals}')
            if status == 'STALLED':
                check(s['after'] == s['snap'][1], f'{name}: RAM changed across {STALL_EXTRA} further iterations'); stall_iters += STALL_EXTRA
        seen |= v; edges |= e; stops[status] = stops.get(status, 0) + 1
        print(f'[{index + 1}/{len(cases)}] {name}: C {status}, arrivals {d.arrivals}, {time.time() - t0:.1f}s', file=sys.stderr, flush=True)
        A716.compare_cpu(name, cpu, s['snap'][0]); A716.compare_ram(name, ram, s['snap'][1])
        check(ram[BASE * 16:BASE * 16 + len(reloc)] == bytes(m[BASE * 16:BASE * 16 + len(reloc)]), f'{name}: wrote into the code image')
        image_untouched += 1
        o_len = o.get('out_len', 0x400); pdst = o.get('pdst', (0, OSEG))
        got = bytes(ram[physical(pdst[1], (pdst[0] + k) & 0xFFFF)] for k in range(o_len))
        if unpacked is not None:                                   # the real corpus: the separate reader is the catcher
            want = dax_reader(lib, o['packed'], unpacked)
            check(want is not None and got == want, f'{name}: decoded block differs from the portable DAX reader'); dax_equal += 1
        elif 'psrc' not in o and 'pdst' not in o and o.get('ss', SS_) == SS_ and not name.startswith('budget'):
            kind, want, i, out_i = expected(o['packed'], o.get('size', len(o['packed'])), o_len)
            if True:
                check(status == kind, f'{name}: stop {status} vs statement {kind}')
                check(got == want, f'{name}: output differs from the independent statement')
                bp = (o.get('sp', SP_) - 2) & 0xFFFF
                got_i = ram[physical(SS_, bp - 2)] | ram[physical(SS_, bp - 1)] << 8
                got_o = ram[physical(SS_, bp - 4)] | ram[physical(SS_, bp - 3)] << 8
                check((got_i, got_o) == (i, out_i), f'{name}: (in, out) {(got_i, got_o)} vs {(i, out_i)}')
                checked += 1
    want = {(a, t) for a, ts in CONDITIONALS.items() for t in ts}
    check(seen == set(decoded), f'starts missing {sorted(hex(a) for a in set(decoded) - seen)}')
    check(edges == want, f'outcomes missing {sorted(want - edges)}')
    return dict(cases=len(cases), corpus_blocks=dax_equal, final_stops=stops, starts=len(seen), starts_total=len(decoded),
                conditional_outcomes=len(edges), conditional_outcomes_total=len(want), independent_checks=checked,
                dax_reader_equal=dax_equal, stall_iterations_confirmed=stall_iters, image_untouched_cases=image_untouched, cpu_words=14, ram_bytes=SIZE, flag_masks=0)


NBASE, NSS, TOP, LOW, WINDOW = 0x3000, 0x6000, 0xF000, 0xE800, 0x0800


def native(image, sites, f):
    """The C model against the real CPU (DOSBox-X) on the 29 corpus blocks and returning synthetic streams. The oracle reads each
    case (packed length, output length, packed bytes) from CASES.BIN, prefills the output segment, calls 050F and records CPU14,
    the stack window, the pointer variables and the output bytes."""
    directory = ROOT / 'resources/0802-050f/native'; directory.mkdir(parents=True, exist_ok=True)
    reloc = Q.relocate(image, sites, NBASE)
    (directory / 'IMAGE.BIN').write_bytes(reloc)
    rng = random.Random(0x050F)
    cases = [(n, p, u) for n, p, u in corpus()]
    for n, o in synthetic(rng):
        if set(o) <= {'packed', 'flags', 'regs'} and expected(o['packed'], len(o['packed']), 0x400)[0] == 'RETURN':
            cases.append((n, o['packed'], 0x400))
    blob = b''.join(struct.pack('<HH', len(p), u) + p for _, p, u in cases)
    (directory / 'CASES.BIN').write_bytes(blob)
    seg0802 = NBASE + 0x0802
    load = []
    for k, start in enumerate(range(0, len(reloc), 0x8000)):
        load += [f'mov ax,{NBASE + 0x800 * k}', 'mov ds,ax', 'mov bx,word ptr cs:[image]', 'xor dx,dx',
                 f'mov cx,{min(0x8000, len(reloc) - start)}', 'mov ah,0x3f', 'int 0x21', 'jc failure']
    t = ['.code16', '.intel_syntax noprefix', '.global _start', '_start: jmp main', '.org 0x200', 'main:',
         'mov ax,cs', 'mov ds,ax', 'mov ss,ax', 'mov sp,0xfff0', 'cld',
         'mov dx,offset outname', 'xor cx,cx', 'mov ah,0x3c', 'int 0x21', 'jc failure', 'mov word ptr [handle],ax',
         'mov word ptr [header],cs', 'mov word ptr [header+2],offset returned', f'mov word ptr [header+4],{len(cases)}',
         'mov bx,word ptr [handle]', 'mov dx,offset header', 'mov cx,6', 'mov ah,0x40', 'int 0x21',
         'mov dx,offset imagename', 'mov ax,0x3d00', 'int 0x21', 'jc failure', 'mov word ptr [image],ax'] + load + [
         'push cs', 'pop ds', 'mov bx,word ptr [image]', 'mov ah,0x3e', 'int 0x21',
         'mov dx,offset casesname', 'mov ax,0x3d00', 'int 0x21', 'jc failure', 'mov word ptr [cases],ax',
         'again:', 'cld', 'push cs', 'pop ds', 'mov bx,word ptr [cases]', 'mov dx,offset lens', 'mov cx,4', 'mov ah,0x3f', 'int 0x21',
         f'mov ax,{PSEG}', 'mov ds,ax', 'mov bx,word ptr cs:[cases]', 'xor dx,dx', 'mov cx,word ptr cs:[lens]', 'mov ah,0x3f', 'int 0x21',
         f'mov ax,{OSEG}', 'mov es,ax', 'xor di,di', 'mov cx,0x8000', 'mov ax,0x5a5a', 'rep stosw',
         f'mov ax,{VSEG}', 'mov es,ax', 'mov word ptr es:[0],0', f'mov word ptr es:[2],{PSEG}', 'mov word ptr es:[4],0',
         f'mov word ptr es:[6],{OSEG}', 'mov ax,word ptr cs:[lens]', 'mov word ptr es:[8],ax', 'mov ax,word ptr cs:[lens+2]',
         'mov word ptr es:[10],ax',
         f'mov ax,{NSS}', 'mov es,ax', f'mov di,{LOW}', f'mov cx,{WINDOW // 2}', 'mov ax,0x5a5a', 'rep stosw',
         'mov si,word ptr cs:[index]', 'and si,7', 'shl si,1', 'mov ax,word ptr cs:[flagtab+si]', 'mov word ptr cs:[fl],ax',
         f'mov ax,{NSS}', 'mov ss,ax', f'mov sp,{TOP}', f'push {VSEG}', 'push 0', f'push {VSEG}', 'push 4', f'push {VSEG}', 'push 8',
         f'push {VSEG}', 'push 10', 'push word ptr cs:[fl]', 'mov ax,0x2222', 'mov ds,ax', 'mov ax,0x1111', 'mov es,ax',
         'mov ax,0x1234', 'mov bx,0x3456', 'mov cx,0x5678', 'mov dx,0x789a', 'mov si,0x9abc', 'mov di,0xbcde', 'mov bp,0x7777',
         'popf', f'lcall {seg0802},0x050f', 'returned:']
    t += [f'mov word ptr cs:[out+{2 * n}],{r}' for n, r in enumerate('ax bx cx dx si di bp sp cs'.split())]
    t += ['mov word ptr cs:[out+18],offset returned', 'mov word ptr cs:[out+20],ds', 'mov word ptr cs:[out+22],es',
          'mov word ptr cs:[out+24],ss', 'mov ax,cs', 'mov ss,ax', 'mov sp,0xfff0', 'pushf', 'pop ax', 'mov word ptr cs:[out+26],ax',
          'cld', 'push cs', 'pop ds', 'mov bx,word ptr [handle]', 'mov dx,offset out', 'mov cx,28', 'mov ah,0x40', 'int 0x21',
          f'mov ax,{NSS}', 'mov ds,ax', f'mov dx,{LOW}', f'mov cx,{WINDOW}', 'mov bx,word ptr cs:[handle]', 'mov ah,0x40', 'int 0x21',
          f'mov ax,{VSEG}', 'mov ds,ax', 'xor dx,dx', 'mov cx,12', 'mov bx,word ptr cs:[handle]', 'mov ah,0x40', 'int 0x21',
          f'mov ax,{OSEG}', 'mov ds,ax', 'xor dx,dx', 'mov cx,word ptr cs:[lens+2]', 'mov bx,word ptr cs:[handle]', 'mov ah,0x40',
          'int 0x21',
          'push cs', 'pop ds', 'inc word ptr [index]', f'cmp word ptr [index],{len(cases)}', 'jae done', 'jmp again', 'done:',
          'mov bx,word ptr [handle]', 'mov ah,0x3e', 'int 0x21', 'mov ax,0x4c00', 'int 0x21', 'failure:', 'mov ax,0x4c01', 'int 0x21',
          'outname: .asciz "RESULT.BIN"', 'imagename: .asciz "IMAGE.BIN"', 'casesname: .asciz "CASES.BIN"', '.align 2',
          'handle: .word 0', 'image: .word 0', 'cases: .word 0', 'index: .word 0', 'fl: .word 0', 'lens: .word 0,0',
          'flagtab: .word ' + ','.join(str(v) for v in (Q.FLAGS + (0x0002, 0x00D7))[:8]), 'header: .space 6', 'out: .space 28']
    (directory / 'oracle.S').write_text('\n'.join(t) + '\n')
    subprocess.run(['as', '--32', str(directory / 'oracle.S'), '-o', str(directory / 'oracle.o')], check=True)
    subprocess.run(['ld', '-m', 'elf_i386', '-Ttext', '0x100', '--oformat', 'binary', str(directory / 'oracle.o'), '-o',
                    str(directory / 'ORACLE.COM')], check=True)
    (directory / 'dosbox.conf').write_text('[sdl]\noutput=surface\n[dosbox]\nmemsize=16\n[cpu]\ncore=normal\ncputype=386\ncycles=max\n'
                                          '[mixer]\nnosound=true\n[midi]\nmididevice=none\n[autoexec]\nmount c '
                                          + str(directory.resolve()) + '\nc:\nORACLE.COM\nexit\n')
    out = directory / 'RESULT.BIN'
    if out.exists():
        out.unlink()
    binary = '/usr/bin/dosbox-x'
    with (directory / 'dosbox.log').open('w') as log:
        subprocess.run([binary, '-conf', str(directory / 'dosbox.conf'), '-fastlaunch', '-exit', '-noconsole'],
                       env=dict(os.environ, SDL_VIDEODRIVER='dummy', SDL_AUDIODRIVER='dummy'), stdout=log, stderr=subprocess.STDOUT,
                       check=True, timeout=900)
    data = out.read_bytes()
    comseg, ret_off, count = struct.unpack_from('<3H', data); check(count == len(cases), 'native count')
    at = 6; flagtab = (Q.FLAGS + (0x0002, 0x00D7))[:8]
    for n, (name, packed, unpacked) in enumerate(cases):
        m, c = build(reloc, packed=packed, out_len=0x10000, base=NBASE, ss=NSS, sp=TOP - 0x14, ret=(comseg, ret_off), flags=flagtab[n & 7])
        put16(m, VSEG, 10, unpacked)
        m[physical(NSS, LOW):physical(NSS, LOW) + WINDOW] = b'\x5a' * WINDOW
        for k, w in enumerate((ret_off, comseg, 10, VSEG, 8, VSEG, 4, VSEG, 0, VSEG)):
            put16(m, NSS, TOP - 0x14 + 2 * k, w)
        c.ds = 0x2222
        status, cpu, ram, _ = run_c(f, c, bytes(m), 1000000)
        check(status == 'RETURN', f'native C status {name}: {status}')
        rec = data[at:at + 28]; at += 28
        for (reg, _), want in zip(Cpu._fields_, struct.unpack_from('<14H', rec)):
            check(getattr(cpu, reg) == want, f'native CPU {name}/{reg}: C {getattr(cpu, reg):04x} DOS {want:04x}')
        for seg, off, length in ((NSS, LOW, WINDOW), (VSEG, 0, 12), (OSEG, 0, unpacked)):
            dos = data[at:at + length]; at += length
            mine = bytes(ram[physical(seg, (off + k) & 0xFFFF)] for k in range(length))
            check(mine == dos, f'native {name} region {seg:04x}:{off:04x}+{length:x} differs')
    check(at == len(data), f'native length {len(data)} vs parsed {at}')
    probe = subprocess.run([binary, '--version'], capture_output=True, text=True, env=dict(os.environ, SDL_VIDEODRIVER='dummy'))
    version = next(line.split(',')[0].strip() for line in (probe.stdout + probe.stderr).splitlines() if 'DOSBox-X version' in line)
    return dict(cases=len(cases), corpus_blocks=len(corpus()), cpu_words=14, dos_stack_mask_bytes_below_int_sp=0,
                compared_regions=[f'{NSS:04X}:{LOW:04X}+{WINDOW:X}', f'{VSEG:04X}:0000+C', f'{OSEG:04X}:0000+(output size)'],
                result_sha256=sha256(data).hexdigest(),
                dosbox_x=dict(path=binary, sha256=sha256(open(binary, 'rb').read()).hexdigest(), version=version,
                              pinned_source='5a9c7c53481c2e933948039c33256776653f4faf', pinned_build_used=False))


def main():
    p = argparse.ArgumentParser()
    for flag in ('--compile-only', '--unicorn', '--native'):
        p.add_argument(flag, action='store_true')
    p.add_argument('--output', type=Path); a = p.parse_args()
    with tempfile.TemporaryDirectory(prefix='r050f-') as tmp:
        f, lib = library(Path(tmp))
        if a.compile_only:
            print('CHECKER_BUILD_OK'); return
        image, sites = Q.load_image()
        body, decoded = source(image)
        report = dict(target=dict(span='0802:[050F,05CB)', bytes=len(body), instructions=len(decoded), sha256=DIGEST,
                                  image_sha256=Q.IMAGE_DIGEST, relocations=len(sites)))
        if a.unicorn:
            report['unicorn'] = differential(Q.relocate(image, sites, BASE), decoded, f, lib)
        if a.native:
            report['native'] = native(image, sites, f)
    if a.output:
        a.output.write_text(json.dumps(report, sort_keys=True, indent=2) + '\n')
    print(json.dumps(report, sort_keys=True))


if __name__ == '__main__':
    main()
