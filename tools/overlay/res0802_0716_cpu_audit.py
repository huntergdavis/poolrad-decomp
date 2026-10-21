#!/usr/bin/env python3
"""RES_0802_0716, the game's ECL/DAX block loader (raw CPU), against the original bytes with every callee REAL.

The whole loader-expanded START.EXE image is loaded at a test base with the loader's own relocation table applied; no byte is
patched. The original runs end to end in Unicorn: this body, its 23 far calls into SYSTEM, and its 7 near-call sites into the
not-yet-modelled 0802 cluster (05CD open, 0193 prompt, 06E7 close, 0020 alloc, 050F decode, 0000 free). The C model composes
the SYSTEM models and stops at each near call; each stop is compared with the original's state at the callee's entry, and the
model is resumed from the ORIGINAL's state at the return (the two-phase seam of #1311), segment by segment.

Declared environment: a DOS file system serving INT 21h 3Dh/3Eh/3Fh/42h from an in-memory archive (any name opens it when
present; reads and seeks are byte-exact, with optional injected faults), the 0059 prompt path's 2Ch/25h/35h, INT 16h keys,
the INT 10h stub and ports 40h/43h/61h. Unicorn: CPU14 + full 1 MiB + file positions + the ordered DOS event log, per segment,
unmasked. Independent expectation: the prior lane model's directory/RLE semantics (re-stated here) on the real ECL corpus.
Media and generated programs stay under ignored resources/."""
import argparse, copy, ctypes, json, random, struct, subprocess, sys, tempfile
from hashlib import sha256
from pathlib import Path
from capstone import Cs, CS_ARCH_X86, CS_MODE_16
from res0709_ega_reset_0000_cpu_audit import Cpu, ROOT, SIZE, REGISTERS, physical, put16, check
import res0899_quit_0000_cpu_audit as Q
CHECKER_ROWS = ('RES_0802_0716',)
ENTRY, END = 0x0716, 0x094A
DIGEST = 'd14db7945a8e717bc536422426c5df4d35913bdd4c5f4fa10f8a51f5103b34d2'
BASE, SEG, SS, STUB_SEG = Q.BASE, dict(Q.SEG, file=0x0802), Q.SS, Q.STUB_SEG
CS0802, DS = BASE + 0x0802, BASE + Q.SEG['data']
HEAP = 0x3000                                           # heap paragraphs 3000..4FFF (above the image, below the stubs)
OUT_SEG = 0x5800                                        # the caller's output variables
RETURN_TO = (0x9001, 0x7000)
SOURCES = ['resident_0802_0716_cpu', 'system_strstore_fullstate_0b25', 'system_string_leaves_cpu', 'system_runtime_leaves_cpu',
           'system_str_1366_cpu', 'system_getmem_023f_cpu', 'system_iocheck_04a9_cpu', 'system_blockread_14fb_cpu',
           'system_filepos_1593_cpu', 'system_seek_1563_cpu', 'system_filecheck_14b6_cpu', 'system_longdiv_0a76_cpu']
NEAR = {0x05CD: 'OPEN', 0x0193: 'PROMPT', 0x06E7: 'CLOSE', 0x0020: 'ALLOC', 0x050F: 'DECODE', 0x0000: 'FREE'}
RESUME = {0x0778: 0x05CD, 0x084D: 0x0193, 0x08B0: 0x06E7, 0x08BF: 0x0020, 0x0920: 0x06E7, 0x0938: 0x050F, 0x0944: 0x0000}
CONDITIONALS = {0x077A: (0x0793, 0x077C), 0x0810: (0x0852, 0x0812), 0x0858: (0x0884, 0x085A), 0x0876: (0x0884, 0x0878),
                0x0878: (0x087D, 0x087A), 0x087F: (0x0884, 0x0881), 0x088A: (0x08B3, 0x088C)}
STATUS = {0: 'RETURN', 1: 'ARGUMENT', 2: 'OPEN', 3: 'PROMPT', 4: 'CLOSE', 5: 'ALLOC', 6: 'DECODE', 7: 'FREE', 8: 'QUIT',
          9: 'RUNERROR', 10: 'DIVIDE', 11: 'SERVICE', 12: 'CALLEE'}
