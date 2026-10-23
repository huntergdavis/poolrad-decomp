#!/usr/bin/env python3
"""OVL_007F_0000 (unit 0x7F [0000,0020)) and its resident closure 00BA +13AB / +11EA, original CPU14/RAM oracle.

The body hands a far pointer to resident 00BA:13AB and returns AX := result + 05DCh. Every callee runs as its ORIGINAL bytes in the oracle and as the merged resident C model
in the C run. Compared unmasked after every case: CPU14, ALL 1,048,576 RAM bytes, the stop, and the remaining budget. The one far segment word (CS:000F) is the loader-relocated
resident segment; the audit places the resident closure at several different segments, so a model that hard-codes the segment cannot pass. Independent statements pin the far-call
frame, the returned value (against the ORIGINAL 13AB run alone) and the stack on return. Media stay under ignored resources/."""
import argparse, ctypes, json, random, struct, subprocess, sys, tempfile, time
from hashlib import sha256
from pathlib import Path
from capstone import Cs, CS_ARCH_X86, CS_MODE_16
from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_CS, UC_X86_REG_IP, UC_X86_REG_SP, UC_X86_REG_SS
from overlay_inventory import find_units, parse_mz
from resident_loader_map import unpack
from u86_procvars_3413_audit import Cpu, ROOT, SIZE, EXE_SHA, OVR_SHA, REGISTERS, physical, put16

CHECKER_ROWS = ('OVL_007F_0000',)
MASK = SIZE - 1
W_CS = 0x6000
W_SHA = '6801f21fb64d058573aaf8afc3174ca5bec2d870219cab7e8b8bb8f9ef46d6a2'
PINS = (('res', 0x11ea, 0x1282, 62, '8331051d67bce5f352938484858963a8e7e6758d43e47bd131f72b9c2980e020'),
        ('res', 0x13ab, 0x148b, 89, '22732f3cfe79dc6feb87722de05c3da242b1d3c7f984db735f8c3d38ef3b7bbe'))
SOURCES = ['overlay_u7f_entry0_0000_cpu', 'resident_00ba_character_0bb8_cpu', 'resident_00ba_character_helpers_cpu', 'overlay_u9e_entry16_10e4', 'overlay_u9e_entry17_110d']
STATUS = {0: 'RETURNED', 1: 'ARGUMENT', 2: 'CODE', 3: 'LIMIT', 4: 'TRANSFER', 5: 'CALLEE'}
NON_ARCHITECTURAL = ('ARGUMENT', 'CODE', 'TRANSFER', 'CALLEE')
SP_MIN, SP_MAX = 0x0040, 0xFFF0
ENTRY_RET = (0x8000, 0x9000)                                 # the entry frame's return address (not in the decoded set: reaching it ends the case)


class Diag(ctypes.Structure):
    _fields_ = [('callee_status', ctypes.c_int), ('site', ctypes.c_uint16)]


def check(ok, message):
    if not ok:
        raise ValueError(message)


