#!/usr/bin/env python3
"""RES_0802_02EB (FileExists) against the original bytes, with StrStore, Assign, Reset, Close and IOResult running as
original bytes and composed in C from their models.

The whole loader-expanded START.EXE image is loaded at a test base with the loader's own relocation table applied. Declared
environment: INT 21h AH=3Dh (Reset's contract) and AH=3Eh (Close's), served by the 0716 audit's DOS file service: the case's
file opens under its name when present (the lowest free handle), an absent file answers error 2, a close answers AL = 01h.
Unicorn: CPU14 + full 1 MiB + the ordered DOS event log. Native (DOSBox-X): the C model against REAL DOS on real files.
Media and generated programs stay under ignored resources/."""
import argparse, ctypes, json, os, random, struct, subprocess, sys, tempfile
from hashlib import sha256
from pathlib import Path
from capstone import Cs, CS_ARCH_X86, CS_MODE_16
from res0709_ega_reset_0000_cpu_audit import Cpu, ROOT, SIZE, REGISTERS, physical, put16, check
import res0899_quit_0000_cpu_audit as Q
import res0802_0716_cpu_audit as A716
CHECKER_ROWS = ('RES_0802_02EB',)
ENTRY, END = 0x02EB, 0x0356
DIGEST = '141b948984a41382c1479d61b09a298f9662139a1bae0fc9f4601484532cd474'
BASE, NSEG = Q.BASE, 0x5800
SOURCES = ['resident_0802_02eb_cpu', 'system_strstore_fullstate_0b25', 'system_ioresult_assign_cpu', 'system_runtime_leaves_cpu',
           'system_reset_1410_cpu', 'system_close_1491_cpu', 'system_filecheck_14b6_cpu']
CONDITIONALS = {0x033C: (0x0344, 0x033E), 0x0342: (0x0348, 0x0344)}
STATUS = {0: 'RETURN', 1: 'ARGUMENT', 2: 'SERVICE', 3: 'CALLEE'}
SERVICE_FN = ctypes.CFUNCTYPE(ctypes.c_int, ctypes.c_void_p, ctypes.POINTER(Cpu), ctypes.POINTER(Cpu))


class Env(ctypes.Structure):
    _fields_ = [('open', SERVICE_FN), ('open_context', ctypes.c_void_p), ('close', SERVICE_FN), ('close_context', ctypes.c_void_p)]


class Diag(ctypes.Structure):
    _fields_ = [('site', ctypes.c_uint16), ('callee_status', ctypes.c_int)]


def source(image):
    body = image[0x802 * 16 + ENTRY:0x802 * 16 + END]
    check(sha256(body).hexdigest() == DIGEST, '0802:02EB identity')
    ins = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(body, ENTRY))
    check(len(ins) == 43 and sum(i.size for i in ins) == len(body), 'decode covers 02EB')
    check([i.address for i in ins if i.mnemonic == 'lcall'] == [0x0300, 0x0310, 0x031F, 0x032A, 0x032F], 'five far calls')
    check({i.address for i in ins if i.mnemonic.startswith('j') and i.mnemonic != 'jmp'} == set(CONDITIONALS), 'conditional sites')
    return body, {i.address: i for i in ins}


def library(directory):
    path = directory / 'r02eb.so'
    subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Wpedantic', '-Werror', '-O2', '-fPIC', '-shared', '-I', str(ROOT / 'include')]
                   + [str(ROOT / f'src/recovered/{n}.c') for n in SOURCES] + ['-o', str(path)], check=True)
    f = ctypes.CDLL(str(path)).pr_r0802_02eb
    f.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t, ctypes.POINTER(Cpu), ctypes.POINTER(Env), ctypes.POINTER(Diag)]
    f.restype = ctypes.c_int
    return f


