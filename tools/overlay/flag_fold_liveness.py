#!/usr/bin/env python3
"""Forward flag liveness over ORIGINAL bytes, for folded-flags proofs (task t44, docs/re/FLAG_FOLDED_SITES.tsv).

A model "folds" an instruction's flags when it does not compute them. That is sound only if nothing observes them. For
each flag the site writes, liveness(...) follows every successor path, PER FLAG BIT, until that flag is
  - KILLED by a later writer of that bit (pinned DOSBox-X rules, docs/re/FLAG_SEMANTICS_CENSUS.md / flags.cpp):
      ADD/ADC/SUB/SBB/CMP/NEG, AND/OR/XOR/TEST (AF = 0), DIV/IDIV, a shift by a non-zero count (AF = 1): all six;
      INC/DEC: all but CF; MUL (MULW/MULB: FillFlagsNoCFOF first): CF/OF/ZF/PF only, SF and AF survive;
      ROL/ROR: CF/OF; RCL/RCR: CF/OF (they also READ CF); CLC/STC/CMC: CF (CMC reads it); SAHF: SZAPC; POPF: all;
    a shift or rotate by CL with an unknown count kills nothing (a zero count writes no flag);
  - OBSERVED: by a reader (Jcc/LOOPcc: their condition bits; ADC/SBB/RCL/RCR/CMC: CF; PUSHF/INT/INTO/INT3: all;
    LAHF: SZAPC; DAA/DAS: CF/AF; AAA/AAS: AF), at a CALL/LCALL (the callee's entry and the model's stop around it),
    at a RET/RETF/IRET (the caller sees the stored FLAGS), when control leaves the span, or at a caller-given HOST STOP:
    an address where the model can stop BEFORE executing that instruction (checked before the instruction's own effect).
Any instruction whose flag effect is not listed above raises NotImplementedError (fail closed).

Library: decode(code, base) -> instructions; liveness(instructions, site, stops=()) -> per-flag result.
CLI:  flag_fold_liveness.py (--unit 0x31 | --segment 0x00BA) --span 0255:02EB --site 0258 [--site ...] [--stop 117F ...] [--json]
      flag_fold_liveness.py --preset u31        (the t44 slice-2 table, docs/re/FLAG_FOLDED_U31_0255_0675.md)
Needs the owner START.EXE / GAME.OVR under resources/ for the CLI; the library takes any bytes."""
import argparse, json, sys
from pathlib import Path
from capstone import Cs, CS_ARCH_X86, CS_MODE_16

FLAGS = 'CPAZSO'
ALL = frozenset(FLAGS)
JCC = {'je': 'Z', 'jne': 'Z', 'jb': 'C', 'jae': 'C', 'jbe': 'CZ', 'ja': 'CZ', 'jl': 'SO', 'jge': 'SO', 'jle': 'ZSO', 'jg': 'ZSO',
       'js': 'S', 'jns': 'S', 'jp': 'P', 'jnp': 'P', 'jo': 'O', 'jno': 'O', 'loope': 'Z', 'loopne': 'Z'}
UNCONDITIONAL = {'jmp'}
READS = {'adc': 'C', 'sbb': 'C', 'rcl': 'C', 'rcr': 'C', 'cmc': 'C', 'salc': 'C', 'pushf': FLAGS, 'lahf': 'SZAPC',
         'int': FLAGS, 'into': FLAGS, 'int3': FLAGS, 'daa': 'CA', 'das': 'CA', 'aaa': 'A', 'aas': 'A'}
BOUNDARY = {'call', 'lcall', 'ret', 'retf', 'iret'}
QUIET = {'mov', 'movzx', 'movsx', 'push', 'pop', 'les', 'lds', 'lea', 'xchg', 'cdq', 'cwde', 'cwd', 'cbw', 'nop', 'not',
         'cld', 'std', 'cli', 'sti', 'loop', 'jcxz', 'xlatb', 'in', 'out', 'hlt',
         'movsb', 'movsw', 'stosb', 'stosw', 'lodsb', 'lodsw'}
FULL = {'add', 'adc', 'sub', 'sbb', 'cmp', 'neg', 'and', 'or', 'xor', 'test', 'div', 'idiv', 'popf'}
SHIFTS = {'shl', 'sal', 'shr', 'sar'}
ROTATES = {'rol', 'ror', 'rcl', 'rcr'}


def decode(code, base):
    return list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(bytes(code), base))


