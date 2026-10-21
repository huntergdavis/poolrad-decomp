#!/usr/bin/env python3
"""Turbo Pascal SYSTEM FilePos 0AF8:1593 + position helper 15DA (raw CPU; runtime coverage, no identity) against the original
bytes, LongDiv 0A76 running as original bytes and composed in C from its covered model. INT 21h AH=42h is the declared
environment: THREE LSEEKs per open call (4201h, 4202h, 4200h), each answered from the case's script (DX:AX and carry); any
other AH or interrupt is refused. Unicorn: CPU14 + full 1 MiB + the ordered LSEEK event log. Native (DOSBox-X): CPU14 + stack +
FileRec + InOutRes + the event log, with a declared INT 21h hook, the DOS frames masked and a declared capture at CS:00E2
(LongDiv's runtime-error target, outside the pieces). Media and generated programs stay under ignored resources/."""
import argparse, ctypes, json, os, random, struct, subprocess, tempfile
from hashlib import sha256
from pathlib import Path
from capstone import Cs, CS_ARCH_X86, CS_MODE_16
from resident_loader_map import unpack
from res0709_ega_reset_0000_cpu_audit import Cpu, ROOT, SIZE, REGISTERS, physical, put16, check
CHECKER_ROWS = ('RES_0AF8_1593',)
PIECES = ((0x1593, 0x15AA, '60365ee418d46db02cab8c9d35bc2b9bd4121e5e7f993b6d2c83174c344c0c9a', 9),
          (0x15DA, 0x161F, '337cdaa3ab878cf422eccc440796d2eceac214d3234178d952eb1a6cd135afa5', 32),
          (0x0A76, 0x0AE4, '131b97677c63a6daa216f4219868f045485e35f999a551feb16daf4ec50d69e5', 55))
OWN = {0x1593, 0x15DA}          # pieces whose starts this model must cover (LongDiv is composed, covered elsewhere)
CS, DS, SS, FREC = 0x6000, 0x5000, 0x2000, 0x3000
FLAGS = (0x0002, 0x00D7, 0x08D7, 0x0402, 0x0893, 0x0003)
CONDITIONALS = {0x1596: (0x15A7, 0x1598), 0x159F: (0x15A7, 0x15A1), 0x15E6: (0x1613, 0x15E8)}
INTS = (0x15F2, 0x1600, 0x160E)
SERVICE_FN = ctypes.CFUNCTYPE(ctypes.c_int, ctypes.c_void_p, ctypes.POINTER(Cpu), ctypes.POINTER(Cpu))
STATUS = {0: 'RETURN', 1: 'ARGUMENT', 2: 'SERVICE', 3: 'ENV', 4: 'DIVIDE', 5: 'TRANSFER'}


def source():
    image = bytes(unpack((ROOT / 'resources/DOS/POOLRAD/START.EXE').read_bytes())[0])
    pieces, decoded, own = [], {}, {}
    for start, end, digest, count in PIECES:
        body = image[physical(0xAF8, start):physical(0xAF8, end)]
        if digest:
            check(sha256(body).hexdigest() == digest, f'FilePos piece {start:04x} identity')
        ins = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(body, start))
        check(len(ins) == count and sum(i.size for i in ins) == len(body), f'decode covers {start:04x}')
        pieces.append((start, body)); decoded.update({i.address: i for i in ins})
        if start in OWN:
            own.update({i.address: i for i in ins})
    check([a for a, i in own.items() if i.mnemonic == 'int'] == list(INTS), 'three INT 21h')
    check([a for a, i in own.items() if i.mnemonic == 'call'] == [0x1593, 0x15A4], 'CALL 15DA and CALL 0A76')
    check({a for a, i in own.items() if i.mnemonic.startswith('j') and i.mnemonic != 'jmp'} == set(CONDITIONALS), 'conditional sites')
    return pieces, decoded, own


def digests(pieces):
    return [sha256(body).hexdigest() for _, body in pieces]