# ---------------------------------------------------------------------------------------------------- the spans, from the original media
def source():
    exe = (ROOT / 'resources/DOS/POOLRAD/START.EXE').read_bytes(); ovr = (ROOT / 'resources/DOS/POOLRAD/GAME.OVR').read_bytes()
    check(sha256(exe).hexdigest() == EXE_SHA and sha256(ovr).hexdigest() == OVR_SHA, 'media provenance')
    unit = next(u for u in find_units(exe[parse_mz(exe)['header_bytes']:], ovr) if u['stub_para'] == 0x7F)
    body = ovr[unit['ovr_offset']:unit['ovr_offset'] + 0x20]
    ins = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(body, 0))
    check(len(body) == 32 and sha256(body).hexdigest() == W_SHA and len(ins) == 12 and sum(i.size for i in ins) == 32, 'unit 0x7F +0000 identity: 32 bytes / 12 instructions')
    check([(i.address, i.op_str) for i in ins if i.mnemonic == 'lcall'] == [(0x0C, '0xba, 0x13ab')], 'one far call, 00BA:13AB at 000C')
    check(not [i for i in ins if i.mnemonic.startswith('j') or i.mnemonic in ('call', 'int', 'in', 'out')], 'straight line: no jump, near call, interrupt or port instruction')
    check([(i.address, int(i.op_str, 0)) for i in ins if i.mnemonic == 'retf'] == [(0x1D, 4)], 'the only return is RETF 4 at 001D')
    check(sorted(f for f in unit['fixups'] if f < 0x20) == [0x0F], 'exactly one relocation in the span: the segment word at 000F')
    resident = bytes(unpack(exe)[0]); bodies = {('w', 0): body}; decoded = {('w', i.address): i for i in ins}
    for kind, lo, hi, count, digest in PINS:
        blob = resident[physical(0xBA, lo):physical(0xBA, hi)]
        r = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(blob, lo))
        check(sha256(blob).hexdigest() == digest and len(r) == count and sum(i.size for i in r) == len(blob), f'resident {lo:04X} identity')
        bodies[kind, lo] = blob; decoded.update(((kind, i.address), i) for i in r)
    check(not [i for k, i in decoded.items() if k[0] == 'res' and i.mnemonic in ('lcall', 'int', 'in', 'out')] and
          sorted((i.address, i.op_str) for k, i in decoded.items() if k[0] == 'res' and i.mnemonic == 'call') == [(0x13B8, '0x11ea')], 'the closure is 13AB -> 11EA, no other call, port or interrupt')
    return bodies, decoded, dict(bytes=32, instructions=12, sha256=W_SHA, file_offset=unit['ovr_offset'], resident=[dict(start=f'{lo:04X}', end=f'{hi:04X}', instructions=n, sha256=h) for _, lo, hi, n, h in PINS])


def library(directory, override=None):
    path = Path(directory) / ('u7f_0000_mut.so' if override else 'u7f_0000.so')
    subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Wpedantic', '-Werror', '-O2', '-fPIC', '-shared', '-I', str(ROOT / 'include')] +
                   [str((override or {}).get(s, ROOT / f'src/recovered/{s}.c')) for s in SOURCES] + ['-o', str(path)], check=True)
    lib = ctypes.CDLL(str(path)); args = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t, ctypes.POINTER(Cpu), ctypes.POINTER(ctypes.c_size_t)]
    lib.pr_u7f_entry0_0000_cpu.argtypes = args + [ctypes.POINTER(Diag)]; lib.pr_u7f_entry0_0000_cpu.restype = ctypes.c_int
    lib.pr_res00ba_helpers_cpu.argtypes = args; lib.pr_res00ba_helpers_cpu.restype = ctypes.c_int
    return lib


# ---------------------------------------------------------------------------------------------------- state
def fixture(bodies, *, rec=(0x3000, 0x0100), kind=7, minor=0, rec_bytes=None, resident_cs=0x7000, w_cs=W_CS, ds=0x1000, ss=0x2000, sp=0x8000, flags=0x0202, regs=None,
            call_entry=0x0000, background=0xA5, args=None):
    """The entry frame holds the pointer's offset at SP+4 and its segment at SP+6. The record's +10h byte is `kind` and +16h is `minor` unless rec_bytes overrides."""
    m = bytearray([background]) * SIZE
    r = regs or (0xABCD, 0x3456, 0x5678, 0x789A, 0x9ABC, 0xBCDE, 0x7777, 0x1111)
    cpu = Cpu(r[0], r[1], r[2], r[3], r[4], r[5], r[6], sp, w_cs, call_entry, ds, r[7], ss, flags | 0x0002)      # FLAGS bit 1 always reads 1 on the machine
    for k in range(0x40):
        m[physical(rec[0], (rec[1] + k) & 0xFFFF)] = (k * 7 + 3) & 0xFF
    if rec_bytes is None:
        m[physical(rec[0], (rec[1] + 0x10) & 0xFFFF)] = kind & 0xFF; m[physical(rec[0], (rec[1] + 0x16) & 0xFFFF)] = minor & 0xFF
    else:
        for off, v in rec_bytes.items():
            m[physical(rec[0], (rec[1] + off) & 0xFFFF)] = v & 0xFF
    # the code goes in AFTER the record, so a record placed over the code reads the code bytes and never overwrites them
    for (kindname, start), blob in bodies.items():
        seg = w_cs if kindname == 'w' else resident_cs
        at = physical(seg, start); m[at:at + len(blob)] = blob
    put16(m, w_cs, 0x0F, resident_cs)                                  # the loader's relocation of the one segment word
    frame = args or (ENTRY_RET[1], ENTRY_RET[0], rec[1], rec[0])
    for k, w in enumerate(frame):
        put16(m, ss, (sp + 2 * k) & 0xFFFF, w)
    return m, cpu, dict(resident_cs=resident_cs, w_cs=w_cs, rec=rec, sp=sp, ss=ss)


