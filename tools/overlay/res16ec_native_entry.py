#!/usr/bin/env python3
"""Same-entry original/C comparison of RES_00BA_16EC from a native parity checkpoint (t24, NC3 evidence).

A checkpoint is the private triple written by the hybrid DOSBox-X build's PR_ParityCheckpoint at a registered arrival:
checkpoint.ram (1 MiB), checkpoint.vram (DOSBox-X planar VGA memory, plane p of address a at linear[4*a+p]) and
checkpoint.json (16 registers, counters, video fields). Both sides of the existing #1558 comparator start from that RAM,
those registers and those planes; the EGA sequencer/graphics-controller/latch registers are NOT in a checkpoint, so both
sides start from the device's reset registers (a declared mask, reported in the receipt). The comparator's synthetic
layout (load base, return frame, loaded overlay segment) is rebound to the live one read from the capture.

One entry, one branch. The receipt carries hashes, counts and offsets only; no capture bytes leave resources/. It confers
no NC discharge, identity, readiness or credit. See docs/re/RES_00BA_16EC_NATIVE_ENTRY.md.
"""
import argparse
import ctypes
from hashlib import sha256
import json
from pathlib import Path
import subprocess
import tempfile

import res00ba_window_16ec_cpu_audit as A
import res00ba_display_1693_cpu_audit as D
import res16ec_port_boundary as B

REGISTER_KEYS = ('eax', 'ebx', 'ecx', 'edx', 'esi', 'edi', 'ebp', 'esp', 'cs', 'eip', 'ds', 'es', 'ss', 'flags')
ENV = dict(table={8: (0x0123, 0xF000)}, in61=0xB0, time_start=4736147, time_step=7)
RESET_REGISTERS = dict(seq=[0, 0, 15, 0, 0], gc=[0, 0, 0, 0, 0, 0, 0, 15, 255])


def require(ok, message):
    if not ok:
        raise ValueError(message)


def planes_from_vram(vram):
    """DOSBox-X stores EGA/VGA planes interleaved: plane p of address a is linear[4 * a + p]."""
    require(len(vram) >= 4 * 65536, 'VRAM shorter than four planes')
    return [bytearray(vram[p:4 * 65536:4]) for p in range(4)]


def load_checkpoint(run):
    run = Path(run)
    meta = json.loads((run / 'checkpoint.json').read_text())
    ram = (run / 'checkpoint.ram').read_bytes()
    vram = (run / 'checkpoint.vram').read_bytes()
    require(len(ram) == A.SIZE, 'checkpoint.ram is not 1 MiB')
    require(meta.get('id') == 'RES_00BA_16EC', 'checkpoint is not a RES_00BA_16EC arrival')
    r = meta['registers']
    cpu = A.Cpu(*[r[k] & 0xFFFF for k in REGISTER_KEYS])
    require(cpu.ip == A.ENTRY, 'captured IP is not 16EC')
    return meta, bytes(ram), planes_from_vram(vram), cpu, dict(json=sha256((run / 'checkpoint.json').read_bytes()).hexdigest(),
                                                               ram=sha256(ram).hexdigest(), vram=sha256(vram).hexdigest())


def entry_facts(ram, cpu):
    """Load base, return frame, tile-stub state and the pushed arguments, read from the captured RAM."""
    w = lambda s, o: ram[A.physical(s, o & 0xFFFF)] | ram[A.physical(s, (o + 1) & 0xFFFF)] << 8
    base = (cpu.cs - A.SEG16EC) & 0xFFFF
    stub_at = A.physical(base + A.STUBSEG, A.STUB_ENTRY)
    stub = ram[stub_at:stub_at + 5]
    loaded = stub[0] == 0xEA and (stub[1] | stub[2] << 8) == 0x015D
    return dict(load_base=base, return_to=(w(cpu.ss, cpu.sp + 2), w(cpu.ss, cpu.sp)), stub_loaded=loaded,
                overlay_segment=(stub[3] | stub[4] << 8) if loaded else None,
                wait=w(cpu.ss, cpu.sp + 4) & 0xFF, row=w(cpu.ss, cpu.sp + 6) & 0xFF,
                message=(w(cpu.ss, cpu.sp + 10), w(cpu.ss, cpu.sp + 8)), title=(w(cpu.ss, cpu.sp + 14), w(cpu.ss, cpu.sp + 12)))


def rebind(base, return_to, overlay_segment):
    """Point the comparator's synthetic layout at the live one. Returns the previous values for restoring."""
    previous = (A.BASE, A.RETURN_TO, A.OVSEG)
    A.BASE, A.RETURN_TO = base, return_to
    if overlay_segment is not None:
        A.OVSEG = overlay_segment
    return previous


def restore(previous):
    A.BASE, A.RETURN_TO, A.OVSEG = previous


