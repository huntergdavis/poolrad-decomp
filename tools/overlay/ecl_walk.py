#!/usr/bin/env python3
"""Static control-flow walk of the ECL bytecode blocks, used to verify the VM contract.

Reads ECL1..ECL8.DAX with a minimal DAX/RLE reader (format verified in docs/FABLE_AUDIT.md),
then for every block starts at the five entry GOTOs (opcode 0x01 at unpacked offsets 2, 6,
10, 14, 18) and follows control flow using:
  * the opcode operand counts of Gold Box Explorer's ECL interpreter table
    (bsimser/Gold-Box-Explorer, src/Common/Plugins/DaxEcl/Program.cs, rev eac30aba, MIT),
  * the tagged operand grammar of that interpreter (Memory.cs LoadEclOpps): 00 BB (byte),
    01/02/03 LO HI (word: address / immediate / indirect), 80 LL + LL packed text bytes
    (inline string), 81 LO HI (string by address); menus 0x15/0x2B and ON GOTO/GOSUB 0x25/0x26
    carry a fixed prefix whose last operand is the count of a following operand list,
  * the virtual address rule  file_offset = target - 0x9900 + 2.

The walk is conservative: it stops a path at any opcode or tag it cannot size and reports what fraction of each block was
covered and how many errors (unknown tag, jump outside the block, jump into the middle of
a decoded instruction) it hit. Counts and offsets only; no block content is printed.

A second mode, --sweep, decodes each block linearly from offset 22 (after the five entry jumps)
with the same grammar until the first undecodable byte, and reports how many bytes decoded,
the stop reason, and the opcode decoded just before the stop. Breaks that follow an
unconditional transfer (EXIT, GOTO, GOSUB, RETURN, NEWECL, PRINT RETURN) mark inline data;
breaks after other opcodes point at operand counts that need checking.

Usage: ecl_walk.py --dir resources/DOS/POOLRAD [--json out.json] [--sweep]
"""
import argparse, collections, json, os, struct

# Engine PREFETCH counts: transcribed from the operand-count dispatcher at GAME.OVR 0x9379
# (overlay unit 0x25 entry 22), see docs/re/FUNCTION_LEDGER.md. This is how many operands the
# engine loads before dispatching a handler; some handlers then read further operands
# themselves (0x0C, 0x34, 0x36 read one more), so this table is NOT the wire format and
# the linear sweep with it breaks early. Kept for comparison (--engine-prefetch-counts).
ENGINE_OPERANDS = {}
for _n, _ops in ((1, (0x01, 0x02, 0x0A, 0x0E, 0x11, 0x12, 0x1D, 0x20, 0x2D, 0x32, 0x34, 0x36, 0x38, 0x39, 0x3C)),
                 (2, (0x03, 0x08, 0x09, 0x0C, 0x0F, 0x10, 0x1F, 0x22)),
                 (3, (0x04, 0x05, 0x06, 0x07, 0x0B, 0x21, 0x28, 0x2A, 0x2F, 0x30, 0x35, 0x37, 0x3B)),
                 (4, (0x14, 0x23)), (5, (0x2E,)), (6, (0x1E, 0x2C)), (8, (0x27,)), (14, (0x29,))):
    for _o in _ops:
        ENGINE_OPERANDS[_o] = _n
for _o in range(0x3E):
    ENGINE_OPERANDS.setdefault(_o, 0)
# menus / jump tables: the handler reads a list after the fixed operands (reference reading)
ENGINE_OPERANDS[0x15] = 3
ENGINE_OPERANDS[0x25] = 2
ENGINE_OPERANDS[0x26] = 2
ENGINE_OPERANDS[0x2B] = 2

