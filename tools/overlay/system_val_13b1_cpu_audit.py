#!/usr/bin/env python3
"""Turbo Pascal SYSTEM Val (longint) 0AF8:13B1 + parser 12CE (raw CPU; runtime coverage, no identity) against the original
bytes. Pure memory: no interrupt, port or service. Unicorn: CPU14 + full 1 MiB. Native (DOSBox-X): CPU14 + the stack window +
the string and Code regions. Media and generated programs stay under ignored resources/."""
import argparse, ctypes, json, os, random, struct, subprocess, tempfile
from hashlib import sha256
from pathlib import Path
from capstone import Cs, CS_ARCH_X86, CS_MODE_16
from resident_loader_map import unpack
from res0709_ega_reset_0000_cpu_audit import Cpu, ROOT, SIZE, REGISTERS, physical, put16, check
CHECKER_ROWS = ('RES_0AF8_13B1',)
PIECES = ((0x13B1, 0x13E2, 'ccbbe96993e3b9e4e8097e2f236090ce7a432c36825b80535b4530076735bfa0', 22),
          (0x12CE, 0x1366, 'c64b8556e8ee0c75d1634824466a6bd1273b5299e94331dfb00777cd10c55748', 74))
ENTRY = 0x13B1
CS, DS, SS, STR, CODE = 0x6000, 0x5000, 0x2000, 0x3000, 0x4000
FLAGS = (0x0002, 0x00D7, 0x08D7, 0x0402, 0x0893, 0x0003)


def source():
    image = bytes(unpack((ROOT / 'resources/DOS/POOLRAD/START.EXE').read_bytes())[0])
    pieces, decoded = [], {}
    for start, end, digest, count in PIECES:
        body = image[physical(0xAF8, start):physical(0xAF8, end)]
        check(sha256(body).hexdigest() == digest, f'Val piece {start:04x} identity')
        ins = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(body, start))
        check(len(ins) == count and sum(i.size for i in ins) == len(body), f'decode covers {start:04x}')
        pieces.append((start, body)); decoded.update({i.address: i for i in ins})
    check(sorted(a for a, i in decoded.items() if i.mnemonic in ('ret', 'retf')) == [0x132E, 0x1330, 0x13DF], 'returns')
    check([(a, i.op_str) for a, i in decoded.items() if i.mnemonic == 'call'] == [(0x13C8, '0x12ce')], 'single near CALL 12CE')
    check(not [a for a, i in decoded.items() if i.mnemonic in ('int', 'in', 'out', 'lcall')], 'pure memory')
    conds = {a: i for a, i in decoded.items() if (i.mnemonic.startswith('j') and i.mnemonic != 'jmp') or i.mnemonic == 'loop'}
    sites = {a: (int(i.op_str, 16), a + i.size) for a, i in conds.items()}
    return pieces, decoded, sites


def library(directory):
    path = directory / 'val.so'
    subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Wpedantic', '-Werror', '-O2', '-fPIC', '-shared', '-I', str(ROOT / 'include'),
                    str(ROOT / 'src/recovered/system_val_13b1_cpu.c'), '-o', str(path)], check=True)
    f = ctypes.CDLL(str(path)).pr_system_val_13b1
    f.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t, ctypes.POINTER(Cpu)]
    f.restype = ctypes.c_int
    return f


def place(m, pieces, cs=CS):
    for start, body in pieces:
        at = physical(cs, start); m[at:at + len(body)] = body


def build(pieces, *, text=b'123', count=None, s=(STR, 0x0100), code=(CODE, 0x0200), ss=SS, sp=0x8000, flags=2, regs=None):
    m = bytearray([0x5A]) * SIZE
    place(m, pieces)
    count = len(text) if count is None else count
    m[physical(s[0], s[1])] = count
    for k, ch in enumerate(text):
        m[physical(s[0], (s[1] + 1 + k) & 0xFFFF)] = ch
    for j, w in enumerate((0x7000, 0x9001, code[1], code[0], s[1], s[0])):
        put16(m, ss, (sp + 2 * j) & 0xFFFF, w)
    r = regs or (0x1234, 0x3456, 0x5678, 0x789A, 0x9ABC, 0xBCDE, 0x7777)
    return m, Cpu(*r, sp, CS, ENTRY, DS, 0x1111, ss, flags)


