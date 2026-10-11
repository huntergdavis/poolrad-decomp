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

Usage: ecl_walk.py --dir resources/DOS/POOLRAD [--json out.json]
"""
import argparse, collections, json, os, struct

# opcode -> fixed operand count (Gold Box Explorer table). None = variable / unknown layout.
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
            n = GBE_OPERANDS.get(op)
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


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--dir", required=True)
    ap.add_argument("--json")
    a = ap.parse_args()
    res = {}
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
