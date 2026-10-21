#!/usr/bin/env python3
"""Turbo Pascal SYSTEM ParamStr 0AF8:16BC + 1712, n <> 0 (raw CPU; stays DECLARED_BOUNDARY, no credit), against the
original bytes. Pure memory: the real bytes read a PSP command tail. Unicorn: CPU14 + full 1 MiB with synthetic PSPs.
Native (DOSBox-X): synthetic PSPs, plus cases that point PrefixSeg at the oracle program's own DOS-built PSP
(launched with a command line). Mode 0 (n = 0) is outside the contract (ARGUMENT); its instructions are excluded
from the start count with that argument. Media and generated programs stay under ignored resources/."""
import argparse, ctypes, json, os, random, struct, subprocess, tempfile
from hashlib import sha256
from pathlib import Path
from capstone import Cs, CS_ARCH_X86, CS_MODE_16
from resident_loader_map import unpack
from res0709_ega_reset_0000_cpu_audit import Cpu, ROOT, SIZE, REGISTERS, physical, put16, check
CHECKER_ROWS = ('RES_0AF8_16BC',)
PIECES = ((0x16BC, 0x170B, '42c78869351a75004ad361295cc0170912fba23737c53d54f1191bd667fe878e', 38),
          (0x1712, 0x1744, '7a1426c52b336769caa137a060de90ccadbd8bfb86f86da78e6e6864313c39e2', 24))
ENTRY, MODE0 = 0x16BC, (0x16CB, 0x16FB)
CS, DS, SS, PSP, DEST = 0x6000, 0x5000, 0x2000, 0x4000, 0x3000
FLAGS = (0x0002, 0x00D7, 0x08D7, 0x0402, 0x0893, 0x0003)
# Feasible conditional outcomes (site: successors). 16C4 JE is only feasible not-taken: taken is mode 0 (ARGUMENT).
CONDITIONALS = {0x16C4: (0x16C6,), 0x1721: (0x172C, 0x1723), 0x1727: (0x172C, 0x1729), 0x172A: (0x1723, 0x172C),
                0x172E: (0x1739, 0x1730), 0x1734: (0x1739, 0x1736), 0x1737: (0x1730, 0x1739), 0x173D: (0x1743, 0x173F),
                0x1741: (0x1721, 0x1743)}


def source():
    image = bytes(unpack((ROOT / 'resources/DOS/POOLRAD/START.EXE').read_bytes())[0])
    pieces, decoded = [], {}
    for start, end, digest, count in PIECES:
        body = image[physical(0xAF8, start):physical(0xAF8, end)]
        check(sha256(body).hexdigest() == digest, f'ParamStr piece {start:04x} identity')
        ins = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(body, start))
        check(len(ins) == count and sum(i.size for i in ins) == len(body), f'decode covers {start:04x}')
        pieces.append((start, body)); decoded.update({i.address: i for i in ins})
    check([a for a, i in sorted(decoded.items()) if i.mnemonic in ('ret', 'retf', 'iret')] == [0x1708, 0x1743], 'returns')
    check([a for a, i in decoded.items() if i.mnemonic == 'int'] == [0x16CD], 'the only INT is mode 0\'s INT 21h')
    check(all(MODE0[0] <= a < MODE0[1] for a, i in decoded.items() if i.mnemonic == 'int'), 'INT inside the excluded mode-0 run')
    live = {a: i for a, i in decoded.items() if not MODE0[0] <= a < MODE0[1]}
    return pieces, decoded, live


def library(directory):
    path = directory / 'paramstr.so'
    subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Wpedantic', '-Werror', '-O2', '-fPIC', '-shared', '-I', str(ROOT / 'include'),
                    str(ROOT / 'src/recovered/system_paramstr_16bc_cpu.c'), '-o', str(path)], check=True)
    f = ctypes.CDLL(str(path)).pr_system_paramstr_16bc
    f.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t, ctypes.POINTER(Cpu)]
    f.restype = ctypes.c_int
    return f


def place(m, pieces):
    for start, body in pieces:
        at = physical(CS, start); m[at:at + len(body)] = body


def write_psp(m, seg, tail, count=None):
    """PSP command tail as DOS builds it: count byte at :0080, the tail bytes from :0081, then 0Dh (not counted)."""
    count = len(tail) if count is None else count
    m[physical(seg, 0x80)] = count
    for k, b in enumerate(tail + b'\r'):
        m[physical(seg, 0x81 + k)] = b


