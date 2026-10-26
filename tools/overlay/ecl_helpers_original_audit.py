#!/usr/bin/env python3
"""Original-byte checker for eleven credited models in src/recovered/ecl_helpers.c that cited only shared tooling.

Each body runs from the ORIGINAL bytes under Unicorn: GAME.OVR units 1Ch, 25h and 38h loaded at their own segments with their
segment fixups applied, and the loader-expanded START.EXE (hash-checked, its relocation table applied) for the SYSTEM leaves
the compares call (StrStore 0B25, FillChar 16A8, StrCmp 0C10). The C model is called through a plain-argument shim
(ecl_helpers_original_shim.c). Compared per call: the model's result (AX or AL), or the data-segment bytes it writes, AND
the whole 64 KiB data segment after the call, which must differ from the state before the call in exactly the model's
bytes. Nothing is masked.

  unit 1Ch +2AEB  PRINT RETURN        pr_rec_ecl_op_print_return     [49ED] += 1; [5E24] := 1; [5E25] += 1; [84DF] := 0 if set
  unit 25h +06AF  operand word        pr_rec_ecl_operand_word        AX = (high << 8) + low
  unit 25h +06D3  text code remap     pr_rec_ecl_char_remap          AL
  unit 25h +06FB  address class       pr_rec_ecl_address_class       AL
  unit 25h +1815  string compare      pr_rec_ecl_compare_strings     [6F78..6F7D]
  unit 25h +18EB  word compare        pr_rec_ecl_compare_words       [6F78..6F7D]
  unit 38h +0377, +03AC, +03E1, +0C06, +0C15  pr_rec_u38_entry8/9/10/30/31  [6816], [6822], [6818], [6819] (8 and 9 read record +A0h)

python3 tools/overlay/ecl_helpers_original_audit.py [--compile-only] [--output report.json]
Media stays under ignored resources/; only counts and hashes are reported."""
import argparse, ctypes, json, random, struct, subprocess, sys, tempfile
from hashlib import sha256
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parent))
from overlay_inventory import find_units, parse_mz

CHECKER_ROWS = ('OVL_001C_2AEB', 'OVL_0025_06AF', 'OVL_0025_06D3', 'OVL_0025_06FB', 'OVL_0025_1815', 'OVL_0025_18EB',
                'OVL_0038_0377', 'OVL_0038_03AC', 'OVL_0038_03E1', 'OVL_0038_0C06', 'OVL_0038_0C15')
ROOT = Path(__file__).resolve().parents[2]
SIZE = 1 << 20
BASE, DATA, SYSTEM = 0x1000, 0x0C7C, 0x0AF8
SEG = {0x1C: 0x5000, 0x25: 0x6000, 0x38: 0x7000}
CHAR = (0x9400, 0x0010)   # the character record DS:[5D92] points at (u38 entries 8 and 9)
SS, SP, RET = 0x8000, 0xF000, (0x9F00, 0x0000)
STRINGS = 0x9000
BODIES = {   # (unit, entry, end, instructions, RETF immediate, SHA-256 of [entry, end))
    'OVL_001C_2AEB': (0x1C, 0x2AEB, 0x2B14, 13, 0),
    'OVL_0025_06AF': (0x25, 0x06AF, 0x06D3, 16, 4),
    'OVL_0025_06D3': (0x25, 0x06D3, 0x06FB, 16, 2),
    'OVL_0025_06FB': (0x25, 0x06FB, 0x0756, 28, 2),
    'OVL_0025_1815': (0x25, 0x1815, 0x18EB, 86, 8),
    'OVL_0025_18EB': (0x25, 0x18EB, 0x1951, 36, 4),
    'OVL_0038_0377': (0x38, 0x0377, 0x03AC, 15, 10),
    'OVL_0038_03AC': (0x38, 0x03AC, 0x03E1, 15, 10),
    'OVL_0038_03E1': (0x38, 0x03E1, 0x0400, 13, 10),
    'OVL_0038_0C06': (0x38, 0x0C06, 0x0C15, 6, 10),
    'OVL_0038_0C15': (0x38, 0x0C15, 0x0C26, 6, 10),
}


def check(cond, msg):
    if not cond:
        raise ValueError(msg)


