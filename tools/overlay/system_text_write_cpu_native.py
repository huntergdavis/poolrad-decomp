"""DOSBox-X corroboration for SYSTEM Write(string) 0919 and Writeln 084A; used by system_text_write_cpu_audit.py.

The relocated loader image is read once from IMAGE.BIN (so the put-character helper's DS word at 07EF is the loader's).
Per case the oracle COM loads the text segment (TextRec, buffer, string and the two real-byte driver stubs) from INPUT.BIN,
sets InOutRes DS:[4426], pushes the routine's frame with a far return to a capture label, and jumps to the entry. After
return it records CPU14, a 1 KiB stack window, the 64 KiB text segment and DS:4420..442F. Interrupts stay off around the
call; nothing in SYSTEM is stubbed."""
import ctypes, os, struct, subprocess
from hashlib import sha256
from res0709_ega_reset_0000_cpu_audit import Cpu, ROOT, SIZE, physical, put16, check

NBASE, TOP, LOW, WINDOW = 0x3000, 0xF000, 0xEC00, 0x400
NAMES = ('0919', '084a')


def specs(A):
    out = []
    for name in NAMES:
        for o in (dict(), dict(mode=0xD7B1), dict(res=0x67), dict(bufsize=1, bufpos=0), dict(bufsize=2, bufpos=1, flushfunc=False),
                  dict(bufsize=3, bufpos=0, r_io=0x65), dict(bufsize=0x40, r_flush=0x66), dict(bufsize=4, bufpos=3, flags=0x0402),
                  dict(flags=0x08D7), dict(flags=0x00D7, bufsize=5, bufpos=4)):
            out.append((name, dict(o)))
        if name == '0919':
            for text, width in ((b'', 0), (b'HELLO', 0), (b'HELLO', 9), (b'AB', 0xFFFF), (b'ABCDEFGHIJKL', 20)):
                out.append((name, dict(text=text, width=width, bufsize=6, bufpos=1)))
    return out