def run_c_with_planes(planes):
    """A.run_c with the captured planes in the C pr_ega instead of the synthetic fill."""
    def run_c(f, lib, m, cpu, envspec, keys, stubs, budget, text_loopbacks=65536):
        keep = []
        q, env = A.Q.c_env(envspec, keep)
        ke = A.K.KeyEnv(q, A.K.keyboard(env, keys, keep), None, 64)
        text = A.O.T.c_driver(stubs, False, keep)
        box_args = [A.O.Env15(A.O.VideoEnv(q.video, None), text, ke, 1 << 20), 1 << 20]
        if any(name == 'verify_target' for name, _ in A.BX.Env03._fields_):
            box_args.append(1)
        e = A.Env16(65536, 65536, 65536, 65536, A.P.Budget(65536, 65536, text_loopbacks, 65536, 10 ** 8, 65536, 65536, 65536),
                    A.M.Env0BF5(ke, 65536, 65536, 65536), 64, A.BX.Env03(*box_args), 10 ** 8, 65536, 65536)
        actual = A.Cpu.from_buffer_copy(cpu); ram = (ctypes.c_uint8 * A.SIZE).from_buffer_copy(m)
        ega = A.M.Ega(); lib.pr_ega_reset(ctypes.byref(ega))
        for p in range(4):
            ctypes.memmove(ctypes.addressof(ega.plane[p]), bytes(planes[p]), 65536)
        events, writes = [], []

        def out(ctx, port, value, size, cs, ip):
            events.append((port, value, size, cs, ip)); return 0

        def write(ctx, off, value):
            writes.append((off, value))
        ports = A.E0.Env(None, A.E0.OUT_FN(out), A.E0.IN_FN()); keep.append(ports)
        bus = A.M.Bus(ram, ctypes.pointer(ega), ctypes.pointer(ports), A.G.WRITE_FN(write), None)
        d = A.Diag16(); left = ctypes.c_size_t(budget)
        status = A.STATUS[f(ctypes.byref(bus), A.SIZE, ctypes.byref(actual), ctypes.byref(left), ctypes.byref(e), ctypes.byref(d))]
        return status, actual, bytes(ram), env, d, events, writes, bytes(ega), left.value
    return run_c


def device_with_planes(planes):
    def new_device():
        d = D.PythonEga(); d.plane = [bytearray(p) for p in planes]; return d
    return new_device


def compare_checkpoint(ram, planes, cpu, facts, library, name='native-entry'):
    """Run the original (Unicorn) and the C model from the checkpoint; A.compare raises on any difference."""
    previous = rebind(facts['load_base'], facts['return_to'], facts['overlay_segment'])
    saved = (A.run_c, A.M.new_device)
    try:
        A.run_c, A.M.new_device = run_c_with_planes(planes), device_with_planes(planes)
        image, sites = A.Q.load_image()
        _, body_ips, _ = A.source(image, sites)
        stubs = {(A.O.STUB_SEG, A.O.IN_STUB): 0, (A.O.STUB_SEG, A.O.FLUSH_STUB): 0}
        info = dict(name=name, budget=1000, sp=cpu.sp, mode5=False, wait=None, row=None, msg=None, plain=False, text_loopbacks=65536)
        trace = B.Trace()
        ustop, seen, edges, _, d = A.compare((bytearray(ram), cpu, ENV, [], stubs, info), body_ips, *library, observe=trace)
    finally:
        A.run_c, A.M.new_device = saved
        restore(previous)
    return dict(stop=ustop, own_instructions=len(seen), edges=len(edges), callee=d.callee, callee_status=d.callee_status,
                site=f'{d.site:04X}', selectors=trace.selectors, box_frames=trace.box_frames, tile_calls=len(trace.tile_frames))


def report(run, result, facts, cpu, hashes, meta):
    return dict(capture=dict(run=str(run), sha256=hashes, instruction=meta.get('instruction'), arrival=meta.get('arrival'), video=meta.get('video')),
                entry=dict(load_base=f'{facts["load_base"]:04X}', cs_ip=f'{cpu.cs:04X}:{cpu.ip:04X}', ss_sp=f'{cpu.ss:04X}:{cpu.sp:04X}', ds=f'{cpu.ds:04X}',
                           bp=f'{cpu.bp:04X}', flags=f'{cpu.flags:04X}', return_to='%04X:%04X' % facts['return_to'],
                           tile_stub='loaded %04X:015D' % facts['overlay_segment'] if facts['stub_loaded'] else 'cold INT 3Fh',
                           wait=facts['wait'], row=facts['row'], message_at='%04X:%04X' % facts['message'], title_at='%04X:%04X' % facts['title'],
                           entry_sp_in_model_profile=0x0800 <= cpu.sp <= 0xFFF0),
                comparison=dict(result, compared='stop, CPU14 with pinned flag rules, 1 MiB RAM, environment events, EGA OUT events, VRAM write operands, packed EGA state, budget'),
                masks=['EGA sequencer/graphics-controller/latch registers are not captured: both sides start from the reset registers ' + json.dumps(RESET_REGISTERS) + '; plane contents are the captured ones',
                       'the environment (INT 21h time, port 61h, keys) is the declared synthetic one, identical on both sides'],
                claims=['same-entry original/C agreement for this one entry up to its stop; no natural-arrival, frequency, closure, identity, readiness or credit claim'],
                tree=subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=A.ROOT, text=True).strip())


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--run', type=Path, required=True, help='directory holding checkpoint.{json,ram,vram}')
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    meta, ram, planes, cpu, hashes = load_checkpoint(args.run)
    facts = entry_facts(ram, cpu)
    with tempfile.TemporaryDirectory(prefix='nc3-') as work:
        result = compare_checkpoint(ram, planes, cpu, facts, A.library(work))
    out = report(args.run, result, facts, cpu, hashes, meta)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(out, indent=2, sort_keys=True) + '\n')
    print(json.dumps({k: out[k] for k in ('entry', 'comparison')}, indent=1, sort_keys=True))


if __name__ == '__main__':
    main()