def build(pieces, *, tail=b' STING', count=None, n=1, dest=(DEST, 0x0100), psp=PSP, ss=SS, sp=0x8000, flags=2, ds=DS, regs=None):
    m = bytearray([0x5A]) * SIZE
    place(m, pieces)
    write_psp(m, psp, tail, count)
    put16(m, ds, 0x4422, psp)
    for j, w in enumerate((0x7000, 0x9001, n, dest[1], dest[0])):
        put16(m, ss, (sp + 2 * j) & 0xFFFF, w)
    r = regs or (0x1234, 0x3456, 0x5678, 0x789A, 0x9ABC, 0xBCDE, 0x7777)
    return m, Cpu(*r, sp, CS, ENTRY, ds, 0x1111, ss, flags)


def expected_word(tail, count, n):
    """Independent: the n-th run of bytes above 20h within the counted tail (b'' past the last)."""
    words = [w for w in bytes(b if b > 0x20 else 0x20 for b in tail[:count]).split(b' ') if w]
    return words[n - 1] if 1 <= n <= len(words) else b''


def compare(name, state, live, f):
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
        if s['prev'] in CONDITIONALS and ip != s['prev']:
            edges.add((s['prev'], ip))
        if s['ret']:
            s['stop'] = 0; return uc.emu_stop()
        check(cs == CS and ip in live, f'escape {name} {cs:04x}:{ip:04x}')
        seen.add(ip); s['prev'] = ip
        if live[ip].mnemonic == 'retf':
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
    return seen, edges, bytes(ram)


TAILS = {'sting': b' STING', 'empty': b'', 'sting-foo': b' STING foo', 'spaced': b'   a   b  c ', 'bare': b'STING',
         'tabs': b'\tSTING\tx', 'high': b' \x80\xffSTING \xe9t\xe9', 'ctl': b'\x01\x1fSTING', 'one-char': b' x',
         'full127': bytes(0x41 + (k % 26) if k % 9 else 0x20 for k in range(127))}


def cases():
    rng = random.Random(0x0AF816BC)
    out = [(f'{k}-n{n}', dict(tail=t, n=n)) for k, t in TAILS.items() for n in (1, 2, 3)]
    out += [(f'sting-n{n:x}', dict(n=n)) for n in (5, 0x7FFF, 0x8000, 0xFFFF)]
    out += [('count-short', dict(tail=b' STING', count=3)), ('count-255', dict(tail=bytes(range(0x21, 0x7F)) * 3, count=255)),
            ('count-over-cr', dict(tail=b' ab', count=6))]
    out += [(f'flags{f:x}', dict(flags=f)) for f in FLAGS]
    out += [(f'sp{sp:x}', dict(sp=sp)) for sp in (0x0006, 0x0007, 0x8001, 0xFFF5, 0xFFF6)]
    out += [('ss-ds', dict(ss=DS)), ('a20-stack', dict(ss=0xFFFF, sp=0x0400)),
            ('dest-straddle-ffff', dict(dest=(DEST, 0xFFFC), tail=b' STING foo')), ('dest-a20', dict(dest=(0xFFFF, 0x0020))),
            ('psp-a20', dict(psp=0xFFFF, tail=b' STING foo', n=2)),
            # Destination over the stack frame (entry SP 8000h): the copy rewrites the saved DS/BP and the return.
            ('alias-dest-on-frame', dict(dest=(SS, 0x7FFA), tail=b' ABCDEFGHIJKL')),
            # Destination over the PSP tail itself: the copy reads bytes it has already rewritten.
            ('alias-dest-on-tail', dict(dest=(PSP, 0x0083), tail=b' STINGSTING'))]
    for k in range(40):
        tail = bytes(rng.choice(b' \t\x00ab\x7f\x80\xffSTING') for _ in range(rng.randrange(0, 60)))
        out.append((f'random{k}', dict(tail=tail, n=rng.choice((1, 1, 2, 3, rng.randrange(1, 65536))), flags=rng.choice(FLAGS),
                                       regs=tuple(rng.randrange(65536) for _ in range(7)),
                                       dest=(rng.choice((DEST, 0x7000)), rng.randrange(0, 0xFE00)))))
    return out


