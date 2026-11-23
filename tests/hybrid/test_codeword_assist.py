"""Synthetic fixtures for the code-word reader. No private record text: every word here is generated below."""
import ast
import unittest
from pathlib import Path

from tools.hybrid import codeword_assist as ca

SEG = 0x38D3           # synthetic unit segment
CHILD_BP, FRAME_BP, UNIT_BP = 0x3A72, 0x3DA4, 0x3EF8


def synthetic_text(i):
    return "".join(chr(65 + ((i * 7 + k * 3) % 26)) for k in range(ca.RECORD_TEXT_LEN))


def synthetic_tail(i):
    return bytes((i * 11 + k) & 0xFF for k in range(14))


def synthetic_table():
    table = bytearray(ca.TABLE_BYTES)
    for i in range(ca.RECORD_COUNT):
        slot = ca.record_slot(i)
        table[slot] = ca.RECORD_TEXT_LEN
        table[slot + 1:slot + 1 + ca.RECORD_TEXT_LEN] = synthetic_text(i).encode("ascii")
        table[slot + 1 + ca.RECORD_TEXT_LEN:slot + ca.RECORD_STRIDE] = synthetic_tail(i)
    return bytes(table)


TABLE = synthetic_table()
DIGEST = ca.table_digest(TABLE)


def snapshot(index=3, attempt=1, ret_ip=ca.UNIT_RET_IP, second_input=False, table=TABLE, head=0x1E, tail=0x1E,
             sites=ca.FIXED_ROW_CALLS, poll=True, child_ret_cs=ca.RES_0709_SEG):
    ss_word = {
        CHILD_BP: FRAME_BP, CHILD_BP + 2: 0x0A30, CHILD_BP + 4: child_ret_cs,
        FRAME_BP: UNIT_BP, FRAME_BP + 2: ret_ip, FRAME_BP + 4: SEG,
        UNIT_BP: 0x0000, UNIT_BP + 2: 0x0000, UNIT_BP + 4: 0x0000,
    }
    ss_byte = {UNIT_BP - ca.INDEX_OFFSET: index, UNIT_BP - ca.ATTEMPT_OFFSET: attempt}
    if second_input:
        ss_word[UNIT_BP] = 0x3F00
        ss_word[0x3F00] = 0x0000
        ss_word[0x3F00 + 2] = ca.UNIT_RET_IP
        ss_word[0x3F00 + 4] = SEG
    return ca.Snapshot(cs=0xF000, ip=0xCFC1, bp=CHILD_BP, ss_word=ss_word, ss_byte=ss_byte, ds_table=table,
                       bda_head=head, bda_tail=tail, printer_sites=frozenset(sites), keyboard_poll=poll)


def run_to_completion(gate, index=3, attempt=1, limit=400):
    """Simulated keyboard: a sent key is buffered at once; the game reads it when the gate waits."""
    state = {"head": 0x1E, "tail": 0x1E}
    sent, reasons = [], []
    for _ in range(limit):
        snap = snapshot(index=index, attempt=attempt, head=state["head"], tail=state["tail"])
        action = gate.step(snap)
        reasons.append(action.reason)
        if action.kind == "SEND":
            sent.append(action.key)
            state["tail"] += 2
        elif action.kind == "WAIT":
            state["head"] = state["tail"]
        elif action.reason == "completed":
            break
    return sent, reasons


class SyntheticFixtures(unittest.TestCase):
    def test_thirteen_records_are_well_formed_and_distinct(self):
        words = [ca.record_text(TABLE, i) for i in range(ca.RECORD_COUNT)]
        self.assertEqual(len(words), 13)
        self.assertTrue(all(w is not None and len(w) == 6 for w in words))
        self.assertEqual(len(set(words)), 13)

    def test_every_index_reads_its_own_synthetic_record(self):
        for i in range(ca.RECORD_COUNT):
            prompt = ca.recognise(snapshot(index=i), DIGEST)
            self.assertIsInstance(prompt, ca.Prompt, msg="index %d" % i)
            self.assertEqual(prompt.index, i)
            self.assertEqual(prompt.text, synthetic_text(i))

    def test_record_slot_layout_is_two_plus_stride(self):
        self.assertEqual(ca.record_slot(0), 2)
        self.assertEqual(ca.record_slot(12), 2 + 21 * 12)
        self.assertEqual(ca.RECORD_STRIDE, 1 + ca.RECORD_TEXT_LEN + 14)


