# Saved-A +1974 continuation to the first untranslated string call

The bounded adapter starts at actual unit-6F `2A8E:1974`, after the
resident +0425 return, and reaches `2A8E:1999` using original instruction
semantics. It reuses the merged SYSTEM B25 capped-copy and B0B StrLoad
adapters. At +1999 it reports original `0AF8:0B98`, return +199E, and
blocks before pushing that call's frame. No string-value helper, expected
snapshot, or callback journal supplies CPU/RAM effects.

Before debugging, bounded `timeout 10s deja "Poolrad unit6F 1974 197E 1983
saved A continuation native CPU RAM Delete CBC"` expired with no reusable
output. Reused repository leads are the
[prior basename/filename observation](SLOT_A_NATIVE_1983_FILENAME.md),
[first-view trace](SLOT_A_NATIVE_FIRST_VIEW_AFTER_196F.md),
[merged B0B ABI join](SLOT_A_SAVED_COPY_1965_JOIN.md), and
[merged B25 instruction adapter](SLOT_A_STRING_0356_ABI.md).
The [preceding +0425 ABI join](https://github.com/huntergdavis/poolrad-decomp/pull/394)
remains a separate PR; it is not self-merged. This fresh branch starts at
main d4ff5a3. PM and p9 were notified of the unit-6F/runtime boundary;
U1C PRINT, RES_0709 B25 observations, and UI source remain p9's work.

## Fresh native pair and writer observations

The ignored run `slot-a-1974-abi-20261014` follows the normal menu/load-A
route without editing guest CPU or memory. Nine caller stops plus four
SYSTEM B25 stops capture all sixteen CPU fields and all 655360 RAM bytes.
The raw debugger transcript has thirteen complete
`EV CS IP DS ES FS GS SS SP BP AX BX CX DX SI DI FLAGS` rows; each RAM dump
is taken with `MEMDUMPBIN 0000 0000 A0000`. The checker pins every image,
row order, and derived CPU JSON. Older partial-register captures are used
only as address leads, not combined into this same-invocation pair.

| Ignored evidence | SHA-256 |
| --- | --- |
| Entry +1974 RAM | `7c7ee407affc465f389c7f390bdff97daf1905bb4e9944887fc20d8c276d28d7` |
| First copy return +1983 RAM | `42a2dadaf5e098405dcf9b328b20300338ea17eb54fa4eb8f9d0a676f348e785` |
| StrLoad return +1993 RAM | `11bb1b4905a84fda74624f844831b6527bc5fb5fcb78494f61e7b8f204f83fbc` |
| Gate +1999 RAM | `e34c73126f13f2c3ee8495f725dfbf9ae1014d6d749c499a579fcc22cd6c62a5` |
| Raw native transcript | `5d47098a9bf6cdb64db889cf3c20faa111bb727764fba4a916a01269d7e6773a` |

At entry CS:IP=2A8E:1974, DS=14A0, ES=SS=1D16, FS=GS=0000,
SP=37B4, BP=3DCA, AX=0008, BX=349A, CX=0000, DX=14A0,
SI=36B3, DI=39BF, FLAGS=7246. At the +1999 gate the unchanged fields
retain those values; SP=37B0, AX=000B, BX=37AC, SI=5386, DI=3BD4.
All sixteen fields are compared independently at every checkpoint.

The first LEA changes DI only. PUSH SS at +1978 writes an existing word,
so the +1974/+1978/+1979 RAM images are identical. PUSH DI at +1979 is
the first value-changing writer: exactly physical 20910/20911 change.
B25's STOSB at +0B3D changes only the length byte at 20D34; its REP MOVSB
changes precisely 20D35..20D3C. The complete +1974-to-+1999 net delta is
28 RAM bytes. The private eight-byte basename is copied to SS:3BD4; the
11-byte prefix is loaded from DS:537A to SS:3AB8. No payload is published.

At B25 entry SS:SP=1D16:37AA, the words are return IP=1983,
return CS=2A8E, cap=0028, destination=1D16:3BD4, source=1D16:39B6.
At +198E the two pointers are source=14A0:537A and destination=1D16:3AB8;
B0B consumes source but retains destination. At +1999 the stack pointers
are source=1D16:3BD4 and destination=1D16:3AB8. The actual next B98 call
is unexecuted by this adapter; its typed dependency preserves the full
state before any frame push.

## Original-byte and C verification

The host pins the same START.EXE, GAME.OVR and SAVGAMA.DAT full hashes as
the preceding joins. The 42-byte original unit-6F +1974..199D span,
including the gated call, hashes
`54afeb7162e304f9236a5bb3c29cf8ffbc95b34b3d10c455d7a47256680437b3`;
the loaded span hashes
`c5df882a2e21af59008381d080f06dd3ce125e398927d03e318589f60ed12e32`.
Exactly the three far-call segment words relocate 0AF8 to 131C. Original
B0B (26 bytes) hashes
`4681f1935f55d71f05afab0b13fff0505aa17e9a04fb372ff559ef28ed88058f`;
B25 (36 bytes) uses its merged hash pin. Both helper bodies are identical
between original input and loaded RAM at all thirteen native stops.

Unicorn 2.1.4 runs these verified loaded original instructions from the
same captured CPU/RAM. Every C instruction compares all sixteen CPU fields
and the entire RAM image; REP is one atomic host step. The natural path
passes **43 instruction steps** and **12 post-entry native CPU/full-RAM
checkpoints**. Four synthetic original-byte cases cover zero length with
16-bit source wrap, cap truncation plus a 255-byte prefix, forward B25
overlap, and forward B0B overlap. They pass 43/44/43/43 steps, respectively:
**216** native/synthetic steps in all. Synthetic frames are never described
as naturally reached inputs. The strict C suite and the complete 216-step
native/synthetic differential also passed with AddressSanitizer and UBSan.

Strict C tests cover immutable B98 reports, stale dependency invalidation,
invalid PCs, failing PUSH/far-call spans, incomplete far operands, delegated
REP memory errors, unsupported DF-set strings, and null arguments. Host
spans must fit RAM <= 1 MiB and one 64 KiB segment; unsupported spans block
atomically at the current instruction. Individual pointer arithmetic wraps
to sixteen bits; multi-byte segment/A20 wrapping is outside this slice.
Undefined logic AF is normalized to observed zero. No full-C identity,
game-function count increase, or playable-route claim follows from this
bounded ABI join. Delete remains a separate unimplemented ABI follow-on.

- `target_function`: reached unit-6F +1974 continuation through gate +1999.
- `source_candidates`: slot_a_continuation_1974.c composing merged B25/B0B.
- `signals`: original/loaded code and relocation; natural full-state pair;
  direct native writer deltas; instruction CPU/full-RAM differential;
  immutable unsupported-call/host-boundary checks.
- `score`: five checked signal categories, not a probability.
- `verdict`: bounded `match` for the measured path and synthetic prefixes;
  `unclear` for subsequent B98/file/UI/runtime behavior and unsupported spans.
- `review_notes`: snapshots supply comparisons only. All thirteen native
  rows belong to one unedited invocation. Gate and credit limits are explicit.

Run `bash tests/replay/check_slot_a_continuation_1974.sh` without media for
strict C checks. Full private reproduction, with Unicorn 2.1.4 installed
under ignored resources:

```sh
POOLRAD_UNICORN_PYTHON="$PWD/resources/venv-string0356/bin/python" \
  bash tests/replay/check_slot_a_continuation_1974.sh \
  resources/oracle-runs/slot-a-1974-abi-20261014
```

The native root contains POOLRAD inputs, thirteen ram-*.bin/cpu-*.json
pairs, and the pinned native.log. Optimized Python assertions are rejected.
Shared CMake registration remains with the integration owner.
