#!/usr/bin/env python3
"""Measure the conditional 16EC -> box -> tile boundary, not strict readiness.

Reuse #1558's synthetic original-byte world and full-state comparator. The
observer only reads the original CPU/memory; it neither skips nor replaces
guest instructions. See docs/re/RES_00BA_16EC_PORT_BOUNDARY.md for the proof's
frame-preservation precondition and the unproved live-port obligations.
"""
import argparse
from collections import Counter
from hashlib import sha256
import json
from pathlib import Path
import subprocess
import tempfile

from unicorn import x86_const as R
import res00ba_window_16ec_cpu_audit as A

PINS = {
    '16ec': 'c8507a56b6044087a23b724d368ac78558bae1ce5fb42f6eace1d29985feb929',
    'box': 'fae6cb41423bcd48237e75a08f5eda70642254eac8449373291d57f34120a8a4',
    'tile': '2a8a7826dcf35576b32a0aa02862f4d7bb43c985918f9b2cb170acf0cbe9df00',
}
BOX_ARGS = [0, 22, 38, 17, 1]  # style, bottom, right, top, left: low bytes
EXCLUDED = (0x01C5, 0x01D7, 0x01DC, 0x01E1, 0x01E9, 0x0249)


def require(condition, message):
    if not condition:
        raise ValueError(message)


def pin_bodies(window, box, tile):
    for name, body in zip(PINS, (window, box, tile)):
        require(sha256(body).hexdigest() == PINS[name], f'{name} body pin mismatch')


def inputs():
    image, sites = A.Q.load_image()
    window, ips, _ = A.source(image, sites)
    box, _ = A.BX.source(image)
    code, fixups, tile, _ = A.O.source()
    pin_bodies(window, box, tile)
    return A.Q.relocate(image, sites, A.BASE), code, fixups, ips


class Trace:
    """Only observations; all offsets below are from the pinned original bodies."""

    def __init__(self):
        self.box_frames = []
        self.tile_frames = []
        self.box_locals = []
        self.move_frames = []
        self.move_return_bases = []
        self.sites = Counter()
        self.selectors = {}
        self.box_bp = None

    def __call__(self, uc, cs, ip):
        ss, sp = uc.reg_read(R.UC_X86_REG_SS), uc.reg_read(R.UC_X86_REG_SP)

        def words(off, count):
            return [int.from_bytes(uc.mem_read(A.physical(ss, (off + 2 * i) & 65535), 2), 'little')
                    for i in range(count)]

        if cs == A.BASE + A.SEG16EC and ip in (0x1707, 0x1757):
            ds = uc.reg_read(R.UC_X86_REG_DS)
            off = 0x49F3 if ip == 0x1707 else 0x6DDA
            self.selectors[f'{off:04X}'] = uc.mem_read(A.physical(ds, off), 1)[0]
        if (cs, ip) == (A.BASE + 0x03F1, 0):
            self.box_frames.append(words(sp + 4, 5))
            self.box_bp = (sp - 2) & 65535
        if cs == A.OVSEG:
            if ip == 0x028B:
                self.move_frames.append(words(sp, 5))  # count, dst off/seg, src off/seg, before CALL
            if ip == 0x0290 and self.box_bp is not None:
                self.move_return_bases.append(words(self.box_bp - 3, 1)[0])
            if ip in dict(A.O.CALL_SITES):
                self.sites[ip] += 1
            if ip == 0x015D:
                self.tile_frames.append(words(sp + 4, 5))
                if self.box_bp is not None:
                    self.box_locals.append(words(self.box_bp - 3, 1)[0])

    def report(self):
        return dict(selectors=self.selectors, box_frames=self.box_frames,
                    tile_calls=len(self.tile_frames),
                    tile_words=sorted({tuple(w[:3]) for w in self.tile_frames}),
                    box_tile_bases=sorted(set(self.box_locals)),
                    move_frames=self.move_frames,
                    move_return_bases=self.move_return_bases,
                    tile_sites={f'{site:04X}': self.sites[site] for site, _ in A.O.CALL_SITES})