READ_FN = ctypes.CFUNCTYPE(ctypes.c_int, ctypes.c_void_p, ctypes.POINTER(Cpu), ctypes.POINTER(Cpu), ctypes.POINTER(ctypes.c_uint8),
                           ctypes.c_size_t)
SEEK_FN = ctypes.CFUNCTYPE(ctypes.c_int, ctypes.c_void_p, ctypes.POINTER(Cpu), ctypes.POINTER(Cpu))


class Env716(ctypes.Structure):
    _fields_ = [('read', READ_FN), ('read_context', ctypes.c_void_p), ('filepos', SEEK_FN), ('filepos_context', ctypes.c_void_p),
                ('seek', SEEK_FN), ('seek_context', ctypes.c_void_p), ('getmem_steps', ctypes.c_uint)]


class Diag716(ctypes.Structure):
    _fields_ = [('site', ctypes.c_uint16), ('callee_status', ctypes.c_int)]


# ---------------------------------------------------------------------------------------------------- the archive
def rle_pack(data):
    """Pack with the decoder's controls: 00..7E = c+1 literals, 81..FF = repeat the next byte 256-c times (2..127)."""
    out, k = bytearray(), 0
    while k < len(data):
        run = 1
        while k + run < len(data) and data[k + run] == data[k] and run < 127:
            run += 1
        if run >= 3:
            out += bytes([256 - run, data[k]]); k += run; continue
        lit = bytearray()
        while k < len(data) and len(lit) < 127:
            r = 1
            while k + r < len(data) and data[k + r] == data[k] and r < 3:
                r += 1
            if r >= 3:
                break
            lit.append(data[k]); k += 1
        out += bytes([len(lit) - 1]) + lit
    return bytes(out)


def rle_unpack(packed, size):
    """Independent restatement of the prior lane model's decoder (docs/re/RESIDENT_ECL_DAX_BLOCK.md)."""
    out, k = bytearray(), 0
    while len(out) < size:
        c = packed[k]
        if c <= 0x7E:
            out += packed[k + 1:k + 2 + c]; k += 2 + c
        elif c >= 0x81:
            out += bytes([packed[k + 1]]) * (256 - c); k += 2
        else:
            raise ValueError('stall control')
    return bytes(out[:size])


def archive(blocks, extra_rows=()):
    """DAX layout: word header length H, then 9-byte rows (id, dword offset, word unpacked, word packed); data at 2 + H + offset."""
    rows, data = bytearray(), bytearray()
    for bid, raw in blocks:
        packed = rle_pack(raw)
        rows += struct.pack('<BIHH', bid, len(data), len(raw), len(packed)); data += packed
    for row in extra_rows:
        rows += row
    return struct.pack('<H', len(rows)) + bytes(rows) + bytes(data)


