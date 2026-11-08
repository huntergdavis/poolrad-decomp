"""Falsifiable controls for the 16EC entry contract: synthetic bodies, then the owner-media census and pins."""
import unittest
from unittest import mock

import res16ec_entry_contract as E

# Hand-assembled 16-bit fragments (hex). Literals live at the offsets named in the comments.
LEA_LOCAL = lambda n: bytes([0x8D, 0x7E, (256 - n) & 0xFF, 0x16, 0x57])          # lea di,[bp-n]; push ss; push di
LIT = lambda off: bytes([0xBF, off & 0xFF, off >> 8, 0x0E, 0x57])                 # mov di,off; push cs; push di
PARAM = lambda k: bytes([0xC4, 0x7E, k, 0x06, 0x57])                             # les di,[bp+k]; push es; push di
STRLOAD = bytes.fromhex('9a0b0bf80a')
STRCAT = bytes.fromhex('9a980bf80a')
STRSTORE = lambda cap: bytes([0xB8, cap & 0xFF, cap >> 8, 0x50]) + bytes.fromhex('9a250bf80a')
TAIL = lambda row, wait: bytes([0xB0, row, 0x50, 0xB0, wait, 0x50])
CALL16EC = bytes.fromhex('9aec16ba00')
OTHER_CALL = bytes.fromhex('9a00001234')                                         # lcall 1234:0000, an unknown callee
PROLOGUE = bytes.fromhex('558bec83ec40')                                          # push bp; mov bp,sp; sub sp,40h
EPILOGUE = bytes.fromhex('8be55dca0c00')                                          # mov sp,bp; pop bp; retf 0Ch


class FakeWorld:
    def __init__(self, code, params=None):
        self.code = code
        self.params = params or {}

    def code_of(self, kind, container):
        return self.code

    def param_bound(self, kind, container, body, k, depth):
        return self.params.get(k, E.UNBOUNDED)


def synthetic(fragments, literals, params=None):
    code = bytearray(0x400)
    body = PROLOGUE + b''.join(fragments) + EPILOGUE
    code[:len(body)] = body
    for off, length in literals.items():
        code[off] = length
    world = FakeWorld(bytes(code), params)
    parsed = E.Body(bytes(code), 0, len(body), 'synthetic')
    call = [k for k, i in enumerate(parsed.ins) if E.far_target(i) == E.TARGET]
    assert len(call) == 1
    return world, parsed, call[0]


def bound_at(world, body, k):
    units = E.push_units(body, k)
    message = E.argument_at(units, 10)
    state = E.Analysis(world, 'ovl', 0x38, body, 0).before(k)
    return message, state[0].get(message[1], E.UNBOUNDED)


