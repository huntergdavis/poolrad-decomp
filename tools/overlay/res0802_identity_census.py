#!/usr/bin/env python3
"""Factor-1 census for the nine t30 side (i) bodies: every transfer into each entry, segment-wide, asserted.

Counts raw E8/E9 transfers inside the 0802 text segment (decoded to the containing known span), relocated 9A/EA far
patterns in the resident image, and overlay far fixups. Reads the ignored media; prints the table the identity
document quotes. Proposal text only: nothing here credits or registers a row.
"""
# NOT_A_ROW_CHECKER: census of caller sites for the t30 identity document; it registers and credits nothing
import argparse
import json
import struct
import sys
from hashlib import sha256
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(Path(__file__).resolve().parent))
from overlay_inventory import find_units, parse_mz  # noqa: E402
from resident_loader_map import unpack  # noqa: E402
from u86_procvars_3413_audit import EXE_SHA, OVR_SHA  # noqa: E402

SEG = 0x802
SPANS = {0x0000: 0x0020, 0x0020: 0x0040, 0x0040: 0x0105, 0x0105: 0x0175, 0x0175: 0x0193, 0x0193: 0x01ED, 0x0213: 0x025D, 0x025D: 0x02EB,
         0x02EB: 0x0356, 0x050F: 0x05CB, 0x05CD: 0x06E7, 0x06E7: 0x070C, 0x0716: 0x094A}
NINE = (0x0716, 0x05CD, 0x0040, 0x050F, 0x025D, 0x0105, 0x02EB, 0x0193, 0x06E7)
# The census is asserted: target -> (near sites as (site, containing entry), overlay far sites, loader far sites).
# 0213 is the unmodelled resident file-wait loop that also calls 02EB; nothing calls it.
EXPECTED = {
    0x0716: ([], 8, 2),
    0x05CD: ([(0x0775, 0x0716)], 0, 0),
    0x0040: ([(0x015F, 0x0105), (0x018C, 0x0175)], 0, 0),
    0x050F: ([(0x0935, 0x0716)], 0, 0),
    0x025D: ([(0x0659, 0x05CD)], 5, 0),
    0x0105: ([(0x01CC, 0x0193)], 0, 0),
    0x02EB: ([(0x0228, 0x0213), (0x028F, 0x025D)], 15, 0),
    0x0193: ([(0x02AA, 0x025D), (0x084A, 0x0716)], 0, 0),
    0x06E7: ([(0x08AD, 0x0716), (0x091D, 0x0716)], 0, 0),
    0x0213: ([], 0, 0),
}


def containing(site):
    for lo, hi in SPANS.items():
        if lo <= site < hi:
            return lo
    return None


def census(exe, ovr):
    if sha256(exe).hexdigest() != EXE_SHA or sha256(ovr).hexdigest() != OVR_SHA:
        raise SystemExit("media provenance mismatch")
    image = bytes(unpack(exe)[0])
    seg = image[SEG * 16:SEG * 16 + 0x10000]
    units = find_units(exe[parse_mz(exe)["header_bytes"]:], ovr)
    out = {}
    for entry in NINE + (0x0213,):
        near = [(o, containing(o)) for o in range(len(seg) - 2)
                if seg[o] in (0xE8, 0xE9) and (o + 3 + struct.unpack_from("<h", seg, o + 1)[0]) & 0xFFFF == entry]
        pattern = struct.pack("<HH", entry, SEG)
        far = [i for i in range(1, len(image) - 3) if image[i:i + 4] == pattern and image[i - 1] in (0x9A, 0xEA)]
        overlay = sum(1 for u in units for f in u["fixups"]
                      if ovr[u["ovr_offset"] + f - 3:u["ovr_offset"] + f - 2] == b"\x9a"
                      and struct.unpack_from("<HH", ovr, u["ovr_offset"] + f - 2) == (entry, SEG))
        out[entry] = dict(near=near, loader_far=len(far), overlay_far=overlay)
    return out


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--output")
    args = parser.parse_args()
    data = census((ROOT / "resources/DOS/POOLRAD/START.EXE").read_bytes(), (ROOT / "resources/DOS/POOLRAD/GAME.OVR").read_bytes())
    rows = {"%04X" % k: dict(near=["%04X in %s" % (s, "%04X" % c if c is not None else "UNKNOWN") for s, c in v["near"]],
                              loader_far=v["loader_far"], overlay_far=v["overlay_far"]) for k, v in data.items()}
    for key, row in rows.items():
        print(key, row)
    for entry, (near, overlay, loader) in EXPECTED.items():
        got = data[entry]
        if got["near"] != near or got["overlay_far"] != overlay or got["loader_far"] != loader:
            raise SystemExit("census for %04X changed: %s" % (entry, got))
    unknown = [k for k, v in data.items() for s, c in v["near"] if c is None]
    if unknown:
        raise SystemExit("a near transfer lies outside the known spans: %s" % unknown)
    from hashlib import sha256 as h
    image = bytes(unpack((ROOT / "resources/DOS/POOLRAD/START.EXE").read_bytes())[0])
    for entry in NINE:
        digest = h(image[SEG * 16 + entry:SEG * 16 + SPANS[entry]]).hexdigest()
        text = (ROOT / ("docs/re/RES_0802_%04X_CPU.md" % entry)).read_text()
        if digest not in text:
            raise SystemExit("body hash of %04X is not in its evidence document" % entry)
        print("%04X span %d B sha256 %s" % (entry, SPANS[entry] - entry, digest))
    if args.output:
        Path(args.output).write_text(json.dumps(rows, indent=1))


if __name__ == "__main__":
    main()