def library(directory):
    path = directory / 'filepos.so'
    subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Wpedantic', '-Werror', '-O2', '-fPIC', '-shared', '-I', str(ROOT / 'include'),
                    str(ROOT / 'src/recovered/system_filepos_1593_cpu.c'), str(ROOT / 'src/recovered/system_longdiv_0a76_cpu.c'),
                    '-o', str(path)], check=True)
    f = ctypes.CDLL(str(path)).pr_system_filepos_1593
    f.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t, ctypes.POINTER(Cpu), SERVICE_FN, ctypes.c_void_p]
    f.restype = ctypes.c_int
    return f


def build(pieces, *, magic=0xD7B3, handle=7, recsize=128, answers=None, f=(FREC, 0x0100), ss=SS, sp=0x8000, flags=2, ds=DS, regs=None):
    """answers: three (DX:AX, carry) for 4201h (current), 4202h (end), 4200h (restore). Default: current 12345h, end 30000h."""
    m = bytearray([0x5A]) * SIZE
    for start, body in pieces:
        at = physical(CS, start); m[at:at + len(body)] = body
    put16(m, ds, 0x4426, 0x7777)
    seg, off = f
    put16(m, seg, off, handle); put16(m, seg, off + 2, magic); put16(m, seg, off + 4, recsize)
    for j, w in enumerate((0x7000, 0x9001, off, seg)):
        put16(m, ss, (sp + 2 * j) & 0xFFFF, w)
    r = regs or (0x1234, 0x3456, 0x5678, 0x789A, 0x9ABC, 0xBCDE, 0x7777)
    answers = answers or ((0x12345, 0), (0x30000, 0), (0x12345, 0))
    return m, Cpu(*r, sp, CS, 0x1593, ds, 0x1111, ss, flags), dict(answers=answers)


def answer(b, script, k):
    """The declared LSEEK number k: DX:AX := the case's answer, CF := its carry."""
    value, carry = script['answers'][k]
    a = Cpu.from_buffer_copy(b)
    a.ax = value & 0xFFFF; a.dx = (value >> 16) & 0xFFFF
    a.flags = (b.flags | 1) if carry else (b.flags & ~1)
    a.ip = (b.ip + 2) & 0xFFFF
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
            s['stop'] = 'DIVIDE' if (cs, ip) == (CS, 0x00E2) else 'TRANSFER'; return uc.emu_stop()
        seen.add(ip); s['prev'] = ip
        if ip == 0x15A7:
            s['ret'] = True

    def intr(em, intno, user):
        ah = uc.reg_read(R.UC_X86_REG_AX) >> 8
        check(intno == 0x21 and ah == 0x42 and len(log) < 3, f'ENV refusal {name}: INT {intno:#x} AH {ah:#x}')
        b = Cpu(*[uc.reg_read(r) & 0xFFFF for r in REGISTERS]); b.ip = uc.reg_read(R.UC_X86_REG_IP) - 2
        log.append((b.ip, b.ax, b.bx, b.cx, b.dx))
        a = answer(b, script, len(log) - 1)
        uc.reg_write(R.UC_X86_REG_AX, a.ax); uc.reg_write(R.UC_X86_REG_DX, a.dx); uc.reg_write(R.UC_X86_REG_EFLAGS, a.flags)
    uc.hook_add(UC_HOOK_CODE, code); uc.hook_add(UC_HOOK_INTR, intr)
    uc.emu_start(physical(c.cs, c.ip), 2 * SIZE, count=5000)
    check(s['stop'] is not None, f'original did not stop {name}')
    clog = []

    def svc(ctx, before, after):
        b = before[0]; clog.append((b.ip, b.ax, b.bx, b.cx, b.dx)); after[0] = answer(b, script, len(clog) - 1); return 0
    actual = Cpu.from_buffer_copy(c); ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(m)
    status = STATUS[f(ram, SIZE, ctypes.byref(actual), SERVICE_FN(svc), None)]
    check(status == s['stop'], f'C status {name}: {status} original {s["stop"]}')
    check(clog == log, f'LSEEK event log {name}: C {clog} original {log}')
    if bytes(ram) != bytes(backing):
        diff = [hex(i) for i, (a, b) in enumerate(zip(bytes(ram), bytes(backing))) if a != b]
        raise ValueError(f'RAM {name}: {diff[:8]} ({len(diff)} bytes)')
    for (n, _), reg in zip(Cpu._fields_, REGISTERS):
        want = uc.reg_read(reg) & 0xFFFF
        check(getattr(actual, n) == want, f'CPU {name}/{n} C {getattr(actual, n):04x} original {want:04x}')
    return seen, edges, bytes(ram), log, status, actual