def expected(text):
    """Independent statement of Val's longint semantics over the counted bytes: (value, code)."""
    n = len(text); i = 0
    while i < n and text[i] == 0x20:
        i += 1
    start = i; neg = False; v = 0
    if i == n:
        return 0, i + 1
    if text[i] in b'+-':
        neg = text[i] == ord('-'); i += 1
        if i == n:
            return 0, i + 1
    if text[i] == ord('$'):
        i += 1
        if i == n:
            return 0, i + 1
        while i < n:
            ch = text[i]; ch = ch - 0x20 if ch >= 0x61 else ch
            d = ch - 0x30 if 0x30 <= ch <= 0x39 else ch - 0x37 if 0x41 <= ch <= 0x46 else None
            if d is None:
                break
            if v >> 28:
                return 0, i + 1
            v = (v << 4) | d; i += 1
    else:
        while i < n:
            d = text[i] - 0x30
            if not 0 <= d <= 9:
                break
            if v >> 28:
                return 0, i + 1
            v = v * 10 + d
            if v >> 31:
                return 0, i + 1
            i += 1
    del start
    if i < n:
        return 0, i + 1
    v = (-v) & 0xFFFFFFFF if neg else v
    return v, 0


def compare(name, state, decoded, sites, f):
    from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE, UC_HOOK_INTR
    from unicorn import x86_const as R
    m, c = state
    uc = Uc(UC_ARCH_X86, UC_MODE_16); backing = (ctypes.c_uint8 * SIZE).from_buffer_copy(m)
    uc.mem_map_ptr(0, SIZE, 7, ctypes.addressof(backing)); uc.mem_map_ptr(SIZE, SIZE, 7, ctypes.addressof(backing))
    for (n, _), reg in zip(Cpu._fields_, REGISTERS):
        uc.reg_write(reg, getattr(c, n))
    s = dict(stop=None, ret=False, prev=None); seen = set(); edges = set()

    def code(em, address, size, user):
        ip = uc.reg_read(R.UC_X86_REG_IP); cs = uc.reg_read(R.UC_X86_REG_CS)
        if s['prev'] in sites:
            edges.add((s['prev'], ip))
        if s['ret']:
            s['stop'] = 0; return uc.emu_stop()
        check(cs == CS and ip in decoded, f'escape {name} {cs:04x}:{ip:04x}')
        seen.add(ip); s['prev'] = ip
        if ip == 0x13DF:
            s['ret'] = True

    def intr(em, intno, user):
        raise ValueError(f'unexpected interrupt {intno:#x} in {name}')
    uc.hook_add(UC_HOOK_CODE, code); uc.hook_add(UC_HOOK_INTR, intr)
    uc.emu_start(physical(c.cs, c.ip), 2 * SIZE, count=20000)
    check(s['stop'] == 0, f'original did not return {name}')
    actual = Cpu.from_buffer_copy(c); ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(m)
    check(f(ram, SIZE, ctypes.byref(actual)) == 0, f'C status {name}')
    if bytes(ram) != bytes(backing):
        diff = [hex(i) for i, (a, b) in enumerate(zip(bytes(ram), bytes(backing))) if a != b]
        raise ValueError(f'RAM {name}: {diff[:8]} ({len(diff)} bytes)')
    for (n, _), reg in zip(Cpu._fields_, REGISTERS):
        want = uc.reg_read(reg) & 0xFFFF
        check(getattr(actual, n) == want, f'CPU {name}/{n} C {getattr(actual, n):04x} original {want:04x}')
    return seen, edges, actual, bytes(ram)


TEXTS = [b'123', b'-45', b'+7', b'  42', b'0', b'0000123', b'$1F', b'$ff', b'$aBcD', b'$7FFFFFFF', b'$FFFFFFFF', b'$100000000',
         b'$', b'-$10', b'', b'   ', b'+', b'-', b'12a', b'abc', b'1 2', b' -', b'2147483647', b'2147483648', b'-2147483648',
         b'268435455', b'268435456', b'2684354559', b'99999999999', b'$G', b'$0f0g', b'9' * 40, b'-0', b'\t5', b'12\x00',
         b'1234567890', b'$12345678', b'$0123456789', b' +$1', b'--1', b'+-1']


