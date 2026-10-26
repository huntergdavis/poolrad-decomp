#!/usr/bin/env python3
"""Aperture census of the recovered C models (VP i20701, i20712).

A model that dereferences a pointer it was handed can reach A0000..AFFFF when the pointer is corrupt. Whether the model reads
what the MACHINE reads there depends on how it dereferences: through a `pr_bus` (the EGA planes and latches) or through flat RAM.
For a FLAT model every existing audit's Unicorn oracle maps A0000.. as ordinary RAM, so model and oracle agree about an aperture
pointer BY CONSTRUCTION: a green differential on a flat model is NOT evidence that it is aperture-safe, and this census says so
in a machine-readable column (`aperture_observable` = no) beside coverage rather than leaving the absence of evidence to read as a pass.

Outputs (written with --write, checked by default and by the test; a new model source without a row fails closed):
  docs/re/aperture_observability.csv   one row per recovered model source: kind (bus | flat), aperture_observable, declared_exclusion
  docs/re/aperture_pairs.csv           every (bus-aware caller, FLAT callee) pair: the 0411 shape, with the pointer's source class
"""
# NOT_A_ROW_CHECKER: static census of C sources and headers; it checks no registered row
import csv
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SRC, INC = ROOT / "src/recovered", ROOT / "include/poolrad/recovered"
OBSERVABILITY = ROOT / "docs/re/aperture_observability.csv"
PAIRS = ROOT / "docs/re/aperture_pairs.csv"
DECLARES = re.compile(r"A0000|AFFFF|aperture", re.I)
# Callee names whose arguments are data pointers it dereferences (the rest take values or only touch module-global state).
POINTER_CALLEES = re.compile(r"strload|strstore|strcopy|strcat|strcmp|charstr|_str_|val_|move|fillchar|set_in|set_expand|getmem|freemem|"
                             r"load_operands|load_word|lead_spaces|trail_spaces|leaf_0711|entry1_0000|step_scan|0b57|clear_box|1037|0714")


# The pointer's SOURCE for every (caller, pointer-taking flat callee) pair: only a caller ARGUMENT, a GLOBAL (e.g. DS:5D92) or a pointer the
# callee RETURNS can be corrupt; a body-built LOCAL or a CODE LITERAL (a string in the caller's own segment) cannot land in the aperture
# except through stack aliasing, which the headers exclude. `read` = the call sites were read; `header` = inferred from the header's
# argument description and declaration only. A pair with no entry here fails the census (so a new pair must be classified).
SOURCES = {
    "resident_00ba_window_16ec_cpu.c": ("caller-argument+local", "read", "StrStore copies the caller string at BP+0A/+0C into the local BP-100h buffer; own stack and DS accesses are flat, aperture aliases excluded by contract"),
    "overlay_u1c_clear_box_2b14.c": ("none", "read", "callback with box coordinates; no data pointer (name matches only)"),
    "overlay_u1c_text_2e7d_cpu.c": ("global", "header", "script operand reads through the DS script pointer; FillChar target is the operand cell"),
    "overlay_u25_entry4_0341_cpu.c": ("global+local+code-literal", "read", "the buffer far pointer DS:[49DE] (FillChar, Move; the body never dereferences it); Str/StrCat/StrLoad operands are body-frame slots or CS literals"),
    "overlay_u40c_tile_015d_cpu.c": ("global+caller-argument", "read", "Move of a tile-set row (the caller's tile set) into the DS:5E4E object; the model STOPS (CONTRACT) when either operand meets A0000 and an EGA is attached"),
    "overlay_u25_entry5_0411_cpu.c": ("callee-returned+caller-argument", "read", "GetMem's block and the caller's record variable: the 0411 defect (par-c) was a flat wrapper deref"),
    "overlay_u38_message_3134_cpu.c": ("code-literal+local", "read", "StrLoad: source is the CS literal, destination a body-built local"),
    "overlay_u3b0_entry_01da_cpu.c": ("code-literal", "read", "set membership against the 32-byte set at CS:01BA"),
    "overlay_u3b0_entry_07a4_cpu.c": ("caller-argument", "header", "string-node far pointers handed in"),
    "overlay_u3b0_entry_0969_cpu.c": ("caller-argument+local", "header", "string node in; a body-built local copy is trimmed"),
    "overlay_u3b0_entry_0a08_cpu.c": ("caller-argument+local", "header", "string node in; a body-built local copy is trimmed"),
    "overlay_u3b0_entry_0c33_cpu.c": ("caller-argument+global", "header", "node pointers and the list head DS:6B29"),
    "overlay_u6f_entry6_0ec4_cpu.c": ("caller-argument+global+callee-returned", "header", "record, blocks, heap and string windows: every access is flat by declaration"),
    "overlay_u77_party_0901_cpu.c": ("local+code-literal+caller-argument", "header", "strings built in locals from literals; the party record pointer is an argument"),
    "overlay_u77_split_17ed_cpu.c": ("caller-argument+callee-returned", "header", "the far node pointer handed in, GetMem's block and the Move operands; the host guard refuses CS/DS/SS inside A0000..AFFFF (docs/re/U77_SPLIT_17ED.md: flat runtime operands exclude the aperture)"),
    "overlay_u7d_entry_05f0_cpu.c": ("code-literal", "header", "StrLoad of a CS literal"),
    "overlay_u7f_number_0294_cpu.c": ("caller-argument+local", "read", "StrStore reads the caller's far prompt string ([BP+8]); the other Str*/Val operands are body-built locals"),
    "resident_00ba_0973_cpu.c": ("caller-argument+local", "read", "node far pointer at BP+0C/0E and the local BP-0102h string; the accepted 1037 formatter writes the local buffer"),
    "resident_00ba_display_1693_cpu.c": ("caller-argument", "read", "StrStore reads the caller's far source string"),
    "resident_00ba_display_1816_cpu.c": ("code-literal", "header", "StrLoad of a CS literal into a local"),
    "resident_0464_compose_08d4_cpu.c": ("caller-argument", "read", "Move operands are the caller's tile pointers; the model STOPS (CONTRACT) when either meets A0000 and an EGA is attached"),
    "resident_0464_blit_1951_cpu.c": ("caller-argument", "read", "Move source and destination are the caller's bitmap pointers; the model STOPS (CONTRACT) when one meets A0000"),
    "resident_0709_msgbox_0bf5_cpu.c": ("caller-argument", "header", "StrStore of the caller's far string"),
    "resident_0709_printer_0626_cpu.c": ("caller-argument", "read", "StrStore of the caller's far string"),
    "resident_0709_printer_0768_cpu.c": ("caller-argument+local+code-literal", "read", "StrStore reads the caller's far string ([SP+4]); StrLoad is a CS literal; the set operands are locals"),
}


