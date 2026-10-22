"""DOSBox-X corroboration for resident 0464:08D4 (synthetic frames; declared setup in RES_0464_COMPOSE_08D4_CPU.md).

A COM program loads per-case DS, source tile set, destination object and aux-mask segments, places the original body at
8800:08D4 and the original SYSTEM Move at 6000:1685 (the two far-call segment words patched to 6000h, as in the Unicorn
fixture), pushes the eight argument words and calls far. After return it records CPU14, a 1 KiB stack window and the four
64 KiB segments. Nothing in the body or Move is stubbed; interrupts stay off around the call."""
import ctypes, os, struct, subprocess
from hashlib import sha256
from res0464_compose_08d4_cpu_audit import (ROOT, SIZE, CS, SYS, DS, SRC, DST, AUX, ENTRY, SEGWORDS, MOVE, FLAGS, Cpu, Bus, WRITE_FN,
                                            Diagnostic, build, check, physical, put16)
TOP, WINDOW, LOW = 0xF000, 1024, 0xEC00
SEGS = (DS, SRC, DST, AUX)
RECORD = 28 + WINDOW + len(SEGS) * 65536


def specifications():
    out = []
    for f in FLAGS:
        for m in (0, 1, 2, 3):
            out.append(dict(mode=m, flags=f))
    for m in (0, 1, 2):
        out += [dict(mode=m, flags=2, flag=fl, aux=ax, rows=r, sw=w, ext=e, x=x, y=y, item=it) for fl, ax, r, w, e, x, y, it in
                ((0, True, 2, 2, 'low', 3, 2, 1), (1, True, 3, 3, 'high', 0, 0, 0), (1, False, 2, 1, 'mixed', 5, 4, 2),
                 (1, True, 1, 0x20, 'mixed', 1, 1, 1), (0, True, 2, 2, 'high', 0, 0, 3))]
        out += [dict(mode=m, flags=2, flag=0, x=0, y=0, src=(0x9000, SRC), dst=(0x0000, DST))]
    return out


def fixture(case, code, segment, returned):
    m, c = build(code, ss=segment, sp=TOP - 0x14, **case)
    put16(m, segment, TOP - 0x14, returned); put16(m, segment, TOP - 0x12, segment)
    return m, c


