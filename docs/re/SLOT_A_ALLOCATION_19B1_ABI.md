# Saved-A +19B1 allocation setup and explicit GetMem gate

The separate addressed adapter implements exactly two original unit-6F
instructions: +19B1 `MOV AX,011D` and +19B4 `PUSH AX`. It stops unchanged
at +19B5 before SYSTEM AF8:023F GetMem, return +19BA, without pushing the
far return frame or supplying allocator effects. The +19AD caller adapter
retains its +19B1 gate. File I/O effects remain explicitly blocked in the
separate probe adapter; no dispatcher or snapshot bridge is added.

This branch starts at fresh merged main
`798da1bc191d51725b3d079c4700de9388c6f9a9`, after PR #442. Before debugging
or implementation, bounded `deja "Poolrad U6F 19B1 allocation setup GetMem
19BA natural saved-A external call CPU RAM ABI"` expired without reusable
output. Reused repository leads are the
[continuation map](U6F_PARTY_CONTINUATIONS.md),
[earlier allocation return](SLOT_A_NATIVE_19BA_RECORD_RETURN.md), and
[verified caller return branch](SLOT_A_PROBE_RETURN_19AD_ABI.md).
Bytes, live CPU, stack effects and RAM were independently rechecked.
The review branch is restacked onto fresh main
`62b04eb9eb70103ace1e0817f1e97f0449a689bc`; the focused audit is rerun
after restacking and the standalone C implementation is unchanged.

## Pinned input and fresh native full state

| Ignored input/span | Bytes | SHA-256 |
| --- | ---: | --- |
| START.EXE | 65231 | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| GAME.OVR | 206105 | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| SAVGAMA.DAT | 13137 | `abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1` |
| Original +19B1..19B9, GAME file 1F997..1F99F | 9 | `27678668b204ad2d84785e89897bf20454582a70e1f7e34bd363cab31373a5b2` |
| Loaded 2A8E:19B1..19B9 | 9 | `963c38901cd3f9bc0c0473f7e06cb17e4f7559edff39d3d1827cfd9d489092f9` |
| Raw three-row native transcript | — | `94028d49d5d5a2d592e799d11ce256d89c2c3c06c65bcbc02876bb3561b71bf4` |
| Native +19B1 and +19B4 RAM | 655360 | `c276543076b2e1d6d9710c2b0a908f7750e155556c1bafa800460c3707f83a05` |
| Native +19B5 RAM | 655360 | `ba06721343a36d5dc458c4846968884735741976e6c1d9237d38e53a3b312835` |

The nine-byte span contains the two setup instructions and the gated far
CALL. Only its segment word relocates AF8 to loaded 131C. All loaded
bytes agree with the independently patched original in all three RAM
images. Each image and all sixteen CPU fields are pinned by the native
transcript, per-stop CPU JSON and RAM hashes.

The new ignored `slot-a-allocation-19b1-abi-20261014` run loads the actual
saved A without guest register/RAM edits and stops after the native file
probe's successful return. The input is native state, not a C probe result.
Throughout the three stops CS=2A8E, DS=14A0, ES=SS=1D16, FS=GS=0000,
BP=3DCA, BX=0006, CX=0000, DX=3734, SI=379C, DI=3704 and FLAGS=7202.

| Stop before instruction | AX | SP | RAM effect since preceding stop |
| --- | --- | --- | --- |
| +19B1 | 0001 | 37B8 | Native post-probe input |
| +19B4 | 011D | 37B8 | None; MOV changes only AX/IP |
| +19B5 | 011D | 37B6 | PUSH changes exactly physical 20916 and 20917 |

The stack word at SS:37B6 changes from 1D16 to 011D, the 285-byte
allocation argument. Flags and every other CPU field are preserved by
both instructions, except their documented AX/SP/IP changes. No allocator
call, heap update or far-return frame is executed in this native capture.
The owned native process was closed at +19B5 after freezing the transcript.

## C parity and bounds

Natural execution matches **two C instructions** against original-byte
Unicorn execution and **two post-entry native CPU/full-RAM checkpoints**.
The three captured rows comprise one input and those two checkpoints.
Five synthetic cases cover zero/all-bits incoming AX, stale arithmetic
flags including DF, SP=0000 word wrapping to FFFE, a high stack offset,
and a stack write aliasing existing local data. Natural and synthetic
runs total **12 C instruction comparisons**. Every comparison checks all
sixteen CPU fields and all 655360 RAM bytes, including the exact PUSH
writer. The media-free strict suite and complete differential also pass
ASan/UBSan; strict Wconversion warnings pass.

Strict checks cover segment-crossing and truncated host word spans,
A20-exceeding addresses, wrong PC/CS, stale reports, null arguments/RAM,
the image size limit and immutable typed GetMem blocking. Spans must fit
one 64 KiB segment and ram_size <= 1 MiB. Individual SP arithmetic wraps
to sixteen bits. The caller supplies audited original/loaded code identity
and nonoverlapping state/report/RAM. Unsupported states preserve CPU/RAM
atomically, and null arguments preserve report. Tests have a three-call /
ten-second per-case bound and sixty-second process bound plus five-second
kill grace. No original media or captures are published.

The merged 0425 progress/timeout guard, immutable PR #410 map, +19AD
branch gate and probe I/O gates are unchanged. This is setup parity only;
no end-to-end C probe/allocator, playable-route, game identity or full-C
count credit is claimed. GetMem runtime ownership and effects remain unresolved.

Two disjoint follow-on scopes are ready for separate branches:

1. **SYSTEM AF8:023F GetMem ABI.** Measure the real +19B5 far-call frame,
   allocator entry/return CPU/full RAM, returned DX:AX and heap writers.
   Stop explicitly at unknown allocator effects; use new runtime-specific
   source/header/test/evidence files. This scope owns no caller stores.
2. **Unit-6F +19BA caller continuation.** From an actual native allocation
   return, verify the ordered AX/DX far-pointer stores and record-loader
   argument setup, stopping before +19D7 -> unit-6F:0AEC, return +19DA.
   Use separate caller-specific new files. This scope supplies no heap
   implementation or record-loader effects and does not inject an allocator
   result into a continuous replay.

These address ranges and new-file scopes avoid pC's key route and pR's
printer work. Neither follow-on is represented as completed here.

- `target_function`: unit-6F +19B1/+19B4 setup and +19B5 GetMem boundary.
- `source_candidates`: slot_a_allocation_19b1_abi.c.
- `signals`: original/loaded byte identity; fresh natural full state;
  exact stack writer; flags/wrap/alias checks and atomic boundaries.
- `score`: four checked signal categories, not a probability.
- `verdict`: bounded `match` for two setup instructions; `unclear` for
  allocator effects, complete saved-A C parity and allocator game/library identity.
- `review_notes`: GetMem is never executed by C or this native capture.
  The actual native post-probe input supplies no translated I/O effects.

Run `bash tests/replay/check_slot_a_allocation_19b1_abi.sh` without media
for strict checks. Full private reproduction:

```sh
POOLRAD_UNICORN_PYTHON="$PWD/resources/venv-string0356/bin/python" \
  bash tests/replay/check_slot_a_allocation_19b1_abi.sh \
  resources/oracle-runs/slot-a-allocation-19b1-abi-20261014
```

Unicorn 2.1.4 and non-optimized Python assertions are required. The six new
owned paths need no shared CMake changes; registration remains with integration.