# Per-pair refinements where one caller mixes sources (a pair listed here overrides the caller's coarse entry above).
PAIR_SOURCES = {
    "resident_00ba_window_16ec_cpu.c": ("caller-argument+local", "read", "StrStore copies the caller string at BP+0A/+0C into the local BP-100h buffer; own stack and DS accesses are flat, aperture aliases excluded by contract"),
    ("resident_00ba_0973_cpu.c", "pr_res_00ba_1037_cpu"): ("local", "read", "1037 receives a byte value, not the node pointer; its destination is the body-built local SS:BP-0102h"),
    ("overlay_u23_append_0239_cpu.c", "pr_system_getmem_023f"): ("callee-returned", "read", "GetMem(3Fh) at 0297..029B and 02FC..0300 takes only the size; the block it returns is the pointer"),
    ("overlay_u23_append_0239_cpu.c", "pr_system_move_0a3f"): ("caller-argument+callee-returned", "read", "Move from ITEM (the far pointer at SS:[BP+0Ah]) into the GetMem block at [BP-4] or [BP-8] (02A6..02B4, 030B..0319)"),
    ("overlay_u23_append_0239_cpu.c", "pr_system_strload_0b0b"): ("code-literal+local", "read", "StrLoad of the 10-character literal at CS:022E into the body-frame temporary SS:[BP-13h] (0256..0260)"),
    ("overlay_u38_0cc3_cpu.c", "pr_system_strload_0b0b"): ("code-literal+local", "read", "StrLoad of the 26-byte message at CS:0CA8 into the body-frame temporary [BP-28h] (0D3A..0D44)"),
    ("overlay_u38_0cc3_cpu.c", "pr_system_freemem_0254"): ("global", "read", "FreeMem(node, 13h) at 0EED..0EF7: the node is the list node found from the global head DS:[6DBB] ([BP-8]/[BP-6]); the model reaches the node itself through the bus, FreeMem is flat"),
    ("resident_00ba_sidecar_remove_1509_cpu.c", "pr_system_strload_0b0b"): ("code-literal+local", "read", "StrLoad of the messages at CS:14CE / CS:14E2 into the body-frame temporaries [BP-18h] / [BP-2Bh] (151A..1524, 15A9..15B3)"),
    ("resident_00ba_sidecar_remove_1509_cpu.c", "pr_system_freemem_0254"): ("caller-argument", "read", "FreeMem(record, 3Fh) at 15E1..15EC: the record is the caller's far pointer SS:[BP+6]/[BP+8]; the model unlinks it through the bus, FreeMem is flat (declared exclusion in the header)"),
    ("overlay_u7f_0a88_cpu.c", "pr_system_strload_0b0b"): ("code-literal+local", "read", "StrLoad of the 10-byte message at CS:0A7D into a body-frame temporary (docs/re/OVL_007F_0A88_CPU.md)"),
    ("overlay_u7f_0203_cpu.c", "pr_system_strload_0b0b"): ("code-literal+local", "read", "StrLoad of the 39-byte message at CS:01DB into the body-frame temporary [BP-2Ah]"),
    ("overlay_u7f_044c_cpu.c", "pr_system_strload_0b0b"): ("code-literal+local", "read", "StrLoad of the 10-byte message at CS:0441 into the body-frame temporary [BP-0Bh]"),
    ("overlay_u77_split_17ed_cpu.c", "pr_system_strload_0b0b"): ("code-literal", "header", "StrLoad of the 188D literal in the caller's own segment"),
    ("overlay_u25_entry4_0341_cpu.c", "pr_system_fillchar_16a8"): ("global", "read", "FillChar of the 1E00h buffer at DS:[49DE]; the declared limit in OVL_0025_0341_CPU.md (the --demo shows the aperture divergence)"),
    ("overlay_u25_entry4_0341_cpu.c", "pr_system_move_1685"): ("global", "read", "Move into the buffer at DS:[49DE]; same declared limit"),
    ("overlay_u25_entry4_0341_cpu.c", "pr_system_str_1366"): ("local", "read", "Str into a body-frame string slot"),
    ("overlay_u25_entry4_0341_cpu.c", "pr_system_strcat_0b98"): ("local", "read", "StrCat on body-frame string slots"),
    ("overlay_u25_entry4_0341_cpu.c", "pr_system_strload_0b0b"): ("code-literal+local", "read", "StrLoad of CS literals (cs:0322/0338/033C) into body-frame slots"),
    ("overlay_u7f_number_0294_cpu.c", "pr_system_charstr_0c3b"): ("local", "read", "builds the one-character string in a body-built local"),
    ("overlay_u7f_number_0294_cpu.c", "pr_system_str_1366"): ("local", "read", "Str(max) into a body-built local"),
    ("overlay_u7f_number_0294_cpu.c", "pr_system_strcat_0b98"): ("local", "read", "text := text + key, both body-built"),
    ("overlay_u7f_number_0294_cpu.c", "pr_system_strcopy_0b57"): ("local", "read", "Copy of the body-built text"),
    ("overlay_u7f_number_0294_cpu.c", "pr_system_strload_0b0b"): ("local", "read", "string load of a body-built value"),
    ("overlay_u7f_number_0294_cpu.c", "pr_system_val_13b1"): ("local", "read", "Val of the body-built text"),
    ("overlay_u7f_number_0294_cpu.c", "pr_system_strstore_0b25"): ("caller-argument", "read", "the first StrStore reads the caller's far prompt ([BP+8]); the other three store locals"),
    ("resident_0709_printer_0768_cpu.c", "pr_system_strstore_0b25"): ("caller-argument", "read", "StrStore of the caller's far string ([SP+4])"),
    ("resident_0709_printer_0768_cpu.c", "pr_system_strload_0b0b"): ("code-literal", "read", "StrLoad of the CS:074E literal"),
    ("resident_0709_printer_0768_cpu.c", "pr_system_set_expand_0d12"): ("local", "read", "expands a set into a body-built local"),
    ("resident_0709_printer_0768_cpu.c", "pr_system_set_in_0dc0"): ("local", "read", "membership in the body-built set"),
    ("resident_0709_printer_0768_cpu.c", "pr_r0709_skip_spaces_0714_cpu"): ("local", "read", "walks the body's own frame (a nested procedure)"),
    ("overlay_u3b0_entry_0969_cpu.c", "pr_u3b0_lead_spaces_08d7_cpu"): ("local", "header", "trims a body-built local copy"),
    ("overlay_u3b0_entry_0969_cpu.c", "pr_u3b0_trail_spaces_0923_cpu"): ("local", "header", "trims a body-built local copy"),
    ("overlay_u3b0_entry_0a08_cpu.c", "pr_u3b0_lead_spaces_08d7_cpu"): ("local", "header", "trims a body-built local copy"),
    ("overlay_u3b0_entry_0a08_cpu.c", "pr_u3b0_trail_spaces_0923_cpu"): ("local", "header", "trims a body-built local copy"),
}
UNREACHABLE = ("none", "code-literal", "local", "code-literal+local", "local+code-literal")


