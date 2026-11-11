#!/usr/bin/env python3
"""Run002 narrow instruction-derived prefix; stops before INT 21h.

This is an experimental checked profile, not a replacement for the recovered
raw resident driver and not a CPU or whole-memory parity claim.
"""
import hashlib
import pathlib
import re
import sys

EXPECTED = {
    'RAM-0742-SEGMENTED.BIN': '2ef55fd5fd893f3e5397130a35bfc77777c4e126eda939bebda5da3cace7de4e',
    'RAM-1386-SEGMENTED.BIN': '2f2490902d61e23ff8503249acccf0034988e5606280b12616c04e7444765f07',
    'CPU-0742.TXT': '0fcc08571fcf5771e1d9c177554ce9b192e75f5090789c8e93d2f38b94d89efd',
    'CPU-1386.TXT': '4f34ecf7352f2dc2a731e9b4e4df5831e4223b9aabec5341ba4aad4a8ee8e773',
}
class Uncovered(Exception):
    pass
class Profile:
    def __init__(self, raw):
        if len(raw) != 0xa0000:
            raise ValueError('expected 640 KiB segmented capture')
        self.data = bytearray(raw)
        self.touched = set()
    def read(self, addr, size):
        if addr < 0 or size < 0 or addr + size > len(self.data):
            raise Uncovered((addr, size))
        return bytes(self.data[addr:addr + size])
    def word(self, seg, off):
        return int.from_bytes(self.read((seg << 4) + off, 2), 'little')
    def push(self, cpu, value):
        cpu['SP'] = (cpu['SP'] - 2) & 0xffff
        at = (cpu['SS'] << 4) + cpu['SP']
        self.read(at, 2)  # Refuse before changing an uncovered address.
        self.data[at:at + 2] = value.to_bytes(2, 'little')
        self.touched.update((at, at + 1))

def parse(data):
    fields = dict((k, int(v, 16)) for k, v in re.findall(r'([A-Z]+)=([0-9A-F]{4})', data.decode()))
    expected = {'AX','BX','CX','DX','SI','DI','BP','SP','CS','IP','DS','ES','SS','FLAGS'}
    if set(fields) != expected:
        raise ValueError('invalid CPU transcription')
    return fields

def run(directory):
    files = {}
    for name, digest in EXPECTED.items():
        raw = (directory / name).read_bytes()
        if hashlib.sha256(raw).hexdigest() != digest:
            raise ValueError('hash mismatch: ' + name)
        files[name] = raw
    mem = Profile(files['RAM-0742-SEGMENTED.BIN'])
    native = files['RAM-1386-SEGMENTED.BIN']
    cpu = parse(files['CPU-0742.TXT'])
    want = parse(files['CPU-1386.TXT'])
    if (cpu['CS'], cpu['IP']) != (0x2e60, 0x0742):
        raise ValueError('unexpected call site')
    # Pin reached loaded code spans (hashes from capture review), including
    # the far call target and the resident dispatch/vector prefix.
    call = mem.read((cpu['CS'] << 4) + cpu['IP'], 5)
    if call != bytes.fromhex('9adc00d210'):
        raise ValueError('far CALL identity mismatch')
    for addr, size, digest in ((0x10d20 + 0x00dc,159,'aae1673000ebd85e67c66df978d2b986de8e4d52cdd7994ade46f1ac5d50bb2a'),
                                (0x10d20 + 0x137c,53,'ec254758235958ca2220f8e010218ee8fb3cb4da5c1a5e59ba39c7468efdf192')):
        if hashlib.sha256(mem.read(addr,size)).hexdigest() != digest:
            raise ValueError('resident loaded code mismatch')
    # Far CALL pushes caller CS, then return IP; no registers/flags change.
    mem.push(cpu,cpu['CS']); mem.push(cpu,0x0747); cpu['CS']=0x10d2; cpu['IP']=0x00dc
    mem.push(cpu,cpu['BP']); cpu['BP']=cpu['SP']
    mode=mem.read((cpu['DS']<<4)+0x5372,1)[0]
    active=mem.word(cpu['DS'],0x5374)
    if mode != 1 or active != 100:
        raise ValueError('captured branch outside supported profile')
    # CMP word [5374],0 updates arithmetic flags; this profile is nonzero
    # and bounded to the captured value. Compute all six arithmetic flags.
    flags=cpu['FLAGS'] & ~0x08d5
    if active & 0x8000: flags |= 0x80
    if active == 0: flags |= 0x40
    if (active & 0xff).bit_count() % 2 == 0: flags |= 0x04
    cpu['FLAGS']=flags
    mem.push(cpu,cpu['CS']); mem.push(cpu,0x00f4); cpu['IP']=0x137c
    mem.push(cpu,cpu['DS']); mem.push(cpu,cpu['ES'])
    cpu['AX']=mem.word(cpu['CS'],0x137f); cpu['DS']=cpu['AX']
    cpu['AX']=0x3508; cpu['IP']=0x1386  # stop BEFORE INT frame or DOS
    if cpu != want:
        raise ValueError('CPU transcription mismatch: ' + repr((cpu,want)))
    if any(mem.data[i] != native[i] for i in mem.touched):
        raise ValueError('touched stack differs from same-run 1386')
    try:
        mem.read(0x9ffff,2)
    except Uncovered:
        pass
    else:
        raise ValueError('uncovered-read test failed')
    try:
        mem.push({'SS': 0xa000, 'SP': 4}, 0)
    except Uncovered:
        pass
    else:
        raise ValueError('uncovered-write test failed')
    print('derived far CALL -> resident pre-INT 1386: CPU14 and',len(mem.touched),'modeled stack bytes match; DOS not called')
    print('conditional on no unobserved interrupt; no full RAM/device parity')

if __name__ == '__main__':
    if len(sys.argv) != 2:
        raise SystemExit('usage: check_slot_a_resident_prefix.py PRIVATE_RUN002_DIR')
    try:
        run(pathlib.Path(sys.argv[1]))
    except (OSError, ValueError, Uncovered) as exc:
        raise SystemExit(str(exc))
