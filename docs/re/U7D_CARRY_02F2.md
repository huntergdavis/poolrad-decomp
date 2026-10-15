# Unit 0x7D +0x02F2 complete carry normalizer

`pr_u7d_carry_02f2` translates the complete call-free routine
`+0x02F2..+0x03A2` into a separate 1 MiB RAM/CPU model. On disjoint
state it normalizes seven array words in ascending order against DS
`0x363A..0x3647`. Each of the first six threshold crossings increments
the next word and subtracts the current threshold once. The seventh
crossing increments word `+0x30` of each party member; it leaves the
seventh array word unchanged. Words wrap at 16 bits.

## Prior work and game ownership

The bounded recall query
`timeout 30s deja "Poolrad U7F 05E0 U7D 02F2 U77 27DF full C claimed translation"`
ended 124 without results. Reused evidence is cited here rather than
treating the recall timeout as a duplicate check.

The existing [entry-1 model](U7D_ENTRY1.md) includes
`pr_u7d_carry_normalize`, a host-array/node helper in
`overlay_u7d_entry1.c`. Its list guard rejects chains longer than
`PR_U7D_LIST_CAP`; its API does not model shared DOS RAM, local/argument
aliases or returned CPU state. The existing [internal +0000 model](U7D_ZERO_CALLEE.md)
also remains bounded. Inspection of current-main recovered sources
found no independent full-RAM/CPU translation of `+0x02F2`.
Those existing models and their APIs are left intact.

Public entry 1 `+0x03D5` copies the seven window-0 words
`+0x018C..+0x0199` into its own local array, increments the selected word,
then passes the far local-array pointer to this routine with aligned
`PUSH CS` at `+0x042E` / `CALL +0x02F2` at `+0x042F`.
The [coordinate writer](U68_COORDINATE_WRITER_073B.md) calls that public
entry through stub `0x008C:0x0025`. This establishes game ownership
without claiming a measured saved-game route.

This routine's seventh carry reads the independently documented
[party head and links](RESIDENT_PARTY_COUNT.md), DS `0x5D96/0x5D98`
and member `+0x104/+0x106`, before incrementing member word `+0x30`.
The field's gameplay meaning is not inferred. The exact routine has
**zero calls, no external callbacks and no unresolved callee**.

## Exact original and ABI

| Artifact | Verified bound |
| --- | --- |
| START.EXE | 65,231 bytes; SHA-256 `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| GAME.OVR | 206,105 bytes; SHA-256 `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| Unit 0x7D | GAME file base `0x23DCC`, public stub segment `0x008C` |
| Function | GAME file `0x240BE..0x2416E`; **177 bytes, 69 instructions, zero calls** |
| Function SHA-256 | `8f39276ad357b6e05367ffc080afedc0df5a5cca2f3cbf54b3d96db18bea3dd9` |
| Exit | `RETF 4` at `+0x03A0`; exact end `+0x03A3` exclusive |

The API enters after CALL. The four-byte far argument is at
SS:entry-SP+4. The routine saves BP and reserves six bytes. Its byte
index is `[BP-1]`; the party pointer occupies unaligned word locals
`[BP-5]` and `[BP-3]`. It returns through the four-byte frame and
discards four argument bytes: final SP is entry SP + 8 modulo 65536.
BX, CX, SI, DS and SS are preserved; BP, CS and IP reload from RAM.
AX, DX, DI and ES reflect the last reached instructions.

The byte index is sign-extended before indexing. The array and its far
argument are reloaded on each original access. In a carry, incrementing
the next element precedes reloading the index, threshold and current
element. The resulting subtraction is retained in DX while the array
pointer is reloaded again for the store. No values are cached across
potentially aliasing writes.

The party path likewise stores both pointer words in RAM. After the
member counter increment, it repeats LES before reading the member's
next link: the counter can alias and change those pointer locals.
The directed `party-local-reload-alias` case exercises that behavior.

Effective offsets wrap at 16 bits. Word accesses, LES operands and
the far return frame cross offset boundaries contiguously; physical
addresses wrap at 1 MiB with A20 disabled. The memory primitives reuse
the reviewed [adjacent party model](U6A_PARTY_AFTER_1601.md).

Every returning path ends with the defined arithmetic flags from
`CMP byte [BP-1],6` equal. Thus PF and ZF are set, CF/AF/SF/OF are clear,
and the other incoming flag bits are preserved. Earlier arithmetic
flags have no observable effect on the returned state. No flag is
masked in either parity comparison.

