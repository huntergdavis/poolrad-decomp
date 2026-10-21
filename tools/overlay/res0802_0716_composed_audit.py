#!/usr/bin/env python3
"""The t30 RES_0802 cluster composed end to end (VP i20311 / i20371): 0716 with every near callee and QUIT run by its real C
model (pr_r0802_0716_composed), against the original, run ONCE from 0716's entry to its end state with every original byte
of the cluster and its runtime closure executing. There are no seams: one comparison per case, at the end state.

Declared environment, identical for the original and the C callbacks: the 0716 audit's DOS file service (INT 21h
3Dh/3Eh/3Fh/42h; one archive, lowest-free handles, the insert-disk event), the 0193 audit's real-byte LOGGING INT 10h stub and
ReadKey's services (INT 21h 2Ch/25h/35h, ports, INT 16h from a per-case key script with AH=01h peeking the script). Compared
unmasked at the end: CPU14, all 1 MiB, the file service state and its ordered log, and the ordered environment event log.
End states: RETURN; EXIT (Halt 0AF8:00E9); RUNERROR (0AF8:00E2); BREAK (Crt 0A40:014B); DIVIDE (INT 0); STALLED (050F's fixed
point, the original stopped at the same 051F arrival and then run 1,000 more arrivals unchanged). Media stay under ignored
resources/."""
import argparse, ctypes, json, random, struct, subprocess, sys, tempfile, time
from hashlib import sha256
from pathlib import Path
from res0709_ega_reset_0000_cpu_audit import Cpu, ROOT, SIZE, REGISTERS, physical, put16, check
import res0899_quit_0000_cpu_audit as Q
import res0899_readkey_0059_cpu_audit as R
import res0802_0040_cpu_audit as B
import res0802_0193_cpu_audit as P
import res0802_0716_cpu_audit as A716
import res0802_05cd_cpu_audit as O
import res0802_050f_cpu_audit as X
import res0802_0020_cpu_audit as G
import res0802_0000_cpu_audit as F
import res0802_06e7_cpu_audit as C6
CHECKER_ROWS = ('RES_0802_0716',)
BASE, STUB_SEG, LOG_SEG, SCRATCH = Q.BASE, Q.STUB_SEG, B.LOG_SEG, P.SCRATCH
SOURCES = list(dict.fromkeys(list(A716.SOURCES) + O.SOURCES + X.SOURCES + G.SOURCES + F.SOURCES + C6.SOURCES + list(R.SOURCES) +
                             ['resident_0802_0193_cpu', 'resident_0802_0175_cpu', 'resident_0802_0105_cpu', 'resident_0802_0040_cpu',
                              'resident_0802_0716_composed_cpu']))
STATUS = {0: 'RETURN', 1: 'ARGUMENT', 2: 'EXIT', 3: 'RUNERROR', 4: 'DIVIDE', 5: 'BREAK', 6: 'STALLED', 7: 'LIMIT', 8: 'SERVICE',
          9: 'CALLEE'}
ENVSPEC = dict(table={8: (0x0123, 0xF000)}, in61=0xB0, time_start=4736147, time_step=7)
DIRTY_FLAGS = (0x0202, 0x0602, 0x0A87, 0x0402, 0x0E57)       # the standing dirty-incoming-flags set (VP i20371)
STALL_EXTRA = 1000
SERVICE_FN = O.SERVICE_FN


class FileEnv(ctypes.Structure):
    _fields_ = [('open', SERVICE_FN), ('open_context', ctypes.c_void_p), ('close', SERVICE_FN), ('close_context', ctypes.c_void_p)]


class Env(ctypes.Structure):
    _fields_ = [('loader', A716.Env716), ('file', FileEnv), ('prompt', P.Env193), ('heap_steps', ctypes.c_uint),
                ('decode_arrivals', ctypes.c_uint32)]


class Diag(ctypes.Structure):
    _fields_ = [('seam', ctypes.c_uint16), ('nested', ctypes.c_uint16), ('leaf_status', ctypes.c_int), ('seams_composed', ctypes.c_uint),
                ('prompts', ctypes.c_uint), ('decode_arrivals', ctypes.c_uint32)]