def differential(pieces, live, f):
    seen, edges = set(), set(); specs = cases(); checked = 0
    for name, options in specs:
        state = build(pieces, **options)
        v, e, ram = compare(name, state, live, f); seen |= v; edges |= e
        if not name.startswith(('alias', 'dest-', 'psp-a20')):
            tail = options.get('tail', b' STING'); count = options.get('count', len(tail))
            full = (tail + b'\r' + b'\x5a' * 300)[:max(count, 0)]
            word = expected_word(full, count, options.get('n', 1))
            seg, off = options.get('dest', (DEST, 0x0100))
            got = bytes(ram[physical(seg, (off + k) & 0xFFFF)] for k in range(1 + len(word)))
            check(got == bytes([len(word)]) + word, f'independent word {name}: {got!r} vs {word!r}')
            checked += 1
    check(seen == set(live), f'starts missing {sorted(set(live) - seen)}')
    want = {(site, t) for site, ts in CONDITIONALS.items() for t in ts}
    check(edges == want, f'outcomes missing {sorted(want - edges)} extra {sorted(edges - want)}')
    return dict(cases=len(specs), stops=dict(RETURN=len(specs)), starts=len(seen), starts_total=len(live),
                starts_excluded='mode-0 run [16CB,16FB): n = 0 is ARGUMENT (RES_0899_0059 passes 1)',
                conditional_outcomes=len(edges), conditional_outcomes_total=len(want), independent_value_checks=checked,
                cpu_words=14, ram_bytes=SIZE, flag_masks=0)


TOP, LOW, WINDOW, DEST_BYTES = 0xF000, 0xEC00, 0x0400, 0x0110
NATIVE_ARGS = 'STING foo'


