#!/usr/bin/env python3
"""Turbo Pascal Crt.KeyPressed 0A40:02FD (raw CPU; stays DECLARED_BOUNDARY, no credit) against the original bytes.

BIOS INT 16h is the single declared environment input. Declared stub, the same in every oracle: AH=01h returns
AX := the case's key word and ZF := (no key pending), every other flag as at the INT. Any other interrupt, or an
AH other than 00h/01h, is refused (ENV) rather than answered. Unicorn: CPU14 + full 1 MiB, the stub played by an
interrupt hook. Native (DOSBox-X): an INT 16h hook stub edits ZF in its IRET frame. Media and generated programs
stay under ignored resources/."""
import argparse, ctypes, json, os, random, struct, subprocess, tempfile
from hashlib import sha256
from capstone import Cs, CS_ARCH_X86, CS_MODE_16
from resident_loader_map import unpack
from res0709_ega_reset_0000_cpu_audit import Cpu, ROOT, SIZE, REGISTERS, physical, put16, check
from pathlib import Path
CHECKER_ROWS = ('RES_0A40_02FD',)
ENTRY, END, INT_IP, AFTER_IP = 0x02FD, 0x030F, 0x0306, 0x0308
DIGEST = '83af1ee6b1d5cebb5dee97b742228b8a4fb3cbb12ade84b39e5ce82275e7fc5e'
CS, DS, SS = 0x6000, 0x5000, 0x2000
FLAGS = (0x0002, 0x00D7, 0x08D7, 0x0402, 0x0893, 0x0003)
CONDITIONALS = {0x0302: (0x030C, 0x0304), 0x030A: (0x030E, 0x030C)}  # site: (taken, fall-through)
SERVICE_FN = ctypes.CFUNCTYPE(ctypes.c_int, ctypes.c_void_p, ctypes.POINTER(Cpu), ctypes.POINTER(Cpu))


def source():
    image = bytes(unpack((ROOT / 'resources/DOS/POOLRAD/START.EXE').read_bytes())[0])
    body = image[physical(0xA40, ENTRY):physical(0xA40, END)]
    check(sha256(body).hexdigest() == DIGEST, 'KeyPressed identity')
    ins = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(body, ENTRY))
    check(len(ins) == 8 and sum(i.size for i in ins) == len(body), 'decode covers the span')
    check([i.address for i in ins if i.mnemonic in ('ret', 'retf', 'iret')] == [0x030E], 'single RETF')
    check([(i.address, i.op_str) for i in ins if i.mnemonic == 'int'] == [(INT_IP, '0x16')], 'single INT 16h')
    check({i.address for i in ins if i.mnemonic.startswith('j')} == set(CONDITIONALS), 'conditional sites')
    return body, {i.address: i for i in ins}


def library(directory):
    path = directory / 'keypressed.so'
    subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Wpedantic', '-Werror', '-O2', '-fPIC', '-shared', '-I', str(ROOT / 'include'),
                    str(ROOT / 'src/recovered/crt_keypressed_02fd_cpu.c'), '-o', str(path)], check=True)
    f = ctypes.CDLL(str(path)).pr_crt_keypressed_02fd
    f.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t, ctypes.POINTER(Cpu), SERVICE_FN, ctypes.c_void_p]
    f.restype = ctypes.c_int
    return f


def build(body, *, buffered=0, pending=0, key=0x1E61, ss=SS, sp=0x8000, flags=2, ds=DS, regs=None):
    m = bytearray([0x5A]) * SIZE
    at = physical(CS, ENTRY); m[at:at + len(body)] = body
    m[physical(ds, 0x8501)] = buffered
    put16(m, ss, sp, 0x7000); put16(m, ss, (sp + 2) & 0xFFFF, 0x9001)
    r = regs or (0x1234, 0x3456, 0x5678, 0x789A, 0x9ABC, 0xBCDE, 0x7777)
    return m, Cpu(*r, sp, CS, ENTRY, ds, 0x1111, ss, flags), (pending, key)


def stub(before, case):
    """The declared INT 16h stub: AH=01h -> AX := key, ZF := not pending; nothing else changes."""
    pending, key = case
    after = Cpu.from_buffer_copy(before)
    after.ax = key; after.flags = (before.flags & ~0x0040) | (0 if pending else 0x0040); after.ip = AFTER_IP
    return after