def library(directory):
    path = directory / 'r0716c.so'
    subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Wpedantic', '-Werror', '-O2', '-fPIC', '-shared', '-I', str(ROOT / 'include')]
                   + [str(ROOT / f'src/recovered/{n}.c') for n in SOURCES] + ['-o', str(path)], check=True)
    f = ctypes.CDLL(str(path)).pr_r0802_0716_composed
    f.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t, ctypes.POINTER(Cpu), ctypes.POINTER(Env), ctypes.POINTER(Diag)]
    f.restype = ctypes.c_int
    return f


def stall_archive(block_id=40):
    """One row whose packed data stalls 050F: a 2-byte literal, then control 7Fh while in (3) is below the packed size (4)."""
    packed = b'\x01AB\x7f'
    row = struct.pack('<BIHH', block_id, 0, 16, len(packed))
    return struct.pack('<H', len(row)) + row + packed


def build(reloc, stub, o):
    o = dict(o); extra = {k: o.pop(k) for k in ('appear_after', 'gone_from', 'seek_fail', 'keys', 'typeahead') if k in o}
    m, c, spec = A716.build(reloc, keys=extra.get('keys', (0x1C0D,)), **o)
    s = physical(STUB_SEG, 0); m[s:s + len(stub)] = stub         # the 0193 audit's logging INT 10h stub, in place of QUIT's
    put16(m, LOG_SEG, 0, 0)
    spec.update(appear_after=extra.get('appear_after'), gone_from=extra.get('gone_from'), seek_fail=extra.get('seek_fail', False),
                typeahead=list(extra.get('typeahead', ())))
    return m, c, spec


def file_service(spec):
    return A716.FileSystem(spec['blob'], spec['read_fail'], spec['seek_answer'], spec['first_handle'], appear_after=spec['appear_after'],
                           gone_from=spec['gone_from'], seek_fail=spec['seek_fail'])