def build(reloc, *, name=b'ECL1.DAX', present=True, filemode=2, inoutres=0, ss=0x7000, sp=0xF000, flags=2, regs=None,
          base=BASE, ret=(0x9001, 0x7000), name_at=(NSEG, 0x0010), first_handle=5):
    m = bytearray([0x5A]) * SIZE
    m[base * 16:base * 16 + len(reloc)] = reloc
    ds = base + Q.SEG['data']
    put16(m, ds, 0x4426, inoutres); m[physical(ds, 0x442C)] = filemode; put16(m, ds, 0x49CE, 0xEEEE)
    seg, off = name_at
    m[physical(seg, off)] = len(name) & 0xFF
    for k, ch in enumerate(name):
        m[physical(seg, (off + 1 + k) & 0xFFFF)] = ch
    ss = ds if ss == 'ds' else ss
    for k, w in enumerate((ret[1], ret[0], off, seg)):
        put16(m, ss, (sp + 2 * k) & 0xFFFF, w)
    r = regs or (0x1234, 0x3456, 0x5678, 0x789A, 0x9ABC, 0xBCDE, 0x7777)
    return m, Cpu(*r, sp, base + 0x0802, ENTRY, ds, 0x1111, ss, flags), dict(present=present, ret=ret, base=base, first_handle=first_handle)


def file_service(spec):
    return A716.FileSystem(b'DAX' if spec['present'] else None, first_handle=spec['first_handle'])


def run_unicorn(name, m, c, spec, decoded):
    from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE, UC_HOOK_INTR
    from unicorn import x86_const as R
    base = spec['base']; fs = file_service(spec)
    uc = Uc(UC_ARCH_X86, UC_MODE_16); backing = (ctypes.c_uint8 * SIZE).from_buffer_copy(m)
    uc.mem_map_ptr(0, SIZE, 7, ctypes.addressof(backing)); uc.mem_map_ptr(SIZE, SIZE, 7, ctypes.addressof(backing))
    for (n, _), reg in zip(Cpu._fields_, REGISTERS):
        uc.reg_write(reg, getattr(c, n))
    s = dict(stop=None, prev=None); seen = set(); edges = set()

    def code(em, address, size, user):
        cs = uc.reg_read(R.UC_X86_REG_CS); ip = uc.reg_read(R.UC_X86_REG_IP)
        if s['prev'] is not None and cs == base + 0x0802:
            edges.add((s['prev'], ip))
        s['prev'] = None
        if (cs, ip) == spec['ret']:
            s['stop'] = 'RETURN'; return uc.emu_stop()
        check(cs in (base + 0x0802, base + Q.SEG['system']), f'escape {name} {cs:04x}:{ip:04x}')
        if cs == base + 0x0802:
            check(ip in decoded, f'escape inside 02EB {name} {ip:04x}'); seen.add(ip)
            if ip in CONDITIONALS:
                s['prev'] = ip

    def intr(em, intno, user):
        ax = uc.reg_read(R.UC_X86_REG_AX); fl = uc.reg_read(R.UC_X86_REG_EFLAGS)
        check(intno == 0x21 and ax >> 8 in (0x3D, 0x3E), f'ENV refusal {name}: INT {intno:#x} AX {ax:#x}')
        ds_, dx_ = uc.reg_read(R.UC_X86_REG_DS), uc.reg_read(R.UC_X86_REG_DX)
        nm = bytes(backing[physical(ds_, dx_):physical(ds_, dx_) + 80]).split(b'\0')[0].decode('latin-1') if ax >> 8 == 0x3D else None
        cf, ax_out, _ = fs.call(ax >> 8, ax & 0xFF, uc.reg_read(R.UC_X86_REG_BX), 0, dx_, nm)
        uc.reg_write(R.UC_X86_REG_AX, ax_out); uc.reg_write(R.UC_X86_REG_EFLAGS, (fl | 1) if cf else (fl & ~1))
    uc.hook_add(UC_HOOK_CODE, code); uc.hook_add(UC_HOOK_INTR, intr)
    uc.emu_start(physical(c.cs, c.ip), 2 * SIZE, count=50000)
    check(s['stop'] == 'RETURN', f'original stop {name}: {s["stop"]}')
    return Cpu(*[uc.reg_read(r) & 0xFFFF for r in REGISTERS]), bytes(backing), fs.log, seen, edges


