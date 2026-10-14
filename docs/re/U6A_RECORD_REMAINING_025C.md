# Unit 0x6A +0x025C record remaining-value helper

`pr_u6a_record_remaining_025c` translates the complete call-free routine
at `+0x025C..+0x0301`. It counts matching nonzero entries in a 21-byte
record list, then subtracts that count from a selected record byte.
Both the 16-bit AX result and the byte result in AL are preserved.

The required recall query was run as
`timeout 30s deja "Poolrad u6A 025C 166 byte alias audit"`; it ended
with status 124 and no result. The owner explicitly authorized continuing
after this bounded timeout. Reused repository evidence is the
[alias-audit lead](FUNCTION_ALIAS_AUDIT.md), the independent
[+0x0824 sort model](U6A_SORT_0824.md), and the verified/native
[active-character pointer mapping](U6F_PARTY_INSERT_1446.md).
The model and tests use separate files from pF's `+0x0824` work.
The audit reuses the CPU-layout and RAM utilities from
[unit 0x5E +0x2879](U5E_PARTY_VALUE_2879.md) and the synthetic native
oracle approach from [unit 0x5E +0x0E52](U5E_HALF_0E52.md).

## Extent and callable ownership

| Artifact | Verified bound |
| --- | --- |
| START.EXE | 65,231 bytes; SHA-256 `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| GAME.OVR | 206,105 bytes; SHA-256 `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| Unit 0x6A | GAME file base `0x17831`, resident stub segment `0x0075` |
| Leaf span | GAME file `0x17A8D..0x17B32`; **166 bytes, 64 instructions** |
| Leaf SHA-256 | `949abe77fa2823648d99cd3b76412560837646604ed9526f5f575dd8d3118b14` |
| Control flow | One unconditional jump, four conditional branches, zero calls; `RETF 4` at `+0x02FF` |

The original 272-byte prologue bound includes 106 bytes after the reached
return. They are excluded from this translation. All reached branches
remain inside the exact 166-byte body.

Two aligned `PUSH CS; CALL +0x025C` sites occur at `+0x05D9/+0x05DA`
and `+0x0A11/+0x0A12`, in bodies `+0x05A4` and `+0x090A`.
A descriptor-seeded CFG walk from public entry 1, `+0x1E6F`, reaches
the first through the following near-call chain:

| Call site | Target |
| --- | --- |
| `+0x1F93` | `+0x13F1` |
| `+0x147A` | `+0x090A` |
| `+0x0991` | `+0x05A4` |
| `+0x05DA` | `+0x025C` |

The audit checks the original entry descriptor and walks instruction
boundaries, rather than accepting raw E8 bytes as call evidence. This is
a static CFG path through conditional branches and returning callouts;
it does not prove a particular gameplay input traverses the whole path.

The `+0x090A` caller passes descriptor bytes from DS `+0x31FA/+0x31FB`
to this leaf and tests its result before appending a tagged byte to the
same list, then reaching the separate `+0x0824` sort. Both use the far
pointer DS `[0x5D92/0x5D94]`. The verified/native u6F insertion evidence
establishes that this pair points to the active character record.
That connects the leaf's data to a game record; the exact gameplay names
of the 21 list bytes, descriptors, and selected capacity byte remain
unmeasured. No spell, inventory, or attribute name is inferred.

## Complete ABI and ordered behavior

The API enters after CALL, with return IP/CS at SS:SP and two word
arguments at SP+4/SP+6. Only their low bytes are used. The routine saves
BP, reserves four bytes, initializes count `[BP-2]` and index `[BP-3]`,
and performs exactly 21 iterations, indices 0 through 20:

1. Reload the far record pointer and test record byte `+0x17+index`.
   An actual zero skips the entry; `0x80` remains eligible.
2. Reload the pointer and byte, mask with `0x7F`, and read descriptor
   byte DS `[0x31FB + 16*masked]`. Compare it to `[BP+8]`.
3. If equal, reload the pointer and byte again and compare descriptor
   DS `[0x31FA + 16*masked]` to `[BP+6]`. If both match, increment count.