def run_unicorn(name, m, c, spec, stop_at=None):
    from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE, UC_HOOK_INTR, UC_HOOK_INSN
    from unicorn import x86_const as RR
    env = Q.Env(**ENVSPEC); fs = file_service(spec); script = list(spec['keys']); ahead = list(spec['typeahead'])
    base = spec['base']; cs0802 = base + 0x0802; sysseg = base + Q.SEG['system']
    uc = Uc(UC_ARCH_X86, UC_MODE_16); backing = (ctypes.c_uint8 * SIZE).from_buffer_copy(m)
    uc.mem_map_ptr(0, SIZE, 7, ctypes.addressof(backing)); uc.mem_map_ptr(SIZE, SIZE, 7, ctypes.addressof(backing))
    for (n, _), reg in zip(Cpu._fields_, REGISTERS):
        uc.reg_write(reg, getattr(c, n))
    s = dict(stop=None, steps=0, arrivals=0, snap=None, after=None)
    known = {base + v for v in Q.SEG.values()} | {cs0802, STUB_SEG, base + R.CRT}

    def cpu_now():
        return Cpu(*[uc.reg_read(r) & 0xFFFF for r in REGISTERS])

    def code(em, address, size, user):
        cs = uc.reg_read(RR.UC_X86_REG_CS); ip = uc.reg_read(RR.UC_X86_REG_IP)
        if (cs, ip) == spec['ret']:
            s['stop'] = 'RETURN'; return uc.emu_stop()
        if (cs, ip) == (sysseg, 0x00E2):
            s['stop'] = 'RUNERROR'; return uc.emu_stop()
        if (cs, ip) == (sysseg, 0x00E9):
            s['stop'] = 'EXIT'; return uc.emu_stop()
        if (cs, ip) == (base + R.CRT, 0x014B):
            s['stop'] = 'BREAK'; return uc.emu_stop()
        check(cs in known, f'escape {name} {cs:04x}:{ip:04x}')
        if stop_at is not None and (cs, ip) == (cs0802, 0x051F):
            s['arrivals'] += 1
            if s['arrivals'] == stop_at:
                s['snap'] = (cpu_now(), bytes(backing))
            elif s['arrivals'] > stop_at:
                check(bytes(cpu_now()) == bytes(s['snap'][0]), f'{name}: CPU changed at 050F arrival {s["arrivals"]}')
                if s['arrivals'] == stop_at + STALL_EXTRA:
                    s['after'] = bytes(backing); s['stop'] = 'STALLED'; return uc.emu_stop()
        s['steps'] += 1
        if s['steps'] > 60000000:
            s['stop'] = 'RUNAWAY'; uc.emu_stop()

    def intr(em, intno, user):
        ax = uc.reg_read(RR.UC_X86_REG_AX); ah, al = ax >> 8, ax & 0xFF; fl = uc.reg_read(RR.UC_X86_REG_EFLAGS)
        if intno == 0:
            s['stop'] = 'DIVIDE'; return uc.emu_stop()
        if intno == 0x16:
            check(ah in (0x00, 0x01), f'ENV refusal {name}: INT 16h AH {ah:#x}')
            # The #1326 keyboard contract: AH=01h reports only type-ahead; AH=00h takes type-ahead first, else the next key
            # the player presses (one waited key per wait).
            if ah == 0x01:
                env.log.append(('16', 1, ahead[0] if ahead else None))
                if ahead:
                    uc.reg_write(RR.UC_X86_REG_AX, ahead[0]); uc.reg_write(RR.UC_X86_REG_EFLAGS, fl & ~0x40)
                else:
                    uc.reg_write(RR.UC_X86_REG_EFLAGS, fl | 0x40)
                return
            src = ahead if ahead else script
            check(bool(src), f'ENV refusal {name}: INT 16h AH 00h with the script empty')
            k = src.pop(0); env.log.append(('16', 0, k)); uc.reg_write(RR.UC_X86_REG_AX, k); return
        check(intno == 0x21, f'ENV refusal {name}: INT {intno:#x}')
        if ah == 0x2C:
            cx, dx = env.gettime(); uc.reg_write(RR.UC_X86_REG_CX, cx); uc.reg_write(RR.UC_X86_REG_DX, dx); return
        if ah in (0x25, 0x35):
            sp = uc.reg_read(RR.UC_X86_REG_SP); ss_ = uc.reg_read(RR.UC_X86_REG_SS)
            for k, v in ((2, fl & 0xFFFF), (4, uc.reg_read(RR.UC_X86_REG_CS)), (6, uc.reg_read(RR.UC_X86_REG_IP))):
                uc.mem_write(physical(ss_, (sp - k) & 0xFFFF), struct.pack('<H', v))
            out = env.dos(ah, al, dict(ds=uc.reg_read(RR.UC_X86_REG_DS), dx=uc.reg_read(RR.UC_X86_REG_DX)))
            for k, v in out.items():
                uc.reg_write(dict(es=RR.UC_X86_REG_ES, bx=RR.UC_X86_REG_BX)[k], v)
            return
        check(ah in (0x3D, 0x3E, 0x3F, 0x42), f'ENV refusal {name}: INT 21h AH {ah:#x}')
        ds_, dx_ = uc.reg_read(RR.UC_X86_REG_DS), uc.reg_read(RR.UC_X86_REG_DX)
        nm = bytes(backing[physical(ds_, dx_):physical(ds_, dx_) + 80]).split(b'\0')[0].decode('latin-1') if ah == 0x3D else None
        at = physical(ds_, dx_)

        def write(chunk):
            for k, v in enumerate(chunk):
                backing[(at + k) % SIZE] = v
        cf, ax_out, dx_out = fs.call(ah, al, uc.reg_read(RR.UC_X86_REG_BX), uc.reg_read(RR.UC_X86_REG_CX), dx_, nm, write)
        uc.reg_write(RR.UC_X86_REG_AX, ax_out)
        if dx_out is not None:
            uc.reg_write(RR.UC_X86_REG_DX, dx_out)
        uc.reg_write(RR.UC_X86_REG_EFLAGS, (fl | 1) if cf else (fl & ~1))

    def port_in(em, port, size, user):
        check(port == 0x61 and size == 1, f'ENV refusal {name}: IN {port:#x}')
        env.log.append(('in', port, env.in61)); return env.in61

    def port_out(em, port, size, value, user):
        check(port in (0x40, 0x43, 0x61, 0xC0) and size == 1, f'ENV refusal {name}: OUT {port:#x}')
        env.log.append(('out', port, value))
    uc.hook_add(UC_HOOK_CODE, code); uc.hook_add(UC_HOOK_INTR, intr)
    uc.hook_add(UC_HOOK_INSN, port_in, None, 1, 0, RR.UC_X86_INS_IN)
    uc.hook_add(UC_HOOK_INSN, port_out, None, 1, 0, RR.UC_X86_INS_OUT)
    uc.emu_start(physical(c.cs, c.ip), 2 * SIZE, count=80000000)
    check(s['stop'] in ('RETURN', 'RUNERROR', 'EXIT', 'BREAK', 'DIVIDE', 'STALLED'), f'original stop {name}: {s["stop"]}')
    if s['stop'] == 'STALLED':
        check(s['after'] == s['snap'][1], f'{name}: RAM changed across {STALL_EXTRA} further 050F iterations')
        return s['stop'], s['snap'][0], s['snap'][1], fs, env
    return s['stop'], cpu_now(), bytes(backing), fs, env


