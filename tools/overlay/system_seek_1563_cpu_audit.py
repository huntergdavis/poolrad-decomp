#!/usr/bin/env python3
"""Turbo Pascal SYSTEM Seek 0AF8:1563 (raw CPU; runtime coverage, no identity) against the original bytes, the open-check helper
14B6 running as original bytes and composed in C from par-d's shared model. INT 21h AH=42h is the declared environment (the case's carry and DX:AX); any other AH or interrupt is refused.
Unicorn: CPU14 + full 1 MiB. Native (DOSBox-X): CPU14 + stack + FileRec + InOutRes, with a declared INT 21h hook and the DOS
frame masked. Media and generated programs stay under ignored resources/."""
import argparse, ctypes, json, os, random, struct, subprocess, tempfile
from hashlib import sha256
from pathlib import Path
from capstone import Cs, CS_ARCH_X86, CS_MODE_16
from resident_loader_map import unpack
from res0709_ega_reset_0000_cpu_audit import Cpu, ROOT, SIZE, REGISTERS, physical, put16, check
CHECKER_ROWS = ('RES_0AF8_1563',)
PIECES = ((0x1563, 0x1593, '598dcb76bff3948d01eb4b14d25969c11f736d4221438ffb29d9d06c9774a4af', 19),
          (0x14B6, 0x14C5, '24d6d6fd5079eeb043c5f3a598502fa40ba66a4e12c919717ccdf847f362d363', 4))
CS, DS, SS, FREC = 0x6000, 0x5000, 0x2000, 0x3000
FLAGS = (0x0002, 0x00D7, 0x08D7, 0x0402, 0x0893, 0x0003)
CONDITIONALS = {0x14BC: (0x14C4, 0x14BE), 0x156C: (0x158F, 0x156E), 0x158A: (0x158F, 0x158C)}
SERVICE_FN = ctypes.CFUNCTYPE(ctypes.c_int, ctypes.c_void_p, ctypes.POINTER(Cpu), ctypes.POINTER(Cpu))
STATUS = {0: 'RETURN', 1: 'ARGUMENT', 2: 'SERVICE', 3: 'ENV', 4: 'TRANSFER'}


def source():
    image = bytes(unpack((ROOT / 'resources/DOS/POOLRAD/START.EXE').read_bytes())[0])
    pieces, decoded = [], {}
    for start, end, digest, count in PIECES:
        body = image[physical(0xAF8, start):physical(0xAF8, end)]
        check(sha256(body).hexdigest() == digest, f'Seek piece {start:04x} identity')
        ins = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(body, start))
        check(len(ins) == count and sum(i.size for i in ins) == len(body), f'decode covers {start:04x}')
        pieces.append((start, body)); decoded.update({i.address: i for i in ins})
    check([a for a, i in decoded.items() if i.mnemonic == 'int'] == [0x1588], 'single INT 21h')
    check([a for a, i in decoded.items() if i.mnemonic == 'mul'] == [0x1571, 0x157A], 'two MULs')
    check({a for a, i in decoded.items() if i.mnemonic.startswith('j')} == set(CONDITIONALS), 'conditional sites')
    return pieces, decoded


def library(directory):
    path = directory / 'seek.so'
    subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Wpedantic', '-Werror', '-O2', '-fPIC', '-shared', '-I', str(ROOT / 'include'),
                    str(ROOT / 'src/recovered/system_seek_1563_cpu.c'),
                    str(ROOT / 'src/recovered/system_filecheck_14b6_cpu.c'), '-o', str(path)], check=True)
    f = ctypes.CDLL(str(path)).pr_system_seek_1563
    f.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t, ctypes.POINTER(Cpu), SERVICE_FN, ctypes.c_void_p]
    f.restype = ctypes.c_int
    return f


