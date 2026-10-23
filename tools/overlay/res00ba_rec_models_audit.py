#!/usr/bin/env python3
"""Original-byte checker for eight credited RES_00BA pr_rec_* models that cited only shared tooling (resident_loader_map.py).

Each body runs from the ORIGINAL bytes of the loader-expanded START.EXE (hash-checked, relocation table applied, at load
base 1000h) under Unicorn. The C model runs through the reusable io adapter (res00ba_io_adapter.py / .c), whose callbacks
read and write the same 1 MiB RAM image. Compared per call:
  - the model's result: AL (23B7 is_zero, 2E03/2E46 result) or AX (2637, 2E8B), and its status (OK);
  - every RAM byte OUTSIDE the call's own stack window [return address - 200h, return address + 4 + RETF bytes), which
    ends exactly at the arguments' top, so it is disjoint from the caller's frame (a non-zero sentinel there is
    compared). The models are contract-level and keep no stack, while the original keeps its locals (and StrLoad's
    copy) below the return address. No other byte is masked;
  - for 1509 and 15F7, the declared boundaries in call order. On the original side they are intercepted at their entries:
    GetMem 0AF8:023F returns the same caller-chosen block (RETF 2); FreeMem 0AF8:0254 and the message 0709:0BF5 are
    recorded and skipped (RETF 6); StrLoad 0AF8:0B0B and Move 0AF8:1685 run from the original bytes and are recorded.
    The models declare exactly these as their io boundaries; the literal's segment is compared relative to the load base.

  RES_00BA_1509  remove a 63-byte record from the character's +C8h list (FreeMem; two not-found messages)
  RES_00BA_15F7  append a copy of a 63-byte record (GetMem, Move)
  RES_00BA_23B7  character byte +10Eh == 0                    RES_00BA_23DB  party count into DS:6814/6815(+index)
  RES_00BA_2637  target reset                                  RES_00BA_2E03  link check
  RES_00BA_2E46  link flags (near-calls 2E03)                  RES_00BA_2E8B  target select into a four-byte slot

python3 tools/overlay/res00ba_rec_models_audit.py [--compile-only] [--output report.json]"""
import argparse, json, random, struct, sys, tempfile
from hashlib import sha256
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parent))
import res00ba_io_adapter as R

CHECKER_ROWS = ('RES_00BA_1509', 'RES_00BA_15F7', 'RES_00BA_23B7', 'RES_00BA_23DB', 'RES_00BA_2637', 'RES_00BA_2E03',
                'RES_00BA_2E46', 'RES_00BA_2E8B')
ROOT = Path(__file__).resolve().parents[2]
SIZE = 1 << 20
BASE, DATA, RES = 0x1000, 0x0C7C, 0x00BA
DS = BASE + DATA
SS, SP, RET = 0x7000, 0xF000, (0x9F00, 0x0000)
CHAR_SEG, NODE_SEG, ITEM_SEG, HEAP_SEG, SLOT_SEG = 0x9000, 0x8800, 0x8600, 0x8C00, 0x8400
BODIES = {   # entry, end, instructions, RETF immediate, near/far calls allowed
    'RES_00BA_1509': (0x1509, 0x15F7, 83, 8), 'RES_00BA_15F7': (0x15F7, 0x1693, 53, 8),
    'RES_00BA_23B7': (0x23B7, 0x23DB, 13, 4), 'RES_00BA_23DB': (0x23DB, 0x2433, 29, 0),
    'RES_00BA_2637': (0x2637, 0x267D, 20, 4), 'RES_00BA_2E03': (0x2E03, 0x2E46, 23, 4),
    'RES_00BA_2E46': (0x2E46, 0x2E8B, 27, 4), 'RES_00BA_2E8B': (0x2E8B, 0x2F57, 69, 8),
}
GETMEM, FREEMEM, MOVE, STRLOAD, MESSAGE = (BASE + 0x0AF8, 0x023F), (BASE + 0x0AF8, 0x0254), (BASE + 0x0AF8, 0x1685), \
    (BASE + 0x0AF8, 0x0B0B), (BASE + 0x0709, 0x0BF5)


def check(cond, msg):
    if not cond:
        raise ValueError(msg)


def phys(seg, off):
    return ((seg << 4) + (off & 0xFFFF)) % SIZE


