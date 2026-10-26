#!/usr/bin/env python3
"""SYSTEM text output Write(string) 0AF8:0919 and Writeln 0AF8:084A (raw CPU) against the original bytes.

The whole loader-expanded START.EXE image is loaded at a test base with the loader's own relocation table applied (the
put-character helper loads DS from the relocated word at CS:07EF). Declared environment, identical in every oracle and in
the C service: each TextRec driver function (InOutFunc +14h, FlushFunc +18h) points at a real-byte stub that marks the
buffer consumed (BufPos := 0) and returns a per-case result in AX through RETF 4. Unicorn: CPU14 + full 1 MiB, unmasked,
with refusal cases stopped at the driver entry. DOSBox-X: the same image and stubs, far-called from a COM. Runtime
coverage only; no identity claim. Media and generated programs stay under ignored resources/."""
import argparse, ctypes, json, os, random, struct, subprocess, tempfile
from hashlib import sha256
from pathlib import Path
from capstone import Cs, CS_ARCH_X86, CS_MODE_16
from resident_loader_map import unpack, relocations
from res0709_ega_reset_0000_cpu_audit import Cpu, ROOT, SIZE, REGISTERS, physical, put16, check

CHECKER_ROWS = ('RES_0AF8_0919', 'RES_0AF8_084A')
IMAGE_DIGEST = 'a42a57e68d3f7815831821cc9365bb917befda45a58e958544451fc7a878df8e'
SYS_SEG, DATA_SEG = 0x0AF8, 0x0C7C
SPANS = {'0919': (0x0919, 0x0957, 32), '084a': (0x084A, 0x086E, 17), '07b8': (0x07B8, 0x07DC, 10), '07dc': (0x07DC, 0x081C, 31),
         '087b': (0x087B, 0x0898, 12)}
ENTRIES = {'0919': ('pr_system_write_str_0919', 6), '084a': ('pr_system_writeln_084a', 4)}
BASE, SS, TSEG, STUB = 0x1000, 0x7000, 0x6000, 0x5000
RET_CS, RET_IP = 0x9001, 0x7000
FLAGS = (0x0002, 0x00D7, 0x08D7, 0x0402, 0x0893, 0x0003, 0x0E02)
VIDEO_FN = None
DRIVER_FN = ctypes.CFUNCTYPE(ctypes.c_int, ctypes.c_void_p, ctypes.POINTER(ctypes.c_uint8), ctypes.POINTER(Cpu), ctypes.POINTER(Cpu))
STATUS = {0: 'RETURNED', 1: 'ARGUMENT', 2: 'SERVICE'}


def stub(result):
    """push bp; mov bp,sp; les di,[bp+6]; mov word es:[di+8],0; mov ax,result; pop bp; retf 4"""
    return bytes.fromhex('55' '8bec' 'c47e06' '26c7450800 00'.replace(' ', '')) + bytes([0xB8, result & 255, result >> 8]) + bytes.fromhex('5d' 'ca0400')


class TextEnv(ctypes.Structure):
    _fields_ = [('driver', DRIVER_FN), ('context', ctypes.c_void_p)]


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
    md = Cs(CS_ARCH_X86, CS_MODE_16); meta, decoded = {}, {}
    for name, (a, b, n) in SPANS.items():
        span = image[SYS_SEG * 16 + a:SYS_SEG * 16 + b]
        ins = list(md.disasm(span, a))
        check(len(ins) == n and sum(i.size for i in ins) == len(span), f'{name}: decode covers the span')
        meta[name] = dict(span=f'0AF8:[{a:04X},{b:04X})', bytes=len(span), instructions=n, sha256=sha256(span).hexdigest())
        decoded.update({i.address: i for i in ins})
    check(SYS_SEG * 16 + 0x07EF in set(sites), 'the DS word at 07EF is a loader relocation')
    return meta, decoded


