#!/usr/bin/env python3
"""Turbo Pascal Dos.GetTime 0A32:00A2 (raw CPU, runtime coverage, no identity claim) against the original bytes.

Unicorn: CPU14 + full 1 MiB, the INT 21h/2Ch service supplied by an interrupt hook (the declared environment input).
Native (DOSBox-X): an INT 21h hook stub returns a fixed time per case and IRETs (declared stub). Media and generated
programs stay under ignored resources/."""
import argparse, ctypes, json, os, random, struct, subprocess, sys, tempfile
from hashlib import sha256
from pathlib import Path
from capstone import Cs, CS_ARCH_X86, CS_MODE_16
from resident_loader_map import unpack
from res0709_ega_reset_0000_cpu_audit import Cpu, ROOT, SIZE, REGISTERS, physical, put16, check
CHECKER_ROWS = ('RES_0A32_00A2',)
ENTRY, END, INT_IP = 0x00A2, 0x00C7, 0x00A7
DIGEST = '175b7cf9fd00cb6dfa0db3b24bef93332f16cf3ca2c711c670f9c80333ced7fc'
CS, DS, SS, DATA = 0x6000, 0x5000, 0x2000, 0x3000
FLAGS = (0x0002, 0x00D7, 0x08D7, 0x0402, 0x0893, 0x0003)
SERVICE_FN = ctypes.CFUNCTYPE(ctypes.c_int, ctypes.c_void_p, ctypes.POINTER(Cpu), ctypes.POINTER(Cpu))


def source():
    image = bytes(unpack((ROOT / 'resources/DOS/POOLRAD/START.EXE').read_bytes())[0])
    body = image[physical(0xA32, ENTRY):physical(0xA32, END)]
    check(sha256(body).hexdigest() == DIGEST, 'GetTime identity')
    ins = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(body, ENTRY))
    check(len(ins) == 19 and [i.address for i in ins if i.mnemonic in ('ret', 'retf', 'iret')] == [0x00C4], 'first RETF / count')
    check([(i.address, i.op_str) for i in ins if i.mnemonic == 'int'] == [(INT_IP, '0x21')], 'single INT 21h')
    return body, {i.address: i for i in ins}


def library(directory):
    path = directory / 'gettime.so'
    subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Wpedantic', '-Werror', '-O2', '-fPIC', '-shared', '-I', str(ROOT / 'include'),
                    str(ROOT / 'src/recovered/dos_gettime_00a2_cpu.c'), '-o', str(path)], check=True)
    f = ctypes.CDLL(str(path)).pr_dos_gettime_00a2
    f.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t, ctypes.POINTER(Cpu), SERVICE_FN, ctypes.c_void_p]
    f.restype = ctypes.c_int
    return f


def build(body, *, time=(13, 42, 7, 55), ptrs=None, ss=SS, sp=0x8000, flags=2, ds=DS, background=0x5A, regs=None):
    """time = (hour, minute, second, sec100) as the DOS service returns them in CH, CL, DH, DL.
    ptrs = far (segment, offset) of Hour, Minute, Second, Sec100."""
    m = bytearray([background]) * SIZE
    at = physical(CS, ENTRY); m[at:at + len(body)] = body
    ptrs = ptrs or ((DATA, 0x0100), (DATA, 0x0110), (DATA, 0x0120), (DATA, 0x0130))
    hour, minute, second, sec100 = ptrs
    words = (0x7000, 0x9001, sec100[1], sec100[0], second[1], second[0], minute[1], minute[0], hour[1], hour[0])
    for j, w in enumerate(words):
        put16(m, ss, (sp + 2 * j) & 0xFFFF, w)
    r = regs or (0x1234, 0x3456, 0x5678, 0x789A, 0x9ABC, 0xBCDE, 0x7777)
    c = Cpu(*r, sp, CS, ENTRY, ds, 0x1111, ss, flags)
    return m, c, time