def media():
    import res0899_quit_0000_cpu_audit as Q
    from capstone import Cs, CS_ARCH_X86, CS_MODE_16
    image, sites = Q.load_image()
    m = bytearray(SIZE)
    for k in range(0x100):                         # the caller's frame above the arguments: a non-zero sentinel, compared
        m[phys(SS, SP + k)] = (0x5B + 7 * k) & 0xFF or 0x5B
    reloc = Q.relocate(image, sites, BASE)
    m[BASE * 16:BASE * 16 + len(reloc)] = reloc
    md, report = Cs(CS_ARCH_X86, CS_MODE_16), dict(image_sha256=Q.IMAGE_DIGEST, relocations=len(sites), load_base=f'{BASE:04X}', bodies={})
    for rid, (lo, hi, count, retf) in BODIES.items():
        body = bytes(image[RES * 16 + lo:RES * 16 + hi])
        ins = list(md.disasm(body, lo))
        check(len(ins) == count and ins[-1].address + ins[-1].size == hi and ins[-1].mnemonic == 'retf'
              and (int(ins[-1].op_str, 16) if ins[-1].op_str else 0) == retf, f'{rid} extent differs')
        report['bodies'][rid] = dict(span=f'00BA:[{lo:04X},{hi:04X})', bytes=hi - lo, instructions=count, sha256=sha256(body).hexdigest())
    return m, report


def run(m, entry, words, block=None, ax=0):
    """Far-call 00BA:entry with argument words pushed in order; returns (uc, ram, boundaries)."""
    rid = next(k for k, v in BODIES.items() if v[0] == entry)
    check(2 * len(words) == BODIES[rid][3], f'{rid}: pushed argument bytes differ from RETF {BODIES[rid][3]}')
    from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE
    from unicorn import x86_const as X
    ram = bytearray(m)
    sp = SP
    for w in words:
        sp -= 2; struct.pack_into('<H', ram, phys(SS, sp), w & 0xFFFF)
    sp -= 4; struct.pack_into('<HH', ram, phys(SS, sp), RET[1], RET[0])
    import ctypes
    uc = Uc(UC_ARCH_X86, UC_MODE_16)
    backing = (ctypes.c_uint8 * SIZE).from_buffer_copy(ram)          # A20 off: the second MiB aliases the first
    uc.mem_map_ptr(0, SIZE, 7, ctypes.addressof(backing)); uc.mem_map_ptr(SIZE, SIZE, 7, ctypes.addressof(backing))
    for reg, v in ((X.UC_X86_REG_CS, BASE + RES), (X.UC_X86_REG_IP, entry), (X.UC_X86_REG_DS, DS), (X.UC_X86_REG_ES, DS),
                   (X.UC_X86_REG_SS, SS), (X.UC_X86_REG_SP, sp), (X.UC_X86_REG_BP, 0x7777), (X.UC_X86_REG_EFLAGS, 0x0002), (X.UC_X86_REG_AX, ax)):
        uc.reg_write(reg, v)
    log, state = [], dict(done=False)

    def word(off):
        s = uc.reg_read(X.UC_X86_REG_SP)
        return struct.unpack('<H', uc.mem_read(phys(SS, (s + off) & 0xFFFF), 2))[0]

    def far_return(n):
        s = uc.reg_read(X.UC_X86_REG_SP)
        ip, cs = word(0), word(2)
        uc.reg_write(X.UC_X86_REG_SP, (s + 4 + n) & 0xFFFF); uc.reg_write(X.UC_X86_REG_CS, cs); uc.reg_write(X.UC_X86_REG_IP, ip)

    def hook(emu, address, size, user):
        if address == phys(*RET):
            state['done'] = True; return emu.emu_stop()
        if address == phys(*GETMEM):
            log.append(('A', word(4))); uc.reg_write(X.UC_X86_REG_AX, block[0]); uc.reg_write(X.UC_X86_REG_DX, block[1]); return far_return(2)
        if address == phys(*FREEMEM):
            log.append(('R', word(8), word(6), word(4))); return far_return(6)
        if address == phys(*MESSAGE):
            log.append(('P', word(8) & 0xFF)); return far_return(6)
        if address == phys(*STRLOAD):
            log.append(('L', (word(6) - BASE) & 0xFFFF, word(4)))
        if address == phys(*MOVE):
            log.append(('M', word(12), word(10), word(4)))
    uc.hook_add(UC_HOOK_CODE, hook)
    uc.emu_start(phys(BASE + RES, entry), 2 * SIZE, count=500000)
    check(state['done'], f'00BA:{entry:04X} did not return')
    return uc, bytearray(backing), log


def window(rid):
    """The excluded stack window for one body: [return address - 200h, return address + 4 + RETF bytes). Arguments are
    pushed downward from SP, so the return address sits at SP - 4 - RETF bytes and the window ends exactly at SP: it can
    never reach the caller's frame above the arguments (which holds a non-zero sentinel and IS compared)."""
    retf = BODIES[rid][3]
    ret = SP - 4 - retf
    return ret - 0x200, ret + 4 + retf