def run_c(f, m, c, spec):
    keep = []
    fs = file_service(spec)
    q, env = Q.c_env(ENVSPEC, keep, stub_at=(STUB_SEG, 0))
    video = B.c_video(keep, LOG_SEG, (STUB_SEG, 0), scratch=SCRATCH)
    q.video = video
    script, ahead = list(spec['keys']), list(spec['typeahead'])

    def kb(ctx, before, after):                                   # the #1326 contract, as the Unicorn hook above
        b = before[0]; ah = b.ax >> 8
        a = Cpu.from_buffer_copy(b); a.ip = (b.ip + 2) & 0xFFFF
        if ah == 0x01:
            env.log.append(('16', 1, ahead[0] if ahead else None))
            if ahead:
                a.ax = ahead[0]; a.flags = b.flags & ~0x40
            else:
                a.flags = b.flags | 0x40
            after[0] = a; return 0
        src = ahead if ahead else script
        if ah == 0x00 and src:
            k = src.pop(0); env.log.append(('16', 0, k)); a.ax = k; after[0] = a; return 0
        return 1
    kbfn = R.KB_FN(kb); keep.append(kbfn)
    ke = R.KeyEnv(q, kbfn, None, 64)

    def service(ctx, before, after):
        b = before[0]; nm = None
        if b.ax >> 8 == 0x3D:
            r = ctypes.cast(ctx, ctypes.POINTER(ctypes.c_uint8)); at = physical(b.ds, b.dx)
            nm = bytes(r[at:at + 80]).split(b'\0')[0].decode('latin-1')
        cf, ax, _ = fs.call(b.ax >> 8, b.ax & 0xFF, b.bx, 0, b.dx, nm)
        a = Cpu.from_buffer_copy(b); a.ax = ax; a.flags = (b.flags | 1) if cf else (b.flags & ~1); a.ip = (b.ip + 2) & 0xFFFF
        after[0] = a; return 0

    def read(ctx, before, after, buffer, length):
        b = before[0]

        def write(chunk):
            for k, v in enumerate(chunk[:length]):
                buffer[k] = v
        cf, ax, _ = fs.call(b.ax >> 8, b.ax & 0xFF, b.bx, b.cx, b.dx, None, write)
        a = Cpu.from_buffer_copy(b); a.ax = ax; a.flags = (b.flags | 1) if cf else (b.flags & ~1); a.ip = (b.ip + 2) & 0xFFFF
        after[0] = a; return 0

    def lseek(ctx, before, after):
        b = before[0]
        cf, ax, dx = fs.call(b.ax >> 8, b.ax & 0xFF, b.bx, b.cx, b.dx)
        a = Cpu.from_buffer_copy(b); a.ax = ax
        if dx is not None:
            a.dx = dx
        a.flags = (b.flags | 1) if cf else (b.flags & ~1); a.ip = (b.ip + 2) & 0xFFFF
        after[0] = a; return 0
    fns = (SERVICE_FN(service), A716.READ_FN(read), A716.SEEK_FN(lseek)); keep += fns
    ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(m); cpu = Cpu.from_buffer_copy(c); d = Diag()
    ctx = ctypes.cast(ram, ctypes.c_void_p)
    e = Env(A716.Env716(fns[1], None, fns[2], None, fns[2], None, 100000), FileEnv(fns[0], ctx, fns[0], ctx),
            P.Env193(B.Env(video, None), ke), 100000, 1000000)
    status = STATUS[f(ram, SIZE, ctypes.byref(cpu), ctypes.byref(e), ctypes.byref(d))]
    return status, cpu, bytes(ram), fs, env, d


