#!/usr/bin/env python3
"""RES_0802_0000 (the FreeMem wrapper) against the original bytes with FreeMem 0AF8:0254 REAL.

The whole loader-expanded START.EXE image is loaded at a test base with the loader's own relocation table applied. No DOS
service is involved. The heap is the GetMem audit's fixture (HeapOrg/HeapPtr/HeapEnd/FreeList/HeapError in DS, free blocks
chained from FreeList to HeapPtr). Catchers, per claim:
- full state: the unmasked CPU14 + 1 MiB differential at the stop (RETURN, RUNERROR at 0AF8:00E2);
- the wrapper's size rounding (the 16-bit wrap to a no-op) and FreeMem's refusals and top-of-heap release: an independent
  statement (expected());
- native (DOSBox-X): the C model against the real CPU, RunError captured at 0AF8:00E2 (declared).
Media stay under ignored resources/."""
import argparse, ctypes, json, os, random, struct, subprocess, sys, tempfile, time
from hashlib import sha256
from pathlib import Path
from capstone import Cs, CS_ARCH_X86, CS_MODE_16
from res0709_ega_reset_0000_cpu_audit import Cpu, ROOT, SIZE, REGISTERS, physical, put16, check
import res0899_quit_0000_cpu_audit as Q
import res0802_0716_cpu_audit as A716
CHECKER_ROWS = ('RES_0802_0000',)
ENTRY, END = 0x0000, 0x0020
DIGEST = 'a88fd0caa6f04a9c644a895914c4fe187d2880def44d34c07ba28cd0300b3458'
BASE, PSEG, HEAP = Q.BASE, 0x5800, 0x7000
SOURCES = ['resident_0802_0000_cpu', 'system_freemem_0254_cpu']
STATUS = {0: 'RETURN', 1: 'ARGUMENT', 2: 'RUNERROR', 3: 'LIMIT', 4: 'CALLEE'}
STEPS = 4096


class Diag(ctypes.Structure):
    _fields_ = [('callee_status', ctypes.c_int)]


def source(image):
    body = image[0x802 * 16 + ENTRY:0x802 * 16 + END]
    check(sha256(body).hexdigest() == DIGEST, '0802:0000 identity')
    ins = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(body, ENTRY))
    check(len(ins) == 12 and sum(i.size for i in ins) == len(body), 'decode covers 0000')
    check([(i.address, i.op_str) for i in ins if i.mnemonic == 'lcall'] == [(0x0017, '0xaf8, 0x254')], 'one far call, FreeMem')
    check(not [i for i in ins if i.mnemonic == 'call' or i.mnemonic.startswith('j')], 'no near calls, no branches')
    return body, {i.address: i for i in ins}


def library(directory):
    path = directory / 'r0000.so'
    subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Wpedantic', '-Werror', '-O2', '-fPIC', '-shared', '-I', str(ROOT / 'include')]
                   + [str(ROOT / f'src/recovered/{n}.c') for n in SOURCES] + ['-o', str(path)], check=True)
    f = ctypes.CDLL(str(path)).pr_r0802_0000
    f.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t, ctypes.POINTER(Cpu), ctypes.c_uint, ctypes.POINTER(Diag)]
    f.restype = ctypes.c_int
    return f


