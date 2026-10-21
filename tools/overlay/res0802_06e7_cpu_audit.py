#!/usr/bin/env python3
"""RES_0802_06E7 (close both archive files) against the original bytes, with Close 1491 and IOCheck 04A9 running as
original bytes and composed in C from their models.

The whole loader-expanded START.EXE image is loaded at a test base with the loader's own relocation table applied (as in
res0802_0716_cpu_audit.py). Declared environment: INT 21h AH=3Eh served by the same DOS file service as the 0716 audit
(a successful close answers AL = 01h with AH kept; an unopened handle answers error 6), Close's own contract.
Unicorn: CPU14 + full 1 MiB + the ordered 3Eh event log. Native (DOSBox-X): the C model against REAL DOS, the oracle
opening real files for the live handles; RunError captured at 0AF8:00E2 (declared). Media stay under ignored resources/."""
import argparse, ctypes, json, os, random, struct, subprocess, sys, tempfile
from hashlib import sha256
from pathlib import Path
from capstone import Cs, CS_ARCH_X86, CS_MODE_16
from res0709_ega_reset_0000_cpu_audit import Cpu, ROOT, SIZE, REGISTERS, physical, put16, check
import res0899_quit_0000_cpu_audit as Q
import res0802_0716_cpu_audit as A716
CHECKER_ROWS = ('RES_0802_06E7',)
ENTRY, END = 0x06E7, 0x070C
DIGEST = 'ed4938d30a0049177fe39d10417d6fc40b42c851dd3f826d31d08a58a675324f'
BASE, FSEG = Q.BASE, 0x5800
SOURCES = ['resident_0802_06e7_cpu', 'system_close_1491_cpu', 'system_filecheck_14b6_cpu', 'system_iocheck_04a9_cpu']
STATUS = {0: 'RETURN', 1: 'ARGUMENT', 2: 'RUNERROR', 3: 'SERVICE', 4: 'CALLEE'}
SERVICE_FN = ctypes.CFUNCTYPE(ctypes.c_int, ctypes.c_void_p, ctypes.POINTER(Cpu), ctypes.POINTER(Cpu))


class Diag(ctypes.Structure):
    _fields_ = [('site', ctypes.c_uint16), ('callee_status', ctypes.c_int)]


def source(image):
    body = image[0x802 * 16 + ENTRY:0x802 * 16 + END]
    check(sha256(body).hexdigest() == DIGEST, '0802:06E7 identity')
    ins = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(body, ENTRY))
    check(len(ins) == 14 and sum(i.size for i in ins) == len(body), 'decode covers 06E7')
    check([i.address for i in ins if i.mnemonic == 'lcall'] == [0x06EF, 0x06F4, 0x06FE, 0x0703], 'four far calls')
    check(not [i for i in ins if i.mnemonic.startswith('j')], 'no branches')
    return body, {i.address: i for i in ins}


def library(directory):
    path = directory / 'r06e7.so'
    subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Wpedantic', '-Werror', '-O2', '-fPIC', '-shared', '-I', str(ROOT / 'include')]
                   + [str(ROOT / f'src/recovered/{n}.c') for n in SOURCES] + ['-o', str(path)], check=True)
    f = ctypes.CDLL(str(path)).pr_r0802_06e7
    f.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t, ctypes.POINTER(Cpu), SERVICE_FN, ctypes.c_void_p, ctypes.POINTER(Diag)]
    f.restype = ctypes.c_int
    return f


def build(reloc, *, a=(7, 0xD7B3), b=(6, 0xD7B3), fa=(FSEG, 0x0100), fb=(FSEG, 0x0200), open_handles=(6, 7), ss=0x7000,
          sp=0xF000, flags=2, regs=None, base=BASE, ret=(0x9001, 0x7000), inoutres=0):
    m = bytearray([0x5A]) * SIZE
    m[base * 16:base * 16 + len(reloc)] = reloc
    ds = base + Q.SEG['data']
    put16(m, ds, 0x4426, inoutres)
    for (seg, off), (handle, magic) in ((fb, b), (fa, a)):
        for k in range(0x80):
            m[physical(seg, (off + k) & 0xFFFF)] = 0
        put16(m, seg, off, handle); put16(m, seg, (off + 2) & 0xFFFF, magic); put16(m, seg, (off + 4) & 0xFFFF, 128)
    ss = ds if ss == 'ds' else ss
    for k, w in enumerate((ret[1], ret[0], fa[1], fa[0], fb[1], fb[0])):
        put16(m, ss, (sp + 2 * k) & 0xFFFF, w)
    r = regs or (0x1234, 0x3456, 0x5678, 0x789A, 0x9ABC, 0xBCDE, 0x7777)
    return m, Cpu(*r, sp, base + 0x0802, ENTRY, ds, 0x1111, ss, flags), dict(open_handles=open_handles, ret=ret, base=base)