def library(directory):
    path = directory / 'text.so'
    subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Wpedantic', '-Werror', '-O2', '-fPIC', '-shared', '-I', str(ROOT / 'include'),
                    str(ROOT / 'src/recovered/system_text_write_cpu.c'), '-o', str(path)], check=True)
    lib = ctypes.CDLL(str(path)); out = {}
    for name, (symbol, _) in ENTRIES.items():
        f = getattr(lib, symbol)
        f.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t, ctypes.POINTER(Cpu), ctypes.POINTER(TextEnv), ctypes.POINTER(ctypes.c_int)]
        f.restype = ctypes.c_int; out[name] = f
    return out


def c_driver(stubs, refuse, keep):
    def drv(ctx, ram, before, after):
        b = before[0]
        result = stubs.get((b.cs, b.ip))
        if result is None or refuse:
            return 1
        a = Cpu.from_buffer_copy(b)
        sp2 = (b.sp - 2) & 0xFFFF
        at = physical(b.ss, sp2); ram[at] = b.bp & 255; ram[(at + 1) % SIZE] = b.bp >> 8           # push bp
        rd = lambda o: ram[physical(b.ss, (b.sp + o) & 0xFFFF)] | ram[physical(b.ss, (b.sp + o + 1) & 0xFFFF)] << 8
        a.di, a.es = rd(4), rd(6)                                                                    # les di,[bp+6]
        t = physical(a.es, (a.di + 8) & 0xFFFF); ram[t] = 0; ram[(t + 1) % SIZE] = 0                # BufPos := 0
        a.ax = result
        a.ip, a.cs = rd(0), rd(2); a.sp = (b.sp + 8) & 0xFFFF                                        # pop bp; retf 4
        after[0] = a; return 0
    fn = DRIVER_FN(drv); keep.append(fn)
    return TextEnv(fn, None)


def build(image_reloc, base, name, *, ss=SS, sp=0xF000, flags=2, mode=0xD7B2, res=0, bufsize=8, bufpos=0, bufend=0x4242, flushfunc=True,
          r_io=0, r_flush=0, text=b'ABC', width=0, t_off=0x0100, buf_off=0x0400, s_off=0x0800, cpu=None, fill=0x5A, stub_seg=STUB):
    m = bytearray([fill]) * SIZE
    b = base * 16; m[b:b + len(image_reloc)] = image_reloc
    ds = base + DATA_SEG
    put16(m, ds, 0x4426, res)
    s0, s1 = stub(r_io), stub(r_flush)
    at = physical(stub_seg, 0); m[at:at + len(s0)] = s0
    at = physical(stub_seg, 0x20); m[at:at + len(s1)] = s1
    t = (TSEG, t_off)
    for off, v in ((0, 5), (2, mode), (4, bufsize), (6, 0), (8, bufpos), (0x0A, bufend), (0x0C, buf_off), (0x0E, TSEG),
                   (0x10, 0x1111), (0x12, 0x2222), (0x14, 0x0000), (0x16, stub_seg), (0x18, 0x0020), (0x1A, stub_seg if flushfunc else 0),
                   (0x1C, 0x3333), (0x1E, 0x4444)):
        put16(m, TSEG, (t_off + off) & 0xFFFF, v)
    m[physical(TSEG, s_off)] = len(text)
    for k, ch in enumerate(text):
        m[physical(TSEG, (s_off + 1 + k) & 0xFFFF)] = ch
    words = (RET_IP, RET_CS, width & 0xFFFF, s_off, TSEG, t_off, TSEG) if name == '0919' else (RET_IP, RET_CS, t_off, TSEG)
    for k, v in enumerate(words):
        put16(m, ss, (sp + 2 * k) & 0xFFFF, v)
    regs = cpu or (0x1234, 0x3456, 0x5678, 0x789A, 0x9ABC, 0xBCDE, 0x7777)
    stubs = {(stub_seg, 0): r_io, (stub_seg, 0x20): r_flush}
    return m, Cpu(*regs, sp, base + SYS_SEG, SPANS[name][0], ds, 0x1111, ss, flags), stubs


