# Owned-row evidence sweep: the 13 rows behind "no cheap credits"

The identity-cleared pool looked like an authoring backlog. It is mostly a
**review** backlog: six of thirteen owned rows already hold evidence at the bar
and are waiting on a verdict, and one more needs a two-line checker change
before it can be judged.

Derived from the queue's ownership set (rows with a caller or a fixup point),
not from the "58/63 composable" figure, which counts readiness only. Every
"owned" claim below is `callers.points > 0` or `callers_fixup_backed > 0`.

## The table

| Row | B | Class | Evidence doc | Audit harness | Native harness | `CHECKER_ROWS` | Measured native | Next action |
| --- | ---: | --- | --- | --- | --- | --- | --- | --- |
| `OVL_03B0_0C33` | 146 | game_evidenced | U3B0_ENTRY_0C33_CPU.md | yes | yes | **yes** | 197,632 RAM bytes | **ACCEPT** |
| `OVL_006A_0824` | 155 | game_evidenced | U6A_0824_NATIVE_FULLC_GATE.md | yes | synthetic only | no | synthetic COM frames | **AUTHOR** — needs a genuine reached caller capture |
| `OVL_001C_2E7D` | 176 | game_verified | U1C_TEXT_2E7D_CPU.md | yes | yes | **added by this PR** | 983,040 RAM, result sha reproduced | **ACCEPT** — packet in this PR |
| `OVL_0077_17ED` | 176 | game_evidenced | U77_SPLIT_17ED.md | no | no | no | — | **AUTHOR** — differential + native |
| `OVL_007D_05F0` | 232 | game_evidenced | U7D_ENTRY_05F0_CPU.md | yes | yes | **no** | 983,040 RAM bytes | **ACCEPT** — register checker first |
| `OVL_03B0_0A08` | 251 | game_evidenced | U3B0_ENTRY_0A08_CPU.md | yes | yes | **yes** | 197,632 RAM bytes | **ACCEPT** |
| `OVL_0023_0239` | 296 | game_evidenced | none | no | no | no | — | **AUTHOR** |
| `OVL_03B0_07A4` | 307 | game_evidenced | U3B0_ENTRY_07A4_CPU.md | yes | yes | **yes** | 197,632 RAM bytes | **ACCEPT** |
| `OVL_0020_11AC` | 316 | game_evidenced | none | no | no | no | — | **AUTHOR** |
| `OVL_0068_0294` | 587 | game_evidenced | OVL_0068_0294_DISPOSITION.md | yes | yes | no | — | **ESTABLISH IDENTITY** — dispositioned "no identity, no strict credit" |
| `OVL_0025_07BC` | 696 | game_evidenced | none | no | no | no | — | **AUTHOR** — no body exists; `ip = 0x07BC` redirect only |
| `OVL_0038_0CC3` | 745 | game_evidenced | none | no | no | no | — | **ESTABLISH IDENTITY** — zero callers |
| `OVL_0033_08A4` | 755 | game_evidenced | U33_BYTE_FLAGS_08A4.md | yes | yes (synthetic frames) | **no** | 2,644 cases / full 1 MiB | **ACCEPT** — register checker first; declared synthetic-native limit |

**Totals: 6 accept (1,867 B), 4 author (1,543 B), 2 establish identity (1,441 B),
1 author-and-blocked (696 B).**

"At the bar" here means: an evidence document that states a measured
original-byte differential with 14 CPU words and full-RAM-or-declared-equivalent
comparison, named stops, a native replay, and its own limits in the same
document. Three of the six accept candidates satisfy that and are also
checker-registered, so they need only a verdict; two (`05F0`, `08A4`) need their
checker registered first; `2E7D` needed `--compile-only` and a registration,
both added here.

## What this changes

- The queue's "composable" count measures readiness, and **readiness is not
  ownership and not evidence**. 40 of 56 uncredited zero-unready rows are
  five-byte callerless tails; see the correction in
  [STRICT_CANDIDATE_QUEUE.md](STRICT_CANDIDATE_QUEUE.md).
- The next three credits in this pool are **verdicts, not bodies**. If the
  acceptance lane takes `0C33`, `0A08`, and `07A4` (all checker-registered,
  all measured), the checkpoint moves 252 → 255 without a line of new C.
- Authoring effort should go to `0239`, `11AC`, `17ED`, `0824` — and only those,
  until the two identity questions (`0294`, `0CC3`) are answered.
