#!/usr/bin/env python3
"""Turbo Pascal SYSTEM BlockRead 0AF8:14FB (raw CPU; runtime coverage, no identity) against the original bytes, the open
check 14B6 running as original bytes and composed in C from par-d's shared model. INT 21h AH=3Fh is the declared environment
(the case's carry, AX and the bytes it writes into the buffer, which the contract permits); any other AH or interrupt is
refused, except INT 0 from the DIV (the EXIT-class DIVIDE stop). Unicorn: CPU14 + full 1 MiB. Native (DOSBox-X): CPU14 + stack
+ FileRec + InOutRes + buffer + result, with declared INT 21h/INT 0 hooks and the interrupt frames masked. Media and
generated programs stay under ignored resources/."""
import argparse, ctypes, json, os, random, struct, subprocess, tempfile
from hashlib import sha256
from pathlib import Path
from capstone import Cs, CS_ARCH_X86, CS_MODE_16
from resident_loader_map import unpack
from res0709_ega_reset_0000_cpu_audit import Cpu, ROOT, SIZE, REGISTERS, physical, put16, check
CHECKER_ROWS = ('RES_0AF8_14FB',)
PIECES = ((0x14FB, 0x1563, 'e015b11ca2dff4700351966c73ece1b3153fb7221d80ced1a9f292f0c8aadf12', 45),
          (0x14B6, 0x14C5, '24d6d6fd5079eeb043c5f3a598502fa40ba66a4e12c919717ccdf847f362d363', 4))
EXCLUDED = {0x1502, 0x1504}   # the BlockWrite entry (MOV BL,40h; MOV CX,65h): a separate endpoint
CS, DS, SS, FREC, BUF, RES = 0x6000, 0x5000, 0x2000, 0x3000, 0x4000, 0x4800
FLAGS = (0x0002, 0x00D7, 0x08D7, 0x0402, 0x0893, 0x0003)
CONDITIONALS = {0x14BC: (0x14C4, 0x14BE), 0x1510: (0x1551, 0x1512), 0x1517: (0x1535, 0x1519), 0x152D: (0x154E, 0x152F),
                0x153C: (0x1543, 0x153E), 0x1546: (0x155F, 0x1548), 0x1558: (0x155F, 0x155A)}
SERVICE_FN = ctypes.CFUNCTYPE(ctypes.c_int, ctypes.c_void_p, ctypes.POINTER(Cpu), ctypes.POINTER(Cpu), ctypes.POINTER(ctypes.c_uint8),
                              ctypes.c_size_t)
STATUS = {0: 'RETURN', 1: 'ARGUMENT', 2: 'SERVICE', 3: 'ENV', 4: 'DIVIDE', 5: 'TRANSFER'}


def source():
    image = bytes(unpack((ROOT / 'resources/DOS/POOLRAD/START.EXE').read_bytes())[0])
    pieces, decoded = [], {}
    for start, end, digest, count in PIECES:
        body = image[physical(0xAF8, start):physical(0xAF8, end)]
        check(sha256(body).hexdigest() == digest, f'BlockRead piece {start:04x} identity')
        ins = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(body, start))
        check(len(ins) == count and sum(i.size for i in ins) == len(body), f'decode covers {start:04x}')
        pieces.append((start, body)); decoded.update({i.address: i for i in ins})
    check([a for a, i in decoded.items() if i.mnemonic == 'int'] == [0x1529], 'single INT 21h')
    check([a for a, i in decoded.items() if i.mnemonic in ('mul', 'div')] == [0x151B, 0x1531], 'one MUL, one DIV')
    check([a for a, i in decoded.items() if i.mnemonic == 'call'] == [0x150D], 'single CALL 14B6')
    check({a for a, i in decoded.items() if i.mnemonic.startswith('j') and i.mnemonic != 'jmp'} == set(CONDITIONALS), 'conditional sites')
    check(EXCLUDED <= set(decoded), 'BlockWrite entry decoded')
    return pieces, decoded