def validate(trace, stop, *, box, loaded=True, null=False):
    require(len(trace.box_frames) == int(box), 'box call count')
    for frame in trace.box_frames:
        require([w & 255 for w in frame] == BOX_ARGS, 'box frame differs from fixed 16EC arguments')
    expected = 92 if box and loaded else 0
    require(len(trace.tile_frames) == expected, 'tile count: expected 92 for the preserved-frame loaded box')
    require(all(v == 0x114 for v in trace.box_locals), 'box tile-base local was not preserved')
    require(not any(trace.sites[site] for site in EXCLUDED), 'excluded tile call site reached')
    # Order matters: corners, paired horizontal edges, then paired vertical edges.
    geometry = [(0, 16, 0x114), (39, 16, 0x114), (0, 23, 0x114), (39, 23, 0x114)]
    geometry += [(x, y, 0x116) for x in range(1, 39) for y in (16, 23)]
    geometry += [(x, y, 0x115) for y in range(17, 23) for x in (0, 39)]
    if expected:
        actual = [(w[4] & 255, w[3] & 255, w[2]) for w in trace.tile_frames]
        require(actual == geometry, 'tile geometry or numbers differ')
    for words in trace.tile_frames:
        require(words[0] & 255 == words[1] & 255 == 0, 'tile flag low byte nonzero')
    for site in (0x028B, 0x02A2):
        require(trace.sites[site] == (0 if null else expected), 'Move/1951 call count')
    require(stop == ('OVERLAY' if box and not loaded else 'RETURNED'), 'unexpected boundary stop')


def cases():
    return ([('mode5', dict(mode5=True), False, True, False),
             ('colour12', dict(mode5=False, flag6dda=1), False, True, False),
             ('colour12-ff', dict(mode5=False, flag6dda=255), False, True, False)] +
            [(f'box-mode{mode}', dict(mode5=False, flag6dda=0, gmode=mode), True, True, False)
             for mode in range(4)] +
            [('box-null', dict(mode5=False, flag6dda=0), True, True, True),
             ('box-cold', dict(mode5=False, flag6dda=0, loaded=False), True, False, False)])


def build(name, options, data, null=False):
    image, code, fixups, _ = data
    state = A.build(f't131-{name}', image, code, fixups, msg=b'Hello', title=b'Ti', **options)
    if null:
        ram, cpu = state[:2]
        for off in (0x5E4A, 0x5E4C):  # band 4 pointer: 5E3A + 4*4
            A.put16(ram, cpu.ds, off, 0)
    return state


def measure(state, ips, library=None):
    trace = Trace()
    if library is None:
        m, cpu, env, keys, _, info = state
        stop = A.run_unicorn(info['name'], m, cpu, env, keys, ips, info['budget'],
                             info['text_loopbacks'], observe=trace)[3]
    else:
        stop = A.compare(state, ips, *library, observe=trace)[0]
    return trace, stop


def alias_state(data, overwrite):
    """Matched initial-memory worlds; no code patch or observer-side write.

    Band 4's object is at 3400:0000 with a two-byte item size. First tile
    114h rebases to item 14h, so source +17h+14h*2 contains 011Fh.
    The bad world's destination+17h is the box base-tile local.
    At 1951, the aliased object's height overlaps the tile body's saved
    source-offset local (zero), so mode 0 has no rows and returns normally.
    """
    state = build('frame-alias', dict(mode5=False, gmode=0), data)
    ram, cpu = state[:2]
    seg = A.O.TILES + 0x400
    for off, value in ((0, 2), (2, 2), (0x11, 2), (0x13, A.O.AUX[0]), (0x15, A.O.AUX[1])):
        A.put16(ram, seg, off, value)
    A.put16(ram, cpu.ds, 0x5E4A, 0)
    A.put16(ram, cpu.ds, 0x5E4C, seg)
    A.put16(ram, cpu.ds, 0x272A, 0x100)
    A.put16(ram, seg, 0x17 + 0x14 * 2, 0x11F)
    if overwrite:
        box_bp = (cpu.sp - 0x114) & 65535
        A.put16(ram, cpu.ds, 0x5E4E, (box_bp - 3 - 0x17) & 65535)
        A.put16(ram, cpu.ds, 0x5E50, cpu.ss)
    return state


