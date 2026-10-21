#!/usr/bin/env python3
"""Static audit of the proposed declared boundary RES_0AF8_00DF (Turbo Pascal divide-error handler, TERMINAL), from the hash-checked
loader-expanded START.EXE image. No model is claimed: this asserts the facts the declaration text states, including the run-time PREMISE
(INT 0's vector is SYSTEM's handler), so the text cannot drift from the bytes. It re-runs the whole RES_0AF8_00E2 audit first, because the handler
falls into that entry. Media stays private. `--compile-only` imports the tools and prints CHECKER_BUILD_OK with no media."""
import argparse
from hashlib import sha256
import json
from pathlib import Path
import re
import struct

from capstone import CS_ARCH_X86, CS_MODE_16, Cs
from resident_loader_map import unpack, relocations
from u86_procvars_3413_audit import ROOT, EXE_SHA, OVR_SHA, physical
import declared_boundary_0af8_00e2_audit as runerror

CHECKER_ROWS = ('RES_0AF8_00DF',)
SEG = 0x0AF8
EXPANDED_SHA = 'a42a57e68d3f7815831821cc9365bb917befda45a58e958544451fc7a878df8e'
ENTRY_BYTES = 'b8c800'                                           # MOV AX,00C8h (200)
ENTRY_SHA = 'f5643d94e9e1bcc2aa33ced5f7eb419bf3551c9bea99e5fbcc02afb353edf860'
INSTALL_SHA = '431bbe40c73a086809752f5d4ffe551c1020383c911c6cfa2aed3a9de6ca80aa'                                      # [005C,0067): PUSH DS; PUSH CS; POP DS; MOV DX,00DFh; MOV AX,2500h; INT 21h
# Every AH=25h issuer in the START image by immediate: image offset -> AL. Only SYSTEM's own init installs INT 0.
AH25_SITES = {0x9E7B: 0x08, 0x9EA3: 0x08, 0xA490: 0x1B, 0xAAF8: 0x3F, 0xAFE2: 0x00, 0xAFEA: 0x23, 0xAFF2: 0x24, 0xAFFA: 0x3F}   # image offset of MOV AX,25xxh -> AL
RESTORE_LOOP_AH25 = 0xB0DF                                       # MOV AH,25h at 0AF8:015F: the Halt vector-restore loop (AL from the CS table)
MD = Cs(CS_ARCH_X86, CS_MODE_16)


def check(condition, message):
    if not condition:
        raise ValueError(message)