def cases():
    rng = random.Random(0x0AF813B1)
    out = [(f'text{k}', dict(text=t)) for k, t in enumerate(TEXTS)]
    out += [(f'flags{f:x}', dict(flags=f, text=b'-1234')) for f in FLAGS]
    out += [(f'sp{sp:x}', dict(sp=sp)) for sp in (0x000A, 0x000B, 0x8001, 0xFFF3, 0xFFF4)]
    out += [('ss-ds', dict(ss=DS)), ('a20-stack', dict(ss=0xFFFF, sp=0x0400)),
            ('string-straddle', dict(s=(STR, 0xFFFC), text=b'12345')), ('string-a20', dict(s=(0xFFFF, 0x0020), text=b'-99')),
            ('count-short', dict(text=b'12345', count=3)), ('count-255', dict(text=(b'1' * 255))),
            # Code over the string's length byte: the store happens after the parse (no effect on it).
            ('alias-code-on-string', dict(s=(STR, 0x0100), code=(STR, 0x0100), text=b'77')),
            # Code over the return frame: the store rewrites the return IP before RETF 8.
            ('alias-code-on-return', dict(code=(SS, 0x8000), text=b'1x'))]
    for k in range(48):
        alphabet = b'0123456789+-$ abcdefABCDEFxyz\x00'
        out.append((f'random{k}', dict(text=bytes(rng.choice(alphabet) for _ in range(rng.randrange(0, 14))), flags=rng.choice(FLAGS),
                                       regs=tuple(rng.randrange(65536) for _ in range(7)))))
    return out


def differential(pieces, decoded, sites, f):
    seen, edges = set(), set(); specs = cases(); checked = 0
    for name, o in specs:
        v, e, final, ram = compare(name, build(pieces, **o), decoded, sites, f); seen |= v; edges |= e
        if name.startswith('alias-code-on-return'):
            check((final.cs, final.ip) != (0x9001, 0x7000), f'fixture {name} did not redirect the return')
            continue
        if name.startswith(('alias', 'string-', 'count-short')):
            continue
        text = o.get('text', b'123')
        value, code = expected(text)
        cseg, coff = o.get('code', (CODE, 0x0200))
        got_code = ram[physical(cseg, coff)] | ram[physical(cseg, coff + 1)] << 8
        check(((final.dx << 16) | final.ax, got_code) == (value, code), f'independent {name} {text!r}: '
              f'{(final.dx << 16) | final.ax:08x}/{got_code} vs {value:08x}/{code}')
        checked += 1
    check(seen == set(decoded), f'starts missing {sorted(hex(a) for a in set(decoded) - seen)}')
    want = {(a, t) for a, ts in sites.items() for t in ts}
    check(edges == want, f'outcomes missing {sorted((hex(a), hex(t)) for a, t in want - edges)}')
    return dict(cases=len(specs), stops=dict(RETURN=len(specs)), starts=len(seen), starts_total=len(decoded),
                conditional_outcomes=len(edges), conditional_outcomes_total=len(want), independent_value_checks=checked,
                cpu_words=14, ram_bytes=SIZE, flag_masks=0)


TOP, LOW, WINDOW, SBYTES = 0xF000, 0xEC00, 0x0400, 0x0110


