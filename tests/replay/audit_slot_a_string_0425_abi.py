#!/usr/bin/env python3
"""Full CPU/RAM parity of the reached +0425 ABI against pinned original bytes."""
import ctypes as ct
from hashlib import sha256
import json
from pathlib import Path
import re
import sys
import unicorn
ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tests/replay'))
from audit_slot_a_string_0356_abi import CPU, State, Report, NAMES, REGS, cpu_values, compare_cpu
from unicorn import Uc, UC_ARCH_X86, UC_MODE_16
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
    for steps in range(200000):
        pc = (state.cpu.cs, state.cpu.ip)
        before_cpu, before_ram = cpu_values(state.cpu), memory.raw
        status = lib.pr_slot_a_string_0425_step(ct.byref(state), ct.byref(report))
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
                            0x6F, 0x197E, 0x0AF8, 0x0B25, 0x1983, 1)
            elif expected_status == 2:
                assert (report.caller, report.call_pc, report.callee_segment,
                        report.callee_pc, report.return_pc, report.caller_is_overlay) == (
                            0x0802, 0x03FC, 0x0AF8, 0x0CBC, 0x0401, 0)
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
    '196f': '76d331baf03e73951f46f88830691760a6ecb96d6af67a112416909bdda8b528',
    '0425': '28cd43235f7a5e4cafdf2c309cffcf6e95c50b5eda59282e0b7384a63be36b79',
    '0426': 'c899cd755c9ebb268c3a4fa1b1e4f4e3c93122d275ee42c163f3a53baa9547ff',
    '1974': 'a7705a78f6b014fa11406def3da8d1a255affdd573223f21d4645d50f6ddc35b',
}

def checked(path, size, digest):
    data = path.read_bytes()
    assert len(data) == size and sha256(data).hexdigest() == digest, path.name
    return data


def native_inputs(root):
    raw = (root / 'native.log').read_bytes()
    assert sha256(raw).hexdigest() == '728a2a6336abfebf39c8759737204b2ab5905cc6b194a7b210770e9609d7f082'
    text = re.sub(r'\x1b\[[0-?]*[ -/]*[@-~]', '', raw.decode(errors='replace'))
    rows = re.findall(r"LOG: EV of 'CS IP DS ES FS GS SS SP BP AX BX CX DX SI DI FLAGS' is:\s*LOG: ([0-9a-f ]+)", text)
    assert len(rows) == 4
    cpus = {name: [int(value, 16) for value in row.split()]
            for name, row in zip(RAM_HASHES, rows)}
    for name, values in cpus.items():
        assert len(values) == 16
        assert values == [json.loads((root / ('cpu-' + name + '.json')).read_text())[n] for n in NAMES]
        assert values[:2] == ([0x2A8E, int(name, 16)] if name in ('196f', '1974')
                             else [0x1026, int(name, 16)])
    images = {name: checked(root / ('ram-' + name + '.bin'), RAM_SIZE, digest)
              for name, digest in RAM_HASHES.items()}
    a, b, c = (images[n] for n in ('196f', '0425', '0426'))
    changed = lambda x, y: {i for i, (u, v) in enumerate(zip(x, y)) if u != v}
    assert changed(a, b) == {0x2090C, 0x2090D, 0x2090E, 0x2090F}
    assert changed(b, c) == {0x2090A, 0x2090B}
    assert c[0x2090A:0x2090C] == (0x3DCA).to_bytes(2, 'little')
    assert len(changed(a, images['1974'])) == 93
    return cpus, images