def library(directory):
    path = directory / 'blockread.so'
    subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Wpedantic', '-Werror', '-O2', '-fPIC', '-shared', '-I', str(ROOT / 'include'),
                    str(ROOT / 'src/recovered/system_blockread_14fb_cpu.c'), str(ROOT / 'src/recovered/system_filecheck_14b6_cpu.c'),
                    '-o', str(path)], check=True)
    f = ctypes.CDLL(str(path)).pr_system_blockread_14fb
    f.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t, ctypes.POINTER(Cpu), SERVICE_FN, ctypes.c_void_p]
    f.restype = ctypes.c_int
    return f


def pattern(seed, n):
    return bytes((seed + 0x25 * k) & 0xFF for k in range(n))


def build(pieces, *, magic=0xD7B3, handle=7, recsize=128, count=4, nil=False, cf=0, got=None, wlen=None, seed=0x31, data=None,
          f=(FREC, 0x0100), buf=(BUF, 0x0020), res=(RES, 0x0010), ss=SS, sp=0x8000, flags=2, ds=DS, regs=None):
    m = bytearray([0x5A]) * SIZE
    for start, body in pieces:
        at = physical(CS, start); m[at:at + len(body)] = body
    put16(m, ds, 0x4426, 0x7777); put16(m, res[0], res[1], 0xEEEE)
    seg, off = f
    put16(m, seg, off, handle); put16(m, seg, off + 2, magic); put16(m, seg, off + 4, recsize)
    rp = (0, 0) if nil else (res[1], res[0])
    for j, w in enumerate((0x7000, 0x9001, rp[0], rp[1], count, buf[1], buf[0], off, seg)):
        put16(m, ss, (sp + 2 * j) & 0xFFFF, w)
    r = regs or (0x1234, 0x3456, 0x5678, 0x789A, 0x9ABC, 0xBCDE, 0x7777)
    cx = (count * recsize) & 0xFFFF
    if cf:
        got = 5 if got is None else got
    got = cx if got is None else got
    if data is None:
        data = pattern(seed, (0 if cf else got) if wlen is None else wlen)
    check(len(data) <= cx or not count, 'declared write within CX')
    return m, Cpu(*r, sp, CS, 0x14FB, ds, 0x1111, ss, flags), dict(cf=cf, got=got, data=data)


def answer(b, script, write):
    """The declared read: writes the case's bytes into the permitted buffer, then on success AX := the bytes read (<= CX),
    CF clear; on error AX := the code, CF set."""
    check(len(script['data']) <= b.cx, 'declared write exceeds CX')
    for k, v in enumerate(script['data']):
        write(k, v)
    a = Cpu.from_buffer_copy(b)
    a.ax = script['got'] & 0xFFFF
    a.flags = (b.flags | 1) if script['cf'] else (b.flags & ~1)
    a.ip = 0x152B
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
        if ip == 0x1560:
            s['ret'] = True

    def intr(em, intno, user):
        if intno == 0:
            s['stop'] = 'DIVIDE'; return uc.emu_stop()
        ah = uc.reg_read(R.UC_X86_REG_AX) >> 8
        check(intno == 0x21 and ah == 0x3F, f'ENV refusal {name}: INT {intno:#x} AH {ah:#x}')
        b = Cpu(*[uc.reg_read(r) & 0xFFFF for r in REGISTERS]); log.append((b.ax >> 8, b.bx, b.cx, b.ds, b.dx))
        base = physical(b.ds, b.dx)
        a = answer(b, script, lambda k, v: backing.__setitem__((base + k) % SIZE, v))
        uc.reg_write(R.UC_X86_REG_AX, a.ax); uc.reg_write(R.UC_X86_REG_EFLAGS, a.flags)
    uc.hook_add(UC_HOOK_CODE, code); uc.hook_add(UC_HOOK_INTR, intr)
    uc.emu_start(physical(c.cs, c.ip), 2 * SIZE, count=1000)
    check(s['stop'] is not None, f'original did not stop {name}')
    clog = []

    def svc(ctx, before, after, buffer, length):
        b = before[0]; clog.append((b.ax >> 8, b.bx, b.cx, b.ds, b.dx)); check(length == b.cx, 'permitted length')
        after[0] = answer(b, script, lambda k, v: buffer.__setitem__(k, v)); return 0
    actual = Cpu.from_buffer_copy(c); ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(m)
    status = STATUS[f(ram, SIZE, ctypes.byref(actual), SERVICE_FN(svc), None)]
    check(status == s['stop'], f'C status {name}: {status} original {s["stop"]}')
    check(clog == log, f'read log {name}: C {clog} original {log}')
    if bytes(ram) != bytes(backing):
        diff = [hex(i) for i, (a, b) in enumerate(zip(bytes(ram), bytes(backing))) if a != b]
        raise ValueError(f'RAM {name}: {diff[:8]} ({len(diff)} bytes)')
    for (n, _), reg in zip(Cpu._fields_, REGISTERS):
        want = uc.reg_read(reg) & 0xFFFF
        check(getattr(actual, n) == want, f'CPU {name}/{n} C {getattr(actual, n):04x} original {want:04x}')
    return seen, edges, bytes(ram), log, status, actual