def service_values(time):
    hour, minute, second, sec100 = time
    return ((hour & 255) << 8) | (minute & 255), ((second & 255) << 8) | (sec100 & 255)


def compare(name, state, body, decoded, f):
    from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE, UC_HOOK_INTR
    from unicorn import x86_const as R
    m, c, time = state
    cx, dx = service_values(time)
    uc = Uc(UC_ARCH_X86, UC_MODE_16); backing = (ctypes.c_uint8 * SIZE).from_buffer_copy(m)
    uc.mem_map_ptr(0, SIZE, 7, ctypes.addressof(backing)); uc.mem_map_ptr(SIZE, SIZE, 7, ctypes.addressof(backing))
    for (n, _), reg in zip(Cpu._fields_, REGISTERS):
        uc.reg_write(reg, getattr(c, n))
    s = dict(stop=None, ret=False, ints=0); seen = set()

    def code(em, address, size, user):
        ip = uc.reg_read(R.UC_X86_REG_IP); cs = uc.reg_read(R.UC_X86_REG_CS)
        if s['ret']:
            s['stop'] = 0; return uc.emu_stop()
        check(cs == CS and ip in decoded, f'escape {name} {cs:04x}:{ip:04x}')
        seen.add(ip)
        if decoded[ip].mnemonic == 'retf':
            s['ret'] = True

    def intr(em, intno, user):
        check(intno == 0x21 and (uc.reg_read(R.UC_X86_REG_AX) >> 8) == 0x2C, f'unexpected interrupt {name}')
        uc.reg_write(R.UC_X86_REG_CX, cx); uc.reg_write(R.UC_X86_REG_DX, dx); s['ints'] += 1
    uc.hook_add(UC_HOOK_CODE, code); uc.hook_add(UC_HOOK_INTR, intr)
    uc.emu_start(physical(c.cs, c.ip), 2 * SIZE, count=1000)
    check(s['stop'] == 0 and s['ints'] == 1, f'original did not return once {name}')
    actual = Cpu.from_buffer_copy(c); ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(m)

    def svc(ctx, before, after):
        after[0] = Cpu.from_buffer_copy(before[0]); after[0].cx = cx; after[0].dx = dx; after[0].ip = 0x00A9; return 0
    status = f(ram, SIZE, ctypes.byref(actual), SERVICE_FN(svc), None)
    check(status == 0, f'C status {name}: {status}')
    if bytes(ram) != bytes(backing):
        diff = [hex(i) for i, (a, b) in enumerate(zip(bytes(ram), bytes(backing))) if a != b]
        raise ValueError(f'RAM {name}: {diff[:8]} ({len(diff)} bytes)')
    for (n, _), reg in zip(Cpu._fields_, REGISTERS):
        want = uc.reg_read(reg) & 0xFFFF
        check(getattr(actual, n) == want, f'CPU {name}/{n} C {getattr(actual, n):04x} original {want:04x}')
    return seen, actual, bytes(ram)