def build(pieces, *, magic=0xD7B3, handle=7, recsize=128, n=10, cf=0, result=None, f=(FREC, 0x0100), ss=SS, sp=0x8000, flags=2,
          ds=DS, regs=None):
    m = bytearray([0x5A]) * SIZE
    for start, body in pieces:
        at = physical(CS, start); m[at:at + len(body)] = body
    put16(m, ds, 0x4426, 0x7777)
    seg, off = f
    put16(m, seg, off, handle); put16(m, seg, off + 2, magic); put16(m, seg, off + 4, recsize)
    for j, w in enumerate((0x7000, 0x9001, n & 0xFFFF, (n >> 16) & 0xFFFF, off, seg)):
        put16(m, ss, (sp + 2 * j) & 0xFFFF, w)
    r = regs or (0x1234, 0x3456, 0x5678, 0x789A, 0x9ABC, 0xBCDE, 0x7777)
    pos = (n * recsize) & 0xFFFFFFFF
    return m, Cpu(*r, sp, CS, 0x1563, ds, 0x1111, ss, flags), dict(cf=cf, result=pos if result is None else result)


def answer(b, script):
    """The declared LSEEK: on success DX:AX := the new position (the case's), CF clear; on error AX := the code, CF set."""
    a = Cpu.from_buffer_copy(b)
    if script['cf']:
        a.ax = script['result'] & 0xFFFF; a.flags = b.flags | 1
    else:
        a.ax = script['result'] & 0xFFFF; a.dx = (script['result'] >> 16) & 0xFFFF; a.flags = b.flags & ~1
    a.ip = 0x158A
    return a


def compare(name, state, decoded, f):
    from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE, UC_HOOK_INTR
    from unicorn import x86_const as R
    m, c, script = state
    uc = Uc(UC_ARCH_X86, UC_MODE_16); backing = (ctypes.c_uint8 * SIZE).from_buffer_copy(m)
    uc.mem_map_ptr(0, SIZE, 7, ctypes.addressof(backing)); uc.mem_map_ptr(SIZE, SIZE, 7, ctypes.addressof(backing))
    for (n, _), reg in zip(Cpu._fields_, REGISTERS):
        uc.reg_write(reg, getattr(c, n))
    s = dict(stop=None, ret=False, prev=None); seen = set(); edges = set(); log = []

    def code(em, address, size, user):
        ip = uc.reg_read(R.UC_X86_REG_IP); cs = uc.reg_read(R.UC_X86_REG_CS)
        if s['prev'] in CONDITIONALS:
            edges.add((s['prev'], ip))
        if s['ret']:
            s['stop'] = 'RETURN'; return uc.emu_stop()
        if cs != CS or ip not in decoded:
            s['stop'] = 'TRANSFER'; return uc.emu_stop()
        seen.add(ip); s['prev'] = ip
        if ip == 0x1590:
            s['ret'] = True

    def intr(em, intno, user):
        ah = uc.reg_read(R.UC_X86_REG_AX) >> 8
        check(intno == 0x21 and ah == 0x42, f'ENV refusal {name}: INT {intno:#x} AH {ah:#x}')
        b = Cpu(*[uc.reg_read(r) & 0xFFFF for r in REGISTERS]); log.append((b.ax, b.bx, b.cx, b.dx))
        a = answer(b, script)
        uc.reg_write(R.UC_X86_REG_AX, a.ax); uc.reg_write(R.UC_X86_REG_DX, a.dx); uc.reg_write(R.UC_X86_REG_EFLAGS, a.flags)
    uc.hook_add(UC_HOOK_CODE, code); uc.hook_add(UC_HOOK_INTR, intr)
    uc.emu_start(physical(c.cs, c.ip), 2 * SIZE, count=1000)
    check(s['stop'] is not None, f'original did not stop {name}')
    clog = []

    def svc(ctx, before, after):
        b = before[0]; clog.append((b.ax, b.bx, b.cx, b.dx)); after[0] = answer(b, script); return 0
    actual = Cpu.from_buffer_copy(c); ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(m)
    status = STATUS[f(ram, SIZE, ctypes.byref(actual), SERVICE_FN(svc), None)]
    check(status == s['stop'], f'C status {name}: {status} original {s["stop"]}')
    check(clog == log, f'LSEEK log {name}: C {clog} original {log}')
    if bytes(ram) != bytes(backing):
        diff = [hex(i) for i, (a, b) in enumerate(zip(bytes(ram), bytes(backing))) if a != b]
        raise ValueError(f'RAM {name}: {diff[:8]} ({len(diff)} bytes)')
    for (n, _), reg in zip(Cpu._fields_, REGISTERS):
        want = uc.reg_read(reg) & 0xFFFF
        check(getattr(actual, n) == want, f'CPU {name}/{n} C {getattr(actual, n):04x} original {want:04x}')
    return seen, edges, bytes(ram), log, status


