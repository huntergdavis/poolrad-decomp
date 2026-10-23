#!/usr/bin/env python3
"""Hash-locked original/C differential for unit 0x25 +07BC entered on its own; stdout is counts/hashes only.

+07BC (the class-1 load hook) is translated once, inside the strict +0E0C model; `pr_u25_load_hook_07bc`
is a thin public entry over that same code. This checker reuses the +0E0C harness (spans, hashes, the
Unicorn runner, the declared pinned-flag splits and the comparison of CPU14 and all 1 MiB) but enters at
+07BC with its own RETF 6 frame: [SP] IP, [SP+2] CS, [SP+4]/[SP+6] the far pointer to the found flag,
[SP+8] the address word. Requires local Capstone 5 and Unicorn 2; owner media stays under ignored resources/.
"""
import argparse
import ctypes
import json
import random
import subprocess
import sys
import tempfile
from pathlib import Path

import u25_load_word_0e0c_audit as H
from pinned_shift_flags import SPLIT_BY_RULE

CHECKER_ROWS = ('OVL_0025_07BC',)
ENTRY = "pr_u25_load_hook_07bc"
KEY = 0x6B00                      # the hook subtracts 6B00h from the address word before its compare chain
REC = (0x5000, 0x0100)            # the active-character record behind DS:[5D92]


