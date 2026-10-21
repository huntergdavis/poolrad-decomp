#!/usr/bin/env python3
"""RES_0802_025D (open with the "insert disk" prompt loop) against the original bytes with every callee REAL: FileExists 02EB,
StrStore, Assign, Reset and, on the prompt path, 0193 with its display stack and ReadKey 0899:0059.

The whole loader-expanded START.EXE image is loaded at a test base with the loader's own relocation table applied. The C model
composes 02EB and the SYSTEM models and stops at the prompt (0193); each stop is compared with the original's state at 0193's
entry and the model resumes from the ORIGINAL's state at the return (the #1311 two-phase seam). Declared environment: the 0716
audit's DOS file service (INT 21h 3Dh/3Eh) with an "insert disk" event (the file appears at the Nth open attempt), plus the
prompt path's INT 16h keys, INT 10h stub, 2Ch/25h/35h and ports. Unicorn: CPU14 + full 1 MiB + file state + the ordered DOS
log, per segment. Native (DOSBox-X): the C model against REAL DOS on the prompt-free paths. Media stay under ignored resources/."""
import argparse, ctypes, json, os, random, struct, subprocess, sys, tempfile, time
from hashlib import sha256
from pathlib import Path
from capstone import Cs, CS_ARCH_X86, CS_MODE_16
from res0709_ega_reset_0000_cpu_audit import Cpu, ROOT, SIZE, REGISTERS, physical, put16, check
import res0899_quit_0000_cpu_audit as Q
import res0802_0716_cpu_audit as A716
CHECKER_ROWS = ('RES_0802_025D',)
ENTRY, END = 0x025D, 0x02EB
DIGEST = 'eeea6613012d3db99c7ad85788fdd3ff5b9888d7cc6701e9ce7a612195210dc6'
BASE, NSEG, STUB_SEG = Q.BASE, 0x5800, Q.STUB_SEG
SOURCES = ['resident_0802_025d_cpu', 'resident_0802_02eb_cpu', 'system_strstore_fullstate_0b25', 'system_ioresult_assign_cpu',
           'system_runtime_leaves_cpu', 'system_reset_1410_cpu', 'system_close_1491_cpu', 'system_filecheck_14b6_cpu']
CONDITIONALS = {0x0299: (0x02AD, 0x029B), 0x029F: (0x02AD, 0x02A1), 0x02B1: (0x02B9, 0x02B3), 0x02B7: (0x0289, 0x02B9),
                0x02BD: (0x02DC, 0x02BF)}
STATUS = {0: 'RETURN', 1: 'ARGUMENT', 2: 'PROMPT', 3: 'SERVICE', 4: 'CALLEE', 5: 'LIMIT'}
WIDE = 65535                                                          # a retry budget no ordinary case reaches
SERVICE_FN = ctypes.CFUNCTYPE(ctypes.c_int, ctypes.c_void_p, ctypes.POINTER(Cpu), ctypes.POINTER(Cpu))


class Env(ctypes.Structure):
    _fields_ = [('open', SERVICE_FN), ('open_context', ctypes.c_void_p), ('close', SERVICE_FN), ('close_context', ctypes.c_void_p)]


class Diag(ctypes.Structure):
    _fields_ = [('site', ctypes.c_uint16), ('callee_status', ctypes.c_int)]


def source(image):
    body = image[0x802 * 16 + ENTRY:0x802 * 16 + END]
    check(sha256(body).hexdigest() == DIGEST, '0802:025D identity')
    ins = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(body, ENTRY))
    check(len(ins) == 63 and sum(i.size for i in ins) == len(body), 'decode covers 025D')
    check([i.address for i in ins if i.mnemonic == 'lcall'] == [0x0271, 0x0284, 0x02C9, 0x02D7], 'four far calls')
    check([i.address for i in ins if i.mnemonic == 'call'] == [0x028F, 0x02AA], 'near calls 02EB and 0193')
    check({i.address for i in ins if i.mnemonic.startswith('j') and i.mnemonic != 'jmp'} == set(CONDITIONALS), 'conditional sites')
    return body, {i.address: i for i in ins}


