#!/usr/bin/env python3
"""Turbo Pascal SYSTEM Reset 0AF8:1410 (raw CPU; runtime coverage, no identity) against the original bytes.

INT 21h is the declared environment: AH=3Dh (open) answers from the case's script (carry and AX); AH=3Eh (close, issued by
the REAL Close bytes the nested call runs in Unicorn) succeeds; any other AH or interrupt is refused. The nested Close is a
named stop in the model (CLOSE). The comparison has two phases: the model's CLOSE stop against the original's state on
arrival at 1491h, then the model resumed at 1445h from the original's post-Close state against the original's final state.
Unicorn: CPU14 + full 1 MiB at both points. Native (DOSBox-X): cases without the nested Close end to end; Close-path cases
captured at the Close entry (a declared native-copy patch). Media and generated programs stay under ignored resources/."""
import argparse, ctypes, json, os, random, struct, subprocess, tempfile
from hashlib import sha256
from pathlib import Path
from capstone import Cs, CS_ARCH_X86, CS_MODE_16
from resident_loader_map import unpack
from res0709_ega_reset_0000_cpu_audit import Cpu, ROOT, SIZE, REGISTERS, physical, put16, check
CHECKER_ROWS = ('RES_0AF8_1410',)
PIECES = ((0x1410, 0x1475, '18b5680c7d5ea7bfdee3902b7a11279f62216c07fbff9c692fb8820363b481f0', 42),
          (0x1491, 0x14C5, None, None))                       # Close + its check helper: the original's own, run by Unicorn
REWRITE_ENTRY = {0x1419, 0x141C}                                # the Rewrite entry's two instructions: a separate endpoint
CS, DS, SS, FREC = 0x6000, 0x5000, 0x2000, 0x3000
FLAGS = (0x0002, 0x00D7, 0x08D7, 0x0402, 0x0893, 0x0003)
CONDITIONALS = {0x142B: (0x1447, 0x142D), 0x1433: (0x143D, 0x1435), 0x144C: (0x1460, 0x144E), 0x1459: (0x1461, 0x145B)}
SERVICE_FN = ctypes.CFUNCTYPE(ctypes.c_int, ctypes.c_void_p, ctypes.POINTER(Cpu), ctypes.POINTER(Cpu))
STATUS = {0: 'RETURN', 1: 'ARGUMENT', 2: 'SERVICE', 3: 'ENV', 4: 'CLOSE'}


def source():
    image = bytes(unpack((ROOT / 'resources/DOS/POOLRAD/START.EXE').read_bytes())[0])
    pieces, decoded = [], {}
    for start, end, digest, count in PIECES:
        body = image[physical(0xAF8, start):physical(0xAF8, end)]
        ins = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(body, start))
        check(sum(i.size for i in ins) == len(body), f'decode covers {start:04x}')
        if digest:
            check(sha256(body).hexdigest() == digest and len(ins) == count, 'Reset span identity')
            decoded.update({i.address: i for i in ins})
        pieces.append((start, body))
    check([a for a, i in decoded.items() if i.mnemonic == 'int'] == [0x1456], 'single INT 21h in Reset')
    check([(a, i.op_str) for a, i in decoded.items() if i.mnemonic == 'call'] == [(0x1442, '0x1491')], 'single near CALL Close')
    check({a for a, i in decoded.items() if i.mnemonic.startswith('j') and i.mnemonic != 'jmp'} == set(CONDITIONALS), 'conditional sites')
    live = {a: i for a, i in decoded.items() if a not in REWRITE_ENTRY}
    return pieces, live


def library(directory):
    path = directory / 'reset.so'
    subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Wpedantic', '-Werror', '-O2', '-fPIC', '-shared', '-I', str(ROOT / 'include'),
                    str(ROOT / 'src/recovered/system_reset_1410_cpu.c'), '-o', str(path)], check=True)
    f = ctypes.CDLL(str(path)).pr_system_reset_1410
    f.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t, ctypes.POINTER(Cpu), SERVICE_FN, ctypes.c_void_p]
    f.restype = ctypes.c_int
    return f