def run_c(f, m, c, spec):
    fs = file_service(spec); sps = []

    def service(ctx, before, after):
        b = before[0]; sps.append((b.ss, b.sp))
        nm = None
        if b.ax >> 8 == 0x3D:
            ram = ctypes.cast(ctx, ctypes.POINTER(ctypes.c_uint8))
            at = physical(b.ds, b.dx); raw = bytes(ram[at:at + 80]); nm = raw.split(b'\0')[0].decode('latin-1')
        cf, ax, _ = fs.call(b.ax >> 8, b.ax & 0xFF, b.bx, 0, b.dx, nm)
        a = Cpu.from_buffer_copy(b); a.ax = ax; a.flags = (b.flags | 1) if cf else (b.flags & ~1); a.ip = (b.ip + 2) & 0xFFFF
        after[0] = a; return 0
    keep = SERVICE_FN(service)
    ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(m); cpu = Cpu.from_buffer_copy(c); d = Diag()
    ctx = ctypes.cast(ram, ctypes.c_void_p)
    env = Env(keep, ctx, keep, ctx)
    status = STATUS[f(ram, SIZE, ctypes.byref(cpu), ctypes.byref(env), ctypes.byref(d))]
    return status, cpu, bytes(ram), fs.log, sps


def expected(o):
    """Independent statement: AL = 1 iff the file opens and the copied name (max 40) is not empty; [49CE] = IOResult."""
    name, present = o.get('name', b'ECL1.DAX'), o.get('present', True)
    if o.get('inoutres', 0):
        return None
    if not name:                                     # Reset's empty name: no INT, handle 0; Close of handle 0: no INT
        return 0, 0
    if present:
        return 1, 0
    return 0, 103                                    # the open fails (2), then Close of a record that is not open sets 103


def cases():
    rng = random.Random(0x080202EB)
    out = [('present', dict()), ('absent', dict(present=False)), ('empty-name', dict(name=b'')),
           ('empty-name-absent', dict(name=b'', present=False)), ('long-name', dict(name=b'X' * 60)),
           ('max-name', dict(name=b'Y' * 40)), ('filemode0', dict(filemode=0)), ('filemode40', dict(filemode=0x40)),
           ('inoutres-pending', dict(inoutres=5)), ('inoutres-pending-absent', dict(inoutres=5, present=False))]
    out += [(f'flags{fl:x}', dict(flags=fl)) for fl in Q.FLAGS] + [(f'flags{fl:x}-absent', dict(flags=fl, present=False)) for fl in Q.FLAGS]
    out += [(f'sp{sp:x}', dict(sp=sp)) for sp in (0x0100, 0x0101, 0x8001, 0xFFF7, 0xFFF8)]
    out += [('ss-ds', dict(ss='ds', sp=0xE000)), ('name-straddle', dict(name_at=(NSEG, 0xFFF8))), ('name-a20', dict(name_at=(0xFFFF, 0x0040)))]
    for k in range(10):
        out.append((f'random{k}', dict(name=bytes(rng.randrange(0x41, 0x5B) for _ in range(rng.choice((0, 1, 8, 12, 45)))),
                                       present=rng.randrange(2) == 0, filemode=rng.choice((0, 1, 2)), flags=rng.choice(Q.FLAGS),
                                       regs=tuple(rng.randrange(65536) for _ in range(7)))))
    return out


