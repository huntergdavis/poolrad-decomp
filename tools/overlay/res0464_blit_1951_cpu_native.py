"""DOSBox-X EGA corroboration for resident 0464:1951 (synthetic frames; declared setup in RES_0464_BLIT_1951_CPU.md).

A COM program loads per-case DS, bitmap and row-target segments, sets EGA mode 0Dh and fills the four planes,
places the original body at 8800:1951 and the original SYSTEM Move at 6000:1685 (the two far-call segment words
patched to 6000h, as in the Unicorn fixture), pushes the four argument words and calls far. After return it
records CPU14, a 1 KiB stack window, the three
64 KiB segments and the four planes. Nothing in the body or Move is stubbed. OUT order is not traced natively;
its effect is compared through the planes (EGA sequencer registers are write-only, so the final map mask cannot be read back)."""
import os, struct, subprocess
from hashlib import sha256
from res0464_blit_1951_cpu_audit import (ROOT, SIZE, CS, SYS, DS, BMP, ENTRY, SEGWORDS, MOVE, FLAGS, Cpu, Ega, Bus,
                                         Env, OUT_FN, IN_FN, WRITE_FN, Diagnostic, build, check, physical, put16)
import ctypes
ROWSEG = 0x3000
TOP, WINDOW, LOW = 0xF000, 1024, 0xEC00
RECORD = 28 + 2 + WINDOW + 3 * 65536 + 4 * 65536


def specifications():
    out = []
    for n, f in enumerate(FLAGS):
        for m in (0, 1, 2, 3):
            out.append(dict(mode=m, flags=f, rows=2, cols=2, x=1, y=3))
    out += [dict(mode=m, flags=2, rows=r, cols=c, x=x, y=y) for m in (0, 1, 2)
            for (r, c, x, y) in ((0, 2, 1, 3), (2, 0, 1, 3), (1, 1, 0, 0), (3, 3, 2, 5), (2, 2, 0xFFFF, 1))]
    out += [dict(mode=1, flags=2, rows=2, cols=2, x=1, y=3, bmp=(0, 0))]
    return out


def fixture(case, code, segment, returned):
    options = dict(case)
    m, c = build(code, row_seg=ROWSEG, ss=segment, sp=TOP - 0x0C, **options)
    put16(m, segment, TOP - 0x0C, returned); put16(m, segment, TOP - 0x0A, segment)
    return m, c