def file_service(open_handles):
    fs = A716.FileSystem()
    fs.handles = {h: 0 for h in open_handles}
    return fs


def run_unicorn(name, m, c, spec, decoded):
    from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE, UC_HOOK_INTR
    from unicorn import x86_const as R
    base = spec['base']; fs = file_service(spec['open_handles']); int_sps = []
    uc = Uc(UC_ARCH_X86, UC_MODE_16); backing = (ctypes.c_uint8 * SIZE).from_buffer_copy(m)
    uc.mem_map_ptr(0, SIZE, 7, ctypes.addressof(backing)); uc.mem_map_ptr(SIZE, SIZE, 7, ctypes.addressof(backing))
    for (n, _), reg in zip(Cpu._fields_, REGISTERS):
        uc.reg_write(reg, getattr(c, n))
    s = dict(stop=None); seen = set()

    def code(em, address, size, user):
        cs = uc.reg_read(R.UC_X86_REG_CS); ip = uc.reg_read(R.UC_X86_REG_IP)
        if (cs, ip) == spec['ret']:
            s['stop'] = 'RETURN'; return uc.emu_stop()
        if (cs, ip) == (base + Q.SEG['system'], 0x00E2):
            s['stop'] = 'RUNERROR'; return uc.emu_stop()
        check(cs in (base + 0x0802, base + Q.SEG['system']), f'escape {name} {cs:04x}:{ip:04x}')
        if cs == base + 0x0802:
            check(ip in decoded, f'escape inside 06E7 {name} {ip:04x}'); seen.add(ip)

    def intr(em, intno, user):
        ax = uc.reg_read(R.UC_X86_REG_AX); fl = uc.reg_read(R.UC_X86_REG_EFLAGS)
        check(intno == 0x21 and ax >> 8 == 0x3E, f'ENV refusal {name}: INT {intno:#x} AX {ax:#x}')
        int_sps.append((uc.reg_read(R.UC_X86_REG_SS), uc.reg_read(R.UC_X86_REG_SP)))
        cf, ax_out, _ = fs.call(0x3E, ax & 0xFF, uc.reg_read(R.UC_X86_REG_BX), 0, 0)
        uc.reg_write(R.UC_X86_REG_AX, ax_out); uc.reg_write(R.UC_X86_REG_EFLAGS, (fl | 1) if cf else (fl & ~1))
    uc.hook_add(UC_HOOK_CODE, code); uc.hook_add(UC_HOOK_INTR, intr)
    uc.emu_start(physical(c.cs, c.ip), 2 * SIZE, count=20000)
    check(s['stop'] in ('RETURN', 'RUNERROR'), f'original stop {name}: {s["stop"]}')
    cpu = Cpu(*[uc.reg_read(r) & 0xFFFF for r in REGISTERS])
    return s['stop'], cpu, bytes(backing), fs.log, seen, int_sps


def run_c(f, m, c, spec):
    fs = file_service(spec['open_handles']); sps = []

    def service(ctx, before, after):
        b = before[0]; sps.append((b.ss, b.sp))
        cf, ax, _ = fs.call(0x3E, b.ax & 0xFF, b.bx, 0, 0)
        a = Cpu.from_buffer_copy(b); a.ax = ax; a.flags = (b.flags | 1) if cf else (b.flags & ~1); a.ip = (b.ip + 2) & 0xFFFF
        after[0] = a; return 0
    keep = SERVICE_FN(service)
    ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(m); cpu = Cpu.from_buffer_copy(c); d = Diag()
    status = STATUS[f(ram, SIZE, ctypes.byref(cpu), keep, None, ctypes.byref(d))]
    return status, cpu, bytes(ram), fs.log, sps