def make_case(bodies, address, *, sp=0x8000, ds=0x1000, ss=0x2000, flag=(0x7800, 0x0040)):
    """+0E0C's world (globals, record pointer, code at CS 3000h), entered at +07BC with the hook's own frame."""
    ram = bytearray(bytes(range(256)) * (H.SIZE // 256))
    cpu = H.Cpu(0x9234, 0x1357, 0x2468, 0xABCD, 0x5678, 0x6789,
                0x4321, sp, 0x3000, 0x07BC, ds, 0x6000, ss, 0x0AD7)
    H.pointer(ram, ds, 0x49D6, 0x4200, 0x0100)
    H.pointer(ram, ds, 0x5D92, REC[0], REC[1])
    H.pointer(ram, ds, 0x5D96, 0, 0)
    for k, value in enumerate((0x1111, 0x7000, flag[1], flag[0], address)):
        H.put16(ram, ss, (sp + 2 * k) & 0xFFFF, value)
    for start, body in bodies.items():
        at = H.physical(cpu.cs, start)
        ram[at:at + len(body)] = body
    return ram, cpu


def rec(ram, off, value, width=1):
    at = H.physical(REC[0], REC[1] + off)
    for i in range(width):
        ram[(at + i) & (H.SIZE - 1)] = (value >> (8 * i)) & 255


def cases(bodies, quick):
    rng = random.Random(0x2507BC)
    keys = range(0, 0x400) if not quick else (0x18, 0x9B, 0xA0, 0xA4, 0xA5, 0xAC, 0xAD, 0xB8, 0xBB, 0xC3, 0xD8, 0xF7, 0xF9,
                                               0x100, 0x10C, 0x10D, 0x11B, 0x2B1, 0x2B4, 0x2CF, 0x312, 0x33E, 0x33F, 0x3FF)
    for k in keys:                                       # every key the compare chain can name, and the not-found default
        yield f"key-{k:03x}", *make_case(bodies, (KEY + k) & 0xFFFF)
    for address in ([0, 0x6AFF, 0xFFFF, 0x6F00] + ([] if quick else [rng.randrange(65536) for _ in range(256)])):
        yield f"address-{address:04x}", *make_case(bodies, address)
    for key, off, signed in ((0x18, 0x14, 0), (0x9B, 0x6E, 0), (0xA0, 0x73, 1), (0xD8, 0xA0, 1), (0xF9, 0xBA, 0), (0x11B, 0x11C, 0), (0xB8, 0x84, 0)):
        for value in (range(256) if not quick else (0, 0x7F, 0x80, 0xFF)):
            ram, cpu = make_case(bodies, KEY + key)
            rec(ram, off, value)
            yield f"byte-{key:03x}-{value:02x}", ram, cpu
    for key in range(0xA5, 0xAD):                        # the indexed byte table at record +76h + (key - A4h)
        for value in (0, 0x80, 0xFF):
            ram, cpu = make_case(bodies, KEY + key)
            rec(ram, 0x76 + key - 0xA4, value)
            yield f"table-{key:03x}-{value:02x}", ram, cpu
    for key, off in ((0xBB, 0x88), (0xBD, 0x8A), (0xBF, 0x8C), (0xC1, 0x8E), (0xC3, 0x90), (0xF7, 0xB8)):
        for value in (0, 1, 0x7FFF, 0x8000, 0xFFFF):
            ram, cpu = make_case(bodies, KEY + key)
            rec(ram, off, value, 2)
            yield f"word-{key:03x}-{value:04x}", ram, cpu
    for a in (0, 1, 2, 255):
        ram, cpu = make_case(bodies, KEY + 0x100)
        rec(ram, 0x10D, a)
        yield f"rec100-{a}", ram, cpu
        for b in (0, 1, 2, 255):
            ram, cpu = make_case(bodies, KEY + 0x10C)
            rec(ram, 0x10E, a)
            rec(ram, 0x10F, b)
            yield f"rec10c-{a}-{b}", ram, cpu
    for value in (range(256) if not quick else (0, 2, 3, 4, 7, 8, 0xC, 0xD, 0x11, 0x12, 0x19, 0x1A, 0xFF)):   # the class table (+15h)
        ram, cpu = make_case(bodies, KEY + 0x2CF)
        rec(ram, 0x15, value)
        yield f"class-{value:02x}", ram, cpu
    for value in (0, 1, 0x80, 0xFF):
        ram, cpu = make_case(bodies, KEY + 0x312)
        ram[H.physical(cpu.ds, 0x5376)] = value
        yield f"g5376-{value:02x}", ram, cpu
    for value in (0, 0x8000, 0xFFFF):
        ram, cpu = make_case(bodies, KEY + 0x33E)
        H.put16(ram, 0x4200, 0x0100 + 0x67C, value)
        yield f"w49d6-{value:04x}", ram, cpu
    for key in (0x2B1, 0x2B4):                           # the two +0756 calls: the party list from DS:[5D96]
        for length in (0, 1, 3, 257):
            for target in (0, max(0, length - 1), -1):
                ram, cpu = make_case(bodies, KEY + key)
                H.pointer(ram, cpu.ds, 0x5D96, 0x5000 if length else 0, 0x100 if length else 0)
                H.pointer(ram, cpu.ds, 0x5D92, 0x9000 if target < 0 else 0x5000 + 0x20 * target, 0x100)
                for i in range(length):
                    H.pointer(ram, 0x5000 + 0x20 * i, 0x204, 0x5000 + 0x20 * (i + 1) if i + 1 < length else 0,
                              0x100 if i + 1 < length else 0)
                yield f"party-{key:03x}-{length}-{target}", ram, cpu
    sp = 0x8000                                          # the found flag aimed at each byte of the hook's own frame
    for k in range(-6, 10):
        for key in (0x18, 0x10D, 0x2B1, 0x3FF):
            yield f"flag-at-sp{k:+03d}-{key:03x}", *make_case(bodies, KEY + key, sp=sp, flag=(0x2000, (sp + k) & 0xFFFF))
    for s in (0, 1, 2, 3, 6, 7, 8, 9, 0xA, 0xB, 0xFFF4, 0xFFF5, 0xFFFE, 0xFFFF):
        for key in (0x18, 0x2CF, 0x2B4, 0x3FF):
            yield f"stack-{s:04x}-{key:03x}", *make_case(bodies, KEY + key, sp=s)
    for s in (0xF, 0x10, 0x11, 0x15):
        yield f"A20-stack-{s:02x}", *make_case(bodies, KEY + 0x18, sp=s, ss=0xFFFF)
    yield "A20-globals", *make_case(bodies, KEY + 0x33E, ds=0xFFFF)
    for key in (0x18, 0x312, 0x33E, 0x10C, 0x2B1, 0x2B4, 0x3FF):
        for s in (0x5D98, 0x5DA8, 0x49D8, 0x5378, 0x3FF6):     # DS = SS: the frame over the globals the hook reads
            if party_link_overlap(key, s):
                continue
            yield f"DS-SS-alias-{key:03x}-{s:04x}", *make_case(bodies, KEY + key, sp=s, ds=0x2000)


# The one excluded shape. With DS = SS, a frame whose bytes cover DS:5D96..5D99 (the party-walk link that +0756 follows) on a
# +0756 key rewrites the link before the walk, and the walk becomes cyclic: it never returns, in DOS or in C, by contract.
# The deepest frame byte is entry SP - 18h, measured from the bytes: the hook's PUSH BP and SUB SP,4 (to SP-6), its two pushed
# pointer words and the far return into +0756 (to SP-0Eh), then +0756's PUSH BP and SUB SP,8 (to SP-18h). The highest is
# entry SP + 9 (the address argument).
FRAME_LOW, FRAME_HIGH = 0x18, 9
PARTY_LINK = (0x5D96, 0x5D99)


def party_link_overlap(key, sp):
    return key in (0x2B1, 0x2B4) and sp - FRAME_LOW <= PARTY_LINK[1] and sp + FRAME_HIGH >= PARTY_LINK[0]


MODEL = H.ROOT / "src/recovered/overlay_u25_load_word_0e0c.c"
MUTANTS = (   # each edits load_hook, the code +07BC and +0E0C share
    ("the flag set to 2 at entry", "w8(ram, c->es, c->di, 1u);", "w8(ram, c->es, c->di, 2u);"),
    ("the key bias 6A00h", "arithmetic(c, c->ax, 0x6B00u, 16u, 1)", "arithmetic(c, c->ax, 0x6A00u, 16u, 1)"),
    ("key 9Bh reads +6Fh", "r8(ram, c->es, (uint16_t)(c->di + 0x6Eu))", "r8(ram, c->es, (uint16_t)(c->di + 0x6Fu))"),
    ("key A0h zero-extends", "al(c, r8(ram, c->es, (uint16_t)(c->di + 0x73u)));\n        cbw(c);", "al(c, r8(ram, c->es, (uint16_t)(c->di + 0x73u)));\n        zero_ah(c);"),
    ("the A5h..ACh range ends at ABh", "(void)arithmetic(c, c->ax, 0xACu, 16u, 1);", "(void)arithmetic(c, c->ax, 0xABu, 16u, 1);"),
    ("the indexed table at +77h", "r8(ram, c->es, (uint16_t)(c->di + 0x76u))", "r8(ram, c->es, (uint16_t)(c->di + 0x77u))"),
    ("key F7h reads a byte", "{0xF7u, 0xB8u, 2u}", "{0xF7u, 0xB8u, 0u}"),
    ("key 100h results swapped", "result(ram, c, c->flags & 0x40u ? 0x80u : 1u);", "result(ram, c, c->flags & 0x40u ? 1u : 0x80u);"),
    ("key 10Ch compares +10Eh with 2", "(void)arithmetic(c, r8(ram, c->es, (uint16_t)(c->di + 0x10Eu)), 1u, 8u, 1);", "(void)arithmetic(c, r8(ram, c->es, (uint16_t)(c->di + 0x10Eu)), 2u, 8u, 1);"),
    ("key 10Dh writes the local", "if (eq16(c, 0x10Du)) goto done;", "if (eq16(c, 0x10Du)) { result(ram, c, 0u); goto done; }"),
    ("class 11h gives 56", "else if (eq8(c, 17u)) result(ram, c, 55u);", "else if (eq8(c, 17u)) result(ram, c, 56u);"),
    ("class range 12h..19h ends at 18h", "(void)arithmetic(c, (uint8_t)c->ax, 25u, 8u, 1);", "(void)arithmetic(c, (uint8_t)c->ax, 24u, 8u, 1);"),
    ("key 312h reads DS:5377", "al(c, r8(ram, c->ds, 0x5376u));", "al(c, r8(ram, c->ds, 0x5377u));"),
    ("key 33Eh reads +67Eh", "c->ax = r16(ram, c->es, (uint16_t)(c->di + 0x67Cu));", "c->ax = r16(ram, c->es, (uint16_t)(c->di + 0x67Eu));"),
    ("the not-found path leaves the flag", "    les(ram, c, c->ss, (uint16_t)(c->bp + 6u));\n    w8(ram, c->es, c->di, 0u);\ndone:", "    les(ram, c, c->ss, (uint16_t)(c->bp + 6u));\ndone:"),
    ("RETF 4", "    finish(ram, c, 6u);\n    return PR_U25_LOAD_WORD_OK;\n}", "    finish(ram, c, 4u);\n    return PR_U25_LOAD_WORD_OK;\n}"),
    ("the 2B4h call returns to 0984h", "push(ram, c, 0x099Du);", "push(ram, c, 0x0984u);"),
)


def mutant_library(directory, text):
    source = directory / "mutant.c"
    source.write_text(text)
    path = directory / "mutant.so"
    subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-shared", "-fPIC", "-I", str(H.ROOT / "include"),
                    str(source), str(H.ROOT / "src/recovered/overlay_u25_party_index_0756.c"), "-o", str(path)], check=True, capture_output=True)
    lib = ctypes.CDLL(str(path))
    getattr(lib, ENTRY).argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t, ctypes.POINTER(H.Cpu)]
    getattr(lib, ENTRY).restype = ctypes.c_int
    return lib


