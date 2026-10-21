"""Native DOSBox-X (machine=ega, video mode 0Dh) corroboration for OVL_007F_0294 as a composed closure; used by
u7f_number_0294_cpu_audit.py.

Layout and declared substitutions are RES_0899_0059's native set (the relocated image read from IMAGE.BIN before every case;
the INT 21h hook for 25h/35h/2Ch; the INT 10h stub; the Halt capture at 0AF8:00E9; the derived IN 61h mask; the GetTime
and INT 16h frame masks) plus:
  * the overlay body, its 17 unit fixups relocated to the native base, read once from OVL.BIN to segment 6800h; the font
    (2 KiB, synthetic) from FONT.BIN to 7000h;
  * the INT 16h hook replays key GROUPS: a word FFFFh ends a group; AH=01h reports a key only before the next separator,
    AH=00h skips separators and takes the next key;
  * EGA: mode 0Dh is set once; before every case the graphics-controller registers are reset (0-5 = 0, 8 = FFh), each plane
    is filled with 31h + 27h*plane and the map mask is set back to 0Fh, matching the C pr_ega_reset plus fill; after the
    case the four planes are read back in full through read-map select. Registers and latches are compared by the Unicorn
    run only (they are not read back natively), and the EGA OUTs are not traced natively."""
import ctypes, os, struct, subprocess
from hashlib import sha256
from res0709_ega_reset_0000_cpu_audit import Cpu, ROOT, SIZE, physical, put16, check
import res0899_quit_0000_cpu_audit as Q
A = None  # the running 0294 audit module

NBASE, NSS, NPSP, NOVL, NFONT, NPROMPT = 0x3000, 0x6000, 0x5800, 0x6800, 0x7000, 0x7800
TOP, LOW, WINDOW = 0xF000, 0xE000, 0x1000
HALT_BYTES = bytes.fromhex('33c933dbba')
TAILS = [b' foo', b' STING']
PROMPTS = [b'How much? ', b'', b'X' * 45]
MAXWORDS = 24


def native_cases():
    g, D, ENTER, ESC, BS = A.g, A.D, A.ENTER, A.ESC, A.BS
    out = [('enter-empty', dict(groups=g(ENTER))), ('esc-empty', dict(groups=g(ESC))), ('42', dict(groups=g(D['4'], D['2'], ENTER))),
           ('bs', dict(groups=g(D['4'], D['2'], BS, D['7'], ENTER))), ('clamp', dict(groups=g(D['9'], D['9'], D['9'], ENTER), maxv=100)),
           ('clamp-then-digit', dict(groups=g(D['5'], D['0'], D['0'], D['1'], ENTER), maxv=300)),
           ('six-digits', dict(groups=g(*[D[x] for x in '1234567'], ENTER), maxv=65535)),
           ('drain-group', dict(groups=(((D['1'], D['2'], D['3']),) + g(ENTER)))), ('ctrl-s', dict(groups=g(0x1F13, D['8'], ENTER))),
           ('empty-prompt', dict(prompt=b'', groups=g(D['9'], ENTER))), ('long-prompt', dict(prompt=b'X' * 45, groups=g(D['1'], ENTER))),
           ('sting-quit', dict(tail=b' STING', groups=g(D['1'], 0x2E03)))]
    out += [(f'colour{k:x}', dict(colour=k, groups=g(D['3'], ENTER))) for k in (0, 7, 0x0F)]
    out += [(f'flags{fl:x}', dict(flags=fl, groups=g(D['6'], BS, D['1'], ENTER))) for fl in A.FLAGS]
    return out


def words(groups):
    w = []
    for grp in groups:
        w += list(grp) + [0xFFFF]
    check(len(w) <= MAXWORDS, 'key script too long for the native table')
    return w + [0xFFFF] * (MAXWORDS - len(w))