def read(path):
    return Path(path).read_text(errors="replace")


def strip(code):
    return re.sub(r"/\*.*?\*/|//[^\n]*", "", code, flags=re.S)


def models():
    headers = {p.name: read(p) for p in INC.glob("*.h")}
    bus_headers = {n for n, t in headers.items() if re.search(r"\bpr_bus\b", t) and n != "memory_bus.h"}
    flat_functions = {}
    for name, text in headers.items():
        for m in re.finditer(r"\b(pr_[a-z0-9_]+)\s*\(\s*uint8_t\s*\*\s*(?:const\s*)?ram\b", text):
            flat_functions.setdefault(m.group(1), name)
    rows = []
    for path in sorted(SRC.glob("*.c")):
        text = read(path)
        code = strip(text)
        header = headers.get(path.stem + ".h", "")
        uses_bus = bool(re.search(r"\bpr_bus\b|pr_bus_(?:read|write)8", code))
        declared = bool(DECLARES.search(header + text[:4000]))
        callees = sorted(fn for fn in flat_functions if re.search(rf"\b{fn}\s*\(", code) and not fn.endswith("_bus_16a8")) if uses_bus else []
        rows.append(dict(model=path.name, kind="bus" if uses_bus else "flat", declared_exclusion="yes" if declared else "no", callees=callees))
    return rows