def _count(i, previous):
    """The shift/rotate count when it is known and non-zero, else None."""
    ops = i.op_str.split(', ')
    if len(ops) == 1:
        return 1
    c = ops[1]
    if c == 'cl':
        if previous is not None and previous.mnemonic == 'mov' and previous.op_str.startswith('cl, '):
            n = int(previous.op_str.split(', ')[1], 0) & 0x1F
            return n or None
        return None
    n = int(c, 0) & 0x1F
    return n or None


def writes(i, previous=None):
    """Flags instruction i writes under the pinned rules (frozenset, possibly empty)."""
    mn = i.mnemonic
    if mn.startswith('rep'):
        return frozenset()
    if mn in FULL:
        return ALL
    if mn in ('inc', 'dec'):
        return frozenset('PAZSO')
    if mn == 'mul':
        return frozenset('COZP')
    if mn in SHIFTS:
        return ALL if _count(i, previous) else frozenset()
    if mn in ROTATES:
        return frozenset('CO') if _count(i, previous) else frozenset()
    if mn in ('clc', 'stc', 'cmc'):
        return frozenset('C')
    if mn == 'sahf':
        return frozenset('SZAPC')
    if mn in QUIET or mn in JCC or mn in UNCONDITIONAL or mn in READS or mn in BOUNDARY:
        return frozenset()
    raise NotImplementedError(f'no pinned flag rule for {i.address:04X} {mn} {i.op_str}')


def reads(i):
    mn = i.mnemonic
    if mn in JCC:
        return frozenset(JCC[mn])
    return frozenset(READS.get(mn, ''))


def _walk(by, nxt, prev, start, flag, stops):
    """Follow one live flag from address `start` (that instruction not yet executed). Returns (killed, observed)."""
    killed, observed, seen, work = set(), set(), set(), [start]
    while work:
        a = work.pop()
        if a in seen:
            continue
        seen.add(a)
        i = by.get(a)
        if i is None:
            observed.add(f'leaves@{a:04X}'); continue
        if a in stops:
            observed.add(f'stop:@{a:04X}'); continue
        mn = i.mnemonic
        if flag in reads(i):
            observed.add(f'{"jcc" if mn in JCC else "read"}:{mn}@{a:04X}')
        if mn in BOUNDARY:
            observed.add(f'{"return" if mn.startswith("ret") or mn == "iret" else "call"}:@{a:04X}'); continue
        if flag in writes(i, prev[a]):
            killed.add(f'{mn}@{a:04X}'); continue
        if mn in UNCONDITIONAL:
            work.append(int(i.op_str, 16))
        elif mn in JCC:
            work += [int(i.op_str, 16), nxt[a]]
        else:
            work.append(nxt[a])
    return sorted(killed), sorted(observed)


def _index(instructions):
    by = {i.address: i for i in instructions}
    order = [i.address for i in instructions]
    prev = {a: (by[order[k - 1]] if k else None) for k, a in enumerate(order)}
    nxt = {i.address: i.address + i.size for i in instructions}
    return by, nxt, prev


def _verdict(flags):
    kinds = {o.split(':')[0] for r in flags.values() for o in r['observed']}
    return 'DEAD' if not kinds else 'BRANCH' if kinds == {'jcc'} else 'EXPOSED'


def entry_liveness(instructions, entry, stops=()):
    """Flags that are live on ARRIVAL at `entry` (e.g. a caller's folded flags reaching a callee): follow all six from
    the entry instruction itself, before it executes. Same result shape and verdicts as liveness()."""
    by, nxt, prev = _index(instructions)
    flags = {}
    for flag in FLAGS:
        killed, observed = _walk(by, nxt, prev, entry, flag, set(stops))
        flags[flag] = dict(killed=killed, observed=observed)
    return dict(site=f'{entry:04X}', instruction=f'(arrival at {by[entry].mnemonic} {by[entry].op_str})', flags=flags,
                verdict=_verdict(flags))