def differential(reloc, decoded, f):
    seen, edges, checked = set(), set(), 0
    for name, o in cases():
        m, c, spec = build(reloc, **o)
        cpu, ram, log, v, e = run_unicorn(name, m, c, spec, decoded); seen |= v; edges |= e
        cst, ccpu, cram, clog, _ = run_c(f, m, c, spec)
        check(cst == 'RETURN', f'{name}: C {cst}')
        for (n, _) in Cpu._fields_:
            check(getattr(ccpu, n) == getattr(cpu, n), f'CPU {name}/{n} C {getattr(ccpu, n):04x} original {getattr(cpu, n):04x}')
        A716.compare_ram(name, cram, ram)
        check(clog == log, f'{name}: DOS log C {clog} original {log}')
        exp = expected(o)
        if exp is not None and not name.startswith(('random', 'sp', 'ss-ds', 'name-')):
            ds = BASE + Q.SEG['data']
            got = ram[physical(ds, 0x49CE)] | ram[physical(ds, 0x49CF)] << 8
            check((cpu.ax & 0xFF, got) == exp, f'{name}: AL/[49CE] {(cpu.ax & 0xFF, got)} vs {exp}')
            checked += 1
    want = {(a, t) for a, ts in CONDITIONALS.items() for t in ts}
    check(seen == set(decoded), f'starts missing {sorted(hex(a) for a in set(decoded) - seen)}')
    check(edges == want, f'outcomes missing {sorted(want - edges)}')
    return dict(cases=len(cases()), stops=dict(RETURN=len(cases())), starts=len(seen), starts_total=len(decoded),
                conditional_outcomes=len(edges), conditional_outcomes_total=len(want), independent_checks=checked,
                cpu_words=14, ram_bytes=SIZE, flag_masks=0)


NBASE, NSS, TOP, LOW, WINDOW = 0x3000, 0x6000, 0xF000, 0xE800, 0x0800
DOS_STACK = 24


