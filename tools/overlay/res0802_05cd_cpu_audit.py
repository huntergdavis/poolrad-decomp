#!/usr/bin/env python3
"""RES_0802_05CD (open the archive pair through 025D and position it) against the original bytes with every callee REAL: 025D
with 02EB under it, StrStore, string load/concat, char-to-string, Assign, Reset, IOCheck, BlockRead, Seek and, on the prompt
path, 0193 with its display stack and ReadKey 0899:0059.

The whole loader-expanded START.EXE image is loaded at a test base with the loader's own relocation table applied. The C model
composes 025D and the SYSTEM models and stops at the prompt (0193, inside 025D); each stop is compared with the original's state
at 0193's entry and the model resumes from the ORIGINAL's state at the return (the #1311 two-phase seam). Declared environment:
the 0716 audit's DOS file service (INT 21h 3Dh/3Eh/3Fh/42h) with the insert-disk event and three declared faults (the disk gone
from the Nth open, the Nth read failing, an LSEEK error), plus the prompt path's INT 16h keys, INT 10h stub, 2Ch/25h/35h and
ports. Unicorn: CPU14 + full 1 MiB + file state + the ordered DOS log, per segment. Native (DOSBox-X): the C model against REAL
DOS on real files, on the prompt-free paths. Media stay under ignored resources/."""
import argparse, ctypes, json, os, random, struct, subprocess, sys, tempfile
from hashlib import sha256
from pathlib import Path
from capstone import Cs, CS_ARCH_X86, CS_MODE_16
from res0709_ega_reset_0000_cpu_audit import Cpu, ROOT, SIZE, REGISTERS, physical, put16, check
import res0899_quit_0000_cpu_audit as Q
import res0802_0716_cpu_audit as A716
CHECKER_ROWS = ('RES_0802_05CD',)
ENTRY, END = 0x05CD, 0x06E7
DIGEST = 'd8067fb72bfede3fb56aa146e941d7bc9524cb23715f8f63e60f4918c65449b5'
BASE, NSEG, STUB_SEG = Q.BASE, 0x5800, Q.STUB_SEG
SOURCES = ['resident_0802_05cd_cpu', 'resident_0802_025d_cpu', 'resident_0802_02eb_cpu', 'system_strstore_fullstate_0b25',
           'system_ioresult_assign_cpu', 'system_runtime_leaves_cpu', 'system_reset_1410_cpu', 'system_close_1491_cpu',
           'system_filecheck_14b6_cpu', 'system_string_leaves_cpu', 'system_string_move_leaves_cpu', 'system_iocheck_04a9_cpu',
           'system_blockread_14fb_cpu', 'system_seek_1563_cpu']
FAR_SITES = [0x05E2, 0x05F5, 0x0605, 0x060F, 0x062C, 0x063B, 0x0640, 0x064A, 0x0670, 0x067A, 0x067F, 0x068D, 0x0692, 0x06AA,
             0x06AF, 0x06CA, 0x06CF]
CONDITIONALS = {0x065E: (0x06DA, 0x0660)}
STATUS = {0: 'RETURN', 1: 'ARGUMENT', 2: 'PROMPT', 3: 'RUNERROR', 4: 'DIVIDE', 5: 'SERVICE', 6: 'CALLEE', 7: 'LIMIT'}
TERMINAL = ('RETURN', 'RUNERROR')
SERVICE_FN = ctypes.CFUNCTYPE(ctypes.c_int, ctypes.c_void_p, ctypes.POINTER(Cpu), ctypes.POINTER(Cpu))
READ_FN, SEEK_FN = A716.READ_FN, A716.SEEK_FN
# Frame layout in NSEG: base and extension strings, the two FileRecs and the header word.
BASE_AT, EXT_AT, F_AT, G_AT, HDR_AT = 0x0010, 0x0060, 0x0200, 0x0300, 0x0400
BLOB = bytes([0x34, 0x12]) + bytes(range(64))


class Env(ctypes.Structure):
    _fields_ = [('open', SERVICE_FN), ('open_context', ctypes.c_void_p), ('close', SERVICE_FN), ('close_context', ctypes.c_void_p),
                ('read', READ_FN), ('read_context', ctypes.c_void_p), ('seek', SEEK_FN), ('seek_context', ctypes.c_void_p),
                ('retries', ctypes.c_uint)]                                  # 025D's retry budget; 65535 is never reached here