def liveness(instructions, site, stops=()):
    """Follow every flag `site` writes. Returns {flag: {'killed': [...], 'observed': [...]}} plus 'verdict'.

    'observed' entries are 'jcc:<mn>@<addr>' (a conditional branch reading it), 'read:<mn>@<addr>', 'call:@<addr>',
    'return:@<addr>', 'stop:@<addr>' (a host stop before that instruction) or 'leaves@<addr>'. Verdict: DEAD when no flag
    is observed anywhere, BRANCH when the only observers are conditional branches (a model that takes the branch from the
    same condition consumes them without a FLAGS image), else EXPOSED."""
    by = {i.address: i for i in instructions}
    order = [i.address for i in instructions]
    prev = {a: (by[order[k - 1]] if k else None) for k, a in enumerate(order)}
    nxt = {i.address: i.address + i.size for i in instructions}
    stops = set(stops)
    start = by[site]
    result = {}
    for flag in sorted(writes(start, prev[site])):
        killed, observed, seen, work = set(), set(), set(), [nxt[site]]
        while work:
            a = work.pop()
            if a in seen:
                continue
            seen.add(a)
            i = by.get(a)
            if i is None:
                observed.add(f'leaves@{a:04X}'); continue
            if a in stops:
                observed.add(f'stop:@{a:04X}'); continue
            mn = i.mnemonic
            if flag in reads(i):
                observed.add(f'{"jcc" if mn in JCC else "read"}:{mn}@{a:04X}')
            if mn in BOUNDARY:
                observed.add(f'{"return" if mn.startswith("ret") or mn == "iret" else "call"}:@{a:04X}'); continue
            if flag in writes(i, prev[a]):
                killed.add(f'{mn}@{a:04X}'); continue
            if mn in UNCONDITIONAL:
                work.append(int(i.op_str, 16))
            elif mn in JCC:
                work += [int(i.op_str, 16), nxt[a]]
            else:
                work.append(nxt[a])
        result[flag] = dict(killed=sorted(killed), observed=sorted(observed))
    kinds = {o.split(':')[0] for r in result.values() for o in r['observed']}
    result_verdict = 'DEAD' if not kinds else 'BRANCH' if kinds == {'jcc'} else 'EXPOSED'
    return dict(site=f'{site:04X}', instruction=f'{start.mnemonic} {start.op_str}', flags=result, verdict=result_verdict)


# ---- media loading (CLI only) ------------------------------------------------------------------------------------------
ROOT = Path(__file__).resolve().parents[2]


def unit_code(unit):
    sys.path.insert(0, str(Path(__file__).resolve().parent))
    from overlay_inventory import find_units, parse_mz
    exe = (ROOT / 'resources/DOS/POOLRAD/START.EXE').read_bytes(); ovr = (ROOT / 'resources/DOS/POOLRAD/GAME.OVR').read_bytes()
    u = {u['stub_para']: u for u in find_units(exe[parse_mz(exe)['header_bytes']:], ovr)}[unit]
    return ovr[u['ovr_offset']:u['ovr_offset'] + u['code_size']]


def segment_code(segment):
    sys.path.insert(0, str(Path(__file__).resolve().parent))
    from resident_loader_map import unpack
    image = bytes(unpack((ROOT / 'resources/DOS/POOLRAD/START.EXE').read_bytes())[0])
    return image[segment * 16:segment * 16 + 0x10000]


def span_instructions(source, kind, lo, hi):
    code = unit_code(source) if kind == 'unit' else segment_code(source)
    return decode(code[lo:hi], lo)