def run_c(lib, state, budget):
    memory, cpu, _ = state
    ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory); out = Cpu.from_buffer_copy(cpu); left = ctypes.c_size_t(budget); diag = Diag()
    status = lib.pr_u7f_entry0_0000_cpu(ram, SIZE, ctypes.byref(out), ctypes.byref(left), ctypes.byref(diag))
    return STATUS[status], bytes(ram), out, left.value, diag


def run_original(state, decoded, budget, entry=None, frame_key=('res', 0x13AB)):
    """Original bytes in Unicorn. Counts one unit per executed instruction of the wrapper and the resident closure; stops at the entry frame's return address (RETURNED), at the
    unit after the budget (LIMIT, CPU at the next unexecuted instruction), or when a callee returns somewhere unexpected (TRANSFER)."""
    memory, cpu, info = state; backing = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
    uc = Uc(UC_ARCH_X86, UC_MODE_16); uc.mem_map_ptr(0, SIZE, 7, ctypes.addressof(backing)); uc.mem_map_ptr(SIZE, SIZE, 7, ctypes.addressof(backing))
    for (name, _), reg in zip(Cpu._fields_, REGISTERS):
        uc.reg_write(reg, getattr(cpu, name))
    left = [budget]; calls = []; pending = [None]; status = [None]; seen = set(); edges = set(); frames = []; w_cs, res_cs = info['w_cs'], info['resident_cs']

    def word(seg, off):
        at = physical(seg, off & 0xFFFF); return backing[at] | (backing[(at + 1) & MASK] << 8)

    def hook(uc, address, size, user):
        cs = uc.reg_read(UC_X86_REG_CS); ip = uc.reg_read(UC_X86_REG_IP)
        if pending[0] is not None:
            target, expected = pending[0]; pending[0] = None
            if expected is None:
                status[0] = 'RETURNED' if target == ENTRY_RET else 'TRANSFER'; uc.emu_stop(); return
            if target != expected:
                status[0] = 'TRANSFER'; uc.emu_stop(); return
        key = ('w' if cs == w_cs else 'res' if cs == res_cs else None, ip)
        if key not in decoded:
            raise AssertionError(('unexpected original PC', key, hex(cs)))
        if not left[0]:
            status[0] = 'LIMIT'; uc.emu_stop(); return
        left[0] -= 1; seen.add(key)
        ins = decoded[key]
        if key == frame_key:
            sp = uc.reg_read(UC_X86_REG_SP); ss = uc.reg_read(UC_X86_REG_SS)
            frames.append(tuple(word(ss, sp + 2 * k) for k in range(4)))          # IP, CS, pointer offset, pointer segment as the resident sees them
        if ins.mnemonic in ('call', 'lcall'):
            calls.append((cs, (ip + ins.size) & 0xFFFF))
        if ins.mnemonic == 'retf':
            sp = uc.reg_read(UC_X86_REG_SP); ss = uc.reg_read(UC_X86_REG_SS)
            pending[0] = ((word(ss, sp + 2), word(ss, sp)), calls.pop() if calls else None)
    uc.hook_add(UC_HOOK_CODE, hook)
    try:
        uc.emu_start(physical(cpu.cs, cpu.ip), 2 * SIZE, count=max(2000, 4 * budget))
    except Exception as e:                                         # a hook-raised assertion surfaces here
        raise
    check(status[0] is not None, 'original execution did not reach return, limit or transfer')
    out = Cpu(*(uc.reg_read(r) & 0xFFFF for r in REGISTERS))
    return status[0], bytes(backing), out, left[0], seen, frames