def native(image, sites, decoded, f):
    directory = ROOT / 'resources/0802-02eb/native'; directory.mkdir(parents=True, exist_ok=True)
    reloc = Q.relocate(image, sites, NBASE)
    (directory / 'IMAGE.BIN').write_bytes(reloc)
    (directory / 'PRESENT.DAX').write_bytes(b'DAX')
    (directory / 'ABSENT.DAX').unlink(missing_ok=True)
    ds, seg0802 = NBASE + Q.SEG['data'], NBASE + 0x0802
    specs = [(n, o) for n, o in cases() if not n.startswith(('sp', 'ss-ds', 'random', 'name-', 'long-name', 'max-name'))]
    names = []
    for _, o in specs:
        nm = o.get('name', b'ECL1.DAX')
        nm = (b'PRESENT.DAX' if o.get('present', True) else b'ABSENT.DAX') if nm else b''
        names.append(nm)
    rows = []
    for (_, o), nm in zip(specs, names):
        padded = nm.ljust(12, b'\0')                     # 10 words per row: flags, FileMode, InOutRes, length, 12 name bytes
        rows.append([o.get('flags', 2), o.get('filemode', 2), o.get('inoutres', 0), len(nm)]
                    + [padded[2 * k] | padded[2 * k + 1] << 8 for k in range(6)])
    load = []
    for k, start in enumerate(range(0, len(reloc), 0x8000)):
        load += [f'mov ax,{NBASE + 0x800 * k}', 'mov ds,ax', 'mov bx,word ptr cs:[image]', 'xor dx,dx',
                 f'mov cx,{min(0x8000, len(reloc) - start)}', 'mov ah,0x3f', 'int 0x21', 'jc failure']
    regions = [(NSS, LOW, WINDOW), (NSEG, 0x0000, 0x40), (ds, 0x4420, 0x10), (ds, 0x49CE, 0x02)]
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
         'mov si,word ptr [index]', 'mov ax,si', 'shl si,1', 'shl si,1', 'add si,ax', 'shl si,1', 'shl si,1', 'add si,offset table',  # 20 bytes/row
         f'mov ax,{ds}', 'mov es,ax', 'mov ax,word ptr cs:[si+4]', 'mov word ptr es:[0x4426],ax', 'mov al,byte ptr cs:[si+2]',
         'mov byte ptr es:[0x442c],al', 'mov word ptr es:[0x49ce],0xeeee',
         f'mov ax,{NSEG}', 'mov es,ax', 'xor di,di', 'mov cx,0x40', 'mov al,0x5a', 'rep stosb',
         'mov al,byte ptr cs:[si+6]', 'mov byte ptr es:[0x10],al', 'push si', 'mov cl,al', 'xor ch,ch', 'add si,8',
         'mov di,0x11', 'rep movsb', 'pop si',
         f'mov ax,{NSS}', 'mov es,ax', f'mov di,{LOW}', f'mov cx,{WINDOW // 2}', 'mov ax,0x5a5a', 'rep stosw',
         f'mov ax,{NSS}', 'mov ss,ax', f'mov sp,{TOP}', f'push {NSEG}', 'push 0x10', 'push word ptr cs:[si]',
         f'mov ax,{ds}', 'mov ds,ax', 'mov ax,0x1111', 'mov es,ax',
         'mov ax,0x1234', 'mov bx,0x3456', 'mov cx,0x5678', 'mov dx,0x789a', 'mov si,0x9abc', 'mov di,0xbcde', 'mov bp,0x7777',
         'popf', f'lcall {seg0802},0x02eb', 'returned:']
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
    for n, ((name, o), nm) in enumerate(zip(specs, names)):
        rec = data[6 + n * rec_len:6 + (n + 1) * rec_len]
        oo = dict(o, name=nm)
        m, c, spec = build(reloc, base=NBASE, ss=NSS, sp=TOP - 8, ret=(comseg, ret_off), first_handle=6, **oo)
        m = bytearray(m)
        m[physical(NSEG, 0):physical(NSEG, 0x40)] = b'\x5a' * 0x40
        m[physical(NSEG, 0x10)] = len(nm); m[physical(NSEG, 0x11):physical(NSEG, 0x11) + len(nm)] = nm
        m[physical(NSS, LOW):physical(NSS, LOW) + WINDOW] = b'\x5a' * WINDOW
        for k, w in enumerate((ret_off, comseg, 0x0010, NSEG)):
            put16(m, NSS, TOP - 8 + 2 * k, w)
        st, cpu, ram, _, sps = run_c(f, bytes(m), c, spec)
        check(st == 'RETURN', f'native C status {name}')
        for (reg, _), want in zip(Cpu._fields_, struct.unpack_from('<14H', rec)):
            check(getattr(cpu, reg) == want, f'native CPU {name}/{reg}: C {getattr(cpu, reg):04x} DOS {want:04x}')
        mask = {physical(s, (sp - k) & 0xFFFF) for s, sp in sps for k in range(1, DOS_STACK + 1)}
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
                compared_regions=[f'{s:04X}:{o:04X}+{n:X}' for s, o, n in regions], result_sha256=sha256(data).hexdigest(),
                dosbox_x=dict(path=binary, sha256=sha256(open(binary, 'rb').read()).hexdigest(), version=version,
                              pinned_source='5a9c7c53481c2e933948039c33256776653f4faf', pinned_build_used=False))


def main():
    p = argparse.ArgumentParser()
    for flag in ('--compile-only', '--unicorn', '--native'):
        p.add_argument(flag, action='store_true')
    p.add_argument('--output', type=Path); a = p.parse_args()
    with tempfile.TemporaryDirectory(prefix='r02eb-') as tmp:
        f = library(Path(tmp))
        if a.compile_only:
            print('CHECKER_BUILD_OK'); return
        image, sites = Q.load_image()
        body, decoded = source(image)
        report = dict(target=dict(span='0802:[02EB,0356)', bytes=len(body), instructions=len(decoded), sha256=DIGEST,
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