def expected(o, script):
    """Independent statement of FilePos's effect: (status, event log, InOutRes, DX:AX or None, carry or None)."""
    magic, recsize, handle = o.get('magic', 0xD7B3), o.get('recsize', 128), o.get('handle', 7)
    if magic != 0xD7B3:
        return 'RETURN', [], 103, 0, 1
    (cur, _), _, (restored, carry) = script['answers']
    log = [(0x15F2, 0x4201, handle, 0, 0), (0x1600, 0x4202, handle, 0, 0), (0x160E, 0x4200, handle, cur >> 16, cur & 0xFFFF)]
    if carry or recsize == 1:
        return 'RETURN', log, 0x7777, restored, None
    if recsize == 0:
        return 'DIVIDE', log, 0x7777, None, None
    signed = restored - (1 << 32) if restored & 0x80000000 else restored
    q = abs(signed) // recsize * (-1 if signed < 0 else 1)
    return 'RETURN', log, 0x7777, q & 0xFFFFFFFF, None


def cases():
    rng = random.Random(0x0AF81593)
    out = [('pos', dict()), ('pos0', dict(answers=((0, 0), (0, 0), (0, 0)))), ('recsize1', dict(recsize=1)),
           ('recsize2', dict(recsize=2)), ('recsize-ffff', dict(recsize=0xFFFF, answers=((0xFFFE0001, 0), (0xFFFF0000, 0), (0xFFFE0001, 0)))),
           ('remainder', dict(recsize=7, answers=((100, 0), (200, 0), (100, 0)))),
           # signed LongDiv: positions >= 2^31 divide as negative LongInts
           ('signed', dict(recsize=2, answers=((0x80000000, 0), (0x80000000, 0), (0x80000000, 0)))),
           ('signed-128', dict(answers=((0xFFFFFF00, 0), (0xFFFFFF00, 0), (0xFFFFFF00, 0)))),
           # no carry check between the LSEEKs; only the restore's carry is seen
           ('err-current', dict(answers=((6, 1), (0x30000, 0), (0x300, 0)))), ('err-end', dict(answers=((0x1000, 0), (6, 1), (0x1000, 0)))),
           ('err-restore', dict(answers=((0x1000, 0), (0x30000, 0), (6, 1)))), ('err-all', dict(answers=((6, 1), (5, 1), (6, 1)))),
           ('restore-differs', dict(answers=((0x1000, 0), (0x30000, 0), (0x2000, 0)))),
           ('err-restore-recsize0', dict(recsize=0, answers=((0x1000, 0), (0x30000, 0), (6, 1)))),
           ('not-open', dict(magic=0xD7B0)), ('unassigned', dict(magic=0x1234)),
           # recsize 0: LongDiv divisor 0 -> runtime error 200 (EXIT-class)
           ('divide', dict(recsize=0)), ('divide-zero-pos', dict(recsize=0, answers=((0, 0), (0, 0), (0, 0))))]
    out += [(f'flags{fl:x}', dict(flags=fl)) for fl in FLAGS] + [(f'flags{fl:x}-closed', dict(flags=fl, magic=0xD7B0)) for fl in FLAGS]
    out += [(f'flags{fl:x}-recsize1', dict(flags=fl, recsize=1)) for fl in FLAGS]
    out += [(f'flags{fl:x}-divide', dict(flags=fl, recsize=0)) for fl in FLAGS]
    out += [(f'sp{sp:x}', dict(sp=sp)) for sp in (0x000A, 0x000B, 0x8001, 0xFFF7, 0xFFF8)]
    out += [('ss-ds', dict(ss=DS)), ('a20-stack', dict(ss=0xFFFF, sp=0x0400)), ('frec-straddle', dict(f=(FREC, 0xFFFD))),
            ('frec-a20', dict(f=(0xFFFF, 0x0040))), ('alias-frec-on-args', dict(f=(SS, 0x8004))),
            # the helper's InOutRes store on its own return slot (SS = DS, entry SP 4428h)
            ('transfer', dict(ss=DS, sp=0x4428, magic=0xD7B0)), ('ss-ds-slot-open', dict(ss=DS, sp=0x4428))]
    for k in range(40):
        out.append((f'random{k}', dict(magic=rng.choice((0xD7B3, 0xD7B3, 0xD7B0, rng.randrange(65536))), handle=rng.randrange(65536),
                                       recsize=rng.choice((0, 1, 2, 128, rng.randrange(65536))),
                                       answers=tuple((rng.randrange(1 << 32), int(rng.randrange(4) == 0)) for _ in range(3)),
                                       flags=rng.choice(FLAGS), regs=tuple(rng.randrange(65536) for _ in range(7)))))
    return out


