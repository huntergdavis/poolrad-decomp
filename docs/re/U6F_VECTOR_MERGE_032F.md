# U6F +032F: complete mode-controlled vector merge

The new `pr_u6f_vector_merge_032f` translates the complete call-free body
through `RETF 8`, with live raw RAM, fourteen CPU words and defined arithmetic
flags. This is an unclaimed substantive source candidate outside the active
U7F/U33/U38/U6C lanes. It does not promote game identity or strict credit;
the current classification is `game_or_other_library_unresolved`.

## Source, selection and callers

The target choice was sent before implementation (inboxes i3412 / i3427).
Branch base: `217e809448be096fcc48b2413714856f5b165284`.
A target-specific 12-second `deja` query completed without a match; no session
finding was reused. Earlier broad bounded candidate recalls expired without
findings. Reused source-layout and CFG methods are
[overlay_inventory.py](../../tools/overlay/overlay_inventory.py) and
[the leaf alias audit](FUNCTION_ALIAS_AUDIT.md). The known video-state role
of `DS:4A1E` is documented in
[the native row-nine page](SLOT_A_ROW9_COL34_PRINTER_PAGE.md).

The U6A pool was checked before the PM redirect: its five substantive
call-free prologue bodies are already modeled or excluded; the remaining
five-byte initializer is not a recovery target. All 33 aligned near-call
sites target known starts. No empty marker or duplicate original was selected.

| Source item | Extent / identity |
| --- | --- |
| `START.EXE` | SHA-256 `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| `GAME.OVR` | 206105 bytes; SHA-256 `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| Unit 6F | original base `0x1DFE6`; runtime stub segment relative to load base `007C` |
| Complete leaf | file `[0x1E315,0x1E45C)`, unit `+032F..+0475`; 327 bytes / 121 starts |
| Leaf SHA-256 | `2df5a5980843eb72e7ed5382d4d4d452d541c22ebfe0d43ee065947b016c2dde` |
| Last instruction | `RETF 8` at `+0473`; no calls, internal branches only |

There are **two direct aligned leaf callers**, both inside unit-6F function
`+0482`: `PUSH CS; CALL +032F` at `+0546` and `+056A`, returning to
`+0549` and `+056D`. Four direct callers were not invented to meet a target.

Nine separate upstream, fixup-backed far calls invoke public entry 3
(`007C:002F`, function `+0482`): unit-6C function `+306D` at `+3192` and
`+3233`; function `+382D` at `+3862`, `+38A3`, `+3B52`, `+3B69`, `+3BA7`,
`+3C5A` and `+3CF9`. These are upstream calls, not nine direct leaf calls or
proof that every call takes a leaf branch. The audit follows each caller's
bounded CFG from its prologue and checks segment fixups. No U6C-owned file
is changed. The public open-claim check found no U6F claim at selection.

## Exact body contract

Entry `SS:SP` holds return IP/CS, destination far pointer, then source far
pointer. After `PUSH BP`, the pointers are at `[BP+6]` and `[BP+10]`.
The routine reserves three local words and reads the mode byte `DS:4A1E`
once. Record fields remain unnamed beyond their observed operations:

- Modes 0/2 interleave inline-byte OR at record offset `+17` and pointed-byte
  AND through the far pointer at `+13`. The source word at `+11` controls
  both vectors' iteration count.
- Mode 1 first ORs the inline bytes using that source count, then loads the
  live global far pointer `DS:5EB2`. Its record's word at `+11`, shifted right
  twice, controls the pointed-byte AND loop.
- Other mode bytes skip vector processing. AH and untouched registers keep
  their incoming values; the last byte CMP against 1 defines return flags.

Each count is decremented as a 16-bit word. The following unsigned comparison
of zero against that bound cannot skip the loop. Thus a zero source count, or
a mode-1 global count below four, produces **65536 iterations**, not an empty
operation. The three taken unsigned-zero `JA` outcomes are impossible for
this instruction sequence. All returning valid-mode paths end with a word
CMP of equal loop values, defining all arithmetic flags. No undefined flag
mask is used in parity checks.

Arguments, nested pointers, locals and the global pointer are reloaded at the
same source-order accesses. An OR can change the later global pointer; an AND
can change a later nested pointer or the far return. The loaded mode stays
selected even if an inline write changes its RAM byte. Aliased loop storage
can retain source nontermination; no host cap silently changes the C body.
Effective offsets/SP wrap at 16 bits. RETF reads IP and CS at separate stack
word offsets, including wrapping SP+2; each individual word and memory far
operand remains physically contiguous. Physical RAM wraps at 1 MiB with A20
disabled.