def native(pieces, f):
    directory = ROOT / 'resources/system-val/native'; directory.mkdir(parents=True, exist_ok=True)
    for start, body in pieces:
        (directory / f'piece{start:04x}.bin').write_bytes(body)
    specs = [(n, o) for n, o in cases() if n.startswith(('text', 'flags'))]
    rows = [(o.get('flags', 2),) for _, o in specs]
    strings = []
    for _, o in specs:
        t = o.get('text', b'123'); strings.append(bytes([len(t)]) + t + b'\x5a' * (0x40 - 1 - len(t)))
    copy = []
    for start, body in pieces:
        copy += [f'mov di,{start}', f'mov si,offset piece{start:04x}', f'mov cx,{len(body)}', 'rep movsb']
    t = ['.code16', '.intel_syntax noprefix', '.global _start', '_start: jmp main', '.org 0x200', 'main:',
         'mov ax,cs', 'mov ds,ax', 'mov ss,ax', f'mov sp,{TOP}', 'cld', f'mov ax,{CS}', 'mov es,ax'] + copy + [
         'mov dx,offset outname', 'xor cx,cx', 'mov ah,0x3c', 'int 0x21', 'jc failure', 'mov word ptr [handle],ax',
         'mov word ptr [header],cs', 'mov word ptr [header+2],offset returned', f'mov word ptr [header+4],{len(rows)}',
         'mov bx,word ptr [handle]', 'mov dx,offset header', 'mov cx,6', 'mov ah,0x40', 'int 0x21',
         'again:', 'cli', 'cld', 'mov ax,cs', 'mov ds,ax', 'mov es,ax', 'mov al,0x5a', f'mov di,{LOW}', f'mov cx,{WINDOW}', 'rep stosb',
         f'mov ax,{STR}', 'mov es,ax', 'mov di,0x0100', f'mov cx,{SBYTES}', 'mov al,0x5a', 'rep stosb',
         f'mov ax,{CODE}', 'mov es,ax', 'mov di,0x0200', 'mov cx,4', 'mov al,0x5a', 'rep stosb',
         'mov si,word ptr cs:[index]', 'mov cl,6', 'shl si,cl', 'add si,offset strings', f'mov ax,{STR}', 'mov es,ax', 'mov di,0x0100',
         'mov cx,0x40', 'rep movsb',
         'mov si,word ptr cs:[index]', 'shl si,1', 'add si,offset table',
         f'mov sp,{TOP}', f'push {STR}', 'push 0x0100', f'push {CODE}', 'push 0x0200', 'push word ptr cs:[si]',
         f'mov ax,{DS}', 'mov ds,ax', 'mov ax,0x1111', 'mov es,ax',
         'mov ax,0x1234', 'mov bx,0x3456', 'mov cx,0x5678', 'mov dx,0x789a', 'mov si,0x9abc', 'mov di,0xbcde', 'mov bp,0x7777',
         'popf', f'lcall {CS},{ENTRY}', 'returned:']
    t += [f'mov word ptr cs:[out+{2 * n}],{r}' for n, r in enumerate('ax bx cx dx si di bp sp cs'.split())]
    t += ['mov word ptr cs:[out+18],offset returned', 'mov word ptr cs:[out+20],ds', 'mov word ptr cs:[out+22],es',
          'mov word ptr cs:[out+24],ss', 'mov sp,0xf800', 'pushf', 'pop ax', 'mov word ptr cs:[out+26],ax', 'sti', 'mov ax,cs', 'mov ds,ax',
          'mov bx,word ptr [handle]', 'mov dx,offset out', 'mov cx,28', 'mov ah,0x40', 'int 0x21',
          'mov bx,word ptr [handle]', f'mov dx,{LOW}', f'mov cx,{WINDOW}', 'mov ah,0x40', 'int 0x21',
          f'mov ax,{STR}', 'mov ds,ax', 'mov bx,word ptr cs:[handle]', 'mov dx,0x0100', f'mov cx,{SBYTES}', 'mov ah,0x40', 'int 0x21',
          f'mov ax,{CODE}', 'mov ds,ax', 'mov bx,word ptr cs:[handle]', 'mov dx,0x0200', 'mov cx,4', 'mov ah,0x40', 'int 0x21',
          'push cs', 'pop ds', 'inc word ptr [index]', f'cmp word ptr [index],{len(rows)}', 'jae done', 'jmp again', 'done:',
          'mov bx,word ptr [handle]', 'mov ah,0x3e', 'int 0x21', 'mov ax,0x4c00', 'int 0x21', 'failure:', 'mov ax,0x4c01', 'int 0x21',
          'outname: .asciz "RESULT.BIN"', 'handle: .word 0', 'index: .word 0', 'header: .space 6', 'out: .space 28', '.align 2', 'table:']
    t += ['.word ' + ','.join(str(v) for v in row) for row in rows]
    t += ['strings:'] + ['.byte ' + ','.join(str(b) for b in row) for row in strings]
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
    data = out.read_bytes(); record = 28 + WINDOW + SBYTES + 4
    check(len(data) == 6 + len(rows) * record, f'native length {len(data)}')
    seg, ret_ip, count = struct.unpack_from('<3H', data); check(count == len(rows), 'native count')
    for n, ((name, o), (flags,), srow) in enumerate(zip(specs, rows, strings)):
        rec = data[6 + n * record:6 + (n + 1) * record]
        m = bytearray(SIZE)
        place(m, pieces)
        m[physical(seg, LOW):physical(seg, LOW) + WINDOW] = b'\x5a' * WINDOW
        m[physical(STR, 0x100):physical(STR, 0x100 + SBYTES)] = b'\x5a' * SBYTES
        m[physical(STR, 0x100):physical(STR, 0x140)] = srow
        m[physical(CODE, 0x200):physical(CODE, 0x204)] = b'\x5a' * 4
        sp = TOP - 0x0C
        for j, w in enumerate((ret_ip, seg, 0x0200, CODE, 0x0100, STR)):
            put16(m, seg, sp + 2 * j, w)
        c = Cpu(0x1234, 0x3456, 0x5678, 0x789A, 0x9ABC, 0xBCDE, 0x7777, sp, CS, ENTRY, DS, 0x1111, seg, flags)
        ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(m)
        check(f(ram, SIZE, ctypes.byref(c)) == 0, f'native C status {name}')
        for (reg, _), want in zip(Cpu._fields_, struct.unpack_from('<14H', rec)):
            check(getattr(c, reg) == want, f'native CPU {name}/{reg}: C {getattr(c, reg):04x} DOS {want:04x}')
        check(bytes(ram[physical(seg, LOW):physical(seg, LOW) + WINDOW]) == rec[28:28 + WINDOW], f'native stack {name}')
        check(bytes(ram[physical(STR, 0x100):physical(STR, 0x100 + SBYTES)]) == rec[28 + WINDOW:28 + WINDOW + SBYTES], f'native string {name}')
        check(bytes(ram[physical(CODE, 0x200):physical(CODE, 0x204)]) == rec[28 + WINDOW + SBYTES:], f'native Code {name}')
    probe = subprocess.run([binary, '--version'], capture_output=True, text=True, env=dict(os.environ, SDL_VIDEODRIVER='dummy'))
    version = next(line.split(',')[0].strip() for line in (probe.stdout + probe.stderr).splitlines() if 'DOSBox-X version' in line)
    return dict(cases=len(rows), cpu_words=14, stack_bytes_compared=WINDOW, string_bytes=SBYTES, code_bytes=4, flag_masks=0,
                result_sha256=sha256(data).hexdigest(),
                dosbox_x=dict(path=binary, sha256=sha256(open(binary, 'rb').read()).hexdigest(), version=version,
                              pinned_source='5a9c7c53481c2e933948039c33256776653f4faf', pinned_build_used=False))


def main():
    p = argparse.ArgumentParser()
    for flag in ('--compile-only', '--unicorn', '--native'):
        p.add_argument(flag, action='store_true')
    p.add_argument('--output', type=Path); a = p.parse_args()
    with tempfile.TemporaryDirectory(prefix='val-') as tmp:
        f = library(Path(tmp))
        if a.compile_only:
            print('CHECKER_BUILD_OK'); return
        pieces, decoded, sites = source()
        report = dict(target=dict(spans=['0AF8:[13B1,13E2)', '0AF8:[12CE,1366)'],
                                  image_spans=[[physical(0xAF8, s), physical(0xAF8, e)] for s, e, _, _ in PIECES],
                                  bytes=sum(len(b) for _, b in pieces), instructions=len(decoded), sha256=[d for _, _, d, _ in PIECES]))
        if a.unicorn:
            report['unicorn'] = differential(pieces, decoded, sites, f)
        if a.native:
            report['native'] = native(pieces, f)
    if a.output:
        a.output.write_text(json.dumps(report, sort_keys=True, indent=2) + '\n')
    print(json.dumps(report, sort_keys=True))


if __name__ == '__main__':
    main()
