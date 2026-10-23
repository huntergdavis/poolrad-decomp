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
        if previous is not None and previous.mnemonic == 'mov' and previous.op_str.split(', ')[0] in ('cl', 'cx'):
            value = previous.op_str.split(', ')[1]
            if value.startswith('0x') or value.isdigit():
                n = int(value, 0) & 0x1F                            # MOV CL,n or MOV CX,n just before (CL is the low byte)
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
    written = writes(start, prev[site])
    if not written and start.mnemonic in SHIFTS | ROTATES:
        # A folded shift/rotate whose count is not known here: analyse every flag it can write (a zero count writes
        # none, so this is the conservative side); never report such a site DEAD vacuously.
        written = ALL if start.mnemonic in SHIFTS else frozenset('CO')
    for flag in sorted(written):
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
], 'slice6': [
    (('segment', 0x0AF8), (0x1254, 0x12A6), 'system_str_1366_cpu.c:119 (12A3 SUB CX,DI; returns to 137A)', [0x12A3]),
    (('segment', 0x0AF8), (0x1366, 0x13B1), 'system_str_1366_cpu.c:119 (flags arriving back at 137A)', [('entry', 0x137A)]),
    (('segment', 0x0AF8), (0x1366, 0x13B1), 'system_str_1366_cpu.c:130', [0x1369]),
    (('segment', 0x0AF8), (0x1254, 0x12A6), 'system_str_1366_cpu.c:130 (flags arriving at the 1254 callee)', [('entry', 0x1254)]),
    (('segment', 0x0AF8), (0x0A5B, 0x0A76), 'system_longmul_0a5b_cpu.c:62', [0x0A5F, 0x0A65, 0x0A6B]),
    (('segment', 0x0AF8), (0x0A5B, 0x0A76), 'system_longmul_0a5b_cpu.c:75', [0x0A71]),
    (('segment', 0x0AF8), (0x0B25, 0x0B49), 'system_strstore_fullstate_0b25.c:34', [0x0B37]),
    (('segment', 0x0AF8), (0x0B57, 0x0B98), 'system_string_move_leaves_cpu.c:95', [0x0B66]),
    (('segment', 0x0AF8), (0x0A76, 0x0AE4), 'system_longdiv_0a76_cpu.c:98', [0x0A9F, 0x0AA1, 0x0AA6, 0x0AA8, 0x0AAA, 0x0AAC, 0x0AB0, 0x0AB2, 0x0AB4, 0x0AB5, 0x0AB7, 0x0AB9]),
    (('segment', 0x0AF8), (0x13E2, 0x1410), 'system_ioresult_assign_cpu.c:84', [0x13FD]),
    (('segment', 0x0AF8), (0x0308, 0x03D3), 'system_getmem_023f_cpu.c:103', [0x0354, 0x0356, 0x0358, 0x035A, 0x0387, 0x0389, 0x038B, 0x038D]),
    (('segment', 0x0AF8), (0x0491, 0x04A2), 'system_freemem_0254_cpu.c:103', [0x0491, 0x0496, 0x0498, 0x049A, 0x049C]),
    (('segment', 0x00BA), (0x2E8B, 0x2F57), 'resident_character_target_select.c:214', [0x2ECD]),
    (('segment', 0x0802), (0x05CD, 0x06E7), 'resident_0802_05cd_cpu.c:187', [0x0622]),
    (('segment', 0x0709), (0x01DF, 0x05EB), 'resident_0709_glyph_01df_cpu.c:3', [0x0212, 0x0255, 0x0257, 0x0286, 0x02A1, 0x02AE, 0x02C1, 0x02D8, 0x030D, 0x0328, 0x0335, 0x0348, 0x035F, 0x039C, 0x03FD, 0x0423, 0x0485, 0x048F, 0x0491, 0x04BF, 0x04DB, 0x04EB, 0x04FA, 0x051C, 0x02DF, 0x0366, 0x03A1, 0x0538, 0x055D, 0x0582, 0x05A7]),
    (('segment', 0x0709), (0x0DD3, 0x0F82), 'resident_0709_clear_0dd3_cpu.c:96', [0x0DE7, 0x0DE9, 0x0DF2, 0x0DF4, 0x0E09, 0x0E28, 0x0E4A, 0x0EB9, 0x0EC2, 0x0ECC, 0x0ECE, 0x0EE0, 0x0EE2, 0x0E2F, 0x0E51, 0x0F06, 0x0F23, 0x0F40, 0x0F5D]),
    (('segment', 0x03F1), (0x0137, 0x02E8), 'resident_03f1_clear_0137_cpu.c:96', [0x014B, 0x014D, 0x0156, 0x0158, 0x016D, 0x018C, 0x01AE, 0x021F, 0x0228, 0x0232, 0x0234, 0x0246, 0x0248, 0x0193, 0x01B5, 0x026C, 0x0289, 0x02A6, 0x02C3]),
    (('segment', 0x00BA), (0x2199, 0x2208), 'resident_00ba_sidecar_lookup_2199_cpu.c:91', [0x219C]),
    (('segment', 0x00BA), (0x26C2, 0x2729), 'resident_00ba_class_value_26c2_cpu.c:46', [0x26C5, 0x26CB, 0x26D1, 0x270D, 0x2713]),
    (('segment', 0x00BA), (0x1037, 0x1073), 'resident_00ba_byte_string_1037_cpu.c:78', [0x103A, 0x1041, 0x1043]),
    (('unit', 0x9E), (0x10E4, 0x110D), 'overlay_u9e_entry16_10e4.c:86', [0x10E7, 0x10ED, 0x10EF]),
    (('unit', 0x9E), (0x10E4, 0x110D), 'overlay_u9e_entry16_10e4.c:97 (not in the TSV; equal by construction, see the doc)', [0x10FE]),
    (('unit', 0x6A), (0x0000, 0x0159), 'overlay_u6a_record_weight_0000.c:111', [0x012A, 0x0136, 0x0142, 0x012F, 0x013B, 0x0147]),
    (('unit', 0x3CE), (0x014E, 0x01B0), 'overlay_u3ce_bitmap_pair_release_014e_cpu.c:36', [0x0157, 0x015D, 0x016C, 0x0172, 0x0182, 0x0188, 0x0197, 0x019D]),
    (('unit', 0x3B4), (0x00C7, 0x00FB), 'overlay_u3b4_case_maps.c:64 (00C7)', [0x00CA]),
    (('unit', 0x3B4), (0x00FB, 0x012F), 'overlay_u3b4_case_maps.c:64 (00FB)', [0x00FE]),
    (('unit', 0x3B4), (0x012F, 0x016D), 'overlay_u3b4_case_maps.c:64 (012F)', [0x0132]),
    (('unit', 0x3B0), (0x11F3, 0x1245), 'overlay_u3b0_dispose_11f3_cpu.c:26', [0x11F6]),
    (('unit', 0x3B0), (0x0CC5, 0x0D88), 'overlay_u3b0_cursor_step_0cc5_cpu.c:102', [0x0CC8]),
    (('unit', 0x33), (0x02BA, 0x0377), 'overlay_u33_stamp_block_02ba.c:114', [0x02BD]),
    (('unit', 0x33), (0x0221, 0x02BA), 'overlay_u33_stamp_block_02ba.c:114 (flags arriving at the 0221 callee)', [('entry', 0x0221)]),
    (('unit', 0x33), (0x0377, 0x040F), 'overlay_u33_stamp_square_0377.c:109', [0x037A]),
    (('unit', 0x38), (0x001F, 0x0058), 'overlay_u38_gated_append_001f_cpu.c:28', [0x0022]),
    (('unit', 0x38), (0x2915, 0x2976), 'overlay_u38_roll_gate_2915_cpu.c:182', [0x2931, 0x2935, 0x2937, 0x2939]),
    (('unit', 0x38), (0x2A87, 0x2AE6), 'overlay_u38_target_index_gate_2a87.c:264', [0x2AB4]),
    (('unit', 0x38), (0x2A87, 0x2AE6), 'overlay_u38_target_index_gate_2a87.c:298', [0x2ACA]),
    (('unit', 0x38), (0x25BC, 0x2605), 'overlay_u38_target_mask81_25bc.c:254', [0x25E9]),
    (('unit', 0x3B0), (0x08D7, 0x0923), 'overlay_u3b0_space_scan_cpu.c:180', [0x090F]),
    (('unit', 0x3B0), (0x0923, 0x0969), 'overlay_u3b0_space_scan_cpu.c:205', [0x0955]),
    (('unit', 0x3B9), (0x048F, 0x04F8), 'overlay_u3b9_bitmap_set_release_048f_cpu.c:63', [0x04BD]),
    (('unit', 0x3B9), (0x048F, 0x04F8), 'overlay_u3b9_bitmap_set_release_048f_cpu.c:74', [0x04B3]),
    (('unit', 0x6F), (0x1446, 0x157F), 'overlay_u6f_party_insert_1446_cpu.c:239', [0x1449]),
    (('unit', 0x6F), (0x1446, 0x157F), 'overlay_u6f_party_insert_1446_cpu.c:331', [0x1541, 0x1551, 0x1555, 0x155E]),
    (('unit', 0x86), (0x0729, 0x07D6), 'overlay_u86_entry3_0729.c:67', [0x0746, 0x075B, 0x0773, 0x0786, 0x07AA, 0x074C, 0x0761, 0x0779, 0x078C, 0x07B0]),
    (('unit', 0x86), (0x0729, 0x07D6), 'overlay_u86_entry3_0729.c:75', [0x0752, 0x0767, 0x077F, 0x0792]),
    (('unit', 0x1C), (0x0AED, 0x0B52), 'overlay_u1c_compare_and_0aed_cpu.c:226', [0x0B23, 0x0B27]),
    (('unit', 0x1C), (0x0A9F, 0x0AED), 'overlay_u1c_return_0a9f_cpu.c:26', [0x0AA2]),
    (('unit', 0x1C), (0x2E7D, 0x2F2D), 'overlay_u1c_text_2e7d_cpu.c:92', [0x2EDE]),
    (('unit', 0x20), (0x0355, 0x04C9), 'overlay_u20_party_award_0355_cpu.c:141', [0x03C0, 0x03ED, 0x041A, 0x0446, 0x03C3, 0x03F0, 0x041D, 0x0449]),
    (('unit', 0x25), (0x1951, 0x19DB), 'overlay_u25_alloc_1951_cpu.c:179', [0x19BB]),
    (('unit', 0x25), (0x1FB1, 0x207E), 'overlay_u25_entry23_1fb1.c:204', [0x2010, 0x2012, 0x2033, 0x2035]),
    (('unit', 0x25), (0x0482, 0x0511), 'overlay_u25_step_probe_0482.c:98', [0x04E2, 0x04EB, 0x04F4, 0x04FD]),
    (('unit', 0x25), (0x0F53, 0x1153), 'overlay_u25_store_string_0f53.c:239', [0x0FBD, 0x0FDD, 0x104B, 0x106B, 0x10B8, 0x10D8, 0x1120, 0x113E, 0x0FC0, 0x0FE0, 0x104E, 0x106E, 0x10BB, 0x10DB]),
    (('unit', 0x33), (0x01AD, 0x0221), 'overlay_u33_cell_pair_01ad.c:140', [0x01B6, 0x01B8]),
    (('unit', 0x33), (0x0810, 0x08A4), 'overlay_u33_neighbourhood_0810.c:164', [0x0813]),
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