# Wire-format operand counts (Gold Box Explorer table; default). Verified on this build by the
# linear sweep: 61% of script bytes decode before the first inline-data region.
GBE_OPERANDS = {
    0x00: 0, 0x01: 1, 0x02: 1, 0x03: 2, 0x04: 3, 0x05: 3, 0x06: 3, 0x07: 3, 0x08: 2, 0x09: 2,
    0x0A: 1, 0x0B: 3, 0x0C: 3, 0x0D: 0, 0x0E: 1, 0x0F: 2, 0x10: 2, 0x11: 1, 0x12: 1, 0x13: 0,
    0x14: 4, 0x15: 3, 0x16: 0, 0x17: 0, 0x18: 0, 0x19: 0, 0x1A: 0, 0x1B: 0, 0x1C: 0, 0x1D: 1,
    0x1E: 6, 0x1F: 2, 0x20: 1, 0x21: 3, 0x22: 2, 0x23: 4, 0x24: 0, 0x25: 2, 0x26: 2,
    0x27: 8, 0x28: 3, 0x29: 14, 0x2A: 3, 0x2B: 2, 0x2C: 6, 0x2D: 1, 0x2E: 5, 0x2F: 3,
    0x30: 3, 0x31: 0, 0x32: 1, 0x33: 0, 0x34: 2, 0x35: 3, 0x36: 2, 0x37: 3, 0x38: 1, 0x39: 1,
    0x3A: 0, 0x3B: 3, 0x3C: 1, 0x3D: 0,
}
COUNTED = {0x15: 2, 0x2B: 1, 0x25: 1, 0x26: 1}   # operand index that holds the item count
TERMINATORS = {0x00, 0x13, 0x20, 0x33}      # EXIT, RETURN, NEWECL, PRINT RETURN
JUMPS = {0x01: "goto", 0x02: "gosub", 0x2D: "call"}
BASE = 0x9900


def read_dax(path):
    b = open(path, "rb").read()
    n = struct.unpack_from("<H", b, 0)[0] // 9
    data = b[2 + 9 * n:]
    out = []
    for i in range(n):
        bid, off, unp, pk = struct.unpack_from("<BIHH", b, 2 + 9 * i)
        src = data[off:off + pk]
        o = bytearray(); j = 0
        while j < len(src):
            c = src[j]; j += 1
            if c < 0x80:
                o += src[j:j + c + 1]; j += c + 1
            else:
                o += bytes([src[j]]) * (256 - c); j += 1
        if len(o) != unp:
            raise SystemExit(f"{path} block {bid}: RLE length mismatch")
        out.append((bid, bytes(o)))
    return out


OPERANDS = GBE_OPERANDS


def operand_len(d, p):
    """Return (length, kind, value) of the tagged operand at p, or (None, reason, None)."""
    if p >= len(d):
        return None, "eof", None
    t = d[p]
    if t == 0x00:
        return 2, "byte", d[p + 1] if p + 1 < len(d) else None
    if t in (0x01, 0x02, 0x03):          # word: 01 = memory address, 02 = immediate, 03 = indirect
        return 3, "word", struct.unpack_from("<H", d, p + 1)[0] if p + 3 <= len(d) else None
    if t == 0x80:                         # inline packed string: length byte then bytes
        if p + 1 >= len(d):
            return None, "eof", None
        return 2 + d[p + 1], "string", d[p + 1]
    if t == 0x81:                         # string by memory address (word)
        return 3, "strref", struct.unpack_from("<H", d, p + 1)[0] if p + 3 <= len(d) else None
    return None, f"tag_{t:02x}", None


def walk(d):
    starts = []
    for o in (2, 6, 10, 14, 18):
        if d[o] == 0x01 and d[o + 1] == 0x01:
            starts.append(struct.unpack_from("<H", d, o + 2)[0] - BASE + 2)
    seen = {}
    errors = collections.Counter()
    work = list(starts)
    unknown_ops = collections.Counter()
    while work:
        p = work.pop()
        while True:
            if p in seen:
                break
            if not (0 <= p < len(d)):
                errors["jump_outside_block"] += 1
                break
            op = d[p]
            n = OPERANDS.get(op)
            if n is None:
                unknown_ops[op] += 1
                seen[p] = (op, 1)
                break
            q = p + 1
            ok = True
            vals = []
            for _ in range(n):
                L, kind, val = operand_len(d, q)
                if L is None:
                    errors[kind] += 1
                    ok = False
                    break
                vals.append((kind, val))
                q += L
            items = []
            if ok and op in COUNTED:
                cnt = vals[COUNTED[op]][1] or 0
                for _ in range(cnt):
                    L, kind, val = operand_len(d, q)
                    if L is None:
                        errors[kind] += 1
                        ok = False
                        break
                    items.append((kind, val))
                    q += L
            if not ok:
                seen[p] = (op, 1)
                break
            seen[p] = (op, q - p)
            if op in (0x25, 0x26):
                for kind, val in items:
                    if kind == "word" and val is not None:
                        t = val - BASE + 2
                        if 0 <= t < len(d):
                            work.append(t)
                        else:
                            errors["jump_outside_block"] += 1
            if op in JUMPS:
                if vals and vals[0][0] == "word":
                    t = vals[0][1] - BASE + 2
                    if 0 <= t < len(d):
                        work.append(t)
                    else:
                        errors["jump_outside_block"] += 1
                if op == 0x01:
                    break
            if op in TERMINATORS:
                break
            p = q
    # overlap check: a decoded instruction starting inside another decoded instruction
    covered = bytearray(len(d))
    overlaps = 0
    for p, (op, L) in seen.items():
        if any(covered[p:p + L]):
            overlaps += 1
        for i in range(p, min(p + L, len(d))):
            covered[i] = 1
    return dict(entry_targets=starts, instructions=len(seen), covered_bytes=int(sum(covered)),
                size=len(d), errors=dict(errors), overlaps=overlaps,
                unknown_layout_ops={f"0x{k:02x}": v for k, v in unknown_ops.items()},
                opcode_histogram={f"0x{k:02x}": v for k, v in collections.Counter(op for op, _ in seen.values()).items()})