class SyntheticControls(unittest.TestCase):
    def test_literal_temp_is_bounded_by_its_literal(self):
        world, body, k = synthetic([LEA_LOCAL(0x10) + LIT(0x300) + STRLOAD + TAIL(10, 1) + CALL16EC], {0x300: 17})
        message, bound = bound_at(world, body, k)
        self.assertEqual(message, ('temp', 0x10, 0, ('lit', 0x300)))
        self.assertEqual(bound, 17)
        self.assertEqual(E.argument_at(E.push_units(body, k), 6), ('imm', 1))
        self.assertEqual(E.argument_at(E.push_units(body, k), 8), ('imm', 10))

    def test_capped_store_from_a_literal(self):
        world, body, k = synthetic([LIT(0x300) + LEA_LOCAL(0x29) + STRSTORE(0x28) + LEA_LOCAL(0x29) + TAIL(10, 0) + CALL16EC], {0x300: 60})
        self.assertEqual(bound_at(world, body, k)[1], 0x28)
        world, body, k = synthetic([LIT(0x300) + LEA_LOCAL(0x29) + STRSTORE(0x28) + LEA_LOCAL(0x29) + TAIL(10, 0) + CALL16EC], {0x300: 9})
        self.assertEqual(bound_at(world, body, k)[1], 9)

    def test_parameter_without_callers_is_unbounded_but_the_cap_still_binds(self):
        frag = PARAM(6) + LEA_LOCAL(0x29) + STRSTORE(0xFF) + LEA_LOCAL(0x29) + TAIL(0, 0) + CALL16EC
        world, body, k = synthetic([frag], {})
        self.assertEqual(bound_at(world, body, k)[1], E.UNBOUNDED)
        world, body, k = synthetic([frag], {}, params={6: 16})
        self.assertEqual(bound_at(world, body, k)[1], 16)
        frag = PARAM(6) + LEA_LOCAL(0x29) + STRSTORE(0x28) + LEA_LOCAL(0x29) + TAIL(0, 0) + CALL16EC
        world, body, k = synthetic([frag], {})
        self.assertEqual(bound_at(world, body, k)[1], 0x28)

    def test_concatenation_chain_adds_and_clamps(self):
        frag = LEA_LOCAL(0x36) + LIT(0x300) + STRLOAD + LIT(0x310) + STRCAT + TAIL(10, 1) + CALL16EC
        world, body, k = synthetic([frag], {0x300: 11, 0x310: 40})
        message, bound = bound_at(world, body, k)
        self.assertEqual(message[:3], ('temp', 0x36, 1))
        self.assertEqual(bound, 51)
        world, body, k = synthetic([frag], {0x300: 200, 0x310: 100})
        self.assertEqual(bound_at(world, body, k)[1], E.UNBOUNDED)

    def test_unknown_callee_taking_the_local_unbounds_it(self):
        frag = LIT(0x300) + LEA_LOCAL(0x29) + STRSTORE(0x28) + LEA_LOCAL(0x29) + OTHER_CALL + LEA_LOCAL(0x29) + TAIL(10, 0) + CALL16EC
        world, body, k = synthetic([frag], {0x300: 9})
        self.assertEqual(bound_at(world, body, k)[1], E.UNBOUNDED)

    def test_direct_length_byte_writes(self):
        store0 = bytes([0xC6, 0x46, 0xD7, 0x00])                                   # mov byte ptr [bp-0x29], 0
        frag = store0 + LEA_LOCAL(0x29) + TAIL(10, 0) + CALL16EC
        world, body, k = synthetic([frag], {})
        self.assertEqual(bound_at(world, body, k)[1], 0)
        word_write = bytes([0xC7, 0x46, 0xD6, 0x34, 0x12])                        # mov word ptr [bp-0x2a], 1234h covers [bp-0x29]
        frag = store0 + word_write + LEA_LOCAL(0x29) + TAIL(10, 0) + CALL16EC
        world, body, k = synthetic([frag], {})
        self.assertEqual(bound_at(world, body, k)[1], E.UNBOUNDED)

    def test_join_takes_the_maximum_over_paths(self):
        # cmp [bp-1],0 ; je +skip ; (store literal 300 cap 28h) ; skip: lea...; call
        skip = LIT(0x300) + LEA_LOCAL(0x29) + STRSTORE(0x28)
        frag = bytes([0xC6, 0x46, 0xD7, 0x03]) + bytes([0x80, 0x7E, 0xFF, 0x00, 0x74, len(skip)]) + skip + LEA_LOCAL(0x29) + TAIL(10, 0) + CALL16EC
        world, body, k = synthetic([frag], {0x300: 20})
        self.assertEqual(bound_at(world, body, k)[1], 20)      # max(3 on the skip path, 20 on the store path)

    def test_t3677_r1_indirect_length_overwrite_is_not_ignored(self):
        # Reviewer control: MOV byte [BP-29h],0 then LEA DI,[BP-29h]; MOV byte SS:[DI],FFh. The write through DI must
        # invalidate the bound; before the fix it stayed 0.
        store0, indirect_ff = bytes.fromhex('c646d700'), bytes.fromhex('8d7ed736c605ff')
        world, body, k = synthetic([store0 + indirect_ff + LEA_LOCAL(0x29) + TAIL(10, 0) + CALL16EC], {})
        self.assertEqual(bound_at(world, body, k)[1], E.UNBOUNDED)
        # The same write without the SS override (ES:[DI] with DI stack-derived) is invalidated too.
        indirect_es = bytes.fromhex('8d7ed726c605ff')
        world, body, k = synthetic([store0 + indirect_es + LEA_LOCAL(0x29) + TAIL(10, 0) + CALL16EC], {})
        self.assertEqual(bound_at(world, body, k)[1], E.UNBOUNDED)

    def test_t3677_r2_a_jump_into_the_argument_setup_is_a_boundary(self):
        # Reviewer control: the reachable path pushes the unknown parameter and jumps over a literal setup straight to
        # the StrLoad call. The literal must not be attributed; the entry is refused as unresolved.
        fake = LEA_LOCAL(0x29) + LIT(0x300)
        frag = LEA_LOCAL(0x29) + PARAM(6) + bytes([0xEB, len(fake)]) + fake + STRLOAD + TAIL(10, 0) + CALL16EC
        world, body, k = synthetic([frag], {0x300: 9})
        self.assertIn(body.index[len(PROLOGUE) + len(LEA_LOCAL(0x29)) + len(PARAM(6)) + 2 + len(fake)], body.targets)
        units = E.push_units(body, k)
        self.assertIsNone(E.argument_at(units, 10))
        state = E.Analysis(world, 'ovl', 0x38, body, 0).before(k)
        self.assertEqual(state[0].get(0x29, E.UNBOUNDED), E.UNBOUNDED)

    def test_taint_round_trips_and_safe_writes(self):
        # A stack pointer stored to a local, reloaded with LES and written through: invalidated.
        store_ptr = bytes.fromhex('8d7ed7') + bytes.fromhex('897ef8') + bytes.fromhex('8c56fa')   # lea di,[bp-29h]; mov [bp-8],di; mov [bp-6],ss
        reload = bytes.fromhex('c47ef8') + bytes.fromhex('26c60500')                               # les di,[bp-8]; mov byte es:[di],0
        frag = LIT(0x300) + LEA_LOCAL(0x29) + STRSTORE(0x28) + store_ptr + reload + LEA_LOCAL(0x29) + TAIL(10, 0) + CALL16EC
        world, body, k = synthetic([frag], {0x300: 9})
        self.assertEqual(bound_at(world, body, k)[1], E.UNBOUNDED)
        # PUSH SS; POP ES; write through ES: invalidated.
        push_ss_pop_es, store_es = bytes.fromhex('1607'), bytes.fromhex('26c606341200')               # push ss; pop es; mov byte es:[1234h],0
        frag = LIT(0x300) + LEA_LOCAL(0x29) + STRSTORE(0x28) + push_ss_pop_es + store_es + LEA_LOCAL(0x29) + TAIL(10, 0) + CALL16EC
        world, body, k = synthetic([frag], {0x300: 9})
        self.assertEqual(bound_at(world, body, k)[1], E.UNBOUNDED)
        # A DS-absolute store below the stack base keeps the bound; one that can reach the stack segment does not.
        frag = LIT(0x300) + LEA_LOCAL(0x29) + STRSTORE(0x28) + bytes.fromhex('a21868') + LEA_LOCAL(0x29) + TAIL(10, 0) + CALL16EC
        world, body, k = synthetic([frag], {0x300: 9})
        self.assertEqual(bound_at(world, body, k)[1], 9)                                  # mov [6818h],al
        frag = LIT(0x300) + LEA_LOCAL(0x29) + STRSTORE(0x28) + bytes.fromhex('a20090') + LEA_LOCAL(0x29) + TAIL(10, 0) + CALL16EC
        world, body, k = synthetic([frag], {0x300: 9})
        self.assertEqual(bound_at(world, body, k)[1], E.UNBOUNDED)                        # mov [9000h],al reaches SS
        # After a call, registers are treated as unknown: a write through ES:[DI] without a reload is invalidated.
        frag = LIT(0x300) + LEA_LOCAL(0x29) + STRSTORE(0x28) + OTHER_CALL + bytes.fromhex('26c60500') + LEA_LOCAL(0x29) + TAIL(10, 0) + CALL16EC
        world, body, k = synthetic([frag], {0x300: 9})
        self.assertEqual(bound_at(world, body, k)[1], E.UNBOUNDED)

    def test_t3683_r1_parameter_pointer_write_is_unknown(self):
        # Reviewer control: LES DI,[BP+6]; MOV byte ES:[DI],FFh. The parameter's contents may be SS:1FD7 for BP 2000h,
        # so the write may hit the local's length byte; syntactic origin proves nothing, and the bound must go.
        store0, effect = bytes.fromhex('c646d700'), bytes.fromhex('c47e0626c605ff')
        world, body, k = synthetic([store0 + effect + LEA_LOCAL(0x29) + TAIL(10, 0) + CALL16EC], {})
        self.assertEqual(bound_at(world, body, k)[1], E.UNBOUNDED)
        effect = bytes.fromhex('c47e0626c6850c0100')                                        # ... mov byte es:[di+10ch],0
        world, body, k = synthetic([store0 + effect + LEA_LOCAL(0x29) + TAIL(10, 0) + CALL16EC], {})
        self.assertEqual(bound_at(world, body, k)[1], E.UNBOUNDED)

    def test_t3683_r3_indirect_call_invalidates_every_local(self):
        # Reviewer control: CALL word ptr [0400h] after the store; the unknown callee may write the local.
        store0, effect = bytes.fromhex('c646d700'), bytes.fromhex('ff160004')
        world, body, k = synthetic([store0 + effect + LEA_LOCAL(0x29) + TAIL(10, 0) + CALL16EC], {})
        self.assertEqual(bound_at(world, body, k)[1], E.UNBOUNDED)

    def test_direct_call_after_a_taken_pointer_invalidates_reachable_locals(self):
        # The local's address was taken (for a string leaf) earlier; a later unrelated callee may hold that pointer.
        frag = LIT(0x300) + LEA_LOCAL(0x29) + STRSTORE(0x28) + OTHER_CALL + LEA_LOCAL(0x29) + TAIL(10, 0) + CALL16EC
        world, body, k = synthetic([frag], {0x300: 9})
        self.assertEqual(bound_at(world, body, k)[1], E.UNBOUNDED)
        # A local whose address was never taken, written by a direct byte store, survives an unrelated call.
        store7 = bytes([0xC6, 0x46, 0xD7, 0x07])
        frag = store7 + OTHER_CALL + LEA_LOCAL(0x29) + TAIL(10, 0) + CALL16EC
        world, body, k = synthetic([frag], {})
        self.assertEqual(bound_at(world, body, k)[1], 7)

    def test_body_cut_at_its_own_return_and_indirect_jump_refused(self):
        code = PROLOGUE + TAIL(1, 1) + EPILOGUE + b'\x05hello' + b'\xff' * 8
        body = E.Body(code, 0, len(code), 'cut')
        self.assertEqual(body.ins[-1].mnemonic, 'retf')
        with self.assertRaisesRegex(ValueError, 'indirect jump'):
            E.Body(PROLOGUE + bytes([0xFF, 0xE0]) + EPILOGUE, 0, len(PROLOGUE) + 2 + len(EPILOGUE), 'indirect')