def differential(pieces, decoded, own, f):
    seen, edges = set(), set(); specs = cases(); checked = 0; stops = {}
    for name, o in specs:
        state = build(pieces, **o)
        v, e, ram, log, status, cpu = compare(name, state, decoded, f); seen |= v; edges |= e
        stops[status] = stops.get(status, 0) + 1
        if name.startswith(('alias', 'transfer', 'ss-ds-slot', 'frec-')):
            continue
        st, want_log, inout, value, carry = expected(o, state[2])
        got_inout = ram[physical(o.get('ds', DS), 0x4426)] | ram[physical(o.get('ds', DS), 0x4427)] << 8
        check(status == st and log == want_log, f'{name}: status/log {status} {log} vs {st} {want_log}')
        check(got_inout == inout, f'{name}: InOutRes {got_inout:04x} vs {inout:04x}')
        if value is not None:
            check((cpu.dx << 16 | cpu.ax) == value, f'{name}: DX:AX {cpu.dx:04x}:{cpu.ax:04x} vs {value:08x}')
        if carry is not None:
            check((cpu.flags & 1) == carry, f'{name}: carry')
        if st == 'DIVIDE':
            check(cpu.ip == 0x00E2 and cpu.ax == 0x00C8, f'{name}: DIVIDE at the runtime-error JMP target, AX 200')
        checked += 1
    check('TRANSFER' in stops and 'DIVIDE' in stops, 'named stops reached')
    check(seen & set(own) == set(own), f'starts missing {sorted(hex(a) for a in set(own) - seen)}')
    want = {(a, t) for a, ts in CONDITIONALS.items() for t in ts}
    check(edges == want, f'outcomes missing {sorted(want - edges)}')
    return dict(cases=len(specs), stops=stops, starts=len(seen & set(own)), starts_total=len(own),
                longdiv_starts_run=len(seen - set(own)), conditional_outcomes=len(edges), conditional_outcomes_total=len(want),
                independent_checks=checked, lseek_event_log='ordered (IP, AX, BX, CX, DX) per INT, C == original',
                cpu_words=14, ram_bytes=SIZE, flag_masks=0)


TOP, LOW, WINDOW = 0xF000, 0xEC00, 0x0400