def parse_dax(blob):
    """Independent directory parse (prior lane model): [(id, offset, unpacked, packed)], header length."""
    h = struct.unpack_from('<H', blob)[0]
    return [struct.unpack_from('<BIHH', blob, 2 + 9 * k) for k in range(h // 9)], h


class FileSystem:
    """The declared DOS file service: one archive, any name; byte-exact reads/seeks; optional injected faults. A handle is the
    lowest free one; a successful close answers AL = 01h with AH kept (both as DOSBox-X 2026.01.02 measured natively)."""
    def __init__(self, blob=None, read_fail=None, seek_answer=None, first_handle=5, appear_after=None, gone_from=None,
                 seek_fail=False):
        self.blob, self.read_fail, self.seek_answer, self.appear_after = blob, read_fail, seek_answer, appear_after
        self.gone_from, self.seek_fail = gone_from, seek_fail   # declared faults: disk removed from the Nth open; LSEEK error
        self.handles, self.next, self.reads, self.opens, self.log = {}, first_handle, 0, 0, []

    def state(self):
        return dict(handles=dict(self.handles), next=self.next, reads=self.reads, opens=self.opens)

    def restore(self, st):
        self.handles, self.next, self.reads, self.opens = dict(st['handles']), st['next'], st['reads'], st.get('opens', 0)

    def call(self, ah, al, bx, cx, dx, read_name=None, write=None):
        """Returns (cf, ax, dx_out or None)."""
        if ah == 0x3D:
            self.opens += 1                                     # "insert disk": absent until the appear_after-th open attempt
            if self.blob is None or (self.appear_after is not None and self.opens < self.appear_after) or \
                    (self.gone_from is not None and self.opens >= self.gone_from):
                self.log.append(('3D', al, read_name, 'absent')); return 1, 2, None
            h = self.next                                       # DOS hands out the LOWEST free handle (>= the first)
            while h in self.handles:
                h += 1
            self.handles[h] = 0
            self.log.append(('3D', al, read_name, h)); return 0, h, None
        if ah == 0x3E:
            if bx not in self.handles:
                self.log.append(('3E', bx, 'invalid')); return 1, 6, None          # invalid handle: DOS error 6
            self.log.append(('3E', bx)); self.handles.pop(bx)
            return 0, (ah << 8) | 0x01, None                    # success: AL := 01h, AH kept (DOSBox-X, measured natively)
        if ah == 0x3F:
            self.reads += 1
            if self.read_fail == self.reads:
                self.log.append(('3F', bx, cx, 'fail')); return 1, 5, None
            pos = self.handles[bx]; chunk = self.blob[pos:pos + cx]
            write(chunk); self.handles[bx] = pos + len(chunk)
            self.log.append(('3F', bx, cx, len(chunk))); return 0, len(chunk), None
        if ah == 0x42:
            if self.seek_fail and al == 0:
                self.log.append(('42', al, bx, cx, dx, 'fail')); return 1, 6, None
            offset = (cx << 16) | dx
            offset -= (1 << 32) if offset & 0x80000000 else 0
            base = {0: 0, 1: self.handles[bx], 2: len(self.blob)}[al]
            new = (base + offset) & 0xFFFFFFFF
            self.handles[bx] = new
            answer = new if self.seek_answer is None or al != 0 else self.seek_answer(new)
            self.log.append(('42', al, bx, cx, dx, answer)); return 0, answer & 0xFFFF, answer >> 16
        raise ValueError(f'ENV refusal: INT 21h AH {ah:#x}')


# ---------------------------------------------------------------------------------------------------- state
def source(image):
    body = image[0x802 * 16 + ENTRY:0x802 * 16 + END]
    check(sha256(body).hexdigest() == DIGEST, '0802:0716 identity')
    ins = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(body, ENTRY))
    check(len(ins) == 242 and sum(i.size for i in ins) == len(body), 'decode covers 0716')
    check({i.address for i in ins if i.mnemonic.startswith('j') and i.mnemonic != 'jmp'} == set(CONDITIONALS), 'conditional sites')
    check(sum(1 for i in ins if i.mnemonic == 'lcall') == 23, '22 SYSTEM far calls + 0899:0000')
    check({i.address + i.size: int(i.op_str, 16) for i in ins if i.mnemonic == 'call'} == RESUME, 'the 7 near-call sites')
    return body, {i.address: i for i in ins}


def build(image_reloc, *, blob, block_id, flag557c=1, heap_paras=0x2000, read_fail=None, seek_answer=None, keys=(0x1C0D,),
          flags=2, sp=0xF000, regs=None, ds_name=b'ECL', ext=b'DAX', name=b'1', ss=SS, base=BASE, heap=HEAP, out=OUT_SEG,
          stub=(STUB_SEG, 0), first_handle=5, ret=RETURN_TO):
    m = bytearray([0x5A]) * SIZE
    b = base * 16; m[b:b + len(image_reloc)] = image_reloc
    ds = base + Q.SEG['data']
    # The data segment beyond the image is the program's BSS: zero, as DOS leaves it after the loader clears it.
    end = b + len(image_reloc)
    if end < physical(ds, 0xFFFF):
        m[end:physical(ds, 0xFFFF) + 1] = bytes(physical(ds, 0xFFFF) + 1 - end)
    put16(m, ds, 0x4426, 0)                                       # InOutRes
    m[physical(ds, 0x442C)] = 2                                   # FileMode
    m[physical(ds, 0x5376)] = 0                                   # directory table index
    m[physical(ds, 0x557C)] = flag557c                            # no disk prompt when absent
    path = b'C:\\POOLRAD\\'; m[physical(ds, 0x2614)] = len(path); m[physical(ds, 0x2615):physical(ds, 0x2615) + len(path)] = path
    m[physical(ds, 0x547C)] = len(ds_name); m[physical(ds, 0x547D):physical(ds, 0x547D) + len(ds_name)] = ds_name
    # Heap: HeapPtr = HeapOrg = FreeList (an empty free list), HeapEnd above it, HeapError = SYSTEM's default 00A9.
    for off, v in ((0x4404, 0), (0x4406, heap), (0x4408, 0), (0x440A, heap + heap_paras), (0x440C, 0), (0x440E, heap),
                   (0x4414, 0x00A9), (0x4416, base + Q.SEG['system'])):
        put16(m, ds, off, v)
    s = physical(*stub); m[s:s + len(Q.INT10_STUB)] = Q.INT10_STUB
    put16(m, 0, 0x40, stub[1]); put16(m, 0, 0x42, stub[0])
    # Caller frame: strings and output variables.
    for off, txt in ((0x0000, name), (0x0040, ext)):
        m[physical(out, off)] = len(txt); m[physical(out, off + 1):physical(out, off + 1) + len(txt)] = txt
    put16(m, out, 0x0080, 0xEEEE); put16(m, out, 0x0084, 0xEEEE); put16(m, out, 0x0086, 0xEEEE)
    frame = (ret[1], ret[0], 0x0084, out, 0x0080, out, block_id, 0x0040, out, 0x0000, out)
    ss = ds if ss == 'ds' else ss
    for k, w in enumerate(frame):
        put16(m, ss, (sp + 2 * k) & 0xFFFF, w)
    r = regs or (0x1234, 0x3456, 0x5678, 0x789A, 0x9ABC, 0xBCDE, 0x7777)
    c = Cpu(*r, sp, base + 0x0802, ENTRY, ds, 0x1111, ss, flags)
    return m, c, dict(blob=blob, read_fail=read_fail, seek_answer=seek_answer, keys=list(keys), base=base, out=out,
                      first_handle=first_handle, stub_seg=stub[0], ret=ret)


def snap(uc, backing, fs, kind, R):
    cpu = Cpu(*[uc.reg_read(r) & 0xFFFF for r in REGISTERS])
    return dict(kind=kind, cpu=cpu, ram=bytes(backing), fs=fs.state(), log=len(fs.log))


# ---------------------------------------------------------------------------------------------------- the original
def run_unicorn(name, m, c, spec, decoded):
    from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE, UC_HOOK_INTR, UC_HOOK_INSN
    from unicorn import x86_const as R
    qenv = Q.Env(table={8: (0x0123, 0xF000)}, in61=0xB0, time_start=4736147, time_step=7)
    fs = FileSystem(spec['blob'], spec['read_fail'], spec['seek_answer'], spec['first_handle']); keys = list(spec['keys'])
    base = spec['base']; cs0802 = base + 0x0802; int_sps = []
    uc = Uc(UC_ARCH_X86, UC_MODE_16); backing = (ctypes.c_uint8 * SIZE).from_buffer_copy(m)
    uc.mem_map_ptr(0, SIZE, 7, ctypes.addressof(backing)); uc.mem_map_ptr(SIZE, SIZE, 7, ctypes.addressof(backing))
    for (n, _), reg in zip(Cpu._fields_, REGISTERS):
        uc.reg_write(reg, getattr(c, n))
    s = dict(stop=None, steps=0, prev=None, pending=None); seen = set(); edges = set(); snaps = []
    known = {base + v for v in Q.SEG.values()} | {cs0802, spec['stub_seg'], base + 0x0A40}

    def code(em, address, size, user):
        cs = uc.reg_read(R.UC_X86_REG_CS); ip = uc.reg_read(R.UC_X86_REG_IP)
        if s['prev'] is not None and cs == cs0802:
            edges.add((s['prev'], ip))
        s['prev'] = None
        if (cs, ip) == spec['ret']:
            snaps.append(snap(uc, backing, fs, 'RETURN', R)); s['stop'] = 'RETURN'; return uc.emu_stop()
        if (cs, ip) == (base + Q.SEG['system'], 0x00E2):
            snaps.append(snap(uc, backing, fs, 'RUNERROR', R)); s['stop'] = 'RUNERROR'; return uc.emu_stop()
        if cs not in known:
            s['stop'] = 'TRANSFER'; return uc.emu_stop()
        sp = uc.reg_read(R.UC_X86_REG_SP); ss_ = uc.reg_read(R.UC_X86_REG_SS)
        ret_ip = struct.unpack('<H', bytes(backing[physical(ss_, sp):physical(ss_, sp) + 2]))[0]
        ret_cs = struct.unpack('<H', bytes(backing[physical(ss_, (sp + 2) & 0xFFFF):physical(ss_, (sp + 2) & 0xFFFF) + 2]))[0]
        if (cs, ip) == (base + 0x0899, 0x0000) and (ret_cs, ret_ip) == (cs0802, 0x0852):
            snaps.append(snap(uc, backing, fs, 'QUIT', R)); s['stop'] = 'QUIT'; return uc.emu_stop()
        if cs == cs0802 and ip in NEAR and ret_cs == cs0802 and ret_ip in RESUME and RESUME[ret_ip] == ip:
            snaps.append(snap(uc, backing, fs, NEAR[ip], R)); s['pending'] = ret_ip
        if cs == cs0802 and ip == s['pending']:
            snaps.append(snap(uc, backing, fs, 'RESUME', R)); s['pending'] = None
        if cs == cs0802 and ENTRY <= ip < END and s['pending'] is None:
            check(ip in decoded, f'escape inside 0716 {name} {ip:04x}'); seen.add(ip)
            if ip in CONDITIONALS:
                s['prev'] = ip
        s['steps'] += 1
        if s['steps'] > 6000000:
            s['stop'] = 'RUNAWAY'; uc.emu_stop()

    def intr(em, intno, user):
        ax = uc.reg_read(R.UC_X86_REG_AX); ah, al = ax >> 8, ax & 0xFF
        fl = uc.reg_read(R.UC_X86_REG_EFLAGS)
        if intno == 0:
            snaps.append(snap(uc, backing, fs, 'DIVIDE', R)); s['stop'] = 'DIVIDE'; return uc.emu_stop()
        if intno == 0x16:
            check(ah in (0, 1), f'ENV refusal {name}: INT 16h AH {ah:#x}')
            if ah == 1:
                uc.reg_write(R.UC_X86_REG_EFLAGS, (fl & ~0x40) if keys else (fl | 0x40))
                if keys:
                    uc.reg_write(R.UC_X86_REG_AX, keys[0])
                return
            check(bool(keys), f'ENV refusal {name}: INT 16h with the key script empty')
            uc.reg_write(R.UC_X86_REG_AX, keys.pop(0)); return
        check(intno == 0x21, f'ENV refusal {name}: INT {intno:#x}')
        if ah == 0x2C:
            cx, dx = qenv.gettime(); uc.reg_write(R.UC_X86_REG_CX, cx); uc.reg_write(R.UC_X86_REG_DX, dx); return
        if ah in (0x25, 0x35):
            sp = uc.reg_read(R.UC_X86_REG_SP); ss_ = uc.reg_read(R.UC_X86_REG_SS)
            for k, v in ((2, fl & 0xFFFF), (4, uc.reg_read(R.UC_X86_REG_CS)), (6, uc.reg_read(R.UC_X86_REG_IP))):
                uc.mem_write(physical(ss_, (sp - k) & 0xFFFF), struct.pack('<H', v))
            out = qenv.dos(ah, al, dict(ds=uc.reg_read(R.UC_X86_REG_DS), dx=uc.reg_read(R.UC_X86_REG_DX)))
            for k, v in out.items():
                uc.reg_write(dict(es=R.UC_X86_REG_ES, bx=R.UC_X86_REG_BX)[k], v)
            return
        ds_, dx_ = uc.reg_read(R.UC_X86_REG_DS), uc.reg_read(R.UC_X86_REG_DX)
        int_sps.append((uc.reg_read(R.UC_X86_REG_SS), uc.reg_read(R.UC_X86_REG_SP)))
        name_ = None
        if ah == 0x3D:
            raw = bytes(backing[physical(ds_, dx_):physical(ds_, dx_) + 80]); name_ = raw.split(b'\0')[0].decode('latin-1')
        base_ = physical(ds_, dx_)

        def write(chunk):
            for k, v in enumerate(chunk):
                backing[(base_ + k) % SIZE] = v
        cf, ax_out, dx_out = fs.call(ah, al, uc.reg_read(R.UC_X86_REG_BX), uc.reg_read(R.UC_X86_REG_CX), dx_, name_, write)
        if ax_out is not None:
            uc.reg_write(R.UC_X86_REG_AX, ax_out)
        if dx_out is not None:
            uc.reg_write(R.UC_X86_REG_DX, dx_out)
        uc.reg_write(R.UC_X86_REG_EFLAGS, (fl | 1) if cf else (fl & ~1))

    def port_in(em, port, size, user):
        check(port == 0x61 and size == 1, f'ENV refusal {name}: IN {port:#x}'); return qenv.in61

    def port_out(em, port, size, value, user):
        check(port in (0x40, 0x43, 0x61, 0xC0) and size == 1, f'ENV refusal {name}: OUT {port:#x}')
    uc.hook_add(UC_HOOK_CODE, code); uc.hook_add(UC_HOOK_INTR, intr)
    uc.hook_add(UC_HOOK_INSN, port_in, None, 1, 0, R.UC_X86_INS_IN)
    uc.hook_add(UC_HOOK_INSN, port_out, None, 1, 0, R.UC_X86_INS_OUT)
    uc.emu_start(physical(c.cs, c.ip), 2 * SIZE, count=8000000)
    check(s['stop'] in ('RETURN', 'RUNERROR', 'QUIT', 'DIVIDE'), f'original stop {name}: {s["stop"]}')
    return snaps, fs, seen, edges, int_sps


# ---------------------------------------------------------------------------------------------------- the C model
def library(directory):
    path = directory / 'r0716.so'
    subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Wpedantic', '-Werror', '-O2', '-fPIC', '-shared', '-I', str(ROOT / 'include')]
                   + [str(ROOT / f'src/recovered/{n}.c') for n in SOURCES] + ['-o', str(path)], check=True)
    f = ctypes.CDLL(str(path)).pr_r0802_0716
    f.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t, ctypes.POINTER(Cpu), ctypes.POINTER(Env716), ctypes.POINTER(Diag716)]
    f.restype = ctypes.c_int
    return f