def cases():
    rng = random.Random(0x0AF81563)
    out = [('seek10', dict()), ('seek0', dict(n=0)), ('recsize1', dict(recsize=1, n=123456)), ('recsize0', dict(recsize=0, n=99)),
           # 32-bit truncation: n * recsize >= 2^32, and a high-word product that overflows 16 bits (DX dropped).
           ('trunc-32', dict(n=0x01000000, recsize=0x0200)), ('trunc-high', dict(n=0x00230001, recsize=0x2000)),
           ('max', dict(n=0xFFFFFFFF, recsize=0xFFFF)), ('negative-n', dict(n=0xFFFFFFFE, recsize=128)),
           ('not-open', dict(magic=0xD7B0)), ('unassigned', dict(magic=0x1234)), ('lseek-error', dict(cf=1, result=6)),
           ('lseek-error-1', dict(cf=1, result=1))]
    out += [(f'flags{fl:x}', dict(flags=fl)) for fl in FLAGS] + [(f'flags{fl:x}-closed', dict(flags=fl, magic=0xD7B0)) for fl in FLAGS]
    out += [(f'sp{sp:x}', dict(sp=sp)) for sp in (0x0006, 0x0007, 0x8001, 0xFFF3, 0xFFF4)]
    out += [('ss-ds', dict(ss=DS)), ('a20-stack', dict(ss=0xFFFF, sp=0x0400)), ('frec-straddle', dict(f=(FREC, 0xFFFD))),
            ('frec-a20', dict(f=(0xFFFF, 0x0040))), ('alias-frec-on-args', dict(f=(SS, 0x8004))),
            # the check's InOutRes store on its own return slot (SS = DS, entry SP 442Ah): RET to 0067h (#1314 self-flag)
            ('transfer', dict(ss=DS, sp=0x442A, magic=0xD7B0)), ('ss-ds-slot-open', dict(ss=DS, sp=0x442A))]
    for k in range(40):
        out.append((f'random{k}', dict(magic=rng.choice((0xD7B3, 0xD7B3, 0xD7B0, rng.randrange(65536))), handle=rng.randrange(65536),
                                       recsize=rng.randrange(65536), n=rng.randrange(1 << 32), cf=rng.randrange(2),
                                       result=rng.randrange(1 << 32), flags=rng.choice(FLAGS), regs=tuple(rng.randrange(65536) for _ in range(7)))))
    return out


def differential(pieces, decoded, f):
    seen, edges = set(), set(); specs = cases(); checked = 0; stops = {}
    for name, o in specs:
        v, e, ram, log, status = compare(name, build(pieces, **o), decoded, f); seen |= v; edges |= e
        stops[status] = stops.get(status, 0) + 1
        if name.startswith(('alias', 'ss-ds', 'frec-', 'transfer')):
            continue
        magic = o.get('magic', 0xD7B3); inout = ram[physical(DS, 0x4426)] | ram[physical(DS, 0x4427)] << 8
        if magic != 0xD7B3:
            check(not log and inout == 103, f'{name}: not open -> InOutRes 103, no LSEEK')
        else:
            pos = (o.get('n', 10) * o.get('recsize', 128)) & 0xFFFFFFFF
            check(log == [(0x4200, o.get('handle', 7), pos >> 16, pos & 0xFFFF)], f'{name}: LSEEK operands {log} vs {pos:08x}')
            check(inout == ((o['result'] & 0xFFFF) if o.get('cf') else 0x7777), f'{name}: InOutRes {inout:04x}')
        checked += 1
    check(stops.get('TRANSFER') == 1, 'the constructed TRANSFER alias reached')
    check(seen == set(decoded), f'starts missing {sorted(hex(a) for a in set(decoded) - seen)}')
    want = {(a, t) for a, ts in CONDITIONALS.items() for t in ts}
    check(edges == want, f'outcomes missing {sorted(want - edges)}')
    return dict(cases=len(specs), stops=stops, starts=len(seen), starts_total=len(decoded),
                conditional_outcomes=len(edges), conditional_outcomes_total=len(want), independent_checks=checked,
                cpu_words=14, ram_bytes=SIZE, flag_masks=0)