def original_13ab_result(bodies, decoded, rec, resident_cs, kind_bytes, flags=0x0202):
    """The ORIGINAL 13AB alone, called by a tiny harness, for the independent value statement: returns its AX."""
    m = bytearray([0xA5]) * SIZE
    for (k, start), blob in bodies.items():
        if k == 'res':
            at = physical(resident_cs, start); m[at:at + len(blob)] = blob
    for off, v in kind_bytes.items():
        m[physical(rec[0], (rec[1] + off) & 0xFFFF)] = v
    ss, sp = 0x2000, 0x8000
    for k, w in enumerate((ENTRY_RET[1], ENTRY_RET[0], rec[1], rec[0])):
        put16(m, ss, sp + 2 * k, w)
    cpu = Cpu(0, 0, 0, 0, 0, 0, 0x7777, sp, resident_cs, 0x13AB, 0x1000, 0, ss, flags)
    state = (m, cpu, dict(resident_cs=resident_cs, w_cs=0x0000, rec=rec, sp=sp, ss=ss))
    st, ram, out, _, _, _ = run_original(state, {k: v for k, v in decoded.items() if k[0] == 'res'}, 100000)
    check(st == 'RETURNED', '13AB alone returned')
    return out.ax


def compare(label, state, bodies, decoded, lib, budget=100000):
    actual = run_c(lib, state, budget); expected = run_original(state, decoded, budget)
    check(actual[0] == expected[0], f'{label}: status C {actual[0]} original {expected[0]}')
    check(actual[3] == expected[3], f'{label}: budget left C {actual[3]} original {expected[3]}')
    # MUL leaves SF, ZF, AF and PF undefined; the ADD that follows each MUL overwrites them, so a COMPLETED call never exposes them. A LIMIT stop on the very next instruction does, and
    # Unicorn keeps the stale value where the model computes one: those flag bits (0x00D4) are not compared at exactly those arrivals, and the arrivals are counted and reported.
    after_mul = {('res', a.address + a.size) for k, a in decoded.items() if k[0] == 'res' and a.mnemonic == 'mul'}
    undefined = 0x00D4 if expected[0] == 'LIMIT' and ('res', expected[2].ip) in after_mul and expected[2].cs == state[2]['resident_cs'] else 0
    for name, _ in Cpu._fields_:
        mask = ~undefined & 0xFFFF if name == 'flags' else 0xFFFF
        check(getattr(actual[2], name) & mask == getattr(expected[2], name) & mask, f'{label}: {name} C {getattr(actual[2], name):04x} original {getattr(expected[2], name):04x}')
    if actual[1] != expected[1]:
        diffs = [i for i in range(SIZE) if actual[1][i] != expected[1][i]]
        raise ValueError(f'{label}: RAM {[(hex(i), actual[1][i], expected[1][i]) for i in diffs[:12]]} ({len(diffs)} bytes)')
    for (kind, start), blob in bodies.items():
        seg = state[2]['w_cs'] if kind == 'w' else state[2]['resident_cs']
        at = physical(seg, start); check(expected[1][at:at + len(blob)] == state[0][at:at + len(blob)], f'{label}: code written')
    return expected, actual, bool(undefined)