After the loop, CX receives the count. Opcode `98h` in 16-bit mode
sign-extends each argument's low byte (CBW): DX receives `[BP+8]` and
SI receives `[BP+6]`. The source computes `3*SI+DX` with 16-bit wrapping,
reloads the record pointer, reads byte `record + 0xB1 + 3*SI + DX`,
zero-extends that byte, and subtracts CX as a **16-bit SUB**.
It copies AL through local `[BP-1]` without clearing AH. For example,
capacity 7 and count 10 return AX `0xFFFD`, rather than `0x00FD`.

The saved BP word and three local bytes remain in RAM. The unused fourth
local byte remains untouched. The function restores BP and executes
RETF 4, advancing the entry SP by eight bytes. BX, DS, and SS are
preserved; CX, DX, SI, DI, ES, AX, and all defined flags match the source.
DI contains the selected record offset before the `+0xB1` memory operand.
The final SUB defines all arithmetic flags, including AF; no flag is masked.

All SS locals, arguments, DS tables, record bytes, and pointer words
share A20-off 1 MiB RAM. Effective offsets wrap at 16 bits, while
multi-byte memory operands and the four-byte RETF frame cross offset
boundaries contiguously. Every LES reload occurs at its original site,
so stack/pointer/list aliases are retained. Only the loop INC writes
the index; there is no host loop cap or unresolved callback. Invalid
host RAM/CPU arguments are rejected before any mutation.

## Validation and parity review

The focused C test checks masked entries, actual zero versus `0x80`,
both descriptor gates, matching counts, zero result, subtraction borrow,
signed selectors, saved frame bytes, returned registers/flags, and
invalid host inputs. The original-byte audit passed **339 synthetic
cases**, comparing the entire 1 MiB RAM image and all 14 CPU words.
Cases include selector boundaries, pointer/local/descriptor aliases,
record and stack offset wrapping, A20 wrapping, and preserved flags.
All 64 instruction starts were reached.
Both outcomes of all four conditional branches were exercised: eight
conditional edges. The repeatable source audit also checks the public-entry
call chain described above. Both focused unit 0x6A CTests passed after a
build with `-Wall -Wextra -Wpedantic -Werror`. Public-tree media exclusion,
all copied-research SHA-256 checks, oracle shell syntax, and diff whitespace
checks passed.

A separate DOSBox-X normal-core/386 oracle executes the hash-locked
original leaf in an ignored synthetic COM. **256 cases passed** for
all 14 CPU words (including every flag bit), 13 stack-frame bytes, the
active pointer, the 21-byte list, and the selected byte. No flag mask is
used. The native result SHA-256 is
`5b2419698858f8a412a904d4ce9544f05d34f601e4765687747f436bbde114ea`.
Its header includes the DOS allocation segment, so different emulator
environments can produce a different result hash while preserving parity.

The synthetic native oracle proves leaf-level execution parity for its
fixtures. It is **not a saved-game call/return capture** and does not prove
the public-entry path is taken during saved gameplay. The active-record
role is supported by the static caller and existing native pointer
mapping; field names and live reach remain the explicit limits.
Original instructions, assembly, COM, native output/logs, and build files
remain under ignored `resources/`; public tests contain synthetic state.

```sh
cmake -S . -B resources/build-u6a-025c \
  -DCMAKE_C_FLAGS='-Wall -Wextra -Wpedantic -Werror'
cmake --build resources/build-u6a-025c --target \
  poolrad_overlay_u6a_record_remaining_025c_tests poolrad_overlay_u6a_sort_0824_tests
ctest --test-dir resources/build-u6a-025c -R poolrad_overlay_u6a --output-on-failure
PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn \
  python3 tools/overlay/u6a_record_remaining_025c_audit.py --unicorn --native
```

| Parity report field | Finding |
| --- | --- |
| `target_function` | Unit `0x6A:+0x025C..+0x0301` |
| `source_candidates` | `src/recovered/overlay_u6a_record_remaining_025c.c` |
| `signals` | Hash-locked extent and CFG/ABI, full-RAM/CPU differential, synthetic native oracle, public-entry caller chain and shared active-record pointer |
| `score` | 4/4 independent evidence checks passed; checklist rather than probability |
| `verdict` | `match` for the documented RAM/CPU contract |
| `review_notes` | Unknown gameplay field names; static CFG reach differs from live native reach; no self-assigned identity or strict credit |

Classification and strict accounting remain the PM's decision. No
classifier, identity ledger, or count is changed by this translation.