def run_c_segment(f, cpu, ram_bytes, spec, fs_state):
    """Run the C model from a state (CPU, RAM, file positions) to its next stop, with the same declared file service."""
    fs = FileSystem(spec['blob'], spec['read_fail'], spec['seek_answer'], spec['first_handle']); fs.restore(fs_state)

    def read(ctx, before, after, buffer, length):
        b = before[0]
        def write(chunk):
            for k, v in enumerate(chunk[:length]):
                buffer[k] = v
        cf, ax, _ = fs.call(b.ax >> 8, b.ax & 0xFF, b.bx, b.cx, b.dx, None, write)
        a = Cpu.from_buffer_copy(b); a.ax = ax; a.flags = (b.flags | 1) if cf else (b.flags & ~1); a.ip = (b.ip + 2) & 0xFFFF
        after[0] = a; return 0

    def lseek(ctx, before, after):
        b = before[0]
        cf, ax, dx = fs.call(b.ax >> 8, b.ax & 0xFF, b.bx, b.cx, b.dx)
        a = Cpu.from_buffer_copy(b); a.ax = ax
        if dx is not None:
            a.dx = dx
        a.flags = (b.flags | 1) if cf else (b.flags & ~1); a.ip = (b.ip + 2) & 0xFFFF
        after[0] = a; return 0
    keep = (READ_FN(read), SEEK_FN(lseek))
    env = Env716(keep[0], None, keep[1], None, keep[1], None, 100000)
    ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(ram_bytes); c = Cpu.from_buffer_copy(cpu); d = Diag716()
    status = STATUS[f(ram, SIZE, ctypes.byref(c), ctypes.byref(env), ctypes.byref(d))]
    return status, c, bytes(ram), fs, d