def build(pieces, *, magic=0xD7B0, name=b'GAME.DAT', handle=0x0007, mode=2, recsize=128, open_cf=0, open_ax=0x0005, close_cf=0,
          close_ax=0x0006, f=(FREC, 0x0100), ss=SS, sp=0x8000, flags=2, ds=DS, regs=None):
    m = bytearray([0x5A]) * SIZE
    for start, body in pieces:
        at = physical(CS, start); m[at:at + len(body)] = body
    m[physical(ds, 0x442C)] = mode; put16(m, ds, 0x4426, 0x7777)
    seg, off = f
    put16(m, seg, off, handle); put16(m, seg, off + 2, magic); put16(m, seg, off + 4, 0x1111)
    for k in range(0x50):
        m[physical(seg, (off + 0x30 + k) & 0xFFFF)] = 0x5A
    for k, ch in enumerate(name + b'\x00'):
        m[physical(seg, (off + 0x30 + k) & 0xFFFF)] = ch
    for j, w in enumerate((0x7000, 0x9001, recsize, off, seg)):
        put16(m, ss, (sp + 2 * j) & 0xFFFF, w)
    r = regs or (0x1234, 0x3456, 0x5678, 0x789A, 0x9ABC, 0xBCDE, 0x7777)
    return m, Cpu(*r, sp, CS, 0x1410, ds, 0x1111, ss, flags), dict(open=(open_cf, open_ax), close=(close_cf, close_ax))


def dos_answer(cpu_flags, ax, script, ah):
    cf, value = script['open' if ah == 0x3D else 'close']
    return value if cf else (value if ah == 0x3D else ax), (cpu_flags & ~1) | cf


def compare(name, state, live, f):
    from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE, UC_HOOK_INTR
    from unicorn import x86_const as R
    m, c, script = state
    uc = Uc(UC_ARCH_X86, UC_MODE_16); backing = (ctypes.c_uint8 * SIZE).from_buffer_copy(m)
    uc.mem_map_ptr(0, SIZE, 7, ctypes.addressof(backing)); uc.mem_map_ptr(SIZE, SIZE, 7, ctypes.addressof(backing))
    for (n, _), reg in zip(Cpu._fields_, REGISTERS):
        uc.reg_write(reg, getattr(c, n))
    s = dict(stop=None, ret=False, prev=None, at_close=None, resume=None); seen = set(); edges = set(); log = []

    def regs():
        return Cpu(*[uc.reg_read(r) & 0xFFFF for r in REGISTERS])

    def code(em, address, size, user):
        ip = uc.reg_read(R.UC_X86_REG_IP); cs = uc.reg_read(R.UC_X86_REG_CS)
        if s['prev'] in CONDITIONALS:
            edges.add((s['prev'], ip))
        s['prev'] = None
        if s['ret']:
            s['stop'] = 'RETURN'; return uc.emu_stop()
        check(cs == CS, f'escape {name} {cs:04x}:{ip:04x}')
        if ip == 0x1491:
            s['at_close'] = (regs(), bytes(backing))
        if ip == 0x1445:
            s['resume'] = (regs(), bytes(backing))
        if ip in live:
            seen.add(ip); s['prev'] = ip
            if ip == 0x1472:
                s['ret'] = True

    def intr(em, intno, user):
        ah = uc.reg_read(R.UC_X86_REG_AX) >> 8
        check(intno == 0x21 and ah in (0x3D, 0x3E), f'ENV refusal {name}: INT {intno:#x} AH {ah:#x}')
        ax, fl = dos_answer(uc.reg_read(R.UC_X86_REG_EFLAGS), uc.reg_read(R.UC_X86_REG_AX), script, ah)
        log.append((ah, uc.reg_read(R.UC_X86_REG_AX) & 0xFF, uc.reg_read(R.UC_X86_REG_DS), uc.reg_read(R.UC_X86_REG_DX)))
        uc.reg_write(R.UC_X86_REG_AX, ax); uc.reg_write(R.UC_X86_REG_EFLAGS, fl)
    uc.hook_add(UC_HOOK_CODE, code); uc.hook_add(UC_HOOK_INTR, intr)
    uc.emu_start(physical(c.cs, c.ip), 2 * SIZE, count=2000)
    check(s['stop'] == 'RETURN', f'original did not return {name}')
    clog = []

    def svc(ctx, before, after):
        b = before[0]; ax, fl = dos_answer(b.flags, b.ax, script, b.ax >> 8)
        clog.append((b.ax >> 8, b.ax & 0xFF, b.ds, b.dx))
        a = Cpu.from_buffer_copy(b); a.ax = ax; a.flags = fl; a.ip = 0x1458; after[0] = a; return 0
    fn = SERVICE_FN(svc)
    actual = Cpu.from_buffer_copy(c); ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(m)
    status = STATUS[f(ram, SIZE, ctypes.byref(actual), fn, None)]
    phases = 1
    if s['at_close'] is not None:
        check(status == 'CLOSE', f'C status {name}: {status}, original reached Close')
        want_cpu, want_ram = s['at_close']
        check(bytes(ram) == want_ram, f'RAM at Close {name}')
        for (n, _), _r in zip(Cpu._fields_, REGISTERS):
            check(getattr(actual, n) == getattr(want_cpu, n), f'CPU at Close {name}/{n}')
        check(s['resume'] is not None, f'original did not come back from Close {name}')
        rcpu, rram = s['resume']
        actual = Cpu.from_buffer_copy(rcpu); ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(rram)
        status = STATUS[f(ram, SIZE, ctypes.byref(actual), fn, None)]; phases = 2
    check(status == 'RETURN', f'C status {name}: {status}')
    check(clog == [e for e in log if e[0] == 0x3D], f'INT 21h 3Dh log {name}: C {clog} original {log}')
    if bytes(ram) != bytes(backing):
        diff = [hex(i) for i, (a, b) in enumerate(zip(bytes(ram), bytes(backing))) if a != b]
        raise ValueError(f'RAM {name}: {diff[:8]} ({len(diff)} bytes)')
    for (n, _), reg in zip(Cpu._fields_, REGISTERS):
        want = uc.reg_read(reg) & 0xFFFF
        check(getattr(actual, n) == want, f'CPU {name}/{n} C {getattr(actual, n):04x} original {want:04x}')
    return seen, edges, bytes(ram), phases, log


