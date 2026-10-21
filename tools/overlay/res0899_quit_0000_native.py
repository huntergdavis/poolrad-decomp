"""Native DOSBox-X corroboration for RES_0899_0000 (QUIT) as a composed closure; used by res0899_quit_0000_cpu_audit.py.

The loader-expanded START.EXE image, relocated to segment 3000h with the loader's own table, is read from IMAGE.BIN
before every case, so each case starts from the original bytes. Declared native substitutions, each named here and in
the evidence doc:
  * INT 21h: a hook in the oracle program serves AH=25h/35h over a shadow vector table and AH=2Ch from the case's
    scripted clock, returning by IRET (the frame flags) with no stack use of its own; other functions chain to DOS.
  * INT 10h: the vector points at the same real-byte stub the Unicorn run uses (sets 0040:0049 := AL, IRET).
  * Halt: the native copy's first five bytes at 0AF8:00E9 (33 C9 33 DB BA, XOR CX,CX; XOR BX,BX; MOV DX,...) are replaced by
    a far JMP to a capture routine, so the EXIT state is recorded instead of terminating the oracle. The model stops at
    that address without executing it, so no modelled byte changes; the receipt records the bytes before and after.
  * Ports: real emulated hardware. IN 61h is live, so the bits that depend on its value (SF/ZF/PF of 13B1's AND, carried
    into Regs.FLAGS and Intr's pushed flags) are found by running the C model with two IN values and masked; the mask
    is asserted to be exactly those bits.
  * The real INT 21h AH=2Ch frame that DOS-side code leaves 6 bytes below GetTime's INT-time SP is outside the GetTime
    model (#1245's stated limit) and is masked at the recorded addresses."""
import ctypes, os, struct, subprocess
from hashlib import sha256
import res0899_quit_0000_cpu_audit as A
from res0709_ega_reset_0000_cpu_audit import Cpu, ROOT, SIZE, physical, put16, check

NBASE, NSS, TOP, LOW, WINDOW = 0x3000, 0x6000, 0xF000, 0xEC00, 0x0400
HALT_BYTES = bytes.fromhex('33c933dbba')
TIMES = 64


def regions(ds):
    """(segment, offset, length) compared natively, in record order."""
    drv = NBASE + A.SEG['driver']
    return [(NSS, LOW, WINDOW), (ds, 0x4A00, 0x20), (ds, 0x5370, 0x08), (ds, 0x2610, 0x04), (drv, 0x01D0, 0x0530),
            (drv, 0x1378, 0x04), (0x0040, 0x0049, 0x01)]


def native_cases():
    out = []
    for saved in (0, 1, 2):
        for w in (0x0000, 0x0001):
            for arg in (0x0000, 0x0001, 0x00FF, 0x0005):
                out.append((f's{saved:x}-w{w:x}-a{arg:x}', dict(saved=saved, w=w, arg=arg)))
    out += [(f'mode{md:02x}', dict(mode=md)) for md in (0x00, 0x03, 0x0D, 0x10, 0x13, 0xFF)]
    out += [(f'flags{fl:x}-{lab}', dict(flags=fl, **o)) for fl in A.FLAGS for lab, o in (('muted', dict(saved=2)), ('on', dict(saved=1, arg=1)))]
    return out