class Diag(ctypes.Structure):
    _fields_ = [('site', ctypes.c_uint16), ('callee_status', ctypes.c_int)]


def source(image):
    body = image[0x802 * 16 + ENTRY:0x802 * 16 + END]
    check(sha256(body).hexdigest() == DIGEST, '0802:05CD identity')
    ins = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(body, ENTRY))
    check(len(ins) == 118 and sum(i.size for i in ins) == len(body), 'decode covers 05CD')
    check([i.address for i in ins if i.mnemonic == 'lcall'] == FAR_SITES, 'the 17 far-call sites')
    check({i.address + i.size: int(i.op_str, 16) for i in ins if i.mnemonic == 'call'} == {0x065C: 0x025D}, 'the one near call, 025D')
    check({i.address for i in ins if i.mnemonic.startswith('j') and i.mnemonic != 'jmp'} == set(CONDITIONALS), 'conditional sites')
    return body, {i.address: i for i in ins}


def library(directory):
    path = directory / 'r05cd.so'
    subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Wpedantic', '-Werror', '-O2', '-fPIC', '-shared', '-I', str(ROOT / 'include')]
                   + [str(ROOT / f'src/recovered/{n}.c') for n in SOURCES] + ['-o', str(path)], check=True)
    f = ctypes.CDLL(str(path)).pr_r0802_05cd
    f.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t, ctypes.POINTER(Cpu), ctypes.POINTER(Env), ctypes.POINTER(Diag)]
    f.restype = ctypes.c_int
    return f


def pstr(m, seg, off, text):
    m[physical(seg, off)] = len(text) & 0xFF
    for k, ch in enumerate(text):
        m[physical(seg, (off + 1 + k) & 0xFFFF)] = ch


def build(reloc, *, base_name=b'ECL1', ext=b'.DAX', flag=1, present=True, appear_after=None, gone_from=None, read_fail=None,
          seek_fail=False, blob=BLOB, header=0xA5A5, table_index=0, table=b'INSERT DISK ', disk=b'E', keys=(0x1C0D,) * 8,
          typeahead=(), ss=0x7000, sp=0xF000, flags=2, regs=None, base=BASE, ret=(0x9001, 0x7000), first_handle=5):
    m = bytearray([0x5A]) * SIZE
    m[base * 16:base * 16 + len(reloc)] = reloc
    ds = base + Q.SEG['data']
    end = base * 16 + len(reloc)
    if end < physical(ds, 0xFFFF):
        m[end:physical(ds, 0xFFFF) + 1] = bytes(physical(ds, 0xFFFF) + 1 - end)     # the data segment's BSS, as DOS leaves it
    put16(m, ds, 0x4426, 0); m[physical(ds, 0x442C)] = 2; put16(m, ds, 0x49CE, 0xEEEE)
    m[physical(ds, 0x5376)] = table_index & 0xFF; pstr(m, ds, 0x2614 + 0x1E * (table_index & 0xFF), table)
    m[physical(ds, 0x547D)] = disk[0]; m[physical(ds, 0x557C)] = flag & 0xFF
    s = physical(STUB_SEG, 0); m[s:s + len(Q.INT10_STUB)] = Q.INT10_STUB
    put16(m, 0, 0x40, 0x0000); put16(m, 0, 0x42, STUB_SEG)
    for at in (F_AT, G_AT):
        m[physical(NSEG, at):physical(NSEG, at + 0x80)] = bytes(0x80)
    pstr(m, NSEG, BASE_AT, base_name); pstr(m, NSEG, EXT_AT, ext); put16(m, NSEG, HDR_AT, header)
    ss = ds if ss == 'ds' else ss
    for k, w in enumerate((ret[1], ret[0], G_AT, NSEG, F_AT, NSEG, HDR_AT, NSEG, EXT_AT, NSEG, BASE_AT, NSEG)):
        put16(m, ss, (sp + 2 * k) & 0xFFFF, w)
    r = regs or (0x1234, 0x3456, 0x5678, 0x789A, 0x9ABC, 0xBCDE, 0x7777)
    c = Cpu(*r, sp, base + 0x0802, ENTRY, ds, 0x1111, ss, flags)
    return m, c, dict(present=present, appear_after=appear_after, gone_from=gone_from, read_fail=read_fail, seek_fail=seek_fail,
                      blob=blob, keys=list(keys), typeahead=list(typeahead), ret=ret, base=base, first_handle=first_handle)