# The t44 slice-2 rows: (unit or segment, span, TSV row, folded instruction addresses).
PRESETS = {'u31': [
    (('segment', 0x00BA), (0x23B7, 0x23DB), 'overlay_u31_candidate_check_0255_cpu.c:196', [0x23BA]),
    (('unit', 0x31), (0x0255, 0x02EB), 'overlay_u31_candidate_check_0255_cpu.c:230', [0x0258]),
    (('unit', 0x31), (0x0255, 0x02EB), 'overlay_u31_candidate_check_0255_cpu.c:239', [0x026A, 0x0270]),
    (('unit', 0x31), (0x0255, 0x02EB), 'overlay_u31_candidate_check_0255_cpu.c:266', [0x02B3, 0x02B9, 0x02BB]),
    (('unit', 0x31), (0x0255, 0x02EB), 'overlay_u31_candidate_check_0255_cpu.c:269', [0x02C1, 0x02C5, 0x02C7]),
    (('unit', 0x31), (0x0255, 0x02EB), 'overlay_u31_candidate_check_0255_cpu.c:273', [0x02D2]),
    (('unit', 0x31), (0x0675, 0x07D6), 'overlay_u31_step_gate_0675.c:82', [0x0689, 0x0699, 0x06B1, 0x06B8, 0x06FB, 0x072A, 0x0736, 0x073D, 0x07B2]),
    (('unit', 0x31), (0x0675, 0x07D6), 'overlay_u31_step_gate_0675.c:185', [0x0678]),
    (('unit', 0x31), (0x0675, 0x07D6), 'overlay_u31_step_gate_0675.c:193', [0x069F, 0x06A1, 0x06A3, 0x06A5]),
    (('unit', 0x31), (0x0675, 0x07D6), 'overlay_u31_step_gate_0675.c:211', [0x06C0]),
    (('unit', 0x31), (0x0675, 0x07D6), 'overlay_u31_step_gate_0675.c:302', [0x075D, 0x0774, 0x078B, 0x07A2]),
    (('unit', 0x31), (0x0675, 0x07D6), 'overlay_u31_step_gate_0675.c:308', [0x07B4]),
    (('unit', 0x31), (0x0675, 0x07D6), '(header: SHR, MUL)', [0x071F, 0x073F]),
], 'u3b0': [
    (('unit', 0x3B0), (0x00CB, 0x01BA), 'overlay_u3b0_item_split_00cb_cpu.c:109 (pair_index, not live)',
     [0x0135, 0x0137, 0x013C, 0x014C, 0x014E, 0x0153, 0x0168, 0x016A, 0x016F, 0x017F, 0x0181, 0x0186]),
    (('unit', 0x3B0), (0x00CB, 0x01BA), 'overlay_u3b0_item_split_00cb_cpu.c:156', [0x013E, 0x015F, 0x0161, 0x0162, 0x0176]),
    (('unit', 0x3B0), (0x00CB, 0x01BA), 'overlay_u3b0_item_split_00cb_cpu.c:219', [0x010B]),
    (('unit', 0x3B0), (0x00CB, 0x01BA), 'overlay_u3b0_item_split_00cb_cpu.c:235', [0x0190]),
    (('unit', 0x3B0), (0x00CB, 0x01BA), 'overlay_u3b0_item_split_00cb_cpu.c:238', [0x0118]),
    (('unit', 0x3B0), (0x00CB, 0x01BA), '(computed by the model: SUB SP, ADD DI, XOR at the SET call, final pair_index)',
     [0x00CE, 0x00E7, 0x011E, 0x019B, 0x019D, 0x01A2]),
], 'slice5': [
    (('unit', 0x38), (0x1490, 0x14EB), 'overlay_u38_helper_cluster_cpu.c:166', [0x1493]),
    (('unit', 0x38), (0x1490, 0x14EB), 'overlay_u38_helper_cluster_cpu.c:179', [0x14B4]),
    (('unit', 0x38), (0x1490, 0x14EB), 'overlay_u38_helper_cluster_cpu.c:198', [0x14B1]),
    (('unit', 0x38), (0x1490, 0x14EB), 'overlay_u38_helper_cluster_cpu.c:202', [0x14D0]),
    (('unit', 0x31), (0x128D, 0x13CA), 'overlay_u31_table_value_128d_cpu.c:111', [0x12E2, 0x12E4, 0x12FA, 0x12FC, 0x134A, 0x134C, 0x1371, 0x1373]),
    (('unit', 0x31), (0x128D, 0x13CA), 'overlay_u31_table_value_128d_cpu.c:135', [0x129A]),
    (('unit', 0x31), (0x128D, 0x13CA), 'overlay_u31_table_value_128d_cpu.c:168', [0x12C0, 0x12C2]),
    (('unit', 0x3B0), (0x0B03, 0x0C33), 'overlay_u3b0_step_scan_0b03_cpu.c:140', [0x0B5A, 0x0B5E, 0x0BF5, 0x0BF9]),
    (('unit', 0x3B0), (0x0B03, 0x0C33), 'overlay_u3b0_step_scan_0b03_cpu.c:148', [0x0B7F, 0x0C0B, 0x0C1F]),
    (('unit', 0x3B0), (0x0B03, 0x0C33), 'overlay_u3b0_step_scan_0b03_cpu.c:196', [0x0B06]),
    (('unit', 0x3C2), (0x002E, 0x01A0), 'overlay_u3c2_record_sort_002e_cpu.c:90',
     [0x0078, 0x007E, 0x0080, 0x008C, 0x0092, 0x0094, 0x00A0, 0x00A6, 0x00A8, 0x00B1, 0x00B7, 0x00B9, 0x00C6, 0x00CC, 0x00CE,
      0x00D7, 0x00DD, 0x00DF, 0x0118, 0x011E, 0x0120, 0x0139, 0x013F, 0x0141, 0x014C, 0x0152, 0x0154, 0x016D, 0x0173, 0x0175]),
    (('unit', 0x3C2), (0x002E, 0x01A0), 'overlay_u3c2_record_sort_002e_cpu.c:269', [0x0072]),
    (('unit', 0x3C2), (0x002E, 0x01A0), 'overlay_u3c2_record_sort_002e_cpu.c:279', [0x0056]),
    (('unit', 0x3C6), (0x0068, 0x00E3), 'overlay_u3c6_slot_offsets_0068.c:67',
     [0x0091, 0x0095, 0x0097, 0x009E, 0x00A5, 0x00BB, 0x00BF, 0x00C1, 0x00C8, 0x00CF]),
    (('unit', 0x3C6), (0x0068, 0x00E3), 'overlay_u3c6_slot_offsets_0068.c:99', [0x006B]),
    (('unit', 0x3C6), (0x0068, 0x00E3), 'overlay_u3c6_slot_offsets_0068.c:126', [0x0080]),
    (('segment', 0x0AF8), (0x04F7, 0x053C), 'system_textassign_insert_cpu.c:78', [0x0529]),
    (('segment', 0x0AF8), (0x0C68, 0x0CBC), 'system_textassign_insert_cpu.c:145', [0x0C6B]),
    (('segment', 0x0AF8), (0x0C68, 0x0CBC), 'system_textassign_insert_cpu.c:152', [0x0C81]),
    (('segment', 0x0AF8), (0x0B57, 0x0B98), 'Copy 0B57, the callee at 0C84/0CA3: flags arriving at its entry', [('entry', 0x0B57)]),
], 'u77': [
    (('unit', 0x77), (0x28BE, 0x2939), 'overlay_u77_weighted_28be_cpu.c:154', [0x28C1]),
    (('unit', 0x77), (0x28BE, 0x2939), 'overlay_u77_weighted_28be_cpu.c:157', [0x28C4]),
    (('unit', 0x77), (0x28BE, 0x2939), 'overlay_u77_weighted_28be_cpu.c:163', [0x28D9, 0x28DE]),
    (('unit', 0x77), (0x28BE, 0x2939), 'overlay_u77_weighted_28be_cpu.c:180', [0x2902, 0x2905]),
    (('unit', 0x77), (0x28BE, 0x2939), 'overlay_u77_weighted_28be_cpu.c:185', [0x2908]),
    (('unit', 0x77), (0x28BE, 0x2939), 'overlay_u77_weighted_28be_cpu.c:187', [0x28D2]),
    (('unit', 0x77), (0x28BE, 0x2939), '(other writers, not in the TSV)', [0x28E5, 0x28F1, 0x28F3, 0x2914, 0x2917]),
]}


