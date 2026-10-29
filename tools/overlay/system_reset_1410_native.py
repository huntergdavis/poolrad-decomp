"""Native DOSBox-X corroboration for SYSTEM Reset 0AF8:1410; used by system_reset_1410_cpu_audit.py.

The original Reset and Close bytes run at 6000:1410/1491. Declared substitutions:
  * INT 21h hook: AH=3Dh answers from the case (carry and AX), AH=3Eh from the case's close answer, by editing CF in the
    IRET frame; other functions chain to DOS. The DOS frame of the real INT (6 bytes below the INT-time SP) is outside the
    model (its service contract) and is masked at the recorded address.
  * Close-path cases (magic D7B3h): the native copy's first five bytes at 6000:1491 (Close's own code, not this body's) are
    replaced by a far JMP to a capture routine, so the state at the model's CLOSE stop is compared; the receipt records the
    bytes. The resume after Close is compared by the Unicorn run only."""

# NOT_A_ROW_CHECKER: Imported native companion; system_reset_1410_cpu_audit.py owns row checking.
if __name__ == "__main__":
    raise SystemExit("No cases executed: invoke tools/overlay/system_reset_1410_cpu_audit.py --native through poolrad-heavy.sh.")
import ctypes, os, struct, subprocess
from hashlib import sha256
import system_reset_1410_cpu_audit as A
from res0709_ega_reset_0000_cpu_audit import Cpu, ROOT, SIZE, physical, put16, check

TOP, LOW, WINDOW = 0xF000, 0xEC00, 0x0400
NAMES = [b'GAME.DAT', b'']