def mutation(bodies, instructions):
    """Each mutant runs the quick case set; the first mismatch catches it."""
    src, out = MODEL.read_text(), []
    for k, (name, old, new) in enumerate(MUTANTS):
        H.require(src.count(old) == 1, f"mutant text occurs once: {name}")
        print(f"[mutant {k + 1}/{len(MUTANTS)}] START {name}", file=sys.stderr, flush=True)
        with tempfile.TemporaryDirectory(dir=H.ROOT / "resources/u25-load-word-parity") as folder:
            try:
                lib = mutant_library(Path(folder), src.replace(old, new))
            except subprocess.CalledProcessError:
                out.append((name, "COMPILE-ERROR"))
                continue
            verdict = "SURVIVED"
            for label, ram, cpu in cases(bodies, True):
                try:
                    H.compare(label, ram, cpu, bodies, instructions, lib, set(), entry=ENTRY)
                except ValueError as error:
                    verdict = f"CAUGHT by {label}: {str(error)[:80]}"
                    break
            out.append((name, verdict))
    return dict(total=len(out), caught=sum(r.startswith("CAUGHT") for _, r in out),
                survivors=[n for n, r in out if not r.startswith("CAUGHT")], results=out)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--ovr", type=Path, default=H.ROOT / "resources/DOS/POOLRAD/GAME.OVR")
    parser.add_argument("--quick", action="store_true")
    parser.add_argument("--mutate", action="store_true", help="run the load_hook mutants on the quick case set")
    parser.add_argument("--native", action="store_true", help="synthetic DOSBox-X oracle entering at +07BC (packaged build)")
    parser.add_argument("--compile-only", action="store_true", help="build and bind the candidate library only; no media")
    args = parser.parse_args()
    if args.compile_only:
        with tempfile.TemporaryDirectory(prefix='poolrad-checker-') as t:
            getattr(H.library(Path(t)), ENTRY)
        print('CHECKER_BUILD_OK')
        return
    bodies, instructions = H.source(args.ovr)
    if args.native:
        from u25_load_hook_07bc_native import native
        directory = H.ROOT / "resources/u25-load-word-parity/native-07bc"
        print(json.dumps(native(directory, bodies, H.library(directory.parent)), sort_keys=True))
        return
    if args.mutate:
        (H.ROOT / "resources/u25-load-word-parity").mkdir(parents=True, exist_ok=True)
        print(json.dumps(mutation(bodies, instructions), sort_keys=True))
        return
    hook = {a for a in instructions if 0x0756 <= a < 0x0A74}      # +07BC and its +0756 callee
    visited, total, transfers, labels = set(), 0, 0, []
    (H.ROOT / "resources/u25-load-word-parity").mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(dir=H.ROOT / "resources/u25-load-word-parity") as folder:
        lib = H.library(Path(folder))
        for index, (label, ram, cpu) in enumerate(cases(bodies, args.quick)):
            if index % 500 == 0:
                print(f"[{index}] {label}: START", file=__import__('sys').stderr, flush=True)
            transfers += H.compare(label, ram, cpu, bodies, instructions, lib, visited, entry=ENTRY) == 2
            total += 1
    unvisited = sorted(hook - visited)
    H.require(args.quick or not unvisited, f"unvisited +07BC/+0756 instructions: {[f'{a:04X}' for a in unvisited]}")
    H.require(not (visited - hook), "execution left +07BC and +0756")
    print(json.dumps(dict(cases=total, internal_return_transfers=transfers, visited_instructions=len(visited & hook),
                          hook_and_0756_instructions=len(hook), declared_flag_split_arrivals=H.SPLIT_ARRIVALS[0],
                          declared_flag_split_by_rule=dict(SPLIT_BY_RULE), game_ovr_sha256=H.OVR_SHA,
                          unvisited=[f"{a:04X}" for a in unvisited]), sort_keys=True))


if __name__ == "__main__":
    main()
