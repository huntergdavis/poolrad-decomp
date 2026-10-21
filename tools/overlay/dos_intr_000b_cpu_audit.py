#!/usr/bin/env python3
"""Turbo Pascal Dos.Intr 0A32:000B (raw CPU, runtime coverage, no identity claim) against the original bytes.

The interrupt handler is the declared environment input. Both oracles run four small declared stub handlers
(real x86 bytes placed at the vector); the C model's service reproduces each stub's effect exactly.
Unicorn: CPU14 + full 1 MiB. One declared split: AF at handler entry (SHL BX,1 leaves AF undefined;
DOSBox-X gives 1, Unicorn 0, the model follows DOSBox-X), visible only to stub C, which stores its entry
flags into AX; the split is asserted to be exactly bit 4 of the stored Regs.AX low byte. Native (DOSBox-X):
user vectors only, interrupts off, CPU14 + the 1 KiB stack window + the 64-byte data region compared
exactly. Media and generated programs stay under ignored resources/."""
import argparse, ctypes, json, os, random, struct, subprocess, tempfile
from hashlib import sha256
from pathlib import Path
from capstone import Cs, CS_ARCH_X86, CS_MODE_16
from resident_loader_map import unpack
from res0709_ega_reset_0000_cpu_audit import Cpu, ROOT, SIZE, REGISTERS, physical, put16, check
CHECKER_ROWS = ('RES_0A32_000B',)
ENTRY, END, TRANSFER_IP, RETURN_IP, CONTINUE_IP = 0x000B, 0x006C, 0x0044, 0x0069, 0x0045
DIGEST = '08854b63986cc7a20465b9594ceb0ac8d86c7467dccdcf49d894a04b79dccde9'
CS, DS, SS, DATA, STUB_SEG = 0x6000, 0x5000, 0x2000, 0x3000, 0x7800
FLAGS = (0x0002, 0x00D7, 0x08D7, 0x0402, 0x0893, 0x0003)
SERVICE_FN = ctypes.CFUNCTYPE(ctypes.c_int, ctypes.c_void_p, ctypes.POINTER(ctypes.c_uint8), ctypes.POINTER(Cpu),
                              ctypes.POINTER(Cpu))
# Declared stub handlers (bytes, offset in STUB_SEG). Their effects are reproduced by stub_effect().
STUBS = {
    'A': (bytes.fromhex('355a5a' '01cb' '268915' 'f7d6' 'bd3412' '061f' 'cf'), 0x0000),  # xor ax,5A5Ah; add bx,cx; mov es:[di],dx; not si; mov bp,1234h; push es; pop ds; iret
    'B': (bytes.fromhex('01c8' 'f9' 'ca0200'), 0x0020),                                # add ax,cx; stc; retf 2 (live flags)
    'C': (bytes.fromhex('9c' '58' 'cf'), 0x0040),                                      # pushf; pop ax; iret (observes entry flags)
    'D': (bytes.fromhex('50' '53' '58' '5b' 'cf'), 0x0060),                            # push ax; push bx; pop ax; pop bx; iret
}
REGS = (0x0A0A, 0x0B0B, 0x0C0C, 0x0D0D, 0x0E0E, 0x0F0F, 0x0120, DATA, DATA, 0xFFFF)  # AX BX CX DX BP SI DI DS ES FLAGS


def source():
    image = bytes(unpack((ROOT / 'resources/DOS/POOLRAD/START.EXE').read_bytes())[0])
    body = image[physical(0xA32, ENTRY):physical(0xA32, END)]
    check(sha256(body).hexdigest() == DIGEST, 'Intr identity')
    ins = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(body, ENTRY))
    check(sum(i.size for i in ins) == len(body), 'decode covers the span')
    rets = [(i.address, i.op_str) for i in ins if i.mnemonic in ('ret', 'retf', 'iret')]
    check(rets == [(TRANSFER_IP, ''), (RETURN_IP, '6')], f'returns {rets}')
    check(ins[[i.address for i in ins].index(TRANSFER_IP) + 1].address == CONTINUE_IP, 'continuation follows the transfer')
    check(not [i for i in ins if i.mnemonic in ('int', 'call', 'lcall') or i.mnemonic.startswith('j')], 'no INT, call or branch')
    return body, {i.address: i for i in ins}