def validate_alias(trace, stop, state):
    require(len(trace.box_frames) == 1 and [w & 255 for w in trace.box_frames[0]] == BOX_ARGS,
            'alias control changed the caller arguments')
    require(trace.box_bp == (state[1].sp - 0x114) & 65535, 'alias control box frame geometry')
    require(trace.box_locals == [0x114, 0x11F], 'alias did not change the next original tile number')
    require(trace.move_return_bases == [0x11F], 'original Move did not overwrite the box local')
    require(len(trace.move_frames) == 1, 'expected one original Move before the error')
    count, off, seg, _, _ = trace.move_frames[0]
    require(count == 2 and A.physical(seg, off) == A.physical(state[1].ss, trace.box_bp - 3),
            'original Move destination is not the protected box local')
    require(len(trace.tile_frames) == 2 and trace.tile_frames[1][2] == 0x11F,
            'alias control did not break the 92-tile conclusion')
    require(trace.sites[0x01C5] == 1 and stop == 'EXIT', 'alias error path did not exit')
    try:
        validate(trace, stop, box=True)
    except ValueError as error:
        require('tile count' in str(error), 'alias rejected for an unexpected reason')
    else:
        raise ValueError('violated preservation condition silently re-admitted the exclusion')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    data = inputs()
    results = []
    controls = []
    with tempfile.TemporaryDirectory(prefix='t131-boundary-') as work:
        library = A.library(work)
        for name, options, box, loaded, null in cases():
            trace, stop = measure(build(name, options, data, null), data[3], library)
            validate(trace, stop, box=box, loaded=loaded, null=null)
            results.append(dict(name=name, stop=stop, **trace.report()))
        for overwrite in (False, True):
            state = alias_state(data, overwrite)
            trace, stop = measure(state, data[3], None if overwrite else library)
            candidate = None
            if overwrite:
                validate_alias(trace, stop, state)
                # The violated world reaches the existing flat-only error-call
                # guard with EGA attached. This is NOT a parity comparison:
                # the original exits, while the candidate refuses at 01C5.
                m, cpu, env, keys, stubs, info = state
                cstop, _, _, _, diagnostic, *_ = A.run_c(*library, m, cpu, env, keys, stubs,
                                                       info['budget'], info['text_loopbacks'])
                require(cstop == 'CALLEE' and diagnostic.callee == 6 and diagnostic.callee_status == 6
                        and diagnostic.box.tile.site == 0x01C5, 'violated world candidate guard changed')
                candidate = dict(stop=cstop, callee=diagnostic.callee, callee_status=diagnostic.callee_status,
                                 tile_site=f'{diagnostic.box.tile.site:04X}', parity=False,
                                 original_status_matches=False)
            else:
                validate(trace, stop, box=True)
            controls.append(dict(name='frame-alias-011f-two-tiles-exit' if overwrite else 'frame-alias-baseline',
                                 synthetic_violation=overwrite, exclusion_available=not overwrite,
                                 full_state_comparison=not overwrite, candidate=candidate, stop=stop, **trace.report()))
    report = dict(head=subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=A.ROOT, text=True).strip(),
                  image_sha256=A.Q.IMAGE_DIGEST, body_sha256=PINS, cases=results,
                  controls=controls, full_state_comparisons=len(results) + 1, original_only_controls=1,
                  live_gameplay=False, native=False,
                  claim='Conditional tile-path exclusion only; no identity, readiness or closure verdict.',
                  preconditions=['Pinned immutable code and correct loaded overlay target',
                                 'Callees preserve box frame and each tile argument frame',
                                 'Inherited #1558 display, aperture and bounded-execution contract'],
                  not_verified=['Live route selector/stack/stub/object state',
                                'Universal no-alias proof', 'Native 16EC parity',
                                'Overlay loading', 'Whole 16EC termination', 'Strict readiness'])
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2, sort_keys=True) + '\n')
    print(f'PASS: {len(results)} boundary + 1 baseline full-state comparisons; 1 original-only alias control and C guard observation. '
          'Alias C/original stops DIFFER; no violation-world parity. Native/live gameplay NOT VERIFIED')


if __name__ == '__main__':
    main()