def phys(seg, off):
    return ((seg << 4) + (off & 0xFFFF)) % SIZE


def library(directory):
    so = directory / 'eclshim.so'
    subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Wpedantic', '-Werror', '-O2', '-fPIC', '-shared', '-I', str(ROOT / 'include'),
                    str(ROOT / 'tools/overlay/ecl_helpers_original_shim.c'), str(ROOT / 'src/recovered/ecl_helpers.c'), '-o', str(so)],
                   check=True)
    lib = ctypes.CDLL(str(so))
    u8, u16, p8 = ctypes.c_uint8, ctypes.c_uint16, ctypes.POINTER(ctypes.c_uint8)
    lib.shim_operand_word.argtypes, lib.shim_operand_word.restype = [u8, u8], u16
    lib.shim_char_remap.argtypes, lib.shim_char_remap.restype = [u8], u8
    lib.shim_address_class.argtypes, lib.shim_address_class.restype = [u16], u8
    lib.shim_compare_words.argtypes, lib.shim_compare_words.restype = [u16, u16, p8, p8], ctypes.c_int
    lib.shim_compare_strings.argtypes = [ctypes.c_char_p, ctypes.c_size_t, ctypes.c_char_p, ctypes.c_size_t, p8, p8]
    lib.shim_compare_strings.restype = ctypes.c_int
    lib.shim_print_return.argtypes = [ctypes.POINTER(u16), p8, p8, p8]
    lib.shim_print_return.restype = ctypes.c_int
    lib.shim_u38_entry.argtypes = [ctypes.c_int, p8, p8, ctypes.POINTER(ctypes.c_uint)]
    lib.shim_u38_entry.restype = ctypes.c_int
    return lib


def media():
    """The base 1 MiB image: relocated START.EXE at BASE, units 1Ch and 25h at their segments with fixups applied."""
    import res0899_quit_0000_cpu_audit as Q
    image, sites = Q.load_image()
    exe = (ROOT / 'resources/DOS/POOLRAD/START.EXE').read_bytes()
    ovr = (ROOT / 'resources/DOS/POOLRAD/GAME.OVR').read_bytes()
    units = {u['stub_para']: u for u in find_units(exe[parse_mz(exe)['header_bytes']:], ovr)}
    m = bytearray([0x5A]) * SIZE
    reloc = Q.relocate(image, sites, BASE)
    m[BASE * 16:BASE * 16 + len(reloc)] = reloc
    report = dict(image_sha256=Q.IMAGE_DIGEST, relocations=len(sites), bodies={})
    for unit, seg in SEG.items():
        u = units[unit]
        code = bytearray(ovr[u['ovr_offset']:u['ovr_offset'] + u['code_size']])
        for f in u['fixups']:
            struct.pack_into('<H', code, f, (struct.unpack_from('<H', code, f)[0] + BASE) & 0xFFFF)
        m[seg * 16:seg * 16 + len(code)] = code
    from capstone import Cs, CS_ARCH_X86, CS_MODE_16
    md = Cs(CS_ARCH_X86, CS_MODE_16)
    for rid, (unit, lo, hi, count, retf) in BODIES.items():
        body = bytes(m[phys(SEG[unit], lo):phys(SEG[unit], hi)])
        ins = list(md.disasm(body, lo))
        check(len(ins) == count and ins[-1].address + ins[-1].size == hi and ins[-1].mnemonic == 'retf'
              and (int(ins[-1].op_str, 16) if ins[-1].op_str else 0) == retf, f'{rid} extent differs')
        calls = [i for i in ins if i.mnemonic in ('lcall', 'call', 'int')]
        check(all(i.mnemonic == 'lcall' and i.op_str.startswith(f'{hex(BASE + SYSTEM)},') for i in calls), f'{rid} calls outside SYSTEM')
        report['bodies'][rid] = dict(span=f'unit {unit:02X}h +[{lo:04X},{hi:04X})', bytes=hi - lo, instructions=count,
                                     sha256=sha256(body).hexdigest(), far_calls=len(calls))
    return m, report