def audit():
    runerror_report = runerror.audit()                           # the continuation is the 00E2 path: re-assert all of it
    exe = (ROOT / 'resources/DOS/POOLRAD/START.EXE').read_bytes()
    check(len(exe) == 65231 and sha256(exe).hexdigest() == EXE_SHA, 'START.EXE provenance')
    image = bytes(unpack(exe)[0])
    check(sha256(image).hexdigest() == EXPANDED_SHA, 'expanded image provenance')
    base = physical(SEG, 0)
    seg = image[base:base + 0x10000]
    check(seg[0xDF:0xE2].hex() == ENTRY_BYTES and sha256(seg[0xDF:0xE2]).hexdigest() == ENTRY_SHA, 'entry bytes')
    ins = {i.address: i for i in MD.disasm(seg[0xDF:0xED], 0xDF)}
    check([(i.address, i.mnemonic, i.op_str) for i in ins.values()][:5] ==
          [(0xDF, 'mov', 'ax, 0xc8'), (0xE2, 'pop', 'cx'), (0xE3, 'pop', 'bx'), (0xE4, 'jmp', '0xed'), (0xE6, 'mov', 'ax, 0xff')],
          'the 00DF entry falls straight into 00E2 (POP CX; POP BX; JMP 00ED)')
    # INT 0's vector is installed at init: SYSTEM saves the 19 vectors first (the table begins with vector 00), then points INT 0 at CS:00DF.
    check(seg[0x1EE] == 0x00, 'the first saved/restored vector is INT 00')
    init = {i.address: i for i in MD.disasm(seg[0x41:0x67], 0x41)}
    check([(init[a].mnemonic, init[a].op_str) for a in (0x44, 0x4E, 0x50)] == [('mov', 'si, 0x1ee'), ('mov', 'ah, 0x35'), ('int', '0x21')], 'the 19 vectors are saved with AH=35h from the 01EE table')
    check(sha256(seg[0x5C:0x67]).hexdigest() == INSTALL_SHA and
          [(init[a].mnemonic, init[a].op_str) for a in (0x5C, 0x5D, 0x5E, 0x5F, 0x62, 0x65)] ==
          [('push', 'ds'), ('push', 'cs'), ('pop', 'ds'), ('mov', 'dx, 0xdf'), ('mov', 'ax, 0x2500'), ('int', '0x21')], 'INT 0 install: DS:DX = CS:00DF, AX = 2500h')
    # Who else issues AH=25h, by immediate: the premise's static support.
    sites = {}
    for m in re.finditer(rb'\xb8.\x25', image):
        sites[m.start()] = image[m.start() + 1]
    ah_only = [m.start() for m in re.finditer(rb'\xb4\x25', image)]
    check(sites == AH25_SITES, f'AH=25h by immediate: {[(hex(a), hex(v)) for a, v in sorted(sites.items())]}')
    check(ah_only == [RESTORE_LOOP_AH25], f'MOV AH,25h sites: {[hex(a) for a in ah_only]}')
    check(sum(1 for a, v in sites.items() if v == 0x00) == 1, 'exactly one immediate AX=2500h in the START image')
    ovr = (ROOT / 'resources/DOS/POOLRAD/GAME.OVR').read_bytes()
    check(sha256(ovr).hexdigest() == OVR_SHA, 'GAME.OVR provenance')
    check(not re.search(rb'\xb8.\x25', ovr) and not re.search(rb'\xb4\x25', ovr), 'GAME.OVR has no AH=25h by immediate')
    # The handler is reached only through the vector: nothing transfers to 00DF directly, and its offset is loaded only at the install.
    near = [o for o in range(len(seg) - 2) if seg[o] in (0xE8, 0xE9) and (o + 3 + struct.unpack_from('<h', seg, o + 1)[0]) & 0xFFFF == 0xDF]
    short = [o for o in range(len(seg) - 1) if seg[o] == 0xEB and (o + 2 + (seg[o + 1] - 256 if seg[o + 1] > 127 else seg[o + 1])) & 0xFFFF == 0xDF]
    loads = [o for o in range(len(seg) - 2) if seg[o] == 0xBA and seg[o + 1:o + 3] == b'\xdf\x00']
    check(not near and not short and loads == [0x5F], f'direct transfers {near} {short}; MOV DX,00DF at {[hex(a) for a in loads]}')
    rels, _ = relocations(exe, image)
    pat = struct.pack('<HH', 0xDF, SEG)
    far = [i - 1 for i in range(1, len(image) - 3) if image[i:i + 4] == pat and image[i - 1] in (0x9A, 0xEA)]
    check(not far, f'a far transfer to 0AF8:00DF appeared: {far}')
    return dict(entry_sha256=ENTRY_SHA, install_sha256=INSTALL_SHA, ah25_by_immediate={f'image+{a:05X}': f'AL={v:02X}' for a, v in sorted(sites.items())},
                int0_installers=1, direct_transfers=0, continuation=dict(reachable_instructions=runerror_report['reachable_instructions'], returns=runerror_report['returns'],
                interrupts=runerror_report['interrupts']), identity='not claimed', credit_claimed=False)


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--compile-only', action='store_true')
    p.add_argument('--output', type=Path)
    a = p.parse_args()
    if a.compile_only:
        print('CHECKER_BUILD_OK')
        return
    report = audit()
    print('declared boundary 00DF', json.dumps(report, sort_keys=True), flush=True)
    if a.output:
        a.output.parent.mkdir(parents=True, exist_ok=True)
        a.output.write_text(json.dumps(report, indent=2, sort_keys=True) + '\n')


if __name__ == '__main__':
    main()