def cases():
    out = list(A716.cases())
    rng = random.Random(0x0716C0DE)
    synth, _ = A716.synthetic(rng)
    out += [('stall-7f-through-0716', dict(blob=stall_archive(), block_id=40)),
            ('insert-disk-through-05cd', dict(blob=synth, block_id=10, flag557c=0, appear_after=2, keys=(0x1C0D, 0x1C0D))),
            ('insert-disk-two-prompts', dict(blob=synth, block_id=11, flag557c=0, appear_after=3, keys=(0x1C0D, 0x1C0D, 0x1C0D))),
            ('insert-disk-typeahead-drained', dict(blob=synth, block_id=12, flag557c=0, appear_after=3, keys=(0x1C0D, 0x1C0D),
                                                   typeahead=(0x1E61, 0x3062)))]
    ecl1 = (ROOT / 'resources/DOS/POOLRAD/ECL1.DAX').read_bytes()
    first = A716.parse_dax(ecl1)[0][0][0]
    out += [(f'dirty-{fl:04x}', dict(blob=ecl1, block_id=first, flags=fl)) for fl in DIRTY_FLAGS]
    big, _ = A716.synthetic(rng, 2, big=(0x30, 0x9000))
    out += [(f'dirty-prompt-{fl:04x}', dict(blob=big, block_id=0x30, flags=fl)) for fl in DIRTY_FLAGS]
    return out


def differential(reloc, stub, f):
    stops, total_seams, total_prompts, cs_, non_architectural = {}, 0, 0, cases(), []
    for index, (name, o) in enumerate(cs_):
        t0 = time.time()
        m, c, spec = build(reloc, stub, o)
        st, cpu, ram, cfs, cenv, d = run_c(f, m, c, spec)
        print(f'[{index + 1}/{len(cs_)}] {name}: start (C {st}, {d.seams_composed} seams, {d.prompts} prompts)', file=sys.stderr, flush=True)
        if st in ('CALLEE', 'SERVICE', 'LIMIT'):
            non_architectural.append((name, st, f'{d.seam:04x}', f'{d.nested:04x}', d.leaf_status))
        check(st != 'CALLEE', f'{name}: C CALLEE at seam {d.seam:04x} nested {d.nested:04x} leaf status {d.leaf_status}')
        ust, ucpu, uram, ufs, uenv = run_unicorn(name, m, c, spec, stop_at=d.decode_arrivals if st == 'STALLED' else None)
        check(st == ust, f'{name}: C {st} original {ust}')
        A716.compare_cpu(name, cpu, ucpu); A716.compare_ram(name, ram, uram)
        check(cfs.state() == ufs.state(), f'{name}: file state C {cfs.state()} original {ufs.state()}')
        check(cfs.log == ufs.log, f'{name}: file log differs')
        check(cenv.log == uenv.log, f'{name}: environment log differs')
        stops[st] = stops.get(st, 0) + 1; total_seams += d.seams_composed; total_prompts += d.prompts
        print(f'[{index + 1}/{len(cs_)}] {name}: {st}, {time.time() - t0:.1f}s', file=sys.stderr, flush=True)
    return dict(cases=len(cs_), end_states=stops, non_architectural_stops=len(non_architectural), non_architectural=non_architectural, seams_composed=total_seams, prompts=total_prompts,
                dirty_flags=[f'{x:04X}' for x in DIRTY_FLAGS], stall_iterations_confirmed=STALL_EXTRA * stops.get('STALLED', 0),
                cpu_words=14, ram_bytes=SIZE, flag_masks=0)


def main():
    p = argparse.ArgumentParser()
    for flag in ('--compile-only', '--unicorn'):
        p.add_argument(flag, action='store_true')
    p.add_argument('--output', type=Path); a = p.parse_args()
    with tempfile.TemporaryDirectory(prefix='r0716c-') as tmp:
        f = library(Path(tmp))
        if a.compile_only:
            print('CHECKER_BUILD_OK'); return
        image, sites = Q.load_image()
        stub = B.assemble_stub(scratch=SCRATCH)
        report = dict(target=dict(entry='0802:0716', image_sha256=Q.IMAGE_DIGEST, relocations=len(sites), sources=len(SOURCES),
                                  int10_stub_sha256=sha256(stub).hexdigest()))
        if a.unicorn:
            report['unicorn'] = differential(Q.relocate(image, sites, BASE), stub, f)
    if a.output:
        a.output.write_text(json.dumps(report, sort_keys=True, indent=2) + '\n')
    print(json.dumps(report, sort_keys=True))


if __name__ == '__main__':
    main()