def expected(o, script):
    """Independent statement of BlockRead's effect: (status, read log, InOutRes, *result or None)."""
    magic, recsize, count, nil = o.get('magic', 0xD7B3), o.get('recsize', 128), o.get('count', 4), o.get('nil', False)
    handle, buf = o.get('handle', 7), o.get('buf', (BUF, 0x0020))
    if magic != 0xD7B3:
        return 'RETURN', [], 103, None if nil else 0
    if count == 0:
        return 'RETURN', [], 0x7777, None if nil else 0
    log = [(0x3F, handle, (count * recsize) & 0xFFFF, buf[0], buf[1])]
    if script['cf']:
        return 'RETURN', log, script['got'], None if nil else 0
    if recsize == 0:
        return 'DIVIDE', log, 0x7777, None if nil else 0xEEEE
    q = script['got'] // recsize
    if nil:
        return 'RETURN', log, 0x7777 if q == count else 100, None
    return 'RETURN', log, 0x7777, q


def cases():
    rng = random.Random(0x0AF814FB)
    out = [('read4', dict()), ('short300', dict(got=300)), ('short256', dict(got=256)), ('short1', dict(got=1)), ('eof0', dict(got=0)),
           ('nil-full', dict(nil=True)), ('nil-short', dict(nil=True, got=300)), ('nil-eof', dict(nil=True, got=0)),
           ('count0', dict(count=0)), ('count0-nil', dict(count=0, nil=True)), ('count0-rec0', dict(count=0, recsize=0)),
           # the LOW WORD of count * recsize, the overflow DROPPED
           ('overflow-zero', dict(count=0x200, recsize=0x100)), ('overflow-drop', dict(count=3, recsize=0x6000, wlen=0x40)),
           ('max', dict(count=0xFFFF, recsize=0xFFFF)), ('recsize1', dict(recsize=1, count=1000, wlen=0x80)),
           ('recsize-ffff', dict(recsize=0xFFFF, count=1, wlen=0x40)),
           ('read-error', dict(cf=1, got=5, wlen=3)), ('read-error-6', dict(cf=1, got=6)), ('read-error-nil', dict(cf=1, nil=True)),
           ('not-open', dict(magic=0xD7B0)), ('unassigned', dict(magic=0x1234)), ('not-open-nil', dict(magic=0xD7B0, nil=True)),
           # DIV by recsize 0 -> INT 0 (RunError 200), EXIT-class
           ('divide', dict(recsize=0, count=3)), ('divide-nil', dict(recsize=0, count=3, nil=True)),
           ('divide-read-error', dict(recsize=0, count=3, cf=1))]
    out += [(f'flags{fl:x}', dict(flags=fl)) for fl in FLAGS] + [(f'flags{fl:x}-closed', dict(flags=fl, magic=0xD7B0)) for fl in FLAGS]
    out += [(f'flags{fl:x}-divide', dict(flags=fl, recsize=0, count=1)) for fl in FLAGS]
    out += [(f'sp{sp:x}', dict(sp=sp)) for sp in (0x0006, 0x0007, 0x8001, 0xFFED, 0xFFEE)]
    out += [('ss-ds', dict(ss=DS)), ('a20-stack', dict(ss=0xFFFF, sp=0x0400)), ('frec-straddle', dict(f=(FREC, 0xFFFD))),
            ('frec-a20', dict(f=(0xFFFF, 0x0040))), ('buf-straddle', dict(buf=(BUF, 0xFFF0), count=1, recsize=0x40)),
            ('buf-a20', dict(buf=(0xFFFF, 0x0008), count=1, recsize=0x40)), ('res-straddle', dict(res=(RES, 0xFFFF))),
            # the read rereads what it wrote: recsize zeroed through the FileRec (DIVIDE), recsize changed, count, result pointer,
            # the return address, InOutRes
            ('alias-buf-frec-divide', dict(buf=(FREC, 0x0100), count=1, recsize=6, data=bytes([7, 0, 0xB3, 0xD7, 0, 0]))),
            ('alias-buf-frec-recsize', dict(buf=(FREC, 0x0100), count=1, recsize=6, data=bytes([7, 0, 0xB3, 0xD7, 2, 0]))),
            ('alias-buf-count', dict(buf=(SS, 0x8008), nil=True, count=4, wlen=0, data=bytes([6, 0]))),
            ('alias-buf-result', dict(buf=(SS, 0x8004), data=bytes([0x30, 0, 0, 0x48]))),
            ('alias-buf-return', dict(buf=(SS, 0x8000), data=bytes([0x34, 0x12, 0x00, 0x70]))),
            ('alias-buf-inoutres', dict(buf=(DS, 0x4420), nil=True, got=300, wlen=16)),
            ('alias-result-on-buf', dict(res=(BUF, 0x0020))), ('alias-result-on-frec', dict(res=(FREC, 0x0104))),
            # the check's InOutRes store on its own return slot (SS = DS, entry SP 442Ah)
            ('transfer', dict(ss=DS, sp=0x442A, magic=0xD7B0)), ('ss-ds-slot-open', dict(ss=DS, sp=0x442A))]
    for k in range(40):
        recsize, count = rng.choice((0, 1, 128, rng.randrange(65536))), rng.choice((0, 1, rng.randrange(65536)))
        cx = (count * recsize) & 0xFFFF
        out.append((f'random{k}', dict(magic=rng.choice((0xD7B3, 0xD7B3, 0xD7B0, rng.randrange(65536))), handle=rng.randrange(65536),
                                       recsize=recsize, count=count, nil=rng.randrange(4) == 0, cf=rng.randrange(3) == 0,
                                       got=rng.randrange(cx + 1), wlen=rng.randrange(min(cx, 0x200) + 1), seed=rng.randrange(256),
                                       flags=rng.choice(FLAGS), regs=tuple(rng.randrange(65536) for _ in range(7)))))
    return out


