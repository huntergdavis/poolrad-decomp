# Acceptance packet: OVL_001C_2E7D (reviewer receipt)

Independent reproduction by the review lane, for par-e's acceptance decision.
**This packet credits nothing.** It exists so the person who writes the
acceptance row is not also the person who measured it.

The author evidence is [U1C_TEXT_2E7D_CPU.md](U1C_TEXT_2E7D_CPU.md). Everything
below was re-derived from the media on current main, not quoted from that file.

## Own span, re-derived

| Check | Author doc | My measurement |
| --- | --- | --- |
| Span | `[2E7D,2F2D)`, 176 B | **176 B** |
| Instructions | 64 | **64** |
| Terminator | `RETF` at `2F2C` | **bare `RETF` at `2F2C`** |
| Body SHA-256 | `4c04ef11f1280fba…` | **`4c04ef11f1280fba187b51494683c9bda992c5ce66e4c57aa003fd2796d59cb0`** |
| Bytes after span | not stated | `55 89 e5` — the next prologue, so the span ends where it should |

## Original-byte differential, reproduced

`tools/overlay/u1c_text_2e7d_cpu_audit.py --unicorn` on current main:

| Field | Reproduced |
| --- | --- |
| cases | **177** |
| stops | OK 154, CODE 13, CALLEE 6, TRANSFER 1, LIMIT 3 |
| cpu_words | **14** |
| ordinary_ram_bytes | **983,040** |
| flag_masks | **0** |
| caller starts / total | **64 / 64** |
| caller outcomes / total | **8 / 8** |
| chain starts / total | **819 / 1,504** |
| chain outcomes / total | **81 / 264** |
| out_events | **5,075** |
| vram_write_operands | **9,548** |

Every figure matches the author doc. The doc's stop names were checked against
the harness enum rather than taken on trust: `OK=0, ARGUMENT=1, CODE=2,
CALLEE=3, TRANSFER=4, LIMIT=5`, so the run's `{2:13, 3:6}` **is** "13 CODE,
6 CALLEE" — the doc is right and my first reading of it was wrong.

## Native replay, reproduced

`u1c_text_2e7d_cpu_audit.py --native`:

| Field | Reproduced |
| --- | --- |
| cases | **48** (returning cases only, as declared) |
| cpu_words | **14** |
| flag_masks | **0** |
| ordinary RAM compared | 1,024 + 65,536 × 3 = **197,632** (stack, DS, font, data) |
| plane bytes | **262,144** |
| OUT events | **3,672** |
| machine / mode / core / cpu | ega / **0Dh** / normal / 386 |
| `RESULT.BIN` SHA-256 | **`9da383dc35335d614bc291b16b3be5d52c7d273006a473443beb23c68e28ed31`** |

The result hash is byte-identical to the doc's, which means the native capture
is reproducible rather than merely reported.

## Documented preconditions, checked in the harness code

Not merely asserted in prose — located in `u1c_text_2e7d_cpu_audit.py`:

- **Aperture excluded from comparison**: `ram[:0xA0000] + ram[0xB0000:]`, i.e.
  983,040 bytes with `A0000..AFFFF` masked. That is exactly the declared
  "initial DS/SS windows outside physical A0000..AFFFF".
- **SP range** `[023A,FFFC]`: exercised at `0x23A, 0x23B, 0x8001, 0xFFFB,
  0xFFFC`, i.e. both endpoints and both sides of the boundary.
- **Budgets and `system_cs`**: `system_cs`, `max_operands`,
  `max_string_iterations`, `max_loopbacks`, `max_glyph_loopbacks` are real
  structure fields, and loaded SYSTEM bindings are required to agree with
  `system_cs`.
- **Aliasing cases** exist for `DS:84C8/84DE/5E26/5E2C` plus altered far
  returns, matching the doc's claim that stack locals and cursor globals stay
  live under supported aliasing.

## Checker condition (this packet's only change)

`OVL_001C_2E7D`'s audit had **no `--compile-only` mode and no `CHECKER_ROWS`
registration**, so the row was not creditable on current main regardless of how
good the evidence is. This packet adds both (`CHECKER_ROWS =
('OVL_001C_2E7D',)`, a `--compile-only` branch that builds and binds the same
shared library without touching media). Verified:

- `u1c_text_2e7d_cpu_audit.py --compile-only` → `CHECKER_BUILD_OK`, exit 0.
- `test_checker_builds` → 20 tests OK, so the CI build gate stays green with the
  row registered.

## Limits I accept, and limits I did not re-derive

Accepted as declared in the author doc, and reproduced here in substance:

- **Full caller coverage is not exhaustive callee coverage.** U25 alternatives
  are unexercised; the glyph `UpCase` path is unreachable with transform 0.
- **Native covers returning cases only** — not the original harness's error,
  budget, refusal, or alias stops.
- **The harness bug is disclosed, not tidied away**: adding a no-op Unicorn
  memory-read hook with shared mapped RAM altered the observed FillChar `RETF`
  destination, and the resolution was to keep the write-only observer rather
  than patch an instruction to compensate. This is a real limitation of the
  differential — it does not establish an all-reads aperture audit.
- **Natural gameplay reachability and arbitrary interrupt behaviour unproven.**

**Scope of this packet: the original-byte differential and the native replay
only.** The ASan/UBSan run (23,076 checks) and the focused CTest were not
re-derived - they need the CMake build rather than the audit harness - and this
packet makes no claim about them. Re-running them to fill the sentence would be
worth less than naming the boundary.

## Verdict for par-e

Everything I measured matches. The row is **ready for an acceptance decision**
with these caveats named: native evidence is returning-cases-only, the read
observer limitation is inherent to the harness, and the ASan/CTest numbers are
the author's, not mine. Credit stays with par-e; my reproduction is attached so
the count is not taken twice from one measurement.
