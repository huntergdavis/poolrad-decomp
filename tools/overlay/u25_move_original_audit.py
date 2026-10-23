#!/usr/bin/env python3
"""Original-byte checker for OVL_0025_19DB (unit 25h entry 19, pr_u25_entry19), which cited only shared tooling.

The original 19DB runs under Unicorn with its two far calls going through overlay stub 03D5 (unit 3BDh) in its LOADED state:
the stub entries for entry 4 (+057A, cell) and entry 2 (+047B, nibble) hold JMP FAR to unit 3BDh's segment, as the overlay
manager writes them. Units 25h and 3BDh are loaded at their own segments with every fixup applied, and the relocated
START.EXE supplies the stub segment and the data segment. The C model (pr_u25_entry19, composing the translated 3BD
entries) runs through u25_move_original_shim.c. Compared per call: the five state bytes [6AAD] x, [6AAE] y, [6AAF] facing,
[6AB1] cell, [6AB0] nibble, AND the whole 64 KiB data segment, which must change in exactly those bytes; plus the
model's status (OK, or UNSPECIFIED exactly when the facing is outside {0,2,4,6}).

The unmatched-facing path of entry 2 returns the unwritten stack byte at its [BP-1]; the model declares that byte as a
host input (nibble_frame_residue). The checker reads it from the original at the instruction that loads it (3BD +0571)
and passes the same value to the model. Nothing else is supplied or masked.

python3 tools/overlay/u25_move_original_audit.py [--compile-only] [--quick] [--output report.json]"""
import argparse, ctypes, json, random, struct, subprocess, sys, tempfile
from hashlib import sha256
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parent))
from overlay_inventory import find_units, parse_mz

CHECKER_ROWS = ('OVL_0025_19DB',)
ROOT = Path(__file__).resolve().parents[2]
SIZE = 1 << 20
BASE, DATA, STUB = 0x1000, 0x0C7C, 0x03D5
DS = BASE + DATA
SEG = {0x25: 0x6000, 0x3BD: 0x7000}
SS, SP, RET = 0x8000, 0xF000, (0x9F00, 0x0000)
MAP = (0x9400, 0x0000)
ENTRY, END, INSNS = 0x19DB, 0x1A65, 50
CELL, NIBBLE, RESIDUE_AT = 0x057A, 0x047B, 0x0571
STATE = (0x6AAD, 0x6AAE, 0x6AAF, 0x6AB1, 0x6AB0)


def check(cond, msg):
    if not cond:
        raise ValueError(msg)


def phys(seg, off):
    return ((seg << 4) + (off & 0xFFFF)) % SIZE


def library(directory):
    so = Path(directory) / 'u25move.so'
    subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Wpedantic', '-Werror', '-O2', '-fPIC', '-shared', '-I', str(ROOT / 'include'),
                    str(ROOT / 'tools/overlay/u25_move_original_shim.c'), str(ROOT / 'src/recovered/overlay_u25_move.c'),
                    str(ROOT / 'src/recovered/overlay_u3bd_cell.c'), str(ROOT / 'src/recovered/overlay_u3bd_nibble.c'), '-o', str(so)], check=True)
    lib = ctypes.CDLL(str(so))
    lib.shim_u25_entry19.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_uint8, ctypes.POINTER(ctypes.c_uint8), ctypes.c_uint8]
    lib.shim_u25_entry19.restype = ctypes.c_int
    return lib


def media():
    import res0899_quit_0000_cpu_audit as Q
    from capstone import Cs, CS_ARCH_X86, CS_MODE_16
    image, sites = Q.load_image()
    exe = (ROOT / 'resources/DOS/POOLRAD/START.EXE').read_bytes(); ovr = (ROOT / 'resources/DOS/POOLRAD/GAME.OVR').read_bytes()
    units = {u['stub_para']: u for u in find_units(exe[parse_mz(exe)['header_bytes']:], ovr)}
    m = bytearray(SIZE)
    reloc = Q.relocate(image, sites, BASE)
    m[BASE * 16:BASE * 16 + len(reloc)] = reloc
    for unit, seg in SEG.items():
        u = units[unit]
        code = bytearray(ovr[u['ovr_offset']:u['ovr_offset'] + u['code_size']])
        for f in u['fixups']:
            struct.pack_into('<H', code, f, (struct.unpack_from('<H', code, f)[0] + BASE) & 0xFFFF)
        m[seg * 16:seg * 16 + len(code)] = code
    check(units[0x3BD]['memory_segment'] == STUB and units[0x3BD]['entries'][2] == NIBBLE and units[0x3BD]['entries'][4] == CELL,
          'unit 3BDh stub/entries differ')
    for slot, target in ((2, NIBBLE), (4, CELL)):                    # loaded stub entries: JMP FAR unit:entry
        at = phys(BASE + STUB, 0x20 + 5 * slot)
        check(m[at:at + 2] == b'\xcd\x3f', 'stub entry is not INT 3Fh in the image')
        m[at:at + 5] = bytes([0xEA]) + struct.pack('<HH', target, SEG[0x3BD])
    body = bytes(m[phys(SEG[0x25], ENTRY):phys(SEG[0x25], END)])
    ins = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(body, ENTRY))
    check(len(ins) == INSNS and ins[-1].mnemonic == 'retf' and ins[-1].address + ins[-1].size == END, '19DB extent differs')
    calls = [(i.address, i.op_str) for i in ins if i.mnemonic == 'lcall']
    check(calls == [(0x1A47, f'{hex(BASE + STUB)}, 0x34'), (0x1A5B, f'{hex(BASE + STUB)}, 0x2a')], f'19DB calls {calls}')
    return m, dict(image_sha256=Q.IMAGE_DIGEST, body=dict(span=f'unit 25h +[{ENTRY:04X},{END:04X})', bytes=END - ENTRY, instructions=INSNS,
                                                          sha256=sha256(body).hexdigest()), stub=f'{STUB:04X} entries 2 and 4 (loaded)')


