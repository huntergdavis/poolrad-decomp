#!/usr/bin/env python3
"""Forward flag liveness over ORIGINAL bytes for folded-flags sites (task t44, docs/re/FLAG_FOLDED_SITES.tsv).

For each folded instruction, every flag it writes is followed along every successor path of the routine until it is
  - overwritten by a later writer (pinned DOSBox-X rules, docs/re/FLAG_SEMANTICS_CENSUS.md: logical ops write all six with
    AF = 0, shifts with a non-zero count write all six with AF = 1, MUL writes CF/OF/ZF/PF and keeps SF/AF, DIV/IDIV write
    all six, INC/DEC keep CF), or
  - read by an observer (Jcc/SETcc/LOOPcc, ADC/SBB/RCL/RCR/CMC, PUSHF/LAHF, INT/INTO, DAA/DAS/AAA/AAS), or
  - exposed at a boundary: a CALL/LCALL (the callee's entry, and the model's stop before or inside that call) or a
    RET/RETF (the caller sees the returned FLAGS).
A Jcc read is reported separately: a model that takes that branch from the same condition consumes it without a FLAGS image.

python3 tools/overlay/flag_fold_liveness.py [--json]   (needs the owner START.EXE and GAME.OVR under resources/)"""
import json, sys
from pathlib import Path
from capstone import Cs, CS_ARCH_X86, CS_MODE_16
sys.path.insert(0, str(Path(__file__).resolve().parent))
from overlay_inventory import find_units, parse_mz
from resident_loader_map import unpack

ROOT = Path(__file__).resolve().parents[2]
ALL = frozenset('CPAZSO')
JCC = {'je': 'Z', 'jne': 'Z', 'jb': 'C', 'jae': 'C', 'jbe': 'CZ', 'ja': 'CZ', 'jl': 'SO', 'jge': 'SO', 'jle': 'ZSO', 'jg': 'ZSO',
       'js': 'S', 'jns': 'S', 'jp': 'P', 'jnp': 'P', 'jo': 'O', 'jno': 'O'}
READERS = {'adc': 'C', 'sbb': 'C', 'rcl': 'C', 'rcr': 'C', 'cmc': 'C', 'salc': 'C', 'pushf': ALL, 'lahf': 'SZAPC', 'int': ALL,
           'into': ALL, 'int3': ALL, 'daa': 'CA', 'das': 'CA', 'aaa': 'A', 'aas': 'A', 'loope': 'Z', 'loopne': 'Z'}
QUIET = {'mov', 'push', 'pop', 'les', 'lds', 'lea', 'xchg', 'cdq', 'cwde', 'cwd', 'cbw', 'nop', 'jmp', 'loop', 'jcxz',
         'cld', 'std', 'cli', 'sti', 'movsb', 'movsw', 'stosb', 'stosw', 'lodsb', 'lodsw', 'rep movsb', 'rep movsw',
         'rep stosb', 'rep stosw'}


def writes(i, previous):
    """The set of flags instruction i writes (pinned), or None when it writes none."""
    mn = i.mnemonic
    if mn in ('add', 'sub', 'cmp', 'adc', 'sbb', 'neg', 'and', 'or', 'xor', 'test', 'div', 'idiv'):
        return ALL
    if mn in ('inc', 'dec'):
        return frozenset('PAZSO')
    if mn == 'mul':
        return frozenset('COZP')
    if mn in ('shl', 'shr', 'sar', 'sal'):
        ops = i.op_str.split(', ')
        if len(ops) == 1 or ops[1] == '1' or (ops[1].startswith('0x') and int(ops[1], 16) & 0x1F) or ops[1].isdigit() and int(ops[1]) & 0x1F:
            return ALL
        if ops[1] == 'cl' and previous is not None and previous.mnemonic == 'mov' and previous.op_str.startswith('cl, ') \
                and int(previous.op_str.split(', ')[1], 0) & 0x1F:
            return ALL                                       # MOV CL,n (n & 1Fh non-zero) just before
        return None                                          # a count that may be zero writes nothing
    if mn in QUIET or mn in JCC or mn in READERS or mn in ('call', 'lcall', 'ret', 'retf'):
        return None
    raise NotImplementedError(f'{i.address:04X} {mn} {i.op_str}')