def differential(pieces, decoded, f):
    seen, edges = set(), set(); specs = cases(); checked = 0; stops = {}
    for name, o in specs:
        state = build(pieces, **o)
        v, e, ram, log, status, cpu = compare(name, state, decoded, f); seen |= v; edges |= e
        stops[status] = stops.get(status, 0) + 1
        if name.startswith(('alias', 'transfer', 'ss-ds-slot', 'buf-', 'res-', 'frec-')):
            continue
        st, want_log, inout, result = expected(o, state[2])
        res = o.get('res', (RES, 0x0010))
        got_inout = ram[physical(o.get('ds', DS), 0x4426)] | ram[physical(o.get('ds', DS), 0x4427)] << 8
        check(status == st and log == want_log, f'{name}: status/log {status} {log} vs {st} {want_log}')
        check(got_inout == inout, f'{name}: InOutRes {got_inout:04x} vs {inout:04x}')
        word = ram[physical(*res)] | ram[physical(res[0], res[1] + 1)] << 8
        check(word == (0xEEEE if result is None else result), f'{name}: result {word:04x} vs {result}')
        if st == 'RETURN' and log and not state[2]['cf']:
            base = physical(*o.get('buf', (BUF, 0x0020)))
            check(bytes(ram[base:base + len(state[2]['data'])]) == state[2]['data'], f'{name}: buffer holds the read')
        if st == 'DIVIDE':
            check(cpu.ip == 0x1531 and cpu.dx == 0, f'{name}: DIVIDE at the DIV with DX 0')
        checked += 1
    check('TRANSFER' in stops and 'DIVIDE' in stops, 'named stops reached')
    check(not seen & EXCLUDED, 'BlockWrite entry never executed')
    check(seen == set(decoded) - EXCLUDED, f'starts missing {sorted(hex(a) for a in set(decoded) - EXCLUDED - seen)}')
    want = {(a, t) for a, ts in CONDITIONALS.items() for t in ts}
    check(edges == want, f'outcomes missing {sorted(want - edges)}')
    return dict(cases=len(specs), stops=stops, starts=len(seen), starts_total=len(decoded) - len(EXCLUDED), body_starts=len({a for a in seen if a >= 0x14FB}), helper_starts=len({a for a in seen if a < 0x14FB}), excluded_starts=sorted(EXCLUDED),
                conditional_outcomes=len(edges), conditional_outcomes_total=len(want), independent_checks=checked,
                cpu_words=14, ram_bytes=SIZE, flag_masks=0)