def native(code, lib):
    directory = ROOT / 'resources/0464-08d4/native'; directory.mkdir(parents=True, exist_ok=True)
    specs = specifications(); data = bytearray()
    for case in specs:
        m, _ = fixture(case, code, 0x0814, 0)
        frame = struct.unpack_from('<8H', m, physical(0x0814, TOP - 0x14 + 4))   # dst off/seg, src off/seg, flag, item, y, x
        data += struct.pack('<9H', case.get('flags', 2), *frame) + b'\0' * 14
        for seg in SEGS:
            data += m[physical(seg, 0):physical(seg, 0) + 65536]
    (directory / 'INPUT.BIN').write_bytes(data)
    (directory / 'body.bin').write_bytes(bytes(code['body'])); (directory / 'move.bin').write_bytes(bytes(code['move']))
    t = ['.code16', '.intel_syntax noprefix', '.global _start', '_start: jmp main', '.org 0x200', 'main:',
         'mov ax,cs', 'mov ds,ax', 'mov ss,ax', f'mov sp,{TOP}', 'cld',
         f'mov ax,{CS}', 'mov es,ax', f'mov di,{ENTRY}', 'mov si,offset body', f'mov cx,{len(code["body"])}', 'rep movsb']
    for w in SEGWORDS:
        t.append(f'mov word ptr es:[{w}],{SYS}')
    t += [f'mov ax,{SYS}', 'mov es,ax', f'mov di,{MOVE}', 'mov si,offset move', f'mov cx,{len(code["move"])}', 'rep movsb',
          'mov dx,offset inputname', 'mov ax,0x3d00', 'int 0x21', 'jc failure', 'mov word ptr [inputhandle],ax',
          'mov dx,offset outputname', 'xor cx,cx', 'mov ah,0x3c', 'int 0x21', 'jc failure', 'mov word ptr [outputhandle],ax',
          'mov word ptr [0xd800],cs', 'mov word ptr [0xd802],offset returned', f'mov word ptr [0xd804],{len(specs)}',
          'mov dx,0xd800', 'mov cx,6', 'call write_output',
          'again:', 'push cs', 'pop ds', 'mov bx,word ptr [inputhandle]', 'mov dx,offset spec', 'mov cx,32', 'mov ah,0x3f', 'int 0x21',
          'jc failure', 'cmp ax,32', 'jne failure']
    for seg in SEGS:
        for off in (0, 0x8000):
            t += [f'mov ax,{seg}', 'mov ds,ax', f'mov dx,{off}', 'mov cx,0x8000', 'call read_input']
    t += ['cli', 'push cs', 'pop ds', 'mov al,0x5a', 'push cs', 'pop es', f'mov di,{LOW}', f'mov cx,{WINDOW - 0x14}', 'rep stosb']
    t += [f'push word ptr cs:[spec+{2 + 2 * k}]' for k in range(7, -1, -1)]
    t += ['push word ptr cs:[spec]', 'mov ax,0x1111', 'mov es,ax', f'mov ax,{DS}', 'mov ds,ax',
          'mov ax,0x1234', 'mov bx,0x3456', 'mov cx,0x5678', 'mov dx,0x789a', 'mov si,0x9abc', 'mov di,0xbcde', 'mov bp,0x7777',
          'popf', f'lcall {CS},{ENTRY}', 'returned:']
    t += [f'mov word ptr cs:[{0xC000 + 2 * n}],{r}' for n, r in enumerate('ax bx cx dx si di bp sp cs'.split())]
    t += ['mov word ptr cs:[0xc012],offset returned', 'mov word ptr cs:[0xc014],ds', 'mov word ptr cs:[0xc016],es',
          'mov word ptr cs:[0xc018],ss', 'mov sp,0xf800', 'pushf', 'pop ax', 'mov word ptr cs:[0xc01a],ax',
          'mov ax,cs', 'mov ds,ax', 'mov es,ax', 'cld', f'mov si,{LOW}', 'mov di,0xc01c', f'mov cx,{WINDOW}', 'rep movsb',
          'mov sp,0xf800', 'sti', 'mov dx,0xc000', f'mov cx,{28 + WINDOW}', 'call write_output']
    for seg in SEGS:
        for off in (0, 0x8000):
            t += [f'mov ax,{seg}', 'mov ds,ax', f'mov dx,{off}', 'mov cx,0x8000', 'call write_output']
    t += ['push cs', 'pop ds', 'mov ss,word ptr cs:[0xd800]', f'mov sp,{TOP}', 'inc word ptr [index]',
          f'cmp word ptr [index],{len(specs)}', 'jae done', 'jmp again', 'done:',
          'mov bx,word ptr [outputhandle]', 'mov ah,0x3e', 'int 0x21', 'mov ax,0x4c00', 'int 0x21',
          'write_output:', 'mov bx,word ptr cs:[outputhandle]', 'mov ah,0x40', 'int 0x21', 'jc failure', 'cmp ax,cx', 'jne failure', 'ret',
          'read_input:', 'mov bx,word ptr cs:[inputhandle]', 'mov ah,0x3f', 'int 0x21', 'jc failure', 'cmp ax,cx', 'jne failure', 'ret',
          'failure:', 'mov ax,0x4c01', 'int 0x21',
          'inputname: .asciz "INPUT.BIN"', 'outputname: .asciz "RESULT.BIN"', 'inputhandle: .word 0', 'outputhandle: .word 0',
          'index: .word 0', 'spec: .space 32', 'body:', f'.incbin "{directory / "body.bin"}"', 'move:', f'.incbin "{directory / "move.bin"}"']
    (directory / 'oracle.S').write_text('\n'.join(t) + '\n')
    subprocess.run(['as', '--32', str(directory / 'oracle.S'), '-o', str(directory / 'oracle.o')], check=True)
    com = directory / 'ORACLE.COM'
    subprocess.run(['ld', '-m', 'elf_i386', '-Ttext', '0x100', '--oformat', 'binary', str(directory / 'oracle.o'), '-o', str(com)], check=True)
    check(com.stat().st_size + 0x100 < 0xC000, 'COM overlaps the output buffer')
    (directory / 'dosbox.conf').write_text('[sdl]\noutput=surface\n[dosbox]\nmemsize=16\n[cpu]\ncore=normal\ncputype=386\n'
                                          'cycles=max\n[mixer]\nnosound=true\n[midi]\nmididevice=none\n[autoexec]\nmount c '
                                          + str(directory.resolve()) + '\nc:\nORACLE.COM\nexit\n')
    out = directory / 'RESULT.BIN'
    if out.exists():
        out.unlink()
    with (directory / 'dosbox.log').open('w') as log:
        subprocess.run(['/usr/bin/dosbox-x', '-conf', str(directory / 'dosbox.conf'), '-fastlaunch', '-exit', '-noconsole'],
                       env=dict(os.environ, SDL_VIDEODRIVER='dummy', SDL_AUDIODRIVER='dummy'), stdout=log, stderr=subprocess.STDOUT,
                       check=True, timeout=900)
    result = out.read_bytes()
    check(len(result) == 6 + len(specs) * RECORD, f'native output length {len(result)}')
    segment, returned, count = struct.unpack_from('<3H', result); check(count == len(specs), 'native count')
    for n, case in enumerate(specs):
        rec = result[6 + n * RECORD:6 + (n + 1) * RECORD]
        m, c = fixture(case, code, segment, returned)
        ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(m)
        bus = Bus(ram, None, None, WRITE_FN(), None); d = Diagnostic()
        status = lib.pr_r0464_compose_08d4_cpu(ctypes.byref(bus), SIZE, ctypes.byref(c), 1 << 20, ctypes.byref(d))
        check(status == 0, f'native C status {n}: {status}')
        for (reg, _), want in zip(Cpu._fields_, struct.unpack_from('<14H', rec)):
            check(getattr(c, reg) == want, f'native CPU {n}/{reg}: C {getattr(c, reg):04x} DOS {want:04x}')
        mine = bytes(ram[physical(segment, LOW):physical(segment, LOW) + WINDOW])
        for seg in SEGS:
            mine += bytes(ram[physical(seg, 0):physical(seg, 0) + 65536])
        if mine != rec[28:]:
            diff = [i for i, (a, b) in enumerate(zip(mine, rec[28:])) if a != b]
            raise ValueError(f'native memory {n}: first {[hex(i) for i in diff[:6]]} ({len(diff)} bytes)')
    binary = '/usr/bin/dosbox-x'
    probe = subprocess.run([binary, '--version'], capture_output=True, text=True, env=dict(os.environ, SDL_VIDEODRIVER='dummy', SDL_AUDIODRIVER='dummy'))
    version = next(line.split(',')[0].strip() for line in (probe.stdout + probe.stderr).splitlines() if 'DOSBox-X version' in line)
    return dict(cases=len(specs), dosbox_x=dict(path=binary, sha256=sha256(open(binary, 'rb').read()).hexdigest(), version=version),
                cpu_words=14, stack_bytes=WINDOW, segment_bytes=len(SEGS) * 65536, flag_masks=0, core='normal', cpu='386',
                result_sha256=sha256(result).hexdigest(), com_segment=f'{segment:04X}')