def library(directory):
    path = directory / 'r025d.so'
    subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Wpedantic', '-Werror', '-O2', '-fPIC', '-shared', '-I', str(ROOT / 'include')]
                   + [str(ROOT / f'src/recovered/{n}.c') for n in SOURCES] + ['-o', str(path)], check=True)
    f = ctypes.CDLL(str(path)).pr_r0802_025d
    f.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t, ctypes.POINTER(Cpu), ctypes.POINTER(Env), ctypes.c_uint,
                  ctypes.POINTER(Diag)]
    f.restype = ctypes.c_int
    return f


def pstr(m, seg, off, text):
    m[physical(seg, off)] = len(text) & 0xFF
    for k, ch in enumerate(text):
        m[physical(seg, (off + 1 + k) & 0xFFFF)] = ch


def build(reloc, *, name=b'ECL1DAX', msg=b'INSERT DISK 2', flag=1, present=True, appear_after=None, keys=(0x1C0D,) * 8,
          typeahead=(),
          ss=0x7000, sp=0xF000, flags=2, regs=None, base=BASE, ret=(0x9001, 0x7000), first_handle=5, retries=WIDE):
    m = bytearray([0x5A]) * SIZE
    m[base * 16:base * 16 + len(reloc)] = reloc
    ds = base + Q.SEG['data']
    end = base * 16 + len(reloc)
    if end < physical(ds, 0xFFFF):
        m[end:physical(ds, 0xFFFF) + 1] = bytes(physical(ds, 0xFFFF) + 1 - end)     # the data segment's BSS, as DOS leaves it
    put16(m, ds, 0x4426, 0); m[physical(ds, 0x442C)] = 2; put16(m, ds, 0x49CE, 0xEEEE)
    s = physical(STUB_SEG, 0); m[s:s + len(Q.INT10_STUB)] = Q.INT10_STUB
    put16(m, 0, 0x40, 0x0000); put16(m, 0, 0x42, STUB_SEG)
    for k in range(0x80):
        m[physical(NSEG, 0x0200 + k)] = 0
    pstr(m, NSEG, 0x0010, name); pstr(m, NSEG, 0x0080, msg)
    ss = ds if ss == 'ds' else ss
    for k, w in enumerate((ret[1], ret[0], 0x0200, NSEG, flag, 0x0080, NSEG, 0x0010, NSEG)):
        put16(m, ss, (sp + 2 * k) & 0xFFFF, w)
    r = regs or (0x1234, 0x3456, 0x5678, 0x789A, 0x9ABC, 0xBCDE, 0x7777)
    c = Cpu(*r, sp, base + 0x0802, ENTRY, ds, 0x1111, ss, flags)
    return m, c, dict(present=present, appear_after=appear_after, keys=list(keys), typeahead=list(typeahead), ret=ret, base=base,
                      first_handle=first_handle, retries=retries)


def file_service(spec):
    return A716.FileSystem(b'DAX' if spec['present'] else None, first_handle=spec['first_handle'], appear_after=spec['appear_after'])


