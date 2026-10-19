# U38 +11F9: active-character string comparison loop

Public entry 39 saves a linked string pointer, checks two active-character
fields, and compares that string against eight DS table strings. Each equal
comparison subtracts four from DS:6822. The complete caller now composes the
merged raw SYSTEM string comparator, preserving CPU14, live RAM ordering,
flags and partial effects at bounded stops.

## Exact executable identity

The queue's [11F9,1276) extent is 125 bytes and decodes to 51 instructions
if adjacent data is treated as code. Actual executable code ends at RETF
0Ah at +1265: **[11F9,1268), 111 bytes / 39 instructions**, GAME.OVR file
[0xFE54,0xFEC3). The final **14 bytes [1268,1276)** are Pascal constants for
the following entry and are excluded from the executable hash and coverage.

| Input/span | SHA-256 |
| --- | --- |
| START.EXE | d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd |
| GAME.OVR | 872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614 |
| U38 [11F9,1268) | 9f175ddec9eef17cf10700899a26277932c64394b7caef4abcf666eb3ce10438 |
| Adjacent [1268,1276) data | eb19eb8681d04b84bfe569f341ba3b2de215ff19ee7c51282198074a48c57ae4 |
| SYSTEM [0C10,0C3B) | 11e2860118cb471f9f1ecb1d0c44c060e0fbb83862f69c1cf39b5e0484298a58 |

The only call is LCALL 0AF8:0C10 at +1250, with loader fixup +1253.
The 43-byte / 20-instruction comparator ends with RETF 8 at +0C38.
C checks the reached LCALL opcode/offset, reads its loaded segment live,
pushes the original far frame and calls pr_system_strcmp_0c10. The oracle
asserts the entry, code/data hashes, decoded extent, call and fixup against
owner media. A relocated comparator body is included in original-byte tests.

## ABI and ordering

Entry SS:[SP+0Ah]/[SP+0Ch] supplies the far record pointer A (offset/segment)
among five word arguments. After PUSH BP; MOV BP,SP; SUB SP,2, the caller
follows A+108h to B and reads B+0Ah/+0Ch. It stores that far string pointer
P at DS:6826/6828 before loading the active character through DS:5D92/5D94.
Byte active+9Fh must equal 1 and (byte active+6Ch & 7Fh) must equal 2.
Either failed comparison returns with its flags and all earlier effects.

The eligible path initializes byte SS:[BP-1] to 1. Each iteration reloads
P, pushes it, reads the live index, clears AH, shifts its word value left
four bits, adds 03F2h, then pushes DS and that table offset. The raw helper
compares Pascal strings, clears DF and returns complete register/flag state.
If equal, SUB BYTE DS:[6822],4 executes before the caller rereads its local
index for CMP BYTE [BP-1],8. On a taken loop-back, INC BYTE [BP-1] executes.
The routine exits through MOV SP,BP; POP BP; RETF 0Ah.

The local index can alias the result byte or saved pointers. For example,
SS=DS and entry SP=6825 places the counter at DS:6822. With all table
strings empty and equal, subtracting four and then incrementing changes
the usual eight-comparison route into **168 comparisons** before the live
counter equals 8. The original oracle and independent semantic simulation
both exercise this route and a cap of 16 that stops after 17 comparisons.
Saved pointers, stack, strings and return-frame aliases remain live.

The API requires exactly 1 MiB A20-off RAM, initial IP=11F9 and
SP in [0010,FFF2], original immutable loaded code/binding, and CPU/status
storage distinct from RAM. Operands use 16-bit effective offsets and
20-bit physical addresses; word/LES pairs read physically contiguous bytes.
Execution is synchronous; interrupts, faults and arbitrary-PC resume are
outside the profile. The four-bit shift's undefined flags are overwritten
by ADD before any observable stop; the comparator's flag contract applies.

max_loopbacks counts taken JNE loop-backs and stops at +1236 before INC.
Normally seven suffice; zero permits the first comparison. LIMIT retains
all prior effects and the last comparator status. CODE stops at +1250
before a malformed reached LCALL, with four argument words already pushed.
ARGUMENT changes no CPU/RAM; nullable callee_status starts at -1. CALLEE and
TRANSFER retain failed/changed dependency state as defensive guards, but
the current read-only comparator cannot produce those stops under this
profile. There is no rollback.

## Measured evidence

- **692 original-chain cases**, comparing all **14 CPU words and 1 MiB RAM**
  without masks: 687 returns, two CODE stops, three LIMIT stops.