Malformed/cyclic party state or aliases can prevent source return.
The C model retains the original traversal without a host cap; its
parity claim covers terminating source paths. Null RAM/CPU or a RAM
size other than exactly 1 MiB is rejected before mutation.

## Validation and parity review

The independent focused C test checks **256 cases**: all 128 carry
masks across two member segment layouts, empty through six-member
parties, one-step subtraction and propagation, counter wrap from
`0xFFFF`, unchanged seventh word, preserved state and invalid host
arguments. Both the new test and the existing bounded caller's test
pass with `-Wall -Wextra -Wpedantic -Werror`.

The original-byte oracle passes **258 returning cases**, comparing
the complete 1 MiB RAM and all 14 CPU words. Returning cases cover all
69 instruction starts and both outcomes of all four conditional
branches. Cases include every carry mask, random thresholds/values,
zero/max thresholds, stack and array boundaries, A20 wrapping,
array/table and array/frame aliases, the party-local reload alias,
DF and incoming arithmetic flags.

Two additional malformed fixtures with entry SP `5` and `6` fail the
original's 20,000-instruction return budget. The separately stored
pointer words wrap through offset zero, while the subsequent LES
reads its four bytes contiguously past offset `0xFFFF`. Their original
return-budget failure is checked explicitly; the uncapped C function
is never invoked on them. Empty-party counterparts return and are
included in the 258 state comparisons. Coverage excludes these
nonreturning fixtures. The budget failure alone is not proof of
nontermination for arbitrary states.

The separate DOSBox-X normal-core/386 oracle passes **128 synthetic
cases**, all carry masks, with exact comparison of all 14 CPU words,
32 frame/sentinel bytes, the seven threshold words, seven array words,
party head and all six member counters/links, including unused members.
The result SHA-256 is
`9046dc83a76cfa65f84c2d2b2c26c85cd247a124a5d87429d4605b51da333d40`.
Its header includes a DOS allocation segment, so another setup may
produce a different result hash while preserving parity.

Native leaf execution enters with FLAGS `0x0002` and hardware
interrupts disabled from frame priming through snapshot capture.
Interrupts are restored before DOS file output. This is part of the
synthetic input contract; every returned flag bit is still compared.
The initial IF-enabled run agreed on all CPU words but an asynchronous
interrupt changed one case's unused frame bytes. Its stacked return
points inside this leaf at `+0x0316`. That diagnostic result remains
ignored with SHA-256
`3099eec61dfeac380f6c73f2eae24000ff3169235a5a24e95e29bb2bb50fdfc5`.
A bounded recall query for that native stack mismatch also timed out
without results. Interrupt isolation removes external stack writes
from this leaf-only comparison rather than masking any RAM or flags.

The native oracle runs a copied hash-locked leaf in a synthetic COM;
it establishes no saved-game call/return route. The original bytes,
generated assembly/COM, logs/results, diagnostic and build files stay
under ignored `resources/`. Public fixtures are synthetic. The
oracle reuses compilation/full-state comparison infrastructure from
the [party reorder oracle](U6A_PARTY_BEFORE_14A9.md) and
[unit 0x5E utility](U5E_PARTY_VALUE_2879.md), with separate U7D
fixtures, source audit and native generator.

```sh
cmake -S . -B resources/build-u7d-02f2 \
  -DCMAKE_C_FLAGS='-Wall -Wextra -Wpedantic -Werror'
cmake --build resources/build-u7d-02f2 --target \
  poolrad_overlay_u7d_carry_02f2_tests poolrad_overlay_u7d_entry1_tests
ctest --test-dir resources/build-u7d-02f2 \
  -R 'poolrad_overlay_u7d_(carry_02f2|entry1)_tests' --output-on-failure
PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn timeout 60s \
  python3 tools/overlay/u7d_carry_02f2_audit.py --unicorn --native
```

| Parity report field | Finding |
| --- | --- |
| `target_function` | Unit `0x7D:+0x02F2..+0x03A2` |
| `source_candidates` | `src/recovered/overlay_u7d_carry_02f2.c` |
| `signals` | Exact hash/CFG and caller, independently documented game state, full-RAM/CPU differential, independent carry fixtures and native execution |
| `score` | 4/4 evidence checks passed; checklist rather than probability |
| `verdict` | `match` for terminating source paths under the documented RAM/CPU contract |
| `review_notes` | Malformed traversal can fail to return; two original return-budget failures excluded from C; native interrupts isolated; saved-game route unmeasured |

No classifier or count ledger is changed. Strict credit and identity
remain the PM's review decision.
