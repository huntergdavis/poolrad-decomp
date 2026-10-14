# RES_0464:0000 reached frame-to-return parity

`pr_r464_bitmask_frame` composes the existing complete
[bitmask value translation](RESIDENT_464_BITMASK.md) with its stack,
16-bit register, FLAGS and far-return effects. The new native gate compares
all 16 CPU-container fields and a complete 1 MiB memory dump from a reached
DOS invocation through its return. It adds no new routine identity.

## Evidence and scope

The required deja recall and reused mapping evidence are recorded in the
[separate +1484 note](RESIDENT_464_REVERSE_1484.md). This gate reuses
`pr_r464_bitmask`, its existing read-only byte audit, and the existing
`pr_slot_a_directory_state` container. No existing lane files were changed
other than the coordinated CMake registrations.

The private `resident-1484-001` probe used the existing heavy-debug DOSBox-X
binary in Bubblewrap with network disabled, a read-only host filesystem,
and writes limited to its copied ignored run directory. `DEBUGBOX START`
started the original hash-locked executable. AUTOTYPE scheduled
`enter l a` with `-w 28 -p 1`; no game state was edited. Breakpoints at
`0C88:1484`, `0C88:0000` and `0C88:007B` selected the first reached leaf.
It stopped at `0C88:0000`; the stack return identified `0C88:067E`.
The probe then removed all breakpoints, armed that return, and resumed.
This is a cold-launch bitmap invocation, not a saved-game movement claim.

At each stop, capture
`EV CS IP DS ES SS FS GS SP BP AX BX CX DX SI DI FLAGS` and
`MEMDUMPBIN 0000 0000 100000`, then preserve the ignored dump before
resuming. Normalized CPU rows use that field order with a leading
`entry` or `return` label. These run-specific files are hash-locked below;
fresh captures need independently reviewed hashes rather than silently
substituting new expected values.

| Boundary | CS:IP | SS:SP | BP | AX | CX | DI | FLAGS |
| --- | --- | --- | --- | --- | --- | --- | --- |
| Entry | `0C88:0000` | `1D16:3A60` | `3A98` | `410D` | `0000` | `0008` | `7602` |
| Return | `0C88:067E` | `1D16:3A6A` | `3A98` | `0000` | `0004` | `0003` | `7646` |

The raw selector is 13, its pushed high byte is nonzero, and the mask
returned is zero. DS is `14A0`, ES is `41E1`, and video byte DS:4A1E is
one. All other captured fields are unchanged. The only changed memory
bytes are the seven stack bytes `[0x20BB9,0x20BC0)`; all remaining bytes
in the 1 MiB dump agree exactly. This is a final-state comparison, not
a trace or write-journal parity claim.

The loaded 123-byte leaf exactly matches the original loader-expanded
body at image `[0x4640,0x46BB)`, SHA-256
`3e7b81eaa0e3e8109456fbdb6bd3ad59390b961eceb5f84fa2858055728e21a1`.
Its caller's original and loaded `+067B..+067D` near-call bytes also
match exactly. The reviewed audit proves the 47-instruction body, its four
branch sites and both direct callers. The resident inventory's
fixup-confirmed GAME.OVR caller to the parent `RES_0464:02A5` is unit
`0x3B9:+0400`. Source-library ownership remains unresolved, so this
game-path invocation earns no new game-specific/full-C count.

| Private artifact | Bytes | SHA-256 |
| --- | ---: | --- |
| `ram-entry.bin` | 1,048,576 | `68d9966d6075358b0bea1fe7eaaa8f0b507e7ef69e4cefd2010d2508b374722f` |
| `ram-return.bin` | 1,048,576 | `84ddb66f11e2b5c4b95db03247a247923bbaf6fca17f0c912ad0c750f9753bf7` |
| `cpu-entry.txt` | 68 | `c974b3b8d3ae0e4dc41a75815d5e4104dbf47c687fa8939fe574aa1bb2cc6a3f` |
| `cpu-return.txt` | 68 | `521778e9e26e8f6e48db5774fb4f87c53ad4777524cadec4ada17167a7cd6039` |

The executable hashes are those in the +1484 note. The native checker
requires the original copied START.EXE, exact capture hashes, code equality,
caller/return-frame proof, video state, full CPU equality and full RAM equality.

## Contract and adversarial review

The adapter accepts a readable stack with `SP in [8,FFF6]`. It reads four
packed bytes and the raw selector, calls the existing value function, and
models saved BP plus all five written locals. AX is the zero-extended mask,
CX is four, DI is three, and SP advances by ten. The final CMP of index
three with three sets ZF/PF and clears CF/AF/SF/OF. Other registers and
nonarithmetic FLAGS are preserved. Invalid host frames reject without mutation.
Interrupt timing, device internals, FPU, hidden CPU state and intermediate
local-store order are outside this 16-bit final-state contract.

| Field | Finding |
| --- | --- |
| target_function | `RES_0464:0000..007A`, reached video-state-1 caller |
| source_candidates | Existing bitmask value function plus new frame adapter |
| signals | Exact original/loaded bytes, native entry/return frame, every captured CPU field, 1 MiB RAM, all branch outcome masks |
| score | 1/1 reached native gate; 512/512 independent original-x86 cases; 4,096/4,096 synthetic frame/outcome cases |
| verdict | `match` for the stated frame-to-return contract |
| review_notes | Native sample returns zero; nonzero outcomes and raw selectors 16..255 are separately checked against original x86; no engine-wide or ownership claim |

## Reproduction

Build `/tmp/poolrad-resident-leaves.so` as shown in the +1484 note, then:

```sh
PYTHONPATH=/tmp/poolrad-capstone python3 \
  tests/recovered/check_resident_464_bitmask_frame_native.py \
  --root resources/oracle-runs/resident-1484-001 \
  --library /tmp/poolrad-resident-leaves.so
PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn python3 \
  tests/recovered/check_resident_464_bitmask_frame_x86.py \
  --exe resources/DOS/POOLRAD/START.EXE \
  --library /tmp/poolrad-resident-leaves.so
cmake -S . -B /tmp/poolrad-resident-1484-build \
  -DCMAKE_C_FLAGS='-Wall -Wextra -Wpedantic -Werror'
cmake --build /tmp/poolrad-resident-1484-build -j4
ctest --test-dir /tmp/poolrad-resident-1484-build --output-on-failure
```

All original media, full memory images, debugger logs and capture scripts
remain ignored. Only synthetic tests and address/hash/count evidence are public.