The API models flat RAM and immutable code. Hardware video-memory semantics
are not supplied by a raw byte array. The current native mode-1 EGA evidence
for other routines identifies the global's role, not this leaf's reached
arguments, allocation ownership or third-party-library provenance.

## Original-byte full-state and strict C tests

The audit checks the whole media hash, exact body extent, all instruction
starts, internal branches, both direct caller boundaries and all nine
upstream fixups before compiling C. **425 returning original/C cases**
compare every 1 MiB RAM byte and all fourteen CPU words, including full FLAGS.
They cover all 256 mode values, seeded data/register/flag states, stack and
array offset boundaries, A20 wrapping, shared DS/SS, both 65536-iteration
underflows, live nested/global pointer aliases, a mode-byte write after its
read, a mutated far return, and a return into the same body. The audit stops
before RETF. At SP FFFE/FFFF it applies the native-established two-word pop
rule because Unicorn reads CS incorrectly there; other offsets execute the
original RETF separately. This disclosed hybrid oracle avoids both an
unintended continuation and repeating the old C/Unicorn false positive.

All 121/121 instruction starts and 15/18 conditional outcomes are reached;
the three omitted outcomes are the impossible unsigned-zero `JA` branches.
The focused registered CTest checks independent OR/AND results, mode-specific
counts, unchanged source data, ABI/register/flag results, atomic invalid-input
guards, a nested-pointer write affecting the next LES, count underflow, and
all four mode routes at FFFD/FFFE/FFFF with distinct linear-CS canaries.
It passes with strict warnings and with ASan/UBSan (sandbox leak detection
disabled). The new CMake source/test additions were coordinated in i3462.

## Synthetic native frames

`u6f_vector_merge_032f_native.py` adapts the capture-before-DOS-I/O pattern from
[the prior u6A generator](../../tools/overlay/u6a_0824_synthetic_native.py).
It copies the hash-locked body unchanged into an ignored COM, at synthetic
offset `+1100`; the call-free relative branches remain relocatable. Native
frames use the actual copied entry and return addresses. Interrupts are
disabled while initializing and capturing the routine frame, then enabled
for DOS file output. Both direction-flag states are tested.

All **384 synthetic DOSBox-X far-call cases** pass: every mode byte plus
128 additional valid-mode vector cases. The comparator requires all fourteen
CPU words without a flag mask, all twenty caller/local frame bytes, both
55-byte records, both 32-byte pointed vectors, and the mode/global-pointer/
length fields to match C from identical inputs. Positive counts stay within
the initialized native buffers; underflows and alias cases are separately
covered by the original-byte full-RAM oracle.

| Ignored native artifact | SHA-256 |
| --- | --- |
| DOSBox-X executable | `99d7460153175d13a21036fb4b0e7524e05b6a1e50cd4e59f2470afad12a0df3` |
| Generated `ORACLE.COM` | `650eb0c6d6dbf1d6d2d53612bce9fea48f5cadac39f7d0705933423b5e46abc0` |
| 88710-byte `RESULT.BIN` | `5d51b85d60f56275fba389e43945cde31f48a8b7d78acbca72d29918c0413123` |

These are synthetic native frames, not natural saved-game reach or a full
native 1 MiB capture. A natural entry/return and ownership classification
remain unmeasured; the model and evidence add no self-credit.

## Reproduction and follow-on

```sh
PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn \
  python3 tools/overlay/u6f_vector_merge_032f_audit.py
PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn \
  python3 tools/overlay/u6f_vector_merge_032f_native.py
cmake -S . -B resources/build-u6f-032f \
  -DCMAKE_C_FLAGS='-Wall -Wextra -Wpedantic -Werror'
cmake --build resources/build-u6f-032f --target poolrad_overlay_u6f_vector_merge_032f_tests
ctest --test-dir resources/build-u6f-032f -R '^poolrad_overlay_u6f_vector_merge_032f_tests$'
```

The same-unit follow-on is the `+0482` caller/frame integration, now with its
two leaf dependencies available for exact comparison. It is not advertised
as a second call-free leaf: the only other call-free U6F prologue body is the
empty initializer `+23E0`, which remains excluded. Broader caller translation
and any ownership/count promotion require PM's separate review.

Original media, disassembly, generated COMs, emulator outputs and private
captures remain ignored under `resources/`; no extracted text, images or
original byte arrays are tracked.
