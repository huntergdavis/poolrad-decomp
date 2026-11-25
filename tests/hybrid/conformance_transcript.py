"""Python side of the reader conformance: the same scenarios as tools/hybrid/dosbox-x/conformance_main.cpp.

Synthetic fixtures only. The transcript format is "kind|key|reason" per poll, with "== name" headers.
"""
from tools.hybrid import codeword_assist as ca

CHILD_BP, FRAME_BP, UNIT_BP, UNIT_SEG = 0x3A72, 0x3DA4, 0x3EF8, 0x38D3


def synthetic_text(i):
    return "".join(chr(65 + ((i * 7 + k * 3) % 26)) for k in range(ca.RECORD_TEXT_LEN))


def build_table(ok=True):
    table = bytearray(ca.TABLE_BYTES)
    for i in range(ca.RECORD_COUNT):
        slot = ca.record_slot(i)
        table[slot] = ca.RECORD_TEXT_LEN
        table[slot + 1:slot + 1 + ca.RECORD_TEXT_LEN] = synthetic_text(i).encode("ascii")
        for k in range(14):
            table[slot + 1 + ca.RECORD_TEXT_LEN + k] = (i * 11 + k) & 0xFF
    if not ok:
        table[ca.record_slot(5) + 1] ^= 1
    return bytes(table)


SYNTH_SHA = ca.table_digest(build_table(True))


class Prompt:
    def __init__(self, index, attempt, rows, table_ok, break_after, key_buffers, max_steps, nonempty_polls):
        self.index, self.attempt, self.rows, self.table_ok = index, attempt, rows, table_ok
        self.break_after, self.key_buffers, self.max_steps, self.nonempty_polls = break_after, key_buffers, max_steps, nonempty_polls


def frame_words(ret_ip):
    ss_word = {CHILD_BP: FRAME_BP, CHILD_BP + 2: 0x0A30, CHILD_BP + 4: 0x0F2D,
               FRAME_BP: UNIT_BP, FRAME_BP + 2: ret_ip, FRAME_BP + 4: UNIT_SEG,
               UNIT_BP: 0, UNIT_BP + 2: 0, UNIT_BP + 4: 0}
    return ss_word


def snapshot(head, tail, ret_ip, index=None, attempt=None, rows=True, table=None, sites=None):
    ss_byte = {}
    if index is not None:
        ss_byte = {UNIT_BP - ca.INDEX_OFFSET: index, UNIT_BP - ca.ATTEMPT_OFFSET: attempt}
    if sites is None:
        sites = frozenset(ca.FIXED_ROW_CALLS) if rows else frozenset(list(ca.FIXED_ROW_CALLS)[:5])
    return ca.Snapshot(cs=0xF000, ip=0xCFC1, bp=CHILD_BP, ss_word=frame_words(ret_ip), ss_byte=ss_byte,
                       ds_table=table, bda_head=head, bda_tail=tail, printer_sites=sites, keyboard_poll=True)


def line(action):
    return "%s|%s|%s" % (action.kind, action.key or "", action.reason)


def run_nonprompt(gate, n, out):
    for _ in range(n):
        out.append(line(gate.step(snapshot(0x1E, 0x1E, 0x0355, table=build_table(True)))))


# Independent of ca.FIXED_ROW_CALLS: the six 5-byte lcall sites in the codeword source note (return = site + 5).
INDEPENDENT_CALL_SITES = (0x0157, 0x0174, 0x0191, 0x0202, 0x0286, 0x02D8)


def run_prompt(gate, p, out, independent_rows=False):
    head, tail = 0x1E, 0x1E
    sends = 0
    broken = False
    for step in range(p.max_steps):
        if step < p.nonempty_polls:
            head, tail = 0x1E, 0x20
        elif step == p.nonempty_polls and p.nonempty_polls > 0:
            head = tail
        ret = 0x0355 if broken else ca.UNIT_RET_IP
        sites = frozenset(site + 5 for site in INDEPENDENT_CALL_SITES) if independent_rows else None
        snap = snapshot(head, tail, ret, index=p.index, attempt=p.attempt, rows=p.rows,
                        table=build_table(p.table_ok), sites=sites)
        action = gate.step(snap)
        out.append(line(action))
        if action.kind == "SEND":
            sends += 1
            if p.key_buffers:
                tail = (tail + 2) & 0xFFFF
            if p.break_after > 0 and sends == p.break_after:
                broken = True
        elif action.kind == "WAIT":
            head = tail
        if action.reason == "completed" or action.reason.startswith("abort:"):
            break


def transcript():
    out = ["digest %s" % SYNTH_SHA]
    out.append("== complete-all")
    gate = ca.Gate(enabled=True, expected_sha256=SYNTH_SHA)
    for i in range(ca.RECORD_COUNT):
        run_prompt(gate, Prompt(i, 1, True, True, 0, True, 80, 0), out)
    out.append("== repeat-same")
    gate = ca.Gate(enabled=True, expected_sha256=SYNTH_SHA)
    run_prompt(gate, Prompt(4, 1, True, True, 0, True, 80, 0), out)
    run_prompt(gate, Prompt(4, 1, True, True, 0, True, 3, 0), out)
    out.append("== attempt-two")
    gate = ca.Gate(enabled=True, expected_sha256=SYNTH_SHA)
    run_prompt(gate, Prompt(4, 1, True, True, 0, True, 80, 0), out)
    run_prompt(gate, Prompt(7, 2, True, True, 0, True, 80, 0), out)
    out.append("== guard-break")
    gate = ca.Gate(enabled=True, expected_sha256=SYNTH_SHA)
    run_prompt(gate, Prompt(2, 1, True, True, 2, True, 80, 0), out)
    run_prompt(gate, Prompt(2, 1, True, True, 0, True, 3, 0), out)
    out.append("== never-buffers")
    gate = ca.Gate(enabled=True, expected_sha256=SYNTH_SHA)
    run_prompt(gate, Prompt(6, 1, True, True, 0, False, 260, 0), out)
    out.append("== buffer-not-empty")
    gate = ca.Gate(enabled=True, expected_sha256=SYNTH_SHA)
    run_prompt(gate, Prompt(3, 1, True, True, 0, True, 80, 3), out)
    out.append("== fixed-rows-missing")
    gate = ca.Gate(enabled=True, expected_sha256=SYNTH_SHA)
    run_prompt(gate, Prompt(5, 1, False, True, 0, True, 3, 0), out)
    out.append("== table-digest")
    gate = ca.Gate(enabled=True, expected_sha256=SYNTH_SHA)
    run_prompt(gate, Prompt(5, 1, True, False, 0, True, 2, 0), out)
    out.append("== assist-off")
    gate = ca.Gate(enabled=False, expected_sha256=SYNTH_SHA)
    run_prompt(gate, Prompt(5, 1, True, True, 0, True, 3, 0), out)
    out.append("== independent-rows")
    gate = ca.Gate(enabled=True, expected_sha256=SYNTH_SHA)
    run_prompt(gate, Prompt(10, 1, True, True, 0, True, 80, 0), out, independent_rows=True)
    out.append("== nonprompt-then-prompt")
    gate = ca.Gate(enabled=True, expected_sha256=SYNTH_SHA)
    run_nonprompt(gate, 5, out)
    run_prompt(gate, Prompt(9, 1, True, True, 0, True, 80, 0), out)
    return "\n".join(out) + "\n"


if __name__ == "__main__":
    import sys
    sys.stdout.write(transcript())
