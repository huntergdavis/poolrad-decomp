# U1C:0126 COMPARE — candidate raw caller composition

This is candidate evidence for OVL_001C_0126, not a game-identity, strict-credit,
readiness or caller-unlock grant. The existing semantic COMPARE model remains
unchanged. The new raw caller composes the existing operand loader, slot reader,
SYSTEM bounded string copy, and already-credited raw string/word compares.

## Original and caller

| Item | Pin |
| --- | --- |
| START.EXE SHA-256 | d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd |
| GAME.OVR SHA-256 | 872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614 |
| Body | Unit 001C [0126,01A3), file offset 0E6C; 125 bytes, 50 instructions |
| Body SHA-256 | 330e74c38244df80bd2af04209126a207b5ebdf5dcf97c4781d4b960e2a52404 |
| Direct caller | Same unit's dispatcher: CS is pushed at 2F58, near call at 2F59 reaches 0126, continuation 2F5C |
| Entry / return | Far frame, no arguments, entry IP 0126; final far return at 01A2 |

The audit verifies both whole executable hashes, the entire body hash and decode,
all seven relocated far-call sites, four U25 overlay entries, and the dispatcher's
call sequence. It loads the original complete U1C and U25 units and the expanded
resident image, then installs already-loaded synthetic overlay stubs. No original
bytes or disassembly are embedded in tracked files.

The caller reserves 0204h local bytes and asks U25:0000 to load two ECL operands.
If either loaded tag at DS:7006/7007 is at least 80h, it copies the two existing
string buffers at DS:70C8/71C8 into separate stack locals, in order, then passes
those local pointers to U25:1815. Otherwise it reads slots 1 and 2 through
U25:0136, stores each result in its original local slot, and passes those words
to U25:18EB. Neither path substitutes a host string or comparison result.

The first tag test short-circuits the second. Each argument, memory operand,
saved register and return frame is read in original order. The buffer copies
are significant when data aliases the stack. AH is not normalized by the
caller's low-byte argument assignments.

## Interface reconciliation

Let S be entry SP before 0126 saves BP. In an ordinary preserved frame the
caller's reserved stack top is S-0206h. These are the actual callee entry frames:

| Caller site | Reached body | Arguments / callee entry SP |
| --- | --- | --- |
| 0130 | U25:0000 through loaded stub 002B:002A | One word, low byte 2; S-020Ch |
| 0152,0166 | SYSTEM:0B25 | Source pointer, destination pointer, limit FFh; S-0214h |
| 0177 | U25:1815 through loaded stub 002B:006B | First and second local string pointers; S-0212h |
| 0181,018C | U25:0136 through loaded stub 002B:0025 | One word, low byte 1 or 2; S-020Ch |
| 019A | U25:18EB through loaded stub 002B:0070 | First and second local values; S-020Eh |

The operand loader requires its entry SP at least 022Ah. Therefore S must be
at least 0436h; this also satisfies the string compare's minimum 0220h. The
caller admits S in **[0436h,FFFBh]**, including odd values, so its far return
does not wrap the stack offset. These calculations do not assume corrupted
frames remain preserved: reached helper refusals and altered returns are
reported, not converted into success.

The existing raw contracts are
[U25 loader](U25_LOADER_0000.md),
[U25 slot value](U25_SLOT_VALUE_0136.md),
[string compare](OVL_0025_1815_CPU.md), and
[word compare](OVL_0025_18EB_CPU.md). The authoritative API declarations are
the corresponding headers plus system_strstore_fullstate_0b25.h. The old
RAW_CPU_INTERFACE_CAVEATS remain unchanged; this PR proves a particular
composition candidate rather than removing a global readiness declaration.

## Execution contract and stops

RAM is exactly 1 MiB, flat and A20-off, separate from CPU, limits, diagnostics
and observer state. Effective offsets wrap at 16 bits; multibyte operands
are physically contiguous. Executed code and loaded SYSTEM/stub bindings
are immutable. There are no asynchronous interrupts, TF traps, hardware
aperture effects, arbitrary-IP resume or cold overlay loading in this contract.

Data/frame aliases are retained. The model reads each live call-segment word
and the loaded stub target. A call or stub guard refusal stops before the
original call's far frame is pushed. These negative controls compare at that
declared boundary, not against a completed original cold-loader path.

| Status | Meaning |
| --- | --- |
| RETURNED | Original far frame destination reached; all compared state retained |
| ARGUMENT | Invalid initial RAM/CPU/limits, entry IP, SP, or zero budget; CPU/RAM and diagnostic unchanged; C-only refusal |
| CODE | Reached caller call or loaded stub is not the expected entry; stop before the call |
| CALLEE | A composed helper refused or exhausted its declared budget; diagnostic identifies the helper/status and preserves reached state |
| TRANSFER | A helper or final return reaches an altered destination; state is exposed there |

