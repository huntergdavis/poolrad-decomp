"""Original-byte flag-liveness certificate using the shared t44 walker.

The fourteen groups preserve the source-line identities selected from
FLAG_FOLDED_SITES.tsv at 9d55a9b0. Adjacent unlabelled writes are included,
so every arithmetic writer in the two owned original spans is accounted for.
"""
import ctypes
from collections import Counter
from hashlib import sha256

import flag_fold_liveness as shared_liveness

ALL = frozenset(('CF', 'PF', 'AF', 'ZF', 'SF', 'OF'))
GROUPS = (
    (140, (0x10D6, 0x10DA, 0x10DC, 0x10E8, 0x10EC, 0x10EE,
           0x10FA, 0x10FE, 0x1100, 0x113E, 0x1142, 0x1144)),
    (160, (0x1185, 0x118A, 0x11A3, 0x11A8, 0x11B6, 0x11BB, 0x11C9,
           0x11CE, 0x1208, 0x120D, 0x1225, 0x122A, 0x1241, 0x1246)),
    (181, (0x0C62,)),
    (188, (0x0C71, 0x0C77, 0x0C7B, 0x0C7D)),
    (194, (0x0C87, 0x0C8C, 0x0C95, 0x0C9E)),
    (313, (0x1196, 0x119B, 0x11AC)),
    (342, (0x117F,)),
    (355, (0x1215, 0x121E, 0x1233, 0x123A, 0x124F)),
    (376, (0x1202,)),
    (386, (0x126B, 0x1272, 0x1277, 0x127D)),
    (412, (0x10AA,)),
    (427, (0x10CF,)),
    (449, (0x111D,)),
    (451, (0x1123, 0x112C, 0x1130, 0x1132, 0x1134)),
)
BRANCH_CONDITIONS = {
    0x0C87: 0x0C8A, 0x0C8C: 0x0C8F, 0x0C95: 0x0C99, 0x0C9E: 0x0CA2,
    0x1196: 0x1199, 0x119B: 0x119E, 0x11AC: 0x11B1, 0x1215: 0x1219,
    0x1233: 0x1235, 0x124F: 0x1251, 0x111D: 0x1121, 0x1123: 0x1127,
    0x1134: 0x1139,
}
LIVE_SHIFTS = {0x1100: 0x111A, 0x1144: 0x1295}
PREVIOUS_ARITH = frozenset((0x0CA4, 0x10AD, 0x1152, 0x1163, 0x1175,
                            0x11D2, 0x11E7, 0x11F8, 0x125A, 0x125F, 0x127F))
STOPS = frozenset((0x117F, 0x11DF))
FLAG_NAMES = dict(C='CF', P='PF', A='AF', Z='ZF', S='SF', O='OF')


def observation_summary(result):
    """Present the shared walker's per-bit evidence in the inventory's format."""
    observers, kills = {}, {}
    for flag, paths in result['flags'].items():
        for field, destination in (('observed', observers), ('killed', kills)):
            for marker in paths[field]:
                at = int(marker.rsplit('@', 1)[1], 16)
                destination.setdefault(at, set()).add(FLAG_NAMES[flag])
    return (sorted((ip, tuple(sorted(flags))) for ip, flags in observers.items()),
            sorted((ip, tuple(sorted(flags))) for ip, flags in kills.items()))


def prove(code, decoded):
    expected = ((0x0C5F, 0x0CB9, 'c153e951cb4be5e40f4fd65d9e6d114a0b2d3cbd615bc73025a3d2c4c387de41'),
                (0x10A7, 0x1298, '50821d75da31383ab9920ef18edaadb16d9ffdea34cc4348c8e86d70903fd1fe'))
    for start, end, digest in expected:
        if sha256(code[start:end]).hexdigest() != digest:
            raise ValueError(f'original span hash changed at {start:04X}')
    owned = {ip: ins for (kind, ip), ins in decoded.items()
             if kind == 'u3c6' and (0x0C5F <= ip < 0x0CB9 or 0x10A7 <= ip < 0x1298)}
    listed = [ip for _, sites in GROUPS for ip in sites]
    instructions = [owned[ip] for ip in sorted(owned)]
    actual_writers = {ip for ip, ins in owned.items() if shared_liveness.writes(ins)}
    if len(listed) != len(set(listed)) or set(listed) | PREVIOUS_ARITH != actual_writers:
        raise ValueError('fold inventory does not account for every original flag writer')
    rows, counts = [], Counter()
    for number, (line, sites) in enumerate(GROUPS, 1):
        entries = []
        for ip in sites:
            result = shared_liveness.liveness(instructions, ip, stops=STOPS)
            observers, kills = observation_summary(result)
            if ip in BRANCH_CONDITIONS:
                if result['verdict'] != 'BRANCH' or observers != [(BRANCH_CONDITIONS[ip], ('ZF',))]:
                    raise ValueError(f'branch observer changed at {ip:04X}: {observers}')
                verdict = 'branch_condition_reproduced'
            elif ip in LIVE_SHIFTS:
                if result['verdict'] != 'EXPOSED' or observers != [(LIVE_SHIFTS[ip], tuple(sorted(ALL)))]:
                    raise ValueError(f'live shift boundary changed at {ip:04X}: {observers}')
                verdict = 'already_computed'
            else:
                if result['verdict'] != 'DEAD' or observers:
                    raise ValueError(f'fold is not dead at {ip:04X}: {observers}')
                verdict = 'dead_before_observation'
            counts[verdict] += 1
            entries.append(dict(writer=f'{ip:04X}', verdict=verdict, shared_liveness=result,
                                first_observers=[dict(ip=f'{at:04X}', flags=list(flags)) for at, flags in observers],
                                overwrites=[dict(ip=f'{at:04X}', flags=list(flags)) for at, flags in kills]))
        rows.append(dict(group=number, original_source_line=line, writers=entries))
    return dict(inventory_revision='9d55a9b0', engine='tools/overlay/flag_fold_liveness.py',
                host_stops=[f'{ip:04X}' for ip in sorted(STOPS)], groups=len(rows), original_instructions=len(owned),
                original_flag_writers=len(actual_writers), classified_writers=len(listed),
                verdict_counts=dict(counts), certificate=rows,
                scope='immutable code; all original branches, calls, returns, and declared host stops')