def compare_cpu(name, actual, want):
    for (n, _) in Cpu._fields_:
        check(getattr(actual, n) == getattr(want, n), f'CPU {name}/{n} C {getattr(actual, n):04x} original {getattr(want, n):04x}')


def compare_ram(name, actual, want):
    if actual != want:
        diff = [hex(i) for i, (a, b) in enumerate(zip(actual, want)) if a != b]
        raise ValueError(f'RAM {name}: {diff[:8]} ({len(diff)} bytes)')


TERMINAL = ('RETURN', 'QUIT', 'RUNERROR', 'DIVIDE')


def two_phase(name, m, c, spec, snaps, ofs, f):
    """Segment by segment: C from the entry (or the original's resume state) to its next stop vs the original's snapshot."""
    cpu, ram, fs_state, log_from = c, bytes(m), FileSystem(first_handle=spec['first_handle']).state(), 0
    seams, k = [], 0
    while True:
        status, actual, ram_c, fs_c, d = run_c_segment(f, cpu, ram, spec, fs_state)
        want = snaps[k]
        check(status == want['kind'], f'{name} segment {len(seams)}: C {status} original {want["kind"]} (site {d.site:04x}, '
                                      f'callee {d.callee_status})')
        compare_cpu(f'{name}#{len(seams)}', actual, want['cpu'])
        compare_ram(f'{name}#{len(seams)}', ram_c, want['ram'])
        check(fs_c.state() == want['fs'], f'{name}#{len(seams)} file state')
        check(fs_c.log == ofs.log[log_from:want['log']], f'{name}#{len(seams)} DOS event log: C {fs_c.log} original {ofs.log[log_from:want["log"]]}')
        seams.append(status)
        if status in TERMINAL:
            return seams, ram_c
        nxt = snaps[k + 1]
        if nxt['kind'] != 'RESUME':
            seams.append(f'callee-{nxt["kind"]}')          # the callee itself ended the program (its own terminal)
            return seams, None
        cpu, ram, fs_state, log_from, k = nxt['cpu'], nxt['ram'], nxt['fs'], nxt['log'], k + 2