def build(reloc, *, size, ptr=(HEAP, 0), used=0x10, heap_paras=0x2000, blocks=(), var=(PSEG, 0x0010), ss=0x6000, sp=0xF000,
          flags=2, regs=None, base=BASE, ret=(0x9001, 0x7000), heap=HEAP):
    """Heap: HeapOrg = heap:0; HeapPtr = heap + used (the allocated part below it); HeapEnd = heap + heap_paras; free blocks
    (segment, offset, paragraphs, granule) chained from FreeList to HeapPtr (the GetMem audit's format). The pointer
    variable at `var` holds `ptr` (segment, offset)."""
    m = bytearray([0x5A]) * SIZE
    m[base * 16:base * 16 + len(reloc)] = reloc
    ds = base + Q.SEG['data']
    end = base * 16 + len(reloc)
    if end < physical(ds, 0xFFFF):
        m[end:physical(ds, 0xFFFF) + 1] = bytes(physical(ds, 0xFFFF) + 1 - end)
    top = (heap + used, 0)
    he = (0x00A9, base + Q.SEG['system'])
    for off, v in ((0x4400, 0), (0x4402, heap), (0x4404, top[1]), (0x4406, top[0]), (0x4408, 0), (0x440A, heap + heap_paras),
                   (0x4414, he[0]), (0x4416, he[1])):
        put16(m, ds, off, v)
    chain = list(blocks) + [(top[0], top[1], None, None)]
    put16(m, ds, 0x440C, chain[0][1]); put16(m, ds, 0x440E, chain[0][0])
    for (seg, off, paras, granule), nxt in zip(chain, chain[1:]):
        put16(m, seg, off, nxt[1]); put16(m, seg, (off + 2) & 0xFFFF, nxt[0])
        put16(m, seg, (off + 4) & 0xFFFF, granule); put16(m, seg, (off + 6) & 0xFFFF, paras)
    put16(m, var[0], var[1], ptr[1]); put16(m, var[0], (var[1] + 2) & 0xFFFF, ptr[0])
    ss = ds if ss == 'ds' else ss
    for k, w in enumerate((ret[1], ret[0], size, var[1], var[0])):
        put16(m, ss, (sp + 2 * k) & 0xFFFF, w)
    r = regs or (0x1234, 0x3456, 0x5678, 0x789A, 0x9ABC, 0xBCDE, 0x7777)
    return m, Cpu(*r, sp, base + 0x0802, ENTRY, ds, 0x1111, ss, flags), dict(ret=ret, base=base)


def run_unicorn(name, m, c, spec, decoded):
    from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE, UC_HOOK_INTR
    from unicorn import x86_const as R
    base = spec['base']; sysseg = base + Q.SEG['system']
    uc = Uc(UC_ARCH_X86, UC_MODE_16); backing = (ctypes.c_uint8 * SIZE).from_buffer_copy(m)
    uc.mem_map_ptr(0, SIZE, 7, ctypes.addressof(backing)); uc.mem_map_ptr(SIZE, SIZE, 7, ctypes.addressof(backing))
    for (n, _), reg in zip(Cpu._fields_, REGISTERS):
        uc.reg_write(reg, getattr(c, n))
    s = dict(stop=None); seen = set()

    def code(em, address, size, user):
        cs = uc.reg_read(R.UC_X86_REG_CS); ip = uc.reg_read(R.UC_X86_REG_IP)
        if (cs, ip) == spec['ret']:
            s['stop'] = 'RETURN'; return uc.emu_stop()
        if (cs, ip) == (sysseg, 0x00E2):
            s['stop'] = 'RUNERROR'; return uc.emu_stop()
        check(cs in (base + 0x0802, sysseg), f'escape {name} {cs:04x}:{ip:04x}')
        if cs == base + 0x0802:
            check(ip in decoded, f'escape inside 0000 {name} {ip:04x}'); seen.add(ip)

    def intr(em, intno, user):
        check(False, f'ENV refusal {name}: INT {intno:#x}')
    uc.hook_add(UC_HOOK_CODE, code); uc.hook_add(UC_HOOK_INTR, intr)
    uc.emu_start(physical(c.cs, c.ip), 2 * SIZE, count=200000)
    check(s['stop'] is not None, f'original did not stop {name}')
    return s['stop'], Cpu(*[uc.reg_read(r) & 0xFFFF for r in REGISTERS]), bytes(backing), seen


def run_c(f, m, c):
    ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(m); cpu = Cpu.from_buffer_copy(c); d = Diag()
    status = STATUS[f(ram, SIZE, ctypes.byref(cpu), STEPS, ctypes.byref(d))]
    return status, cpu, bytes(ram)


