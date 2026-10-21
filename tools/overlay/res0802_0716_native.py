"""Native DOSBox-X corroboration for RES_0802_0716; used by res0802_0716_cpu_audit.py.

The loader-expanded START.EXE image, relocated to segment 3000h with the loader's own table, is read from IMAGE.BIN before
every pass; the data segment's BSS beyond the image is zeroed, as in the Unicorn layout. File I/O is REAL DOS: each case's
archive is a real file in the mounted directory, named by the path the code itself builds (DS:547C "ECL" + the case's
name + "DAX"). Declared native substitutions, each named here and in the evidence doc:
  * OPEN capture (pass A): the native copy's five bytes at 0802:05CD (55 89 E5 81 EC: PUSH BP; MOV BP,SP; SUB SP,..., the
    callee's own code, not this body's) are replaced by a far JMP to a capture routine, so the state at the first seam is
    recorded and compared with the C model's OPEN stop.
  * RunError capture (both passes): the five bytes at 0AF8:00E2 are replaced by a far JMP to a capture routine, so a RunError
    is recorded instead of terminating the oracle.
  * The INT 10h vector points at the oracle's copy of the same stub the Unicorn run uses (not reached natively).
  * Real DOS (DOSBox-X 2026.01.02) uses the 24 bytes below each INT 21h's SP: its IRET frame and 18 bytes of saved registers
    (measured: every native-only stack byte lies in such a window). They are outside the declared file service and are
    masked at the addresses the Unicorn run records (DOS_STACK below each INT-time SP).
Pass B runs the WHOLE original end to end under real DOS and is compared with the Unicorn original under the declared file
service on the same layout: this corroborates the declared environment that every Unicorn seam relies on."""
import ctypes, os, struct, subprocess
from hashlib import sha256
from res0709_ega_reset_0000_cpu_audit import Cpu, ROOT, SIZE, physical, check
import res0899_quit_0000_cpu_audit as Q
A = None  # the running audit module (set by native())

NBASE, NSS, NHEAP, NOUT, TOP, LOW, WINDOW = 0x3000, 0x6000, 0x5000, 0x5800, 0xF000, 0xE000, 0x1000
OPEN_BYTES = bytes.fromhex('5589e581ec')
HEAP_LEN = 0x8000
DOS_STACK = 24


def regions(ds):
    return [(NSS, LOW, WINDOW), (ds, 0x4400, 0x30), (ds, 0x557C, 0x08), (NOUT, 0x0000, 0x90), (NHEAP, 0x0000, HEAP_LEN)]


def native_cases():
    out = []
    for name, o in A.cases():
        if name.startswith(('read-fault', 'position-', 'packed-limit', 'random', 'sp-', 'ss-ds', 'flags')):
            continue                                  # Unicorn only: injected faults, the prompt path, layout variants
        out.append((name, o))
    out += [(f'flags{fl:x}', dict(o, flags=fl)) for fl in Q.FLAGS for n, o in A.cases() if n == 'synth-last']
    return out


def layout(reloc, o, index, comseg, stub_off, ret_off):
    o = dict(o); bid = o.pop('block_id')
    o.setdefault('heap_paras', 0x0800)
    return A.build(reloc, block_id=bid, base=NBASE, heap=NHEAP, out=NOUT, ss=NSS, sp=TOP - 0x16, stub=(comseg, stub_off),
                   first_handle=6, name=str(index).encode(), ret=(comseg, ret_off), **o)