def library(directory):
    path = directory / 'intr.so'
    subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Wpedantic', '-Werror', '-O2', '-fPIC', '-shared', '-I', str(ROOT / 'include'),
                    str(ROOT / 'src/recovered/dos_intr_000b_cpu.c'), '-o', str(path)], check=True)
    f = ctypes.CDLL(str(path)).pr_dos_intr_000b
    f.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t, ctypes.POINTER(Cpu), SERVICE_FN, ctypes.c_void_p]
    f.restype = ctypes.c_int
    return f


def rd(ram, seg, off):
    at = physical(seg, off & 0xFFFF)
    return ram[at] | ram[(at + 1) % SIZE] << 8


def wr(ram, seg, off, v):
    at = physical(seg, off & 0xFFFF)
    ram[at] = v & 255; ram[(at + 1) % SIZE] = (v >> 8) & 255


def parity(v):
    return 0x0004 if bin(v & 255).count('1') % 2 == 0 else 0


def stub_effect(kind, ram, b):
    """The declared handler's effect, from the handler-entry CPU b, returning the CPU at its return."""
    a = Cpu.from_buffer_copy(b)
    if kind == 'A':
        a.ax ^= 0x5A5A; a.bx = (a.bx + a.cx) & 0xFFFF; wr(ram, a.es, a.di, a.dx); a.si ^= 0xFFFF; a.bp = 0x1234; a.ds = a.es
    elif kind == 'B':
        r = a.ax + a.cx; res = r & 0xFFFF
        of = 0x0800 if (~(a.ax ^ a.cx) & (a.ax ^ res)) & 0x8000 else 0
        fl = 0x0001 | parity(res) | ((a.ax ^ a.cx ^ res) & 0x10) | (0x0040 if res == 0 else 0) | (0x0080 if res & 0x8000 else 0) | of
        a.ax = res; a.flags = (a.flags & ~0x08D5) | fl
        a.ip = rd(ram, a.ss, a.sp); a.cs = rd(ram, a.ss, a.sp + 2); a.sp = (a.sp + 6) & 0xFFFF
        return a
    elif kind == 'C':
        wr(ram, a.ss, a.sp - 2, a.flags); a.ax = a.flags
    elif kind == 'D':
        wr(ram, a.ss, a.sp - 2, a.ax); wr(ram, a.ss, a.sp - 4, a.bx); a.ax, a.bx = a.bx, a.ax
    a.ip = rd(ram, a.ss, a.sp); a.cs = rd(ram, a.ss, a.sp + 2); a.flags = rd(ram, a.ss, a.sp + 4); a.sp = (a.sp + 6) & 0xFFFF
    return a


def make_service(kinds_by_address):
    def svc(ctx, ram, before, after):
        kind = kinds_by_address.get((before[0].cs, before[0].ip))
        if kind is None:
            return 1
        after[0] = stub_effect(kind, ram, before[0])
        return 0
    return SERVICE_FN(svc)


def build(body, *, intno=0x61, stub='A', ss=SS, sp=0x8000, flags=2, ds=DS, regs_ptr=(DATA, 0x0100), regs=REGS,
          cpu=None, intno_high=0xA5):
    m = bytearray([0x5A]) * SIZE
    at = physical(CS, ENTRY); m[at:at + len(body)] = body
    for j, w in enumerate(regs):  # first: the vector and the frame below may overlay the record (regs-on-ivt, alias cases)
        put16(m, regs_ptr[0], (regs_ptr[1] + 2 * j) & 0xFFFF, w)
    for code, off in STUBS.values():
        s = physical(STUB_SEG, off); m[s:s + len(code)] = code
    put16(m, 0, intno * 4, STUBS[stub][1]); put16(m, 0, intno * 4 + 2, STUB_SEG)
    for j, w in enumerate((0x7000, 0x9001, regs_ptr[1], regs_ptr[0], intno_high << 8 | intno)):
        put16(m, ss, (sp + 2 * j) & 0xFFFF, w)
    r = cpu or (0x1234, 0x3456, 0x5678, 0x789A, 0x9ABC, 0xBCDE, 0x7777)
    c = Cpu(*r, sp, CS, ENTRY, ds, 0x1111, ss, flags)
    return m, c, stub


STUB_ADDRESSES = {(STUB_SEG, off): kind for kind, (_, off) in STUBS.items()}


