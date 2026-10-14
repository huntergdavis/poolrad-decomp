#!/usr/bin/env python3
"""Pinned native and original-byte full-state continuation audit."""
import ctypes as ct
from hashlib import sha256
import json
from pathlib import Path
import re
import sys
import unicorn
from unicorn import Uc, UC_ARCH_X86, UC_MODE_16
ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tests/replay'))
from audit_slot_a_string_0356_abi import CPU, State, Report, NAMES, REGS, cpu_values, compare_cpu
sys.path.insert(0, str(ROOT / 'tests/recovered'))
import check_slot_a_u6f_196a_resident_native as pins
RAM_SIZE = 655360

def differential(lib, label, image, values, expected_status, checkpoints=None):
    memory = ct.create_string_buffer(image, len(image))
    cpu = CPU(**dict(zip(NAMES, values)))
    state = State(cpu, ct.cast(memory, ct.POINTER(ct.c_uint8)), len(image))
    report = Report()
    uc = Uc(UC_ARCH_X86, UC_MODE_16)
    uc.mem_map(0, 0x100000)
    uc.mem_write(0, image)
    for register, value in zip(REGS, values):
        uc.reg_write(register, value)
    seen = set()
    for steps in range(1000):
        pc = (state.cpu.cs, state.cpu.ip)
        before_cpu, before_ram = cpu_values(state.cpu), memory.raw
        status = lib.pr_slot_a_continuation_1974_step(ct.byref(state), ct.byref(report))
        if status:
            if status != expected_status:
                raise AssertionError(f"{label}: unexpected status {status} at {pc[0]:04X}:{pc[1]:04X}")
            compare_cpu(cpu_values(state.cpu), before_cpu, "blocked state")
            if memory.raw != before_ram:
                raise AssertionError("blocked instruction mutated RAM")
            if not report.dependency_valid:
                raise AssertionError("missing typed call dependency")
            if expected_status == 1:
                assert (report.caller, report.call_pc, report.callee_segment,
                        report.callee_pc, report.return_pc, report.caller_is_overlay) == (
                            0x6F, 0x1999, 0x0AF8, 0x0B98, 0x199E, 1)
            if checkpoints and seen != set(checkpoints):
                raise AssertionError("not all native checkpoints were reached")
            print(f"{label}: {steps} C instruction steps, CPU/full-RAM Unicorn differential passed; blocker={status}")
            return steps, len(seen)
        for repetition in range(65537):
            uc.emu_start((uc.reg_read(REGS[0]) << 4) + uc.reg_read(REGS[1]),
                         0x100000, count=1)
            if (uc.reg_read(REGS[0]), uc.reg_read(REGS[1])) != pc:
                break
        else:
            raise AssertionError("Unicorn instruction did not finish")
        context = f"{label} after {pc[0]:04X}:{pc[1]:04X}"
        compare_cpu(cpu_values(state.cpu), [uc.reg_read(r) & 0xFFFF for r in REGS], context)
        if memory.raw != bytes(uc.mem_read(0, len(image))):
            raise AssertionError(context + ": full RAM differs")
        if checkpoints:
            key = (state.cpu.cs, state.cpu.ip)
            for checkpoint, (target, expected_cpu, expected_ram) in checkpoints.items():
                if checkpoint not in seen and key == target:
                    compare_cpu(cpu_values(state.cpu), expected_cpu, checkpoint)
                    if memory.raw != expected_ram:
                        raise AssertionError(checkpoint + ": native full RAM differs")
                    seen.add(checkpoint)
    raise AssertionError("C instruction bound exhausted")


RAM_HASHES = {
    "1974": "7c7ee407affc465f389c7f390bdff97daf1905bb4e9944887fc20d8c276d28d7",
    "1978": "7c7ee407affc465f389c7f390bdff97daf1905bb4e9944887fc20d8c276d28d7",
    "1979": "7c7ee407affc465f389c7f390bdff97daf1905bb4e9944887fc20d8c276d28d7",
    "197a": "c5bd50eb7f763bd8c48574b3012c5760757a1ba1ccf4037fe655b9494f44a230",
    "197e": "dece8eb6572f341587c216ff56419cce01dadae6175875964c0c329ab691707f",
    "0b25": "de0d96a79e0468e459b47a11388ada3259768d987ae0a52038729d4d04cd59a7",
    "0b3d": "de0d96a79e0468e459b47a11388ada3259768d987ae0a52038729d4d04cd59a7",
    "0b3e": "03ef5fcdb465643825fd47f33d2ea6f58c8cf541e2cb0a43cc9c77af1a09fe6c",
    "0b44": "42a2dadaf5e098405dcf9b328b20300338ea17eb54fa4eb8f9d0a676f348e785",
    "1983": "42a2dadaf5e098405dcf9b328b20300338ea17eb54fa4eb8f9d0a676f348e785",
    "198e": "a00dc553efec3b1cc0e705674acc353b18cdef1359466ce1d5f7f17d1801545c",
    "1993": "11bb1b4905a84fda74624f844831b6527bc5fb5fcb78494f61e7b8f204f83fbc",
    "1999": "e34c73126f13f2c3ee8495f725dfbf9ae1014d6d749c499a579fcc22cd6c62a5"
}