class GuardRefusals(unittest.TestCase):
    def assertSilent(self, snap, reason, digest=DIGEST):
        result = ca.recognise(snap, digest)
        self.assertIsInstance(result, str)
        self.assertEqual(result, reason)

    def test_not_keyboard_poll_is_silent(self):
        self.assertSilent(snapshot(poll=False), "not-keyboard-poll")

    def test_index_out_of_range_is_silent(self):
        self.assertSilent(snapshot(index=13), "index-out-of-range")
        self.assertSilent(snapshot(index=255), "index-out-of-range")

    def test_attempt_out_of_range_is_silent(self):
        self.assertSilent(snapshot(attempt=0), "attempt-out-of-range")
        self.assertSilent(snapshot(attempt=4), "attempt-out-of-range")

    def test_wrong_table_digest_is_silent(self):
        other = bytearray(TABLE)
        other[ca.record_slot(3) + 1] ^= 1
        self.assertSilent(snapshot(table=bytes(other)), "table-digest-mismatch")

    def test_no_input_return_frame_is_silent(self):
        self.assertSilent(snapshot(ret_ip=0x0355), "no-unique-input-return")

    def test_two_input_return_frames_is_silent(self):
        self.assertSilent(snapshot(second_input=True), "no-unique-input-return")

    def test_wrong_resident_caller_is_silent(self):
        self.assertSilent(snapshot(child_ret_cs=0x0F00), "input-routine-chain-mismatch")

    def test_fixed_rows_not_printed_is_silent(self):
        missing = set(ca.FIXED_ROW_CALLS) - {0x0157}
        self.assertSilent(snapshot(sites=missing), "fixed-rows-not-printed")

    def test_missing_table_is_silent(self):
        self.assertSilent(snapshot(table=None), "table-unavailable")

    def test_non_letter_record_is_silent(self):
        bad = bytearray(TABLE)
        bad[ca.record_slot(5) + 2] = ord("1")
        self.assertSilent(snapshot(index=5, table=bytes(bad)), "record-malformed", digest=ca.table_digest(bytes(bad)))


class GateBehaviour(unittest.TestCase):
    def test_assist_off_is_silent_for_a_recognised_prompt(self):
        gate = ca.Gate(enabled=False, expected_sha256=DIGEST)
        self.assertEqual(gate.step(snapshot()).reason, "assist-off")
        self.assertEqual(gate.step(snapshot()).kind, "SILENT")

    def test_one_plan_per_occurrence_letters_then_return(self):
        gate = ca.Gate(enabled=True, expected_sha256=DIGEST)
        sent, _ = run_to_completion(gate, index=4)
        self.assertEqual(sent, list(synthetic_text(4)) + [ca.RETURN])
        again = gate.step(snapshot(index=4, head=0x1E, tail=0x1E))
        self.assertEqual((again.kind, again.reason), ("SILENT", "occurrence-already-handled"))

    def test_new_attempt_is_a_new_occurrence(self):
        gate = ca.Gate(enabled=True, expected_sha256=DIGEST)
        first, _ = run_to_completion(gate, index=4, attempt=1)
        second, _ = run_to_completion(gate, index=7, attempt=2)
        self.assertEqual(first, list(synthetic_text(4)) + [ca.RETURN])
        self.assertEqual(second, list(synthetic_text(7)) + [ca.RETURN])

    def test_a_non_empty_buffer_is_waited_for_not_typed_over(self):
        gate = ca.Gate(enabled=True, expected_sha256=DIGEST)
        action = gate.step(snapshot(head=0x1E, tail=0x20))
        self.assertEqual((action.kind, action.reason), ("WAIT", "buffer-not-empty"))
        self.assertIsNone(gate.plan)

    def test_guard_break_mid_plan_aborts_and_never_resends(self):
        gate = ca.Gate(enabled=True, expected_sha256=DIGEST)
        state = {"head": 0x1E, "tail": 0x1E}
        sent = []
        for step in range(50):
            ret = 0x0355 if len(sent) == 2 else ca.UNIT_RET_IP
            snap = snapshot(index=2, ret_ip=ret, head=state["head"], tail=state["tail"])
            action = gate.step(snap)
            if action.kind == "SEND":
                sent.append(action.key)
                state["tail"] += 2
            elif action.kind == "WAIT":
                state["head"] = state["tail"]
            elif action.kind == "SILENT" and action.reason.startswith("abort:"):
                break
        self.assertEqual(len(sent), 2)
        self.assertIsNone(gate.plan)
        self.assertEqual(len(gate.aborted), 1)
        for _ in range(20):
            action = gate.step(snapshot(index=2, head=state["head"], tail=state["tail"]))
            self.assertNotEqual(action.kind, "SEND")

    def test_keys_that_never_buffer_abort_without_resend(self):
        gate = ca.Gate(enabled=True, expected_sha256=DIGEST)
        first = gate.step(snapshot(index=6, head=0x1E, tail=0x1E))
        self.assertEqual(first.kind, "SEND")
        reasons = []
        for _ in range(ca.MAX_WAIT_STEPS + 5):
            a = gate.step(snapshot(index=6, head=0x1E, tail=0x1E))
            reasons.append((a.kind, a.reason))
            self.assertNotEqual(a.kind, "SEND")
        self.assertIn(("SILENT", "abort:key-not-buffered"), reasons)

    def test_keyboard_map_covers_every_letter_and_return(self):
        for ch in map(chr, range(65, 91)):
            self.assertIn(ch, ca.MAKE_CODES)
        self.assertEqual(ca.MAKE_CODES[ca.RETURN], 0x1C)


class StaticBoundary(unittest.TestCase):
    FORBIDDEN_IMPORTS = {"subprocess", "pytesseract", "PIL", "pyautogui", "pynput", "ctypes"}

    def test_no_host_typing_ocr_or_memory_write_import(self):
        source = Path(ca.__file__).read_text()
        imported = set()
        for node in ast.walk(ast.parse(source)):
            if isinstance(node, ast.Import):
                imported.update(a.name.split(".")[0] for a in node.names)
            elif isinstance(node, ast.ImportFrom) and node.module:
                imported.add(node.module.split(".")[0])
        self.assertEqual(imported & self.FORBIDDEN_IMPORTS, set())
        self.assertNotIn("xdotool", source.replace("#", ""))


if __name__ == "__main__":
    unittest.main()