def cases():
    rng = random.Random(0x0A3200A2)
    out = [(f'hour{h}', dict(time=(h, (h * 7) % 60, (h * 13) % 60, (h * 17) % 100))) for h in range(24)]
    out += [(f'edge-{t}', dict(time=t)) for t in ((0, 0, 0, 0), (23, 59, 59, 99), (255, 255, 255, 255), (24, 60, 60, 100))]
    out += [(f'flags{f:x}', dict(flags=f)) for f in FLAGS]
    out += [(f'sp{sp:x}', dict(sp=sp)) for sp in (0x0002, 0x0003, 0x8001, 0xFFEB, 0xFFEC)]
    out += [('ss-ds', dict(ss=DS)), ('a20-stack', dict(ss=0xFFFF, sp=0x0400)),
            ('same-target', dict(ptrs=((DATA, 0x0100),) * 4)),
            ('overlapping-targets', dict(ptrs=((DATA, 0x0100), (DATA, 0x0101), (DATA, 0x0102), (DATA, 0x0103)))),
            ('straddle-ffff', dict(ptrs=((DATA, 0xFFFF), (DATA, 0x0110), (DATA, 0x0120), (DATA, 0xFFFF)))),
            ('a20-target', dict(ptrs=((0xFFFF, 0x0020), (DATA, 0x0110), (DATA, 0x0120), (0xFFFF, 0x0030)))),
            # Sec100 is stored first, through [BP+6]; aim it at the Hour pointer's offset word [BP+12h], which the last
            # LES rereads: the Hour store then follows the rewritten pointer (entry SP 8000h, BP = 7FFEh).
            ('alias-sec100-onto-hour-ptr', dict(ptrs=((DATA, 0x0100), (DATA, 0x0110), (DATA, 0x0120), (SS, 0x8010)))),
            ('alias-onto-return', dict(ptrs=((DATA, 0x0100), (DATA, 0x0110), (DATA, 0x0120), (SS, 0x8000)))),
            ('df-set', dict(flags=0x0402))]
    for k in range(32):
        out.append((f'random{k}', dict(time=tuple(rng.randrange(256) for _ in range(4)), flags=rng.choice(FLAGS),
                                       regs=tuple(rng.randrange(65536) for _ in range(7)),
                                       ptrs=tuple((rng.choice((DATA, 0x4000, 0x7000)), rng.randrange(0, 0xFFFF)) for _ in range(4)))))
    return out


def differential(body, decoded, f):
    seen = set(); specs = cases()
    for name, options in specs:
        state = build(body, **options)
        v, final, ram = compare(name, state, body, decoded, f); seen |= v
        if name == 'alias-onto-return':  # the Sec100 store rewrote the return IP
            check((final.cs, final.ip) != (0x9001, 0x7000), f'fixture {name} did not redirect the return')
        if name == 'alias-sec100-onto-hour-ptr':  # Hour went through the rewritten pointer, not to DATA:0100
            hour_at = physical(DATA, service_values(options.get('time', (13, 42, 7, 55)))[1] & 0xFF)
            check(ram[physical(DATA, 0x100)] == 0x5A and ram[hour_at] == 13, f'fixture {name} did not follow the rewritten pointer')
    check(seen == set(decoded), f'starts missing {sorted(set(decoded) - seen)}')
    return dict(cases=len(specs), stops=dict(RETURN=len(specs)), starts=len(seen), starts_total=len(decoded),
                conditional_outcomes_total=0, cpu_words=14, ram_bytes=SIZE, flag_masks=0)


TOP, LOW, WINDOW = 0xF000, 0xEC00, 0x0400