def run_unicorn(name, m, c, spec, decoded):
    from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE, UC_HOOK_INTR, UC_HOOK_INSN
    from unicorn import x86_const as R
    base = spec['base']; cs0802 = base + 0x0802; fs = file_service(spec); keys = list(spec['keys']); ahead = list(spec['typeahead'])
    qenv = Q.Env(table={8: (0x0123, 0xF000)}, in61=0xB0, time_start=4736147, time_step=7)
    uc = Uc(UC_ARCH_X86, UC_MODE_16); backing = (ctypes.c_uint8 * SIZE).from_buffer_copy(m)
    uc.mem_map_ptr(0, SIZE, 7, ctypes.addressof(backing)); uc.mem_map_ptr(SIZE, SIZE, 7, ctypes.addressof(backing))
    for (n, _), reg in zip(Cpu._fields_, REGISTERS):
        uc.reg_write(reg, getattr(c, n))
    s = dict(stop=None, prev=None, pending=False, steps=0, retries=0); seen = set(); edges = set(); snaps = []
    known = {base + v for v in Q.SEG.values()} | {cs0802, STUB_SEG, base + 0x0A40}

    def snap(kind):
        snaps.append(dict(kind=kind, cpu=Cpu(*[uc.reg_read(r) & 0xFFFF for r in REGISTERS]), ram=bytes(backing), fs=fs.state(),
                          log=len(fs.log)))

    def code(em, address, size, user):
        cs = uc.reg_read(R.UC_X86_REG_CS); ip = uc.reg_read(R.UC_X86_REG_IP)
        if s['prev'] is not None and cs == cs0802:
            edges.add((s['prev'], ip))
        s['prev'] = None
        if (cs, ip) == spec['ret']:
            snap('RETURN'); s['stop'] = 'RETURN'; return uc.emu_stop()
        check(cs in known, f'escape {name} {cs:04x}:{ip:04x}')
        if cs == cs0802 and ip == 0x0193 and not s['pending']:
            sp = uc.reg_read(R.UC_X86_REG_SP); ss_ = uc.reg_read(R.UC_X86_REG_SS)
            if struct.unpack('<H', bytes(backing[physical(ss_, sp):physical(ss_, sp) + 2]))[0] == 0x02AD:
                snap('PROMPT'); s['pending'] = True
        if cs == cs0802 and ip == 0x02AD and s['pending']:
            snap('RESUME'); s['pending'] = False; s['retries'] = 0             # the model's budget is fresh per entry
        if cs == cs0802 and ip == 0x02B7 and not s['pending'] and uc.reg_read(R.UC_X86_REG_EFLAGS) & 0x40:
            if s['retries'] >= spec['retries']:                         # the taken loop-back the model refuses: JE not run
                snap('LIMIT'); s['stop'] = 'LIMIT'; return uc.emu_stop()
            s['retries'] += 1
        if cs == cs0802 and ENTRY <= ip < END and not s['pending']:
            check(ip in decoded, f'escape inside 025D {name} {ip:04x}'); seen.add(ip)
            if ip in CONDITIONALS:
                s['prev'] = ip
        s['steps'] += 1
        if s['steps'] > 3000000:
            s['stop'] = 'RUNAWAY'; uc.emu_stop()

    def intr(em, intno, user):
        ax = uc.reg_read(R.UC_X86_REG_AX); ah, al = ax >> 8, ax & 0xFF; fl = uc.reg_read(R.UC_X86_REG_EFLAGS)
        if intno == 0x16:
            # Declared keyboard: AH=01h reports only TYPE-AHEAD (keys already pending); AH=00h waits, taking type-ahead first,
            # else the next key the player presses (one per prompt). Both the original and the seams use this script.
            check(ah in (0, 1), f'ENV refusal {name}: INT 16h AH {ah:#x}')
            if ah == 1:
                uc.reg_write(R.UC_X86_REG_EFLAGS, (fl & ~0x40) if ahead else (fl | 0x40))
                if ahead:
                    uc.reg_write(R.UC_X86_REG_AX, ahead[0])
                return
            src = ahead if ahead else keys
            check(bool(src), f'ENV refusal {name}: INT 16h with the key script empty'); uc.reg_write(R.UC_X86_REG_AX, src.pop(0)); return
        check(intno == 0x21, f'ENV refusal {name}: INT {intno:#x}')
        if ah == 0x2C:
            cx, dx = qenv.gettime(); uc.reg_write(R.UC_X86_REG_CX, cx); uc.reg_write(R.UC_X86_REG_DX, dx); return
        if ah in (0x25, 0x35):
            sp = uc.reg_read(R.UC_X86_REG_SP); ss_ = uc.reg_read(R.UC_X86_REG_SS)
            for k, v in ((2, fl & 0xFFFF), (4, uc.reg_read(R.UC_X86_REG_CS)), (6, uc.reg_read(R.UC_X86_REG_IP))):
                uc.mem_write(physical(ss_, (sp - k) & 0xFFFF), struct.pack('<H', v))
            out = qenv.dos(ah, al, dict(ds=uc.reg_read(R.UC_X86_REG_DS), dx=uc.reg_read(R.UC_X86_REG_DX)))
            for k, v in out.items():
                uc.reg_write(dict(es=R.UC_X86_REG_ES, bx=R.UC_X86_REG_BX)[k], v)
            return
        check(ah in (0x3D, 0x3E), f'ENV refusal {name}: INT 21h AH {ah:#x}')
        ds_, dx_ = uc.reg_read(R.UC_X86_REG_DS), uc.reg_read(R.UC_X86_REG_DX)
        nm = bytes(backing[physical(ds_, dx_):physical(ds_, dx_) + 80]).split(b'\0')[0].decode('latin-1') if ah == 0x3D else None
        cf, ax_out, _ = fs.call(ah, al, uc.reg_read(R.UC_X86_REG_BX), 0, dx_, nm)
        uc.reg_write(R.UC_X86_REG_AX, ax_out); uc.reg_write(R.UC_X86_REG_EFLAGS, (fl | 1) if cf else (fl & ~1))

    def port_in(em, port, size, user):
        check(port == 0x61 and size == 1, f'ENV refusal {name}: IN {port:#x}'); return qenv.in61

    def port_out(em, port, size, value, user):
        check(port in (0x40, 0x43, 0x61, 0xC0) and size == 1, f'ENV refusal {name}: OUT {port:#x}')
    uc.hook_add(UC_HOOK_CODE, code); uc.hook_add(UC_HOOK_INTR, intr)
    uc.hook_add(UC_HOOK_INSN, port_in, None, 1, 0, R.UC_X86_INS_IN)
    uc.hook_add(UC_HOOK_INSN, port_out, None, 1, 0, R.UC_X86_INS_OUT)
    uc.emu_start(physical(c.cs, c.ip), 2 * SIZE, count=4000000)
    check(s['stop'] in ('RETURN', 'LIMIT'), f'original stop {name}: {s["stop"]}')
    return snaps, fs, seen, edges


