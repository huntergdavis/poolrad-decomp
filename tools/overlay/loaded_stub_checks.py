"""Host refusal checks for loaded-overlay preconditions, not DOS executions."""
import ctypes


def verify_refusals(memory, cpu, invoke, call_sites, *, check_target_cs=True):
    """Exercise cold/malformed stubs and each live call; require atomic ARGUMENT.

    invoke takes a fresh ctypes RAM array and CPU and returns the model status.
    call_sites contains (caller segment, LCALL offset) pairs in the valid fixture.
    """
    size = 1 << 20
    assert len(memory) == size
    physical = lambda seg, off: ((seg << 4) + (off & 65535)) & (size - 1)
    word = lambda at: memory[at] | memory[(at + 1) & (size - 1)] << 8
    edits, stubs = [], set()
    for seg, off in call_sites:
        at = physical(seg, off)
        assert memory[at] == 0x9A, (seg, off, 'fixture lacks LCALL')
        stub = physical(word(physical(seg, off + 3)), word(physical(seg, off + 1)))
        assert memory[stub] == 0xEA, (seg, off, 'fixture lacks loaded stub')
        edits += [(f'call-{seg:04X}:{off:04X}-opcode', [(at, 0x90)]),
                  (f'call-{seg:04X}:{off:04X}-offset',
                   [(physical(seg, off + 1), memory[physical(seg, off + 1)] ^ 1)])]
        stubs.add(stub)
    for at in sorted(stubs):
        edits += [(f'stub-{at:05X}-unit-absent', [(at, 0xCD), ((at + 1) & (size - 1), 0x3F)]),
                  (f'stub-{at:05X}-opcode', [(at, 0x90)]),
                  (f'stub-{at:05X}-entry', [((at + 1) & (size - 1), memory[(at + 1) & (size - 1)] ^ 1)])]
        if check_target_cs:
            edits.append((f'stub-{at:05X}-segment',
                          [((at + 3) & (size - 1), memory[(at + 3) & (size - 1)] ^ 1)]))
    for label, changes in edits:
        altered = bytearray(memory)
        for at, value in changes:
            altered[at] = value
        ram = (ctypes.c_uint8 * size).from_buffer_copy(altered)
        state = type(cpu).from_buffer_copy(bytes(cpu))
        status = invoke(ram, state)
        assert status == 1, (label, 'expected ARGUMENT', status)
        assert bytes(state) == bytes(cpu), (label, 'CPU changed')
        assert bytes(ram) == altered, (label, 'RAM changed')
    return dict(cases=len(edits), unit_absent_cases=len(stubs), cpu_words=14,
                ram_bytes=size, preserved=True, original_execution=False)
