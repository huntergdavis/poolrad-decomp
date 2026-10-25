"""DOSBox-X corroboration for OVL_040C_015D; used by ovl040c_015d_cpu_audit.py --native.

The relocated loader image is read once from IMAGE.BIN at the audit's base (1000h). Per case the oracle COM loads the
fixture's regions (the data segment, the tile sets, the 08D4/Move/aux objects, the unit 40Ch code with its fixups, the
top 8 KiB of the stack segment and the 1951 row targets) from INPUT.BIN, sets all fourteen registers from the case, and
far-jumps to 015D with a far return to a capture label. Interrupts stay off around the call. The error branch (QUIT ends
the process) and video modes other than 0 and 2 (the sequencer OUTs and A000 stores would drive the real adapter) are
Unicorn-only. Every region is compared, with CPU14, against the C model rerun on the same fixture."""
from native_emulator_provenance import describe_dosbox
import ctypes, os, struct, subprocess
from hashlib import sha256
from res0709_ega_reset_0000_cpu_audit import Cpu, ROOT, SIZE, physical, put16, check

REGIONS = ((0x1C7C0, 0x10000), (0x30000, 0x14000), (0x44000, 0x18000), (0x7E000, 0x2000), (0x80000, 0xC000))
STACK = 0x4000


def selected(A):
    if hasattr(A, 'native_cases'):
        return A.native_cases()
    out = []
    for name, o in A.cases():
        number = o.get('number', 0x10)
        if number == 0 or 0x11F <= number <= 0x7FFF or o.get('mode', 0) not in (0, 2) or 'ovseg' in o or 'ss' in o or 'sp' in o:
            continue
        out.append((name, dict(o)))
    return out


def chunks(start, length):
    for k in range(0, length, 0x8000):
        yield (start + k) >> 4, min(0x8000, length - k)