def compare(name, state, decoded, f):
    from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE, UC_HOOK_INTR
    from unicorn import x86_const as R
    m, c, stub = state
    uc = Uc(UC_ARCH_X86, UC_MODE_16); backing = (ctypes.c_uint8 * SIZE).from_buffer_copy(m)
    uc.mem_map_ptr(0, SIZE, 7, ctypes.addressof(backing)); uc.mem_map_ptr(SIZE, SIZE, 7, ctypes.addressof(backing))
    for (n, _), reg in zip(Cpu._fields_, REGISTERS):
        uc.reg_write(reg, getattr(c, n))
    stub_ips = {cs_ip: {i.address for i in Cs(CS_ARCH_X86, CS_MODE_16).disasm(STUBS[k][0], cs_ip[1])}
                for cs_ip, k in STUB_ADDRESSES.items()}
    s = dict(stop=None, ret=False, handler=0); seen = set()

    def code(em, address, size, user):
        ip = uc.reg_read(R.UC_X86_REG_IP); cs = uc.reg_read(R.UC_X86_REG_CS)
        if s['ret']:
            s['stop'] = 0; return uc.emu_stop()
        if cs == CS and ip in decoded:
            seen.add(ip)
            if ip == RETURN_IP:
                s['ret'] = True
            return
        check(cs == STUB_SEG and any(ip in v for v in stub_ips.values()), f'escape {name} {cs:04x}:{ip:04x}')
        if ip == STUBS[stub][1]:
            s['handler'] += 1

    def intr(em, intno, user):
        raise ValueError(f'unexpected interrupt {intno:#x} in {name}')
    uc.hook_add(UC_HOOK_CODE, code); uc.hook_add(UC_HOOK_INTR, intr)
    uc.emu_start(physical(c.cs, c.ip), 2 * SIZE, count=1000)
    check(s['stop'] == 0 and s['handler'] == 1, f'original did not run the handler once and return {name}')
    actual = Cpu.from_buffer_copy(c); ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(m)
    status = f(ram, SIZE, ctypes.byref(actual), make_service(STUB_ADDRESSES), None)
    check(status == 0, f'C status {name}: {status}')
    diff = [i for i, (a, b) in enumerate(zip(bytes(ram), bytes(backing))) if a != b]
    for (n, _), reg in zip(Cpu._fields_, REGISTERS):
        want = uc.reg_read(reg) & 0xFFFF
        check(getattr(actual, n) == want, f'CPU {name}/{n} C {getattr(actual, n):04x} original {want:04x}')
    return seen, actual, bytes(ram), bytes(backing), diff


def cases():
    rng = random.Random(0x0A32000B)
    out = [(f'int{n:02x}', dict(intno=n)) for n in range(256)]
    out += [(f'stub{k}-int{n:02x}', dict(stub=k, intno=n)) for k in 'BCD' for n in (0x00, 0x10, 0x21, 0x61, 0x80, 0xFF)]
    out += [(f'flags{f:x}-{k}', dict(flags=f, stub=k)) for f in FLAGS + (0x0202,) for k in 'ABCD']
    out += [(f'sp{sp:x}', dict(sp=sp)) for sp in (0x0014, 0x0015, 0x8001, 0xFFF5, 0xFFF6)]
    out += [('ss-ds', dict(ss=DS)), ('a20-stack', dict(ss=0xFFFF, sp=0x0400)),
            ('regs-straddle-ffff', dict(regs_ptr=(DATA, 0xFFF1), regs=REGS[:6] + (0xFFFF,) + REGS[7:])),
            ('regs-wrap-even', dict(regs_ptr=(DATA, 0xFFF0), regs=REGS[:6] + (0xFFF8,) + REGS[7:])),
            ('regs-a20', dict(regs_ptr=(0xFFFF, 0x0020), regs=REGS[:7] + (0xFFFF, 0xFFFF, 0))),
            # Regs is the interrupt vector table entry itself: the record is read after the vector.
            ('regs-on-ivt', dict(intno=0x61, regs_ptr=(0, 0x0184))),
            # Regs over the saved BP and the return address (entry SP 8000h): Regs.BX is the return IP, stub A adds CX
            # (the return CS) into BX, and the BX store at 0045h rewrites the return IP the RETF 6 then uses.
            ('alias-regs-on-frame', dict(regs_ptr=(SS, 0x7FFE))),
            ('intno-high-byte-zero', dict(intno_high=0))]
    for k in range(32):
        out.append((f'random{k}', dict(intno=rng.randrange(256), stub=rng.choice('ABD'), flags=rng.choice(FLAGS),
                                       cpu=tuple(rng.randrange(65536) for _ in range(7)),
                                       regs=tuple(rng.randrange(65536) for _ in range(6)) + (rng.randrange(0x0100, 0xFF00) & ~1, DATA,
                                                                                            rng.choice((DATA, 0x4000)), rng.randrange(65536)),
                                       regs_ptr=(rng.choice((DATA, 0x4000, 0x5800)), rng.randrange(0x0200, 0xFF00)),
                                       intno_high=rng.randrange(256))))
    return out