Positive operand/string limits retain the loader's loop-header meaning. The
string-compare budget counts U25:1815 instructions, not its bounded SYSTEM
leaves. An oracle instruction safety limit is an audit failure, never a return.
The slot reader inherits the load-hook/party-index contract: cyclic party lists
may not terminate and have no synthetic success substitution. This caller has
no general termination proof.

## Observability and chosen cases

Before **every instruction of the 0126 body**, an optional read-only observer
records all 14 CPU words and SHA-256 of all 1 MiB RAM. The original hook records
the same boundary sequence. At the final stop, CPU words and all RAM bytes are
also compared directly. No registers, flags or RAM bytes are masked. The
observer may not mutate or reenter the guest; it is measurement, not a service.

This catches flags that a later callee overwrites. The wrong_sub_flags mutant
toggles all six arithmetic flags after the caller's local-frame subtraction.
In the initial smoke witness, final CPU/RAM still agree, but the next boundary
at 012D differs in FLAGS. Reversed word arguments and ignoring the second tag
are independent additional mutants. All three must be killed; a survivor fails
the audit. Mutant search stops at the first witness and is not a frequency.

The full suite enumerates **1,354 cases by construction**:

- 512 literal-byte cases: all 256 values separately in each operand position.
- 512 tag-byte cases: all 256 tags separately in each operand position, with
  specified zero payload and a fixed peer. These include retained/unrecognized
  tag behaviour, not an assumption that every tag is an ordinary game input.
- 25 literal-word pairs from 0,1,7FFFh,8000h,FFFFh.
- 256 string-length cases, covering every length 0..255 with equal contents;
  the branch/control/alias cases also include unequal strings.
- 49 baseline, branch, stack-edge, frame-alias, flags, A20, call/stub guard,
  operand-limit and compare-budget cases.

This is chosen dimension coverage, not the Cartesian product of CPU/RAM or
a measured frequency in play. The reduced suite has 108 cases and cannot
substitute for the full registered run. Seven additional C-only checks require
atomic entry refusal. Full audits require all 50 caller instruction starts and
all four outcomes of its two conditional branches; this is not a fresh coverage
claim for every transitive callee or an exhaustive caller/environment census.

The compiled source list is explicit. Cpu's layout is declared locally in this
audit so importing another checker cannot add unrelated source ownership.
Actual discover() must find exactly one owner for OVL_001C_0126, and the source
map must select exactly the twelve compiled production files without fallback.
The profile adds only this audit with empty args; removing that entry must
reproduce the parent profile. The existing credit_evidence_kind.csv is untouched.

## Validation and reproduction

The first 90-case development smoke passed with 50 starts, four conditional
outcomes, 67 returns, seven transfers, eleven code-boundary stops and five
callee-limit stops; all three mutants were killed. That run predates the added
tag sweep and atomic-refusal tests and is **not** acceptance of the final suite.

FULL_REGISTERED_RECEIPT_PENDING: the final registered execution and exact-head
media receipt will be recorded after they complete. The PR description carries
the exact-head media result; a compilation or smoke pass is not that result.

    python tools/overlay/check_checker_builds.py --execute OVL_001C_0126 --output resources/t144-0126/execution.json
    PATH=/path/to/venv/bin:$PATH tools/run_media_checks.sh

Use the shared poolrad-heavy.sh wrapper for heavy execution, one job at a time.
Only the exact-head media gate uses the VP-assigned credit priority.

Reused source/evidence: ecl_helpers.c's semantic COMPARE, FUNCTION_LEDGER.md
row 26, the existing U25 raw headers/models, and the 0AED caller audit's loaded
stub/oracle setup. Dunking Sheep memory context returned session
281c8700-5acc-438b-b93d-c061e28437eb as the 0126/interface-caveat lead; source and
current media were read directly. No Deja command was run.

| Review field | Scope |
| --- | --- |
| target_function | OVL_001C_0126, 125-byte COMPARE caller |
| source_candidates | New raw caller composed with existing raw U25/SYSTEM helpers; existing semantic model unchanged |
| signals | Body/media/call pins, dispatcher frame, explicit callee frames, boundary CPU/RAM observations, full final state, three mutants |
| score | Development smoke measured; full registered and exact-head gates pending |
| verdict | Candidate, independent review pending |
| review_notes | No accounting/readiness/credit promotion; reused callee limits remain load-bearing |

NOT VERIFIED: full final registered run until its receipt is present; exact-head
media until reported; native DOSBox parity; live dispatcher entry/continuation;
cold loader behaviour; arbitrary cyclic party-list termination; all-state
Cartesian parity; every transitive callee state; TF/interrupt/hardware effects;
all possible callers; independent verdict; identity, credit, readiness or unlocks.