def run(m):
    from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE
    from unicorn import x86_const as X
    ram = bytearray(m)
    struct.pack_into('<HH', ram, phys(SS, SP - 4), RET[1], RET[0])
    backing = (ctypes.c_uint8 * SIZE).from_buffer_copy(ram)
    uc = Uc(UC_ARCH_X86, UC_MODE_16)
    uc.mem_map_ptr(0, SIZE, 7, ctypes.addressof(backing)); uc.mem_map_ptr(SIZE, SIZE, 7, ctypes.addressof(backing))
    for reg, v in ((X.UC_X86_REG_CS, SEG[0x25]), (X.UC_X86_REG_IP, ENTRY), (X.UC_X86_REG_DS, DS), (X.UC_X86_REG_ES, DS),
                   (X.UC_X86_REG_SS, SS), (X.UC_X86_REG_SP, SP - 4), (X.UC_X86_REG_BP, 0x7777), (X.UC_X86_REG_EFLAGS, 0x0002)):
        uc.reg_write(reg, v)
    state = dict(done=False, residue=None)

    def hook(emu, address, size, user):
        if address == phys(*RET):
            state['done'] = True; return emu.emu_stop()
        if address == phys(SEG[0x3BD], RESIDUE_AT):
            bp = uc.reg_read(X.UC_X86_REG_BP)
            state['residue'] = bytes(uc.mem_read(phys(SS, (bp - 1) & 0xFFFF), 1))[0]
    uc.hook_add(UC_HOOK_CODE, hook)
    uc.emu_start(phys(SEG[0x25], ENTRY), 2 * SIZE, count=200000)
    check(state['done'], '19DB did not return')
    return bytearray(backing), state['residue']


def differential(m, lib, quick=False):
    rng = random.Random(0x19DB)
    n, statuses = 0, {}
    values = (0, 1, 7, 14, 15, 16, 0x7F, 0x80, 0xFF)
    for case in range(60 if quick else 3000):
        mm = bytearray(m)
        x = rng.choice(values) if rng.random() < 0.5 else rng.randrange(256)
        y = rng.choice(values) if rng.random() < 0.5 else rng.randrange(256)
        facing = rng.choice((0, 2, 4, 6, 0, 2, 4, 6, 1, 3, 5, 7, 8, 0xFF))
        cell, nibble, map_id = rng.randrange(256), rng.randrange(256), rng.choice((0, 1, 2, rng.randrange(256)))
        for addr, v in zip(STATE, (x, y, facing, cell, nibble)):
            mm[phys(DS, addr)] = v
        mm[phys(DS, 0x84DC)] = map_id
        struct.pack_into('<HH', mm, phys(DS, 0x6A5C), MAP[1], MAP[0])
        buf = bytes(rng.randrange(256) for _ in range(1024))
        mm[phys(*MAP):phys(*MAP) + 1024] = buf
        for k in range(0x40):                                          # the caller's stack below SP: random residue
            mm[phys(SS, SP - 0x40 + k)] = rng.randrange(256)
        ram, residue = run(mm)
        io = (ctypes.c_uint8 * 5)(x, y, facing, cell, nibble)
        status = lib.shim_u25_entry19(io, map_id, (ctypes.c_uint8 * 1024)(*buf), residue if residue is not None else 0xA5)
        unmatched = facing not in (0, 2, 4, 6)
        got = [ram[phys(DS, a)] for a in STATE]
        check(got == list(io), f'case {case} (x={x:02x} y={y:02x} f={facing:02x} map={map_id}): original {got} C {list(io)}')
        # matched facing: OK; unmatched: UNSPECIFIED when entry 2 reached its selector dispatch, OK when the map-id/range gate
        # returned first. Either way the state bytes above must already equal the original's.
        check(status == 0 if not unmatched else status in (0, 2), f'case {case}: status {status} (facing {facing:02x})')
        statuses[status] = statuses.get(status, 0) + 1
        want = bytearray(mm[phys(DS, 0):phys(DS, 0) + 0x10000])
        for a, v in zip(STATE, io):
            want[a] = v
        check(bytes(ram[phys(DS, 0):phys(DS, 0) + 0x10000]) == bytes(want), f'case {case}: data segment differs elsewhere')
        n += 1
    return dict(cases=n, statuses=statuses)


def main():
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument('--compile-only', action='store_true'); p.add_argument('--quick', action='store_true'); p.add_argument('--output', type=Path)
    a = p.parse_args()
    with tempfile.TemporaryDirectory() as tmp:
        lib = library(tmp)
        if a.compile_only:
            print('CHECKER_BUILD_OK'); return
        m, report = media()
        report['unicorn'] = differential(m, lib, a.quick)
        report.update(compared='five state bytes, the whole 64 KiB data segment, the model status', masks=0)
    if a.output:
        a.output.parent.mkdir(parents=True, exist_ok=True); a.output.write_text(json.dumps(report, indent=1, sort_keys=True) + '\n')
    print(json.dumps(report['unicorn']))


if __name__ == '__main__':
    main()
