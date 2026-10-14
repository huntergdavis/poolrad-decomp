# Controlled native SYSTEM Delete ABI and resident caller frame

This separate bounded map measures resident `1026:03FC -> SYSTEM
131C:0CBC -> 1026:0401` with synthetic `ABA` source and `A` needle.
It records the actual count/index/string arguments, nested helper frames,
register effects, and target writes. **The C Delete implementation remains
absent and BLOCKED_DELETE remains unchanged.** Native/emulator behavior
inside Delete is not presented as C parity or full-C identity credit.

The [preceding +1974 continuation](https://github.com/huntergdavis/poolrad-decomp/pull/403)
is a separate PR, stopping before +1999 -> SYSTEM B98. This fresh mapping
branch starts from main 63a433b. Before debugging,
`timeout 10s deja "Poolrad resident 0802 03FC SYSTEM AF8 CBC Delete native
caller frame RETF"` returned no matches (searched zero sessions).
Reused leads were independently checked against original bytes:
[the 0356 inventory](RESIDENT_STRING_0356.md),
[party-string boundary](SLOT_A_PARTY_STRING_BOUNDARY.md), and
[merged C instruction/gate adapter](SLOT_A_STRING_0356_ABI.md).
No UI, key dispatch, shared CMake, or existing C source is edited.

## Controlled input and native provenance

The isolated ignored run `slot-a-delete-abi-20261014` takes the normal
menu/load-A route to the first natural unit-6F +196A caller. This caller's
actual needle/source/output far pointers are 1D16:3BB6, 1D16:3C82,
and 1D16:3AB6. Its source pointer differs from a later previously captured
caller at 1D16:3CAB; it is read from the actual stack, not assumed.

At that stopped caller, debugger SM writes synthetic Pascal `A` at the
needle and `ABA` at the selected source. The two requested spans total six
bytes; exactly five byte values change because one existing length byte
already agrees. Before/after captures prove every CPU field and all RAM
outside those spans unchanged. No registers, caller frames, code, or
original media/save files are modified. Subsequent Delete reachability is
**controlled**, not a naturally reached saved-A Delete observation.

Eighteen complete EV CPU/RAM stops include natural and controlled +196A,
+03FC, Delete entry/prologue, both substring entries/returns, B98 entry/
return, B25 entry and writers, epilogue, and resident return +0401. Each
captures all sixteen CPU fields and 655360 RAM bytes; all images and raw
console output remain under ignored resources. The checker pins every
row, image, input file, and original/loaded code span.

| Ignored evidence | SHA-256 |
| --- | --- |
| Natural caller RAM | `03832ea833e47411c07ada59cfdbeb9703748e07c58b490688c963fa0cc1dfec` |
| Controlled caller RAM | `2653e11eaa25bd5d1901b674e93e5f7f4046aa0fac41f09c91c59da7dd40e444` |
| Reached +03FC RAM | `ffed4537737c02607b84c395b070d75feb0001670518ed85a2a2a73a0d1a8501` |
| Delete entry RAM | `0413cdde392f828e2c92cdcc55c249695c63e1bab5ccde3efaf288be83505a20` |
| Resident +0401 return RAM | `9b9098f7607d4915d69cdf9b6a24b7e422b9390f186f598a8070ef4dbe62488c` |
| Raw eighteen-row transcript | `ac6e7f6e47fb8e1758687e20b90737dee67e2d4ad54c6a11176b5d018ba53c30` |

The original five-byte +03FC call hashes
`a5def57c24a53296a5350790c360ecfb2e5a61c7e1f5981cc27f78918529077f`;
loaded call hash is
`14b42bc81a603890a934028783a90ef4c631b709a6b3e42d77c9de802a76c416`.
It relocates original AF8:CBC to loaded 131C:CBC. The 86-byte Delete body
at START file B932..B987 is byte-identical in loaded RAM and hashes
`14be20a0c6b74485ec3dcc683266dc07bc2c6e130d9ddcb2f83434b926996a33`.
B57 (65 bytes), B98 (44), B25 (36), and the C-prefix C10 compare (43) also
match their independently pinned original bodies. Delete's helper calls
are PUSH CS followed by near CALL; their RETF instructions consume the
paired far frames without additional relocations inside this body.

## Actual caller and helper frames

At resident +03FC, SS:SP=1D16:3496, BP=37A2. The words at SP are count=1,
index=1, source offset=36A2, source segment=1D16. Far CALL pushes
return 1026:0401, giving Delete entry SP=3492. Its prologue PUSH BP and
MOV BP,SP give BP=3490; SUB SP,0200 gives SP=3290.

| Delete BP-relative argument | Measured value |
| --- | --- |
| [BP+6], signed count word | 0001 |
| [BP+8], index word | 0001 |
| [BP+A], far Pascal source/target | 1D16:36A2 |
| [BP-100], prefix local | 1D16:3390 |
| [BP-200], suffix local | 1D16:3290 |

| Nested entry | SS:SP | Return | Arguments following IP/CS frame |
| --- | --- | --- | --- |
| B57 prefix | 1D16:3280 | 131C:0CE1 | count 0, index 1, source 1D16:36A2, output 1D16:3390 |
| B57 suffix | 1D16:327C | 131C:0CFB | count FF, index 2, source 1D16:36A2, output 1D16:3290 |
| B98 concatenate | 1D16:3284 | 131C:0CFF | source 1D16:3290, destination 1D16:3390 |
| B25 store | 1D16:3282 | 131C:0D0C | cap FF, destination 1D16:36A2, source 1D16:3390 |

B57 RETF 8 consumes count/index/source while retaining output. B98 RETF 4
consumes source while retaining destination. B25 RETF 10 consumes the
cap, destination, and retained source. Delete then restores its frame and
RETF 8 consumes count/index/source, returning to resident +0401 with
SP=349E. No expected frame or callback result is injected into execution.

At +03FC AX=0001, BX=3492, CX=0000, DX=14A0, SI=34A0, DI=36A2.
At +0401 AX=0002, BX=3282, CX=0000, DX=14A0, SI=3393, DI=36A5.
BP=37A2, DS=14A0, ES=SS=1D16, FS=GS=0000, FLAGS=7246 agree at both
boundaries; CS:IP and SP change as described above. All fields, including
intermediate flags, are independently checked at native checkpoints.

The caller far CALL changes exactly physical 205F2/205F3; Delete PUSH BP
changes 205F0/205F1. At B25 +B3D the first target-string write changes
only length byte 20802 from 3 to 2. Its REP MOVSB changes 20803/20804,
producing synthetic Pascal `BA`; the old trailing `A` remains outside the
new length. The complete +03FC-to-+0401 net delta is 31 RAM bytes.

## Verification and explicit remaining work

The existing C adapter runs the controlled +196A prefix for **141 C
instruction steps**, matching all CPU fields and full RAM with original-byte
Unicorn 2.1.4 execution, then matches the actual native +03FC state and
blocks unchanged with the original AF8:CBC/0401 dependency. No C Delete
instruction runs. Separately, **125 original-byte instructions**, including
the Delete transfer and nested helpers, match **15 native post-call
CPU/full-RAM checkpoints** through the actual +0401 return. The same
audit also passed with the unchanged C prefix instrumented by ASan/UBSan;
Delete-body comparison remains native/original-byte emulation only.

Three emulator-only count words, 0000/FFFF/8000, exercise the signed JLE
at +CC7. Each takes eight instructions and returns without dereferencing
an intentionally invalid source pointer. These are decoded-branch/emulator
checks, not native observations. Negative counts cannot originate from the
0356 caller's zero-extended byte count. All variants preserve the existing
C BLOCKED_DELETE outcome. Native positive count=1 is the measured case;
other positive index/count values, aliases, and B98 overflow paths remain
outside this native claim.

Delete's nested B98 near-call frame is now measured. It supplies an ABI
lead for the continuation's reported B98 dependency; the unit-6F +1999
call remains unexecuted by its C adapter. B98 C register/flag/RAM
semantics and the complete C Delete body remain untranslated here. Runtime
helpers retain unresolved library/game provenance. No full-C or game
identity credit, count change, or playable-route claim is made.

- `target_function`: controlled resident +03FC Delete caller and SYSTEM CBC
  argument/frame/runtime effects through +0401.
- `source_candidates`: new original-byte ABI audit; existing +0356 C prefix
  and immutable Delete gate. No candidate C Delete body is added.
- `signals`: hash-linked original/loaded bytes; bounded edit provenance;
  actual nested frames and native writers; C-prefix/native full-state match;
  Delete native/emulator checkpoints and retained gate checks.
- `score`: five checked signal categories, not a probability.
- `verdict`: bounded `match` for the measured ABI and controlled native/
  emulator effects; `unclear` for C Delete implementation or general runtime.
- `review_notes`: controlled reachability is explicit. The decoded signed
  no-op branch is distinct from positive-count native evidence. Existing
  game-string value-only C output does not supply native ABI effects.

Run `bash tests/replay/check_slot_a_delete_abi_map.sh` to retain the existing
strict C blocker suite without media. Full private reproduction:

```sh
POOLRAD_UNICORN_PYTHON="$PWD/resources/venv-string0356/bin/python" \
  bash tests/replay/check_slot_a_delete_abi_map.sh \
  resources/oracle-runs/slot-a-delete-abi-20261014
```

The ignored native root holds original POOLRAD inputs, eighteen RAM/CPU
pairs, and native.log. Optimized Python assertions are rejected. This
mapping changes only this note and its two focused audit/runner files.