def file_service(spec):
    return A716.FileSystem(spec['blob'] if spec['present'] else None, read_fail=spec['read_fail'], first_handle=spec['first_handle'],
                           appear_after=spec['appear_after'], gone_from=spec['gone_from'], seek_fail=spec['seek_fail'])


def run_unicorn(name, m, c, spec, decoded):
    from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE, UC_HOOK_INTR, UC_HOOK_INSN
    from unicorn import x86_const as R
    base = spec['base']; cs0802 = base + 0x0802; fs = file_service(spec); keys = list(spec['keys']); ahead = list(spec['typeahead'])
    qenv = Q.Env(table={8: (0x0123, 0xF000)}, in61=0xB0, time_start=4736147, time_step=7)
    uc = Uc(UC_ARCH_X86, UC_MODE_16); backing = (ctypes.c_uint8 * SIZE).from_buffer_copy(m)
    uc.mem_map_ptr(0, SIZE, 7, ctypes.addressof(backing)); uc.mem_map_ptr(SIZE, SIZE, 7, ctypes.addressof(backing))
    for (n, _), reg in zip(Cpu._fields_, REGISTERS):
        uc.reg_write(reg, getattr(c, n))
    s = dict(stop=None, prev=None, pending=False, steps=0); seen = set(); edges = set(); snaps = []
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
        if (cs, ip) == (base + Q.SEG['system'], 0x00E2):
            snap('RUNERROR'); s['stop'] = 'RUNERROR'; return uc.emu_stop()
        check(cs in known, f'escape {name} {cs:04x}:{ip:04x}')
        if cs == cs0802 and ip == 0x0193 and not s['pending']:
            sp = uc.reg_read(R.UC_X86_REG_SP); ss_ = uc.reg_read(R.UC_X86_REG_SS)
            if struct.unpack('<H', bytes(backing[physical(ss_, sp):physical(ss_, sp) + 2]))[0] == 0x02AD:
                snap('PROMPT'); s['pending'] = True
        if cs == cs0802 and ip == 0x02AD and s['pending']:
            snap('RESUME'); s['pending'] = False
        if cs == cs0802 and ENTRY <= ip < END and not s['pending']:
            check(ip in decoded, f'escape inside 05CD {name} {ip:04x}'); seen.add(ip)
            if ip in CONDITIONALS:
                s['prev'] = ip
        s['steps'] += 1
        if s['steps'] > 3000000:
            s['stop'] = 'RUNAWAY'; uc.emu_stop()

    def intr(em, intno, user):
        ax = uc.reg_read(R.UC_X86_REG_AX); ah, al = ax >> 8, ax & 0xFF; fl = uc.reg_read(R.UC_X86_REG_EFLAGS)
        if intno == 0x16:
            # Declared keyboard (the #1326 contract): AH=01h reports only type-ahead; AH=00h takes type-ahead first, else the next
            # key the player presses (one per prompt). Both the original and the seams use this script.
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
        check(ah in (0x3D, 0x3E, 0x3F, 0x42), f'ENV refusal {name}: INT 21h AH {ah:#x}')
        ds_, dx_ = uc.reg_read(R.UC_X86_REG_DS), uc.reg_read(R.UC_X86_REG_DX)
        nm = bytes(backing[physical(ds_, dx_):physical(ds_, dx_) + 80]).split(b'\0')[0].decode('latin-1') if ah == 0x3D else None
        at = physical(ds_, dx_)

        def write(chunk):
            for k, v in enumerate(chunk):
                backing[(at + k) % SIZE] = v
        cf, ax_out, dx_out = fs.call(ah, al, uc.reg_read(R.UC_X86_REG_BX), uc.reg_read(R.UC_X86_REG_CX), dx_, nm, write)
        uc.reg_write(R.UC_X86_REG_AX, ax_out)
        if dx_out is not None:
            uc.reg_write(R.UC_X86_REG_DX, dx_out)
        uc.reg_write(R.UC_X86_REG_EFLAGS, (fl | 1) if cf else (fl & ~1))

    def port_in(em, port, size, user):
        check(port == 0x61 and size == 1, f'ENV refusal {name}: IN {port:#x}'); return qenv.in61

    def port_out(em, port, size, value, user):
        check(port in (0x40, 0x43, 0x61, 0xC0) and size == 1, f'ENV refusal {name}: OUT {port:#x}')
    uc.hook_add(UC_HOOK_CODE, code); uc.hook_add(UC_HOOK_INTR, intr)
    uc.hook_add(UC_HOOK_INSN, port_in, None, 1, 0, R.UC_X86_INS_IN)
    uc.hook_add(UC_HOOK_INSN, port_out, None, 1, 0, R.UC_X86_INS_OUT)
    uc.emu_start(physical(c.cs, c.ip), 2 * SIZE, count=4000000)
    check(s['stop'] in TERMINAL, f'original stop {name}: {s["stop"]}')
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

    def read(ctx, before, after, buffer, length):
        b = before[0]; sps.append((b.ss, b.sp))

        def write(chunk):
            for k, v in enumerate(chunk[:length]):
                buffer[k] = v
        cf, ax, _ = fs.call(b.ax >> 8, b.ax & 0xFF, b.bx, b.cx, b.dx, None, write)
        a = Cpu.from_buffer_copy(b); a.ax = ax; a.flags = (b.flags | 1) if cf else (b.flags & ~1); a.ip = (b.ip + 2) & 0xFFFF
        after[0] = a; return 0

    def lseek(ctx, before, after):
        b = before[0]; sps.append((b.ss, b.sp))
        cf, ax, dx = fs.call(b.ax >> 8, b.ax & 0xFF, b.bx, b.cx, b.dx)
        a = Cpu.from_buffer_copy(b); a.ax = ax
        if dx is not None:
            a.dx = dx
        a.flags = (b.flags | 1) if cf else (b.flags & ~1); a.ip = (b.ip + 2) & 0xFFFF
        after[0] = a; return 0
    keep = (SERVICE_FN(service), READ_FN(read), SEEK_FN(lseek))
    ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(ram_bytes); c = Cpu.from_buffer_copy(cpu); d = Diag()
    ctx = ctypes.cast(ram, ctypes.c_void_p)
    env = Env(keep[0], ctx, keep[0], ctx, keep[1], None, keep[2], None, 65535)
    status = STATUS[f(ram, SIZE, ctypes.byref(c), ctypes.byref(env), ctypes.byref(d))]
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
        if status in TERMINAL:
            return seams, ram_c, actual, fs_c
        nxt = snaps[k + 1]
        check(nxt['kind'] == 'RESUME', f'{name}: no resume after {status}')
        cpu, ram, fs_state, log_from, k = nxt['cpu'], nxt['ram'], nxt['fs'], nxt['log'], k + 2