def compare(name, state, decoded, f):
    from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE, UC_HOOK_INTR
    from unicorn import x86_const as R
    m, c, case = state
    uc = Uc(UC_ARCH_X86, UC_MODE_16); backing = (ctypes.c_uint8 * SIZE).from_buffer_copy(m)
    uc.mem_map_ptr(0, SIZE, 7, ctypes.addressof(backing)); uc.mem_map_ptr(SIZE, SIZE, 7, ctypes.addressof(backing))
    for (n, _), reg in zip(Cpu._fields_, REGISTERS):
        uc.reg_write(reg, getattr(c, n))
    s = dict(stop=None, ret=False, ints=0, prev=None); seen = set(); edges = set()

    def code(em, address, size, user):
        ip = uc.reg_read(R.UC_X86_REG_IP); cs = uc.reg_read(R.UC_X86_REG_CS)
        if s['prev'] in CONDITIONALS:
            edges.add((s['prev'], ip))
        if s['ret']:
            s['stop'] = 0; return uc.emu_stop()
        check(cs == CS and ip in decoded, f'escape {name} {cs:04x}:{ip:04x}')
        seen.add(ip); s['prev'] = ip
        if decoded[ip].mnemonic == 'retf':
            s['ret'] = True

    def intr(em, intno, user):
        ah = uc.reg_read(R.UC_X86_REG_AX) >> 8
        check(intno == 0x16 and ah == 0x01, f'ENV refusal {name}: INT {intno:#x} AH {ah:#x}')
        pending, key = case
        uc.reg_write(R.UC_X86_REG_AX, key)
        uc.reg_write(R.UC_X86_REG_EFLAGS, (uc.reg_read(R.UC_X86_REG_EFLAGS) & ~0x40) | (0 if pending else 0x40))
        s['ints'] += 1
    uc.hook_add(UC_HOOK_CODE, code); uc.hook_add(UC_HOOK_INTR, intr)
    uc.emu_start(physical(c.cs, c.ip), 2 * SIZE, count=100)
    check(s['stop'] == 0, f'original did not return {name}')
    actual = Cpu.from_buffer_copy(c); ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(m); calls = []

    def svc(ctx, before, after):
        calls.append(1); after[0] = stub(before[0], case); return 0
    status = f(ram, SIZE, ctypes.byref(actual), SERVICE_FN(svc), None)
    check(status == 0, f'C status {name}: {status}')
    check(len(calls) == s['ints'], f'service calls {name}: C {len(calls)} original {s["ints"]}')
    if bytes(ram) != bytes(backing):
        diff = [hex(i) for i, (a, b) in enumerate(zip(bytes(ram), bytes(backing))) if a != b]
        raise ValueError(f'RAM {name}: {diff[:8]} ({len(diff)} bytes)')
    for (n, _), reg in zip(Cpu._fields_, REGISTERS):
        want = uc.reg_read(reg) & 0xFFFF
        check(getattr(actual, n) == want, f'CPU {name}/{n} C {getattr(actual, n):04x} original {want:04x}')
    return seen, edges


def cases():
    rng = random.Random(0x0A4002FD)
    out = [(f'buf{b:02x}-pend{p}', dict(buffered=b, pending=p)) for b in (0x00, 0x01, 0x03, 0x80, 0xFF) for p in (0, 1)]
    out += [(f'flags{f:x}-{label}', dict(flags=f, **o)) for f in FLAGS
            for label, o in (('none', dict(pending=0)), ('key', dict(pending=1)), ('buffered', dict(buffered=0x3B)))]
    out += [(f'key{k:04x}', dict(pending=1, key=k)) for k in (0x0000, 0x011B, 0x3B00, 0xFFFF)]
    out += [(f'sp{sp:x}', dict(sp=sp)) for sp in (0x0000, 0x0001, 0x8001, 0xFFFB, 0xFFFC)]
    out += [('ss-ds', dict(ss=DS)), ('a20-stack', dict(ss=0xFFFF, sp=0x0400)), ('a20-ds', dict(ds=0xFFFF, buffered=0x41)),
            ('a20-ds-zero', dict(ds=0xFFFF, pending=1))]
    for k in range(32):
        out.append((f'random{k}', dict(buffered=rng.choice((0, 0, rng.randrange(256))), pending=rng.randrange(2),
                                       key=rng.randrange(65536), flags=rng.choice(FLAGS),
                                       regs=tuple(rng.randrange(65536) for _ in range(7)))))
    return out


def differential(body, decoded, f):
    seen, edges = set(), set(); specs = cases()
    for name, options in specs:
        v, e = compare(name, build(body, **options), decoded, f); seen |= v; edges |= e
    check(seen == set(decoded), f'starts missing {sorted(set(decoded) - seen)}')
    want = {(site, t) for site, ts in CONDITIONALS.items() for t in ts}
    check(edges == want, f'outcomes missing {sorted(want - edges)}')
    return dict(cases=len(specs), stops=dict(RETURN=len(specs)), starts=len(seen), starts_total=len(decoded),
                conditional_outcomes=len(edges), conditional_outcomes_total=len(want), cpu_words=14, ram_bytes=SIZE, flag_masks=0)