def differential(body, decoded, f):
    seen = set(); specs = cases(); splits = 0
    for name, options in specs:
        state = build(body, **options)
        v, final, ram, original, diff = compare(name, state, decoded, f); seen |= v
        if options.get('stub') == 'C':
            # Declared split: only AF (bit 4) of the stored entry flags differs, C/DOSBox-X 1 versus Unicorn 0, and only in
            # the low byte of Regs.AX. Stub C's own PUSHF word (entry SP - 0Ch) is overwritten by PUSH BP at 0048h.
            rp = options.get('regs_ptr', (DATA, 0x0100))
            expected = [physical(*rp)]
            check(diff == expected and all(ram[i] ^ original[i] == 0x10 and ram[i] & 0x10 for i in diff),
                  f'stub C split {name}: {[hex(i) for i in diff]}')
            splits += 1
        else:
            check(not diff, f'RAM {name}: {[hex(i) for i in diff[:8]]} ({len(diff)} bytes)')
        if name == 'alias-regs-on-frame':
            check((final.cs, final.ip) != (0x9001, 0x7000), f'fixture {name} did not redirect the return')
    check(seen == set(decoded), f'starts missing {sorted(set(decoded) - seen)}')
    return dict(cases=len(specs), stops=dict(RETURN=len(specs)), starts=len(seen), starts_total=len(decoded),
                conditional_outcomes_total=0, cpu_words=14, ram_bytes=SIZE, flag_masks=0,
                declared_split=dict(cases=splits, what='AF at handler entry, stub C only: model/DOSBox-X 1, Unicorn 0; '
                                                       'asserted to be exactly bit 4 of the low byte of Regs.AX'))


TOP, LOW, WINDOW = 0xF000, 0xEC00, 0x0400
NATIVE_INTS = (0x60, 0x61, 0x66, 0x67, 0xF1, 0xFF)