def expected(o):
    """Independent statement: request := (size + 7) AND FFF8h in 16 bits. 0 -> FreeMem's no-op, with no pointer check, and the
    heap unchanged. Otherwise the pointer must have offset 0 or 8 and lie in [HeapOrg, HeapPtr), else RunError 204. A block
    ending exactly at HeapPtr (the top) is released by lowering HeapPtr to it (only stated for an empty free list)."""
    req = (o['size'] + 7) & 0xFFF8
    heap, used = o.get('heap', HEAP), o.get('used', 0x10)
    seg, off = o.get('ptr', (HEAP, 0))
    top = (heap + used, 0)
    if req == 0:
        return 'RETURN', top
    linear, lo, hi = seg * 16 + off, heap * 16, top[0] * 16
    if off not in (0, 8) or not lo <= linear < hi:
        return 'RUNERROR', top
    if linear + req == hi and not o.get('blocks'):
        return 'RETURN', (linear >> 4, linear & 15)
    return None, None


def cases():
    rng = random.Random(0x08020000)
    out = [('free-top-256', dict(size=0x100, used=0x10)), ('free-top-9-rounds-16', dict(size=9, used=0x01)),
           ('free-top-at-8', dict(size=8, ptr=(HEAP, 8), used=0x01)),
           ('free-middle', dict(size=0x100, used=0x20)), ('free-middle-second', dict(size=0x80, ptr=(HEAP + 0x08, 0), used=0x20)),
           ('size-0-no-check', dict(size=0, ptr=(0x1234, 3))),
           # the narrow fact: FFF9h..FFFFh + 7 wraps, AND FFF8h gives 0, so FreeMem does NOTHING (no check, no release)
           ('wrap-fff9-bogus-pointer', dict(size=0xFFF9, ptr=(0x1234, 3))), ('wrap-ffff-valid-pointer', dict(size=0xFFFF)),
           ('fff8-no-wrap-refused', dict(size=0xFFF8, used=0x10)),
           ('bad-offset-4', dict(size=0x10, ptr=(HEAP, 4))), ('below-heaporg', dict(size=0x10, ptr=(HEAP - 1, 0))),
           ('at-heapptr', dict(size=0x10, ptr=(HEAP + 0x10, 0), used=0x10)),
           ('duplicate', dict(size=0x10, used=0x20, blocks=((HEAP, 0, 1, 0),))),
           ('var-straddles-ffff', dict(size=0x100, var=(PSEG, 0xFFFE))), ('var-on-stack', dict(size=0x100, var=(0x6000, 0xF00C)))]
    out += [(f'flags{fl:x}', dict(size=0x100, flags=fl)) for fl in Q.FLAGS]
    out += [(f'sp{sp:x}', dict(size=0x100, sp=sp)) for sp in (0x002C, 0x002D, 0x8001, 0xFFF5, 0xFFF6)]
    out += [('ss-ds', dict(size=0x100, ss='ds', sp=0xE000))]
    for k in range(10):
        out.append((f'random{k}', dict(size=rng.randrange(65536), used=rng.choice((0x01, 0x10, 0x100, 0x1000)),
                                       flags=rng.choice(Q.FLAGS), regs=tuple(rng.randrange(65536) for _ in range(7)))))
    return out