TOP, LOW, WINDOW, BUFLEN, RESLEN = 0xF000, 0xEC00, 0x0400, 0x0400, 0x0020


def native_specs():
    return [(n, o) for n, o in cases() if not n.startswith(('sp', 'a20', 'ss-ds', 'random', 'alias', 'frec-', 'buf-', 'res-', 'transfer'))]


def native(pieces, f):
    directory = ROOT / 'resources/system-blockread/native'; directory.mkdir(parents=True, exist_ok=True)
    for start, body in pieces:
        (directory / f'piece{start:04x}.bin').write_bytes(body)
    specs = native_specs(); rows = []
    for _, o in specs:
        m, c, script = build(pieces, **o)
        data = script['data']
        check(data == pattern(o.get('seed', 0x31), len(data)) and len(data) <= BUFLEN - 0x20, 'native writes are the seed pattern')
        rows.append((o.get('flags', 2), o.get('magic', 0xD7B3), o.get('handle', 7), o.get('recsize', 128), o.get('count', 4),
                     int(o.get('nil', False)), script['cf'], script['got'], len(data), o.get('seed', 0x31)))
    ROW = 2 * len(rows[0])
    copy = []
    for start, body in pieces:
        copy += [f'mov di,{start}', f'mov si,offset piece{start:04x}', f'mov cx,{len(body)}', 'rep movsb']

    def fill(seg, off, n):
        return [f'mov ax,{seg}', 'mov es,ax', f'mov di,{off}', f'mov cx,{n}', 'mov al,0x5a', 'rep stosb']

    def dump(seg, off, n):
        return [f'mov ax,{seg}', 'mov ds,ax', 'mov bx,word ptr cs:[handle]', f'mov dx,{off}', f'mov cx,{n}', 'mov ah,0x40', 'int 0x21']
    t = ['.code16', '.intel_syntax noprefix', '.global _start', '_start: jmp main', '.org 0x200', 'main:',
         'mov ax,cs', 'mov ds,ax', 'mov ss,ax', f'mov sp,{TOP}', 'cld', f'mov ax,{CS}', 'mov es,ax'] + copy + [
         'mov dx,offset outname', 'xor cx,cx', 'mov ah,0x3c', 'int 0x21', 'jc failure', 'mov word ptr [handle],ax',
         'mov word ptr [header],cs', 'mov word ptr [header+2],offset returned', f'mov word ptr [header+4],{len(rows)}',
         'mov bx,word ptr [handle]', 'mov dx,offset header', 'mov cx,6', 'mov ah,0x40', 'int 0x21',
         'xor ax,ax', 'mov es,ax', 'mov ax,word ptr es:[0x84]', 'mov word ptr [old21],ax', 'mov ax,word ptr es:[0x86]', 'mov word ptr [old21+2],ax',
         'mov ax,word ptr es:[0]', 'mov word ptr [old00],ax', 'mov ax,word ptr es:[2]', 'mov word ptr [old00+2],ax',
         'again:', 'cli', 'cld', 'mov ax,cs', 'mov ds,ax', 'mov es,ax', 'mov al,0x5a', f'mov di,{LOW}', f'mov cx,{WINDOW}', 'rep stosb',
         'mov si,word ptr [index]', f'mov ax,{ROW}', 'mul si', 'mov si,ax', 'add si,offset table']
    t += fill(FREC, 0x0100, 0x10) + ['mov ax,word ptr [si+4]', 'mov word ptr es:[0x0100],ax', 'mov ax,word ptr [si+2]',
                                     'mov word ptr es:[0x0102],ax', 'mov ax,word ptr [si+6]', 'mov word ptr es:[0x0104],ax']
    t += fill(DS, 0x4420, 0x10) + ['mov word ptr es:[0x4426],0x7777']
    t += fill(BUF, 0, BUFLEN) + fill(RES, 0, RESLEN) + ['mov word ptr es:[0x0010],0xeeee']
    t += ['mov ax,word ptr [si+12]', 'mov word ptr [s_cf],ax', 'mov ax,word ptr [si+14]', 'mov word ptr [s_ax],ax',
          'mov ax,word ptr [si+16]', 'mov word ptr [s_len],ax', 'mov ax,word ptr [si+18]', 'mov word ptr [s_seed],ax',
          'xor ax,ax', 'mov es,ax', 'mov word ptr es:[0x84],offset int21', 'mov word ptr es:[0x86],cs',
          'mov word ptr es:[0],offset div0', 'mov word ptr es:[2],cs',
          f'mov sp,{TOP}', f'push {FREC}', 'push 0x0100', f'push {BUF}', 'push 0x0020', 'push word ptr cs:[si+8]',
          'cmp word ptr cs:[si+10],0', 'jne nilres', f'push {RES}', 'push 0x0010', 'jmp pushed', 'nilres:', 'push 0', 'push 0', 'pushed:',
          'push word ptr cs:[si]', f'mov ax,{DS}', 'mov ds,ax', 'mov ax,0x1111', 'mov es,ax',
          'mov ax,0x1234', 'mov bx,0x3456', 'mov cx,0x5678', 'mov dx,0x789a', 'mov si,0x9abc', 'mov di,0xbcde', 'mov bp,0x7777',
          'popf', f'lcall {CS},0x14fb', 'returned:']
    t += [f'mov word ptr cs:[out+{2 * n}],{r}' for n, r in enumerate('ax bx cx dx si di bp sp cs'.split())]
    t += ['mov word ptr cs:[out+18],offset returned', 'mov word ptr cs:[out+20],ds', 'mov word ptr cs:[out+22],es',
          'mov word ptr cs:[out+24],ss', 'mov sp,0xf800', 'pushf', 'pop ax', 'mov word ptr cs:[out+26],ax', 'jmp tail',
          # INT 0 capture (the DIVIDE stop): registers as at the fault, CS:IP and FLAGS from the frame, SP above it.
          'div0:'] + [f'mov word ptr cs:[out+{2 * n}],{r}' for n, r in enumerate('ax bx cx dx si di bp'.split())] + [
          'mov bp,sp', 'mov ax,word ptr [bp]', 'mov word ptr cs:[out+18],ax', 'mov ax,word ptr [bp+2]', 'mov word ptr cs:[out+16],ax',
          'mov ax,word ptr [bp+4]', 'mov word ptr cs:[out+26],ax', 'lea ax,[bp+6]', 'mov word ptr cs:[out+14],ax',
          'mov word ptr cs:[out+20],ds', 'mov word ptr cs:[out+22],es', 'mov word ptr cs:[out+24],ss', 'mov sp,0xf800',
          'tail:', 'cld', 'mov ax,cs', 'mov ds,ax',
          'xor ax,ax', 'mov es,ax', 'mov ax,word ptr [old21]', 'mov word ptr es:[0x84],ax', 'mov ax,word ptr [old21+2]', 'mov word ptr es:[0x86],ax',
          'mov ax,word ptr [old00]', 'mov word ptr es:[0],ax', 'mov ax,word ptr [old00+2]', 'mov word ptr es:[2],ax',
          'sti', 'mov bx,word ptr [handle]', 'mov dx,offset out', 'mov cx,28', 'mov ah,0x40', 'int 0x21',
          'mov bx,word ptr [handle]', f'mov dx,{LOW}', f'mov cx,{WINDOW}', 'mov ah,0x40', 'int 0x21']
    t += dump(FREC, 0x0100, 0x10) + dump(DS, 0x4420, 0x10) + dump(BUF, 0, BUFLEN) + dump(RES, 0, RESLEN)
    t += ['push cs', 'pop ds', 'inc word ptr [index]', f'cmp word ptr [index],{len(rows)}', 'jae done', 'jmp again', 'done:',
          'mov bx,word ptr [handle]', 'mov ah,0x3e', 'int 0x21', 'mov ax,0x4c00', 'int 0x21', 'failure:', 'mov ax,0x4c01', 'int 0x21',
          # Declared INT 21h hook: AH=3Fh writes the case's s_len pattern bytes at DS:DX, then AX and CF (in the IRET frame).
          'int21:', 'cmp ah,0x3f', 'je d_read', 'jmp dword ptr cs:[old21]',
          'd_read:', 'mov word ptr cs:[save],bp', 'mov bp,sp', 'and word ptr [bp+4],0xfffe',
          'mov word ptr cs:[s_es],es', 'mov word ptr cs:[s_di],di', 'mov word ptr cs:[s_cx],cx',
          'mov ax,ds', 'mov es,ax', 'mov di,dx', 'mov cx,word ptr cs:[s_len]', 'mov al,byte ptr cs:[s_seed]', 'cld', 'jcxz w_done',
          'w_loop:', 'stosb', 'add al,0x25', 'loop w_loop', 'w_done:',
          'mov es,word ptr cs:[s_es]', 'mov di,word ptr cs:[s_di]', 'mov cx,word ptr cs:[s_cx]', 'mov ax,word ptr cs:[s_ax]',
          'cmp word ptr cs:[s_cf],0', 'je d_ok', 'or word ptr [bp+4],1', 'd_ok:', 'mov bp,word ptr cs:[save]', 'iret',
          'outname: .asciz "RESULT.BIN"', 'handle: .word 0', 'index: .word 0', 'save: .word 0', 's_cf: .word 0', 's_ax: .word 0',
          's_len: .word 0', 's_seed: .word 0', 's_es: .word 0', 's_di: .word 0', 's_cx: .word 0', 'old21: .word 0,0', 'old00: .word 0,0',
          'header: .space 6', 'out: .space 28', '.align 2', 'table:']
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
    data = out.read_bytes(); record = 28 + WINDOW + 0x10 + 0x10 + BUFLEN + RESLEN
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
        m[physical(BUF, 0):physical(BUF, BUFLEN)] = b'\x5a' * BUFLEN
        m[physical(RES, 0):physical(RES, RESLEN)] = b'\x5a' * RESLEN; put16(m, RES, 0x0010, 0xEEEE)
        sp = TOP - 0x12; rp = (0, 0) if o.get('nil') else (0x0010, RES)
        for j, w in enumerate((ret_ip, seg, rp[0], rp[1], o.get('count', 4), 0x0020, BUF, 0x0100, FREC)):
            put16(m, seg, sp + 2 * j, w)
        c.ss, c.sp = seg, sp
        sps = []

        def svc(ctx, before, after, buffer, length, script=script):
            sps.append(before[0].sp); after[0] = answer(before[0], script, lambda k, v: buffer.__setitem__(k, v)); return 0
        ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(m)
        status = STATUS[f(ram, SIZE, ctypes.byref(c), SERVICE_FN(svc), None)]
        check(status in ('RETURN', 'DIVIDE'), f'native C status {name}')
        stops[status] = stops.get(status, 0) + 1
        if status == 'DIVIDE':
            sps.append(c.sp)    # the INT 0 frame below the faulting SP
        for (reg, _), want in zip(Cpu._fields_, struct.unpack_from('<14H', rec)):
            check(getattr(c, reg) == want, f'native CPU {name}/{reg}: C {getattr(c, reg):04x} DOS {want:04x}')
        mask = {physical(seg, (s - k) & 0xFFFF) for s in sps for k in range(1, 7)}
        window = bytes(ram[physical(seg, LOW):physical(seg, LOW) + WINDOW])
        for k in range(WINDOW):
            if physical(seg, LOW + k) not in mask:
                check(window[k] == rec[28 + k], f'native stack {name} at {LOW + k:04x}')
        at = 28 + WINDOW
        for label, s0, o0, size in (('FileRec', FREC, 0x100, 0x10), ('InOutRes', DS, 0x4420, 0x10), ('buffer', BUF, 0, BUFLEN),
                                    ('result', RES, 0, RESLEN)):
            check(bytes(ram[physical(s0, o0):physical(s0, o0) + size]) == rec[at:at + size], f'native {label} {name}')
            at += size
    probe = subprocess.run([binary, '--version'], capture_output=True, text=True, env=dict(os.environ, SDL_VIDEODRIVER='dummy'))
    version = next(line.split(',')[0].strip() for line in (probe.stdout + probe.stderr).splitlines() if 'DOSBox-X version' in line)
    return dict(cases=len(rows), stops=stops, cpu_words=14, stack_bytes=WINDOW, filerec_bytes=0x10, data_bytes=0x10, buffer_bytes=BUFLEN,
                result_bytes=RESLEN, flag_masks=0, result_sha256=sha256(data).hexdigest(),
                dosbox_x=dict(path=binary, sha256=sha256(open(binary, 'rb').read()).hexdigest(), version=version,
                              pinned_source='5a9c7c53481c2e933948039c33256776653f4faf', pinned_build_used=False))


def main():
    p = argparse.ArgumentParser()
    for flag in ('--compile-only', '--unicorn', '--native'):
        p.add_argument(flag, action='store_true')
    p.add_argument('--output', type=Path); a = p.parse_args()
    with tempfile.TemporaryDirectory(prefix='blockread-') as tmp:
        f = library(Path(tmp))
        if a.compile_only:
            print('CHECKER_BUILD_OK'); return
        pieces, decoded = source()
        report = dict(target=dict(spans=['0AF8:[14FB,1563)', '0AF8:[14B6,14C5) (composed: par-d system_filecheck_14b6_cpu)'],
                                  bytes=104, instructions=45, sha256=[d for _, _, d, _ in PIECES]))
        if a.unicorn:
            report['unicorn'] = differential(pieces, decoded, f)
        if a.native:
            report['native'] = native(pieces, f)
    if a.output:
        a.output.write_text(json.dumps(report, sort_keys=True, indent=2) + '\n')
    print(json.dumps(report, sort_keys=True))


if __name__ == '__main__':
    main()