def native(code, lib):
    directory = ROOT / 'resources/blit1951/native'; directory.mkdir(parents=True, exist_ok=True)
    specs = specifications()
    data = bytearray()
    for case in specs:
        m, c = fixture(case, code, 0x0814, 0)
        bmp = case.get('bmp', (0x0100, BMP))
        data += struct.pack('<8H', case['flags'], bmp[0], bmp[1], case['x'], case['y'], 0, 0, 0)
        for seg in (DS, BMP, ROWSEG):
            data += m[physical(seg, 0):physical(seg, 0) + 65536]
    (directory / 'INPUT.BIN').write_bytes(data)
    (directory / 'body.bin').write_bytes(bytes(code['body'])); (directory / 'move.bin').write_bytes(bytes(code['move']))
    t = ['.code16', '.intel_syntax noprefix', '.global _start', '_start: jmp main', '.org 0x200', 'main:',
         'mov ax,cs', 'mov ds,ax', 'mov ss,ax', f'mov sp,{TOP}', 'mov ax,0x000d', 'int 0x10', 'cld',
         f'mov ax,{CS}', 'mov es,ax', f'mov di,{ENTRY}', 'mov si,offset body', f'mov cx,{len(code["body"])}', 'rep movsb']
    for w in SEGWORDS:
        t.append(f'mov word ptr es:[{w}],{SYS}')
    t += [f'mov ax,{SYS}', 'mov es,ax', f'mov di,{MOVE}', 'mov si,offset move', f'mov cx,{len(code["move"])}', 'rep movsb',
          'mov dx,offset inputname', 'mov ax,0x3d00', 'int 0x21', 'jc failure', 'mov word ptr [inputhandle],ax',
          'mov dx,offset outputname', 'xor cx,cx', 'mov ah,0x3c', 'int 0x21', 'jc failure', 'mov word ptr [outputhandle],ax',
          'mov word ptr [0xd800],cs', 'mov word ptr [0xd802],offset returned', f'mov word ptr [0xd804],{len(specs)}',
          'mov dx,0xd800', 'mov cx,6', 'call write_output',
          'again:', 'push cs', 'pop ds', 'mov bx,word ptr [inputhandle]', 'mov dx,offset spec', 'mov cx,16', 'mov ah,0x3f', 'int 0x21',
          'jc failure', 'cmp ax,16', 'jne failure']
    for seg in (DS, BMP, ROWSEG):
        for off in (0, 0x8000):
            t += [f'mov ax,{seg}', 'mov ds,ax', f'mov dx,{off}', 'mov cx,0x8000', 'call read_input']
    # Interrupts off from the stack-window fill until the record is taken: a timer IRQ during the plane fill
    # would run the BIOS handler on this stack and leave its pushes below the call frame.
    t += ['cli', 'push cs', 'pop ds', 'mov al,0x5a', 'push cs', 'pop es', f'mov di,{LOW}', f'mov cx,{WINDOW - 12}', 'rep stosb',
          # planes: write mode 0, all planes enabled per plane, pattern 31h + 27h*p
          'mov dx,0x3ce', 'mov ax,0x0000', 'out dx,ax', 'mov ax,0x0001', 'out dx,ax', 'mov ax,0x0003', 'out dx,ax',
          'mov ax,0x0005', 'out dx,ax', 'mov ax,0xff08', 'out dx,ax', 'mov ax,0xa000', 'mov es,ax']
    for p in range(4):
        t += ['mov dx,0x3c4', f'mov ax,{((1 << p) << 8) | 2}', 'out dx,ax', 'xor di,di', f'mov ax,{(0x31 + p * 0x27) * 257}',
              'mov cx,0x8000', 'rep stosw']
    t += ['mov dx,0x3c4', 'mov ax,0x0f02', 'out dx,ax', 'cli',
          'push word ptr cs:[spec+8]', 'push word ptr cs:[spec+6]', 'push word ptr cs:[spec+4]', 'push word ptr cs:[spec+2]',
          'push word ptr cs:[spec]', 'mov ax,0x1111', 'mov es,ax', f'mov ax,{DS}', 'mov ds,ax',
          'mov ax,0x1234', 'mov bx,0x3456', 'mov cx,0x5678', 'mov dx,0x789a', 'mov si,0x9abc', 'mov di,0xbcde', 'mov bp,0x7777',
          'popf', f'lcall {CS},{ENTRY}', 'returned:']
    t += [f'mov word ptr cs:[{0xC000 + 2 * n}],{r}' for n, r in enumerate('ax bx cx dx si di bp sp cs'.split())]
    t += ['mov word ptr cs:[0xc012],offset returned', 'mov word ptr cs:[0xc014],ds', 'mov word ptr cs:[0xc016],es',
          'mov word ptr cs:[0xc018],ss', 'mov sp,0xf800', 'pushf', 'pop ax', 'mov word ptr cs:[0xc01a],ax',
          'mov dx,0x3c4', 'mov al,2', 'out dx,al', 'inc dx', 'in al,dx', 'mov byte ptr cs:[0xc01c],al', 'mov byte ptr cs:[0xc01d],0',
          # no stack use and an explicit CLD before the window copy: the body may return with DF set
          'mov ax,cs', 'mov ds,ax', 'mov es,ax', 'cld', f'mov si,{LOW}', 'mov di,0xc01e', f'mov cx,{WINDOW}', 'rep movsb',
          'mov sp,0xf800', 'sti', 'mov dx,0xc000', f'mov cx,{30 + WINDOW}', 'call write_output']
    for seg in (DS, BMP, ROWSEG):
        for off in (0, 0x8000):
            t += [f'mov ax,{seg}', 'mov ds,ax', f'mov dx,{off}', 'mov cx,0x8000', 'call write_output']
    for p in range(4):
        t += ['mov dx,0x3ce', 'mov ax,0x0005', 'out dx,ax', f'mov ax,{(p << 8) | 4}', 'out dx,ax']
        for off in (0, 0x8000):
            t += ['mov ax,0xa000', 'mov ds,ax', 'mov ax,0x9000', 'mov es,ax', f'mov si,{off}', 'xor di,di', 'mov cx,0x8000', 'cld',
                  'rep movsb', 'mov ax,0x9000', 'mov ds,ax', 'xor dx,dx', 'mov cx,0x8000', 'call write_output']
    t += ['push cs', 'pop ds', 'mov ss,word ptr cs:[0xd800]', f'mov sp,{TOP}', 'inc word ptr [index]',
          f'cmp word ptr [index],{len(specs)}', 'jae done', 'jmp again', 'done:',
          'mov bx,word ptr [outputhandle]', 'mov ah,0x3e', 'int 0x21', 'mov ax,3', 'int 0x10', 'mov ax,0x4c00', 'int 0x21',
          'write_output:', 'mov bx,word ptr cs:[outputhandle]', 'mov ah,0x40', 'int 0x21', 'jc failure', 'cmp ax,cx', 'jne failure', 'ret',
          'read_input:', 'mov bx,word ptr cs:[inputhandle]', 'mov ah,0x3f', 'int 0x21', 'jc failure', 'cmp ax,cx', 'jne failure', 'ret',
          'failure:', 'mov ax,0x4c01', 'int 0x21',
          'inputname: .asciz "INPUT.BIN"', 'outputname: .asciz "RESULT.BIN"', 'inputhandle: .word 0', 'outputhandle: .word 0',
          'index: .word 0', 'spec: .space 16', 'body:', f'.incbin "{directory / "body.bin"}"', 'move:', f'.incbin "{directory / "move.bin"}"']
    (directory / 'oracle.S').write_text('\n'.join(t) + '\n')
    subprocess.run(['as', '--32', str(directory / 'oracle.S'), '-o', str(directory / 'oracle.o')], check=True)
    com = directory / 'ORACLE.COM'
    subprocess.run(['ld', '-m', 'elf_i386', '-Ttext', '0x100', '--oformat', 'binary', str(directory / 'oracle.o'), '-o', str(com)], check=True)
    check(com.stat().st_size + 0x100 < 0xC000, 'COM overlaps the output buffer')
    (directory / 'dosbox.conf').write_text('[sdl]\noutput=surface\n[dosbox]\nmachine=ega\nmemsize=16\n[cpu]\ncore=normal\ncputype=386\n'
                                          'cycles=max\n[mixer]\nnosound=true\n[midi]\nmididevice=none\n[autoexec]\nmount c '
                                          + str(directory.resolve()) + '\nc:\nORACLE.COM\nexit\n')
    out = directory / 'RESULT.BIN'
    if out.exists():
        out.unlink()
    with (directory / 'dosbox.log').open('w') as log:
        subprocess.run(['/usr/bin/dosbox-x', '-conf', str(directory / 'dosbox.conf'), '-fastlaunch', '-exit', '-noconsole'],
                       env=dict(os.environ, SDL_VIDEODRIVER='dummy', SDL_AUDIODRIVER='dummy'), stdout=log, stderr=subprocess.STDOUT,
                       check=True, timeout=600)
    result = out.read_bytes()
    check(len(result) == 6 + len(specs) * RECORD, f'native output length {len(result)}')
    segment, returned, count = struct.unpack_from('<3H', result); check(count == len(specs), 'native count')
    for n, case in enumerate(specs):
        rec = result[6 + n * RECORD:6 + (n + 1) * RECORD]
        m, c = fixture(case, code, segment, returned)
        ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(m); ega = Ega(); lib.pr_ega_reset(ctypes.byref(ega))
        for p in range(4):
            ctypes.memset(ctypes.addressof(ega.plane[p]), 0x31 + p * 0x27, 65536)
        ega.seq[2] = 0x0F
        env = Env(None, OUT_FN(lambda *a: 0), IN_FN()); bus = Bus(ram, ctypes.pointer(ega), ctypes.pointer(env), WRITE_FN(lambda *a: None), None)
        d = Diagnostic()
        status = lib.pr_r0464_blit_1951_cpu(ctypes.byref(bus), SIZE, ctypes.byref(c), 1 << 20, ctypes.byref(d))
        check(status == 0, f'native C status {n}: {status}')
        for (reg, _), want in zip(Cpu._fields_, struct.unpack_from('<14H', rec)):
            check(getattr(c, reg) == want, f'native CPU {n}/{reg}: C {getattr(c, reg):04x} DOS {want:04x}')
        # rec[28] is an IN from 3C5h: EGA sequencer registers are write-only (DOSBox-X machine=ega returns FFh),
        # so the final map mask is not natively observable and is not compared.
        mine = bytes(ram[physical(segment, LOW):physical(segment, LOW) + WINDOW])
        for seg in (DS, BMP, ROWSEG):
            mine += bytes(ram[physical(seg, 0):physical(seg, 0) + 65536])
        mine += bytes(ega)[:4 * 65536]
        if mine != rec[30:]:
            diff = [i for i, (a, b) in enumerate(zip(mine, rec[30:])) if a != b]
            raise ValueError(f'native memory {n}: first {[hex(i) for i in diff[:6]]} ({len(diff)} bytes)')
    binary = '/usr/bin/dosbox-x'
    probe = subprocess.run([binary, '--version'], capture_output=True, text=True,
                           env=dict(os.environ, SDL_VIDEODRIVER='dummy', SDL_AUDIODRIVER='dummy'))
    version = next(line.split(',')[0].strip() for line in (probe.stdout + probe.stderr).splitlines() if 'DOSBox-X version' in line)
    dosbox_x = dict(path=binary, sha256=sha256(open(binary, 'rb').read()).hexdigest(), version=version,
                    pinned_source='5a9c7c53481c2e933948039c33256776653f4faf', pinned_build_used=False)
    return dict(cases=len(specs), dosbox_x=dosbox_x, cpu_words=14, stack_bytes=WINDOW, segment_bytes=3 * 65536, plane_bytes=4 * 65536,
                flag_masks=0, machine='ega', video_mode='0Dh', core='normal', cpu='386', result_sha256=sha256(result).hexdigest(),
                com_segment=f'{segment:04X}')