def run(m, unit, entry, words, budget=200000):
    """Far-call unit:entry with the Pascal argument words (pushed in order) on a fresh copy of m; return (uc, ram)."""
    from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE
    from unicorn import x86_const as R
    ram = bytearray(m)
    sp = SP
    for w in words:
        sp -= 2; struct.pack_into('<H', ram, phys(SS, sp), w & 0xFFFF)
    sp -= 4; struct.pack_into('<HH', ram, phys(SS, sp), RET[1], RET[0])
    uc = Uc(UC_ARCH_X86, UC_MODE_16)
    uc.mem_map(0, SIZE); uc.mem_write(0, bytes(ram))
    for reg, v in ((R.UC_X86_REG_CS, SEG[unit]), (R.UC_X86_REG_IP, entry), (R.UC_X86_REG_DS, BASE + DATA), (R.UC_X86_REG_ES, BASE + DATA),
                   (R.UC_X86_REG_SS, SS), (R.UC_X86_REG_SP, sp), (R.UC_X86_REG_BP, 0x7777), (R.UC_X86_REG_EFLAGS, 0x0002)):
        uc.reg_write(reg, v)
    done = []

    def hook(emu, address, size, user):
        if address == phys(*RET):
            done.append(1); emu.emu_stop()
    uc.hook_add(UC_HOOK_CODE, hook)
    uc.emu_start(phys(SEG[unit], entry), 2 * SIZE, count=budget)
    check(done, f'{unit:02X}:{entry:04X} did not return')
    return uc, bytearray(uc.mem_read(0, SIZE))


def ds_bytes(ram):
    at = phys(BASE + DATA, 0)
    return bytes(ram[at:at + 0x10000])


def expect_only(before, after, changes, label):
    """after's data segment must equal before's with exactly `changes` {offset: byte} applied."""
    want = bytearray(ds_bytes(before))
    for off, v in changes.items():
        want[off] = v
    got = ds_bytes(after)
    if got != bytes(want):
        diff = [hex(i) for i in range(0x10000) if got[i] != want[i]]
        raise ValueError(f'{label}: data segment differs at {diff[:8]} ({len(diff)} bytes)')