def c_layout(image_reloc, body, fixups, o, comseg, stub_off, ret_off):
    m = bytearray([0x5A]) * SIZE
    b = NBASE * 16; m[b:b + len(image_reloc)] = image_reloc
    at = physical(NOVL, A.ENTRY); m[at:at + len(body)] = body
    for f in fixups:
        put16(m, NOVL, f, (struct.unpack_from('<H', m, physical(NOVL, f))[0] + NBASE) & 0xFFFF)
    ds = NBASE + Q.SEG['data']
    m[physical(ds, 0x5372)] = 1; m[physical(ds, 0x5373)] = 0; put16(m, ds, 0x5374, 1); put16(m, ds, 0x2612, 0)
    m[physical(ds, 0x4A1C)] = 3; m[physical(ds, 0x49F7)] = 0; m[physical(ds, 0x8501)] = 0; m[physical(ds, 0x8502)] = 0
    put16(m, ds, 0x4422, NPSP)
    tail = o.get('tail', b' foo'); m[physical(NPSP, 0x80)] = len(tail)
    for k, ch in enumerate(tail + b'\r'):
        m[physical(NPSP, 0x81 + k)] = ch
    m[physical(ds, 0x4A1E)] = 1; put16(m, ds, 0x4A16, 0xA000); put16(m, ds, 0x4A18, 0xA200)
    put16(m, ds, 0x5E20, 0x0000); put16(m, ds, 0x5E22, NFONT)
    for j in range(2048):
        m[physical(NFONT, j)] = ((j * 37 + (j >> 3) * 13) ^ 0xA6) & 255
    put16(m, 0, 0x40, stub_off); put16(m, 0, 0x42, comseg)
    s = physical(comseg, stub_off); m[s:s + len(Q.INT10_STUB)] = Q.INT10_STUB
    m[physical(0x0040, 0x0049)] = 0x5A
    prompt = o.get('prompt', b'How much? '); m[physical(NPROMPT, 0x100)] = len(prompt)
    for k, ch in enumerate(prompt):
        m[physical(NPROMPT, 0x101 + k)] = ch
    sp = TOP - 0x0C
    for j, w in enumerate((ret_off, comseg, o.get('colour', 0x0E), 0x0100, NPROMPT, o.get('maxv', 100))):
        put16(m, NSS, sp + 2 * j, w)
    c = Cpu(0x1234, 0x3456, 0x5678, 0x789A, 0x9ABC, 0xBCDE, 0x7777, sp, NOVL, A.ENTRY, ds, 0x1111, NSS, o.get('flags', 2))
    return m, c