def native(pieces, f):
    directory = ROOT / 'resources/system-filepos/native'; directory.mkdir(parents=True, exist_ok=True)
    for start, body in pieces:
        (directory / f'piece{start:04x}.bin').write_bytes(body)
    specs = [(n, o) for n, o in cases() if not n.startswith(('sp', 'a20', 'ss-ds', 'random', 'alias', 'frec-', 'transfer'))]
    rows = []
    for _, o in specs:
        m, c, script = build(pieces, **o)
        row = [o.get('flags', 2), o.get('magic', 0xD7B3), o.get('handle', 7), o.get('recsize', 128)]
        for value, carry in script['answers']:
            row += [value & 0xFFFF, value >> 16, carry]
        rows.append(tuple(row))
    ROW = 2 * len(rows[0])
    copy = []
    for start, body in pieces:
        copy += [f'mov di,{start}', f'mov si,offset piece{start:04x}', f'mov cx,{len(body)}', 'rep movsb']
    # Declared capture at CS:00E2 (LongDiv's runtime-error target, not in any piece): JMP FAR to the harness.
    copy += ['mov byte ptr es:[0x00e2],0xea', 'mov word ptr es:[0x00e3],offset diverr', 'mov word ptr es:[0x00e5],cs']
    t = ['.code16', '.intel_syntax noprefix', '.global _start', '_start: jmp main', '.org 0x200', 'main:',
         'mov ax,cs', 'mov ds,ax', 'mov ss,ax', f'mov sp,{TOP}', 'cld', f'mov ax,{CS}', 'mov es,ax'] + copy + [
         'mov dx,offset outname', 'xor cx,cx', 'mov ah,0x3c', 'int 0x21', 'jc failure', 'mov word ptr [handle],ax',
         'mov word ptr [header],cs', 'mov word ptr [header+2],offset returned', f'mov word ptr [header+4],{len(rows)}',
         'mov bx,word ptr [handle]', 'mov dx,offset header', 'mov cx,6', 'mov ah,0x40', 'int 0x21',
         'xor ax,ax', 'mov es,ax', 'mov ax,word ptr es:[0x84]', 'mov word ptr [old21],ax', 'mov ax,word ptr es:[0x86]', 'mov word ptr [old21+2],ax',
         'again:', 'cli', 'cld', 'mov ax,cs', 'mov ds,ax', 'mov es,ax', 'mov al,0x5a', f'mov di,{LOW}', f'mov cx,{WINDOW}', 'rep stosb',
         'mov si,word ptr [index]', f'mov ax,{ROW}', 'mul si', 'mov si,ax', 'add si,offset table', 'mov word ptr [table_at],si',
         'mov word ptr [calls],0', 'mov di,offset events', 'mov cx,30', 'xor al,al', 'rep stosb',
         f'mov ax,{FREC}', 'mov es,ax', 'mov di,0x0100', 'mov cx,0x10', 'mov al,0x5a', 'rep stosb',
         'mov ax,word ptr [si+4]', 'mov word ptr es:[0x0100],ax', 'mov ax,word ptr [si+2]', 'mov word ptr es:[0x0102],ax',
         'mov ax,word ptr [si+6]', 'mov word ptr es:[0x0104],ax',
         f'mov ax,{DS}', 'mov es,ax', 'mov di,0x4420', 'mov cx,0x10', 'mov al,0x5a', 'rep stosb', 'mov word ptr es:[0x4426],0x7777',
         'xor ax,ax', 'mov es,ax', 'mov word ptr es:[0x84],offset int21', 'mov word ptr es:[0x86],cs',
         f'mov sp,{TOP}', f'push {FREC}', 'push 0x0100', 'push word ptr cs:[si]', f'mov ax,{DS}', 'mov ds,ax', 'mov ax,0x1111', 'mov es,ax',
         'mov ax,0x1234', 'mov bx,0x3456', 'mov cx,0x5678', 'mov dx,0x789a', 'mov si,0x9abc', 'mov di,0xbcde', 'mov bp,0x7777',
         'popf', f'lcall {CS},0x1593', 'returned:']
    t += [f'mov word ptr cs:[out+{2 * n}],{r}' for n, r in enumerate('ax bx cx dx si di bp sp cs'.split())]
    t += ['mov word ptr cs:[out+18],offset returned', 'mov word ptr cs:[out+20],ds', 'mov word ptr cs:[out+22],es',
          'mov word ptr cs:[out+24],ss', 'mov sp,0xf800', 'pushf', 'pop ax', 'mov word ptr cs:[out+26],ax', 'jmp tail',
          'diverr:'] + [f'mov word ptr cs:[out+{2 * n}],{r}' for n, r in enumerate('ax bx cx dx si di bp sp'.split())] + [
          f'mov word ptr cs:[out+16],{CS}', 'mov word ptr cs:[out+18],0x00e2', 'mov word ptr cs:[out+20],ds', 'mov word ptr cs:[out+22],es',
          'mov word ptr cs:[out+24],ss', 'mov sp,0xf800', 'pushf', 'pop ax', 'mov word ptr cs:[out+26],ax',
          'tail:', 'cld', 'mov ax,cs', 'mov ds,ax',
          'xor ax,ax', 'mov es,ax', 'mov ax,word ptr [old21]', 'mov word ptr es:[0x84],ax', 'mov ax,word ptr [old21+2]', 'mov word ptr es:[0x86],ax',
          'sti', 'mov bx,word ptr [handle]', 'mov dx,offset out', 'mov cx,28', 'mov ah,0x40', 'int 0x21',
          'mov bx,word ptr [handle]', 'mov dx,offset events', 'mov cx,30', 'mov ah,0x40', 'int 0x21',
          'mov bx,word ptr [handle]', f'mov dx,{LOW}', f'mov cx,{WINDOW}', 'mov ah,0x40', 'int 0x21',
          f'mov ax,{FREC}', 'mov ds,ax', 'mov bx,word ptr cs:[handle]', 'mov dx,0x0100', 'mov cx,0x10', 'mov ah,0x40', 'int 0x21',
          f'mov ax,{DS}', 'mov ds,ax', 'mov bx,word ptr cs:[handle]', 'mov dx,0x4420', 'mov cx,0x10', 'mov ah,0x40', 'int 0x21',
          'push cs', 'pop ds', 'inc word ptr [index]', f'cmp word ptr [index],{len(rows)}', 'jae done', 'jmp again', 'done:',
          'mov bx,word ptr [handle]', 'mov ah,0x3e', 'int 0x21', 'mov ax,0x4c00', 'int 0x21', 'failure:', 'mov ax,0x4c01', 'int 0x21',
          # Declared INT 21h hook: AH=42h records (return IP, AX, BX, CX, DX) and answers the case's next (DX:AX, carry),
          # CF edited in the IRET frame; three at most.
          'int21:', 'cmp ah,0x42', 'je d_seek', 'jmp dword ptr cs:[old21]',
          'd_seek:', 'mov word ptr cs:[save],bp', 'mov word ptr cs:[s_si],si', 'mov word ptr cs:[s_di],di', 'mov bp,sp',
          'mov di,word ptr cs:[calls]', 'cmp di,3', 'jae d_full', 'mov si,di', 'shl di,1', 'shl di,1', 'add di,word ptr cs:[calls]',
          'shl di,1', 'add di,offset events', 'mov si,word ptr [bp]', 'mov word ptr cs:[di],si', 'mov word ptr cs:[di+2],ax',
          'mov word ptr cs:[di+4],bx', 'mov word ptr cs:[di+6],cx', 'mov word ptr cs:[di+8],dx',
          'mov si,word ptr cs:[calls]', 'mov di,si', 'shl si,1', 'add si,di', 'shl si,1', 'add si,word ptr cs:[table_at]',
          'mov ax,word ptr cs:[si+8]', 'mov dx,word ptr cs:[si+10]', 'and word ptr [bp+4],0xfffe', 'cmp word ptr cs:[si+12],0', 'je d_ok',
          'or word ptr [bp+4],1', 'd_ok:', 'inc word ptr cs:[calls]', 'd_full:',
          'mov si,word ptr cs:[s_si]', 'mov di,word ptr cs:[s_di]', 'mov bp,word ptr cs:[save]', 'iret',
          'outname: .asciz "RESULT.BIN"', 'handle: .word 0', 'index: .word 0', 'save: .word 0', 's_si: .word 0', 's_di: .word 0',
          'calls: .word 0', 'table_at: .word 0', 'old21: .word 0,0', 'header: .space 6', 'out: .space 28', 'events: .space 30',
          '.align 2', 'table:']
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
    data = out.read_bytes(); record = 28 + 30 + WINDOW + 0x10 + 0x10
    check(len(data) == 6 + len(rows) * record, f'native length {len(data)}')
    seg, ret_ip, count = struct.unpack_from('<3H', data); check(count == len(rows), 'native count')
    stops = {}
    for n, (name, o) in enumerate(specs):
        rec = data[6 + n * record:6 + (n + 1) * record]
        m, c, script = build(pieces, **o)
        m = bytearray(m)
        m[physical(seg, LOW):physical(seg, LOW) + WINDOW] = b'\x5a' * WINDOW
        m[physical(FREC, 0x100):physical(FREC, 0x110)] = b'\x5a' * 0x10
        put16(m, FREC, 0x100, o.get('handle', 7)); put16(m, FREC, 0x102, o.get('magic', 0xD7B3)); put16(m, FREC, 0x104, o.get('recsize', 128))
        m[physical(DS, 0x4420):physical(DS, 0x4430)] = b'\x5a' * 0x10; put16(m, DS, 0x4426, 0x7777)
        sp = TOP - 0x08
        for j, w in enumerate((ret_ip, seg, 0x0100, FREC)):
            put16(m, seg, sp + 2 * j, w)
        c.ss, c.sp = seg, sp
        sps, events = [], []

        def svc(ctx, before, after, script=script):
            b = before[0]; sps.append(b.sp); events.append((b.ip + 2, b.ax, b.bx, b.cx, b.dx))
            after[0] = answer(b, script, len(events) - 1); return 0
        ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(m)
        status = STATUS[f(ram, SIZE, ctypes.byref(c), SERVICE_FN(svc), None)]
        check(status in ('RETURN', 'DIVIDE'), f'native C status {name}')
        stops[status] = stops.get(status, 0) + 1
        for (reg, _), want in zip(Cpu._fields_, struct.unpack_from('<14H', rec)):
            check(getattr(c, reg) == want, f'native CPU {name}/{reg}: C {getattr(c, reg):04x} DOS {want:04x}')
        native_events = [e for e in struct.iter_unpack('<5H', rec[28:58]) if any(e)]
        check(native_events == events, f'native LSEEK event log {name}: DOS {native_events} C {events}')
        mask = {physical(seg, (s - k) & 0xFFFF) for s in sps for k in range(1, 7)}
        window = bytes(ram[physical(seg, LOW):physical(seg, LOW) + WINDOW])
        for k in range(WINDOW):
            if physical(seg, LOW + k) not in mask:
                check(window[k] == rec[58 + k], f'native stack {name} at {LOW + k:04x}')
        check(bytes(ram[physical(FREC, 0x100):physical(FREC, 0x110)]) == rec[58 + WINDOW:58 + WINDOW + 0x10], f'native FileRec {name}')
        check(bytes(ram[physical(DS, 0x4420):physical(DS, 0x4430)]) == rec[58 + WINDOW + 0x10:], f'native InOutRes {name}')
    probe = subprocess.run([binary, '--version'], capture_output=True, text=True, env=dict(os.environ, SDL_VIDEODRIVER='dummy'))
    version = next(line.split(',')[0].strip() for line in (probe.stdout + probe.stderr).splitlines() if 'DOSBox-X version' in line)
    return dict(cases=len(rows), stops=stops, cpu_words=14, lseek_event_log=True, stack_bytes=WINDOW, filerec_bytes=0x10, data_bytes=0x10,
                flag_masks=0, result_sha256=sha256(data).hexdigest(),
                dosbox_x=dict(path=binary, sha256=sha256(open(binary, 'rb').read()).hexdigest(), version=version,
                              pinned_source='5a9c7c53481c2e933948039c33256776653f4faf', pinned_build_used=False))


def main():
    p = argparse.ArgumentParser()
    for flag in ('--compile-only', '--unicorn', '--native'):
        p.add_argument(flag, action='store_true')
    p.add_argument('--output', type=Path); a = p.parse_args()
    with tempfile.TemporaryDirectory(prefix='filepos-') as tmp:
        f = library(Path(tmp))
        if a.compile_only:
            print('CHECKER_BUILD_OK'); return
        pieces, decoded, own = source()
        report = dict(target=dict(spans=['0AF8:[1593,15AA)', '0AF8:[15DA,161F)', '0AF8:[0A76,0AE4) (composed: system_longdiv_0a76_cpu)'],
                                  bytes=23 + 69, instructions=9 + 32, sha256=digests(pieces)))
        if a.unicorn:
            report['unicorn'] = differential(pieces, decoded, own, f)
        if a.native:
            report['native'] = native(pieces, f)
    if a.output:
        a.output.write_text(json.dumps(report, sort_keys=True, indent=2) + '\n')
    print(json.dumps(report, sort_keys=True))


if __name__ == '__main__':
    main()