def show(rows, as_json):
    if as_json:
        print(json.dumps(rows, indent=1)); return
    for r in rows:
        killers = sorted({k for f in r['flags'].values() for k in f['killed']})
        observers = sorted({o for f in r['flags'].values() for o in f['observed']})
        live = ''.join(f for f in FLAGS if r['flags'].get(f, {}).get('observed'))
        print(f"{r.get('row', ''):44} {r['site']} {r['instruction']:24} {r['verdict']:8} observed-flags={live or '-'} "
              f"killed-by={','.join(killers) or '-'} observers={';'.join(observers) or '-'}")


def main():
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    g = p.add_mutually_exclusive_group(required=True)
    g.add_argument('--unit', type=lambda s: int(s, 16)); g.add_argument('--segment', type=lambda s: int(s, 16))
    g.add_argument('--preset', choices=sorted(PRESETS))
    p.add_argument('--span'); p.add_argument('--site', action='append', default=[]); p.add_argument('--stop', action='append', default=[])
    p.add_argument('--json', action='store_true')
    a = p.parse_args()
    rows = []
    if a.preset:
        for (kind, source), (lo, hi), row, sites in PRESETS[a.preset]:
            ins = span_instructions(source, kind, lo, hi)
            rows += [dict(row=row, **(entry_liveness(ins, s[1]) if isinstance(s, tuple) else liveness(ins, s))) for s in sites]
    else:
        lo, hi = (int(x, 16) for x in a.span.split(':'))
        ins = span_instructions(a.unit if a.unit is not None else a.segment, 'unit' if a.unit is not None else 'segment', lo, hi)
        stops = [int(s, 16) for s in a.stop]
        rows = [liveness(ins, int(s, 16), stops) for s in a.site]
    show(rows, a.json)


if __name__ == '__main__':
    main()