TOP, LOW, WINDOW = 0xF000, 0xEC00, 0x0400


def native(body, f):
    """DOSBox-X: per case, an INT 16h hook stub answers AH=01h with the case's key word and ZF (declared stub)."""
    directory = ROOT / 'resources/crt-keypressed/native'; directory.mkdir(parents=True, exist_ok=True)
    (directory / 'body.bin').write_bytes(body)
    specs = [(n, o) for n, o in cases() if not n.startswith(('sp', 'a20', 'ss-ds', 'random'))]
    rows = [(o.get('flags', 2), o.get('buffered', 0), o.get('pending', 0), o.get('key', 0x1E61)) for _, o in specs]
    t = ['.code16', '.intel_syntax noprefix', '.global _start', '_start: jmp main', '.org 0x200', 'main:',
         'mov ax,cs', 'mov ds,ax', 'mov ss,ax', f'mov sp,{TOP}', 'cld',
         f'mov ax,{CS}', 'mov es,ax', f'mov di,{ENTRY}', 'mov si,offset body', f'mov cx,{len(body)}', 'rep movsb',
         'xor ax,ax', 'mov es,ax', 'mov ax,word ptr es:[0x58]', 'mov word ptr [old16],ax', 'mov ax,word ptr es:[0x5a]',
         'mov word ptr [old16+2],ax',
         'mov dx,offset outname', 'xor cx,cx', 'mov ah,0x3c', 'int 0x21', 'jc failure', 'mov word ptr [handle],ax',
         'mov word ptr [header],cs', 'mov word ptr [header+2],offset returned', f'mov word ptr [header+4],{len(rows)}',
         'mov bx,word ptr [handle]', 'mov dx,offset header', 'mov cx,6', 'mov ah,0x40', 'int 0x21',
         'again:', 'cli', 'cld', 'mov ax,cs', 'mov ds,ax', 'mov es,ax', 'mov al,0x5a', f'mov di,{LOW}', f'mov cx,{WINDOW}',
         'rep stosb', f'mov ax,{DS}', 'mov es,ax', 'mov di,0x8500', 'mov cx,0x0010', 'mov al,0x5a', 'rep stosb',
         'mov si,word ptr cs:[index]', 'shl si,1', 'shl si,1', 'shl si,1', 'add si,offset table',  # 8 bytes/row
         'mov al,byte ptr cs:[si+2]', 'mov byte ptr es:[0x8501],al',
         'mov al,byte ptr cs:[si+4]', 'mov byte ptr cs:[case_pending],al', 'mov ax,word ptr cs:[si+6]', 'mov word ptr cs:[case_ax],ax',
         'xor ax,ax', 'mov es,ax', 'mov word ptr es:[0x58],offset int16', 'mov word ptr es:[0x5a],cs',
         f'mov sp,{TOP}', 'push word ptr cs:[si]', f'mov ax,{DS}', 'mov ds,ax', 'mov ax,0x1111', 'mov es,ax',
         'mov ax,0x1234', 'mov bx,0x3456', 'mov cx,0x5678', 'mov dx,0x789a', 'mov si,0x9abc', 'mov di,0xbcde', 'mov bp,0x7777',
         'popf', f'lcall {CS},{ENTRY}', 'returned:']
    t += [f'mov word ptr cs:[out+{2 * n}],{r}' for n, r in enumerate('ax bx cx dx si di bp sp cs'.split())]
    t += ['mov word ptr cs:[out+18],offset returned', 'mov word ptr cs:[out+20],ds', 'mov word ptr cs:[out+22],es',
          'mov word ptr cs:[out+24],ss', 'mov sp,0xf800', 'pushf', 'pop ax', 'mov word ptr cs:[out+26],ax',
          'mov ax,cs', 'mov ds,ax', 'xor ax,ax', 'mov es,ax', 'mov ax,word ptr [old16]', 'mov word ptr es:[0x58],ax',
          'mov ax,word ptr [old16+2]', 'mov word ptr es:[0x5a],ax', 'sti',
          'mov bx,word ptr [handle]', 'mov dx,offset out', 'mov cx,28', 'mov ah,0x40', 'int 0x21',
          'mov bx,word ptr [handle]', f'mov dx,{LOW}', f'mov cx,{WINDOW}', 'mov ah,0x40', 'int 0x21',
          f'mov ax,{DS}', 'mov ds,ax', 'mov bx,word ptr cs:[handle]', 'mov dx,0x8500', 'mov cx,0x0010', 'mov ah,0x40', 'int 0x21',
          'push cs', 'pop ds', 'inc word ptr [index]', f'cmp word ptr [index],{len(rows)}', 'jae done', 'jmp again', 'done:',
          'mov bx,word ptr [handle]', 'mov ah,0x3e', 'int 0x21', 'mov ax,0x4c00', 'int 0x21', 'failure:', 'mov ax,0x4c01', 'int 0x21',
          # Declared stub: AH=01h -> AX := case key, ZF in the IRET frame := not pending; other AH chain to the BIOS.
          'int16:', 'cmp ah,1', 'jne chain', 'mov ax,word ptr cs:[case_ax]', 'push bp', 'mov bp,sp', 'and word ptr [bp+6],0xffbf',
          'test byte ptr cs:[case_pending],1', 'jnz pending', 'or word ptr [bp+6],0x40', 'pending:', 'pop bp', 'iret',
          'chain:', 'jmp dword ptr cs:[old16]',
          'outname: .asciz "RESULT.BIN"', 'handle: .word 0', 'index: .word 0', 'header: .space 6', 'out: .space 28',
          'old16: .word 0,0', 'case_ax: .word 0', 'case_pending: .byte 0', '.align 2', 'table:']
    t += ['.word ' + ','.join(str(v) for v in row) for row in rows]
    t += ['body:', f'.incbin "{directory / "body.bin"}"']
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
    data = out.read_bytes(); record = 28 + WINDOW + 0x10
    check(len(data) == 6 + len(rows) * record, f'native length {len(data)}')
    seg, ret_ip, count = struct.unpack_from('<3H', data); check(count == len(rows), 'native count')
    for n, ((name, o), (flags, buffered, pending, key)) in enumerate(zip(specs, rows)):
        rec = data[6 + n * record:6 + (n + 1) * record]
        m = bytearray(SIZE)
        at = physical(CS, ENTRY); m[at:at + len(body)] = body
        m[physical(seg, LOW):physical(seg, LOW) + WINDOW] = b'\x5a' * WINDOW
        m[physical(DS, 0x8500):physical(DS, 0x8510)] = b'\x5a' * 0x10
        m[physical(DS, 0x8501)] = buffered
        sp = TOP - 4
        put16(m, seg, sp, ret_ip); put16(m, seg, sp + 2, seg)
        c = Cpu(0x1234, 0x3456, 0x5678, 0x789A, 0x9ABC, 0xBCDE, 0x7777, sp, CS, ENTRY, DS, 0x1111, seg, flags)
        ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(m)

        def svc(ctx, before, after, case=(pending, key)):
            after[0] = stub(before[0], case); return 0
        check(f(ram, SIZE, ctypes.byref(c), SERVICE_FN(svc), None) == 0, f'native C status {name}')
        for (reg, _), want in zip(Cpu._fields_, struct.unpack_from('<14H', rec)):
            check(getattr(c, reg) == want, f'native CPU {name}/{reg}: C {getattr(c, reg):04x} DOS {want:04x}')
        # Stack window from the INT-time SP (= entry SP) upward; the stub's IRET frame below it is BIOS-side (stated limit).
        window = bytes(ram[physical(seg, LOW):physical(seg, LOW) + WINDOW]); lo = sp - LOW
        check(window[lo:] == rec[28 + lo:28 + WINDOW], f'native stack {name}')
        check(bytes(ram[physical(DS, 0x8500):physical(DS, 0x8510)]) == rec[28 + WINDOW:], f'native data {name}')
    probe = subprocess.run([binary, '--version'], capture_output=True, text=True, env=dict(os.environ, SDL_VIDEODRIVER='dummy'))
    version = next(line.split(',')[0].strip() for line in (probe.stdout + probe.stderr).splitlines() if 'DOSBox-X version' in line)
    return dict(cases=len(rows), cpu_words=14, stack_bytes_compared='from entry SP to F000', data_bytes=0x10, flag_masks=0,
                declared_stub='INT 16h AH=01h -> case AX, ZF in the IRET frame', result_sha256=sha256(data).hexdigest(),
                dosbox_x=dict(path=binary, sha256=sha256(open(binary, 'rb').read()).hexdigest(), version=version,
                              pinned_source='5a9c7c53481c2e933948039c33256776653f4faf', pinned_build_used=False))


def main():
    p = argparse.ArgumentParser()
    for flag in ('--compile-only', '--unicorn', '--native'):
        p.add_argument(flag, action='store_true')
    p.add_argument('--output', type=Path); a = p.parse_args()
    with tempfile.TemporaryDirectory(prefix='keypressed-') as tmp:
        f = library(Path(tmp))
        if a.compile_only:
            print('CHECKER_BUILD_OK'); return
        body, decoded = source()
        report = dict(target=dict(span='0A40:[02FD,030F)', image_span=[physical(0xA40, ENTRY), physical(0xA40, END)],
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