def observability(rows):
    out = []
    for row in rows:
        # Only a bus-aware model, audited against an oracle that models the EGA, can show an aperture pointer; a flat model's oracle
        # maps A0000.. as RAM, so the hole is invisible there by construction.
        out.append(dict(model=row["model"], kind=row["kind"], aperture_observable="yes-with-ega-oracle" if row["kind"] == "bus" else "no",
                        declared_exclusion=row["declared_exclusion"]))
    return out


def pairs(rows):
    out = []
    for row in rows:
        for callee in row["callees"]:
            takes = bool(POINTER_CALLEES.search(callee))
            source = (PAIR_SOURCES.get((row["model"], callee)) or SOURCES.get(row["model"])) if takes else None
            if takes and source is None:
                raise SystemExit(f"unclassified pointer pair: {row['model']} -> {callee} (add it to SOURCES)")
            reach = "unreachable-by-construction" if source and source[0] in UNREACHABLE else (
                "can-reach-under-a-corrupt-pointer" if source else "no-pointer")
            out.append(dict(caller=row["model"], flat_callee=callee, callee_takes_pointer="yes" if takes else "no",
                            pointer_source=source[0] if source else "-", classified_by=source[1] if source else "-",
                            reach=reach, caller_declares_exclusion=row["declared_exclusion"]))
    return out


def write(path, rows):
    with Path(path).open("w", newline="") as handle:
        writer = csv.DictWriter(handle, fieldnames=list(rows[0]), lineterminator="\n")
        writer.writeheader()
        writer.writerows(rows)


def main():
    rows = models()
    obs, prs = observability(rows), pairs(rows)
    if "--write" in sys.argv:
        write(OBSERVABILITY, obs)
        write(PAIRS, prs)
    for path, want in ((OBSERVABILITY, obs), (PAIRS, prs)):
        have = list(csv.DictReader(path.open())) if path.exists() else []
        if have != [{k: str(v) for k, v in r.items()} for r in want]:
            print(f"STALE or missing: {path.name} (run with --write)")
            return 1
    kinds = {k: sum(1 for r in obs if r["kind"] == k) for k in ("bus", "flat")}
    print(f"{len(obs)} models: {kinds['bus']} bus-aware, {kinds['flat']} flat (aperture NOT observable in their audits); "
          f"{len(prs)} (bus-aware caller, flat callee) pairs, {sum(1 for p in prs if p['callee_takes_pointer'] == 'yes')} with a pointer-taking callee")
    return 0


if __name__ == "__main__":
    sys.exit(main())
