# RES_0802:094A: isolated evidence for the existing selector-push leaf

This is an evidence candidate for the **19-byte local body**, not a credit,
identity, readiness, or caller-unlock grant. The candidate already exists as
static selector_push in
[src/recovered/overlay_u25_store_word_0c77.c](../../src/recovered/overlay_u25_store_word_0c77.c).
The audit compiles that unchanged production source with a test-only entry
wrapper. It does not supply a second implementation or change the production
API. The reviewer decides parity and ownership; any credit grant belongs
in a separate non-author PR.

## Original and candidate

| Input/body | Offset or extent | Length | SHA-256 |
| --- | --- | ---: | --- |
| Owner START.EXE | packed file | 65,231 | d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd |
| Owner GAME.OVR | overlay file | 206,105 | 872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614 |
| Selector push | loader-expanded image [0896A,0897D), 0802:[094A,095D) | 19 | 9cb0afed6f73df887b61f47778db5d55bd9063b59c8f6067ab0c38ec3350d205 |

The checker uses resident_loader_map.unpack, verifies contiguous copied
provenance at packed file [089A1,089B4), rejects fill or a relocation inside
the body, and requires eight complete instructions, no call or branch,
and RETF 2. No original bytes or decoded text are published.

The body saves BP, establishes its frame, reads DS:5376, writes that byte
to DS:5377, then reads the argument low byte at [BP+6] and writes it to
DS:5376. It restores BP and far-returns, consuming one argument word.
AH and flags are preserved. **The argument read occurs after the save
store.** A save-destination/argument alias changes the subsequently read byte.
Data aliases with saved BP or the return frame can change restored BP or
the return destination; the comparison includes those effects.

## Caller and execution contract

Entry is immediately after a far CALL: IP 094A; SS:SP contains return IP,
return CS, and the argument word. Entry SP is **0002..FFF9 inclusive**,
so PUSH/POP/frame access and final SP+6 do not wrap the stack offset.
This is an audit limit, not a new production guard or proof about live
callers. The known Unicorn/DOSBox far-return stack-top disagreement is
outside this interval.

RAM is a writable 1 MiB A20-off image, also mapped at the high alias in
Unicorn. The C CPU state object is separate from guest RAM. Code remains
immutable. DS/SS and data/frame aliases are admitted.
The body uses load-base 1000, 4000, or 8000 (CS 1802/4802/8802), choosing a
placement disjoint from case data and stack writes. A colliding swept input
is preserved by relocating the body, not omitted.

Declared flags have bit 1 set; CF/PF/AF/ZF/SF/IF/DF/OF, both IOPL bits,
and NT vary independently; all other bits are clear. TF and interrupt delivery are excluded. IF is
tested as preserved state, with no asynchronous interrupt injected.
Flat RAM does not emulate hardware memory devices.

The checker pins two documented relocation-backed overlay callers,
including their LCALL target and segment fixup:

| Caller | Call / segment fixup | Caller body bytes / SHA-256 | Documented pushed word |
| --- | --- | --- | --- |
| 0025:0A74 | 0C03 / 0C06 | 515 / 258b587e214213205991b5eb810131948000e266d244bf086a7b427bb8bf0938 | 0300 OR (value AND 00FF) |
| 006F:0482 | 048C / 048F | 540 / a939d6fc0f5514a7efd9e9a08cbff48ef14ff41eddd13652284fd381489240aa | low byte 3; inherited AH |

Argument reconstruction is reused from
[RESIDENT_DAX_SELECTOR_PUSH.md](RESIDENT_DAX_SELECTOR_PUSH.md). This audit
pins complete caller bodies and their call/fixup sites but does not execute
caller paths. Every argument word is admitted, so parity does not depend on
these two argument shapes. No live caller SP, continuation, or closure is certified.

## Cases chosen by dimensions

Default execution enumerates **600,505 cases by construction**. Counts describe
test coverage, not frequency during play. The deterministic generator records
a digest over every group/input tuple.

| Group | Selection rule | Cases |
| --- | --- | ---: |
| BP, DS, SS, AX | Every 16-bit value for each word separately | 262,144 |
| Return IP, return CS, argument word | Every 16-bit value for each word separately | 196,608 |
| Entry SP | Every admitted value 0002..FFF9 | 65,528 |
| Flags | All 2,048 combinations of the eleven admitted bits | 2,048 |
| Prior DS:5377 | Every byte, checking independence from overwritten state | 256 |
| Prior selector x argument low byte | Full 256 x 256 product; argument high byte A5 | 65,536 |
| Frame aliases | DS:5376 aligned to every byte from entry SP-2 through SP+5; separately sweep old selector and argument low byte | 4,096 |
| Different-segment/A20 aliases | Two segment geometries x eight frame-byte positions x 256 complementary byte values | 4,096 |
| BX/CX/DX/SI/DI/ES | Walking-one and walking-zero sentinels for each bit | 192 |
| Baseline | Distinct register/frame/data sentinels | 1 |

Each case resets all 14 CPU words and identically seeds every RAM byte the
body can read before writing. The frame is seeded before the data bytes;
when physical bytes overlap, the final data seeds win. Overlapping bytes
cannot independently hold both logical inputs. The normal selector/argument
product does not overlap the frame. Both RAM images initially repeat 0..255;
background state then persists only after a successful full-RAM comparison.
Identical input/code writes preserve their equality before the next case.
An engine refuses another case after any mismatch; each mutant starts a new
engine. No RAM byte is masked or excluded from comparison. A separate
fresh-RAM mode cross-checks alias, A20, stack-edge, flags, and code-placement
fixtures, and an injected unrelated-byte store proves divergence stops reuse.
Unread background bytes are not exhaustively varied.