def differential(reloc, decoded, f):
    seen, stops, checked, cs_ = set(), {}, 0, cases()
    for index, (name, o) in enumerate(cs_):
        t0 = time.time()
        print(f'[{index + 1}/{len(cs_)}] {name}: start', file=sys.stderr, flush=True)
        m, c, spec = build(reloc, **o)
        st, cpu, ram, v = run_unicorn(name, m, c, spec, decoded); seen |= v
        cst, ccpu, cram = run_c(f, m, c)
        check(cst == st, f'{name}: C {cst} original {st}')
        A716.compare_cpu(name, ccpu, cpu); A716.compare_ram(name, cram, ram)
        stops[st] = stops.get(st, 0) + 1
        kind, top = expected(o)
        if kind is not None and not name.startswith(('var-on-stack', 'ss-ds')):
            ds = BASE + Q.SEG['data']; var = o.get('var', (PSEG, 0x0010))
            got_top = (ram[physical(ds, 0x4406)] | ram[physical(ds, 0x4407)] << 8, ram[physical(ds, 0x4404)] | ram[physical(ds, 0x4405)] << 8)
            check(st == kind, f'{name}: stop {st} vs statement {kind}')
            check(st != 'RETURN' or got_top == top, f'{name}: HeapPtr {got_top} vs {top}')
            check(st != 'RUNERROR' or cpu.ax == 0x00CC, f'{name}: RunError AX {cpu.ax:04x}')
            want_ptr = o.get('ptr', (HEAP, 0))                     # the wrapper never clears the pointer variable
            got = (ram[physical(var[0], (var[1] + 2) & 0xFFFF)] | ram[physical(var[0], (var[1] + 3) & 0xFFFF)] << 8,
                   ram[physical(var[0], var[1])] | ram[physical(var[0], (var[1] + 1) & 0xFFFF)] << 8)
            check(got == want_ptr, f'{name}: pointer variable {got} vs {want_ptr}')
            checked += 1
        print(f'[{index + 1}/{len(cs_)}] {name}: {st}, {time.time() - t0:.1f}s', file=sys.stderr, flush=True)
    check(seen == set(decoded), f'starts missing {sorted(hex(a) for a in set(decoded) - seen)}')
    for kind in ('RETURN', 'RUNERROR'):
        check(stops.get(kind), f'stop kind {kind} not exercised')
    return dict(cases=len(cs_), stops=stops, starts=len(seen), starts_total=len(decoded), conditional_outcomes=0,
                independent_checks=checked, freemem_max_steps=STEPS, cpu_words=14, ram_bytes=SIZE, flag_masks=0)


NBASE, NSS, TOP, LOW, WINDOW = 0x3000, 0x6000, 0xF000, 0xE800, 0x0800