def differential(m, lib):
    from unicorn import x86_const as R
    rng = random.Random(0xEC1)
    counts = {}
    ds = BASE + DATA

    # unit 25h +06AF operand word: every (low, high) on a 1/4 grid plus the edges
    n = 0
    for low in list(range(0, 256, 3)) + [255]:
        for high in list(range(0, 256, 5)) + [255]:
            uc, ram = run(m, 0x25, 0x06AF, (low, high))
            want = lib.shim_operand_word(low, high)
            check(uc.reg_read(R.UC_X86_REG_AX) == want, f'06AF ({low:02x},{high:02x}): original {uc.reg_read(R.UC_X86_REG_AX):04x} C {want:04x}')
            expect_only(m, ram, {}, f'06AF ({low},{high})'); n += 1
    counts['OVL_0025_06AF'] = n

    # unit 25h +06D3 text code remap: every byte code
    for code in range(256):
        uc, ram = run(m, 0x25, 0x06D3, (code,))
        check(uc.reg_read(R.UC_X86_REG_AX) & 0xFF == lib.shim_char_remap(code), f'06D3 {code:02x}')
        expect_only(m, ram, {}, f'06D3 {code:02x}')
    counts['OVL_0025_06D3'] = 256

    # unit 25h +06FB address class: every range edge +-1 and 3000 random addresses
    edges = [0x4900, 0x4CFF, 0x6B00, 0x6EFF, 0x9700, 0x98FF, 0x9900, 0xB6FF]
    addrs = sorted({(e + d) & 0xFFFF for e in edges for d in (-1, 0, 1)} | {0, 0xFFFF} | {rng.randrange(0x10000) for _ in range(3000)})
    for a in addrs:
        uc, ram = run(m, 0x25, 0x06FB, (a,))
        check(uc.reg_read(R.UC_X86_REG_AX) & 0xFF == lib.shim_address_class(a), f'06FB {a:04x}')
        expect_only(m, ram, {}, f'06FB {a:04x}')
    counts['OVL_0025_06FB'] = len(addrs)

    def flags_of(ram):
        at = phys(ds, 0x6F78)
        return bytes(ram[at:at + 6])

    # unit 25h +18EB word compare: edges and random pairs, prior flag bytes random (the clear must hide them)
    pairs = [(a, b) for a in (0, 1, 0x7FFF, 0x8000, 0xFFFF) for b in (0, 1, 0x7FFF, 0x8000, 0xFFFF)]
    pairs += [(rng.randrange(0x10000), rng.randrange(0x10000)) for _ in range(400)] + [(v, v) for v in (5, 0x1234, 0xFFFE)]
    for a, b in pairs:
        prior = bytes(rng.randrange(256) for _ in range(6))
        mm = bytearray(m); at = phys(ds, 0x6F78); mm[at:at + 6] = prior
        uc, ram = run(mm, 0x25, 0x18EB, (a, b))
        out = (ctypes.c_uint8 * 6)()
        status = lib.shim_compare_words(a, b, (ctypes.c_uint8 * 6)(*prior), out)
        check(status == 0 and flags_of(ram) == bytes(out), f'18EB ({a:04x},{b:04x}): original {flags_of(ram).hex()} C {bytes(out).hex()}')
        expect_only(mm, ram, {0x6F78 + k: out[k] for k in range(6)}, f'18EB ({a:04x},{b:04x})')
    counts['OVL_0025_18EB'] = len(pairs)

    # unit 25h +1815 string compare: Pascal strings (length byte + text) at STRINGS, equal, prefix, longer, empty, max
    def rand_text(k):
        return bytes(rng.randrange(0x20, 0x7F) for _ in range(k))
    cases = [(b'', b''), (b'A', b''), (b'', b'A'), (b'ABC', b'ABC'), (b'ABC', b'ABD'), (b'ABD', b'ABC'), (b'AB', b'ABC'), (b'ABC', b'AB'),
             (b'\xff', b'\x01'), (b'\x01', b'\xff'), (bytes(255), bytes(255)), (b'Z' * 255, b'Z' * 254)]
    for _ in range(200):
        x = rand_text(rng.randrange(0, 12)); y = x if rng.random() < 0.3 else rand_text(rng.randrange(0, 12))
        cases.append((x, y))
    for x, y in cases:
        prior = bytes(rng.randrange(256) for _ in range(6))
        mm = bytearray(m); at = phys(ds, 0x6F78); mm[at:at + 6] = prior
        a_off, b_off = 0x0100, 0x0300
        for off, s in ((a_off, x), (b_off, y)):
            p = phys(STRINGS, off); mm[p] = len(s); mm[p + 1:p + 1 + len(s)] = s
        uc, ram = run(mm, 0x25, 0x1815, (STRINGS, a_off, STRINGS, b_off))
        out = (ctypes.c_uint8 * 6)()
        status = lib.shim_compare_strings(x, len(x), y, len(y), (ctypes.c_uint8 * 6)(*prior), out)
        check(status == 0 and flags_of(ram) == bytes(out), f'1815 ({x!r},{y!r}): original {flags_of(ram).hex()} C {bytes(out).hex()}')
        expect_only(mm, ram, {0x6F78 + k: out[k] for k in range(6)}, f'1815 ({len(x)},{len(y)})')
    counts['OVL_0025_1815'] = len(cases)

    # unit 1Ch +2AEB PRINT RETURN: [84DF] zero and non-zero, [5E25] including FF, PC including FFFF
    n = 0
    for b84df in (0, 1, 0x80, 0xFF):
        for b5e25 in (0, 1, 0x7F, 0xFF):
            for pc in (0x9900, 0xFFFF, rng.randrange(0x10000)):
                b5e24 = rng.randrange(256)
                mm = bytearray(m)
                struct.pack_into('<H', mm, phys(ds, 0x49ED), pc)
                mm[phys(ds, 0x84DF)], mm[phys(ds, 0x5E24)], mm[phys(ds, 0x5E25)] = b84df, b5e24, b5e25
                uc, ram = run(mm, 0x1C, 0x2AEB, ())
                cp, c84, c24, c25 = ctypes.c_uint16(pc), ctypes.c_uint8(b84df), ctypes.c_uint8(b5e24), ctypes.c_uint8(b5e25)
                status = lib.shim_print_return(ctypes.byref(cp), ctypes.byref(c84), ctypes.byref(c24), ctypes.byref(c25))
                check(status == 0, '2AEB status')
                expect_only(mm, ram, {0x49ED: cp.value & 0xFF, 0x49EE: cp.value >> 8, 0x84DF: c84.value, 0x5E24: c24.value, 0x5E25: c25.value},
                            f'2AEB (84DF={b84df:02x}, 5E25={b5e25:02x}, PC={pc:04x})')
                n += 1
    counts['OVL_001C_2AEB'] = n

    # unit 38h entries 8 (+0377), 9 (+03AC), 10 (+03E1), 30 (+0C06), 31 (+0C15): five discarded word arguments, RETF 0Ah.
    # Modifier bytes [6816], [6822], [6818], [6819] random with edges; record byte +A0h over 0..9 and random (entries 8, 9).
    mod_addrs = (0x6816, 0x6822, 0x6818, 0x6819)
    for rid, which, entry in (('OVL_0038_0377', 8, 0x0377), ('OVL_0038_03AC', 9, 0x03AC), ('OVL_0038_03E1', 10, 0x03E1),
                              ('OVL_0038_0C06', 30, 0x0C06), ('OVL_0038_0C15', 31, 0x0C15)):
        n = 0
        a0_values = list(range(10)) + [0x7F, 0x80, 0xFF] if which in (8, 9) else [0]
        for a0 in a0_values:
            for mods in ([0, 0, 0, 0], [0xFF, 0xFF, 0xFF, 0xFF], [2, 1, 0x80, 0x02], [0xFE, 0x03, 0x01, 0xFD], [0x7F, 0x80, 0x7F, 0x80]) + \
                    tuple([rng.randrange(256) for _ in range(4)] for _ in range(12)):
                mm = bytearray(m)
                for addr, v in zip(mod_addrs, mods):
                    mm[phys(ds, addr)] = v
                struct.pack_into('<HH', mm, phys(ds, 0x5D92), CHAR[1], CHAR[0])
                record = bytes(rng.randrange(256) for _ in range(285))
                record = record[:0xA0] + bytes([a0]) + record[0xA1:]
                rec_at = phys(*CHAR); mm[rec_at:rec_at + 285] = record
                uc, ram = run(mm, 0x38, entry, tuple(rng.randrange(0x10000) for _ in range(5)))
                io = (ctypes.c_uint8 * 4)(*mods); applied = ctypes.c_uint(0)
                status = lib.shim_u38_entry(which, io, (ctypes.c_uint8 * 285)(*record), ctypes.byref(applied))
                check(status == 0, f'{rid} status {status}')
                expect_only(mm, ram, {addr: io[k] for k, addr in enumerate(mod_addrs)}, f'{rid} (A0={a0:02x}, mods={bytes(mods).hex()})')
                check(bytes(ram[rec_at:rec_at + 285]) == record, f'{rid}: the record changed')
                n += 1
        counts[rid] = n
    return counts


def main():
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument('--compile-only', action='store_true')
    p.add_argument('--output', type=Path)
    a = p.parse_args()
    with tempfile.TemporaryDirectory() as tmp:
        lib = library(Path(tmp))
        if a.compile_only:
            print('CHECKER_BUILD_OK'); return
        m, report = media()
        report['cases'] = differential(m, lib)
        for _row in ('OVL_001C_2AEB', 'OVL_0025_06AF', 'OVL_0025_06D3', 'OVL_0025_06FB', 'OVL_0025_1815', 'OVL_0025_18EB', 'OVL_0038_0377', 'OVL_0038_03AC', 'OVL_0038_03E1', 'OVL_0038_0C06', 'OVL_0038_0C15'):
            print(f"ROW_EXECUTED {_row} PASS")
        report.update(compared='result register or written bytes, plus the whole 64 KiB data segment', masks=0)
    text = json.dumps(report, indent=1, sort_keys=True)
    if a.output:
        a.output.parent.mkdir(parents=True, exist_ok=True); a.output.write_text(text + '\n')
    print(json.dumps(report['cases'], sort_keys=True))


if __name__ == '__main__':
    main()
