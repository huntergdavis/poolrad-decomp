"""Native DOSBox-X corroboration for RES_0709_0BF5 (the message-line prompt); used by res0709_msgbox_0bf5_cpu_audit.py.

The loader-expanded START.EXE image, relocated to segment 3000h with the loader's own table, is read from IMAGE.BIN before every case, so each case starts from the original
bytes. DOSBox-X runs as machine=ega in video mode 0Dh; before each case the four planes are filled with the C model's pattern (31h + 27h*plane) through the real sequencer and
graphics-controller ports. The cases use plain keys only (no ctrl-S driver path, no QUIT, no Ctrl-Break): INT 16h is a real INT into a hook in the oracle program that replays
the case's key script (AH=00h takes the next word, AH=01h reports it and sets ZF in the IRET frame when the script is exhausted). The four planes are read back after the return
through graphics-controller read mode 0 (map select 0..3). Compared against the C model composed with StrStore, 05EB, 0626 (+01DF, UpCase) and ReadKey: all 14 CPU words, the
stack window (minus the 10 DOS-side bytes under each INT 16h call: its 3-word frame and the hook's saved BP and BX), the whole 64 KiB data segment, the key count, and all four
planes (262,144 bytes). Not compared: OUT events (no port trap), the EGA indexed registers, the other 983,000 or so bytes of RAM. Modes 1 and 3 of the game's own DS:4A1E only."""
import os, struct, subprocess
from hashlib import sha256
from res0709_ega_reset_0000_cpu_audit import Cpu, ROOT, SIZE, physical, put16, check

NBASE, NSS, TOP, LOW, WINDOW, NMSG, NPSP, NFONT = 0x3000, 0x6000, 0xF000, 0xEC00, 0x0400, 0x2E00, 0x2F00, 0x5000
A = K = DATA_SEG = None          # bound to the RUNNING audit module in native(): importing it by name would make a second module with its own ctypes classes
FONTOFF = 0x1000
PATTERN = [0x31 + p * 0x27 for p in range(4)]


def specs():
    out = []
    for mode in (1, 3):
        for n, (msg, color) in enumerate(((b'Insert disk 2', 0x0E), (b'Hi', 0x0F), (b'x' * 40, 0x05), (b'', 0x01), (b'y' * 60, 0x0C), (b'A', 0x00))):
            out.append((f'n-mode{mode}-m{n}', dict(msg=msg, mode=mode, color=color, keys=(K['a'],))))
    for pa, pb in ((0xA000, 0xA200), (0xA200, 0xA000), (0xA000, 0xA000)):
        out.append((f'n-pages{pa:04x}{pb:04x}', dict(msg=b'Pages', mode=1, color=0x0A, pages=(pa, pb), keys=(K['a'],))))
    for label, keys in (('drain3', (K['a'], K['x'], K['a'], K['x'])), ('drainext', (K['a'], K['f1'], K['a'])), ('ext', (K['f1'],)), ('zero', (K['zero'],)), ('zerothen', (K['zero'], K['a']))):
        out.append((f'n-keys-{label}', dict(msg=b'Press a key', mode=1, color=0x0E, keys=keys)))
    out += [(f'n-flags{fl:04x}', dict(msg=b'Fl', mode=1, color=0x09, flags=fl, keys=(K['a'],))) for fl in A.FLAGS + tuple(x for x in A.DIRTY_FLAGS if not x & 0x200)]
    return out