def expected(o):
    """Independent statement of Reset's effect on the FileRec and InOutRes: (magic, handle, recsize, inoutres)."""
    magic, handle = o.get('magic', 0xD7B0), o.get('handle', 7)
    if magic not in (0xD7B0, 0xD7B3):
        return magic, handle, 0x1111, 102
    if not o.get('name', b'GAME.DAT'):
        return 0xD7B3, 0, o.get('recsize', 128), 0x7777
    cf, ax = o.get('open_cf', 0), o.get('open_ax', 5)
    if cf:
        closed = magic == 0xD7B3                                  # Close sets the magic to D7B0 even if DOS fails
        return (0xD7B0 if closed else magic), handle, 0x1111, ax
    return 0xD7B3, ax, o.get('recsize', 128), 0x7777


def cases():
    rng = random.Random(0x0AF81410)
    out = [('closed-open', dict()), ('closed-empty-name', dict(name=b'')), ('closed-open-error', dict(open_cf=1, open_ax=2)),
           ('unassigned', dict(magic=0x1234)), ('unassigned-zero', dict(magic=0x0000)), ('input-magic', dict(magic=0xD7B1)),
           ('open-then-reopen', dict(magic=0xD7B3, handle=7)), ('open-std-handle', dict(magic=0xD7B3, handle=2)),
           ('open-handle-4', dict(magic=0xD7B3, handle=4)), ('open-handle-5', dict(magic=0xD7B3, handle=5)),
           ('open-close-error', dict(magic=0xD7B3, handle=9, close_cf=1, close_ax=6)), ('open-empty-name', dict(magic=0xD7B3, name=b'')),
           ('open-reopen-error', dict(magic=0xD7B3, open_cf=1, open_ax=3))]
    out += [(f'mode{md}', dict(mode=md)) for md in (0, 1, 2, 0x40, 0xFF)]
    out += [(f'recsize{r}', dict(recsize=r)) for r in (0, 1, 0xFFFF)]
    out += [(f'flags{f:x}', dict(flags=f)) for f in FLAGS] + [(f'flags{f:x}-close', dict(flags=f, magic=0xD7B3)) for f in FLAGS]
    out += [(f'sp{sp:x}', dict(sp=sp)) for sp in (0x000E, 0x000F, 0x8001, 0xFFF5, 0xFFF6)]
    out += [('ss-ds', dict(ss=DS)), ('a20-stack', dict(ss=0xFFFF, sp=0x0400)), ('frec-straddle', dict(f=(FREC, 0xFFFC))),
            ('frec-a20', dict(f=(0xFFFF, 0x0040))), ('name-wrap', dict(f=(FREC, 0xFFD8), name=b'ABCDEFGHIJKL')),
            # FileRec over the stack frame: the handle/recsize stores rewrite the return before RETF 6.
            ('alias-frec-on-frame', dict(f=(SS, 0x7FFC), sp=0x8000))]
    for k in range(24):
        out.append((f'random{k}', dict(magic=rng.choice((0xD7B0, 0xD7B3, rng.randrange(65536))), name=rng.choice((b'', b'A', b'SAVE.DAT')),
                                       handle=rng.randrange(12), mode=rng.randrange(256), recsize=rng.randrange(65536),
                                       open_cf=rng.randrange(2), open_ax=rng.randrange(1, 20), close_cf=rng.randrange(2),
                                       flags=rng.choice(FLAGS), regs=tuple(rng.randrange(65536) for _ in range(7)))))
    return out