def run_unicorn(label, m, c, stub_seg, refuse):
    from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE, UC_HOOK_INTR
    from unicorn import x86_const as R
    uc = Uc(UC_ARCH_X86, UC_MODE_16); backing = (ctypes.c_uint8 * SIZE).from_buffer_copy(m)
    uc.mem_map_ptr(0, SIZE, 7, ctypes.addressof(backing)); uc.mem_map_ptr(SIZE, SIZE, 7, ctypes.addressof(backing))
    for (n, _), reg in zip(Cpu._fields_, REGISTERS):
        uc.reg_write(reg, getattr(c, n))
    s = dict(stop=None, steps=0, last=None); seen, edges, drivers = set(), set(), [0]

    def code(em, address, size, user):
        cs = uc.reg_read(R.UC_X86_REG_CS); ip = uc.reg_read(R.UC_X86_REG_IP)
        if (cs, ip) == (RET_CS, RET_IP):
            s['stop'] = 'RETURNED'; return uc.emu_stop()
        if cs == stub_seg:
            if ip in (0, 0x20):
                drivers[0] += 1
                if refuse:
                    s['stop'] = 'SERVICE'; return uc.emu_stop()
            s['last'] = None; return
        check(cs == BASE + SYS_SEG, f'escape {label} {cs:04x}:{ip:04x}')
        if s['last'] is not None:
            edges.add((s['last'], ip))
        seen.add(ip); s['last'] = ip
        s['steps'] += 1
        if s['steps'] > 2_000_000:
            s['stop'] = 'RUNAWAY'; uc.emu_stop()

    def intr(em, intno, user):
        raise ValueError(f'unexpected INT {intno:#x} {label}')
    uc.hook_add(UC_HOOK_CODE, code); uc.hook_add(UC_HOOK_INTR, intr)
    uc.emu_start(physical(c.cs, c.ip), 2 * SIZE, count=5_000_000)
    check(s['stop'] in ('RETURNED', 'SERVICE'), f'original did not stop {label}: {s["stop"]}')
    return uc, backing, s['stop'], seen, edges, drivers[0]


def compare(fs, label, name, state, refuse=False):
    m, c, stubs = state
    stub_seg = next(iter(stubs))[0]
    uc, backing, stop, seen, edges, drivers = run_unicorn(label, m, c, stub_seg, refuse)
    actual = Cpu.from_buffer_copy(c); ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(m); keep = []
    env = c_driver(stubs, refuse, keep); st = ctypes.c_int(99)
    status = fs[name](ram, SIZE, ctypes.byref(actual), ctypes.byref(env), ctypes.byref(st))
    check(STATUS.get(status) == stop, f'status {label}: C {STATUS.get(status)} original {stop}')
    if bytes(ram) != bytes(backing):
        diff = [hex(i) for i, (a, b) in enumerate(zip(bytes(ram), bytes(backing))) if a != b]
        raise ValueError(f'RAM {label}: {diff[:8]} ({len(diff)} bytes)')
    for (n, _), reg in zip(Cpu._fields_, REGISTERS):
        want = uc.reg_read(reg) & 0xFFFF
        check(getattr(actual, n) == want, f'CPU {label}/{n} C {getattr(actual, n):04x} original {want:04x}')
    return stop, seen, edges, drivers


