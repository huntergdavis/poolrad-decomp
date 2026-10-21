#!/usr/bin/env python3
"""Static audit of the proposed declared boundary RES_0AF8_00E2 (Turbo Pascal RunError, TERMINAL), from the hash-checked
loader-expanded START.EXE image. No model is claimed: this asserts the facts the declaration text states, so the declaration
cannot drift from the bytes. Media stays private. `--compile-only` imports the tools and prints CHECKER_BUILD_OK with no media."""
import argparse
from hashlib import sha256
import json
from pathlib import Path
import struct

from capstone import CS_ARCH_X86, CS_MODE_16, Cs
from resident_loader_map import unpack, relocations
from u86_procvars_3413_audit import ROOT, EXE_SHA, OVR_SHA, physical

CHECKER_ROWS = ('RES_0AF8_00E2',)
SEG = 0x0AF8
EXPANDED_SHA = 'a42a57e68d3f7815831821cc9365bb917befda45a58e958544451fc7a878df8e'
ENTRY_BYTES = '595beb07'                                         # POP CX; POP BX; JMP 00ED
ENTRY_SHA = 'e2ff41187088c0654f5e48c1714af6a71859757a7185c54e7394181a109057d1'
TAIL_SHA = '8a92e5adb370154bde79926e5d3f20f189f4dcdd015d4d783d0f683583e01c0d'      # [00ED,01EE): shared with Halt, plus the print helpers 01A5..01ED
DATA_SHA = '961ee5d0f2c12a0c94237230a4824ec721f3a780fb88e1e927e42b284f334cc9'      # [01EE,0222): vector table and message strings
NEAR_JUMPS = [0x0251, 0x026B, 0x04B4, 0x04DC, 0x04F4, 0x0AE1, 0x11B1, 0x11B7, 0x11BD]   # E9 jumps into 00E2 (04B4 is IOCheck)
SHORT_JUMPS = [0x00DD]                                           # PUSH ES; JMP SHORT 00E2 at 00DC
FAR_CALLERS = {0xAA26: 0xAA20}                                   # resident far call (RunError(209)) -> its function
INTERRUPTS = {0x0164: 0x25, 0x01A3: 0x4C, 0x01EB: 0x06}          # INT 21h site -> AH
MD = Cs(CS_ARCH_X86, CS_MODE_16)


def check(condition, message):
    if not condition:
        raise ValueError(message)