def native(pieces, f):
    directory = ROOT / 'resources/system-reset/native'; directory.mkdir(parents=True, exist_ok=True)
    for start, body in pieces:
        (directory / f'piece{start:04x}.bin').write_bytes(body)
    close_bytes = dict(pieces)[0x1491][:5]
    specs = [(n, o) for n, o in A.cases() if not n.startswith(('sp', 'a20', 'ss-ds', 'random', 'alias', 'frec-', 'name-wrap'))]
    rows = []
    for _, o in specs:
        rows.append((o.get('flags', 2), o.get('magic', 0xD7B0), o.get('handle', 7), o.get('mode', 2), o.get('recsize', 128),
                     o.get('open_cf', 0), o.get('open_ax', 5), o.get('close_cf', 0), o.get('close_ax', 6), NAMES.index(o.get('name', b'GAME.DAT'))))
    ROW = 2 * len(rows[0])
    copy = []
    for start, body in pieces:
        copy += [f'mov di,{start}', f'mov si,offset piece{start:04x}', f'mov cx,{len(body)}', 'rep movsb']
    t = ['.code16', '.intel_syntax noprefix', '.global _start', '_start: jmp main', '.org 0x200', 'main:',
         'mov ax,cs', 'mov ds,ax', 'mov ss,ax', f'mov sp,{TOP}', 'cld',
         'mov dx,offset outname', 'xor cx,cx', 'mov ah,0x3c', 'int 0x21', 'jc failure', 'mov word ptr [handle],ax',
         'mov word ptr [header],cs', 'mov word ptr [header+2],offset returned', f'mov word ptr [header+4],{len(rows)}',
         'mov bx,word ptr [handle]', 'mov dx,offset header', 'mov cx,6', 'mov ah,0x40', 'int 0x21',
         'xor ax,ax', 'mov es,ax', 'mov ax,word ptr es:[0x84]', 'mov word ptr [old21],ax', 'mov ax,word ptr es:[0x86]', 'mov word ptr [old21+2],ax',
         'again:', 'cli', 'cld', 'mov ax,cs', 'mov ds,ax', f'mov ax,{A.CS}', 'mov es,ax'] + copy + [
         'mov si,word ptr [index]', f'mov ax,{ROW}', 'mul si', 'mov si,ax', 'add si,offset table', 'mov word ptr [row],si',
         # Close-path cases: capture at the Close entry.
         f'cmp word ptr [si+2],0xd7b3', 'jne no_capture', 'mov byte ptr es:[0x1491],0xea', 'mov word ptr es:[0x1492],offset capture',
         'mov word ptr es:[0x1494],cs', 'no_capture:',
         'mov ax,cs', 'mov es,ax', 'mov al,0x5a', f'mov di,{LOW}', f'mov cx,{WINDOW}', 'rep stosb',
         f'mov ax,{A.FREC}', 'mov es,ax', 'mov di,0x0100', 'mov cx,0x80', 'mov al,0x5a', 'rep stosb',
         'mov ax,word ptr [si+4]', 'mov word ptr es:[0x0100],ax', 'mov ax,word ptr [si+2]', 'mov word ptr es:[0x0102],ax',
         'mov word ptr es:[0x0104],0x1111',
         'push si', 'mov ax,word ptr [si+18]', 'mov cl,4', 'shl ax,cl', 'mov si,ax', 'add si,offset names', 'mov di,0x0130', 'mov cx,16', 'rep movsb',
         'pop si',
         f'mov ax,{A.DS}', 'mov es,ax', 'push si', 'mov di,0x4420', 'mov cx,0x10', 'mov al,0x5a', 'rep stosb', 'pop si',
         'mov al,byte ptr [si+6]', 'mov byte ptr es:[0x442c],al', 'mov word ptr es:[0x4426],0x7777',
         'mov ax,word ptr [si+10]', 'mov word ptr [open_cf],ax', 'mov ax,word ptr [si+12]', 'mov word ptr [open_ax],ax',
         'mov ax,word ptr [si+14]', 'mov word ptr [close_cf],ax', 'mov ax,word ptr [si+16]', 'mov word ptr [close_ax],ax',
         'xor ax,ax', 'mov es,ax', 'mov word ptr es:[0x84],offset int21', 'mov word ptr es:[0x86],cs',
         f'mov sp,{TOP}', f'push {A.FREC}', 'push 0x0100', 'push word ptr cs:[si+8]', 'push word ptr cs:[si]',
         f'mov ax,{A.DS}', 'mov ds,ax', 'mov ax,0x1111', 'mov es,ax',
         'mov ax,0x1234', 'mov bx,0x3456', 'mov cx,0x5678', 'mov dx,0x789a', 'mov si,0x9abc', 'mov di,0xbcde', 'mov bp,0x7777',
         'popf', f'lcall {A.CS},0x1410', 'returned:', 'mov word ptr cs:[out+16],cs', 'mov word ptr cs:[out+18],offset returned', 'jmp record',
         'capture:', f'mov word ptr cs:[out+16],{A.CS}', 'mov word ptr cs:[out+18],0x1491', 'record:']
    t += [f'mov word ptr cs:[out+{2 * n}],{r}' for n, r in enumerate('ax bx cx dx si di bp sp'.split())]
    t += ['mov word ptr cs:[out+20],ds', 'mov word ptr cs:[out+22],es', 'mov word ptr cs:[out+24],ss', 'mov ax,cs', 'mov ss,ax',
          'mov sp,0xf800', 'pushf', 'pop ax', 'mov word ptr cs:[out+26],ax', 'cld', 'mov ax,cs', 'mov ds,ax',   # DF may be set by the case
          'xor ax,ax', 'mov es,ax', 'mov ax,word ptr [old21]', 'mov word ptr es:[0x84],ax', 'mov ax,word ptr [old21+2]', 'mov word ptr es:[0x86],ax',
          'sti', 'mov bx,word ptr [handle]', 'mov dx,offset out', 'mov cx,28', 'mov ah,0x40', 'int 0x21',
          'mov bx,word ptr [handle]', f'mov dx,{LOW}', f'mov cx,{WINDOW}', 'mov ah,0x40', 'int 0x21',
          f'mov ax,{A.FREC}', 'mov ds,ax', 'mov bx,word ptr cs:[handle]', 'mov dx,0x0100', 'mov cx,0x80', 'mov ah,0x40', 'int 0x21',
          f'mov ax,{A.DS}', 'mov ds,ax', 'mov bx,word ptr cs:[handle]', 'mov dx,0x4420', 'mov cx,0x10', 'mov ah,0x40', 'int 0x21',
          'push cs', 'pop ds', 'mov ax,cs', 'mov es,ax', 'mov si,offset close_orig', f'mov di,offset piece1491', 'mov cx,5', 'rep movsb',
          'inc word ptr [index]', f'cmp word ptr [index],{len(rows)}', 'jae done', 'jmp again', 'done:',
          'mov bx,word ptr [handle]', 'mov ah,0x3e', 'int 0x21', 'mov ax,0x4c00', 'int 0x21', 'failure:', 'mov ax,0x4c01', 'int 0x21',
          # Declared INT 21h hook: 3Dh/3Eh answered from the case by editing CF in the IRET frame; others chain.
          'int21:', 'cmp ah,0x3d', 'je d_open', 'cmp ah,0x3e', 'je d_close', 'jmp dword ptr cs:[old21]',
          'd_open:', 'mov word ptr cs:[save],bp', 'mov bp,sp', 'and word ptr [bp+4],0xfffe', 'cmp word ptr cs:[open_cf],0', 'je d_ok',
          'or word ptr [bp+4],1', 'mov ax,word ptr cs:[open_ax]', 'jmp d_out', 'd_ok:', 'mov ax,word ptr cs:[open_ax]', 'jmp d_out',
          'd_close:', 'mov word ptr cs:[save],bp', 'mov bp,sp', 'and word ptr [bp+4],0xfffe', 'cmp word ptr cs:[close_cf],0', 'je d_out',
          'or word ptr [bp+4],1', 'mov ax,word ptr cs:[close_ax]',
          'd_out:', 'mov bp,word ptr cs:[save]', 'iret',
          'outname: .asciz "RESULT.BIN"', 'handle: .word 0', 'index: .word 0', 'row: .word 0', 'save: .word 0',
          'open_cf: .word 0', 'open_ax: .word 0', 'close_cf: .word 0', 'close_ax: .word 0', 'old21: .word 0,0',
          'header: .space 6', 'out: .space 28', 'close_orig: .byte ' + ','.join(str(b) for b in close_bytes),
          'names: .byte ' + ','.join(str(b) for n in NAMES for b in (n + b'\x00' + b'\x5a' * (15 - len(n)))), '.align 2', 'table:']
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
    data = out.read_bytes(); record = 28 + WINDOW + 0x80 + 0x10
    check(len(data) == 6 + len(rows) * record, f'native length {len(data)}')
    seg, ret_ip, count = struct.unpack_from('<3H', data); check(count == len(rows), 'native count')
    stops = {}
    for n, (name, o) in enumerate(specs):
        rec = data[6 + n * record:6 + (n + 1) * record]
        m, c, script = A.build(pieces, **o)
        m = bytearray(m)
        m[physical(seg, LOW):physical(seg, LOW) + WINDOW] = b'\x5a' * WINDOW
        sp = TOP - 0x0A
        for j, w in enumerate((ret_ip, seg, o.get('recsize', 128), 0x0100, A.FREC)):
            put16(m, seg, sp + 2 * j, w)
        c.ss, c.sp = seg, sp
        sps = []

        def svc(ctx, before, after, script=script):
            b = before[0]; sps.append(b.sp); ax, fl = A.dos_answer(b.flags, b.ax, script, b.ax >> 8)
            a = Cpu.from_buffer_copy(b); a.ax = ax; a.flags = fl; a.ip = 0x1458; after[0] = a; return 0
        ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(m)
        status = A.STATUS[f(ram, SIZE, ctypes.byref(c), A.SERVICE_FN(svc), None)]
        stops[status] = stops.get(status, 0) + 1
        check(status == ('CLOSE' if o.get('magic', 0xD7B0) == 0xD7B3 else 'RETURN'), f'native C status {name}: {status}')
        for (reg, _), want in zip(Cpu._fields_, struct.unpack_from('<14H', rec)):
            check(getattr(c, reg) == want, f'native CPU {name}/{reg}: C {getattr(c, reg):04x} DOS {want:04x}')
        mask = {physical(seg, (s - k) & 0xFFFF) for s in sps for k in range(1, 7)}
        window = bytes(ram[physical(seg, LOW):physical(seg, LOW) + WINDOW])
        for k in range(WINDOW):
            if physical(seg, LOW + k) not in mask:
                check(window[k] == rec[28 + k], f'native stack {name} at {LOW + k:04x}')
        check(bytes(ram[physical(A.FREC, 0x100):physical(A.FREC, 0x180)]) == rec[28 + WINDOW:28 + WINDOW + 0x80], f'native FileRec {name}')
        check(bytes(ram[physical(A.DS, 0x4420):physical(A.DS, 0x4430)]) == rec[28 + WINDOW + 0x80:], f'native InOutRes {name}')
    probe = subprocess.run([binary, '--version'], capture_output=True, text=True, env=dict(os.environ, SDL_VIDEODRIVER='dummy'))
    version = next(line.split(',')[0].strip() for line in (probe.stdout + probe.stderr).splitlines() if 'DOSBox-X version' in line)
    return dict(cases=len(rows), stops=stops, cpu_words=14, stack_bytes=WINDOW, filerec_bytes=0x80, data_bytes=0x10, flag_masks=0,
                close_capture=dict(at='0AF8:1491 (native copy, Close entry)', before=close_bytes.hex()), result_sha256=sha256(data).hexdigest(),
                dosbox_x=dict(path=binary, sha256=sha256(open(binary, 'rb').read()).hexdigest(), version=version,
                              pinned_source='5a9c7c53481c2e933948039c33256776653f4faf', pinned_build_used=False))