def c_run(image_reloc, o, comseg, stub_off, in61, keep):
    """The C model on the native layout; returns (status, cpu, ram, GetTime INT-time SPs)."""
    m = bytearray([0x5A]) * SIZE
    b = NBASE * 16; m[b:b + len(image_reloc)] = image_reloc
    ds = NBASE + A.SEG['data']
    m[physical(ds, 0x5372)] = 0xEE; m[physical(ds, 0x5373)] = o.get('saved', 1)
    put16(m, ds, 0x5374, o.get('w', 1)); put16(m, ds, 0x2612, o.get('arg', 0)); m[physical(ds, 0x4A1C)] = o.get('mode', 3)
    for k, v in enumerate((0x0B0B, 0x0C0C, 0x0D0D, 0x0E0E, 0x0F0F, 0x1010, 0x2121, 0x3131, 0x0246)):
        put16(m, ds, 0x4A04 + 2 * k, v)
    put16(m, 0, 0x40, stub_off); put16(m, 0, 0x42, comseg)
    s = physical(comseg, stub_off); m[s:s + len(A.INT10_STUB)] = A.INT10_STUB
    m[physical(0x0040, 0x0049)] = 0x5A
    sp = TOP - 4
    put16(m, NSS, sp, 0x7000); put16(m, NSS, sp + 2, 0x9001)
    c = Cpu(0x1234, 0x3456, 0x5678, 0x789A, 0x9ABC, 0xBCDE, 0x7777, sp, NBASE + A.SEG['body'], 0, ds, 0x1111, NSS, o.get('flags', 2))
    spec = dict(table={8: (0x0123, 0xF000)}, in61=in61, time_start=4736147, time_step=7)
    int_sps = []
    q, env = A.c_env(spec, keep, stub_at=(comseg, stub_off), time_sps=int_sps)
    ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(m); d = A.QuitDiag()
    status = A.F(ram, SIZE, ctypes.byref(c), ctypes.byref(q), ctypes.byref(d))
    return status, c, bytes(ram), int_sps, env