def native_inputs(root):
    raw = (root / 'native.log').read_bytes()
    assert sha256(raw).hexdigest() == '5d47098a9bf6cdb64db889cf3c20faa111bb727764fba4a916a01269d7e6773a'
    text = re.sub(r'\x1b\[[0-?]*[ -/]*[@-~]', '', raw.decode(errors='replace'))
    rows = re.findall(r"LOG: EV of 'CS IP DS ES FS GS SS SP BP AX BX CX DX SI DI FLAGS' is:\s*LOG: ([0-9a-f ]+)", text)
    assert len(rows) == len(RAM_HASHES)
    cpus = {name: [int(v, 16) for v in row.split()] for name, row in zip(RAM_HASHES, rows)}
    for name, values in cpus.items():
        assert len(values) == 16
        assert values[:2] == [0x131C if name.startswith('0b') else 0x2A8E, int(name, 16)]
        assert values == [json.loads((root / ('cpu-' + name + '.json')).read_text())[n] for n in NAMES]
    images = {name: pins.checked(root / ('ram-' + name + '.bin'), RAM_SIZE, digest)
              for name, digest in RAM_HASHES.items()}
    changed = lambda a,b: {i for i,(x,y) in enumerate(zip(images[a],images[b])) if x != y}
    assert not changed('1974','1979')  # first PUSH writes an existing SS word
    assert changed('1979','197a') == {0x20910,0x20911}
    assert changed('0b3d','0b3e') == {0x20D34}
    assert changed('0b3e','0b44') == set(range(0x20D35,0x20D3D))
    assert len(changed('1974','1999')) == 28
    word = lambda label, off: int.from_bytes(images[label][0x1D160+off:0x1D160+off+2], 'little')
    assert [word('0b25', p) for p in range(0x37AA,0x37B8,2)] == [
        0x1983,0x2A8E,0x28,0x3BD4,0x1D16,0x39B6,0x1D16]
    assert [word('198e', p) for p in range(0x37B0,0x37B8,2)] == [0x537A,0x14A0,0x3AB8,0x1D16]
    assert [word('1999', p) for p in range(0x37B0,0x37B8,2)] == [0x3BD4,0x1D16,0x3AB8,0x1D16]
    return cpus, images


def original_code(root, ram):
    start = pins.checked(root / 'POOLRAD/START.EXE',65231,pins.START_HASH)
    game = pins.checked(root / 'POOLRAD/GAME.OVR',206105,pins.GAME_HASH)
    pins.checked(root / 'POOLRAD/SAVGAMA.DAT',13137,pins.SAVE_HASH)
    body = game[0x1DFE6+0x1974:0x1DFE6+0x199E]
    assert sha256(body).hexdigest() == '54afeb7162e304f9236a5bb3c29cf8ffbc95b34b3d10c455d7a47256680437b3'
    loaded = bytearray(body)
    for pc in (0x197E,0x198E,0x1999):
        i = pc-0x1974
        assert body[i] == 0x9A and body[i+3:i+5] == b'\xf8\x0a'
        loaded[i+3:i+5] = (0x131C).to_bytes(2,'little')
    assert ram[0x2A8E0+0x1974:0x2A8E0+0x199E] == loaded
    assert sha256(loaded).hexdigest() == 'c5df882a2e21af59008381d080f06dd3ce125e398927d03e318589f60ed12e32'
    for pc,off,size,digest in (
        (0xB0B,0xB781,26,'4681f1935f55d71f05afab0b13fff0505aa17e9a04fb372ff559ef28ed88058f'),
        (0xB25,0xB79B,36,'009955722e8f73d0a0f7076aa418d141f642c5a729b2a58dfa7a7a3922989118'),
    ):
        source = start[off:off+size]
        assert sha256(source).hexdigest() == digest
        assert ram[0x131C0+pc:0x131C0+pc+size] == source


def main():
    if len(sys.argv) != 3:
        raise SystemExit('usage: audit_slot_a_continuation_1974.py SHARED_LIBRARY IGNORED_NATIVE_ROOT')
    if unicorn.__version__ != '2.1.4' or not __debug__:
        raise SystemExit('requires Unicorn 2.1.4 and non-optimized Python assertions')
    library,root = (Path(arg).resolve(strict=True) for arg in sys.argv[1:])
    if any(not p.is_relative_to((ROOT/'resources').resolve()) for p in (library,root)):
        raise SystemExit('private inputs and library must stay in this clone resources/')
    lib = ct.CDLL(str(library))
    lib.pr_slot_a_continuation_1974_step.argtypes = [ct.POINTER(State),ct.POINTER(Report)]
    lib.pr_slot_a_continuation_1974_step.restype = ct.c_int
    cpus,images = native_inputs(root)
    for image in images.values(): original_code(root,image)
    checkpoints = {name: ((v[0],v[1]),v,images[name]) for name,v in cpus.items() if name != '1974'}
    steps,seen = differential(lib,'native saved-A',images['1974'],cpus['1974'],1,checkpoints)
    print(f'native: {seen} post-entry CPU/full-RAM checkpoints; {steps} instruction steps')
    for name,length,prefix,overlap in (
        ('zero-length-wrap',0,0,0),('cap-and-max-prefix',255,255,0),
        ('B25-forward-overlap',5,3,1),('B0B-forward-overlap',8,3,2),
    ):
        image = bytearray(images['1974']);values = cpus['1974'].copy()
        values[7] = 0x8000
        if overlap == 2: values[8],values[2] = 0x568D,values[6]
        values[15] |= 0x400
        stack = values[6]<<4
        address = ((values[8]-0x1F7)&65535) if overlap == 1 else (0xFFFF if length == 0 else 0x5000)
        image[stack+0x8000:stack+0x8004] = address.to_bytes(2,'little')+values[6].to_bytes(2,'little')
        image[stack+address:stack+address+1+length] = bytes([length])+b'A'*length
        table = (values[2]<<4)+0x537A
        image[table:table+1+prefix] = bytes([prefix])+b'B'*prefix
        differential(lib,'synthetic-'+name,bytes(image),values,1)
    print('1974 native/original-byte/Unicorn/C differential complete')


if __name__ == '__main__': main()