# ---------------------------------------------------------------------------------------------------- cases and independent statements
def cases(quick=False):
    rng = random.Random(0x7F0000)
    out = []

    def add(name, budget=100000, **kw):
        out.append((name, kw, budget))
    for k in range(256):
        add(f'kind-{k:02x}', kind=k, minor=k ^ 0x5A)
    for minor in range(256):
        add(f'minor-{minor:02x}', kind=0x12, minor=minor)
    for k in (0x12, 0x13, 0x1B, 0x1D, 0x20):
        for base in range(0xFFF0, 0x10000, 2):
            add(f'rec-top-{k:02x}-{base:04x}', rec=(0x3000, base), kind=k, minor=0x40)           # +10h / +16h wrap at the 64 KiB segment end for the lower bases
    for resident_cs in (0x7000, 0x7345, 0x0BA0, 0xA3C0, 0xFE00, 0x1234):
        add(f'resident-at-{resident_cs:04x}', resident_cs=resident_cs, kind=0x13)
    for w_cs in (0x6000, 0x2345, 0x9F00):
        add(f'wrapper-at-{w_cs:04x}', w_cs=w_cs, resident_cs=0x7345, kind=0x11)
    for sp in (SP_MIN, SP_MIN + 1, 0x0101, 0x7FFF, 0x8000, 0xF001, SP_MAX - 1, SP_MAX):
        add(f'sp-{sp:04x}', sp=sp, kind=0x12, minor=0x33)
    add('ss-ds', ss=0x1000, sp=0xE000, kind=0x16); add('ss-record', ss=0x3000, rec=(0x3000, 0x0100), sp=0x8000, kind=0x17)
    for fl in (0x0002, 0x0202, 0x0602, 0x0A87, 0x0402, 0x0E57, 0x08D7, 0x0893, 0x0003, 0x7202):
        add(f'flags-{fl:04x}', flags=fl, kind=0x15)
    for seg, off in ((0xFFFF, 0x0200), (0xFFF0, 0x0010), (0x0000, 0x0000), (0xA000, 0x0100), (0xAFFF, 0xFFF0)):
        add(f'record-{seg:04x}-{off:04x}', rec=(seg, off), kind=0x12, minor=0x70)
    # the pointer inside the frames this body and its callee create: the record's +10h byte lands on each stack word from SP-30 up to the entry frame
    sp0 = 0x8000
    for back in range(0, 0x24):
        add(f'record-in-frame-{back:02x}', rec=(0x2000, (sp0 - 6 - back - 0x10) & 0xFFFF), sp=sp0, rec_bytes={})
    add('record-in-code-wrapper', rec=(W_CS, (0 - 0x10) & 0xFFFF), rec_bytes={}); add('record-in-code-resident', rec=(0x7000, (0x13AB - 0x10) & 0xFFFF), rec_bytes={})
    add('record-over-args', rec=(0x2000, (sp0 + 4 - 0x10) & 0xFFFF), rec_bytes={})
    add('record-in-eg-window', rec=(0xA000, 0x0000), kind=0x14)
    for budget in range(0, 175):
        add(f'budget-{budget}', budget=budget, kind=0x12, minor=0x20)
    for k in range(12 if quick else 60):
        add(f'random{k}', kind=rng.randrange(256), minor=rng.randrange(256), regs=tuple(rng.randrange(65536) for _ in range(8)), flags=rng.choice((2, 0x202, 0x08D7, rng.randrange(65536) & ~0x0100)),        # TF is excluded: a single-step trap is an interrupt, outside this model
            rec=(rng.randrange(0x1000, 0x9000), rng.randrange(65536)), resident_cs=rng.choice((0x7000, rng.randrange(0x4000, 0x9000))), sp=rng.randrange(SP_MIN, SP_MAX + 1))
    return out


def statements(name, kw, st, bodies, decoded, expected):
    """Independent of the C model: (1) the frame the resident sees is the entry frame's pointer words with this body's return address; (2) AX is the ORIGINAL 13AB's result + 05DC,
    with the flags of that ADD; (3) the stack, BP and CS:IP are restored."""
    status, ram, out, left, seen, frames = expected
    if status != 'RETURNED':
        return 0
    memory, cpu, info = st
    rec = info['rec']
    check(frames and frames[0] == (0x0011, info['w_cs'], rec[1], rec[0]), f'{name}: the far-call frame {frames[:1]} is not (0011, {info["w_cs"]:04x}, pointer offset, pointer segment)')
    # 13AB alone is run on a copy of the record bytes; it is only comparable when the record does not overlap the stack words or the code the composed run writes or pushes
    lo = physical(info['ss'], (info['sp'] - 0x50) & 0xFFFF); hi = lo + 0x60
    recs = physical(rec[0], rec[1]); touches = lambda a, n: (a < hi and a + n > lo) if lo < hi else True
    alone = None if touches(recs, 0x40) else original_13ab_result(bodies, decoded, rec, info['resident_cs'], {off: memory[physical(rec[0], (rec[1] + off) & 0xFFFF)] for off in range(0x40)}, flags=cpu.flags)
    if alone is not None:
        check(out.ax == (alone + 0x05DC) & 0xFFFF, f'{name}: AX {out.ax:04x} is not 13AB alone {alone:04x} + 05DC')
        a, b = alone, 0x05DC; wide = a + b; r = wide & 0xFFFF
        want = (1 if wide > 0xFFFF else 0) | (4 if bin(r & 0xFF).count('1') % 2 == 0 else 0) | (0x10 if (a ^ b ^ r) & 0x10 else 0) | (0x40 if r == 0 else 0) | (0x80 if r & 0x8000 else 0) | \
               (0x800 if (~(a ^ b) & (a ^ r)) & 0x8000 else 0)
        check(out.flags & 0x08D5 == want, f'{name}: flags {out.flags & 0x08D5:04x} are not the ADD AX,05DC flags {want:04x}')
        check(out.flags & ~0x08D5 == cpu.flags & ~0x08D5, f'{name}: non-arithmetic flags changed')
    check(out.sp == (cpu.sp + 8) & 0xFFFF and out.bp == cpu.bp and (out.cs, out.ip) == ENTRY_RET, f'{name}: SP/BP/CS:IP on return')
    return 2 if alone is not None else 1