def native(pieces, f):
    """DOSBox-X: synthetic PSP tails at segment 4000h, and cases whose PrefixSeg is the oracle's own PSP (command line
    'STING foo' as DOS built it)."""
    directory = ROOT / 'resources/system-paramstr/native'; directory.mkdir(parents=True, exist_ok=True)
    for start, body in pieces:
        (directory / f'piece{start:04x}.bin').write_bytes(body)
    synth = [(k, t, n) for k, t in TAILS.items() if k != 'full127' for n in (1, 2)]
    rows = [(2, n, 0) for _, _, n in synth] + [(2, n, 1) for n in (1, 2, 3)] + [(fl, 1, 0) for fl in FLAGS]
    names = [f'{k}-n{n}' for k, _, n in synth] + [f'dos-psp-n{n}' for n in (1, 2, 3)] + [f'flags{fl:x}' for fl in FLAGS]
    tails = [t for _, t, _ in synth] + [None] * 3 + [b' STING'] * len(FLAGS)
    tail_rows = []
    for t in tails:
        t = t or b''
        tail_rows.append(bytes([len(t)]) + t + b'\r' + b'\x5a' * (0x80 - 2 - len(t)))
    copy = []
    for start, body in pieces:
        copy += [f'mov di,{start}', f'mov si,offset piece{start:04x}', f'mov cx,{len(body)}', 'rep movsb']
    t = ['.code16', '.intel_syntax noprefix', '.global _start', '_start: jmp main', '.org 0x200', 'main:',
         'mov ax,cs', 'mov ds,ax', 'mov ss,ax', f'mov sp,{TOP}', 'cld', f'mov ax,{CS}', 'mov es,ax'] + copy + [
         'mov dx,offset outname', 'xor cx,cx', 'mov ah,0x3c', 'int 0x21', 'jc failure', 'mov word ptr [handle],ax',
         'mov word ptr [header],cs', 'mov word ptr [header+2],offset returned', f'mov word ptr [header+4],{len(rows)}',
         'mov bx,word ptr [handle]', 'mov dx,offset header', 'mov cx,6', 'mov ah,0x40', 'int 0x21',
         # The oracle's own PSP tail (as DOS built it), recorded once so the C side sees the same bytes.
         'mov bx,word ptr [handle]', 'mov dx,0x80', 'mov cx,0x80', 'mov ah,0x40', 'int 0x21',
         'again:', 'cli', 'cld', 'mov ax,cs', 'mov ds,ax', 'mov es,ax', 'mov al,0x5a', f'mov di,{LOW}', f'mov cx,{WINDOW}', 'rep stosb',
         f'mov ax,{DEST}', 'mov es,ax', 'mov di,0x0100', f'mov cx,{DEST_BYTES}', 'mov al,0x5a', 'rep stosb',
         f'mov ax,{PSP}', 'mov es,ax', 'mov di,0', 'mov cx,0x100', 'mov al,0x5a', 'rep stosb',
         'mov si,word ptr cs:[index]', 'shl si,1', 'shl si,1', 'shl si,1', 'shl si,1', 'shl si,1', 'shl si,1', 'shl si,1',
         'add si,offset tails', 'mov di,0x80', 'mov cx,0x80', 'rep movsb',  # synthetic tail row -> 4000:0080
         'mov si,word ptr cs:[index]', 'mov ax,si', 'shl si,1', 'add si,ax', 'shl si,1', 'add si,offset table',  # 6 bytes/row
         f'mov ax,{DS}', 'mov es,ax', f'mov word ptr es:[0x4422],{PSP}', 'test word ptr cs:[si+4],1', 'jz synthetic',
         'mov word ptr es:[0x4422],cs', 'synthetic:',
         f'mov sp,{TOP}', f'push {DEST}', 'push 0x0100', 'push word ptr cs:[si+2]', 'push word ptr cs:[si]',
         f'mov ax,{DS}', 'mov ds,ax', 'mov ax,0x1111', 'mov es,ax',
         'mov ax,0x1234', 'mov bx,0x3456', 'mov cx,0x5678', 'mov dx,0x789a', 'mov si,0x9abc', 'mov di,0xbcde', 'mov bp,0x7777',
         'popf', f'lcall {CS},{ENTRY}', 'returned:']
    t += [f'mov word ptr cs:[out+{2 * n}],{r}' for n, r in enumerate('ax bx cx dx si di bp sp cs'.split())]
    t += ['mov word ptr cs:[out+18],offset returned', 'mov word ptr cs:[out+20],ds', 'mov word ptr cs:[out+22],es',
          'mov word ptr cs:[out+24],ss', 'mov sp,0xf800', 'pushf', 'pop ax', 'mov word ptr cs:[out+26],ax', 'sti',
          'mov ax,cs', 'mov ds,ax',
          'mov bx,word ptr [handle]', 'mov dx,offset out', 'mov cx,28', 'mov ah,0x40', 'int 0x21',
          'mov bx,word ptr [handle]', f'mov dx,{LOW}', f'mov cx,{WINDOW}', 'mov ah,0x40', 'int 0x21',
          f'mov ax,{DEST}', 'mov ds,ax', 'mov bx,word ptr cs:[handle]', 'mov dx,0x0100', f'mov cx,{DEST_BYTES}', 'mov ah,0x40', 'int 0x21',
          'push cs', 'pop ds', 'inc word ptr [index]', f'cmp word ptr [index],{len(rows)}', 'jae done', 'jmp again', 'done:',
          'mov bx,word ptr [handle]', 'mov ah,0x3e', 'int 0x21', 'mov ax,0x4c00', 'int 0x21', 'failure:', 'mov ax,0x4c01', 'int 0x21',
          'outname: .asciz "RESULT.BIN"', 'handle: .word 0', 'index: .word 0', 'header: .space 6', 'out: .space 28', '.align 2', 'table:']
    t += ['.word ' + ','.join(str(v) for v in row) for row in rows]
    t += ['tails:'] + ['.byte ' + ','.join(str(b) for b in row) for row in tail_rows]
    for start, _ in pieces:
        t += [f'piece{start:04x}:', f'.incbin "{directory / f"piece{start:04x}.bin"}"']
    (directory / 'oracle.S').write_text('\n'.join(t) + '\n')
    subprocess.run(['as', '--32', str(directory / 'oracle.S'), '-o', str(directory / 'oracle.o')], check=True)
    com = directory / 'ORACLE.COM'
    subprocess.run(['ld', '-m', 'elf_i386', '-Ttext', '0x100', '--oformat', 'binary', str(directory / 'oracle.o'), '-o', str(com)], check=True)
    check(com.stat().st_size + 0x100 < LOW, 'COM reaches the stack window')
    (directory / 'dosbox.conf').write_text('[sdl]\noutput=surface\n[dosbox]\nmemsize=16\n[cpu]\ncore=normal\ncputype=386\ncycles=max\n'
                                          '[mixer]\nnosound=true\n[midi]\nmididevice=none\n[autoexec]\nmount c '
                                          + str(directory.resolve()) + f'\nc:\nORACLE.COM {NATIVE_ARGS}\nexit\n')
    out = directory / 'RESULT.BIN'
    if out.exists():
        out.unlink()
    binary = '/usr/bin/dosbox-x'
    with (directory / 'dosbox.log').open('w') as log:
        subprocess.run([binary, '-conf', str(directory / 'dosbox.conf'), '-fastlaunch', '-exit', '-noconsole'],
                       env=dict(os.environ, SDL_VIDEODRIVER='dummy', SDL_AUDIODRIVER='dummy'), stdout=log, stderr=subprocess.STDOUT,
                       check=True, timeout=300)
    data = out.read_bytes(); record = 28 + WINDOW + DEST_BYTES
    check(len(data) == 6 + 0x80 + len(rows) * record, f'native length {len(data)}')
    seg, ret_ip, count = struct.unpack_from('<3H', data); check(count == len(rows), 'native count')
    dos_tail = data[6:6 + 0x80]
    check(dos_tail[0] == len(' ' + NATIVE_ARGS) and dos_tail[1:1 + dos_tail[0]] == (' ' + NATIVE_ARGS).encode() and
          dos_tail[1 + dos_tail[0]] == 0x0D, f'DOS-built PSP tail {dos_tail[:16]!r}')
    for k, (name, (flags, n, real), tail_row) in enumerate(zip(names, rows, tail_rows)):
        rec = data[6 + 0x80 + k * record:6 + 0x80 + (k + 1) * record]
        m = bytearray(SIZE)
        place(m, pieces)
        m[physical(seg, LOW):physical(seg, LOW) + WINDOW] = b'\x5a' * WINDOW
        m[physical(DEST, 0x100):physical(DEST, 0x100 + DEST_BYTES)] = b'\x5a' * DEST_BYTES
        m[physical(PSP, 0):physical(PSP, 0x100)] = b'\x5a' * 0x100
        m[physical(PSP, 0x80):physical(PSP, 0x100)] = tail_row
        m[physical(seg, 0x80):physical(seg, 0x100)] = dos_tail
        put16(m, DS, 0x4422, seg if real else PSP)
        sp = TOP - 0x0A
        for j, w in enumerate((ret_ip, seg, n, 0x0100, DEST)):
            put16(m, seg, sp + 2 * j, w)
        c = Cpu(0x1234, 0x3456, 0x5678, 0x789A, 0x9ABC, 0xBCDE, 0x7777, sp, CS, ENTRY, DS, 0x1111, seg, flags)
        ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(m)
        check(f(ram, SIZE, ctypes.byref(c)) == 0, f'native C status {name}')
        for (reg, _), want in zip(Cpu._fields_, struct.unpack_from('<14H', rec)):
            check(getattr(c, reg) == want, f'native CPU {name}/{reg}: C {getattr(c, reg):04x} DOS {want:04x}')
        check(bytes(ram[physical(seg, LOW):physical(seg, LOW) + WINDOW]) == rec[28:28 + WINDOW], f'native stack {name}')
        check(bytes(ram[physical(DEST, 0x100):physical(DEST, 0x100 + DEST_BYTES)]) == rec[28 + WINDOW:], f'native dest {name}')
        if real:
            word = expected_word((' ' + NATIVE_ARGS).encode(), len(' ' + NATIVE_ARGS), n)
            check(rec[28 + WINDOW:28 + WINDOW + 1 + len(word)] == bytes([len(word)]) + word, f'native DOS-PSP word {name}')
    probe = subprocess.run([binary, '--version'], capture_output=True, text=True, env=dict(os.environ, SDL_VIDEODRIVER='dummy'))
    version = next(line.split(',')[0].strip() for line in (probe.stdout + probe.stderr).splitlines() if 'DOSBox-X version' in line)
    return dict(cases=len(rows), dos_psp_cases=3, dos_command_line=NATIVE_ARGS, cpu_words=14, stack_bytes_compared=WINDOW,
                dest_bytes=DEST_BYTES, flag_masks=0, result_sha256=sha256(data).hexdigest(),
                dosbox_x=dict(path=binary, sha256=sha256(open(binary, 'rb').read()).hexdigest(), version=version,
                              pinned_source='5a9c7c53481c2e933948039c33256776653f4faf', pinned_build_used=False))