def c_run(image_reloc, o, comseg, ret_off, kb_sps):
    m = bytearray([0x5A]) * SIZE
    b = NBASE * 16; m[b:b + len(image_reloc)] = image_reloc
    ds = DATA_SEG
    m[physical(ds, 0x4A1E)] = o.get('mode', 1)
    pages = o.get('pages', (0xA000, 0xA200)); put16(m, ds, 0x4A16, pages[0]); put16(m, ds, 0x4A18, pages[1])
    put16(m, ds, 0x5E20, FONTOFF); put16(m, ds, 0x5E22, NFONT)
    for j, v in enumerate((1, 2, 4, 8)):
        m[physical(ds, 0x25C9 + j)] = v
    for j, v in enumerate((0, 0x5555, 0xAAAA, 0xFFFF)):
        put16(m, ds, 0x25CD + 2 * j, v)
    for j in range(256):
        m[physical(ds, 0x25D5 + j)] = j % 4
    for j in range(8):
        put16(m, ds, 0x25E5 + 2 * j, 0xC000 >> (2 * j))
    for j in range(2048):
        m[physical(NFONT, FONTOFF + j)] = ((j * 37 + (j >> 3) * 13) ^ 0xA6) & 255
    m[physical(ds, 0x5372)] = 1; m[physical(ds, 0x5373)] = 0; put16(m, ds, 0x5374, 0); put16(m, ds, 0x2612, 1); m[physical(ds, 0x4A1C)] = 3
    m[physical(ds, 0x49F7)] = 0; m[physical(ds, 0x8501)] = 0; m[physical(ds, 0x8502)] = 0
    put16(m, ds, 0x4422, NPSP); m[physical(NPSP, 0x80)] = 0; m[physical(NPSP, 0x81)] = 0x0D
    msg = o.get('msg', b'Insert disk')
    m[physical(NMSG, 0x100)] = len(msg) & 0xFF
    for k, ch in enumerate(msg[:255]):
        m[physical(NMSG, 0x101 + k)] = ch
    sp = TOP - 10
    put16(m, NSS, sp, ret_off); put16(m, NSS, sp + 2, comseg)
    put16(m, NSS, sp + 4, 0x0100); put16(m, NSS, sp + 6, NMSG); put16(m, NSS, sp + 8, (0xAB << 8) | o.get('color', 0x0E))
    c = Cpu(0x1234, 0x3456, 0x5678, 0x789A, 0x9ABC, 0xBCDE, 0x7777, sp, NBASE + A.SEG['vid'], A.ENTRY, ds, 0x1111, NSS, o.get('flags', 2))
    spec = dict(table={8: (0x0123, 0xF000)}, in61=0xB0, time_start=4736147, time_step=7)
    return A.run_c_native(m, c, spec, list(o.get("keys", (K["a"],))), None, kb_sps)