def expected(spec, block_id, ram):
    """Independent statement of the effect on U^ and P^ for a run that returned."""
    blob, out = spec['blob'], spec['out']
    u = ram[physical(out, 0x80)] | ram[physical(out, 0x81)] << 8
    p_off = ram[physical(out, 0x84)] | ram[physical(out, 0x85)] << 8
    p_seg = ram[physical(out, 0x86)] | ram[physical(out, 0x87)] << 8
    if blob is None:
        return (u, p_seg, p_off) == (0, 0, 0)
    rows, h = parse_dax(blob)
    hit = next((r for r in rows if r[0] == block_id), None)
    if hit is None:
        return (u, p_seg, p_off) == (0, 0, 0)
    _, offset, unpacked, packed = hit
    data = rle_unpack(blob[2 + h + offset:2 + h + offset + packed], unpacked)
    at = physical(p_seg, p_off)
    return u == unpacked and bytes(ram[at:at + unpacked]) == data


def synthetic(rng, n=3, big=None):
    blocks = [(10 + k, bytes(rng.choice((0, 0, 7, rng.randrange(256))) for _ in range(rng.randrange(40, 900)))) for k in range(n)]
    extra = () if big is None else (struct.pack('<BIHH', big[0], 0, 16, big[1]),)
    return archive(blocks, extra), blocks