def walk(ins, start, live):
    """Follow `live` flags from the instruction after `start`. Returns (overwritten_at, observers, boundaries, jcc_reads)."""
    by = {i.address: i for i in ins}; order = [i.address for i in ins]
    prev = {order[k]: by[order[k - 1]] if k else None for k in range(len(order))}
    nxt = {i.address: i.address + i.size for i in ins}
    seen, work = set(), [(nxt[start], frozenset(live))]
    overwritten, observers, boundaries, jcc = set(), set(), set(), set()
    while work:
        a, fl = work.pop()
        if not fl or (a, fl) in seen:
            continue
        seen.add((a, fl))
        i = by.get(a)
        if i is None:
            boundaries.add(f'leaves span at {a:04X} with {"".join(sorted(fl))}'); continue
        mn = i.mnemonic
        if mn in JCC and set(JCC[mn]) & fl:
            jcc.add(f'{mn}@{a:04X} reads {"".join(sorted(set(JCC[mn]) & fl))}')
        if mn in READERS and set(READERS[mn]) & fl:
            observers.add(f'{mn}@{a:04X} reads {"".join(sorted(set(READERS[mn]) & fl))}')
        if mn in ('call', 'lcall', 'ret', 'retf'):
            boundaries.add(f'{mn}@{a:04X} with {"".join(sorted(fl))} live'); continue
        w = writes(i, prev[a])
        if w:
            hit = fl & w
            if hit:
                overwritten.add(f'{mn}@{a:04X}')
            fl = fl - w
        if mn == 'jmp':
            work.append((int(i.op_str, 16), fl))
        elif mn in JCC:
            work += [(int(i.op_str, 16), fl), (nxt[a], fl)]
        else:
            work.append((nxt[a], fl))
    return sorted(overwritten), sorted(observers), sorted(boundaries), sorted(jcc)


def sources():
    exe = (ROOT / 'resources/DOS/POOLRAD/START.EXE').read_bytes(); ovr = (ROOT / 'resources/DOS/POOLRAD/GAME.OVR').read_bytes()
    u = {u['stub_para']: u for u in find_units(exe[parse_mz(exe)['header_bytes']:], ovr)}[0x31]
    u31 = ovr[u['ovr_offset']:u['ovr_offset'] + u['code_size']]
    image = bytes(unpack(exe)[0])
    md = Cs(CS_ARCH_X86, CS_MODE_16)
    return {'u31 +0255': list(md.disasm(u31[0x0255:0x02EB], 0x0255)), 'u31 +0675': list(md.disasm(u31[0x0675:0x07D6], 0x0675)),
            '00BA:23B7': list(md.disasm(image[0xBA * 16 + 0x23B7:0xBA * 16 + 0x23DB], 0x23B7))}


# (body, TSV file:line, folded instruction addresses)
SITES = [
    ('00BA:23B7', 'overlay_u31_candidate_check_0255_cpu.c:196', [0x23BA]),
    ('u31 +0255', 'overlay_u31_candidate_check_0255_cpu.c:230', [0x0258]),
    ('u31 +0255', 'overlay_u31_candidate_check_0255_cpu.c:239', [0x026A, 0x0270]),
    ('u31 +0255', 'overlay_u31_candidate_check_0255_cpu.c:266', [0x02B3, 0x02B9, 0x02BB]),
    ('u31 +0255', 'overlay_u31_candidate_check_0255_cpu.c:269', [0x02C1, 0x02C5, 0x02C7]),
    ('u31 +0255', 'overlay_u31_candidate_check_0255_cpu.c:273', [0x02D2]),
    ('u31 +0675', 'overlay_u31_step_gate_0675.c:82', [0x0689, 0x0699, 0x06B1, 0x06B8, 0x06FB, 0x072A, 0x0736, 0x073D, 0x07B2]),
    ('u31 +0675', 'overlay_u31_step_gate_0675.c:185', [0x0678]),
    ('u31 +0675', 'overlay_u31_step_gate_0675.c:193', [0x069F, 0x06A1, 0x06A3, 0x06A5]),
    ('u31 +0675', 'overlay_u31_step_gate_0675.c:211', [0x06C0]),
    ('u31 +0675', 'overlay_u31_step_gate_0675.c:302', [0x075D, 0x0774, 0x078B, 0x07A2]),
    ('u31 +0675', 'overlay_u31_step_gate_0675.c:308', [0x07B4]),
    ('u31 +0675', '(header: SHR, MUL)', [0x071F, 0x073F]),
]


def main():
    bodies = sources(); rows = []
    for body, where, addrs in SITES:
        ins = bodies[body]; by = {i.address: i for i in ins}
        order = [i.address for i in ins]
        for a in addrs:
            i = by[a]; k = order.index(a)
            w = writes(i, by[order[k - 1]] if k else None)
            ow, obs, bnd, jcc = walk(ins, a, w)
            rows.append(dict(body=body, tsv=where, site=f'{a:04X}', instruction=f'{i.mnemonic} {i.op_str}', folded=''.join(sorted(w)),
                             overwritten_by=ow, observers=obs, boundaries=bnd, jcc_reads=jcc,
                             verdict='DEAD' if not obs and not bnd else 'EXPOSED'))
    if '--json' in sys.argv:
        print(json.dumps(rows, indent=1)); return
    for r in rows:
        print(f"{r['body']:10} {r['site']} {r['instruction']:24} {r['folded']:6} {r['verdict']:8} over={','.join(r['overwritten_by'])} "
              f"jcc={';'.join(r['jcc_reads'])} obs={';'.join(r['observers'])} bnd={';'.join(r['boundaries'])}  [{r['tsv']}]")


if __name__ == '__main__':
    main()