def native(body, f):
    """DOSBox-X: per case, an INT 21h hook stub answers AH=2Ch with the case's CX/DX and IRETs (declared stub)."""
    directory = ROOT / 'resources/dos-gettime/native'; directory.mkdir(parents=True, exist_ok=True)
    (directory / 'body.bin').write_bytes(body)
    specs = [(n, o) for n, o in cases() if not n.startswith(('sp', 'a20', 'ss-ds', 'random', 'alias', 'straddle', 'same', 'overlapping'))]
    rows = []
    for name, o in specs:
        cx, dx = service_values(o.get('time', (13, 42, 7, 55)))
        rows.append((o.get('flags', 2), cx, dx))
    t = ['.code16', '.intel_syntax noprefix', '.global _start', '_start: jmp main', '.org 0x200', 'main:',
         'mov ax,cs', 'mov ds,ax', 'mov ss,ax', f'mov sp,{TOP}', 'cld',
         f'mov ax,{CS}', 'mov es,ax', f'mov di,{ENTRY}', 'mov si,offset body', f'mov cx,{len(body)}', 'rep movsb',
         'xor ax,ax', 'mov es,ax', 'mov ax,word ptr es:[0x84]', 'mov word ptr [old21],ax', 'mov ax,word ptr es:[0x86]',
         'mov word ptr [old21+2],ax',
         'mov dx,offset outname', 'xor cx,cx', 'mov ah,0x3c', 'int 0x21', 'jc failure', 'mov word ptr [handle],ax',
         'mov word ptr [header],cs', 'mov word ptr [header+2],offset returned', f'mov word ptr [header+4],{len(rows)}',
         'mov bx,word ptr [handle]', 'mov dx,offset header', 'mov cx,6', 'mov ah,0x40', 'int 0x21',
         'again:', 'cli', 'cld', 'mov ax,cs', 'mov ds,ax', 'mov es,ax', 'mov al,0x5a', f'mov di,{LOW}', f'mov cx,{WINDOW - 0x14}',
         'rep stosb', f'mov ax,{DATA}', 'mov es,ax', 'mov di,0x0100', 'mov cx,0x0040', 'mov al,0x5a', 'rep stosb',
         'mov si,word ptr cs:[index]', 'mov ax,si', 'shl si,1', 'add si,ax', 'shl si,1', 'add si,offset table',  # 6 bytes/row
         'mov ax,word ptr cs:[si+2]', 'mov word ptr cs:[case_cx],ax', 'mov ax,word ptr cs:[si+4]', 'mov word ptr cs:[case_dx],ax',
         'xor ax,ax', 'mov es,ax', 'mov word ptr es:[0x84],offset int21', 'mov word ptr es:[0x86],cs',
         f'mov sp,{TOP}', f'push {DATA}', 'push 0x0100', f'push {DATA}', 'push 0x0110', f'push {DATA}', 'push 0x0120',
         f'push {DATA}', 'push 0x0130', 'push word ptr cs:[si]', f'mov ax,{DS}', 'mov ds,ax', 'mov ax,0x1111', 'mov es,ax',
         'mov ax,0x1234', 'mov bx,0x3456', 'mov cx,0x5678', 'mov dx,0x789a', 'mov si,0x9abc', 'mov di,0xbcde', 'mov bp,0x7777',
         'popf', f'lcall {CS},{ENTRY}', 'returned:']
    t += [f'mov word ptr cs:[out+{2 * n}],{r}' for n, r in enumerate('ax bx cx dx si di bp sp cs'.split())]
    t += ['mov word ptr cs:[out+18],offset returned', 'mov word ptr cs:[out+20],ds', 'mov word ptr cs:[out+22],es',
          'mov word ptr cs:[out+24],ss', 'mov sp,0xf800', 'pushf', 'pop ax', 'mov word ptr cs:[out+26],ax',
          'mov ax,cs', 'mov ds,ax', 'xor ax,ax', 'mov es,ax', 'mov ax,word ptr [old21]', 'mov word ptr es:[0x84],ax',
          'mov ax,word ptr [old21+2]', 'mov word ptr es:[0x86],ax', 'sti',
          'mov bx,word ptr [handle]', 'mov dx,offset out', 'mov cx,28', 'mov ah,0x40', 'int 0x21',
          'mov bx,word ptr [handle]', f'mov dx,{LOW}', f'mov cx,{WINDOW}', 'mov ah,0x40', 'int 0x21',
          f'mov ax,{DATA}', 'mov ds,ax', 'mov bx,word ptr cs:[handle]', 'mov dx,0x0100', 'mov cx,0x0040', 'mov ah,0x40', 'int 0x21',
          'push cs', 'pop ds', 'inc word ptr [index]', f'cmp word ptr [index],{len(rows)}', 'jae done', 'jmp again', 'done:',
          'mov bx,word ptr [handle]', 'mov ah,0x3e', 'int 0x21', 'mov ax,0x4c00', 'int 0x21', 'failure:', 'mov ax,0x4c01', 'int 0x21',
          # Declared stub: AH=2Ch answers with the case's CX/DX and returns by IRET (flags as pushed); else chains.
          'int21:', 'cmp ah,0x2c', 'jne chain', 'mov cx,word ptr cs:[case_cx]', 'mov dx,word ptr cs:[case_dx]', 'iret',
          'chain:', 'jmp dword ptr cs:[old21]',
          'outname: .asciz "RESULT.BIN"', 'handle: .word 0', 'index: .word 0', 'header: .space 6', 'out: .space 28',
          'old21: .word 0,0', 'case_cx: .word 0', 'case_dx: .word 0', 'table:']
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
    data = out.read_bytes(); record = 28 + WINDOW + 0x40
    check(len(data) == 6 + len(rows) * record, f'native length {len(data)}')
    seg, ret_ip, count = struct.unpack_from('<3H', data); check(count == len(rows), 'native count')
    for n, ((name, o), (flags, cx, dx)) in enumerate(zip(specs, rows)):
        rec = data[6 + n * record:6 + (n + 1) * record]
        m = bytearray(SIZE)
        at = physical(CS, ENTRY); m[at:at + len(body)] = body
        m[physical(seg, LOW):physical(seg, LOW) + WINDOW - 0x14] = b'\x5a' * (WINDOW - 0x14)
        m[physical(DATA, 0x100):physical(DATA, 0x140)] = b'\x5a' * 0x40
        sp = TOP - 0x14
        for j, w in enumerate((ret_ip, seg, 0x0130, DATA, 0x0120, DATA, 0x0110, DATA, 0x0100, DATA)):
            put16(m, seg, sp + 2 * j, w)
        c = Cpu(0x1234, 0x3456, 0x5678, 0x789A, 0x9ABC, 0xBCDE, 0x7777, sp, CS, ENTRY, DS, 0x1111, seg, flags)
        ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(m)

        def svc(ctx, before, after, cx=cx, dx=dx):
            after[0] = Cpu.from_buffer_copy(before[0]); after[0].cx = cx; after[0].dx = dx; after[0].ip = 0x00A9; return 0
        check(f(ram, SIZE, ctypes.byref(c), SERVICE_FN(svc), None) == 0, f'native C status {n}')
        for (reg, _), want in zip(Cpu._fields_, struct.unpack_from('<14H', rec)):
            check(getattr(c, reg) == want, f'native CPU {name}/{reg}: C {getattr(c, reg):04x} DOS {want:04x}')
        # Stack window from the INT-time SP (entry SP - 2) upward; the stub's IRET frame below it is DOS's (stated limit).
        int_sp = sp - 2
        window = bytes(ram[physical(seg, LOW):physical(seg, LOW) + WINDOW]); native_window = rec[28:28 + WINDOW]
        lo = int_sp - LOW
        check(window[lo:] == native_window[lo:], f'native stack {name}')
        check(bytes(ram[physical(DATA, 0x100):physical(DATA, 0x140)]) == rec[28 + WINDOW:], f'native data {name}')
    probe = subprocess.run([binary, '--version'], capture_output=True, text=True, env=dict(os.environ, SDL_VIDEODRIVER='dummy'))
    version = next(line.split(',')[0].strip() for line in (probe.stdout + probe.stderr).splitlines() if 'DOSBox-X version' in line)
    return dict(cases=len(rows), cpu_words=14, stack_bytes_compared='from INT-time SP to F000', data_bytes=0x40, flag_masks=0,
                declared_stub='INT 21h AH=2Ch -> case CX/DX, IRET', result_sha256=sha256(data).hexdigest(),
                dosbox_x=dict(path=binary, sha256=sha256(open(binary, 'rb').read()).hexdigest(), version=version,
                              pinned_source='5a9c7c53481c2e933948039c33256776653f4faf', pinned_build_used=False))


def main():
    p = argparse.ArgumentParser()
    for flag in ('--compile-only', '--unicorn', '--native'):
        p.add_argument(flag, action='store_true')
    p.add_argument('--output', type=Path); a = p.parse_args()
    with tempfile.TemporaryDirectory(prefix='gettime-') as tmp:
        f = library(Path(tmp))
        if a.compile_only:
            print('CHECKER_BUILD_OK'); return
        body, decoded = source()
        report = dict(target=dict(span='0A32:[00A2,00C7)', image_span=[physical(0xA32, ENTRY), physical(0xA32, END)],
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