def cases():
    rng = random.Random(0x08020716)
    out = []
    for k in range(1, 9):
        path = ROOT / f'resources/DOS/POOLRAD/ECL{k}.DAX'
        blob = path.read_bytes()
        rows, _ = parse_dax(blob)
        out += [(f'ecl{k}-id{r[0]}', dict(blob=blob, block_id=r[0])) for r in rows]
        out.append((f'ecl{k}-absent', dict(blob=blob, block_id=0xEE)))
    synth, blocks = synthetic(rng)
    out += [('absent-file', dict(blob=None, block_id=1)), ('synth-first', dict(blob=synth, block_id=10)),
            ('synth-last', dict(blob=synth, block_id=12)), ('synth-absent', dict(blob=synth, block_id=99))]
    big, _ = synthetic(rng, 2, big=(0x30, 0x9000))
    out += [('packed-limit-prompt', dict(blob=big, block_id=0x30)), ('packed-limit-before-match', dict(blob=big, block_id=0x31))]
    for k in (1, 2, 3, 4, 5, 6):
        out.append((f'read-fault-{k}', dict(blob=synth, block_id=12, read_fail=k)))
    sparse = archive([(20, bytes(4000)), (21, bytes(range(256)) * 2)])
    out += [('heap-fails-in-alloc', dict(blob=synth, block_id=11, heap_paras=0x0002)),
            ('heap-fails-in-getmem', dict(blob=sparse, block_id=20, heap_paras=0x0020)),
            ('sparse-block', dict(blob=sparse, block_id=20)), ('incompressible-block', dict(blob=sparse, block_id=21)),
            ('sp-low', dict(blob=synth, block_id=12, sp=0x1200)), ('sp-high', dict(blob=synth, block_id=12, sp=0xFFEA)),
            ('ss-ds', dict(blob=synth, block_id=12, ss='ds', sp=0xE000)),
            ('position-high', dict(blob=synth, block_id=99, seek_answer=lambda new: new | 0x10000)),
            ('position-negative', dict(blob=synth, block_id=99, seek_answer=lambda new: new | 0x80000000))]
    out += [(f'flags{fl:x}', dict(blob=synth, block_id=11, flags=fl)) for fl in Q.FLAGS]
    for k in range(6):
        out.append((f'random{k}', dict(blob=synth, block_id=rng.choice((10, 11, 12, 99)),
                                       regs=tuple(rng.randrange(65536) for _ in range(7)), flags=rng.choice(Q.FLAGS))))
    return out