def decode_one(d, p):
    """Decode the instruction at p. Return (end, vals, items) or (None, reason, None)."""
    op = d[p]
    n = OPERANDS.get(op)
    if n is None:
        return None, f"op_{op:02x}", None
    q = p + 1
    vals = []
    for _ in range(n):
        L, kind, val = operand_len(d, q)
        if L is None:
            return None, kind, None
        vals.append((kind, val))
        q += L
    items = []
    if op in COUNTED:
        cnt = vals[COUNTED[op]][1] or 0
        for _ in range(cnt):
            L, kind, val = operand_len(d, q)
            if L is None:
                return None, kind, None
            items.append((kind, val))
            q += L
    return q, vals, items


def sweep(d, start=22):
    """Linear decode from start until the first error."""
    p = start
    last = None
    reason = "end_of_block"
    while p < len(d):
        end, info, _ = decode_one(d, p)
        if end is None:
            reason = info
            break
        last = d[p]
        p = end
    return dict(start=start, decoded_bytes=p - start, body_bytes=len(d) - start, stop_offset=p,
                stop_reason=reason, last_opcode=(f"0x{last:02x}" if last is not None else None),
                after_transfer=(last in TERMINATORS or last in (0x01, 0x02)) if last is not None else None)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--dir", required=True)
    ap.add_argument("--json")
    ap.add_argument("--sweep", action="store_true", help="linear sweep report instead of the control-flow walk")
    ap.add_argument("--engine-prefetch-counts", action="store_true", help="use the engine's prefetch table (unit 0x25 entry 22) instead of the wire-format table")
    a = ap.parse_args()
    global OPERANDS
    OPERANDS = ENGINE_OPERANDS if a.engine_prefetch_counts else GBE_OPERANDS
    res = {}
    if a.sweep:
        tot_dec = tot_body = 0
        after = 0
        for n in range(1, 9):
            for bid, d in read_dax(os.path.join(a.dir, f"ECL{n}.DAX")):
                r = sweep(d)
                res[f"ECL{n}#{bid}"] = r
                tot_dec += r["decoded_bytes"]; tot_body += r["body_bytes"]; after += bool(r["after_transfer"])
                print(f"ECL{n}#{bid:<3d} decoded={r['decoded_bytes']:5d}/{r['body_bytes']:5d} ({r['decoded_bytes']/r['body_bytes']:4.0%}) "
                      f"stop={r['stop_reason']:<12s} at {r['stop_offset']:#06x} after_opcode={r['last_opcode']} transfer={r['after_transfer']}")
        print(f"TOTAL linear-decodable before first error: {tot_dec}/{tot_body} ({tot_dec/tot_body:.1%}); "
              f"blocks whose first break follows an unconditional transfer: {after}/{len(res)}")
        if a.json:
            json.dump(res, open(a.json, "w"), indent=1)
        return
    tot_ins = tot_cov = tot_size = tot_err = 0
    for n in range(1, 9):
        for bid, d in read_dax(os.path.join(a.dir, f"ECL{n}.DAX")):
            r = walk(d)
            res[f"ECL{n}#{bid}"] = r
            tot_ins += r["instructions"]; tot_cov += r["covered_bytes"]; tot_size += r["size"]
            tot_err += sum(r["errors"].values()) + r["overlaps"]
            print(f"ECL{n}#{bid:<3d} size={r['size']:5d} insns={r['instructions']:4d} covered={r['covered_bytes']/r['size']:.0%} "
                  f"errors={sum(r['errors'].values())} overlaps={r['overlaps']} stopped_at_variable_ops={sum(r['unknown_layout_ops'].values())}")
    print(f"TOTAL blocks={len(res)} insns={tot_ins} covered={tot_cov}/{tot_size} ({tot_cov/tot_size:.1%}) errors+overlaps={tot_err}")
    if a.json:
        json.dump(res, open(a.json, "w"), indent=1)


if __name__ == "__main__":
    main()