def native(image, sites, fs, A):
    directory = ROOT / 'resources/system-text/native'; directory.mkdir(parents=True, exist_ok=True)
    image_reloc = A.relocate(image, sites, NBASE)
    (directory / 'IMAGE.BIN').write_bytes(image_reloc)
    ds = NBASE + A.DATA_SEG; rows = specs(A); data = bytearray(); frames = []
    for name, o in rows:
        m, c, _ = A.build(image_reloc, NBASE, name, stub_seg=A.TSEG, sp=0x8000, **o)
        words = [struct.unpack_from('<H', m, physical(A.SS, 0x8000 + 4 + 2 * k))[0] for k in range(A.ENTRIES[name][1] // 2 + 2)] \
            if name == '0919' else [struct.unpack_from('<H', m, physical(A.SS, 0x8004 + 2 * k))[0] for k in range(2)]
        frames.append(words)
        hdr = struct.pack('<HHHH', A.SPANS[name][0], o.get('flags', 2), o.get('res', 0), len(words)) + b''.join(struct.pack('<H', w) for w in words)
        data += hdr.ljust(32, b'\0') + m[physical(A.TSEG, 0):physical(A.TSEG, 0) + 65536]
    (directory / 'INPUT.BIN').write_bytes(data)
    load = []
    for k, start in enumerate(range(0, len(image_reloc), 0x8000)):
        load += [f'mov ax,{NBASE + 0x800 * k}', 'mov ds,ax', 'mov bx,word ptr cs:[image]', 'xor dx,dx',
                 f'mov cx,{min(0x8000, len(image_reloc) - start)}', 'call read_image']
    t = ['.code16', '.intel_syntax noprefix', '.global _start', '_start: jmp main', '.org 0x200', 'main:',
         'mov ax,cs', 'mov ds,ax', 'mov ss,ax', 'mov sp,0xfff0', 'cld',
         'mov dx,offset imagename', 'mov ax,0x3d00', 'int 0x21', 'jc failure', 'mov word ptr cs:[image],ax'] + load + [
         'push cs', 'pop ds', 'mov bx,word ptr [image]', 'mov ah,0x3e', 'int 0x21',
         'mov dx,offset inputname', 'mov ax,0x3d00', 'int 0x21', 'jc failure', 'mov word ptr [input],ax',
         'mov dx,offset outname', 'xor cx,cx', 'mov ah,0x3c', 'int 0x21', 'jc failure', 'mov word ptr [handle],ax',
         'mov word ptr [header],cs', 'mov word ptr [header+2],offset capture', f'mov word ptr [header+4],{len(rows)}',
         'mov dx,offset header', 'mov cx,6', 'call write_out',
         'again:', 'push cs', 'pop ds', 'mov dx,offset spec', 'mov cx,32', 'call read_input']
    for off in (0, 0x8000):
        t += [f'mov ax,{A.TSEG}', 'mov ds,ax', f'mov dx,{off}', 'mov cx,0x8000', 'call read_input']
    t += ['cli', 'cld', 'push cs', 'pop es', f'mov di,{LOW}', f'mov cx,{WINDOW}', 'mov al,0x5a', 'rep stosb',
          f'mov ax,{ds}', 'mov es,ax', 'mov ax,word ptr cs:[spec+4]', 'mov word ptr es:[0x4426],ax',
          'mov ax,word ptr cs:[spec]', 'mov word ptr cs:[target],ax', f'mov word ptr cs:[target+2],{NBASE + A.SYS_SEG}',
          'mov ax,cs', 'mov ss,ax', f'mov sp,{TOP}',
          'mov cx,word ptr cs:[spec+6]', 'mov si,cx', 'shl si,1', 'add si,offset spec+6', 'pushw:', 'push word ptr cs:[si]', 'sub si,2', 'loop pushw',
          'push cs', 'push offset capture', 'push word ptr cs:[spec+2]',
          f'mov ax,{ds}', 'mov ds,ax', 'mov ax,0x1111', 'mov es,ax',
          'mov ax,0x1234', 'mov bx,0x3456', 'mov cx,0x5678', 'mov dx,0x789a', 'mov si,0x9abc', 'mov di,0xbcde', 'mov bp,0x7777',
          'popf', 'jmp dword ptr cs:[target]', 'capture:']
    t += [f'mov word ptr cs:[out+{2 * n}],{r}' for n, r in enumerate('ax bx cx dx si di bp sp'.split())]
    t += ['mov word ptr cs:[out+16],cs', 'mov word ptr cs:[out+18],offset capture', 'mov word ptr cs:[out+20],ds', 'mov word ptr cs:[out+22],es',
          'mov word ptr cs:[out+24],ss', 'mov ax,cs', 'mov ss,ax', 'mov sp,0xfff0', 'pushf', 'pop ax', 'mov word ptr cs:[out+26],ax', 'sti',
          'push cs', 'pop ds', 'mov dx,offset out', 'mov cx,28', 'call write_out',
          f'mov dx,{LOW}', f'mov cx,{WINDOW}', 'call write_out']
    for off in (0, 0x8000):
        t += [f'mov ax,{A.TSEG}', 'mov ds,ax', f'mov dx,{off}', 'mov cx,0x8000', 'call write_out']
    t += [f'mov ax,{ds}', 'mov ds,ax', 'mov dx,0x4420', 'mov cx,16', 'call write_out', 'push cs', 'pop ds',
          'inc word ptr [index]', f'cmp word ptr [index],{len(rows)}', 'jae done', 'jmp again', 'done:',
          'mov bx,word ptr [handle]', 'mov ah,0x3e', 'int 0x21', 'mov ax,0x4c00', 'int 0x21',
          'write_out:', 'mov bx,word ptr cs:[handle]', 'mov ah,0x40', 'int 0x21', 'jc failure', 'cmp ax,cx', 'jne failure', 'ret',
          'read_input:', 'mov bx,word ptr cs:[input]', 'mov ah,0x3f', 'int 0x21', 'jc failure', 'cmp ax,cx', 'jne failure', 'ret',
          'read_image:', 'mov ah,0x3f', 'int 0x21', 'jc failure', 'ret',
          'failure:', 'mov ax,0x4c01', 'int 0x21',
          'imagename: .asciz "IMAGE.BIN"', 'inputname: .asciz "INPUT.BIN"', 'outname: .asciz "RESULT.BIN"',
          'image: .word 0', 'input: .word 0', 'handle: .word 0', 'index: .word 0', '.align 2', 'header: .space 6', 'out: .space 28',
          'target: .word 0,0', 'spec: .space 32']
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
    with (directory / 'dosbox.log').open('w') as log:
        subprocess.run(['/usr/bin/dosbox-x', '-conf', str(directory / 'dosbox.conf'), '-fastlaunch', '-exit', '-noconsole'],
                       env=dict(os.environ, SDL_VIDEODRIVER='dummy', SDL_AUDIODRIVER='dummy'), stdout=log, stderr=subprocess.STDOUT,
                       check=True, timeout=600)
    result = out.read_bytes(); rec = 28 + WINDOW + 65536 + 16
    check(len(result) == 6 + len(rows) * rec, f'native length {len(result)}')
    comseg, capture, count = struct.unpack_from('<3H', result); check(count == len(rows), 'native count')
    for n, ((name, o), words) in enumerate(zip(rows, frames)):
        r = result[6 + n * rec:6 + (n + 1) * rec]
        sp = TOP - 2 * len(words) - 4
        m, c, stubs = A.build(image_reloc, NBASE, name, stub_seg=A.TSEG, ss=comseg, sp=sp, cpu=None, **o)
        put16(m, comseg, sp, capture); put16(m, comseg, sp + 2, comseg)
        for i in range(LOW, sp):
            m[physical(comseg, i)] = 0x5A
        ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(m); keep = []
        env = A.c_driver(stubs, False, keep); st = ctypes.c_int(99)
        status = fs[name](ram, SIZE, ctypes.byref(c), ctypes.byref(env), ctypes.byref(st))
        check(status == 0, f'native C status {n} {name}: {status}')
        want = struct.unpack_from('<14H', r)
        for k, (reg, _) in enumerate(Cpu._fields_):
            check(getattr(c, reg) == want[k], f'native CPU {n} {name} {o}/{reg}: C {getattr(c, reg):04x} DOS {want[k]:04x}')
        mine = bytes(ram[physical(comseg, LOW):physical(comseg, LOW) + WINDOW]) + bytes(ram[physical(A.TSEG, 0):physical(A.TSEG, 0) + 65536]) \
            + bytes(ram[physical(ds, 0x4420):physical(ds, 0x4420) + 16])
        if mine != r[28:]:
            diff = [i for i, (x, y) in enumerate(zip(mine, r[28:])) if x != y]
            raise ValueError(f'native bytes {n} {name} {o}: {[(hex(i), hex(mine[i]), hex(r[28 + i])) for i in diff[:16]]} ({len(diff)}) cpu {want}')
    return dict(cases=len(rows), compared=['CPU14', f'stack {WINDOW} B', 'text segment 64 KiB', 'DS:4420+10'], flag_masks=0,
                result_sha256=sha256(result).hexdigest(), image_bin_sha256=sha256(image_reloc).hexdigest())