def native(body, f):
    """DOSBox-X: per case, the case's user vector points at a declared stub in the COM; interrupts stay off."""
    directory = ROOT / 'resources/dos-intr/native'; directory.mkdir(parents=True, exist_ok=True)
    (directory / 'body.bin').write_bytes(body)
    specs = [(f'{k}-int{n:02x}', dict(stub=k, intno=n)) for k in 'ABCD' for n in NATIVE_INTS]
    specs += [(f'flags{fl:x}-{k}', dict(flags=fl, stub=k, intno=0x61)) for fl in FLAGS for k in 'AC']
    names = sorted(STUBS)
    rows = [(o.get('flags', 2), o['intno'], names.index(o['stub'])) for _, o in specs]
    t = ['.code16', '.intel_syntax noprefix', '.global _start', '_start: jmp main', '.org 0x200', 'main:',
         'mov ax,cs', 'mov ds,ax', 'mov ss,ax', f'mov sp,{TOP}', 'cld',
         f'mov ax,{CS}', 'mov es,ax', f'mov di,{ENTRY}', 'mov si,offset body', f'mov cx,{len(body)}', 'rep movsb',
         'mov dx,offset outname', 'xor cx,cx', 'mov ah,0x3c', 'int 0x21', 'jc failure', 'mov word ptr [handle],ax',
         'mov word ptr [header],cs', 'mov word ptr [header+2],offset returned', f'mov word ptr [header+4],{len(rows)}',
         'mov bx,word ptr [handle]', 'mov dx,offset header', 'mov cx,6', 'mov ah,0x40', 'int 0x21',
         'again:', 'cli', 'cld', 'mov ax,cs', 'mov ds,ax', 'mov es,ax', 'mov al,0x5a', f'mov di,{LOW}', f'mov cx,{WINDOW}',
         'rep stosb', f'mov ax,{DATA}', 'mov es,ax', 'mov di,0x0100', 'mov cx,0x0040', 'mov al,0x5a', 'rep stosb',
         'mov di,0x0100', 'mov si,offset regs', 'mov cx,10', 'rep movsw',
         'mov si,word ptr cs:[index]', 'mov ax,si', 'shl si,1', 'add si,ax', 'shl si,1', 'add si,offset table',  # 6 bytes/row
         'mov word ptr cs:[row],si',
         'xor ax,ax', 'mov es,ax', 'mov bx,word ptr cs:[si+2]', 'shl bx,1', 'shl bx,1',
         'mov ax,word ptr es:[bx]', 'mov word ptr cs:[oldvec],ax', 'mov ax,word ptr es:[bx+2]', 'mov word ptr cs:[oldvec+2],ax',
         'mov di,word ptr cs:[si+4]', 'shl di,1', 'mov ax,word ptr cs:[stubs+di]', 'mov word ptr es:[bx],ax', 'mov word ptr es:[bx+2],cs',
         f'mov sp,{TOP}', 'mov ax,word ptr cs:[si+2]', 'or ax,0xa500', 'push ax', f'push {DATA}', 'push 0x0100',
         'push word ptr cs:[si]', f'mov ax,{DS}', 'mov ds,ax', 'mov ax,0x1111', 'mov es,ax',
         'mov ax,0x1234', 'mov bx,0x3456', 'mov cx,0x5678', 'mov dx,0x789a', 'mov si,0x9abc', 'mov di,0xbcde', 'mov bp,0x7777',
         'popf', f'lcall {CS},{ENTRY}', 'returned:']
    t += [f'mov word ptr cs:[out+{2 * n}],{r}' for n, r in enumerate('ax bx cx dx si di bp sp cs'.split())]
    t += ['mov word ptr cs:[out+18],offset returned', 'mov word ptr cs:[out+20],ds', 'mov word ptr cs:[out+22],es',
          'mov word ptr cs:[out+24],ss', 'mov sp,0xf800', 'pushf', 'pop ax', 'mov word ptr cs:[out+26],ax',
          'mov ax,cs', 'mov ds,ax', 'xor ax,ax', 'mov es,ax', 'mov si,word ptr [row]', 'mov bx,word ptr [si+2]', 'shl bx,1', 'shl bx,1',
          'mov ax,word ptr [oldvec]', 'mov word ptr es:[bx],ax', 'mov ax,word ptr [oldvec+2]', 'mov word ptr es:[bx+2],ax', 'sti',
          'mov bx,word ptr [handle]', 'mov dx,offset out', 'mov cx,28', 'mov ah,0x40', 'int 0x21',
          'mov bx,word ptr [handle]', f'mov dx,{LOW}', f'mov cx,{WINDOW}', 'mov ah,0x40', 'int 0x21',
          f'mov ax,{DATA}', 'mov ds,ax', 'mov bx,word ptr cs:[handle]', 'mov dx,0x0100', 'mov cx,0x0040', 'mov ah,0x40', 'int 0x21',
          'push cs', 'pop ds', 'inc word ptr [index]', f'cmp word ptr [index],{len(rows)}', 'jae done', 'jmp again', 'done:',
          'mov bx,word ptr [handle]', 'mov ah,0x3e', 'int 0x21', 'mov ax,0x4c00', 'int 0x21', 'failure:', 'mov ax,0x4c01', 'int 0x21']
    for k in names:
        t += [f'stub{k}:', '.byte ' + ','.join(str(b) for b in STUBS[k][0])]
    t += ['stubs: .word ' + ','.join(f'stub{k}' for k in names),
          'outname: .asciz "RESULT.BIN"', 'handle: .word 0', 'index: .word 0', 'row: .word 0', 'header: .space 6', 'out: .space 28',
          'oldvec: .word 0,0', 'regs: .word ' + ','.join(str(w) for w in REGS), 'table:']
    t += ['.word ' + ','.join(str(v) for v in row) for row in rows]
    t += ['body:', f'.incbin "{directory / "body.bin"}"']
    (directory / 'oracle.S').write_text('\n'.join(t) + '\n')
    subprocess.run(['as', '--32', str(directory / 'oracle.S'), '-o', str(directory / 'oracle.o')], check=True)
    com = directory / 'ORACLE.COM'
    subprocess.run(['ld', '-m', 'elf_i386', '-Ttext', '0x100', '--oformat', 'binary', str(directory / 'oracle.o'), '-o', str(com)], check=True)
    check(com.stat().st_size + 0x100 < LOW, 'COM reaches the stack window')
    nm = subprocess.run(['nm', str(directory / 'oracle.o')], capture_output=True, text=True, check=True).stdout
    symbols = {line.split()[2]: int(line.split()[0], 16) for line in nm.splitlines() if len(line.split()) == 3}
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
    data = out.read_bytes(); record = 28 + WINDOW + 0x40
    check(len(data) == 6 + len(rows) * record, f'native length {len(data)}')
    seg, ret_ip, count = struct.unpack_from('<3H', data); check(count == len(rows), 'native count')
    stub_at = {k: symbols[f'stub{k}'] + 0x100 for k in names}  # object symbols are .text-relative; the COM loads at 100h
    kinds = {(seg, off): k for k, off in stub_at.items()}
    for n, ((name, o), (flags, intno, _)) in enumerate(zip(specs, rows)):
        rec = data[6 + n * record:6 + (n + 1) * record]
        m = bytearray(SIZE)
        at = physical(CS, ENTRY); m[at:at + len(body)] = body
        m[physical(seg, LOW):physical(seg, LOW) + WINDOW] = b'\x5a' * WINDOW
        m[physical(DATA, 0x100):physical(DATA, 0x140)] = b'\x5a' * 0x40
        for j, w in enumerate(REGS):
            put16(m, DATA, 0x100 + 2 * j, w)
        put16(m, 0, intno * 4, stub_at[o['stub']]); put16(m, 0, intno * 4 + 2, seg)
        sp = TOP - 0x0A
        for j, w in enumerate((ret_ip, seg, 0x0100, DATA, 0xA500 | intno)):
            put16(m, seg, sp + 2 * j, w)
        c = Cpu(0x1234, 0x3456, 0x5678, 0x789A, 0x9ABC, 0xBCDE, 0x7777, sp, CS, ENTRY, DS, 0x1111, seg, flags)
        ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(m)
        check(f(ram, SIZE, ctypes.byref(c), make_service(kinds), None) == 0, f'native C status {name}')
        for (reg, _), want in zip(Cpu._fields_, struct.unpack_from('<14H', rec)):
            check(getattr(c, reg) == want, f'native CPU {name}/{reg}: C {getattr(c, reg):04x} DOS {want:04x}')
        check(bytes(ram[physical(seg, LOW):physical(seg, LOW) + WINDOW]) == rec[28:28 + WINDOW], f'native stack {name}')
        check(bytes(ram[physical(DATA, 0x100):physical(DATA, 0x140)]) == rec[28 + WINDOW:], f'native data {name}')
    probe = subprocess.run([binary, '--version'], capture_output=True, text=True, env=dict(os.environ, SDL_VIDEODRIVER='dummy'))
    version = next(line.split(',')[0].strip() for line in (probe.stdout + probe.stderr).splitlines() if 'DOSBox-X version' in line)
    return dict(cases=len(rows), cpu_words=14, stack_bytes_compared=WINDOW, data_bytes=0x40, flag_masks=0,
                declared_stubs='A/B/C/D at user vectors ' + ','.join(f'{n:02X}h' for n in NATIVE_INTS),
                result_sha256=sha256(data).hexdigest(),
                dosbox_x=dict(path=binary, sha256=sha256(open(binary, 'rb').read()).hexdigest(), version=version,
                              pinned_source='5a9c7c53481c2e933948039c33256776653f4faf', pinned_build_used=False))


def main():
    p = argparse.ArgumentParser()
    for flag in ('--compile-only', '--unicorn', '--native'):
        p.add_argument(flag, action='store_true')
    p.add_argument('--output', type=Path); a = p.parse_args()
    with tempfile.TemporaryDirectory(prefix='intr-') as tmp:
        f = library(Path(tmp))
        if a.compile_only:
            print('CHECKER_BUILD_OK'); return
        body, decoded = source()
        report = dict(target=dict(span='0A32:[000B,006C)', image_span=[physical(0xA32, ENTRY), physical(0xA32, END)],
                                  bytes=len(body), instructions=len(decoded), sha256=DIGEST))
        if a.unicorn:
            report['unicorn'] = differential(body, decoded, f)
        if a.native:
            report['native'] = native(body, f)
    if a.output:
        a.output.write_text(json.dumps(report, sort_keys=True, indent=2) + '\n')
    print(json.dumps(report, sort_keys=True))


if __name__ == '__main__':
    main()