@unittest.skipUnless((E.ROOT / 'resources/DOS/POOLRAD/START.EXE').is_file() and (E.ROOT / 'resources/DOS/POOLRAD/GAME.OVR').is_file(),
                     'owner media unavailable')
class OwnerMediaCensus(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.world = E.World()
        cls.report = E.check(cls.world)

    def test_pinned_entries_and_bounds(self):
        self.assertEqual((self.report['far_sites'], self.report['near_sites']), (63, 4))
        self.assertEqual(E.table(self.report['entries']), E.EXPECTED)
        self.assertEqual(self.report['unbounded'], E.EXPECTED_UNBOUNDED)
        self.assertEqual((self.report['bounded'], self.report['max_bound']), (64, 51))

    def test_pin_mutation_is_refused(self):
        mutated = list(E.EXPECTED)
        site, owner, kind, row, wait, cls, bound, title = mutated[0]
        mutated[0] = (site, owner, kind, row, wait, cls, bound + 1, title)
        with mock.patch.object(E, 'EXPECTED', mutated):
            with self.assertRaisesRegex(ValueError, 'pinned measurement'):
                E.check(self.world)

    def test_unbounded_set_change_is_refused(self):
        with mock.patch.object(E, 'EXPECTED_UNBOUNDED', E.EXPECTED_UNBOUNDED[:-1]):
            with self.assertRaisesRegex(ValueError, 'without a sound bound changed'):
                E.check(self.world)

    def test_census_count_change_is_refused(self):
        original = E.sites

        def fewer(exe, ovr, seg, off):
            return original(exe, ovr, seg, off)[1:]
        with mock.patch.object(E, 'sites', fewer):
            with self.assertRaisesRegex(ValueError, 'entry census changed'):
                E.check(self.world)

    def test_selector_invariant_rejects_a_low_stack_segment(self):
        header = E.parse_mz(self.world.exe)
        loader = header['header_bytes'] + 16 * header['cs']
        exe = bytearray(self.world.exe)
        low = E.DATA_SEG + 0x100
        exe[loader + 10:loader + 12] = low.to_bytes(2, 'little')
        fake = mock.Mock(exe=bytes(exe), image=self.world.image)
        with mock.patch.dict(E.EXEPACK, {'real_ss': low}):
            with self.assertRaisesRegex(ValueError, 'not below the stack segment'):
                E.selector_invariant(fake)
        with self.assertRaisesRegex(ValueError, 'EXEPACK header differs'):
            E.selector_invariant(fake)

    def test_selector_invariant_rejects_a_changed_body(self):
        image = bytearray(self.world.image)
        image[E.TARGET[0] * 16 + 0x1708] ^= 1
        fake = mock.Mock(exe=self.world.exe, image=bytes(image))
        with self.assertRaisesRegex(ValueError, 'body pin mismatch'):
            E.selector_invariant(fake)


if __name__ == '__main__':
    unittest.main()