def native(A, code, fixups, body_ips, f, regions=REGIONS, directory=None):
    REGIONS = regions
    directory = directory or ROOT / 'resources/ovl040c-015d/native'; directory.mkdir(parents=True, exist_ok=True)
    image, sites = A.Q.load_image()
    image_reloc = A.Q.relocate(image, sites, A.BASE)
    (directory / 'IMAGE.BIN').write_bytes(image_reloc)
    rows = selected(A); data = bytearray(); states = []
    for name, o in rows:
        m, c, envspec, keys, stubs = A.build(image_reloc, code, fixups, **o)
        states.append((name, o))
        data += struct.pack('<14H', *(getattr(c, n) for n, _ in Cpu._fields_))
        for start, length in REGIONS:
            data += m[start:start + length]
    (directory / 'INPUT.BIN').write_bytes(data)
    t = ['.code16', '.intel_syntax noprefix', '.global _start', '_start: jmp main', '.org 0x200', 'main:',
         'mov ax,cs', 'mov ds,ax', 'mov ss,ax', f'mov sp,{STACK}', 'cld',
         'mov dx,offset imagename', 'mov ax,0x3d00', 'int 0x21', 'jc failure', 'mov word ptr cs:[image],ax']
    for seg, n in chunks(A.BASE * 16, len(image_reloc)):
        t += [f'mov ax,{seg}', 'mov ds,ax', 'mov bx,word ptr cs:[image]', 'xor dx,dx', f'mov cx,{n}', 'mov ah,0x3f', 'int 0x21', 'jc failure']
    t += ['push cs', 'pop ds', 'mov bx,word ptr [image]', 'mov ah,0x3e', 'int 0x21',
          'mov dx,offset inputname', 'mov ax,0x3d00', 'int 0x21', 'jc failure', 'mov word ptr [input],ax',
          'mov dx,offset outname', 'xor cx,cx', 'mov ah,0x3c', 'int 0x21', 'jc failure', 'mov word ptr [handle],ax',
          'mov word ptr [header],cs', 'mov word ptr [header+2],offset capture', f'mov word ptr [header+4],{len(rows)}',
          'mov dx,offset header', 'mov cx,6', 'call write_out',
          'again:', 'push cs', 'pop ds', 'mov dx,offset regs', 'mov cx,28', 'call read_input']
    for start, length in REGIONS:
        for seg, n in chunks(start, length):
            t += [f'mov ax,{seg}', 'mov ds,ax', 'xor dx,dx', f'mov cx,{n}', 'call read_input']
    # the far return the routine's RETF 0Ah takes: [SP] capture, [SP+2] this COM's CS
    t += ['cli', 'mov ax,word ptr cs:[regs+24]', 'mov es,ax', 'mov di,word ptr cs:[regs+14]',
          'mov word ptr es:[di],offset capture', 'mov word ptr es:[di+2],cs',
          'mov ax,word ptr cs:[regs+16]', 'mov word ptr cs:[target+2],ax', 'mov ax,word ptr cs:[regs+18]', 'mov word ptr cs:[target],ax',
          'mov ss,word ptr cs:[regs+24]', 'mov sp,word ptr cs:[regs+14]', 'push word ptr cs:[regs+26]',
          'mov es,word ptr cs:[regs+22]', 'mov bx,word ptr cs:[regs+2]', 'mov cx,word ptr cs:[regs+4]', 'mov dx,word ptr cs:[regs+6]',
          'mov si,word ptr cs:[regs+8]', 'mov di,word ptr cs:[regs+10]', 'mov bp,word ptr cs:[regs+12]',
          'mov ds,word ptr cs:[regs+20]', 'mov ax,word ptr cs:[regs]', 'popf', 'jmp dword ptr cs:[target]', 'capture:']
    t += [f'mov word ptr cs:[out+{2 * n}],{r}' for n, r in enumerate('ax bx cx dx si di bp sp'.split())]
    t += ['mov word ptr cs:[out+16],cs', 'mov word ptr cs:[out+18],offset capture', 'mov word ptr cs:[out+20],ds', 'mov word ptr cs:[out+22],es',
          'mov word ptr cs:[out+24],ss', 'mov ax,cs', 'mov ss,ax', f'mov sp,{STACK}', 'pushf', 'pop ax', 'mov word ptr cs:[out+26],ax',
          'cld', 'sti', 'push cs', 'pop ds', 'mov dx,offset out', 'mov cx,28', 'call write_out']
    for start, length in REGIONS:
        for seg, n in chunks(start, length):
            t += [f'mov ax,{seg}', 'mov ds,ax', 'xor dx,dx', f'mov cx,{n}', 'call write_out']
    t += ['push cs', 'pop ds', 'inc word ptr [index]', f'cmp word ptr [index],{len(rows)}', 'jae done', 'jmp again', 'done:',
          'mov bx,word ptr [handle]', 'mov ah,0x3e', 'int 0x21', 'mov ax,0x4c00', 'int 0x21',
          'write_out:', 'mov bx,word ptr cs:[handle]', 'mov ah,0x40', 'int 0x21', 'jc failure', 'cmp ax,cx', 'jne failure', 'ret',
          'read_input:', 'mov bx,word ptr cs:[input]', 'mov ah,0x3f', 'int 0x21', 'jc failure', 'cmp ax,cx', 'jne failure', 'ret',
          'failure:', 'mov ax,0x4c01', 'int 0x21',
          'imagename: .asciz "IMAGE.BIN"', 'inputname: .asciz "INPUT.BIN"', 'outname: .asciz "RESULT.BIN"',
          'image: .word 0', 'input: .word 0', 'handle: .word 0', 'index: .word 0', '.align 2', 'header: .space 6', 'out: .space 28',
          'target: .word 0,0', 'regs: .space 28']
    (directory / 'oracle.S').write_text('\n'.join(t) + '\n')
    subprocess.run(['as', '--32', str(directory / 'oracle.S'), '-o', str(directory / 'oracle.o')], check=True)
    com = directory / 'ORACLE.COM'
    subprocess.run(['ld', '-m', 'elf_i386', '-Ttext', '0x100', '--oformat', 'binary', str(directory / 'oracle.o'), '-o', str(com)], check=True)
    check(com.stat().st_size + 0x100 < STACK - 0x400, 'COM reaches its stack')
    (directory / 'dosbox.conf').write_text('[sdl]\noutput=surface\n[dosbox]\nmemsize=16\n[cpu]\ncore=normal\ncputype=386\ncycles=max\n'
                                          '[mixer]\nnosound=true\n[midi]\nmididevice=none\n[autoexec]\nmount c '
                                          + str(directory.resolve()) + '\nc:\nORACLE.COM\nexit\n')
    out = directory / 'RESULT.BIN'
    if out.exists():
        out.unlink()
    emulator = describe_dosbox('/usr/bin/dosbox-x')
    with (directory / 'dosbox.log').open('w') as log:
        subprocess.run([emulator['path'], '-conf', str(directory / 'dosbox.conf'), '-fastlaunch', '-exit', '-noconsole'],
                       env=dict(os.environ, SDL_VIDEODRIVER='dummy', SDL_AUDIODRIVER='dummy'), stdout=log, stderr=subprocess.STDOUT,
                       check=True, timeout=900)
    result = out.read_bytes(); rec = 28 + sum(n for _, n in REGIONS)
    check(len(result) == 6 + len(rows) * rec, f'native length {len(result)}')
    comseg, capture, count = struct.unpack_from('<3H', result); check(count == len(rows), 'native count')
    check(comseg * 16 + 0x10000 <= A.BASE * 16 or comseg * 16 + STACK <= A.BASE * 16, 'the COM sits below the image')
    stops = {}
    for n, (name, o) in enumerate(states):
        r = result[6 + n * rec:6 + (n + 1) * rec]
        m, c, envspec, keys, stubs = A.build(image_reloc, code, fixups, **o)
        put16(m, c.ss, c.sp, capture); put16(m, c.ss, (c.sp + 2) & 0xFFFF, comseg)
        put16(m, c.ss, (c.sp - 2) & 0xFFFF, c.flags)          # the oracle's PUSH/POPF of the entry flags
        status, actual, ram, cenv, d = A.run_c(f, m, c, envspec, keys, stubs)
        check(status == 'RETURN', f'native C status {name}: {status}')
        stops[status] = stops.get(status, 0) + 1
        want = struct.unpack_from('<14H', r)
        for k, (reg, _) in enumerate(Cpu._fields_):
            check(getattr(actual, reg) == want[k], f'native CPU {name}/{reg}: C {getattr(actual, reg):04x} DOS {want[k]:04x}')
        mine = b''.join(ram[s:s + length] for s, length in REGIONS)
        if mine != r[28:]:
            diff = [i for i, (x, y) in enumerate(zip(mine, r[28:])) if x != y]
            raise ValueError(f'native bytes {name}: {[(hex(i), hex(mine[i]), hex(r[28 + i])) for i in diff[:8]]} ({len(diff)})')
    return dict(dosbox_x=emulator, cases=len(rows), stops=stops, compared=['CPU14'] + [f'{s:05X}+{n:X}' for s, n in REGIONS], flag_masks=0,
                result_sha256=sha256(result).hexdigest(), image_bin_sha256=sha256(image_reloc).hexdigest())