def differential(bodies, decoded, lib, quick=False, only=None):
    alone_checked = 0; undefined_arrivals = 0; stats = dict(stops={}, instructions=0); seen = set(); non_arch = []; stated = 0; n = 0; t0 = time.time(); cs = [c for c in cases(quick) if only is None or c[0] in only]
    for index, (name, kw, budget) in enumerate(cs):
        if index % 50 == 0 or name.startswith(('record-in', 'record-over', 'random')):
            print(f'[{index + 1}/{len(cs)}] {name}: START', file=sys.stderr, flush=True)
        st = fixture(bodies, **kw)
        expected, actual, masked = compare(name, st, bodies, decoded, lib, budget)
        undefined_arrivals += int(masked)
        stats['stops'][expected[0]] = stats['stops'].get(expected[0], 0) + 1; seen |= expected[4]
        if actual[0] in NON_ARCHITECTURAL:
            non_arch.append((name, actual[0]))
        check(actual[0] not in NON_ARCHITECTURAL, f'{name}: C stopped {actual[0]}')
        if budget == 100000 and 'rec_bytes' not in kw:
            r = statements(name, kw, st, bodies, decoded, expected); stated += 1 if r else 0; alone_checked += 1 if r == 2 else 0
        n += 1
    check(only is not None or len(seen & {k for k in decoded if k[0] == 'w'}) == 12, 'every wrapper instruction start was reached')
    return dict(cases=n, end_states=stats['stops'], starts=f'{len(seen)}/{len(decoded)}', wrapper_starts=f'{len([k for k in seen if k[0] == "w"])}/12',
                non_architectural_stops=len(non_arch), cpu_words=14, ram_bytes=SIZE, flag_masks=0, independent_statements_checked=stated, value_vs_13ab_alone_checked=alone_checked, mul_undefined_flag_arrivals=undefined_arrivals, seconds=round(time.time() - t0, 1))


def main():
    p = argparse.ArgumentParser(description=__doc__)
    for flag in ('--compile-only', '--unicorn', '--quick', '--mutate', '--native'):
        p.add_argument(flag, action='store_true')
    p.add_argument('--output', type=Path); a = p.parse_args()
    with tempfile.TemporaryDirectory(prefix='u7f-0000-') as tmp:
        lib = library(tmp)
        if a.compile_only:
            print('CHECKER_BUILD_OK'); return
        bodies, decoded, meta = source(); report = dict(target=meta)
        if a.unicorn:
            report['unicorn'] = differential(bodies, decoded, lib, a.quick)
        if a.native:
            from ovl_007f_0000_native import native
            report['native'] = native(bodies, lib)
        if a.mutate:
            report['mutation'] = mutation_check(bodies, decoded)
    if a.output:
        a.output.parent.mkdir(parents=True, exist_ok=True); a.output.write_text(json.dumps(report, sort_keys=True, indent=2) + '\n')
    print(json.dumps(report, sort_keys=True))