def run_c(f, cpu, ram_bytes, spec, fs_state):
    fs = file_service(spec); fs.restore(fs_state); sps = []

    def service(ctx, before, after):
        b = before[0]; sps.append((b.ss, b.sp))
        nm = None
        if b.ax >> 8 == 0x3D:
            r = ctypes.cast(ctx, ctypes.POINTER(ctypes.c_uint8)); at = physical(b.ds, b.dx)
            nm = bytes(r[at:at + 80]).split(b'\0')[0].decode('latin-1')
        cf, ax, _ = fs.call(b.ax >> 8, b.ax & 0xFF, b.bx, 0, b.dx, nm)
        a = Cpu.from_buffer_copy(b); a.ax = ax; a.flags = (b.flags | 1) if cf else (b.flags & ~1); a.ip = (b.ip + 2) & 0xFFFF
        after[0] = a; return 0
    keep = SERVICE_FN(service)
    ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(ram_bytes); c = Cpu.from_buffer_copy(cpu); d = Diag()
    ctx = ctypes.cast(ram, ctypes.c_void_p)
    env = Env(keep, ctx, keep, ctx)
    status = STATUS[f(ram, SIZE, ctypes.byref(c), ctypes.byref(env), spec['retries'], ctypes.byref(d))]
    if status == 'LIMIT':
        check(d.site == 0x02B7 and d.callee_status == -1, f'LIMIT diagnostic {d.site:04x}/{d.callee_status}')
    return status, c, bytes(ram), fs, sps


def two_phase(name, m, c, spec, snaps, ofs, f):
    cpu, ram, fs_state, log_from, k, seams = c, bytes(m), file_service(spec).state(), 0, 0, []
    while True:
        status, actual, ram_c, fs_c, _ = run_c(f, cpu, ram, spec, fs_state)
        want = snaps[k]
        check(status == want['kind'], f'{name} segment {len(seams)}: C {status} original {want["kind"]}')
        A716.compare_cpu(f'{name}#{len(seams)}', actual, want['cpu'])
        A716.compare_ram(f'{name}#{len(seams)}', ram_c, want['ram'])
        check(fs_c.state() == want['fs'], f'{name}#{len(seams)} file state')
        check(fs_c.log == ofs.log[log_from:want['log']], f'{name}#{len(seams)} DOS log C {fs_c.log} original {ofs.log[log_from:want["log"]]}')
        seams.append(status)
        if status in ('RETURN', 'LIMIT'):
            return seams, ram_c, actual
        nxt = snaps[k + 1]
        check(nxt['kind'] == 'RESUME', f'{name}: no resume after {status}')
        cpu, ram, fs_state, log_from, k = nxt['cpu'], nxt['ram'], nxt['fs'], nxt['log'], k + 2