def native(image, sites, f):
    """The C model against the real CPU on the empty-free-list cases (releases, no-ops, refusals), RunError captured by a far
    JMP planted at 0AF8:00E2 (declared). No DOS service runs inside the body."""
    directory = ROOT / 'resources/0802-0000/native'; directory.mkdir(parents=True, exist_ok=True)
    reloc = Q.relocate(image, sites, NBASE)
    (directory / 'IMAGE.BIN').write_bytes(reloc)
    sysseg, ds, seg0802 = NBASE + Q.SEG['system'], NBASE + Q.SEG['data'], NBASE + 0x0802
    err_before = reloc[Q.SEG['system'] * 16 + 0xE2:Q.SEG['system'] * 16 + 0xE7]
    specs = [(n, o) for n, o in cases() if set(o) <= {'size', 'ptr', 'used', 'flags'}]
    rows = [(o.get('flags', 2), o['size'], o.get('used', 0x10), o.get('ptr', (HEAP, 0))[0], o.get('ptr', (HEAP, 0))[1])
            for _, o in specs]
    load = []
    for k, start in enumerate(range(0, len(reloc), 0x8000)):
        load += [f'mov ax,{NBASE + 0x800 * k}', 'mov ds,ax', 'mov bx,word ptr cs:[image]', 'xor dx,dx',
                 f'mov cx,{min(0x8000, len(reloc) - start)}', 'mov ah,0x3f', 'int 0x21', 'jc failure']
    regions = [(NSS, LOW, WINDOW), (PSEG, 0x0010, 4), (ds, 0x4400, 0x18), (ds, 0x8754, 2)]
    writes = []
    for seg, off, n in regions:
        writes += [f'mov ax,{seg}', 'mov ds,ax', 'mov bx,word ptr cs:[handle]', f'mov dx,{off}', f'mov cx,{n}', 'mov ah,0x40', 'int 0x21']
    t = ['.code16', '.intel_syntax noprefix', '.global _start', '_start: jmp main', '.org 0x200', 'main:',
         'mov ax,cs', 'mov ds,ax', 'mov ss,ax', 'mov sp,0xfff0', 'cld',
         'mov dx,offset outname', 'xor cx,cx', 'mov ah,0x3c', 'int 0x21', 'jc failure', 'mov word ptr [handle],ax',
         'mov word ptr [header],cs', 'mov word ptr [header+2],offset returned', f'mov word ptr [header+4],{len(rows)}',
         'mov bx,word ptr [handle]', 'mov dx,offset header', 'mov cx,6', 'mov ah,0x40', 'int 0x21',
         'mov dx,offset imagename', 'mov ax,0x3d00', 'int 0x21', 'jc failure', 'mov word ptr [image],ax'] + load + [
         'push cs', 'pop ds', 'mov bx,word ptr [image]', 'mov ah,0x3e', 'int 0x21',
         f'mov ax,{sysseg}', 'mov es,ax', 'mov byte ptr es:[0xe2],0xea', 'mov word ptr es:[0xe3],offset cap_err', 'mov word ptr es:[0xe5],cs',
         'again:', 'cld', 'push cs', 'pop ds',
         'mov si,word ptr [index]', 'mov ax,si', 'shl si,1', 'shl si,1', 'add si,ax', 'shl si,1', 'add si,offset table',   # 10 bytes/row
         f'mov ax,{ds}', 'mov es,ax', 'mov word ptr es:[0x4400],0', f'mov word ptr es:[0x4402],{HEAP}', 'mov word ptr es:[0x4404],0',
         'mov ax,word ptr cs:[si+4]', f'add ax,{HEAP}', 'mov word ptr es:[0x4406],ax', 'mov word ptr es:[0x440e],ax',
         'mov word ptr es:[0x4408],0', f'mov word ptr es:[0x440a],{HEAP + 0x2000}', 'mov word ptr es:[0x440c],0',
         'mov word ptr es:[0x4414],0xa9', f'mov word ptr es:[0x4416],{sysseg}', 'mov word ptr es:[0x8754],0',
         f'mov ax,{PSEG}', 'mov es,ax', 'mov ax,word ptr cs:[si+8]', 'mov word ptr es:[0x10],ax', 'mov ax,word ptr cs:[si+6]',
         'mov word ptr es:[0x12],ax',
         f'mov ax,{NSS}', 'mov es,ax', f'mov di,{LOW}', f'mov cx,{WINDOW // 2}', 'mov ax,0x5a5a', 'rep stosw',
         f'mov ax,{NSS}', 'mov ss,ax', f'mov sp,{TOP}', f'push {PSEG}', 'push 0x10', 'push word ptr cs:[si+2]',
         'push word ptr cs:[si]', f'mov ax,{ds}', 'mov ds,ax', 'mov ax,0x1111', 'mov es,ax',
         'mov ax,0x1234', 'mov bx,0x3456', 'mov cx,0x5678', 'mov dx,0x789a', 'mov si,0x9abc', 'mov di,0xbcde', 'mov bp,0x7777',
         'popf', f'lcall {seg0802},0x0000', 'returned:']
    t += [f'mov word ptr cs:[out+{2 * n}],{r}' for n, r in enumerate('ax bx cx dx si di bp sp cs'.split())]
    t += ['mov word ptr cs:[out+18],offset returned', 'jmp common', 'cap_err:']
    t += [f'mov word ptr cs:[out+{2 * n}],{r}' for n, r in enumerate('ax bx cx dx si di bp sp'.split())]
    t += [f'mov word ptr cs:[out+16],{sysseg}', 'mov word ptr cs:[out+18],0xe2',
          'common:', 'mov word ptr cs:[out+20],ds', 'mov word ptr cs:[out+22],es', 'mov word ptr cs:[out+24],ss',
          'mov ax,cs', 'mov ss,ax', 'mov sp,0xfff0', 'pushf', 'pop ax', 'mov word ptr cs:[out+26],ax', 'cld', 'push cs', 'pop ds',
          'mov bx,word ptr [handle]', 'mov dx,offset out', 'mov cx,28', 'mov ah,0x40', 'int 0x21'] + writes + [
          'push cs', 'pop ds', 'inc word ptr [index]', f'cmp word ptr [index],{len(rows)}', 'jae done', 'jmp again', 'done:',
          'mov bx,word ptr [handle]', 'mov ah,0x3e', 'int 0x21', 'mov ax,0x4c00', 'int 0x21', 'failure:', 'mov ax,0x4c01', 'int 0x21',
          'outname: .asciz "RESULT.BIN"', 'imagename: .asciz "IMAGE.BIN"', 'handle: .word 0', 'image: .word 0', 'index: .word 0',
          '.align 2', 'header: .space 6', 'out: .space 28', 'table:']
    t += ['.word ' + ','.join(str(v) for v in row) for row in rows]
    (directory / 'oracle.S').write_text('\n'.join(t) + '\n')
    subprocess.run(['as', '--32', str(directory / 'oracle.S'), '-o', str(directory / 'oracle.o')], check=True)
    subprocess.run(['ld', '-m', 'elf_i386', '-Ttext', '0x100', '--oformat', 'binary', str(directory / 'oracle.o'), '-o',
                    str(directory / 'ORACLE.COM')], check=True)
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
    data = out.read_bytes(); rec_len = 28 + sum(n for _, _, n in regions)
    check(len(data) == 6 + len(rows) * rec_len, f'native length {len(data)}')
    comseg, ret_off, count = struct.unpack_from('<3H', data); check(count == len(rows), 'native count')
    stops = {}
    for n, (name, o) in enumerate(specs):
        rec = data[6 + n * rec_len:6 + (n + 1) * rec_len]
        m, c, _ = build(reloc, base=NBASE, ss=NSS, sp=TOP - 10, ret=(comseg, ret_off), **o)
        m = bytearray(m); put16(m, ds, 0x8754, 0)
        m[physical(NSS, LOW):physical(NSS, LOW) + WINDOW] = b'\x5a' * WINDOW
        for k, w in enumerate((ret_off, comseg, o['size'], 0x0010, PSEG)):
            put16(m, NSS, TOP - 10 + 2 * k, w)
        st, cpu, ram = run_c(f, bytes(m), c)
        stops[st] = stops.get(st, 0) + 1
        for (reg, _), want in zip(Cpu._fields_, struct.unpack_from('<14H', rec)):
            check(getattr(cpu, reg) == want, f'native CPU {name}/{reg}: C {getattr(cpu, reg):04x} DOS {want:04x}')
        at = 28
        for seg, off, length in regions:
            mine = bytes(ram[physical(seg, (off + k) & 0xFFFF)] for k in range(length))
            check(mine == data[6 + n * rec_len + at:6 + n * rec_len + at + length], f'native {name} region {seg:04x}:{off:04x} differs')
            at += length
    probe = subprocess.run([binary, '--version'], capture_output=True, text=True, env=dict(os.environ, SDL_VIDEODRIVER='dummy'))
    version = next(line.split(',')[0].strip() for line in (probe.stdout + probe.stderr).splitlines() if 'DOSBox-X version' in line)
    return dict(cases=len(rows), stops=stops, cpu_words=14, dos_stack_mask_bytes_below_int_sp=0,
                compared_regions=[f'{s:04X}:{o:04X}+{n:X}' for s, o, n in regions],
                captures=dict(runerror=dict(at='0AF8:00E2', before=err_before.hex())), result_sha256=sha256(data).hexdigest(),
                dosbox_x=dict(path=binary, sha256=sha256(open(binary, 'rb').read()).hexdigest(), version=version,
                              pinned_source='5a9c7c53481c2e933948039c33256776653f4faf', pinned_build_used=False))


def main():
    p = argparse.ArgumentParser()
    for flag in ('--compile-only', '--unicorn', '--native'):
        p.add_argument(flag, action='store_true')
    p.add_argument('--output', type=Path); a = p.parse_args()
    with tempfile.TemporaryDirectory(prefix='r0000-') as tmp:
        f = library(Path(tmp))
        if a.compile_only:
            print('CHECKER_BUILD_OK'); return
        image, sites = Q.load_image()
        body, decoded = source(image)
        report = dict(target=dict(span='0802:[0000,0020)', bytes=len(body), instructions=len(decoded), sha256=DIGEST,
                                  image_sha256=Q.IMAGE_DIGEST, relocations=len(sites)))
        if a.unicorn:
            report['unicorn'] = differential(Q.relocate(image, sites, BASE), decoded, f)
        if a.native:
            report['native'] = native(image, sites, f)
    if a.output:
        a.output.write_text(json.dumps(report, sort_keys=True, indent=2) + '\n')
    print(json.dumps(report, sort_keys=True))


if __name__ == '__main__':
    main()