def original_code(root, ram):
    # Reuse PR389's independently pinned +0356 body and relocation audit.
    sys.path.insert(0, str(ROOT / 'tests/recovered'))
    import check_slot_a_u6f_196a_resident_native as native
    native.check_code(root, ram)
    start = (root / 'POOLRAD/START.EXE').read_bytes()
    game = (root / 'POOLRAD/GAME.OVR').read_bytes()
    source = start[0x847C:0x8566]
    assert sha256(source).hexdigest() == '28c69009d8bd4218b1eb50754c3f32a6f36ef40d3927d630fc8481d10b99e5a3'
    relocated = bytearray(source)
    for pc in (0x43B, 0x46A, 0x47D, 0x49D, 0x4AC, 0x4D8, 0x504):
        offset = pc - 0x425
        assert source[offset] == 0x9A and source[offset + 3:offset + 5] == b'\xf8\x0a'
        relocated[offset + 3:offset + 5] = (0x131C).to_bytes(2, 'little')
    assert ram[0x10260 + 0x425:0x10260 + 0x50F] == relocated
    assert sha256(relocated).hexdigest() == '7a6d1534f0a3b448a6eebc18108cc78743e14b842f3637acd7305e1cb8658bb5'
    for pc, file_offset, size, digest in (
        (0xB25, 0xB79B, 36, '009955722e8f73d0a0f7076aa418d141f642c5a729b2a58dfa7a7a3922989118'),
        (0xB57, 0xB7CD, 65, '18383182a06f0521f0873061e61004f0b1c6bec8f3b30b1158ec7ad7822940fa'),
        (0xC10, 0xB886, 43, '11e2860118cb471f9f1ecb1d0c44c060e0fbb83862f69c1cf39b5e0484298a58'),
        (0xC3B, 0xB8B1, 18, 'b2f0ef7b50632ff33ad1b2b251db935542c81b596360ac8b7cd6b391f3fca484'),
        (0x182A, 0xC4A0, 19, '738c9a82c7faab6335f0952cc5d989d273cb85828a0735f34966220f370d012c'),
    ):
        body = start[file_offset:file_offset + size]
        assert sha256(body).hexdigest() == digest
        assert ram[0x131C0 + pc:0x131C0 + pc + size] == body
    for pc, size, original_hash, loaded_hash in (
        (0x196F, 5, 'fe81385603a3fc55df47de14941cf1bdbf72d0bd7c936edaecb4ceda0c23d52e',
         '78cec5fcdde872ab5e4c4add6d5debd1f4cf3d3430f3e12b1a051c4ce061d147'),
        (0x1974, 15, '40340a7f4f937729d3786c40ce28c8cc26d796f9abd977727eaafe0a254b1253',
         '85ccb6667d3a08f5fe27f10b74ee586c9487648ced403928853c8d8c06d4aa6c'),
    ):
        assert sha256(game[0x1DFE6 + pc:0x1DFE6 + pc + size]).hexdigest() == original_hash
        assert sha256(ram[0x2A8E0 + pc:0x2A8E0 + pc + size]).hexdigest() == loaded_hash


def main():
    if len(sys.argv) != 3:
        raise SystemExit('usage: audit_slot_a_string_0425_abi.py SHARED_C_LIBRARY IGNORED_NATIVE_ROOT')
    if unicorn.__version__ != '2.1.4' or not __debug__:
        raise SystemExit('requires Unicorn 2.1.4 and non-optimized Python assertions')
    library, root = (Path(arg).resolve(strict=True) for arg in sys.argv[1:])
    private = (ROOT / 'resources').resolve()
    if not library.is_relative_to(private) or not root.is_relative_to(private):
        raise SystemExit('library and native inputs must stay in this clone resources/')
    lib = ct.CDLL(str(library))
    lib.pr_slot_a_string_0425_step.argtypes = [ct.POINTER(State), ct.POINTER(Report)]
    lib.pr_slot_a_string_0425_step.restype = ct.c_int
    cpus, images = native_inputs(root)
    for image in images.values():
        original_code(root, image)
    checkpoints = {name: ((values[0], values[1]), values, images[name])
                   for name, values in cpus.items() if name != '196f'}
    steps, seen = differential(lib, 'native saved-A', images['196f'], cpus['196f'], 1, checkpoints)
    print(f'native: {seen} post-entry CPU/full-RAM checkpoints match, including +1974; steps={steps}')
    for name, source, alias, table_alias, status in (
        ('empty-source', b'', False, False, 1),
        ('ascii-boundaries', b'@az[\x60z{\xff', False, False, 1),
        ('prefix-truncation', b'abcdefghijklmno', False, False, 1),
        ('source-output-alias', b'abcdefghijk', True, False, 1),
        ('table-miss-binary', b'\x00\x80\xffABC', False, False, 1),
        ('delete-boundary', b'\xe0ABC', False, False, 2),
        ('live-table-local-alias', b'ABCDEFGH', False, True, 2),
    ):
        image = bytearray(images['196f'])
        values = cpus['196f'].copy()
        values[7], values[8] = 0x8000, 0x8100
        values[15] |= 0x400  # reached helper must clear incoming DF
        if table_alias:
            values[2] = values[6] + 0x71E  # DS:0D1A aliases BP-100 after entry
        stack = values[6] << 4
        def put_word(offset, value):
            image[stack + offset:stack + offset + 2] = value.to_bytes(2, 'little')
        for offset, address in ((0x8000, 0x5000), (0x8004, 0x5000 if alias else 0x5200)):
            put_word(offset, address)
            put_word(offset + 2, values[6])
        table = (values[2] << 4) + 0xD1A
        image[table:table + 10] = bytes(range(0xE0, 0xEA))
        image[stack + 0x5000:stack + 0x5001 + len(source)] = bytes([len(source)]) + source
        differential(lib, 'synthetic-' + name, bytes(image), values, status)
    print('0425 original-byte/native/Unicorn/C differential complete')


if __name__ == '__main__':
    main()