MUTANT_CASES = ('kind-01', 'kind-13', 'minor-33', 'resident-at-7345', 'wrapper-at-2345', 'budget-3', 'budget-8', 'budget-20', 'budget-60', 'flags-0a87', 'sp-0040', 'record-in-frame-04', 'rec-top-12-fff8')
EQUIVALENT = 'equivalent in the differential: the resident closure writes only its own frame, so no input makes it return anywhere but 0011; the C test cannot reach it either'
MUTANTS = (
    ('ADD constant 05DBh', [("arith16(c, c->ax, 0x05DCu, 0)", "arith16(c, c->ax, 0x05DBu, 0)")]),
    ('the two pointer words pushed swapped', [("push(ram, c, r16(ram, c->ss, (uint16_t)(c->bp + 8u)));", "push(ram, c, r16(ram, c->ss, (uint16_t)(c->bp + 6u)));"),
                                              ("push(ram, c, r16(ram, c->ss, (uint16_t)(c->bp + 6u))); c->ip = 0x000Cu;", "push(ram, c, r16(ram, c->ss, (uint16_t)(c->bp + 8u))); c->ip = 0x000Cu;")]),
    ('the resident segment hard-coded (not read live)', [("uint16_t resident = r16(ram, cs, 0x000Fu);", "uint16_t resident = 0x7000u; (void)r16(ram, cs, 0x000Fu);")]),
    ('the local store [BP-2] dropped', [("w16(ram, c->ss, (uint16_t)(c->bp - 2u), c->ax); c->ip = 0x0017u;", "c->ip = 0x0017u;")]),
    ('RETF pops 2 argument bytes', [("c->sp = (uint16_t)(c->sp + 4u);        /* 001D RETF 4 */", "c->sp = (uint16_t)(c->sp + 2u);        /* 001D RETF 4 */")]),
    ('SUB SP,4', [("arith16(c, c->sp, 2u, 1)", "arith16(c, c->sp, 4u, 1)")]),
    ('ADD carry never set', [("if (subtract ? a < b : wide > 0xFFFFu) f |= 0x0001u;", "if (subtract ? a < b : 0) f |= 0x0001u;")]),
    ('ADD auxiliary carry never set', [("if ((a ^ b ^ r) & 0x10u) f |= 0x0010u;", "")]),
    ('the far call is not charged to the budget', [("    --*budget;\n    {\n        uint16_t resident", "    {\n        uint16_t resident")]),
    ('the call offset check expects 13ACh', [("r16(ram, cs, 0x000Du) != 0x13ABu", "r16(ram, cs, 0x000Du) != 0x13ACu")]),
    ('the entry SP lower bound is 0050h', [("c->sp < 0x0040u", "c->sp < 0x0050u")]),
    ('a resident LIMIT is reported as CALLEE', [("return s == PR_RES00BA_LIMIT ? PR_U7F_0000_LIMIT : PR_U7F_0000_CALLEE;", "return PR_U7F_0000_CALLEE;")]),
    ('MOV BP,SP loads SP-2', [("c->bp = c->sp; c->ip = 0x0003u;", "c->bp = (uint16_t)(c->sp - 2u); c->ip = 0x0003u;")]),
    ('the return-address (TRANSFER) check dropped', [("if (c->cs != cs || c->ip != 0x0011u) return PR_U7F_0000_TRANSFER;", "")]),
)


def mutation_check(bodies, decoded):
    out = []; src = (ROOT / 'src/recovered/overlay_u7f_entry0_0000_cpu.c').read_text()
    for k, (name, edits) in enumerate(MUTANTS):
        t0 = time.time(); print(f'[mutant {k + 1}/{len(MUTANTS)}] START {name}', file=sys.stderr, flush=True)
        text = src
        for old, new in edits:
            check(text.count(old) == 1, f'mutant text occurs once: {name}: {old[:50]}'); text = text.replace(old, new)
        with tempfile.TemporaryDirectory(prefix='u7f-0000-mut-') as tmp:
            path = Path(tmp) / 'mutant.c'; path.write_text(text)
            try:
                mlib = library(tmp, override={'overlay_u7f_entry0_0000_cpu': path})
            except subprocess.CalledProcessError:
                out.append((name, 'COMPILE-ERROR')); continue
            try:
                differential(bodies, decoded, mlib, quick=True, only=MUTANT_CASES)
                out.append((name, 'SURVIVED'))
            except (ValueError, AssertionError) as e:
                out.append((name, 'CAUGHT: ' + str(e)[:90]))
        print(f'[mutant {k + 1}/{len(MUTANTS)}] {out[-1][1][:70]} ({time.time() - t0:.0f}s)', file=sys.stderr, flush=True)
    caught = sum(1 for _, r in out if r.startswith('CAUGHT'))
    return dict(total=len(out), caught=caught, results=out, equivalent_note=EQUIVALENT, cases=list(MUTANT_CASES))


if __name__ == '__main__':
    sys.modules['ovl_007f_0000_cpu_audit'] = sys.modules[__name__]        # the native module imports this audit by name and must share its ctypes classes
    main()