def cases(name):
    rng = random.Random(int('0af8' + name, 16)); out = []
    for mode in (0xD7B2, 0xD7B1, 0xD7B0, 0x0000):
        for res in (0, 0x0067):
            out.append((f'mode{mode:x}-res{res:x}', dict(mode=mode, res=res)))
    for bufsize, bufpos in ((8, 0), (4, 2), (2, 1), (1, 0), (3, 2), (0x40, 0x3E)):
        for flush in (True, False):
            for rio, rfl in ((0, 0), (0x0065, 0), (0, 0x0066)):
                out.append((f'buf{bufsize}-{bufpos}-fl{int(flush)}-r{rio:x}-{rfl:x}',
                            dict(bufsize=bufsize, bufpos=bufpos, flushfunc=flush, r_io=rio, r_flush=rfl)))
    if name == '0919':
        for text in (b'', b'X', b'HELLO', b'ABCDEFGHIJKLMNOPQRST'):
            for width in (0, 1, 3, 7, 25, 0xFFFF, 0x8000):
                out.append((f'len{len(text)}-w{width:x}', dict(text=text, width=width, bufsize=6, bufpos=1)))
    out += [(f'flags{f:x}', dict(flags=f)) for f in FLAGS]
    out += [('ss-a20', dict(ss=0xFFFF, sp=0x8000)), ('sp-low', dict(sp=0x0040)), ('buf-wrap', dict(buf_off=0xFFFC, bufsize=6, bufpos=2)),
            ('refuse-io', dict(bufsize=1, bufpos=0, refuse=True)), ('refuse-flush', dict(bufsize=0x40, flushfunc=True, refuse=True)),
            ('stub-high', dict(stub_seg=0xF800, bufsize=2))]
    for k in range(50):
        out.append((f'random{k}', dict(mode=rng.choice((0xD7B2, 0xD7B2, 0xD7B1)), res=rng.choice((0, 0, 0x67)), bufsize=rng.randrange(1, 12),
                                       bufpos=rng.randrange(0, 4), flushfunc=rng.random() < 0.6, r_io=rng.choice((0, 0, 0x65)),
                                       r_flush=rng.choice((0, 0, 0x66)), text=bytes(rng.randrange(32, 127) for _ in range(rng.randrange(0, 12))),
                                       width=rng.choice((0, 3, 9, rng.randrange(0x10000))), flags=rng.choice(FLAGS),
                                       sp=rng.randrange(0x0800, 0xF000) & ~1, cpu=tuple(rng.randrange(65536) for _ in range(7)),
                                       fill=rng.randrange(256))))
    return out


def differential(image_reloc, decoded, fs):
    report, all_seen, all_edges = {}, set(), set()
    for name in ENTRIES:
        stops, drivers = {}, 0
        for label, o in cases(name):
            o = dict(o); refuse = o.pop('refuse', False)
            if name == '084a':
                o.pop('text', None); o.pop('width', None)
            stop, seen, edges, d = compare(fs, f'{name} {label}', name, build(image_reloc, BASE, name, **o), refuse)
            stops[stop] = stops.get(stop, 0) + 1; drivers += d; all_seen |= seen; all_edges |= edges
        report[name] = dict(cases=sum(stops.values()), stops=stops, driver_calls=drivers)
    starts = set(decoded)
    branches = {(a, t) for a, i in decoded.items() if (i.mnemonic.startswith('j') and i.mnemonic != 'jmp') or i.mnemonic.startswith('loop')
                for t in (int(i.op_str, 16), a + i.size)}
    check(starts <= all_seen, f'starts missing {sorted(hex(a) for a in starts - all_seen)}')
    check(branches <= all_edges, f'outcomes missing {sorted((hex(a), hex(b)) for a, b in branches - all_edges)}')
    report.update(starts=f'{len(starts & all_seen)}/{len(starts)}', outcomes=f'{len(branches & all_edges)}/{len(branches)}',
                  cpu_words=14, ram_bytes=SIZE, flag_masks=0)
    return report


def main():
    p = argparse.ArgumentParser()
    for flag in ('--compile-only', '--unicorn', '--native'):
        p.add_argument(flag, action='store_true')
    p.add_argument('--output', type=Path); a = p.parse_args()
    with tempfile.TemporaryDirectory(prefix='text-') as tmp:
        fs = library(Path(tmp))
        if a.compile_only:
            print('CHECKER_BUILD_OK'); return
        image, sites = load_image()
        meta, decoded = source(image, sites)
        report = dict(targets=meta, image_sha256=IMAGE_DIGEST, relocations=len(sites), load_base=f'{BASE:04X}')
        if a.unicorn:
            report['unicorn'] = differential(relocate(image, sites, BASE), decoded, fs)
            for _row in ('RES_0AF8_084A', 'RES_0AF8_0919'):
                print(f"ROW_EXECUTED {_row} PASS")
        if a.native:
            import system_text_write_cpu_native as N
            report['native'] = N.native(image, sites, fs, __import__(__name__))
    if a.output:
        a.output.parent.mkdir(parents=True, exist_ok=True); a.output.write_text(json.dumps(report, sort_keys=True, indent=2) + '\n')
    print(json.dumps(report, sort_keys=True))


if __name__ == '__main__':
    import sys
    sys.modules['system_text_write_cpu_audit'] = sys.modules[__name__]
    main()