def program(rows, regions):
    ds_seg = DATA_SEG
    writes = []
    for seg, off, n in regions:
        for k in range(0, n, 0x4000):
            writes += [f'mov ax,{seg}', 'mov ds,ax', 'mov bx,word ptr cs:[handle]', f'mov dx,{off + k}', f'mov cx,{min(0x4000, n - k)}', 'mov ah,0x40', 'int 0x21']
    for plane in range(4):                                                    # the planes, through GC read mode 0 and map select
        writes += ['mov dx,0x3ce', 'mov ax,0x0005', 'out dx,ax', f'mov ax,{(plane << 8) | 4}', 'out dx,ax']
        for off in (0, 0x8000):
            writes += ['mov ax,0xa000', 'mov ds,ax', f'mov si,{off}', 'mov ax,0x7000', 'mov es,ax', 'xor di,di', 'mov cx,0x8000', 'cld', 'rep movsb',
                       'mov ax,0x7000', 'mov ds,ax', 'mov bx,word ptr cs:[handle]', 'xor dx,dx', 'mov cx,0x8000', 'mov ah,0x40', 'int 0x21']
    writes += ['push cs', 'pop ds', 'mov bx,word ptr [handle]', 'mov dx,offset kidx', 'mov cx,2', 'mov ah,0x40', 'int 0x21']      # the number of keys taken
    fill = []
    for chunk in range(2):
        fill += [f'mov ax,{NBASE + 0x1000 * chunk}', 'mov es,ax', 'xor di,di', 'mov cx,0x8000', 'mov ax,0x5a5a', 'rep stosw']
    load = []
    for k, start in enumerate(range(0, A.IMAGE_LEN, 0x8000)):
        load += [f'mov ax,{NBASE + 0x800 * k}', 'mov ds,ax', 'mov bx,word ptr cs:[image]', 'xor dx,dx',
                 f'mov cx,{min(0x8000, A.IMAGE_LEN - start)}', 'mov ah,0x3f', 'int 0x21', 'jc failure']
    planes = ['mov dx,0x3ce', 'mov ax,0x0000', 'out dx,ax', 'mov ax,0x0001', 'out dx,ax', 'mov ax,0x0003', 'out dx,ax', 'mov ax,0x0004', 'out dx,ax', 'mov ax,0x0005', 'out dx,ax',
              'mov ax,0xff08', 'out dx,ax', 'mov ax,0xa000', 'mov es,ax']
    for plane in range(4):
        planes += ['mov dx,0x3c4', f'mov ax,{((1 << plane) << 8) | 2}', 'out dx,ax', 'xor di,di', f'mov ax,{PATTERN[plane] * 257}', 'mov cx,0x8000', 'rep stosw']
    planes += ['mov dx,0x3c4', 'mov ax,0x0f02', 'out dx,ax']
    regs = (0x1234, 0x3456, 0x5678, 0x789A, 0x9ABC, 0xBCDE, 0x7777)
    t = ['.code16', '.intel_syntax noprefix', '.global _start', '_start: jmp main', '.org 0x200', 'main:',
         'mov ax,cs', 'mov ds,ax', 'mov ss,ax', 'mov sp,0xfff0', 'cld', 'mov ax,0x000d', 'int 0x10',
         'mov dx,offset outname', 'xor cx,cx', 'mov ah,0x3c', 'int 0x21', 'jc failure', 'mov word ptr [handle],ax',
         'mov word ptr [header],cs', 'mov word ptr [header+2],offset returned', f'mov word ptr [header+4],{len(rows)}', 'mov word ptr [header+6],0',
         'mov bx,word ptr [handle]', 'mov dx,offset header', 'mov cx,8', 'mov ah,0x40', 'int 0x21',
         'xor ax,ax', 'mov es,ax', 'mov ax,word ptr es:[0x58]', 'mov word ptr [old16],ax', 'mov ax,word ptr es:[0x5a]', 'mov word ptr [old16+2],ax',
         'again:', 'cld'] + fill + [
         'push cs', 'pop ds', 'mov dx,offset imagename', 'mov ax,0x3d00', 'int 0x21', 'jc failure', 'mov word ptr cs:[image],ax'] + load + [
         'push cs', 'pop ds', 'mov bx,word ptr [image]', 'mov ah,0x3e', 'int 0x21',
         'mov ax,word ptr [index]', 'mov bx,24', 'mul bx', 'add ax,offset table', 'mov bx,ax',                          # bx = the row (twelve words)
         f'mov ax,{ds_seg}', 'mov es,ax', 'mov al,byte ptr cs:[bx+2]', 'mov byte ptr es:[0x4a1e],al', 'mov ax,word ptr cs:[bx+4]', 'mov word ptr es:[0x4a16],ax',
         'mov ax,word ptr cs:[bx+6]', 'mov word ptr es:[0x4a18],ax', f'mov word ptr es:[0x5e20],{FONTOFF}', f'mov word ptr es:[0x5e22],{NFONT}',
         'mov di,0x25c9', 'mov si,offset masks', 'mov cx,4', 'rep movsb', 'mov di,0x25cd', 'mov si,offset gtab', 'mov cx,8', 'rep movsb',
         'mov di,0x25d5', 'mov si,offset attr_table', 'mov cx,256', 'rep movsb', 'mov di,0x25e5', 'mov si,offset ctab', 'mov cx,16', 'rep movsb',
         f'mov ax,{NFONT}', 'mov es,ax', f'mov di,{FONTOFF}', 'mov si,offset font', 'mov cx,2048', 'rep movsb',
         f'mov ax,{ds_seg}', 'mov es,ax',
         'mov byte ptr es:[0x5372],1', 'mov byte ptr es:[0x5373],0', 'mov word ptr es:[0x5374],0', 'mov word ptr es:[0x2612],1', 'mov byte ptr es:[0x4a1c],3',
         'mov byte ptr es:[0x49f7],0', 'mov byte ptr es:[0x8501],0', 'mov byte ptr es:[0x8502],0', f'mov word ptr es:[0x4422],{NPSP}',
         f'mov ax,{NPSP}', 'mov es,ax', 'mov byte ptr es:[0x80],0', 'mov byte ptr es:[0x81],0x0d',
         'push cs', 'pop ds', 'mov word ptr [kidx],0', 'mov ax,word ptr [bx+14]', 'mov word ptr [kcount],ax',
         'mov ax,word ptr [bx+16]', 'mov word ptr [keys],ax', 'mov ax,word ptr [bx+18]', 'mov word ptr [keys+2],ax',
         'mov ax,word ptr [bx+20]', 'mov word ptr [keys+4],ax', 'mov ax,word ptr [bx+22]', 'mov word ptr [keys+6],ax',
         'mov ax,word ptr [bx+8]', f'mov cx,{SLOT}', 'mul cx', 'add ax,offset msgs', 'mov si,ax', f'mov ax,{NMSG}', 'mov es,ax', 'mov di,0x100', f'mov cx,{SLOT}', 'rep movsb'] + planes + [
         'xor ax,ax', 'mov es,ax', 'mov word ptr es:[0x58],offset int16', 'mov word ptr es:[0x5a],cs',
         f'mov ax,{NSS}', 'mov es,ax', f'mov di,{LOW}', f'mov cx,{WINDOW // 2}', 'mov ax,0x5a5a', 'rep stosw',
         'cli', f'mov ax,{NSS}', 'mov ss,ax', f'mov sp,{TOP}',
         'push word ptr cs:[bx+10]', f'push {NMSG}', 'push 0x100',                                                   # colour word, then the message far pointer
         'push cs', 'push offset returned', 'push word ptr cs:[bx]',
         f'mov ax,{ds_seg}', 'mov ds,ax', 'mov ax,0x1111', 'mov es,ax']
    for r, v in zip('ax bx cx dx si di bp'.split(), regs):
        t.append(f'mov {r},{v:#x}')
    t += ['popf', f'ljmp {NBASE + A.SEG["vid"]},{A.ENTRY}', 'returned:']
    t += [f'mov word ptr cs:[out+{2 * n}],{r}' for n, r in enumerate('ax bx cx dx si di bp sp cs'.split())]
    t += ['mov word ptr cs:[out+18],offset returned', 'mov word ptr cs:[out+20],ds', 'mov word ptr cs:[out+22],es', 'mov word ptr cs:[out+24],ss',
          'mov ax,cs', 'mov ss,ax', 'mov sp,0xfff0', 'pushf', 'pop ax', 'mov word ptr cs:[out+26],ax',
          'mov ax,cs', 'mov ds,ax', 'xor ax,ax', 'mov es,ax', 'mov ax,word ptr [old16]', 'mov word ptr es:[0x58],ax', 'mov ax,word ptr [old16+2]', 'mov word ptr es:[0x5a],ax',
          'mov bx,word ptr [handle]', 'mov dx,offset out', 'mov cx,28', 'mov ah,0x40', 'int 0x21'] + writes + [
          'push cs', 'pop ds', 'sti', 'inc word ptr [index]', f'cmp word ptr [index],{len(rows)}', 'jae done', 'jmp again', 'done:',
          'mov bx,word ptr [handle]', 'mov ah,0x3e', 'int 0x21', 'mov ax,3', 'int 0x10', 'mov ax,0x4c00', 'int 0x21', 'failure:', 'mov ax,0x4c01', 'int 0x21']
    t += ['int16:', 'cmp ah,1', 'je kb_peek', 'cmp ah,0', 'je kb_take', 'jmp dword ptr cs:[old16]',
          'kb_peek:', 'push bp', 'mov bp,sp', 'push bx', 'mov bx,word ptr cs:[kidx]', 'cmp bx,word ptr cs:[kcount]', 'jae peek_empty', 'shl bx,1',
          'mov ax,word ptr cs:[keys+bx]', 'and word ptr [bp+6],0xffbf', 'jmp peek_done',
          'peek_empty:', 'or word ptr [bp+6],0x40',
          'peek_done:', 'pop bx', 'pop bp', 'iret',
          'kb_take:', 'push bx', 'mov bx,word ptr cs:[kidx]', 'shl bx,1', 'mov ax,word ptr cs:[keys+bx]', 'inc word ptr cs:[kidx]', 'pop bx', 'iret',
          'outname: .asciz "RESULT.BIN"', 'imagename: .asciz "IMAGE.BIN"', 'handle: .word 0', 'image: .word 0', 'index: .word 0', 'kidx: .word 0', 'kcount: .word 0',
          '.align 2', 'keys: .word 0,0,0,0', 'header: .space 8', 'out: .space 28', 'old16: .word 0,0',
          'masks: .byte 1,2,4,8', 'gtab: .word 0,0x5555,0xaaaa,0xffff', 'ctab: .word ' + ','.join(str(0xC000 >> (2 * j)) for j in range(8)),
          'attr_table: .byte ' + ','.join(str(j % 4) for j in range(256)), 'font: .byte ' + ','.join(str(((j * 37 + (j >> 3) * 13) ^ 0xA6) & 255) for j in range(2048)),
          '.align 2', 'msgs:']
    for msg in MESSAGES:
        slot = bytes([len(msg) & 0xFF]) + msg[:255]
        t.append('.byte ' + ','.join(str(x) for x in slot.ljust(SLOT, b'\0')))
    t += ['.align 2', 'table:'] + ['.word ' + ','.join(str(v) for v in row) for row in rows]
    return t