TOP, LOW, WINDOW = 0xF000, 0xEC00, 0x0400


def native(pieces, f):
    directory = ROOT / 'resources/system-seek/native'; directory.mkdir(parents=True, exist_ok=True)
    for start, body in pieces:
        (directory / f'piece{start:04x}.bin').write_bytes(body)
    specs = [(n, o) for n, o in cases() if not n.startswith(('sp', 'a20', 'ss-ds', 'random', 'alias', 'frec-', 'transfer'))]
    rows = []
    for _, o in specs:
        m, c, script = build(pieces, **o)
        n = o.get('n', 10)
        rows.append((o.get('flags', 2), o.get('magic', 0xD7B3), o.get('handle', 7), o.get('recsize', 128), n & 0xFFFF, n >> 16,
                     script['cf'], script['result'] & 0xFFFF, script['result'] >> 16))
    ROW = 2 * len(rows[0])
    copy = []
    for start, body in pieces:
        copy += [f'mov di,{start}', f'mov si,offset piece{start:04x}', f'mov cx,{len(body)}', 'rep movsb']
    t = ['.code16', '.intel_syntax noprefix', '.global _start', '_start: jmp main', '.org 0x200', 'main:',
         'mov ax,cs', 'mov ds,ax', 'mov ss,ax', f'mov sp,{TOP}', 'cld', f'mov ax,{CS}', 'mov es,ax'] + copy + [
         'mov dx,offset outname', 'xor cx,cx', 'mov ah,0x3c', 'int 0x21', 'jc failure', 'mov word ptr [handle],ax',
         'mov word ptr [header],cs', 'mov word ptr [header+2],offset returned', f'mov word ptr [header+4],{len(rows)}',
         'mov bx,word ptr [handle]', 'mov dx,offset header', 'mov cx,6', 'mov ah,0x40', 'int 0x21',
         'xor ax,ax', 'mov es,ax', 'mov ax,word ptr es:[0x84]', 'mov word ptr [old21],ax', 'mov ax,word ptr es:[0x86]', 'mov word ptr [old21+2],ax',
         'again:', 'cli', 'cld', 'mov ax,cs', 'mov ds,ax', 'mov es,ax', 'mov al,0x5a', f'mov di,{LOW}', f'mov cx,{WINDOW}', 'rep stosb',
         'mov si,word ptr [index]', f'mov ax,{ROW}', 'mul si', 'mov si,ax', 'add si,offset table',
         f'mov ax,{FREC}', 'mov es,ax', 'mov di,0x0100', 'mov cx,0x10', 'mov al,0x5a', 'rep stosb',
         'mov ax,word ptr [si+4]', 'mov word ptr es:[0x0100],ax', 'mov ax,word ptr [si+2]', 'mov word ptr es:[0x0102],ax',
         'mov ax,word ptr [si+6]', 'mov word ptr es:[0x0104],ax',
         f'mov ax,{DS}', 'mov es,ax', 'mov di,0x4420', 'mov cx,0x10', 'mov al,0x5a', 'rep stosb', 'mov word ptr es:[0x4426],0x7777',
         'mov ax,word ptr [si+12]', 'mov word ptr [s_cf],ax', 'mov ax,word ptr [si+14]', 'mov word ptr [s_lo],ax',
         'mov ax,word ptr [si+16]', 'mov word ptr [s_hi],ax',
         'xor ax,ax', 'mov es,ax', 'mov word ptr es:[0x84],offset int21', 'mov word ptr es:[0x86],cs',
         f'mov sp,{TOP}', f'push {FREC}', 'push 0x0100', 'push word ptr cs:[si+10]', 'push word ptr cs:[si+8]', 'push word ptr cs:[si]',
         f'mov ax,{DS}', 'mov ds,ax', 'mov ax,0x1111', 'mov es,ax',
         'mov ax,0x1234', 'mov bx,0x3456', 'mov cx,0x5678', 'mov dx,0x789a', 'mov si,0x9abc', 'mov di,0xbcde', 'mov bp,0x7777',
         'popf', f'lcall {CS},0x1563', 'returned:']
    t += [f'mov word ptr cs:[out+{2 * n}],{r}' for n, r in enumerate('ax bx cx dx si di bp sp cs'.split())]
    t += ['mov word ptr cs:[out+18],offset returned', 'mov word ptr cs:[out+20],ds', 'mov word ptr cs:[out+22],es',
          'mov word ptr cs:[out+24],ss', 'mov sp,0xf800', 'pushf', 'pop ax', 'mov word ptr cs:[out+26],ax', 'cld', 'mov ax,cs', 'mov ds,ax',
          'xor ax,ax', 'mov es,ax', 'mov ax,word ptr [old21]', 'mov word ptr es:[0x84],ax', 'mov ax,word ptr [old21+2]', 'mov word ptr es:[0x86],ax',
          'sti', 'mov bx,word ptr [handle]', 'mov dx,offset out', 'mov cx,28', 'mov ah,0x40', 'int 0x21',
          'mov bx,word ptr [handle]', f'mov dx,{LOW}', f'mov cx,{WINDOW}', 'mov ah,0x40', 'int 0x21',
          f'mov ax,{FREC}', 'mov ds,ax', 'mov bx,word ptr cs:[handle]', 'mov dx,0x0100', 'mov cx,0x10', 'mov ah,0x40', 'int 0x21',
          f'mov ax,{DS}', 'mov ds,ax', 'mov bx,word ptr cs:[handle]', 'mov dx,0x4420', 'mov cx,0x10', 'mov ah,0x40', 'int 0x21',
          'push cs', 'pop ds', 'inc word ptr [index]', f'cmp word ptr [index],{len(rows)}', 'jae done', 'jmp again', 'done:',
          'mov bx,word ptr [handle]', 'mov ah,0x3e', 'int 0x21', 'mov ax,0x4c00', 'int 0x21', 'failure:', 'mov ax,0x4c01', 'int 0x21',
          # Declared INT 21h hook: AH=42h answers from the case (DX:AX, or AX and CF), CF edited in the IRET frame.
          'int21:', 'cmp ah,0x42', 'je d_seek', 'jmp dword ptr cs:[old21]',
          'd_seek:', 'mov word ptr cs:[save],bp', 'mov bp,sp', 'and word ptr [bp+4],0xfffe', 'mov ax,word ptr cs:[s_lo]',
          'cmp word ptr cs:[s_cf],0', 'je d_ok', 'or word ptr [bp+4],1', 'jmp d_out', 'd_ok:', 'mov dx,word ptr cs:[s_hi]',
          'd_out:', 'mov bp,word ptr cs:[save]', 'iret',
          'outname: .asciz "RESULT.BIN"', 'handle: .word 0', 'index: .word 0', 'save: .word 0', 's_cf: .word 0', 's_lo: .word 0',
          's_hi: .word 0', 'old21: .word 0,0', 'header: .space 6', 'out: .space 28', '.align 2', 'table:']
    t += ['.word ' + ','.join(str(v) for v in row) for row in rows]
    for start, _ in pieces:
        t += [f'piece{start:04x}:', f'.incbin "{directory / f"piece{start:04x}.bin"}"']
    (directory / 'oracle.S').write_text('\n'.join(t) + '\n')
    subprocess.run(['as', '--32', str(directory / 'oracle.S'), '-o', str(directory / 'oracle.o')], check=True)
    com = directory / 'ORACLE.COM'
    subprocess.run(['ld', '-m', 'elf_i386', '-Ttext', '0x100', '--oformat', 'binary', str(directory / 'oracle.o'), '-o', str(com)], check=True)
    check(com.stat().st_size + 0x100 < LOW, 'COM reaches the stack window')
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
                       check=True, timeout=300)
    data = out.read_bytes(); record = 28 + WINDOW + 0x10 + 0x10
    check(len(data) == 6 + len(rows) * record, f'native length {len(data)}')
    seg, ret_ip, count = struct.unpack_from('<3H', data); check(count == len(rows), 'native count')
    for n, (name, o) in enumerate(specs):
        rec = data[6 + n * record:6 + (n + 1) * record]
        m, c, script = build(pieces, **o)
        m = bytearray(m)
        m[physical(seg, LOW):physical(seg, LOW) + WINDOW] = b'\x5a' * WINDOW
        m[physical(FREC, 0x100):physical(FREC, 0x110)] = b'\x5a' * 0x10
        put16(m, FREC, 0x100, o.get('handle', 7)); put16(m, FREC, 0x102, o.get('magic', 0xD7B3)); put16(m, FREC, 0x104, o.get('recsize', 128))
        m[physical(DS, 0x4420):physical(DS, 0x4430)] = b'\x5a' * 0x10; put16(m, DS, 0x4426, 0x7777)
        sp = TOP - 0x0C; nn = o.get('n', 10)
        for j, w in enumerate((ret_ip, seg, nn & 0xFFFF, nn >> 16, 0x0100, FREC)):
            put16(m, seg, sp + 2 * j, w)
        c.ss, c.sp = seg, sp
        sps = []

        def svc(ctx, before, after, script=script):
            sps.append(before[0].sp); after[0] = answer(before[0], script); return 0
        ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(m)
        check(STATUS[f(ram, SIZE, ctypes.byref(c), SERVICE_FN(svc), None)] == 'RETURN', f'native C status {name}')
        for (reg, _), want in zip(Cpu._fields_, struct.unpack_from('<14H', rec)):
            check(getattr(c, reg) == want, f'native CPU {name}/{reg}: C {getattr(c, reg):04x} DOS {want:04x}')
        mask = {physical(seg, (s - k) & 0xFFFF) for s in sps for k in range(1, 7)}
        window = bytes(ram[physical(seg, LOW):physical(seg, LOW) + WINDOW])
        for k in range(WINDOW):
            if physical(seg, LOW + k) not in mask:
                check(window[k] == rec[28 + k], f'native stack {name} at {LOW + k:04x}')
        check(bytes(ram[physical(FREC, 0x100):physical(FREC, 0x110)]) == rec[28 + WINDOW:28 + WINDOW + 0x10], f'native FileRec {name}')
        check(bytes(ram[physical(DS, 0x4420):physical(DS, 0x4430)]) == rec[28 + WINDOW + 0x10:], f'native InOutRes {name}')
    probe = subprocess.run([binary, '--version'], capture_output=True, text=True, env=dict(os.environ, SDL_VIDEODRIVER='dummy'))
    version = next(line.split(',')[0].strip() for line in (probe.stdout + probe.stderr).splitlines() if 'DOSBox-X version' in line)
    return dict(cases=len(rows), cpu_words=14, stack_bytes=WINDOW, filerec_bytes=0x10, data_bytes=0x10, flag_masks=0,
                result_sha256=sha256(data).hexdigest(),
                dosbox_x=dict(path=binary, sha256=sha256(open(binary, 'rb').read()).hexdigest(), version=version,
                              pinned_source='5a9c7c53481c2e933948039c33256776653f4faf', pinned_build_used=False))


def main():
    p = argparse.ArgumentParser()
    for flag in ('--compile-only', '--unicorn', '--native'):
        p.add_argument(flag, action='store_true')
    p.add_argument('--output', type=Path); a = p.parse_args()
    with tempfile.TemporaryDirectory(prefix='seek-') as tmp:
        f = library(Path(tmp))
        if a.compile_only:
            print('CHECKER_BUILD_OK'); return
        pieces, decoded = source()
        report = dict(target=dict(spans=['0AF8:[1563,1593)', '0AF8:[14B6,14C5) (composed: par-d system_filecheck_14b6_cpu)'], bytes=63, instructions=23, sha256=[d for _, _, d, _ in PIECES]))
        if a.unicorn:
            report['unicorn'] = differential(pieces, decoded, f)
        if a.native:
            report['native'] = native(pieces, f)
    if a.output:
        a.output.write_text(json.dumps(report, sort_keys=True, indent=2) + '\n')
    print(json.dumps(report, sort_keys=True))


if __name__ == '__main__':
    main()