def native(image, sites, body, fixups, lib, audit):
    global A
    A = audit
    directory = ROOT / 'resources/u7f-0294/native'; directory.mkdir(parents=True, exist_ok=True)
    image_reloc = Q.relocate(image, sites, NBASE)
    sysseg = NBASE + Q.SEG['system']
    check(image_reloc[sysseg * 16 + 0xE9 - NBASE * 16:][:5] == HALT_BYTES, 'Halt entry bytes before the native capture')
    (directory / 'IMAGE.BIN').write_bytes(image_reloc)
    ovl = bytearray(body)
    for f in fixups:
        struct.pack_into('<H', ovl, f - A.ENTRY, (struct.unpack_from('<H', ovl, f - A.ENTRY)[0] + NBASE) & 0xFFFF)
    (directory / 'OVL.BIN').write_bytes(bytes(ovl))
    (directory / 'FONT.BIN').write_bytes(bytes(((j * 37 + (j >> 3) * 13) ^ 0xA6) & 255 for j in range(2048)))
    specs = native_cases()
    clock = Q.clock(4736147, 7)
    times = [w for n in range(160) for w in clock(n)]
    rows = []
    for _, o in specs:
        rows.append((o.get('flags', 2), o.get('colour', 0x0E), o.get('maxv', 100), TAILS.index(o.get('tail', b' foo')),
                     PROMPTS.index(o.get('prompt', b'How much? '))) + tuple(words(o['groups'])))
    ROW = 2 * len(rows[0])
    ds = NBASE + Q.SEG['data']
    fill = []
    for chunk in range(2):
        fill += [f'mov ax,{NBASE + 0x1000 * chunk}', 'mov es,ax', 'xor di,di', 'mov cx,0x8000', 'mov ax,0x5a5a', 'rep stosw']
    def load(name_label, seg, length):
        out = ['push cs', 'pop ds', f'mov dx,offset {name_label}', 'mov ax,0x3d00', 'int 0x21', 'jc failure', 'mov word ptr cs:[image],ax']
        for k, start in enumerate(range(0, length, 0x8000)):
            out += [f'mov ax,{seg + 0x800 * k}', 'mov ds,ax', 'mov bx,word ptr cs:[image]', 'xor dx,dx',
                    f'mov cx,{min(0x8000, length - start)}', 'mov ah,0x3f', 'int 0x21', 'jc failure']
        return out + ['push cs', 'pop ds', 'mov bx,word ptr [image]', 'mov ah,0x3e', 'int 0x21']
    ovl_load = ['push cs', 'pop ds', 'mov dx,offset ovlname', 'mov ax,0x3d00', 'int 0x21', 'jc failure', 'mov word ptr cs:[image],ax',
                f'mov ax,{NOVL}', 'mov ds,ax', 'mov bx,word ptr cs:[image]', f'mov dx,{A.ENTRY}', f'mov cx,{len(ovl)}', 'mov ah,0x3f', 'int 0x21',
                'jc failure', 'push cs', 'pop ds', 'mov bx,word ptr [image]', 'mov ah,0x3e', 'int 0x21']
    ega_reset = ['mov dx,0x3ce'] + [x for r in (0, 1, 3, 4, 5) for x in (f'mov ax,{r}', 'out dx,ax')] + ['mov ax,0xff08', 'out dx,ax',
                 'mov ax,0xa000', 'mov es,ax']
    for plane in range(4):
        ega_reset += ['mov dx,0x3c4', f'mov ax,{((1 << plane) << 8) | 2}', 'out dx,ax', 'xor di,di', f'mov ax,{(0x31 + plane * 0x27) * 257}',
                      'mov cx,0x8000', 'rep stosw']
    ega_reset += ['mov dx,0x3c4', 'mov ax,0x0f02', 'out dx,ax']
    readback = []
    for plane in range(4):
        readback += ['mov dx,0x3ce', 'mov ax,0x0005', 'out dx,ax', f'mov ax,{(plane << 8) | 4}', 'out dx,ax']
        for off in (0, 0x8000):
            readback += ['mov ax,0xa000', 'mov ds,ax', 'mov ax,0x5000', 'mov es,ax', f'mov si,{off}', 'xor di,di', 'mov cx,0x8000', 'cld',
                         'rep movsb', 'mov ax,0x5000', 'mov ds,ax', 'mov bx,word ptr cs:[handle]', 'xor dx,dx', 'mov cx,0x8000', 'mov ah,0x40',
                         'int 0x21']
    writes = []
    for seg, off, n in ((NSS, LOW, WINDOW), (ds, 0, 0x8000), (ds, 0x8000, 0x8000)):
        writes += [f'mov ax,{seg}', 'mov ds,ax', 'mov bx,word ptr cs:[handle]', f'mov dx,{off}', f'mov cx,{n}', 'mov ah,0x40', 'int 0x21']
    tails_bin = b''.join(bytes([len(t)]) + t + b'\r' + b'\x5a' * (30 - len(t)) for t in TAILS)
    prompts_bin = b''.join(bytes([len(p)]) + p + b'\x5a' * (63 - len(p)) for p in PROMPTS)
    t = ['.code16', '.intel_syntax noprefix', '.global _start', '_start: jmp main', '.org 0x200', 'main:',
         'mov ax,cs', 'mov ds,ax', 'mov ss,ax', 'mov sp,0xfff0', 'cld', 'mov ax,0x000d', 'int 0x10',
         'mov dx,offset outname', 'xor cx,cx', 'mov ah,0x3c', 'int 0x21', 'jc failure', 'mov word ptr [handle],ax',
         'mov word ptr [header],cs', 'mov word ptr [header+2],offset stub10', f'mov word ptr [header+4],{len(rows)}',
         'mov word ptr [header+6],offset returned',
         'mov bx,word ptr [handle]', 'mov dx,offset header', 'mov cx,8', 'mov ah,0x40', 'int 0x21'] + ovl_load + load('fontname', NFONT, 2048) + [
         'xor ax,ax', 'mov es,ax', 'mov ax,word ptr es:[0x84]', 'mov word ptr cs:[old21],ax', 'mov ax,word ptr es:[0x86]', 'mov word ptr cs:[old21+2],ax',
         'mov ax,word ptr es:[0x40]', 'mov word ptr cs:[old10],ax', 'mov ax,word ptr es:[0x42]', 'mov word ptr cs:[old10+2],ax',
         'mov ax,word ptr es:[0x58]', 'mov word ptr cs:[old16],ax', 'mov ax,word ptr es:[0x5a]', 'mov word ptr cs:[old16+2],ax',
         'again:', 'cld'] + fill + load('imagename', NBASE, len(image_reloc)) + [
         f'mov ax,{sysseg}', 'mov es,ax', 'mov byte ptr es:[0xe9],0xea', 'mov word ptr es:[0xea],offset capture_exit', 'mov word ptr es:[0xec],cs',
         'push cs', 'pop ds', 'mov si,word ptr [index]', f'mov ax,{ROW}', 'mul si', 'mov si,ax', 'add si,offset table', 'mov word ptr cs:[row],si',
         f'mov ax,{ds}', 'mov es,ax', 'mov byte ptr es:[0x5372],1', 'mov byte ptr es:[0x5373],0', 'mov word ptr es:[0x5374],1',
         'mov word ptr es:[0x2612],0', 'mov byte ptr es:[0x4a1c],3', 'mov byte ptr es:[0x49f7],0', 'mov word ptr es:[0x8501],0',
         f'mov word ptr es:[0x4422],{NPSP}', 'mov byte ptr es:[0x4a1e],1', 'mov word ptr es:[0x4a16],0xa000', 'mov word ptr es:[0x4a18],0xa200',
         'mov word ptr es:[0x5e20],0', f'mov word ptr es:[0x5e22],{NFONT}',
         'push si', 'mov ax,word ptr cs:[si+6]', 'mov cl,5', 'shl ax,cl', 'mov si,ax', 'add si,offset tails', f'mov ax,{NPSP}', 'mov es,ax',
         'mov di,0x80', 'mov cx,32', 'rep movsb', 'pop si',
         'push si', 'mov ax,word ptr cs:[si+8]', 'mov cl,6', 'shl ax,cl', 'mov si,ax', 'add si,offset prompts', f'mov ax,{NPROMPT}', 'mov es,ax',
         'mov di,0x100', 'mov cx,64', 'rep movsb', 'pop si',
         'lea ax,[si+10]', 'mov word ptr cs:[kbase],ax', 'mov word ptr cs:[kidx],0',
         f'mov ax,{NSS}', 'mov es,ax', f'mov di,{LOW}', f'mov cx,{WINDOW // 2}', 'mov ax,0x5a5a', 'rep stosw'] + ega_reset + [
         'mov word ptr cs:[vtable+32],0x0123', 'mov word ptr cs:[vtable+34],0xf000', 'mov word ptr cs:[tidx],0',
         'cli', 'xor ax,ax', 'mov es,ax', 'mov word ptr es:[0x84],offset int21', 'mov word ptr es:[0x86],cs',
         'mov word ptr es:[0x40],offset stub10', 'mov word ptr es:[0x42],cs', 'mov word ptr es:[0x58],offset int16', 'mov word ptr es:[0x5a],cs',
         'mov ax,0x40', 'mov es,ax', 'mov al,byte ptr es:[0x49]', 'mov byte ptr cs:[oldmode],al', 'mov byte ptr es:[0x49],0x5a',
         f'mov ax,{NSS}', 'mov ss,ax', f'mov sp,{TOP}', 'push word ptr cs:[si+4]', f'push {NPROMPT}', 'push 0x0100',
         'push word ptr cs:[si+2]', 'push cs', 'push offset returned', 'push word ptr cs:[si]',
         f'mov ax,{ds}', 'mov ds,ax', 'mov ax,0x1111', 'mov es,ax',
         'mov ax,0x1234', 'mov bx,0x3456', 'mov cx,0x5678', 'mov dx,0x789a', 'mov si,0x9abc', 'mov di,0xbcde', 'mov bp,0x7777',
         'popf', f'ljmp {NOVL},{A.ENTRY}',
         'returned:', 'mov word ptr cs:[out+16],cs', 'mov word ptr cs:[out+18],offset returned', 'jmp record',
         'capture_exit:', f'mov word ptr cs:[out+16],{sysseg}', 'mov word ptr cs:[out+18],0xe9', 'record:']
    t += [f'mov word ptr cs:[out+{2 * n}],{r}' for n, r in enumerate('ax bx cx dx si di bp sp'.split())]
    t += ['mov word ptr cs:[out+20],ds', 'mov word ptr cs:[out+22],es', 'mov word ptr cs:[out+24],ss', 'mov ax,cs', 'mov ss,ax',
          'mov sp,0xfff0', 'pushf', 'pop ax', 'mov word ptr cs:[out+26],ax', 'mov ax,cs', 'mov ds,ax', 'xor ax,ax', 'mov es,ax',
          'mov ax,word ptr [old21]', 'mov word ptr es:[0x84],ax', 'mov ax,word ptr [old21+2]', 'mov word ptr es:[0x86],ax',
          'mov ax,word ptr [old10]', 'mov word ptr es:[0x40],ax', 'mov ax,word ptr [old10+2]', 'mov word ptr es:[0x42],ax',
          'mov ax,word ptr [old16]', 'mov word ptr es:[0x58],ax', 'mov ax,word ptr [old16+2]', 'mov word ptr es:[0x5a],ax',
          'mov bx,word ptr [handle]', 'mov dx,offset out', 'mov cx,28', 'mov ah,0x40', 'int 0x21'] + writes + readback + [
          'push cs', 'pop ds', 'mov bx,word ptr [handle]', 'mov dx,offset vtable+32', 'mov cx,4', 'mov ah,0x40', 'int 0x21',
          'mov bx,word ptr [handle]', 'mov dx,offset tidx', 'mov cx,2', 'mov ah,0x40', 'int 0x21',
          'mov bx,word ptr [handle]', 'mov dx,offset kidx', 'mov cx,2', 'mov ah,0x40', 'int 0x21',
          'mov ax,0x40', 'mov es,ax', 'mov al,byte ptr cs:[oldmode]', 'mov byte ptr es:[0x49],al', 'sti',
          'inc word ptr [index]', f'cmp word ptr [index],{len(rows)}', 'jae done', 'jmp again', 'done:',
          'mov bx,word ptr [handle]', 'mov ah,0x3e', 'int 0x21', 'mov ax,0x0003', 'int 0x10', 'mov ax,0x4c00', 'int 0x21',
          'failure:', 'mov ax,0x4c01', 'int 0x21',
          'int21:', 'cmp ah,0x2c', 'je t2c', 'cmp ah,0x35', 'je f35', 'cmp ah,0x25', 'je f25', 'jmp dword ptr cs:[old21]',
          'f35:', 'mov bl,al', 'xor bh,bh', 'shl bx,1', 'shl bx,1', 'les bx,dword ptr cs:[vtable+bx]', 'iret',
          'f25:', 'mov word ptr cs:[save],bx', 'mov bl,al', 'xor bh,bh', 'shl bx,1', 'shl bx,1', 'mov word ptr cs:[vtable+bx],dx',
          'mov word ptr cs:[vtable+bx+2],ds', 'mov bx,word ptr cs:[save]', 'iret',
          't2c:', 'mov word ptr cs:[save],bx', 'mov bx,word ptr cs:[tidx]', 'mov cx,word ptr cs:[times+bx]', 'mov dx,word ptr cs:[times+bx+2]',
          'add word ptr cs:[tidx],4', 'mov bx,word ptr cs:[save]', 'iret',
          # Declared INT 16h hook: key groups (FFFFh ends a group).
          'int16:', 'cmp ah,1', 'je kb_peek', 'cmp ah,0', 'je kb_take', 'jmp dword ptr cs:[old16]',
          'kb_take:', 'mov word ptr cs:[save],bx', 'kb_skip:', 'mov bx,word ptr cs:[kidx]', 'shl bx,1', 'add bx,word ptr cs:[kbase]',
          'mov ax,word ptr cs:[bx]', 'inc word ptr cs:[kidx]', 'cmp ax,0xffff', 'je kb_skip', 'mov bx,word ptr cs:[save]', 'iret',
          'kb_peek:', 'mov word ptr cs:[save],bx', 'mov word ptr cs:[save2],bp', 'mov bp,sp', 'and word ptr [bp+4],0xffbf',
          'mov bx,word ptr cs:[kidx]', 'shl bx,1', 'add bx,word ptr cs:[kbase]', 'cmp word ptr cs:[bx],0xffff', 'jne kb_key',
          'or word ptr [bp+4],0x40', 'jmp kb_out', 'kb_key:', 'mov ax,word ptr cs:[bx]',
          'kb_out:', 'mov bp,word ptr cs:[save2]', 'mov bx,word ptr cs:[save]', 'iret',
          'stub10:', '.byte ' + ','.join(str(x) for x in Q.INT10_STUB),
          'outname: .asciz "RESULT.BIN"', 'imagename: .asciz "IMAGE.BIN"', 'ovlname: .asciz "OVL.BIN"', 'fontname: .asciz "FONT.BIN"',
          'handle: .word 0', 'image: .word 0', 'index: .word 0', 'row: .word 0', 'save: .word 0', 'save2: .word 0', 'tidx: .word 0',
          'kidx: .word 0', 'kbase: .word 0', 'oldmode: .byte 0', '.align 2', 'header: .space 8', 'out: .space 28',
          'old21: .word 0,0', 'old10: .word 0,0', 'old16: .word 0,0', 'vtable: .space 1024', 'times: .word ' + ','.join(str(x) for x in times),
          'tails: .byte ' + ','.join(str(x) for x in tails_bin), 'prompts: .byte ' + ','.join(str(x) for x in prompts_bin), '.align 2', 'table:']
    t += ['.word ' + ','.join(str(v) for v in row) for row in rows]
    (directory / 'oracle.S').write_text('\n'.join(t) + '\n')
    subprocess.run(['as', '--32', str(directory / 'oracle.S'), '-o', str(directory / 'oracle.o')], check=True)
    com = directory / 'ORACLE.COM'
    subprocess.run(['ld', '-m', 'elf_i386', '-Ttext', '0x100', '--oformat', 'binary', str(directory / 'oracle.o'), '-o', str(com)], check=True)
    check(com.stat().st_size + 0x100 < 0xF000, 'COM reaches its own stack')
    (directory / 'dosbox.conf').write_text('[sdl]\noutput=surface\n[dosbox]\nmachine=ega\nmemsize=16\n[cpu]\ncore=normal\ncputype=386\ncycles=max\n'
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
    data = out.read_bytes(); record = 28 + WINDOW + 0x10000 + 4 * 0x10000 + 4 + 2 + 2
    check(len(data) == 8 + len(rows) * record, f'native length {len(data)} expected {8 + len(rows) * record}')
    comseg, stub_off, count, ret_off = struct.unpack_from('<4H', data); check(count == len(rows), 'native count')
    stops, masked = {}, 0
    for n, (name, o) in enumerate(specs):
        rec = data[8 + n * record:8 + (n + 1) * record]
        m, c = c_layout(image_reloc, body, fixups, o, comseg, stub_off, ret_off)
        spec = dict(table={8: (0x0123, 0xF000)}, in61=0x00, time_start=4736147, time_step=7)
        res = run_native_c(lib, m, c, spec, o['groups'], comseg, stub_off)
        res2 = run_native_c(lib, m, c, dict(spec, in61=0x80), o['groups'], comseg, stub_off)
        st, actual, ram, env, ega, sps = res
        check(st in ('RETURN', 'EXIT') and st == res2[0], f'native C status {name}: {st}/{res2[0]}')
        stops[st] = stops.get(st, 0) + 1
        for (reg, _), want in zip(Cpu._fields_, struct.unpack_from('<14H', rec)):
            check(getattr(actual, reg) == want, f'native CPU {name}/{reg}: C {getattr(actual, reg):04x} DOS {want:04x}')
        dep = {i: ram[i] ^ res2[2][i] for i in range(SIZE) if ram[i] != res2[2][i]}
        check(all(v & ~0xC4 == 0 for v in dep.values()) and len(dep) <= 2 and (not dep or physical(ds, 0x4A14) in dep), f'{name}: IN-dependent bytes {dep}')
        masked += bool(dep)
        frame_mask = {physical(NSS, (sp - k) & 0xFFFF) for sp in sps for k in range(1, 7)}
        at = 28
        for seg, off, length in ((NSS, LOW, WINDOW), (ds, 0, 0x10000)):
            for k in range(length):
                p = physical(seg, (off + k) & 0xFFFF)
                if p in frame_mask:
                    continue
                mask = ~dep.get(p, 0) & 0xFF
                check(ram[p] & mask == rec[at + k] & mask, f'native {name} at {seg:04x}:{(off + k) & 0xFFFF:04x}: C {ram[p]:02x} DOS {rec[at + k]:02x}')
            at += length
        check(ega[:4 * 0x10000] == rec[at:at + 4 * 0x10000], f'native EGA planes {name}')
        at += 4 * 0x10000
        check(struct.unpack_from('<HH', rec, at) == env.table.get(8), f'native vector entry {name}')
        check(struct.unpack_from('<H', rec, at + 4)[0] == 4 * sum(1 for e in env.log if e[0] == '2c'), f'native GetTime calls {name}')
    probe = subprocess.run([binary, '--version'], capture_output=True, text=True, env=dict(os.environ, SDL_VIDEODRIVER='dummy'))
    version = next(line.split(',')[0].strip() for line in (probe.stdout + probe.stderr).splitlines() if 'DOSBox-X version' in line)
    return dict(cases=len(rows), stops=stops, cpu_words=14, stack_bytes=WINDOW, data_segment_bytes=0x10000, plane_bytes=4 * 0x10000,
                in61_masked_cases=masked, machine='ega', video_mode='0Dh', halt_capture=dict(at='0AF8:00E9', before=HALT_BYTES.hex()),
                result_sha256_per_run=sha256(data).hexdigest(), image_bin_sha256=sha256(image_reloc).hexdigest(),
                dosbox_x=dict(path=binary, sha256=sha256(open(binary, 'rb').read()).hexdigest(), version=version,
                              pinned_source='5a9c7c53481c2e933948039c33256776653f4faf', pinned_build_used=False))


def run_native_c(lib, m, c, spec, groups, comseg, stub_off):
    """The C model on the native layout, recording the INT-time SPs of the GetTime and keyboard service calls."""
    keep = []; sps = []
    q, env = Q.c_env(spec, keep, stub_at=(comseg, stub_off), time_sps=sps)
    keys = A.Keys(groups, env.log)

    def kb(ctx, before, after):
        bb = before[0]; sps.append(bb.sp); ah = bb.ax >> 8; a = Cpu.from_buffer_copy(bb); a.ip = (bb.ip + 2) & 0xFFFF
        if ah == 0x01:
            k = keys.pending(); env.log.append(('16', 1, k))
            if k is not None:
                a.ax = k; a.flags = bb.flags & ~0x40
            else:
                a.flags = bb.flags | 0x40
            after[0] = a; return 0
        if ah == 0x00:
            k = keys.take()
            if k is None:
                return 1
            env.log.append(('16', 0, k)); a.ax = k; after[0] = a; return 0
        return 1
    import res0899_readkey_0059_cpu_audit as K
    import res0709_printer_0626_cpu_audit as P
    kfn = K.KB_FN(kb); keep.append(kfn)
    e = A.Env(K.KeyEnv(q, kfn, None, 64), 65536, 65536, 65536, 64)
    ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(m); ega = P.Ega(); lib.pr_ega_reset(ctypes.byref(ega))
    for p in range(4):
        ctypes.memset(ctypes.addressof(ega.plane[p]), 0x31 + p * 0x27, 65536)
    penv = P.Env(None, P.OUT_FN(lambda *a: 0), P.IN_FN()); wfn = P.WRITE_FN(lambda *a: None); keep += [penv, wfn]
    bus = P.Bus(ram, ctypes.pointer(ega), ctypes.pointer(penv), wfn, None)
    actual = Cpu.from_buffer_copy(c); d = A.Diag()
    status = A.STATUS.get(lib.pr_u7f_number_0294_cpu(ctypes.byref(bus), SIZE, ctypes.byref(actual), ctypes.byref(e), ctypes.byref(d)))
    return status, actual, bytes(ram), env, bytes(ega), sps