def expected(o):
    """Independent statement: 3Eh for each handle above 4 (B first, then A), magic D7B0 after a close of an open file;
    InOutRes 103 for a magic other than D7B3, the DOS error for an unopened handle; IOCheck then stops at the first error."""
    log, open_ = [], set(o.get('open_handles', (6, 7)))
    for key in ('b', 'a'):
        handle, magic = o.get(key, (6, 0xD7B3) if key == 'b' else (7, 0xD7B3))
        if magic != 0xD7B3:
            return 'RUNERROR', log, 103
        if handle > 4:
            if handle in open_:
                log.append(('3E', handle)); open_.discard(handle)
            else:
                log.append(('3E', handle, 'invalid')); return 'RUNERROR', log, 6
    return 'RETURN', log, 0


def cases():
    rng = random.Random(0x080206E7)
    out = [('both-open', dict()), ('a-standard', dict(a=(2, 0xD7B3))), ('b-standard', dict(b=(4, 0xD7B3))),
           ('both-standard', dict(a=(0, 0xD7B3), b=(1, 0xD7B3))), ('handle-5', dict(b=(5, 0xD7B3), open_handles=(5, 7))),
           ('b-closed', dict(b=(6, 0xD7B0))), ('a-closed', dict(a=(7, 0xD7B0))), ('b-unassigned', dict(b=(6, 0x1234))),
           ('b-invalid-handle', dict(b=(15, 0xD7B3))), ('a-invalid-handle', dict(a=(15, 0xD7B3))),
           ('same-filerec', dict(fa=(FSEG, 0x0200))), ('same-filerec-standard', dict(fa=(FSEG, 0x0200), b=(3, 0xD7B3))),
           ('inoutres-pending', dict(inoutres=2))]
    out += [(f'flags{fl:x}', dict(flags=fl)) for fl in Q.FLAGS]
    out += [(f'sp{sp:x}', dict(sp=sp)) for sp in (0x000C, 0x000D, 0x8001, 0xFFF3, 0xFFF4)]
    out += [('ss-ds', dict(ss='ds', sp=0xE000)), ('frec-straddle', dict(fb=(FSEG, 0xFFFE))), ('frec-a20', dict(fa=(0xFFFF, 0x0040)))]
    for k in range(12):
        out.append((f'random{k}', dict(a=(rng.choice((2, 6, 7, 15)), rng.choice((0xD7B3, 0xD7B3, 0xD7B0))),
                                       b=(rng.choice((3, 6, 7, 15)), rng.choice((0xD7B3, 0xD7B3, 0xD7B0))),
                                       flags=rng.choice(Q.FLAGS), regs=tuple(rng.randrange(65536) for _ in range(7)))))
    return out


def differential(reloc, decoded, f):
    seen, stops, checked = set(), {}, 0
    for name, o in cases():
        m, c, spec = build(reloc, **o)
        st, cpu, ram, log, v, _ = run_unicorn(name, m, c, spec, decoded); seen |= v
        cst, ccpu, cram, clog, _ = run_c(f, m, c, spec)
        check(cst == st, f'{name}: C {cst} original {st}')
        for (n, _) in Cpu._fields_:
            check(getattr(ccpu, n) == getattr(cpu, n), f'CPU {name}/{n} C {getattr(ccpu, n):04x} original {getattr(cpu, n):04x}')
        A716.compare_ram(name, cram, ram)
        check(clog == log, f'{name}: 3Eh log C {clog} original {log}')
        stops[st] = stops.get(st, 0) + 1
        if not name.startswith(('same-filerec', 'inoutres', 'frec-', 'random', 'sp', 'ss-ds')):
            est, elog, inout = expected(o)
            ds = BASE + Q.SEG['data']
            got = ram[physical(ds, 0x4426)] | ram[physical(ds, 0x4427)] << 8
            check((st, log) == (est, elog), f'{name}: expected {est} {elog}, got {st} {log}')
            check(st == 'RETURN' or cpu.ax == inout, f'{name}: RunError AX {cpu.ax:04x} vs {inout}')
            check(st != 'RETURN' or got == 0, f'{name}: InOutRes')
            checked += 1
    check(seen == set(decoded), f'starts missing {sorted(hex(a) for a in set(decoded) - seen)}')
    return dict(cases=len(cases()), stops=stops, starts=len(seen), starts_total=len(decoded), conditional_outcomes=0,
                independent_checks=checked, cpu_words=14, ram_bytes=SIZE, flag_masks=0)