def audit():
    exe = (ROOT / 'resources/DOS/POOLRAD/START.EXE').read_bytes()
    check(len(exe) == 65231 and sha256(exe).hexdigest() == EXE_SHA, 'START.EXE provenance')
    image = bytes(unpack(exe)[0])
    check(sha256(image).hexdigest() == EXPANDED_SHA, 'expanded image provenance')
    base = physical(SEG, 0)
    seg = image[base:base + 0x10000]
    check(seg[0xE2:0xE6].hex() == ENTRY_BYTES and sha256(seg[0xE2:0xE6]).hexdigest() == ENTRY_SHA, 'entry bytes')
    check(sha256(seg[0xED:0x1EE]).hexdigest() == TAIL_SHA and sha256(seg[0x1EE:0x222]).hexdigest() == DATA_SHA, 'tail or data hash')
    ins = {i.address: i for i in MD.disasm(seg[0xDC:0x1EE], 0xDC)}
    show = lambda a: (ins[a].mnemonic, ins[a].op_str)
    check([show(a) for a in (0xDC, 0xDD, 0xDF, 0xE2, 0xE3, 0xE4)] ==
          [('push', 'es'), ('jmp', '0xe2'), ('mov', 'ax, 0xc8'), ('pop', 'cx'), ('pop', 'bx'), ('jmp', '0xed')], 'the 00DC/00DF/00E2 entry family')
    # ErrorAddr := CX:BX and the print guard: from 00E2 (CX,BX = the caller's address) the message prints; from 00E9 (CX = BX = 0) it does not.
    check([show(a) for a in (0xE9, 0xEB, 0x11B, 0x11F, 0x16C, 0x16F, 0x173)] ==
          [('xor', 'cx, cx'), ('xor', 'bx, bx'), ('mov', 'word ptr [0x441e], cx'), ('mov', 'word ptr [0x4420], bx'),
           ('mov', 'ax, word ptr [0x441e]'), ('or', 'ax, word ptr [0x4420]'), ('je', '0x19e')], 'ErrorAddr store or print guard')
    check([show(a) for a in (0xED, 0xF0, 0xF3)] == [('mov', 'dx, 0xc7c'), ('mov', 'ds, dx'), ('mov', 'word ptr [0x441c], ax')], 'ExitCode := AX')
    # Every INT in [00E2,01EE) and its AH.
    ints = {a: i for a, i in ins.items() if a >= 0xE2 and i.mnemonic == 'int'}
    check({a: i.op_str for a, i in ints.items()} == {a: '0x21' for a in INTERRUPTS}, f'interrupts {sorted(ints)}')
    for a, ah in INTERRUPTS.items():
        previous = [i for i in ins.values() if i.address < a and i.mnemonic == 'mov' and i.op_str.startswith('ah, ')][-1]
        check(int(previous.op_str.split(', ')[1], 16) == ah, f'AH at {a:04x} is {previous.op_str}')
    check(seg[0x1EE:0x201].hex() == '00021b2123243435363738393a3b3c3d3e3f75', 'the 19-vector table')
    check(seg[0x201:0x219] == b'Runtime error \x00 at \x00.\r\n\x00' and seg[0x201:0x210].count(b'\x00') == 1, 'message strings at 0201, 0210, 0215')
    # TERMINAL: every path from 00E2 ends in the terminate INT 21h AH=4Ch, except the ExitProc dispatch RETF, which returns to 0123.
    seen, work, rets = set(), [0xE2], []
    while work:
        a = work.pop()
        while a not in seen:
            check(a in ins, f'undecoded address {a:04x} reachable from 00E2')
            seen.add(a)
            i = ins[a]
            if i.mnemonic in ('ret', 'retf', 'iret'):
                rets.append((a, i.mnemonic))
                break
            if i.mnemonic == 'int' and INTERRUPTS.get(a) == 0x4C:   # DOS terminate: control never continues past it
                break
            if i.mnemonic == 'jmp':
                check(i.op_str.startswith('0x'), 'indirect jump')
                a = int(i.op_str, 16)
                continue
            if i.mnemonic.startswith('j') or i.mnemonic.startswith('loop'):
                check(i.op_str.startswith('0x'), 'indirect branch')
                work.append(int(i.op_str, 16))
            if i.mnemonic == 'call':
                check(i.op_str.startswith('0x'), 'indirect call')
            a += i.size
    check(rets == [(0x013F, 'retf')], f'returns reachable from RunError: {rets}')
    check([show(a) for a in (0x138, 0x13B, 0x13C, 0x13D, 0x13E)] ==
          [('mov', 'ax, 0x123'), ('push', 'cs'), ('push', 'ax'), ('push', 'es'), ('push', 'bx')], 'the one RETF is the ExitProc dispatch (it returns to 0123)')
    check({0x1A5, 0x1B3, 0x1CD, 0x1E7} <= {int(i.op_str, 16) for i in ins.values() if i.mnemonic == 'call' and i.op_str.startswith('0x')}, 'print helpers are called')
    # Who transfers here.
    near = [o for o in range(len(seg) - 2) if seg[o] == 0xE9 and (o + 3 + struct.unpack_from('<h', seg, o + 1)[0]) & 0xFFFF == 0xE2]
    short = [o for o in range(len(seg) - 1) if seg[o] == 0xEB and (o + 2 + (seg[o + 1] - 256 if seg[o + 1] > 127 else seg[o + 1])) & 0xFFFF == 0xE2]
    check(near == NEAR_JUMPS and short == SHORT_JUMPS, f'near transfers: {[hex(a) for a in near]} {[hex(a) for a in short]}')
    check(not any(seg[o] == 0xE8 and (o + 3 + struct.unpack_from('<h', seg, o + 1)[0]) & 0xFFFF == 0xE2 for o in range(len(seg) - 2)), 'a near CALL to 00E2 appeared')
    from overlay_inventory import find_units, parse_mz
    ovr = (ROOT / 'resources/DOS/POOLRAD/GAME.OVR').read_bytes()
    check(sha256(ovr).hexdigest() == OVR_SHA, 'GAME.OVR provenance')
    direct = [f"{u['stub_para']:04X}:{f - 3:04X}" for u in find_units(exe[parse_mz(exe)['header_bytes']:], ovr) for f in u['fixups']
              if ovr[u['ovr_offset'] + f - 3:u['ovr_offset'] + f + 2][:1] == b'\x9a' and struct.unpack_from('<HH', ovr, u['ovr_offset'] + f - 2) == (0xE2, SEG)]
    check(not direct, f'direct overlay far calls appeared: {direct}')
    rels, _ = relocations(exe, image)
    pattern = struct.pack('<HH', 0xE2, SEG)
    far = [i - 1 for i in range(1, len(image) - 3) if image[i:i + 4] == pattern and image[i - 1] == 0x9A and (i + 2) in rels]
    targets = {i for i in range(len(image) - 2) if image[i:i + 3] == b'\x55\x89\xe5'}
    decoded = {}
    for site in far:
        for start in sorted((t for t in targets if t <= site and site - t < 0x300), reverse=True):
            run = list(MD.disasm(image[start:site + 5], start))
            if run and run[-1].address == site and run[-1].mnemonic == 'lcall' and sum(x.size for x in run) == site + 5 - start:
                decoded[site] = start
                break
    check(decoded == FAR_CALLERS and far == sorted(FAR_CALLERS), f'resident far callers {[hex(a) for a in far]} {decoded}')
    return dict(entry_sha256=ENTRY_SHA, tail_sha256=TAIL_SHA, data_sha256=DATA_SHA, reachable_instructions=len(seen), returns=[f'{a:04X} {m}' for a, m in rets],
                interrupts={f'{a:04X}': f'AH={ah:02X}' for a, ah in INTERRUPTS.items()}, near_jumps=[f'{a:04X}' for a in near], short_jumps=[f'{a:04X}' for a in short],
                overlay_far_calls=0, resident_far_calls={f'image+{a:05X}': f'function image+{f:05X}' for a, f in decoded.items()}, identity='not claimed', credit_claimed=False)


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--compile-only', action='store_true')
    p.add_argument('--output', type=Path)
    a = p.parse_args()
    if a.compile_only:
        print('CHECKER_BUILD_OK')
        return
    report = audit()
    print('declared boundary 00E2', json.dumps(report, sort_keys=True), flush=True)
    if a.output:
        a.output.parent.mkdir(parents=True, exist_ok=True)
        a.output.write_text(json.dumps(report, indent=2, sort_keys=True) + '\n')


if __name__ == '__main__':
    main()
