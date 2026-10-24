# RES_00BA +23DB: raw-CPU model of the party count (no new credit)

`RES_00BA_23DB` is **already credited**. Its credit rests on the semantic translation `pr_rec_res_00ba_23db`
([RESIDENT_PARTY_COUNT.md](RESIDENT_PARTY_COUNT.md)), which wR:p1's #1482 checks against the original bytes
(`res00ba_rec_models_audit.py`). That model is an io-callback interface, not a CPU.

This change adds the **raw-CPU, full-state model** the hybrid executor needs. Route 3 reaches this body 181 times (codex's glue-gap census,
i22516). It is the 23B7/2E03 pattern. **No credit or ledger change is proposed.** Assigned by par-a (i22518), claimed in i22523.

**Pre-claim grep** (VP i22025, widened by PM i22086), on main 015a1741: `grep -rniE '(\+|0x|:)23DB\b|RES_00BA_23DB'` over
`src/recovered`, `tools/overlay`, `include` and the docs CSVs. Every hit was opened:
- the semantic `pr_rec_res_00ba_23db` (`resident_party_count.c/.h`);
- wR:p1's io adapter;
- the rec-models audit's span table;
- the queue's interface-caveat note;
- two 23B7 audits that name 23DB only as their END bound.

No raw translation existed.

## Executable extent

START.EXE resident segment 00BA **[23DB,2433), 88 bytes / 29 instructions**, from the relocated image of
`res0899_quit_0000_cpu_audit.load_image`. There is no relocation in the span, and no call, interrupt or port. The one return is `RETF` at
2432; there are no arguments.

## What the body does

1. `PUSH BP; MOV BP,SP; SUB SP,4`.
2. `DS:[6814] := 0`, `DS:[6815] := 0`.
3. The local far pointer `[BP-4]` := the party head `DS:[5D96]/[5D98]`.
4. **Loop** while the pointer is not null. The test is `OR` of its two words, so a null segment with a non-zero offset is not null:
   - when the character's byte `+10Dh` is non-zero: `INC BYTE DS:[6814h + CBW(byte +10Eh)]`. The index is signed, so it can reach any
     byte from 6794h to 6893h, beyond the initialised pair;
   - the pointer := the character's link `+104h/+106h`.
5. `MOV SP,BP; POP BP; RETF`.

A cyclic party list never ends, as in DOS.

## Role

The body counts the party's characters into two byte counters, selected by a signed byte field. The role is the credited one; the meanings
of `+10Dh` and `+10Eh` stay unassigned, as in the semantic model's header.

## Raw contract and stops

`pr_res_00ba_party_count_23db_cpu(ram, size, cpu, budget)`:
- exactly 1 MiB of A20-off RAM, flat;
- IP 23DB; entry SP in `[0006h,FFFCh]`;
- **each instruction costs one unit of `*budget`.**

The flags follow the pinned DOSBox-X stored rules: OR clears CF, OF and AF; INC keeps CF. The arithmetic helpers are 2E03's, verbatim,
plus `inc8`. LES reads its segment word at (offset+2) mod 64K.

| Stop | Pinned by |
| --- | --- |
| RETURNED | Unicorn and native |
| LIMIT | Unicorn: the budget sweep and the cyclic lists, with equal state and equal budget at the same unexecuted instruction; C test |
| TRANSFER | kept as a stop. No case reached it: the counter `INC` over the frame (`counter-on-frame*`) ends LIMIT or RETURNED in both runs |
| ARGUMENT | C test: null RAM, CPU or budget; short RAM; IP; SP. CPU and RAM untouched |

## Checks

**Original-byte Unicorn differential: 349 cases, plus 1 targeted stop added in the mutation round, 0 mismatches.** Compared after every
case, **unmasked**: CPU14, **all 1,048,576 RAM bytes**, the stop, and **the remaining budget**.
- 273 RETURNED, 76 LIMIT, plus the targeted LIMIT stop.
- **29/29 instruction starts, 4/4 conditional outcomes.**
- **271 independent statements**: the counters, and SP+4, BP and CS:IP, from the stated rule without the C model, on plain cases that alias
  neither the frame nor each other.

Cases:
- an empty party; one character; three mixed;
- signed indices 00/01/02/7F/80/FE/FF; flag bytes 01/80/FF;
- a 260-node party (a counter wraps through 00h);
- a 128-node party (a counter reaches 80h);
- a null-segment head with a non-zero offset;
- link and field offsets wrapping at 16 bits; A20 wraps of a character and of DS;
- six flag images, on parties and on the empty party; five entry SPs;
- with DS = SS, a counter and the head on each frame byte;
- a character's link on the frame, and a counter that is a character's link;
- two cyclic parties; a budget sweep; 200 random cases.

**Mutation: 11 deliberate mutants, 11 caught by the differential**, in two stated rounds:
- the counters are not zeroed; the index is zero-extended (no CBW); the flag byte is +10Ch; the index byte is +10Fh; the next link is +100h; the head is DS:5D92; only the offset word tests null; JE as JNE on the flag; INC without OF; the budget is not charged in the loop body; RETF pops an argument word;
- round 1 caught 10. **"INC without OF"** survived as a **case gap**: OF is set only when a counter goes 7Fh→80h, and the next flag writer
  (the OR at 23FB) overwrites it, so only a stop right after that INC can see it;
- round 2 added `stop-after-inc-80` at budget 2179, measured with the C model (LIMIT at 241A, flags 0A92h), and it catches the mutant (C 0292h);
- one further candidate, **"INC clears CF"**, is **equivalent** and is not counted. The only flag writer before the INC is
  `CMP BYTE ES:[DI+10Dh],0`, which never borrows, so CF is always 0 at the INC; LES, MOV, CBW and MOV write no flags.

**Native (synthetic DOSBox-X; packaged DOSBox-X 2026.01.02, sha256 99d74601, the same release as pinned 5a9c7c53):** 17 cases.
- The original bytes run under a real DOS.
- The cases: empty, one, three mixed; five signed indices; a 128-node party to 80h; a link wrap; four flag images; SP bounds.
- CPU14 and five 64 KiB regions: **5,571,036 bytes, 0 mismatches, no masks.** IF is cleared in every native case.

**C test** (asserts on): the empty party; counting with a negative index; the OR flags at return; a cyclic party to LIMIT; budget 0; the guards.

**Census:** no new flag rows. The helpers are 2E03's verbatim plus an `inc8` that hashes to an existing row. The shared row `670398ea…` now
lists 2E03, 23B7 and 23DB, with a widened caller description.

**Aperture census:** a new `flat` row.

**credit_evidence_kind.csv:** regenerated. `RES_00BA_23DB` now lists this checker beside `res00ba_rec_models_audit.py`. That touches a
credit table, but **the credit itself is unchanged**.

## What this does not show

- Credit or identity: unchanged.
- The hybrid glue: par-b's next step, after merge.
- Cyclic parties beyond the budget. They never end, by contract.

## Reproduction

    python3 tools/overlay/res00ba_party_count_23db_cpu_audit.py --unicorn --mutate --native --output resources/res00ba-23db-evidence.json
    python3 tools/overlay/res00ba_party_count_23db_cpu_audit.py --compile-only
    python3 tools/overlay/check_checker_builds.py --require-current-main RES_00BA_23DB