def differential(pieces, live, f):
    seen, edges = set(), set(); specs = cases(); two = 0; checked = 0
    for name, o in specs:
        v, e, ram, phases, log = compare(name, build(pieces, **o), live, f); seen |= v; edges |= e; two += phases == 2
        if name.startswith(('alias', 'ss-ds', 'frec-', 'name-wrap')):
            continue
        seg, off = o.get('f', (FREC, 0x0100))
        got = (ram[physical(seg, off + 2)] | ram[physical(seg, off + 3)] << 8, ram[physical(seg, off)] | ram[physical(seg, off + 1)] << 8,
               ram[physical(seg, off + 4)] | ram[physical(seg, off + 5)] << 8, ram[physical(DS, 0x4426)] | ram[physical(DS, 0x4427)] << 8)
        want = expected(o)
        if o.get('magic', 0xD7B0) == 0xD7B3 and o.get('close_cf', 0) and o.get('handle', 7) > 4 and want[3] == 0x7777:
            want = want[:3] + (o.get('close_ax', 6),)      # Close stored its DOS error; Reset leaves it
        check(got == want, f'independent {name}: {[hex(x) for x in got]} vs {[hex(x) for x in want]}')
        if o.get('magic', 0xD7B0) == 0xD7B3 and o.get('name', b'GAME.DAT'):
            closed = [e for e in log if e[0] == 0x3E]
            check(bool(closed) == (o.get('handle', 7) > 4), f'{name}: Close issues 3Eh only for handles above 4')
        checked += 1
    check(seen == set(live), f'starts missing {sorted(hex(a) for a in set(live) - seen)}')
    want = {(a, t) for a, ts in CONDITIONALS.items() for t in ts}
    check(edges == want, f'outcomes missing {sorted((hex(a), hex(t)) for a, t in want - edges)}')
    return dict(cases=len(specs), stops=dict(RETURN=len(specs)), close_stop_then_resume=two, starts=len(seen), starts_total=len(live),
                excluded_starts='1419/141C: the Rewrite entry (a separate endpoint)', conditional_outcomes=len(edges),
                conditional_outcomes_total=len(want), independent_checks=checked, cpu_words=14, ram_bytes=SIZE, flag_masks=0)


def main():
    p = argparse.ArgumentParser()
    for flag in ('--compile-only', '--unicorn', '--native'):
        p.add_argument(flag, action='store_true')
    p.add_argument('--output', type=Path); a = p.parse_args()
    with tempfile.TemporaryDirectory(prefix='reset-') as tmp:
        f = library(Path(tmp))
        if a.compile_only:
            print('CHECKER_BUILD_OK'); return
        pieces, live = source()
        report = dict(target=dict(span='0AF8:[1410,1475)', entry='1410 (Reset)', bytes=0x65, instructions=42, sha256=PIECES[0][2]))
        if a.unicorn:
            report['unicorn'] = differential(pieces, live, f)
        if a.native:
            import system_reset_1410_native
            report['native'] = system_reset_1410_native.native(pieces, f)
    if a.output:
        a.output.write_text(json.dumps(report, sort_keys=True, indent=2) + '\n')
    print(json.dumps(report, sort_keys=True))


if __name__ == '__main__':
    main()