def main():
    p = argparse.ArgumentParser()
    for flag in ('--compile-only', '--unicorn', '--native'):
        p.add_argument(flag, action='store_true')
    p.add_argument('--output', type=Path); a = p.parse_args()
    with tempfile.TemporaryDirectory(prefix='paramstr-') as tmp:
        f = library(Path(tmp))
        if a.compile_only:
            print('CHECKER_BUILD_OK'); return
        pieces, decoded, live = source()
        report = dict(target=dict(spans=['0AF8:[16BC,170B)', '0AF8:[1712,1744)'],
                                  image_spans=[[physical(0xAF8, s), physical(0xAF8, e)] for s, e, _, _ in PIECES],
                                  bytes=sum(len(b) for _, b in pieces), instructions=len(decoded),
                                  mode0_excluded=[a for a in sorted(decoded) if MODE0[0] <= a < MODE0[1]],
                                  sha256=[d for _, _, d, _ in PIECES]))
        if a.unicorn:
            report['unicorn'] = differential(pieces, live, f)
        if a.native:
            report['native'] = native(pieces, f)
    if a.output:
        a.output.write_text(json.dumps(report, sort_keys=True, indent=2) + '\n')
    print(json.dumps(report, sort_keys=True))


if __name__ == '__main__':
    main()