def expected(o, spec):
    """Independent statement: (AL, prompts). Flag 1: one FileExists, no prompt. Flag 0: prompt and retry until the file exists.
    FileExists opens and closes once per check; when it exists, Reset opens it again (the FileRec handle is the lowest free)."""
    present, after, flag = o.get('present', True), o.get('appear_after'), o.get('flag', 1) & 0xFF   # read as a byte
    if not o.get('name', b'ECL1DAX'):
        return 0, 0
    exists_at = None if not present else (1 if after is None else after)
    if flag:
        return (1 if exists_at == 1 else 0), 0
    return 1, exists_at - 1


def cases():
    rng = random.Random(0x0802025D)
    out = [('present-flag1', dict()), ('present-flag0', dict(flag=0)), ('absent-flag1', dict(present=False)),
           ('appears-after-2-flag1', dict(appear_after=2)),
           # the [557C] = 0 "insert disk" loop: the prompt runs (real 0193 + ReadKey), then the retry finds the file
           ('insert-disk-1-prompt', dict(flag=0, appear_after=2)), ('insert-disk-2-prompts', dict(flag=0, appear_after=3)),
           ('insert-disk-flag-ff', dict(flag=0xFF00, appear_after=2)), ('flag-high-byte-only', dict(flag=0x0100, appear_after=2)),
           ('empty-name-flag1', dict(name=b'')), ('long-name', dict(name=b'N' * 60)), ('long-message', dict(flag=0, appear_after=2, msg=b'M' * 60)),
           ('empty-message', dict(flag=0, appear_after=2, msg=b'')),
           ('typeahead-drained', dict(flag=0, appear_after=3, typeahead=(0x1E61, 0x3062, 0x2E63))),
           # the declared retry budget: the resume's taken 02B7 loop-back refused, against the original stopped at that arrival
           ('retry-budget-0', dict(flag=0, appear_after=3, retries=0)), ('retry-budget-1', dict(flag=0, appear_after=3, retries=1))]
    out += [(f'flags{fl:x}', dict(flags=fl)) for fl in Q.FLAGS] + [(f'flags{fl:x}-prompt', dict(flags=fl, flag=0, appear_after=2)) for fl in Q.FLAGS]
    out += [(f'sp{sp:x}', dict(sp=sp)) for sp in (0x0200, 0x0400, 0x8001, 0xFFED, 0xFFEE)]
    out += [('ss-ds', dict(ss='ds', sp=0xE000))]
    for k in range(8):
        flag = rng.choice((0, 1))
        out.append((f'random{k}', dict(flag=flag, present=True, appear_after=rng.choice((None, 2, 3)) if flag == 0 else None,
                                       flags=rng.choice(Q.FLAGS), regs=tuple(rng.randrange(65536) for _ in range(7)))))
    return out


def differential(reloc, decoded, f):
    seen, edges, checked, segments, prompts_total = set(), set(), 0, 0, 0
    cs_, limits = cases(), {}
    for index, (name, o) in enumerate(cs_):
        t0 = time.time(); print(f'[{index + 1}/{len(cs_)}] {name}: start', file=sys.stderr, flush=True)
        m, c, spec = build(reloc, **o)
        snaps, ofs, v, e = run_unicorn(name, m, c, spec, decoded); seen |= v; edges |= e
        seams, ram, cpu = two_phase(name, m, c, spec, snaps, ofs, f)
        segments += len(seams); prompts = seams.count('PROMPT'); prompts_total += prompts
        print(f'[{index + 1}/{len(cs_)}] {name}: {"/".join(seams)}, {time.time() - t0:.1f}s', file=sys.stderr, flush=True)
        if seams[-1] == 'LIMIT':
            limits[name] = '/'.join(seams)
        elif not name.startswith(('random', 'sp', 'ss-ds', 'flag-high')):
            want_al, want_prompts = expected(o, spec)
            check((cpu.ax & 0xFF, prompts) == (want_al, want_prompts), f'{name}: (AL, prompts) {(cpu.ax & 0xFF, prompts)} vs {(want_al, want_prompts)}')
            magic = ram[physical(NSEG, 0x202)] | ram[physical(NSEG, 0x203)] << 8
            check(magic == (0xD7B3 if want_al else 0x0000), f'{name}: FileRec magic {magic:04x}')
            checked += 1
    want = {(a, t) for a, ts in CONDITIONALS.items() for t in ts}
    check(seen == set(decoded), f'starts missing {sorted(hex(a) for a in set(decoded) - seen)}')
    check(edges == want, f'outcomes missing {sorted(want - edges)}')
    return dict(cases=len(cases()), limits=limits, segments=segments, prompts=prompts_total, starts=len(seen), starts_total=len(decoded),
                conditional_outcomes=len(edges), conditional_outcomes_total=len(want), independent_checks=checked,
                cpu_words=14, ram_bytes=SIZE, flag_masks=0)