SHIFT_TRACE_C = r'''
static uint16_t shift_trace[2][3];
static size_t shift_trace_count;
void pr_u3c6_shift_trace_reset(void) { shift_trace_count = 0u; }
size_t pr_u3c6_shift_trace_size(void) { return shift_trace_count; }
uint16_t pr_u3c6_shift_trace_value(size_t index, unsigned field) { return shift_trace[index][field]; }
static void record_shift(uint16_t site, const pr_u3c6_10a7_cpu *cpu)
{
    if (shift_trace_count < 2u) {
        shift_trace[shift_trace_count][0] = site;
        shift_trace[shift_trace_count][1] = cpu->flags;
        shift_trace[shift_trace_count][2] = cpu->di;
    }
    ++shift_trace_count;
}

'''


def instrument_shifts(source, directory, mutation=None):
    """Observe the two live slot_index results in a temporary checker build.

    The normal probe only reads CPU words into a separate host buffer. Optional
    mutations deliberately break one site's AF or PF for negative controls.
    The committed recovered C and the native checker are unchanged.
    """
    text = source.read_text()
    call, anchor = 'slot_index(&m, 1);', 'static size_t physical('
    if text.count(call) != 2 or text.count(anchor) != 1:
        raise ValueError('live-shift probe anchors changed')
    if mutation is not None and (mutation[0] not in LIVE_SHIFTS or mutation[1] not in ('af-zero', 'pf-word')):
        raise ValueError(f'unknown live-shift mutation: {mutation}')
    pieces = text.split(call)
    text = pieces[0]
    for site, rest in zip(LIVE_SHIFTS, pieces[1:]):
        broken = ''
        if mutation == (site, 'af-zero'):
            broken = ' cpu->flags &= (uint16_t)~0x0010u;'
        elif mutation == (site, 'pf-word'):
            broken = (' cpu->flags = (uint16_t)((cpu->flags & ~0x0004u) | '
                      '((even_parity((uint8_t)cpu->di) == even_parity((uint8_t)(cpu->di >> 8))) ? 0x0004u : 0u));')
        text += call + broken + f' record_shift(0x{site:04X}u, cpu);' + rest
    text = text.replace(anchor, SHIFT_TRACE_C + anchor, 1)
    path = directory / 'u3c610a7-shift-probe.c'
    path.write_text(text)
    return path


def bind_shift_trace(module, function):
    module.pr_u3c6_shift_trace_reset.argtypes = []
    module.pr_u3c6_shift_trace_reset.restype = None
    module.pr_u3c6_shift_trace_size.argtypes = []
    module.pr_u3c6_shift_trace_size.restype = ctypes.c_size_t
    module.pr_u3c6_shift_trace_value.argtypes = [ctypes.c_size_t, ctypes.c_uint]
    module.pr_u3c6_shift_trace_value.restype = ctypes.c_uint16

    def values():
        count = module.pr_u3c6_shift_trace_size()
        if count > 2:
            raise ValueError('live-shift trace overflow')
        return [tuple(module.pr_u3c6_shift_trace_value(i, field) for field in range(3)) for i in range(count)]

    function.shift_trace_reset = module.pr_u3c6_shift_trace_reset
    function.shift_trace_values = values
    function.shift_trace_counts = Counter()


def compare_shift_trace(function, expected, label):
    actual = function.shift_trace_values()
    if len(actual) != len(expected):
        raise ValueError(f'live-shift trace length {label}: C={len(actual)}, original={len(expected)}')
    for (site, flags, di), (want_site, want_flags, want_di) in zip(actual, expected):
        if (site, flags, di) != (want_site, want_flags, want_di):
            raise ValueError(f'live-shift mismatch {label} at +{want_site:04X}: '
                             f'C site/FLAGS/DI={site:04X}/{flags:04X}/{di:04X}, '
                             f'pinned={want_site:04X}/{want_flags:04X}/{want_di:04X}')
        function.shift_trace_counts[site] += 1