The original runs all eight instructions, including far return. The C runs
the existing static helper. Every case compares **14 CPU words and all
1,048,576 RAM bytes, with no masks**. A mismatch fails the run. The reduced
2,773-case --smoke mode checks the harness; it cannot replace the default sweep.

This is exhaustive **one dimension at a time** with specified byte/alias
products, not the Cartesian product of all state. CS placements and unread
preserved registers are not exhaustive. Other higher-order interactions
remain unverified.

## Mutants and fail-closed checks

Private compiler inputs derive both mutants from the same production source:

1. flip_cf toggles CF after the helper returns. Equal RAM must not hide the
   flags mismatch.
2. early_argument moves the argument read before the prior-selector save,
   retaining the old selector for the save. Nonalias cases can match;
   a save-destination/argument alias must kill it.

The audit requires both mutants to differ and records each first deterministic
witness: group, synthetic inputs, CPU differences, and RAM equality.
A survivor fails the audit. Media tests also require complete dimension
enumeration and refusal of an incorrect media identity.

## Ownership and dependency limits

The 0025:0A74 caller is a verified legacy game identity. The body directly
operates on DS:5376/5377, whose selector/save role and caller arguments are
documented in [RESIDENT_DAX_SELECTOR_PUSH.md](RESIDENT_DAX_SELECTOR_PUSH.md).
[RESIDENT_DAX_FILE_OPEN.md](RESIDENT_DAX_FILE_OPEN.md) traces the consumer of
5376 to a configured path record at DS:2614 + 1E * selector.

These are two-factor identity **evidence offered for review**, not an
authorship ruling. The older document leaves game/library ownership unresolved;
this PR does not silently change that disposition. Downstream I/O use alone
does not prove game ownership.

The body has no callees. Its callers remain incomplete: 0A74 still reaches
03BD:0E08, and the 0C77 candidate stops before those three external calls.
Admitting or crediting 094A alone does not unlock any of those bodies.

## Validation receipt

The registered differential returned **AGREE, exit 0**, at
b598dada0a050bb5812a8cc002fb50f44d3e5754 on 2026-10-25:
**600,505 cases by construction**, 14 CPU words and the full 1 MiB compared
without masks, in **932.256 seconds**. The seven focused regression tests
also passed, including the fresh-RAM cross-check, poisoned unrelated byte,
bad-media refusal, complete dimension enumeration, and exact source selection.

The full parameter-sequence digest is
e7074448ad1cf467adf545b5358d07f0f308729d60310adce2919aab410cb5ec.
The production candidate digest is
c4d3cd66df8ab08c2cd9c64ec1d7fd13e42cb9bec5db2db05816f928868073b4;
the audit digest is
d8100cc023a751958372374dcdadf52ef40b964e1f1e3de61888b250224f9c10.
Toolchain: Python 3.14.4, Capstone 5.0.7, Unicorn 2.1.4, GCC 15.2.0.

Both required mutants were killed. The CF mutant fails the first baseline
case: original flags 0ED7 versus mutant 0ED6, with identical RAM. The ordering
mutant's first witness is case 2,450 of the reduced deterministic mutant
search: DS=SS=2000, entry SP=5373, old selector=00. The original returns
AX=A600, CS=0000; the mutant returns AX=A6C3, CS=C300 and different RAM.
DS:5377 aliases the argument byte, and DS:5376 aliases the high byte of the
return CS. Mutant search stops on the first mismatch; it is not an exhaustive
mutant run or a measurement of natural reachability.

Private receipts are retained under resources/t144-094a/. The final PR
description carries the required owner-media receipt for its exact head;
the focused differential alone does not establish that gate.

Run on the owner-media host, one heavy job at a time:

    ../poolrad-heavy.sh /path/to/python tools/overlay/res0802_selector_094a_cpu_audit.py --progress --output resources/094a/full.json
    /path/to/python tools/overlay/check_checker_builds.py --execute RES_0802_094A
    PATH=/path/to/venv/bin:$PATH tools/run_media_checks.sh

The execution profile adds one key. Removing it reproduced the parent JSON;
deleting its one added line also reproduced the parent file byte-for-byte.
Both checks passed. The execution gate selects exactly the existing production
source, with no conservative fallback to unrelated sources. No accounting,
identity, readiness, or production source changes.

| Parity report field | Finding |
| --- | --- |
| target_function | RES_0802:094A, 19-byte selector-push body |
| source_candidates | Existing static selector_push in overlay_u25_store_word_0c77.c |
| signals | Loader/body/caller pins, no-callee shape, exact frame/alias order, CPU/RAM differential, flag and ordering mutants |
| score | Pinned body/callers; 600,505-case agreement; both required mutants killed; seven regression tests passed |
| verdict | likely_match within the stated contract; independent reviewer verdict pending |
| review_notes | Only this body under the stated contract; no credit or caller unlock |

NOT VERIFIED: all-state Cartesian parity; TF/interrupt delivery; excluded
stack offsets; mutable code; native DOSBox parity; live caller SP/continuation;
natural game reachability; all possible callers; ownership/credit;
caller/transitive closure or unlocks. Historical broader store-word
results are context, not rerun by this isolated audit.

Required Dunking Sheep recall was attempted on 2026-10-25 and timed out;
memory status reported a healthy store. No recall result was reused.
The existing source and linked evidence documents are the reused inputs.