def expected(o):
    """Independent statement from the decode: (final stop, AL, prompts, header word, f's file position).
    025D: FileExists once per attempt (open + close); flag byte 0 prompts and retries until the file exists; when it exists,
    Reset(f) opens it. Then this body: Reset(g) (the next open), IOCheck; BlockRead 2 bytes of the file's start into the header
    word, IOCheck; header word += 2 (16-bit wrap); Seek(f, header word), IOCheck; AL := 1."""
    present, after, flag = o.get('present', True), o.get('appear_after'), o.get('flag', 1) & 0xFF
    blob, header = o.get('blob', BLOB), o.get('header', 0xA5A5)
    exists_at = None if not present else (1 if after is None else after)
    if flag and exists_at != 1:
        return 'RETURN', 0, 0, header, None
    prompts = exists_at - 1
    g_open = exists_at + 2                                      # FileExists opens exists_at times, Reset(f) once more
    if o.get('gone_from') is not None and o['gone_from'] <= g_open:
        return 'RUNERROR', None, prompts, header, None
    if o.get('read_fail') == 1:
        return 'RUNERROR', None, prompts, header, None
    got = blob[:2]
    header = (header & ~((1 << (8 * len(got))) - 1)) | int.from_bytes(got, 'little') if got else header
    header = (header + 2) & 0xFFFF
    if o.get('seek_fail'):
        return 'RUNERROR', None, prompts, header, None
    return 'RETURN', 1, prompts, header, header