SLOT = 260
MESSAGES = []


def native(image, sites, decoded, f, lib, audit):
    global MESSAGES, A, K, DATA_SEG
    A = audit; K = A.K; DATA_SEG = NBASE + A.SEG['data']
    A.F, A.LIB = f, lib
    image_reloc = A.Q.relocate(image, sites, NBASE)
    A.IMAGE_LEN = len(image_reloc)
    sp_specs = specs()
    directory = ROOT / 'resources/0709-0bf5/native'; directory.mkdir(parents=True, exist_ok=True)
    (directory / 'IMAGE.BIN').write_bytes(image_reloc)
    MESSAGES = []
    for _, o in sp_specs:
        if o.get('msg', b'Insert disk') not in MESSAGES:
            MESSAGES.append(o.get('msg', b'Insert disk'))
    rows = []
    for _, o in sp_specs:
        keys = list(o.get('keys', (K['a'],)))
        check(len(keys) <= 4, 'at most four scripted keys')
        pages = o.get('pages', (0xA000, 0xA200))
        rows.append((o.get('flags', 2), o.get('mode', 1), pages[0], pages[1], MESSAGES.index(o.get('msg', b'Insert disk')), (0xAB << 8) | o.get('color', 0x0E),
                     len(keys), *(keys + [0] * (4 - len(keys)))))
    # row words: flags, mode, pageA, pageB, msg_idx, colour_word, kcount, k0..k3 -> pad to twelve words; the table reads: [0] flags [2] mode [4] pageA [6] pageB [8] msg [10] colour [14] kcount [16..22] keys
    rows = [(r[0], r[1], r[2], r[3], r[4], r[5], 0, r[6], r[7], r[8], r[9], r[10]) for r in rows]
    regions = [(NSS, LOW, WINDOW), (DATA_SEG, 0, 0x10000)]
    t = program(rows, regions)
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
                       env=dict(os.environ, SDL_VIDEODRIVER='dummy', SDL_AUDIODRIVER='dummy'), stdout=log, stderr=subprocess.STDOUT, check=True, timeout=1800)
    data = out.read_bytes(); record = 28 + sum(n for _, _, n in regions) + 4 * 65536 + 2
    check(len(data) == 8 + len(rows) * record, f'native length {len(data)} expected {8 + len(rows) * record}')
    comseg, ret_off, count, _ = struct.unpack_from('<4H', data); check(count == len(rows), 'native count')
    compared = 0; changed = 0; touched = 0
    for n, (name, o) in enumerate(sp_specs):
        rec = data[8 + n * record:8 + (n + 1) * record]
        taken = struct.unpack_from('<H', rec, record - 2)[0]
        kb_sps = []
        stop, status, c, ram, env, planes = c_run(image_reloc, o, comseg, ret_off, kb_sps)
        check(stop == 'RETURN', f'native C stop {name}: {stop}')
        check(taken == sum(1 for e in env.log if e[0] == '16' and e[1] == 0), f'native keys taken {name}: DOS {taken}, C {sum(1 for e in env.log if e[0] == "16" and e[1] == 0)}')
        for (reg, _), want in zip(Cpu._fields_, struct.unpack_from('<14H', rec)):
            check(getattr(c, reg) == want, f'native CPU {name}/{reg}: C {getattr(c, reg):04x} DOS {want:04x}')
        frame_mask = {physical(NSS, (sp - k) & 0xFFFF) for sp in kb_sps for k in range(1, 11)}
        at = 28
        for seg, off, length in regions:
            for k in range(length):
                p = physical(seg, (off + k) & 0xFFFF)
                if p in frame_mask:
                    continue
                check(ram[p] == rec[at + k], f'native {name} at {seg:04x}:{(off + k) & 0xFFFF:04x}: C {ram[p]:02x} DOS {rec[at + k]:02x}')
            at += length
            compared += length
        got = rec[at:at + 4 * 65536]
        if got != planes[:4 * 65536]:
            diff = [(hex(i), planes[i], got[i]) for i in range(4 * 65536) if planes[i] != got[i]]
            raise ValueError(f'native {name}: planes differ at {diff[:6]} ({len(diff)} bytes)')
        compared += 4 * 65536
        delta = sum(1 for p in range(4) for b in got[p * 65536:(p + 1) * 65536] if b != PATTERN[p])
        changed += delta; touched += delta > 0
    probe = subprocess.run([binary, '--version'], capture_output=True, text=True, env=dict(os.environ, SDL_VIDEODRIVER='dummy'))
    version = next(line.split(',')[0].strip() for line in (probe.stdout + probe.stderr).splitlines() if 'DOSBox-X version' in line)
    return dict(cases=len(rows), result_sha256=sha256(data).hexdigest(), compared_units=compared, plane_bytes_per_case=4 * 65536, plane_bytes_changed_from_fill=changed, cases_that_drew=touched,
                regions=[f'{s:04X}:{o:04X}+{n:X}' for s, o, n in regions], machine='ega', video_mode='0Dh', image_bin_sha256=sha256(image_reloc).hexdigest(),
                dosbox_x=dict(path=binary, sha256=sha256(open(binary, 'rb').read()).hexdigest(), version=version,
                              pinned_source='5a9c7c53481c2e933948039c33256776653f4faf', pinned_build_used=False))