def native(image, sites, decoded, f, audit):
    global A
    A = audit
    directory = ROOT / 'resources/0802-0716/native'; directory.mkdir(parents=True, exist_ok=True)
    reloc = Q.relocate(image, sites, NBASE)
    seg0802, sysseg, ds = NBASE + 0x0802, NBASE + Q.SEG['system'], NBASE + Q.SEG['data']
    check(reloc[0x802 * 16 + 0x05CD:0x802 * 16 + 0x05D2] == OPEN_BYTES, '05CD entry bytes before the native capture')
    err_bytes = reloc[Q.SEG['system'] * 16 + 0x00E2:Q.SEG['system'] * 16 + 0x00E7]
    (directory / 'IMAGE.BIN').write_bytes(reloc)
    specs = native_cases()
    check(len(specs) < 100, 'case names fit 8.3')
    for n, (name, o) in enumerate(specs):
        target = directory / f'ECL{n}DAX'
        if o['blob'] is None:
            target.unlink(missing_ok=True)
        else:
            target.write_bytes(o['blob'])
    rows = []
    for n, (name, o) in enumerate(specs):
        digits = str(n).encode()                     # the name's chars; a one-char name leaves the next byte 5Ah, as in Unicorn
        rows.append((o.get('flags', 2), o['block_id'], o.get('flag557c', 1), o.get('heap_paras', 0x0800), len(digits),
                     digits[0] | ((digits[1] if len(digits) > 1 else 0x5A) << 8)))
    image_end = NBASE * 16 + len(reloc); bss_end = physical(ds, 0xFFFF) + 1
    zero = []
    at = image_end
    while at < bss_end:
        n_ = min(0x8000, bss_end - at)
        zero += [f'mov ax,{at >> 4}', 'mov es,ax', f'mov di,{at & 0xF}', f'mov cx,{n_}', 'xor al,al', 'rep stosb']
        at += n_
    load = []
    for k, start in enumerate(range(0, len(reloc), 0x8000)):
        load += [f'mov ax,{NBASE + 0x800 * k}', 'mov ds,ax', 'mov bx,word ptr cs:[image]', 'xor dx,dx',
                 f'mov cx,{min(0x8000, len(reloc) - start)}', 'mov ah,0x3f', 'int 0x21', 'jc failure']
    writes = []
    for seg, off, length in regions(ds):
        writes += [f'mov ax,{seg}', 'mov ds,ax', 'mov bx,word ptr cs:[handle]', f'mov dx,{off}', f'mov cx,{length}', 'mov ah,0x40', 'int 0x21']
    path = b'C:\\POOLRAD\\'
    t = ['.code16', '.intel_syntax noprefix', '.global _start', '_start: jmp main', '.org 0x200', 'main:',
         'mov ax,cs', 'mov ds,ax', 'mov ss,ax', 'mov sp,0xfff0', 'cld',
         'mov dx,offset outname', 'xor cx,cx', 'mov ah,0x3c', 'int 0x21', 'jc failure', 'mov word ptr [handle],ax',
         'mov word ptr [header],cs', 'mov word ptr [header+2],offset stub10', 'mov word ptr [header+4],offset returned',
         f'mov word ptr [header+6],{len(rows)}',
         'mov bx,word ptr [handle]', 'mov dx,offset header', 'mov cx,8', 'mov ah,0x40', 'int 0x21',
         'xor ax,ax', 'mov es,ax', 'mov ax,word ptr es:[0x40]', 'mov word ptr [old10],ax', 'mov ax,word ptr es:[0x42]', 'mov word ptr [old10+2],ax',
         'again:', 'cld', 'push cs', 'pop ds', 'mov dx,offset imagename', 'mov ax,0x3d00', 'int 0x21', 'jc failure',
         'mov word ptr cs:[image],ax'] + load + [
         'push cs', 'pop ds', 'mov bx,word ptr [image]', 'mov ah,0x3e', 'int 0x21', 'cld'] + zero + [
         'push cs', 'pop ds',
         # RunError capture (both passes), and the OPEN capture on pass A.
         f'mov ax,{sysseg}', 'mov es,ax', 'mov byte ptr es:[0xe2],0xea', 'mov word ptr es:[0xe3],offset cap_err', 'mov word ptr es:[0xe5],cs',
         'cmp word ptr cs:[pass],0', 'jne no_open_patch',
         f'mov ax,{seg0802}', 'mov es,ax', 'mov byte ptr es:[0x5cd],0xea', 'mov word ptr es:[0x5ce],offset cap_open', 'mov word ptr es:[0x5d0],cs',
         'no_open_patch:',
         'mov si,word ptr [index]', 'mov ax,si', 'shl si,1', 'add si,ax', 'shl si,1', 'shl si,1', 'add si,offset table',   # 12 bytes/row
         f'mov ax,{ds}', 'mov es,ax', 'mov word ptr es:[0x4426],0', 'mov byte ptr es:[0x442c],2', 'mov byte ptr es:[0x5376],0',
         'mov al,byte ptr cs:[si+4]', 'mov byte ptr es:[0x557c],al',
         f'mov byte ptr es:[0x2614],{len(path)}'] + [f'mov byte ptr es:[{0x2615 + k}],{b}' for k, b in enumerate(path)] + [
         'mov byte ptr es:[0x547c],3', 'mov byte ptr es:[0x547d],0x45', 'mov byte ptr es:[0x547e],0x43', 'mov byte ptr es:[0x547f],0x4c',
         'mov word ptr es:[0x4404],0', f'mov word ptr es:[0x4406],{NHEAP}', 'mov word ptr es:[0x4408],0',
         f'mov ax,{NHEAP}', 'add ax,word ptr cs:[si+6]', 'mov word ptr es:[0x440a],ax',
         'mov word ptr es:[0x440c],0', f'mov word ptr es:[0x440e],{NHEAP}', 'mov word ptr es:[0x4414],0xa9', f'mov word ptr es:[0x4416],{sysseg}',
         # The caller's variables and strings (name = the case's index digits, ext "DAX"), and the heap area.
         f'mov ax,{NHEAP}', 'mov es,ax', 'xor di,di', f'mov cx,{HEAP_LEN}', 'mov al,0x5a', 'rep stosb',
         f'mov ax,{NOUT}', 'mov es,ax', 'xor di,di', 'mov cx,0x90', 'mov al,0x5a', 'rep stosb',
         'mov al,byte ptr cs:[si+8]', 'mov byte ptr es:[0],al', 'mov ax,word ptr cs:[si+10]', 'mov word ptr es:[1],ax',
         'mov byte ptr es:[0x40],3', 'mov byte ptr es:[0x41],0x44', 'mov byte ptr es:[0x42],0x41', 'mov byte ptr es:[0x43],0x58',
         'mov word ptr es:[0x80],0xeeee', 'mov word ptr es:[0x84],0xeeee', 'mov word ptr es:[0x86],0xeeee',
         f'mov ax,{NSS}', 'mov es,ax', f'mov di,{LOW}', f'mov cx,{WINDOW // 2}', 'mov ax,0x5a5a', 'rep stosw',
         'cli', 'xor ax,ax', 'mov es,ax', 'mov word ptr es:[0x40],offset stub10', 'mov word ptr es:[0x42],cs', 'sti',
         f'mov ax,{NSS}', 'mov ss,ax', f'mov sp,{TOP}',
         f'push {NOUT}', 'push 0', f'push {NOUT}', 'push 0x40', 'push word ptr cs:[si+2]', f'push {NOUT}', 'push 0x80', f'push {NOUT}',
         'push 0x84', 'push word ptr cs:[si]',
         f'mov ax,{ds}', 'mov ds,ax', 'mov ax,0x1111', 'mov es,ax',
         'mov ax,0x1234', 'mov bx,0x3456', 'mov cx,0x5678', 'mov dx,0x789a', 'mov si,0x9abc', 'mov di,0xbcde', 'mov bp,0x7777',
         'popf', f'lcall {seg0802},0x0716', 'returned:']
    t += [f'mov word ptr cs:[out+{2 * n}],{r}' for n, r in enumerate('ax bx cx dx si di bp sp cs'.split())]
    t += ['mov word ptr cs:[out+18],offset returned', 'jmp common', 'cap_open:']
    t += [f'mov word ptr cs:[out+{2 * n}],{r}' for n, r in enumerate('ax bx cx dx si di bp sp'.split())]
    t += [f'mov word ptr cs:[out+16],{seg0802}', 'mov word ptr cs:[out+18],0x5cd', 'jmp common', 'cap_err:']
    t += [f'mov word ptr cs:[out+{2 * n}],{r}' for n, r in enumerate('ax bx cx dx si di bp sp'.split())]
    t += [f'mov word ptr cs:[out+16],{sysseg}', 'mov word ptr cs:[out+18],0xe2',
          'common:', 'mov word ptr cs:[out+20],ds', 'mov word ptr cs:[out+22],es', 'mov word ptr cs:[out+24],ss',
          'mov ax,cs', 'mov ss,ax', 'mov sp,0xfff0', 'pushf', 'pop ax', 'mov word ptr cs:[out+26],ax', 'cld',
          'mov ax,cs', 'mov ds,ax', 'xor ax,ax', 'mov es,ax', 'cli',
          'mov ax,word ptr [old10]', 'mov word ptr es:[0x40],ax', 'mov ax,word ptr [old10+2]', 'mov word ptr es:[0x42],ax', 'sti',
          'mov bx,word ptr [handle]', 'mov dx,offset out', 'mov cx,28', 'mov ah,0x40', 'int 0x21'] + writes + [
          # A RunError ends a case before its files are closed: close every case handle (6..19) so the next pass starts
          # with DOS's handle table as the Unicorn run assumes (lowest free handle 6; RESULT.BIN holds 5).
          'mov bx,6', 'closeall:', 'mov ah,0x3e', 'push bx', 'int 0x21', 'pop bx', 'inc bx', 'cmp bx,20', 'jb closeall',
          'push cs', 'pop ds', 'xor word ptr [pass],1', 'jnz again', 'inc word ptr [index]', f'cmp word ptr [index],{len(rows)}',
          'jae done', 'jmp again', 'done:',
          'mov bx,word ptr [handle]', 'mov ah,0x3e', 'int 0x21', 'mov ax,0x4c00', 'int 0x21', 'failure:', 'mov ax,0x4c01', 'int 0x21',
          'stub10:', '.byte ' + ','.join(str(x) for x in Q.INT10_STUB),
          'outname: .asciz "RESULT.BIN"', 'imagename: .asciz "IMAGE.BIN"', 'handle: .word 0', 'image: .word 0', 'index: .word 0',
          'pass: .word 0', '.align 2', 'header: .space 8', 'out: .space 28', 'old10: .word 0,0', 'table:']
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
                       check=True, timeout=900)
    data = out.read_bytes(); rec_len = 28 + sum(n for _, _, n in regions(ds))
    check(len(data) == 8 + 2 * len(rows) * rec_len, f'native length {len(data)} expected {8 + 2 * len(rows) * rec_len}')
    comseg, stub_off, ret_off, count = struct.unpack_from('<4H', data); check(count == len(rows), 'native count')
    stops = {}
    for n, (name, o) in enumerate(specs):
        for pss in (0, 1):
            rec = data[8 + (2 * n + pss) * rec_len:8 + (2 * n + pss + 1) * rec_len]
            m, c, spec = layout(reloc, o, n, comseg, stub_off, ret_off)
            if pss == 0:
                status, actual, ram, _, _ = A.run_c_segment(A.F, c, bytes(m), spec, A.FileSystem(first_handle=6).state())
                check(status == 'OPEN', f'native C pass A {name}: {status}')
                mask = set()
            else:
                snaps, ofs, _, _, int_sps = A.run_unicorn(name, m, c, spec, A.DECODED)
                final = snaps[-1]; status, actual, ram = final['kind'], final['cpu'], final['ram']
                mask = {physical(s, (sp - k) & 0xFFFF) for s, sp in int_sps for k in range(1, DOS_STACK + 1)}
            stops[(pss, status)] = stops.get((pss, status), 0) + 1
            for (reg, _), want in zip(Cpu._fields_, struct.unpack_from('<14H', rec)):
                check(getattr(actual, reg) == want, f'native {name} pass {pss} CPU {reg}: model {getattr(actual, reg):04x} DOS {want:04x}')
            at = 28
            for seg, off, length in regions(ds):
                for k in range(length):
                    p = physical(seg, (off + k) & 0xFFFF)
                    if p not in mask:
                        check(ram[p] == rec[at + k], f'native {name} pass {pss} at {seg:04x}:{(off + k) & 0xFFFF:04x}: '
                                                     f'model {ram[p]:02x} DOS {rec[at + k]:02x}')
                at += length
    probe = subprocess.run([binary, '--version'], capture_output=True, text=True, env=dict(os.environ, SDL_VIDEODRIVER='dummy'))
    version = next(line.split(',')[0].strip() for line in (probe.stdout + probe.stderr).splitlines() if 'DOSBox-X version' in line)
    return dict(cases=len(rows), passes=2 * len(rows), stops={f'{"AB"[k[0]]}:{k[1]}': v for k, v in sorted(stops.items())},
                cpu_words=14, dos_stack_mask_bytes_below_int_sp=DOS_STACK, compared_regions=[f'{s:04X}:{o:04X}+{n:X}' for s, o, n in regions(ds)],
                captures=dict(open=dict(at='0802:05CD', before=OPEN_BYTES.hex()), runerror=dict(at='0AF8:00E2', before=err_bytes.hex())),
                result_sha256=sha256(data).hexdigest(), image_bin_sha256=sha256(reloc).hexdigest(),
                dosbox_x=dict(path=binary, sha256=sha256(open(binary, 'rb').read()).hexdigest(), version=version,
                              pinned_source='5a9c7c53481c2e933948039c33256776653f4faf', pinned_build_used=False))