def cases():
    rng = random.Random(0x080205CD)
    out = [('present-flag1', dict()), ('present-flag0', dict(flag=0)), ('absent-flag1', dict(present=False)),
           ('appears-after-2-flag1', dict(appear_after=2)),
           # the [557C] = 0 "insert disk" loop through 025D, now under its real caller
           ('insert-disk-1-prompt', dict(flag=0, appear_after=2)), ('insert-disk-2-prompts', dict(flag=0, appear_after=3)),
           ('flag-high-byte-ignored', dict(flag=0x0100, appear_after=2)),
           ('typeahead-drained', dict(flag=0, appear_after=3, typeahead=(0x1E61, 0x3062))),
           # the header word: the low-word wrap, a short file, an empty file
           ('header-1234', dict()), ('header-fffe-wraps', dict(blob=bytes([0xFE, 0xFF]) + bytes(8))),
           ('header-ffff-wraps', dict(blob=bytes([0xFF, 0xFF]) + bytes(8))), ('header-0000', dict(blob=bytes(10))),
           ('short-file-1-byte', dict(blob=b'\x77')), ('empty-file', dict(blob=b'')),
           # the three terminal IOChecks
           ('g-reset-fails', dict(gone_from=3)), ('g-reset-fails-after-prompt', dict(flag=0, appear_after=2, gone_from=4)),
           ('read-fails', dict(read_fail=1)), ('seek-fails', dict(seek_fail=True)),
           # the message parts and names
           ('table-index-3', dict(table_index=3, flag=0, appear_after=2)), ('table-index-ff', dict(table_index=0xFF)),
           ('disk-z', dict(disk=b'Z', flag=0, appear_after=2)), ('long-base', dict(base_name=b'B' * 60)), ('empty-ext', dict(ext=b'')),
           ('long-table-entry', dict(table=b'T' * 29, flag=0, appear_after=2))]
    out += [(f'flags{fl:x}', dict(flags=fl)) for fl in Q.FLAGS] + [(f'flags{fl:x}-prompt', dict(flags=fl, flag=0, appear_after=2))
                                                                     for fl in Q.FLAGS]
    out += [(f'sp{sp:x}', dict(sp=sp)) for sp in (0x066A, 0x0800, 0x8001, 0xFFE7, 0xFFE8)]
    out += [('ss-ds', dict(ss='ds', sp=0xE000))]
    for k in range(8):
        flag = rng.choice((0, 1))
        out.append((f'random{k}', dict(flag=flag, appear_after=rng.choice((None, 2, 3)) if flag == 0 else None,
                                       blob=bytes(rng.randrange(256) for _ in range(rng.choice((1, 2, 6)))), header=rng.randrange(65536),
                                       table_index=rng.randrange(4), flags=rng.choice(Q.FLAGS),
                                       regs=tuple(rng.randrange(65536) for _ in range(7)))))
    return out


def differential(reloc, decoded, f):
    seen, edges, checked, segments, prompts_total, stops = set(), set(), 0, 0, 0, {}
    for name, o in cases():
        m, c, spec = build(reloc, **o)
        snaps, ofs, v, e = run_unicorn(name, m, c, spec, decoded); seen |= v; edges |= e
        seams, ram, cpu, fs_c = two_phase(name, m, c, spec, snaps, ofs, f)
        segments += len(seams); prompts = seams.count('PROMPT'); prompts_total += prompts; stops[seams[-1]] = stops.get(seams[-1], 0) + 1
        if not name.startswith(('random', 'sp', 'ss-ds')):
            kind, al, want_prompts, header, fpos = expected(o)
            check((seams[-1], prompts) == (kind, want_prompts), f'{name}: (stop, prompts) {(seams[-1], prompts)} vs {(kind, want_prompts)}')
            got_header = ram[physical(NSEG, HDR_AT)] | ram[physical(NSEG, HDR_AT + 1)] << 8
            check(got_header == header, f'{name}: header word {got_header:04x} vs {header:04x}')
            if kind == 'RETURN':
                check(cpu.ax & 0xFF == al, f'{name}: AL {cpu.ax & 0xFF} vs {al}')
                if fpos is not None:
                    fh = ram[physical(NSEG, F_AT)] | ram[physical(NSEG, F_AT + 1)] << 8
                    check(fs_c.handles.get(fh) == fpos, f'{name}: f position {fs_c.handles.get(fh)} vs {fpos}')
            checked += 1
    want = {(a, t) for a, ts in CONDITIONALS.items() for t in ts}
    check(seen == set(decoded), f'starts missing {sorted(hex(a) for a in set(decoded) - seen)}')
    check(edges == want, f'outcomes missing {sorted(want - edges)}')
    return dict(cases=len(cases()), segments=segments, prompts=prompts_total, final_stops=stops, starts=len(seen),
                starts_total=len(decoded), conditional_outcomes=len(edges), conditional_outcomes_total=len(want),
                independent_checks=checked, cpu_words=14, ram_bytes=SIZE, flag_masks=0)