NBASE, NSS, TOP, LOW, WINDOW = 0x3000, 0x6000, 0xF000, 0xE800, 0x0800
DOS_STACK = 24


def native(image, sites, f):
    """The C model against REAL DOS on real files, on the paths with no prompt (the prompt path needs the keyboard and video:
    Unicorn only, declared). Each case calls 025D directly; the archive is PRESENT.DAX, an absent name is ABSENT.DAX."""
    directory = ROOT / 'resources/0802-025d/native'; directory.mkdir(parents=True, exist_ok=True)
    reloc = Q.relocate(image, sites, NBASE)
    (directory / 'IMAGE.BIN').write_bytes(reloc)
    (directory / 'PRESENT.DAX').write_bytes(b'DAX'); (directory / 'ABSENT.DAX').unlink(missing_ok=True)
    ds, seg0802 = NBASE + Q.SEG['data'], NBASE + 0x0802
    specs = []
    for fl in Q.FLAGS:
        specs += [(f'present-flag1-{fl:x}', dict(flags=fl, flag=1, present=True)), (f'absent-flag1-{fl:x}', dict(flags=fl, flag=1, present=False)),
                  (f'present-flag0-{fl:x}', dict(flags=fl, flag=0, present=True))]
    specs += [('empty-name-flag1', dict(flag=1, present=True, name=b''))]
    rows = []
    for _, o in specs:
        nm = b'' if o.get('name', b'x') == b'' else (b'PRESENT.DAX' if o['present'] else b'ABSENT.DAX')
        o['name'] = nm
        padded = nm.ljust(12, b'\0')
        rows.append([o.get('flags', 2), o['flag'], len(nm)] + [padded[2 * k] | padded[2 * k + 1] << 8 for k in range(6)])
    load = []
    for k, start in enumerate(range(0, len(reloc), 0x8000)):
        load += [f'mov ax,{NBASE + 0x800 * k}', 'mov ds,ax', 'mov bx,word ptr cs:[image]', 'xor dx,dx',
                 f'mov cx,{min(0x8000, len(reloc) - start)}', 'mov ah,0x3f', 'int 0x21', 'jc failure']
    image_end = NBASE * 16 + len(reloc); bss_end = physical(ds, 0xFFFF) + 1
    zero, at = [], image_end
    while at < bss_end:
        n_ = min(0x8000, bss_end - at)
        zero += [f'mov ax,{at >> 4}', 'mov es,ax', f'mov di,{at & 0xF}', f'mov cx,{n_}', 'xor al,al', 'rep stosb']
        at += n_
    msg = b'INSERT DISK 2'
    regions = [(NSS, LOW, WINDOW), (NSEG, 0x0000, 0x0300), (ds, 0x4420, 0x10), (ds, 0x49CE, 0x02)]
    writes = []
    for seg, off, n in regions:
        writes += [f'mov ax,{seg}', 'mov ds,ax', 'mov bx,word ptr cs:[handle]', f'mov dx,{off}', f'mov cx,{n}', 'mov ah,0x40', 'int 0x21']
    t = ['.code16', '.intel_syntax noprefix', '.global _start', '_start: jmp main', '.org 0x200', 'main:',
         'mov ax,cs', 'mov ds,ax', 'mov ss,ax', 'mov sp,0xfff0', 'cld',
         'mov dx,offset outname', 'xor cx,cx', 'mov ah,0x3c', 'int 0x21', 'jc failure', 'mov word ptr [handle],ax',
         'mov word ptr [header],cs', 'mov word ptr [header+2],offset returned', f'mov word ptr [header+4],{len(rows)}',
         'mov bx,word ptr [handle]', 'mov dx,offset header', 'mov cx,6', 'mov ah,0x40', 'int 0x21',
         'again:', 'cld', 'push cs', 'pop ds', 'mov dx,offset imagename', 'mov ax,0x3d00', 'int 0x21', 'jc failure',
         'mov word ptr cs:[image],ax'] + load + ['push cs', 'pop ds', 'mov bx,word ptr [image]', 'mov ah,0x3e', 'int 0x21', 'cld'] + zero + [
         'push cs', 'pop ds',
         'mov si,word ptr [index]', 'mov ax,si', 'shl si,1', 'shl si,1', 'shl si,1', 'add si,ax', 'shl si,1', 'add si,offset table',  # 18 bytes/row
         f'mov ax,{ds}', 'mov es,ax', 'mov word ptr es:[0x4426],0', 'mov byte ptr es:[0x442c],2', 'mov word ptr es:[0x49ce],0xeeee',
         f'mov ax,{NSEG}', 'mov es,ax', 'xor di,di', 'mov cx,0x300', 'mov al,0x5a', 'rep stosb',
         'mov di,0x200', 'mov cx,0x80', 'xor al,al', 'rep stosb',
         'mov al,byte ptr cs:[si+4]', 'mov byte ptr es:[0x10],al', 'push si', 'mov cl,al', 'xor ch,ch', 'add si,6', 'mov di,0x11',
         'rep movsb', 'pop si', f'mov byte ptr es:[0x80],{len(msg)}'] + [f'mov byte ptr es:[{0x81 + k}],{b}' for k, b in enumerate(msg)] + [
         f'mov ax,{NSS}', 'mov es,ax', f'mov di,{LOW}', f'mov cx,{WINDOW // 2}', 'mov ax,0x5a5a', 'rep stosw',
         f'mov ax,{NSS}', 'mov ss,ax', f'mov sp,{TOP}', f'push {NSEG}', 'push 0x10', f'push {NSEG}', 'push 0x80', 'push word ptr cs:[si+2]',
         f'push {NSEG}', 'push 0x200', 'push word ptr cs:[si]', f'mov ax,{ds}', 'mov ds,ax', 'mov ax,0x1111', 'mov es,ax',
         'mov ax,0x1234', 'mov bx,0x3456', 'mov cx,0x5678', 'mov dx,0x789a', 'mov si,0x9abc', 'mov di,0xbcde', 'mov bp,0x7777',
         'popf', f'lcall {seg0802},0x025d', 'returned:']
    t += [f'mov word ptr cs:[out+{2 * n}],{r}' for n, r in enumerate('ax bx cx dx si di bp sp cs'.split())]
    t += ['mov word ptr cs:[out+18],offset returned', 'mov word ptr cs:[out+20],ds', 'mov word ptr cs:[out+22],es',
          'mov word ptr cs:[out+24],ss', 'mov ax,cs', 'mov ss,ax', 'mov sp,0xfff0', 'pushf', 'pop ax', 'mov word ptr cs:[out+26],ax',
          'cld', 'push cs', 'pop ds', 'mov bx,word ptr [handle]', 'mov dx,offset out', 'mov cx,28', 'mov ah,0x40', 'int 0x21'] + writes + [
          'mov bx,6', 'closeall:', 'mov ah,0x3e', 'push bx', 'int 0x21', 'pop bx', 'inc bx', 'cmp bx,20', 'jb closeall',
          'push cs', 'pop ds', 'inc word ptr [index]', f'cmp word ptr [index],{len(rows)}', 'jae done', 'jmp again', 'done:',
          'mov bx,word ptr [handle]', 'mov ah,0x3e', 'int 0x21', 'mov ax,0x4c00', 'int 0x21', 'failure:', 'mov ax,0x4c01', 'int 0x21',
          'outname: .asciz "RESULT.BIN"', 'imagename: .asciz "IMAGE.BIN"', 'handle: .word 0', 'image: .word 0', 'index: .word 0',
          '.align 2', 'header: .space 6', 'out: .space 28', 'table:']
    t += ['.word ' + ','.join(str(v) for v in row) for row in rows]
    (directory / 'oracle.S').write_text('\n'.join(t) + '\n')
    subprocess.run(['as', '--32', str(directory / 'oracle.S'), '-o', str(directory / 'oracle.o')], check=True)
    com = directory / 'ORACLE.COM'
    subprocess.run(['ld', '-m', 'elf_i386', '-Ttext', '0x100', '--oformat', 'binary', str(directory / 'oracle.o'), '-o', str(com)], check=True)
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
                       check=True, timeout=600)
    data = out.read_bytes(); rec_len = 28 + sum(n for _, _, n in regions)
    check(len(data) == 6 + len(rows) * rec_len, f'native length {len(data)}')
    comseg, ret_off, count = struct.unpack_from('<3H', data); check(count == len(rows), 'native count')
    for n, (name, o) in enumerate(specs):
        rec = data[6 + n * rec_len:6 + (n + 1) * rec_len]
        m, c, spec = build(reloc, base=NBASE, ss=NSS, sp=TOP - 0x12, ret=(comseg, ret_off), first_handle=6, **o)
        m = bytearray(m)
        m[physical(NSEG, 0):physical(NSEG, 0x300)] = b'\x5a' * 0x300
        m[physical(NSEG, 0x200):physical(NSEG, 0x280)] = bytes(0x80)
        pstr(m, NSEG, 0x0010, o['name']); pstr(m, NSEG, 0x0080, msg)
        m[physical(NSS, LOW):physical(NSS, LOW) + WINDOW] = b'\x5a' * WINDOW
        for k, w in enumerate((ret_off, comseg, 0x0200, NSEG, o['flag'], 0x0080, NSEG, 0x0010, NSEG)):
            put16(m, NSS, TOP - 0x12 + 2 * k, w)
        status, cpu, ram, _, sps = run_c(f, c, bytes(m), spec, file_service(spec).state())
        check(status == 'RETURN', f'native C status {name}: {status}')
        for (reg, _), want in zip(Cpu._fields_, struct.unpack_from('<14H', rec)):
            check(getattr(cpu, reg) == want, f'native CPU {name}/{reg}: C {getattr(cpu, reg):04x} DOS {want:04x}')
        mask = {physical(s_, (sp - k) & 0xFFFF) for s_, sp in sps for k in range(1, DOS_STACK + 1)}
        at = 28
        for seg, off, length in regions:
            for k in range(length):
                p = physical(seg, (off + k) & 0xFFFF)
                if p not in mask:
                    check(ram[p] == rec[at + k], f'native {name} at {seg:04x}:{(off + k) & 0xFFFF:04x}: C {ram[p]:02x} DOS {rec[at + k]:02x}')
            at += length
    probe = subprocess.run([binary, '--version'], capture_output=True, text=True, env=dict(os.environ, SDL_VIDEODRIVER='dummy'))
    version = next(line.split(',')[0].strip() for line in (probe.stdout + probe.stderr).splitlines() if 'DOSBox-X version' in line)
    return dict(cases=len(rows), cpu_words=14, dos_stack_mask_bytes_below_int_sp=DOS_STACK,
                compared_regions=[f'{s_:04X}:{o_:04X}+{n_:X}' for s_, o_, n_ in regions], result_sha256=sha256(data).hexdigest(),
                dosbox_x=dict(path=binary, sha256=sha256(open(binary, 'rb').read()).hexdigest(), version=version,
                              pinned_source='5a9c7c53481c2e933948039c33256776653f4faf', pinned_build_used=False))


def main():
    p = argparse.ArgumentParser()
    for flag in ('--compile-only', '--unicorn', '--native'):
        p.add_argument(flag, action='store_true')
    p.add_argument('--output', type=Path); a = p.parse_args()
    with tempfile.TemporaryDirectory(prefix='r025d-') as tmp:
        f = library(Path(tmp))
        if a.compile_only:
            print('CHECKER_BUILD_OK'); return
        image, sites = Q.load_image()
        body, decoded = source(image)
        report = dict(target=dict(span='0802:[025D,02EB)', bytes=len(body), instructions=len(decoded), sha256=DIGEST,
                                  image_sha256=Q.IMAGE_DIGEST, relocations=len(sites)))
        if a.unicorn:
            report['unicorn'] = differential(Q.relocate(image, sites, BASE), decoded, f)
        if a.native:
            report['native'] = native(image, sites, f)
    if a.output:
        a.output.write_text(json.dumps(report, sort_keys=True, indent=2) + '\n')
    print(json.dumps(report, sort_keys=True))


if __name__ == '__main__':
    main()