def differential(reloc, decoded, f):
    seen, edges, stops, checked, seam_count = set(), set(), {}, 0, 0
    for name, o in cases():
        o = dict(o); bid = o.pop('block_id')
        m, c, spec = build(reloc, block_id=bid, **o)
        snaps, ofs, v, e, _ = run_unicorn(name, m, c, spec, decoded); seen |= v; edges |= e
        seams, ram = two_phase(name, m, c, spec, snaps, ofs, f)
        seam_count += len(seams)
        stops[seams[-1]] = stops.get(seams[-1], 0) + 1
        if seams[-1] == 'RETURN':
            check(expected(spec, bid, ram), f'{name}: U^/P^ and the decoded block (independent expectation)'); checked += 1
    feasible = {(a, t) for a, ts in CONDITIONALS.items() for t in ts}
    missing = sorted(set(decoded) - seen)
    return dict(cases=len(cases()), final_stops=stops, segments=seam_count, starts=len(seen), starts_total=len(decoded),
                starts_missing=[hex(a) for a in missing], conditional_outcomes=len(edges & feasible),
                conditional_outcomes_total=len(feasible), outcomes_missing=sorted((hex(a), hex(b)) for a, b in feasible - edges),
                independent_checks=checked, cpu_words=14, ram_bytes=SIZE, flag_masks=0)


def main():
    p = argparse.ArgumentParser()
    for flag in ('--compile-only', '--unicorn', '--native'):
        p.add_argument(flag, action='store_true')
    p.add_argument('--output', type=Path); a = p.parse_args()
    with tempfile.TemporaryDirectory(prefix='r0716-') as tmp:
        f = library(Path(tmp))
        if a.compile_only:
            print('CHECKER_BUILD_OK'); return
        image, sites = Q.load_image()
        body, decoded = source(image)
        reloc = Q.relocate(image, sites, BASE)
        report = dict(target=dict(span='0802:[0716,094A)', bytes=len(body), instructions=len(decoded), sha256=DIGEST,
                                  image_sha256=Q.IMAGE_DIGEST, relocations=len(sites), load_base=f'{BASE:04X}'))
        global F, DECODED
        F, DECODED = f, decoded
        if a.unicorn:
            report['unicorn'] = differential(reloc, decoded, f)
        if a.native:
            import res0802_0716_native
            report['native'] = res0802_0716_native.native(image, sites, decoded, f, sys.modules[__name__])
    if a.output:
        a.output.write_text(json.dumps(report, sort_keys=True, indent=2) + '\n')
    print(json.dumps(report, sort_keys=True))


if __name__ == '__main__':
    main()