NBASE, NSS, TOP, LOW, WINDOW = 0x3000, 0x6000, 0xF000, 0xE800, 0x0800
DOS_STACK = 24
NATIVE_FILES = {b'HA': bytes([0x34, 0x12]) + bytes(range(32)), b'HB': bytes([0xFE, 0xFF]) + bytes(8), b'HC': b'\x77'}


def native(image, sites, f):
    """The C model against REAL DOS on real files, on the prompt-free paths (the prompt path needs the keyboard and video:
    Unicorn only, declared). Each case calls 05CD directly with base names HA/HB/HC (present, three headers) or HZ (absent) and
    extension .DAX; after the return the oracle asks DOS for f's position (LSEEK 4201h, 0) when f is open."""
    directory = ROOT / 'resources/0802-05cd/native'; directory.mkdir(parents=True, exist_ok=True)
    reloc = Q.relocate(image, sites, NBASE)
    (directory / 'IMAGE.BIN').write_bytes(reloc)
    for stem, data in NATIVE_FILES.items():
        (directory / (stem.decode() + '.DAX')).write_bytes(data)
    (directory / 'HZ.DAX').unlink(missing_ok=True)
    ds, seg0802 = NBASE + Q.SEG['data'], NBASE + 0x0802
    specs = []
    for fl in Q.FLAGS:
        specs += [(f'ha-flag1-{fl:x}', dict(flags=fl, flag=1, stem=b'HA')), (f'hz-flag1-{fl:x}', dict(flags=fl, flag=1, stem=b'HZ')),
                  (f'ha-flag0-{fl:x}', dict(flags=fl, flag=0, stem=b'HA'))]
    specs += [('hb-wrap', dict(flag=1, stem=b'HB')), ('hc-short', dict(flag=1, stem=b'HC'))]
    rows = [[o.get('flags', 2), o['flag'], o['stem'][0] | o['stem'][1] << 8] for _, o in specs]
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
    table, disk, header = b'INSERT DISK ', b'E', 0xA5A5
    regions = [(NSS, LOW, WINDOW), (NSEG, 0x0000, 0x0480), (ds, 0x4420, 0x10)]
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
         'mov si,word ptr [index]', 'mov ax,si', 'shl si,1', 'add si,ax', 'shl si,1', 'add si,offset table',   # 6 bytes/row
         f'mov ax,{ds}', 'mov es,ax', 'mov word ptr es:[0x4426],0', 'mov byte ptr es:[0x442c],2', 'mov word ptr es:[0x49ce],0xeeee',
         'mov byte ptr es:[0x5376],0', f'mov byte ptr es:[0x547d],{disk[0]}', 'mov al,byte ptr cs:[si+2]', 'mov byte ptr es:[0x557c],al',
         f'mov byte ptr es:[0x2614],{len(table)}'] + [f'mov byte ptr es:[{0x2615 + k}],{b}' for k, b in enumerate(table)] + [
         f'mov ax,{NSEG}', 'mov es,ax', 'xor di,di', 'mov cx,0x480', 'mov al,0x5a', 'rep stosb',
         f'mov di,{F_AT}', 'mov cx,0x80', 'xor al,al', 'rep stosb', f'mov di,{G_AT}', 'mov cx,0x80', 'rep stosb',
         f'mov byte ptr es:[{BASE_AT}],2', 'mov ax,word ptr cs:[si+4]', f'mov word ptr es:[{BASE_AT + 1}],ax',
         f'mov byte ptr es:[{EXT_AT}],4'] + [f'mov byte ptr es:[{EXT_AT + 1 + k}],{b}' for k, b in enumerate(b'.DAX')] + [
         f'mov word ptr es:[{HDR_AT}],{header}',
         f'mov ax,{NSS}', 'mov es,ax', f'mov di,{LOW}', f'mov cx,{WINDOW // 2}', 'mov ax,0x5a5a', 'rep stosw',
         f'mov ax,{NSS}', 'mov ss,ax', f'mov sp,{TOP}',
         f'push {NSEG}', f'push {BASE_AT}', f'push {NSEG}', f'push {EXT_AT}', f'push {NSEG}', f'push {HDR_AT}',
         f'push {NSEG}', f'push {F_AT}', f'push {NSEG}', f'push {G_AT}', 'push word ptr cs:[si]', f'mov ax,{ds}', 'mov ds,ax', 'mov ax,0x1111', 'mov es,ax',
         'mov ax,0x1234', 'mov bx,0x3456', 'mov cx,0x5678', 'mov dx,0x789a', 'mov si,0x9abc', 'mov di,0xbcde', 'mov bp,0x7777',
         'popf', f'lcall {seg0802},0x05cd', 'returned:']
    t += [f'mov word ptr cs:[out+{2 * n}],{r}' for n, r in enumerate('ax bx cx dx si di bp sp cs'.split())]
    t += ['mov word ptr cs:[out+18],offset returned', 'mov word ptr cs:[out+20],ds', 'mov word ptr cs:[out+22],es',
          'mov word ptr cs:[out+24],ss', 'mov ax,cs', 'mov ss,ax', 'mov sp,0xfff0', 'pushf', 'pop ax', 'mov word ptr cs:[out+26],ax',
          # f's position from DOS itself (LSEEK 4201h, 0), when f is open (FileRec magic D7B3h); else FFFF:FFFF
          'mov word ptr cs:[out+28],0xffff', 'mov word ptr cs:[out+30],0xffff', f'mov ax,{NSEG}', 'mov es,ax',
          f'cmp word ptr es:[{F_AT + 2}],0xd7b3', 'jne nopos', f'mov bx,word ptr es:[{F_AT}]', 'xor cx,cx', 'xor dx,dx',
          'mov ax,0x4201', 'int 0x21', 'jc nopos', 'mov word ptr cs:[out+28],ax', 'mov word ptr cs:[out+30],dx', 'nopos:',
          'cld', 'push cs', 'pop ds', 'mov bx,word ptr [handle]', 'mov dx,offset out', 'mov cx,32', 'mov ah,0x40', 'int 0x21'] + writes + [
          'mov bx,6', 'closeall:', 'mov ah,0x3e', 'push bx', 'int 0x21', 'pop bx', 'inc bx', 'cmp bx,20', 'jb closeall',
          'push cs', 'pop ds', 'inc word ptr [index]', f'cmp word ptr [index],{len(rows)}', 'jae done', 'jmp again', 'done:',
          'mov bx,word ptr [handle]', 'mov ah,0x3e', 'int 0x21', 'mov ax,0x4c00', 'int 0x21', 'failure:', 'mov ax,0x4c01', 'int 0x21',
          'outname: .asciz "RESULT.BIN"', 'imagename: .asciz "IMAGE.BIN"', 'handle: .word 0', 'image: .word 0', 'index: .word 0',
          '.align 2', 'header: .space 6', 'out: .space 32', 'table:']
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
    data = out.read_bytes(); rec_len = 32 + sum(n for _, _, n in regions)
    check(len(data) == 6 + len(rows) * rec_len, f'native length {len(data)}')
    comseg, ret_off, count = struct.unpack_from('<3H', data); check(count == len(rows), 'native count')
    for n, (name, o) in enumerate(specs):
        rec = data[6 + n * rec_len:6 + (n + 1) * rec_len]
        stem = o['stem']; present = stem in NATIVE_FILES
        m, c, spec = build(reloc, base_name=stem, ext=b'.DAX', flag=o['flag'], present=present,
                           blob=NATIVE_FILES.get(stem, b''), header=header, table=table, disk=disk, flags=o.get('flags', 2),
                           base=NBASE, ss=NSS, sp=TOP - 0x18, ret=(comseg, ret_off), first_handle=6)
        m = bytearray(m)
        m[physical(NSEG, 0):physical(NSEG, 0x480)] = b'\x5a' * 0x480
        for at_ in (F_AT, G_AT):
            m[physical(NSEG, at_):physical(NSEG, at_ + 0x80)] = bytes(0x80)
        pstr(m, NSEG, BASE_AT, stem); pstr(m, NSEG, EXT_AT, b'.DAX'); put16(m, NSEG, HDR_AT, header)
        m[physical(NSS, LOW):physical(NSS, LOW) + WINDOW] = b'\x5a' * WINDOW
        for k, w in enumerate((ret_off, comseg, G_AT, NSEG, F_AT, NSEG, HDR_AT, NSEG, EXT_AT, NSEG, BASE_AT, NSEG)):
            put16(m, NSS, TOP - 0x18 + 2 * k, w)
        status, cpu, ram, fs_c, sps = run_c(f, c, bytes(m), spec, file_service(spec).state())
        check(status == 'RETURN', f'native C status {name}: {status}')
        for (reg, _), want in zip(Cpu._fields_, struct.unpack_from('<14H', rec)):
            check(getattr(cpu, reg) == want, f'native CPU {name}/{reg}: C {getattr(cpu, reg):04x} DOS {want:04x}')
        pos_lo, pos_hi = struct.unpack_from('<2H', rec, 28)
        fh = ram[physical(NSEG, F_AT)] | ram[physical(NSEG, F_AT + 1)] << 8
        magic = ram[physical(NSEG, F_AT + 2)] | ram[physical(NSEG, F_AT + 3)] << 8
        model_pos = fs_c.handles.get(fh) if magic == 0xD7B3 else 0xFFFFFFFF
        check((pos_hi << 16 | pos_lo) == model_pos, f'native {name}: f position DOS {pos_hi << 16 | pos_lo:#x} model {model_pos:#x}')
        mask = {physical(s_, (sp - k) & 0xFFFF) for s_, sp in sps for k in range(1, DOS_STACK + 1)}
        at = 32
        for seg, off, length in regions:
            for k in range(length):
                p = physical(seg, (off + k) & 0xFFFF)
                if p not in mask:
                    check(ram[p] == rec[at + k], f'native {name} at {seg:04x}:{(off + k) & 0xFFFF:04x}: C {ram[p]:02x} DOS {rec[at + k]:02x}')
            at += length
    probe = subprocess.run([binary, '--version'], capture_output=True, text=True, env=dict(os.environ, SDL_VIDEODRIVER='dummy'))
    version = next(line.split(',')[0].strip() for line in (probe.stdout + probe.stderr).splitlines() if 'DOSBox-X version' in line)
    return dict(cases=len(rows), cpu_words=14, dos_stack_mask_bytes_below_int_sp=DOS_STACK, f_position_from_dos=True,
                compared_regions=[f'{s_:04X}:{o_:04X}+{n_:X}' for s_, o_, n_ in regions], result_sha256=sha256(data).hexdigest(),
                dosbox_x=dict(path=binary, sha256=sha256(open(binary, 'rb').read()).hexdigest(), version=version,
                              pinned_source='5a9c7c53481c2e933948039c33256776653f4faf', pinned_build_used=False))


def main():
    p = argparse.ArgumentParser()
    for flag in ('--compile-only', '--unicorn', '--native'):
        p.add_argument(flag, action='store_true')
    p.add_argument('--output', type=Path); a = p.parse_args()
    with tempfile.TemporaryDirectory(prefix='r05cd-') as tmp:
        f = library(Path(tmp))
        if a.compile_only:
            print('CHECKER_BUILD_OK'); return
        image, sites = Q.load_image()
        body, decoded = source(image)
        report = dict(target=dict(span='0802:[05CD,06E7)', bytes=len(body), instructions=len(decoded), sha256=DIGEST,
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