def native(image, sites, decoded, f, audit=None):
    global A
    A = audit or A   # the running audit module: its ctypes classes are the ones f was declared with
    A.F = f
    directory = ROOT / 'resources/0899-quit/native'; directory.mkdir(parents=True, exist_ok=True)
    image_reloc = A.relocate(image, sites, NBASE)
    halt_at = (NBASE + A.SEG['system']) * 16 + 0x00E9 - NBASE * 16
    check(image_reloc[halt_at:halt_at + 5] == HALT_BYTES, 'Halt entry bytes before the native capture')
    (directory / 'IMAGE.BIN').write_bytes(image_reloc)
    specs = native_cases()
    clock = A.clock(4736147, 7)
    times = [w for n in range(TIMES) for w in clock(n)]
    rows = [(o.get('flags', 2), o.get('saved', 1), o.get('w', 1), o.get('arg', 0), o.get('mode', 3)) for _, o in specs]
    ds = NBASE + A.SEG['data']; drv = NBASE + A.SEG['driver']; sysseg = NBASE + A.SEG['system']
    regs_rest = (0x0B0B, 0x0C0C, 0x0D0D, 0x0E0E, 0x0F0F, 0x1010, 0x2121, 0x3131, 0x0246)
    rec_len = 28 + sum(n for _, _, n in regions(ds))
    writes = []
    for seg, off, n in regions(ds):
        writes += [f'mov ax,{seg}', 'mov ds,ax', 'mov bx,word ptr cs:[handle]', f'mov dx,{off}', f'mov cx,{n}', 'mov ah,0x40', 'int 0x21']
    fill = []
    for chunk in range(2):                      # 128 KiB of 5Ah from the load base: the image area and the data segment's BSS
        fill += [f'mov ax,{NBASE + 0x1000 * chunk}', 'mov es,ax', 'xor di,di', 'mov cx,0x8000', 'mov ax,0x5a5a', 'rep stosw']
    load = []
    for k, start in enumerate(range(0, len(image_reloc), 0x8000)):   # IMAGE.BIN in 32 KiB reads into consecutive memory
        load += [f'mov ax,{NBASE + 0x800 * k}', 'mov ds,ax', 'mov bx,word ptr cs:[image]', 'xor dx,dx',
                 f'mov cx,{min(0x8000, len(image_reloc) - start)}', 'mov ah,0x3f', 'int 0x21', 'jc failure']
    t = ['.code16', '.intel_syntax noprefix', '.global _start', '_start: jmp main', '.org 0x200', 'main:',
         'mov ax,cs', 'mov ds,ax', 'mov ss,ax', 'mov sp,0xfff0', 'cld',
         'mov dx,offset outname', 'xor cx,cx', 'mov ah,0x3c', 'int 0x21', 'jc failure', 'mov word ptr [handle],ax',
         'mov word ptr [header],cs', 'mov word ptr [header+2],offset stub10', f'mov word ptr [header+4],{len(rows)}',
         'mov bx,word ptr [handle]', 'mov dx,offset header', 'mov cx,6', 'mov ah,0x40', 'int 0x21',
         'xor ax,ax', 'mov es,ax', 'mov ax,word ptr es:[0x84]', 'mov word ptr [old21],ax', 'mov ax,word ptr es:[0x86]', 'mov word ptr [old21+2],ax',
         'mov ax,word ptr es:[0x40]', 'mov word ptr [old10],ax', 'mov ax,word ptr es:[0x42]', 'mov word ptr [old10+2],ax',
         'again:', 'cld'] + fill + [
         'push cs', 'pop ds', 'mov dx,offset imagename', 'mov ax,0x3d00', 'int 0x21', 'jc failure', 'mov word ptr cs:[image],ax'] + load + [
         'push cs', 'pop ds', 'mov bx,word ptr [image]', 'mov ah,0x3e', 'int 0x21',
         # Native capture at Halt: JMP FAR cs:capture over the first five bytes of 0AF8:00E9 (declared).
         f'mov ax,{sysseg}', 'mov es,ax', 'mov byte ptr es:[0xe9],0xea', 'mov word ptr es:[0xea],offset capture', 'mov word ptr es:[0xec],cs',
         'mov si,word ptr [index]', 'mov ax,si', 'shl si,1', 'shl si,1', 'add si,ax', 'shl si,1', 'add si,offset table',   # 10 bytes/row
         f'mov ax,{ds}', 'mov es,ax', 'mov byte ptr es:[0x5372],0xee', 'mov al,byte ptr cs:[si+2]', 'mov byte ptr es:[0x5373],al',
         'mov ax,word ptr cs:[si+4]', 'mov word ptr es:[0x5374],ax', 'mov ax,word ptr cs:[si+6]', 'mov word ptr es:[0x2612],ax',
         'mov al,byte ptr cs:[si+8]', 'mov byte ptr es:[0x4a1c],al'] + [
         f'mov word ptr es:[{0x4A04 + 2 * k}],{v}' for k, v in enumerate(regs_rest)] + [
         f'mov ax,{NSS}', 'mov es,ax', f'mov di,{LOW}', f'mov cx,{WINDOW // 2}', 'mov ax,0x5a5a', 'rep stosw',
         'mov word ptr cs:[vtable+32],0x0123', 'mov word ptr cs:[vtable+34],0xf000', 'mov word ptr cs:[tidx],0',
         'cli', 'xor ax,ax', 'mov es,ax', 'mov word ptr es:[0x84],offset int21', 'mov word ptr es:[0x86],cs',
         'mov word ptr es:[0x40],offset stub10', 'mov word ptr es:[0x42],cs',
         'mov ax,0x40', 'mov es,ax', 'mov al,byte ptr es:[0x49]', 'mov byte ptr cs:[oldmode],al', 'mov byte ptr es:[0x49],0x5a',
         f'mov ax,{NSS}', 'mov ss,ax', f'mov sp,{TOP}', 'push 0x9001', 'push 0x7000', 'push word ptr cs:[si]',
         f'mov ax,{ds}', 'mov ds,ax', 'mov ax,0x1111', 'mov es,ax',
         'mov ax,0x1234', 'mov bx,0x3456', 'mov cx,0x5678', 'mov dx,0x789a', 'mov si,0x9abc', 'mov di,0xbcde', 'mov bp,0x7777',
         'popf', f'ljmp {NBASE + A.SEG["body"]},0', 'capture:']
    t += [f'mov word ptr cs:[out+{2 * n}],{r}' for n, r in enumerate('ax bx cx dx si di bp sp'.split())]
    t += [f'mov word ptr cs:[out+16],{sysseg}', 'mov word ptr cs:[out+18],0xe9', 'mov word ptr cs:[out+20],ds', 'mov word ptr cs:[out+22],es',
          'mov word ptr cs:[out+24],ss', 'mov ax,cs', 'mov ss,ax', 'mov sp,0xfff0',   # leave the case stack untouched
          'pushf', 'pop ax', 'mov word ptr cs:[out+26],ax', 'mov ax,cs', 'mov ds,ax', 'xor ax,ax', 'mov es,ax',
          'mov ax,word ptr [old21]', 'mov word ptr es:[0x84],ax', 'mov ax,word ptr [old21+2]', 'mov word ptr es:[0x86],ax',
          'mov ax,word ptr [old10]', 'mov word ptr es:[0x40],ax', 'mov ax,word ptr [old10+2]', 'mov word ptr es:[0x42],ax',
          'mov bx,word ptr [handle]', 'mov dx,offset out', 'mov cx,28', 'mov ah,0x40', 'int 0x21'] + writes + [
          'push cs', 'pop ds', 'mov bx,word ptr [handle]', 'mov dx,offset vtable+32', 'mov cx,4', 'mov ah,0x40', 'int 0x21',
          'mov bx,word ptr [handle]', 'mov dx,offset tidx', 'mov cx,2', 'mov ah,0x40', 'int 0x21',
          'mov ax,0x40', 'mov es,ax', 'mov al,byte ptr cs:[oldmode]', 'mov byte ptr es:[0x49],al', 'sti',
          'inc word ptr [index]', f'cmp word ptr [index],{len(rows)}', 'jae done', 'jmp again', 'done:',
          'mov bx,word ptr [handle]', 'mov ah,0x3e', 'int 0x21', 'mov ax,0x4c00', 'int 0x21', 'failure:', 'mov ax,0x4c01', 'int 0x21',
          # Declared INT 21h hook: 25h/35h over the shadow table, 2Ch from the scripted clock; IRET; no stack use.
          'int21:', 'cmp ah,0x2c', 'je t2c', 'cmp ah,0x35', 'je f35', 'cmp ah,0x25', 'je f25', 'jmp dword ptr cs:[old21]',
          'f35:', 'mov bl,al', 'xor bh,bh', 'shl bx,1', 'shl bx,1', 'les bx,dword ptr cs:[vtable+bx]', 'iret',
          'f25:', 'mov word ptr cs:[save],bx', 'mov bl,al', 'xor bh,bh', 'shl bx,1', 'shl bx,1', 'mov word ptr cs:[vtable+bx],dx',
          'mov word ptr cs:[vtable+bx+2],ds', 'mov bx,word ptr cs:[save]', 'iret',
          't2c:', 'mov word ptr cs:[save],bx', 'mov bx,word ptr cs:[tidx]', 'mov cx,word ptr cs:[times+bx]', 'mov dx,word ptr cs:[times+bx+2]',
          'add word ptr cs:[tidx],4', 'mov bx,word ptr cs:[save]', 'iret',
          'stub10:', '.byte ' + ','.join(str(x) for x in A.INT10_STUB),
          'outname: .asciz "RESULT.BIN"', 'imagename: .asciz "IMAGE.BIN"', 'handle: .word 0', 'image: .word 0', 'index: .word 0',
          'save: .word 0', 'tidx: .word 0', 'oldmode: .byte 0', '.align 2', 'header: .space 6', 'out: .space 28',
          'old21: .word 0,0', 'old10: .word 0,0', 'vtable: .space 1024', 'times: .word ' + ','.join(str(x) for x in times), 'table:']
    t += ['.word ' + ','.join(str(v) for v in row) for row in rows]
    (directory / 'oracle.S').write_text('\n'.join(t) + '\n')
    subprocess.run(['as', '--32', str(directory / 'oracle.S'), '-o', str(directory / 'oracle.o')], check=True)
    com = directory / 'ORACLE.COM'
    subprocess.run(['ld', '-m', 'elf_i386', '-Ttext', '0x100', '--oformat', 'binary', str(directory / 'oracle.o'), '-o', str(com)], check=True)
    check(com.stat().st_size + 0x100 < 0xF000, 'COM reaches its own stack')
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
                       check=True, timeout=600)
    data = out.read_bytes(); record = rec_len + 4 + 2
    check(len(data) == 6 + len(rows) * record, f'native length {len(data)} expected {6 + len(rows) * record}')
    comseg, stub_off, count = struct.unpack_from('<3H', data); check(count == len(rows), 'native count')
    masked_cases, masked_bits = 0, 0
    for n, (name, o) in enumerate(specs):
        rec = data[6 + n * record:6 + (n + 1) * record]
        keep = []
        st, c, ram, int_sps, env = c_run(image_reloc, o, comseg, stub_off, 0x00, keep)
        st2, c2, ram2, _, _ = c_run(image_reloc, o, comseg, stub_off, 0x80, keep)
        check(A.STATUS.get(st) == 'EXIT' and A.STATUS.get(st2) == 'EXIT', f'native C status {name}')
        for (reg, _), want in zip(Cpu._fields_, struct.unpack_from('<14H', rec)):
            check(getattr(c, reg) == want, f'native CPU {name}/{reg}: C {getattr(c, reg):04x} DOS {want:04x}')
        # Mask: exactly the bytes that depend on the live IN 61h value (two C runs), asserted to be SF/ZF/PF bits only.
        dep = {i: ram[i] ^ ram2[i] for i in range(SIZE) if ram[i] != ram2[i]}
        check(all(v & ~0xC4 == 0 for v in dep.values()), f'{name}: IN-dependent bits beyond SF/ZF/PF {dep}')
        check(set(dep) <= {physical(ds, 0x4A14), physical(NSS, TOP - 4 - 18)}, f'{name}: IN-dependent bytes {sorted(map(hex, dep))}')
        if dep:
            masked_cases += 1; masked_bits = max(masked_bits, len(dep))
        frame_mask = {physical(NSS, (sp - k) & 0xFFFF) for sp in int_sps for k in range(1, 7)}
        at = 28
        for seg, off, length in regions(ds):
            for k in range(length):
                p = physical(seg, (off + k) & 0xFFFF)
                if p in frame_mask:
                    continue
                want = rec[at + k]; got = ram[p]
                mask = ~dep.get(p, 0) & 0xFF
                check(got & mask == want & mask, f'native {name} at {seg:04x}:{(off + k) & 0xFFFF:04x}: C {got:02x} DOS {want:02x}')
            at += length
        check(struct.unpack_from('<HH', rec, at) == env.table.get(8), f'native vector entry {name}: DOS {struct.unpack_from("<HH", rec, at)} C {env.table.get(8)}')
        check(struct.unpack_from('<H', rec, at + 4)[0] == 4 * len(int_sps), f'native GetTime calls {name}')
    probe = subprocess.run([binary, '--version'], capture_output=True, text=True, env=dict(os.environ, SDL_VIDEODRIVER='dummy'))
    version = next(line.split(',')[0].strip() for line in (probe.stdout + probe.stderr).splitlines() if 'DOSBox-X version' in line)
    return dict(cases=len(rows), cpu_words=14, compared_regions=[f'{s:04X}:{o:04X}+{n:X}' for s, o, n in regions(ds)],
                in61_masked_cases=masked_cases, in61_masked_bytes_max=masked_bits, halt_capture=dict(at='0AF8:00E9', before=HALT_BYTES.hex(),
                after='EA <capture> <oracle CS>'), result_sha256=sha256(data).hexdigest(), image_bin_sha256=sha256(image_reloc).hexdigest(),
                dosbox_x=dict(path=binary, sha256=sha256(open(binary, 'rb').read()).hexdigest(), version=version,
                              pinned_source='5a9c7c53481c2e933948039c33256776653f4faf', pinned_build_used=False))