def compare_ram(c_ram, o_ram, label, rid):
    lo_off, hi_off = window(rid)
    check(hi_off == SP, f'{rid}: window does not end at the arguments')
    lo, hi = phys(SS, lo_off), phys(SS, hi_off)
    if c_ram[:lo] != o_ram[:lo] or c_ram[hi:] != o_ram[hi:]:
        diff = [hex(i) for i in range(SIZE) if not lo <= i < hi and c_ram[i] != o_ram[i]]
        raise ValueError(f'{label}: RAM differs outside the stack window at {diff[:8]} ({len(diff)} bytes)')


def rand_record(rng, zero_bias=0.6):
    return bytes(0 if rng.random() < zero_bias else rng.randrange(256) for _ in range(285))


def differential(m, lib, quick=False):
    from unicorn import x86_const as X
    rng = random.Random(0x00BA)
    N = (lambda full: 10) if quick else (lambda full: full)
    counts = {}

    def put(ram, seg, off, data):
        for k, b in enumerate(data):
            ram[phys(seg, off + k)] = b

    def pointer_record(rng, ram, k):
        """A character record at CHAR_SEG:0100h+k*200h, random, whose far-pointer fields sometimes point at other records."""
        off = 0x0100 + k * 0x200
        rec = bytearray(rand_record(rng))
        for field in (0x6E, 0x104, 0xC8, 0x108, 0x10A):
            if rng.random() < 0.5:
                struct.pack_into('<HH', rec, field, 0x0100 + rng.randrange(6) * 0x200, CHAR_SEG)
        put(ram, CHAR_SEG, off, rec)
        return off

    # single-character readers and writers: 23B7, 2637, 2E03, 2E46, 2E8B
    for rid, entry, fn, result_reg in (('RES_00BA_23B7', 0x23B7, R.res_23b7, 'al'), ('RES_00BA_2637', 0x2637, R.res_2637, 'ax'),
                                       ('RES_00BA_2E03', 0x2E03, R.res_2e03, 'al'), ('RES_00BA_2E46', 0x2E46, R.res_2e46, 'al'),
                                       ('RES_00BA_2E8B', 0x2E8B, None, 'ax')):
        n = 0
        for case in range(N(400)):
            mm = bytearray(m)
            for k in range(6):
                pointer_record(rng, mm, k)
            for g in range(0x10):                                     # a few DS globals the link checks read
                mm[phys(DS, 0x5D90 + g)] = rng.randrange(256) if rng.random() < 0.5 else 0
            char = (0x0100 + rng.randrange(6) * 0x200, CHAR_SEG)
            ax = rng.randrange(0x10000)                                # the entry AX (AH survives in 2637's result)
            if rid == 'RES_00BA_2E8B':
                slot = (0x0010 + rng.randrange(8) * 4, SLOT_SEG)
                uc, ram, _ = run(mm, entry, (char[1], char[0], slot[1], slot[0]), ax=ax)
                cram = bytearray(mm)
                status, value = R.res_2e8b(lib, cram, DS, char, slot, ax)
            elif rid == 'RES_00BA_2637':
                uc, ram, _ = run(mm, entry, (char[1], char[0]), ax=ax)
                cram = bytearray(mm)
                status, value = R.res_2637(lib, cram, DS, char, ax)
            else:
                uc, ram, _ = run(mm, entry, (char[1], char[0]), ax=ax)
                cram = bytearray(mm)
                status, value = fn(lib, cram, DS, char)
            got = uc.reg_read(X.UC_X86_REG_AX) & (0xFF if result_reg == 'al' else 0xFFFF)
            check(status == 0 and got == value, f'{rid} case {case}: original {result_reg}={got:04x} C {value:04x} (status {status})')
            compare_ram(cram, ram, f'{rid} case {case}', rid)
            n += 1
        counts[rid] = n

    # 23DB party count: a list from DS:5D96/5D98 linked at +104h/+106h, bytes +10Dh/+10Eh
    n = 0
    for case in range(N(400)):
        mm = bytearray(m)
        k = rng.randrange(0, 7)
        offs = [0x0100 + i * 0x200 for i in range(k)]
        for i, off in enumerate(offs):
            rec = bytearray(rand_record(rng, 0.3))
            nxt = (offs[i + 1], CHAR_SEG) if i + 1 < k else (0, 0)
            struct.pack_into('<HH', rec, 0x104, *nxt)
            rec[0x10D] = rng.choice((0, 0, 1, rng.randrange(256)))
            rec[0x10E] = rng.choice((0, 1, 2, 0xFF, 0x80, rng.randrange(256)))
            put(mm, CHAR_SEG, off, rec)
        struct.pack_into('<HH', mm, phys(DS, 0x5D96), *((offs[0], CHAR_SEG) if k else (0, 0)))
        for b in range(0x6780, 0x6880):
            mm[phys(DS, b)] = rng.randrange(256)
        uc, ram, _ = run(mm, 0x23DB, ())
        cram = bytearray(mm)
        status = R.res_23db(lib, cram, DS)
        check(status == 0, f'23DB case {case}: status {status}')
        compare_ram(cram, ram, f'RES_00BA_23DB case {case}', 'RES_00BA_23DB')
        n += 1
    counts['RES_00BA_23DB'] = n

    def item_list(mm, k):
        """k 63-byte nodes at NODE_SEG linked at +2Ah/+2Ch, head in the character at +C8h/+CAh; returns the node pointers."""
        nodes = [(0x0040 + i * 0x80, NODE_SEG) for i in range(k)]
        for i, (off, seg) in enumerate(nodes):
            data = bytearray(rng.randrange(256) for _ in range(0x3F))
            struct.pack_into('<HH', data, 0x2A, *(nodes[i + 1] if i + 1 < k else (0, 0)))
            put(mm, seg, off, data)
        char = (0x0100, CHAR_SEG)
        put(mm, CHAR_SEG, 0x0100, rand_record(rng))
        struct.pack_into('<HH', mm, phys(CHAR_SEG, 0x0100 + 0xC8), *(nodes[0] if k else (0, 0)))
        return char, nodes

    # 15F7 append: GetMem returns a caller-chosen heap block; Move runs from the original bytes
    n = 0
    for case in range(N(300)):
        mm = bytearray(m)
        char, nodes = item_list(mm, rng.randrange(0, 6))
        source = (0x0200 + rng.randrange(8), ITEM_SEG)
        put(mm, *source[::-1], bytes(rng.randrange(256) for _ in range(0x3F)))
        block = (0x0010 + rng.randrange(0x40) * 0x40, HEAP_SEG)
        uc, ram, olog = run(mm, 0x15F7, (char[1], char[0], source[1], source[0]), block=block)
        cram = bytearray(mm)
        status, clog = R.res_15f7(lib, cram, DS, source, char, block)
        check(status == 0, f'15F7 case {case}: status {status}')
        check(olog == clog, f'15F7 case {case}: boundaries original {olog} C {clog}')
        compare_ram(cram, ram, f'RES_00BA_15F7 case {case}', 'RES_00BA_15F7')
        n += 1
    counts['RES_00BA_15F7'] = n

    # 1509 remove: target null (message 1), first / middle / last node, or absent (message 2)
    n = 0
    for case in range(N(300)):
        mm = bytearray(m)
        k = rng.randrange(0, 6)
        char, nodes = item_list(mm, k)
        kind = rng.choice(('null', 'absent') + (('first', 'middle', 'last') if k else ()))
        target = {'null': (0, 0), 'absent': (0x0F00, NODE_SEG), 'first': nodes[0] if k else None,
                  'middle': nodes[k // 2] if k else None, 'last': nodes[-1] if k else None}[kind]
        uc, ram, olog = run(mm, 0x1509, (char[1], char[0], target[1], target[0]))
        cram = bytearray(mm)
        status, clog = R.res_1509(lib, cram, DS, target, char)
        check(olog == clog, f'1509 case {case} ({kind}): boundaries original {olog} C {clog}')
        compare_ram(cram, ram, f'RES_00BA_1509 case {case} ({kind})', 'RES_00BA_1509')
        n += 1
    counts['RES_00BA_1509'] = n
    return counts


def main():
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument('--compile-only', action='store_true')
    p.add_argument('--quick', action='store_true', help='ten cases per body (a smoke run)')
    p.add_argument('--output', type=Path)
    a = p.parse_args()
    with tempfile.TemporaryDirectory() as tmp:
        lib = R.build(tmp)
        if a.compile_only:
            print('CHECKER_BUILD_OK'); return
        m, report = media()
        report['cases'] = differential(m, lib, a.quick)
        report.update(compared='result register, every RAM byte outside the per-body stack window, boundaries in order', masks=0,
                      windows={rid: f'SS:[{window(rid)[0]:04X},{window(rid)[1]:04X})' for rid in BODIES})
    text = json.dumps(report, indent=1, sort_keys=True)
    if a.output:
        a.output.parent.mkdir(parents=True, exist_ok=True); a.output.write_text(text + '\n')
    print(json.dumps(report['cases'], sort_keys=True))


if __name__ == '__main__':
    main()