NBASE, NSS, TOP, LOW, WINDOW = 0x3000, 0x6000, 0xF000, 0xE800, 0x0800
DOS_STACK = 24


def native(image, sites, decoded, f):
    directory = ROOT / 'resources/0802-06e7/native'; directory.mkdir(parents=True, exist_ok=True)
    reloc = Q.relocate(image, sites, NBASE)
    (directory / 'IMAGE.BIN').write_bytes(reloc)
    sysseg, ds, seg0802 = NBASE + Q.SEG['system'], NBASE + Q.SEG['data'], NBASE + 0x0802
    err_before = reloc[Q.SEG['system'] * 16 + 0xE2:Q.SEG['system'] * 16 + 0xE7]
    specs = [(n, o) for n, o in cases() if not n.startswith(('sp', 'ss-ds', 'random', 'frec-a20', 'handle-5'))]
    # Rows: flags, A handle, A magic, B handle, B magic, A off, B off, inoutres. "Open" handles are made real by opening
    # IMAGE.BIN twice per case (DOS returns 6 and 7: RESULT.BIN holds 5); the oracle closes 6..19 after every case.
    rows = [(o.get('flags', 2), *o.get('a', (7, 0xD7B3)), *o.get('b', (6, 0xD7B3)), o.get('fa', (FSEG, 0x100))[1],
             o.get('fb', (FSEG, 0x200))[1], o.get('inoutres', 0)) for _, o in specs]
    load = []
    for k, start in enumerate(range(0, len(reloc), 0x8000)):
        load += [f'mov ax,{NBASE + 0x800 * k}', 'mov ds,ax', 'mov bx,word ptr cs:[image]', 'xor dx,dx',
                 f'mov cx,{min(0x8000, len(reloc) - start)}', 'mov ah,0x3f', 'int 0x21', 'jc failure']
    regions = [(NSS, LOW, WINDOW), (FSEG, 0x0100, 0x80), (FSEG, 0x0200, 0x80), (FSEG, 0xFF80, 0x80), (ds, 0x4420, 0x10)]
    writes = []
    for seg, off, n in regions:
        writes += [f'mov ax,{seg}', 'mov ds,ax', 'mov bx,word ptr cs:[handle]', f'mov dx,{off}', f'mov cx,{n}', 'mov ah,0x40', 'int 0x21']
    t = ['.code16', '.intel_syntax noprefix', '.global _start', '_start: jmp main', '.org 0x200', 'main:',
         'mov ax,cs', 'mov ds,ax', 'mov ss,ax', 'mov sp,0xfff0', 'cld',
         'mov dx,offset outname', 'xor cx,cx', 'mov ah,0x3c', 'int 0x21', 'jc failure', 'mov word ptr [handle],ax',
         'mov word ptr [header],cs', 'mov word ptr [header+2],offset returned', f'mov word ptr [header+4],{len(rows)}',
         'mov bx,word ptr [handle]', 'mov dx,offset header', 'mov cx,6', 'mov ah,0x40', 'int 0x21',
         'again:', 'cld', 'push cs', 'pop ds', 'mov dx,offset imagename', 'mov ax,0x3d00', 'int 0x21', 'jc failure',
         'mov word ptr cs:[image],ax'] + load + [
         'push cs', 'pop ds', 'mov bx,word ptr [image]', 'mov ah,0x3e', 'int 0x21',
         'mov dx,offset imagename', 'mov ax,0x3d00', 'int 0x21', 'mov word ptr [h6],ax',      # two live handles: 6 and 7
         'mov dx,offset imagename', 'mov ax,0x3d00', 'int 0x21', 'mov word ptr [h7],ax',
         f'mov ax,{sysseg}', 'mov es,ax', 'mov byte ptr es:[0xe2],0xea', 'mov word ptr es:[0xe3],offset cap_err', 'mov word ptr es:[0xe5],cs',
         'mov si,word ptr [index]', 'mov ax,si', 'shl si,1', 'shl si,1', 'shl si,1', 'shl si,1', 'add si,offset table',   # 16 bytes/row
         f'mov ax,{ds}', 'mov es,ax', 'mov ax,word ptr cs:[si+14]', 'mov word ptr es:[0x4426],ax',
         f'mov ax,{FSEG}', 'mov es,ax', 'xor di,di', 'mov cx,0x8000', 'xor ax,ax', 'rep stosw',
         'mov di,word ptr cs:[si+12]', 'mov ax,word ptr cs:[si+6]', 'mov word ptr es:[di],ax', 'mov ax,word ptr cs:[si+8]',
         'mov word ptr es:[di+2],ax', 'mov word ptr es:[di+4],128',
         'mov di,word ptr cs:[si+10]', 'mov ax,word ptr cs:[si+2]', 'mov word ptr es:[di],ax', 'mov ax,word ptr cs:[si+4]',
         'mov word ptr es:[di+2],ax', 'mov word ptr es:[di+4],128',
         f'mov ax,{NSS}', 'mov es,ax', f'mov di,{LOW}', f'mov cx,{WINDOW // 2}', 'mov ax,0x5a5a', 'rep stosw',
         f'mov ax,{NSS}', 'mov ss,ax', f'mov sp,{TOP}', f'push {FSEG}', 'push word ptr cs:[si+12]', f'push {FSEG}', 'push word ptr cs:[si+10]',
         'push word ptr cs:[si]', f'mov ax,{ds}', 'mov ds,ax', 'mov ax,0x1111', 'mov es,ax',
         'mov ax,0x1234', 'mov bx,0x3456', 'mov cx,0x5678', 'mov dx,0x789a', 'mov si,0x9abc', 'mov di,0xbcde', 'mov bp,0x7777',
         'popf', f'lcall {seg0802},0x06e7', 'returned:']
    t += [f'mov word ptr cs:[out+{2 * n}],{r}' for n, r in enumerate('ax bx cx dx si di bp sp cs'.split())]
    t += ['mov word ptr cs:[out+18],offset returned', 'jmp common', 'cap_err:']
    t += [f'mov word ptr cs:[out+{2 * n}],{r}' for n, r in enumerate('ax bx cx dx si di bp sp'.split())]
    t += [f'mov word ptr cs:[out+16],{sysseg}', 'mov word ptr cs:[out+18],0xe2',
          'common:', 'mov word ptr cs:[out+20],ds', 'mov word ptr cs:[out+22],es', 'mov word ptr cs:[out+24],ss',
          'mov ax,cs', 'mov ss,ax', 'mov sp,0xfff0', 'pushf', 'pop ax', 'mov word ptr cs:[out+26],ax', 'cld', 'push cs', 'pop ds',
          'mov bx,word ptr [handle]', 'mov dx,offset out', 'mov cx,28', 'mov ah,0x40', 'int 0x21',
          'mov bx,word ptr [handle]', 'mov dx,offset h6', 'mov cx,4', 'mov ah,0x40', 'int 0x21'] + writes + [
          'mov bx,6', 'closeall:', 'mov ah,0x3e', 'push bx', 'int 0x21', 'pop bx', 'inc bx', 'cmp bx,20', 'jb closeall',
          'push cs', 'pop ds', 'inc word ptr [index]', f'cmp word ptr [index],{len(rows)}', 'jae done', 'jmp again', 'done:',
          'mov bx,word ptr [handle]', 'mov ah,0x3e', 'int 0x21', 'mov ax,0x4c00', 'int 0x21', 'failure:', 'mov ax,0x4c01', 'int 0x21',
          'outname: .asciz "RESULT.BIN"', 'imagename: .asciz "IMAGE.BIN"', 'handle: .word 0', 'image: .word 0', 'index: .word 0',
          'h6: .word 0', 'h7: .word 0', '.align 2', 'header: .space 6', 'out: .space 28', 'table:']
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
    data = out.read_bytes(); rec_len = 28 + 4 + sum(n for _, _, n in regions)
    check(len(data) == 6 + len(rows) * rec_len, f'native length {len(data)}')
    comseg, ret_off, count = struct.unpack_from('<3H', data); check(count == len(rows), 'native count')
    stops = {}
    for n, (name, o) in enumerate(specs):
        rec = data[6 + n * rec_len:6 + (n + 1) * rec_len]
        h6, h7 = struct.unpack_from('<2H', rec, 28)
        check((h6, h7) == (6, 7), f'native live handles {name}: {h6} {h7}')
        m, c, spec = build(reloc, base=NBASE, ss=NSS, sp=TOP - 10, ret=(comseg, ret_off), open_handles=(6, 7), **o)
        m = bytearray(m)
        m[physical(FSEG, 0):physical(FSEG, 0xFFFF) + 1] = bytes(0x10000)
        for (seg, off), (handle, magic) in ((o.get('fb', (FSEG, 0x200)), o.get('b', (6, 0xD7B3))), (o.get('fa', (FSEG, 0x100)), o.get('a', (7, 0xD7B3)))):
            put16(m, seg, off, handle); put16(m, seg, (off + 2) & 0xFFFF, magic); put16(m, seg, (off + 4) & 0xFFFF, 128)
        m[physical(NSS, LOW):physical(NSS, LOW) + WINDOW] = b'\x5a' * WINDOW
        for k, w in enumerate((ret_off, comseg, o.get('fa', (FSEG, 0x100))[1], FSEG, o.get('fb', (FSEG, 0x200))[1], FSEG)):
            put16(m, NSS, TOP - 12 + 2 * k, w)
        c.sp = TOP - 12
        st, cpu, ram, _, sps = run_c(f, bytes(m), c, spec)
        stops[st] = stops.get(st, 0) + 1
        for (reg, _), want in zip(Cpu._fields_, struct.unpack_from('<14H', rec)):
            check(getattr(cpu, reg) == want, f'native CPU {name}/{reg}: C {getattr(cpu, reg):04x} DOS {want:04x}')
        mask = {physical(s, (sp - k) & 0xFFFF) for s, sp in sps for k in range(1, DOS_STACK + 1)}
        at = 32
        for seg, off, length in regions:
            for k in range(length):
                p = physical(seg, (off + k) & 0xFFFF)
                if p not in mask:
                    check(ram[p] == rec[at + k], f'native {name} at {seg:04x}:{(off + k) & 0xFFFF:04x}: C {ram[p]:02x} DOS {rec[at + k]:02x}')
            at += length
    probe = subprocess.run([binary, '--version'], capture_output=True, text=True, env=dict(os.environ, SDL_VIDEODRIVER='dummy'))
    version = next(line.split(',')[0].strip() for line in (probe.stdout + probe.stderr).splitlines() if 'DOSBox-X version' in line)
    return dict(cases=len(rows), stops=stops, cpu_words=14, dos_stack_mask_bytes_below_int_sp=DOS_STACK,
                compared_regions=[f'{s:04X}:{o:04X}+{n:X}' for s, o, n in regions],
                captures=dict(runerror=dict(at='0AF8:00E2', before=err_before.hex())), result_sha256=sha256(data).hexdigest(),
                dosbox_x=dict(path=binary, sha256=sha256(open(binary, 'rb').read()).hexdigest(), version=version,
                              pinned_source='5a9c7c53481c2e933948039c33256776653f4faf', pinned_build_used=False))


def main():
    p = argparse.ArgumentParser()
    for flag in ('--compile-only', '--unicorn', '--native'):
        p.add_argument(flag, action='store_true')
    p.add_argument('--output', type=Path); a = p.parse_args()
    with tempfile.TemporaryDirectory(prefix='r06e7-') as tmp:
        f = library(Path(tmp))
        if a.compile_only:
            print('CHECKER_BUILD_OK'); return
        image, sites = Q.load_image()
        body, decoded = source(image)
        report = dict(target=dict(span='0802:[06E7,070C)', bytes=len(body), instructions=len(decoded), sha256=DIGEST,
                                  image_sha256=Q.IMAGE_DIGEST, relocations=len(sites)))
        if a.unicorn:
            report['unicorn'] = differential(Q.relocate(image, sites, BASE), decoded, f)
        if a.native:
            report['native'] = native(image, sites, decoded, f)
    if a.output:
        a.output.write_text(json.dumps(report, sort_keys=True, indent=2) + '\n')
    print(json.dumps(report, sort_keys=True))


if __name__ == '__main__':
    main()