- **39/39 caller starts and 8/8 conditional outcomes**; including SYSTEM,
  **59/59 composed starts and 14/14 Jcc outcomes**. These coverage counts
  do not constitute exhaustive input or alias proofs.
- **64 DOSBox-X normal/386 returning cases**, 37 with at least one match.
  CPU14, the 256-byte stack window and all 65,536 DS bytes match unmasked.
- **4,392 ASan/UBSan semantic checks** and focused CTest **1/1 passed**.
- Native RESULT.BIN SHA-256:
  275d470c5a64a2cc08e869a994e6398c52834a6c7eea85b4081645d195342861.

The original corpus includes all 256 marker values, all 256 kind values,
random records/table strings, equal/unequal/empty/prefix/long strings,
relocated SYSTEM code, all caller branches, exact and short caps, odd and
boundary stacks, SS=DS, A20 wrapping, pointer/string wrapping, randomized
RAM and alias cases involving the counter, result, saved P and return frame.
Semantic checks independently enumerate all 256 matching-slot masks and
calculate modulo-byte decrements, both gates, alias-loop evolution, budget
stops, pushed frames and argument guards. Original differential checks the
callee diagnostic is -1 or 0; semantic cases assert the expected value.

Native replay uses synthetic entry frames with IRQs masked during execution.
It measures returning paths; CODE/LIMIT and the extended alias loop are
covered by the original differential and semantic corpus. It does not prove
natural gameplay reach. The normal-core COM runs the pinned comparator at
0AF8:0C10 (COM offset 3A50h), with caller bytes copied to 8800:11F9.
Original media, decoded bytes, generated assembly/COM, synthetic data blobs
and captures remain under ignored resources/.

With owner media under resources/DOS/POOLRAD and Capstone/Unicorn installed:

    python3 tools/overlay/u38_character_loop_11f9_audit.py --unicorn --native --output resources/u38-11f9/report.json
    cmake -S . -B resources/u38-11f9/build -DBUILD_TESTING=ON
    cmake --build resources/u38-11f9/build --target poolrad_overlay_u38_character_loop_11f9_cpu_tests
    ctest --test-dir resources/u38-11f9/build -R '^poolrad_overlay_u38_character_loop_11f9_cpu_tests$' --output-on-failure

Private receipt resources/evidence.json SHA-256:
5fa738432ecdfd5a27f6f5f9a5f9c2eb16241bc487a952d6bf64cbf0b5ef7508.
It pins 19 public build/tool/queue inputs, 19 private artifacts,
media/tool binaries and 14 unchanged dependency files against main
5438ae2f027244b0a258e9ecca5a1826de3c5a02. The document is excluded from its own receipt.

## Reuse and queue caveats

Pre-implementation recall (deja "U38 11F9 1276 FillChar composition")
returned no indexed match. Source inspection corrected that initial helper
hypothesis to SYSTEM string comparison +0C10. The unpublished branch
codex/fullc-u38-11f9 at 528fc2ce31d952cb6febd289e1cc7c9785e234fe supplied
body/fixup/ABI provenance, including the trailing-data boundary. Its inline
comparator and cached pre-subtraction loop index are not reused here.
Implementation and oracle reuse follow merged
[U38 +0679/+0947](U38_STRING_MATCH_CPU.md) and
[SYSTEM string/move leaves](SYSTEM_STRING_MOVE_LEAVES.md). The shared
comparator source and headers are linked unchanged.

The requested schema-4 queue is now exported as schema 8. This target was
rank 73, next after submitted #1066/#1067. The --no-git export omitted the
old unpublished branch, which explicit branch/all-state PR checks found.
No existing +11F9 PR was found. The extent and ownership caveats were sent
to the VP before authoring. The classifier/queue ledgers are not edited.

- target_function: OVL_0038_11F9, public entry 39, executable [11F9,1268).
- source_candidates: overlay_u38_character_loop_11f9_cpu.c composing
  unchanged pr_system_strcmp_0c10.
- signals: original hashes/entry/bounds, call/fixup, two active-character
  gates, live alias ordering, CPU14/full-RAM comparison, native replay,
  independent semantic assertions.
- score: five evidence categories (identity, structure, differential,
  native replay, contract checks); no probabilistic score.
- verdict: likely_match within the stated profile; independent review pending.
- review_notes: 14 data bytes excluded from queue extent; bounded loop;
  synthetic native frames and no natural-game reachability claim.
  No acceptance, accounting or credit changes.
