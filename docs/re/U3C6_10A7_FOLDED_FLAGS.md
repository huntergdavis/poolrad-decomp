# U3C6 +10A7: folded flag evidence

The first t44 slice covers the 14 comments for
`overlay_u3c6_caller_10a7_cpu.c` in `FLAG_FOLDED_SITES.tsv` at `9d55a9b0`.
Those are historical source-line identities, not 14 individual instructions.
Adjacent unlabelled writers are included so the certificate accounts for every
arithmetic writer in this model's two owned original spans.

The shared walker proves the existing folds sound under the stated contract.
Thirteen CMP/OR writes feed only ZF branches; existing C expressions reproduce
those decisions. Two SHLs expose all six arithmetic flags, already computed
by the model. This change adds evidence and makes their checker fail closed
on the pinned AF and stored-PF rules. The recovered C, APIs and credit stay unchanged.

## Original evidence and scope

Media provenance is the [parent model's](U3C6_CALLER_10A7_CPU.md#exact-original):
GAME.OVR SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`,
START.EXE SHA-256
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`.

| Owned original span | Bytes / instructions | SHA-256 |
| --- | --- | --- |
| Entry 15, `+0C5F..+0CB8` | 90 / 32 | `c153e951cb4be5e40f4fd65d9e6d114a0b2d3cbd615bc73025a3d2c4c387de41` |
| Caller, `+10A7..+1297` | 497 / 191 | `50821d75da31383ab9920ef18edaadb16d9ffdea34cc4348c8e86d70903fd1fe` |

There are 68 arithmetic writers in these 223 instructions. The 14 groups
contain 57: 42 dead before observation, 13 observed only by ZF branches, and two
already-computed live shifts. The other 11 writers already compute arithmetic
flags. These counts describe this slice, not the project's remaining folds.

The proof follows original control flow and tracks CF/PF/AF/ZF/SF/OF separately.
Under the [pinned rules](FLAG_SEMANTICS_CENSUS.md), MULW preserves AF/SF and INC
preserves CF; neither is a full overwrite. All calls, returns, PUSHF and INT
boundaries expose flags. The declared stops at `+117F` (loop cap) and `+11DF`
(FillChar/CODE) observe flags before that instruction executes. Conditional
branches observe their input flags. All 13 branch-only writers feed ZF branches,
so no lazy/stored discrepancy changes the branch decision. The walker reports
BRANCH for these writes, not DEAD; the C condition is their required consumer.

The contract keeps code immutable except for the existing tested FillChar-site
rejection. The proof does not claim arbitrary debugger snapshots or asynchronous
interrupt observation between every original instruction. Separately compiled
callees retain their own contracts; this slice does not certify their folds.

## Method

`tools/overlay/u3c6_10a7_folded_flags.py` calls the shared
`tools/overlay/flag_fold_liveness.py` walker, checks both original-span hashes,
and rejects any missing writer or changed observer. These direct commands
reproduce the same 57 classifications (span ends are exclusive):

```sh
python3 tools/overlay/flag_fold_liveness.py --unit 0x3C6 --span 0C5F:0CB9 \
  --site 0C62 --site 0C71 --site 0C77 --site 0C7B --site 0C7D \
  --site 0C87 --site 0C8C --site 0C95 --site 0C9E --json
python3 tools/overlay/flag_fold_liveness.py --unit 0x3C6 --span 10A7:1298 --stop 117F --stop 11DF \
  --site 10AA --site 10CF --site 10D6 --site 10DA --site 10DC --site 10E8 --site 10EC \
  --site 10EE --site 10FA --site 10FE --site 1100 --site 111D --site 1123 --site 112C \
  --site 1130 --site 1132 --site 1134 --site 113E --site 1142 --site 1144 --site 117F \
  --site 1185 --site 118A --site 1196 --site 119B --site 11A3 --site 11A8 --site 11AC \
  --site 11B6 --site 11BB --site 11C9 --site 11CE --site 1202 --site 1208 --site 120D \
  --site 1215 --site 121E --site 1225 --site 122A --site 1233 --site 123A --site 1241 \
  --site 1246 --site 124F --site 126B --site 1272 --site 1277 --site 127D --json
```

## Inventory

| Group / legacy C line | Dead writers → overwrites | Observed writer → first observer |
| --- | --- | --- |
| 1 / 140 | 10D6 → 10DA; 10DA → 10DC; 10DC → 10E8; 10E8 → 10EC; 10EC → 10EE; 10EE → 10FA; 10FA → 10FE; 10FE → 1100; 113E → 1142; 1142 → 1144 | 1100 → 111A (existing SHL); 1144 → 1295 (existing SHL) |
| 2 / 160 | 1185 → 118A(CF/OF/PF/ZF),1196(AF/SF); 118A → 1196(CF/OF/PF/ZF); 11A3 → 11A8(CF/OF/PF/ZF),11AC(AF/SF); 11A8 → 11AC(CF/OF/PF/ZF); 11B6 → 11BB(CF/OF/PF/ZF),11C9(AF/SF); 11BB → 11C9(CF/OF/PF/ZF); 11C9 → 11CE(CF/OF/PF/ZF),11D2(AF/SF); 11CE → 11D2(CF/OF/PF/ZF); 1208 → 120D(CF/OF/PF/ZF),1215(AF/SF); 120D → 1215(CF/OF/PF/ZF); 1225 → 122A(CF/OF/PF/ZF),1233(AF/SF); 122A → 1233(CF/OF/PF/ZF); 1241 → 1246(CF/OF/PF/ZF),124F(AF/SF); 1246 → 124F(CF/OF/PF/ZF) | — |
| 3 / 181 | 0C62 → 0C71(AF/OF/PF/SF/ZF),0C77(CF) | — |
| 4 / 188 | 0C71 → 0C77(AF/OF/PF/SF/ZF); 0C77 → 0C7B; 0C7B → 0C7D; 0C7D → 0C87 | — |
| 5 / 194 | — | 0C87 → 0C8A (ZF branch); 0C8C → 0C8F (ZF branch); 0C95 → 0C99 (ZF branch); 0C9E → 0CA2 (ZF branch) |
| 6 / 313 | — | 1196 → 1199 (ZF branch); 119B → 119E (ZF branch); 11AC → 11B1 (ZF branch) |
| 7 / 342 | 117F → 1185(AF/OF/PF/SF/ZF) | — |
| 8 / 355 | 121E → 1225; 123A → 1241 | 1215 → 1219 (ZF branch); 1233 → 1235 (ZF branch); 124F → 1251 (ZF branch) |
| 9 / 376 | 1202 → 1208(AF/OF/PF/SF/ZF) | — |
| 10 / 386 | 126B → 1272; 1272 → 1277(CF/OF/PF/ZF),127D(AF/SF); 1277 → 127D(CF/OF/PF/ZF); 127D → 127F | — |
| 11 / 412 | 10AA → 10AD | — |
| 12 / 427 | 10CF → 10D6 | — |
| 13 / 449 | — | 111D → 1121 (ZF branch) |
| 14 / 451 | 112C → 1130; 1130 → 1132; 1132 → 1134 | 1123 → 1127 (ZF branch); 1134 → 1139 (ZF branch) |

## Branch consumers and exposed shifts

Every row below is **BRANCH**, with only ZF read. JE takes the edge for equality
(ZF=1), and JNE for inequality (ZF=0). The existing C comparisons reproduce
these tests; none requires a stored FLAGS image after the branch.

| Writer | Original comparison | Consumer reading ZF |
| --- | --- | --- |
| `+0C87` | DX versus `[BP+8]` (pointer segment) | `+0C8A JNE` |
| `+0C8C` | AX versus `[BP+6]` (pointer offset) | `+0C8F JNE` |
| `+0C95` | byte `[BP-2]` versus zero (found slot) | `+0C99 JNE` |
| `+0C9E` | AL versus byte `[5F2A]` (slot count) | `+0CA2 JNE` |
| `+1196` | DX versus `[BP+0E]` (Q segment) | `+1199 JNE` |
| `+119B` | AX versus `[BP+0C]` (Q offset) | `+119E JNE` |
| `+11AC` | byte `[DI+66DC]` versus 1Fh | `+11B1 JE` |
| `+1215` | OR pointer offset/segment; zero iff both zero | `+1219 JE` |
| `+1233` | AX versus CX (X coordinate) | `+1235 JNE` |
| `+124F` | AX versus CX (Y coordinate) | `+1251 JNE` |
| `+111D` | byte `[BP-4]` versus zero | `+1121 JNE` |
| `+1123` | byte `[BP-5]` versus zero | `+1127 JE` |
| `+1134` | byte `[DI+2744]` versus FFh (rank) | `+1139 JNE` |

The two **EXPOSED** writes are `SHL DI,1` at `+1100`, observed by the
`CALL +0CB9` at `+111A`, and `SHL DI,1` at `+1144`, observed by the
`RETF 0Ah` at `+1295`. Both use `slot_index(&m, 1)` in the existing C.
For old DI = 2s and result DI = 4s, the pinned nonzero-shift rules are:
AF=1, stored PF=even parity of the result's **low byte**, CF=old bit 15,
OF=old/result sign-bit XOR, ZF=(result=0), SF=result bit 15. Other FLAGS
bits are preserved. These rules were read directly from unchanged
`src/cpu/flags.cpp` at pinned revision
`5a9c7c53481c2e933948039c33256776653f4faf`: `DOFLAG_PF` at line 504
and the stored `t_SHLw` case at lines 720–728. File SHA-256:
`3ea36c4c56278ef14ec0f89ac93966197ed389ce4daaee7afa3d9de39e02d893`.
The shared walker correctly treats these SHLs as
writing all six arithmetic flags, including AF.

## Validation

The audit builds a temporary probe from the unchanged recovered C. It reads
FLAGS and DI after each of the two live `slot_index` calls into a separate
host buffer; the positive probe changes no emulated state. The oracle records
the same points immediately after each original SHL. It requires the entire
FLAGS word, setting only the expected AF bit to the pinned value of one.
Stored PF is independently checked against low-byte parity. This observes
`+1100` before the callee can overwrite flags. Final slot-clearing returns
also require AF=1; matching Unicorn's AF=0 no longer passes.

The `fail-rank-slot64-pf` fixture reaches both sites with DI=0100h: low-byte
parity is even while whole-word parity is odd. Four temporary negative
controls separately force AF=0 or whole-word PF at each site; each must fail
at that site's FLAGS probe. The native oracle compiles ordinary source. Set `PINNED_DOSBOX_X` to the
pinned executable; the report records its SHA-256, core and CPU type.

```sh
python3 -m unittest discover -s tools/overlay -p test_flag_fold_liveness.py
python3 tools/overlay/u3c6_caller_10a7_cpu_audit.py --unicorn --native --negative-tests --dosbox-x "$PINNED_DOSBOX_X" \
  --output resources/t44-10a7/report.json
```

Measured on 2026-10-23:

- Shared walker: all 57 classified writers, 42 DEAD / 13 BRANCH / 2 EXPOSED;
  21 synthetic walker tests pass.
- Original-byte differential: **144 cases**, 139 returns / 1 CODE / 2 CALLEE /
  1 TRANSFER / 1 LIMIT; CPU14 and the full 1 MiB RAM match, with the pinned
  AF=1 expectation at 72 slot-clearing returns and no masked bits.
- Coverage: **191/191** caller instruction starts, **34/34** conditional
  outcomes; 133 immediate observations at `+1100` and 72 at `+1144`.
- Negative controls: all four fail at their intended site. The positive
  slot64 FLAGS/DI image is `0616/0100`; AF=0 produces `0606/0100`,
  and word-parity PF produces `0612/0100`, independently at each site.

Pinned DOSBox-X normal/386 also passes **48 cases**, comparing CPU14, the
512-byte stack window, all 64 KiB DS and the 0700h-byte map window, with no
mask. All 29 slot-clearing returns have AF=1. The binary SHA-256 is
`c05ce28a60258c320c5a209a477603d91fb2b7558c0cc03919e518ddd99eb457`;
result SHA-256 is
`6fa0411461f81622f35571719a7362b348a5a60f6f149c567be5e121206f7c86`.
The pinned build includes existing instrumentation in `core_normal.cpp`,
`cpu.cpp` and `debug.cpp`; `flags.cpp` is unchanged.

All 27 local repository-hygiene steps passed, including 207 actual checker
builds for 240 rows. After rebasing from `e216742c` to `1354e06a`,
all 26 non-build steps passed again; only the U9E dispatch checker hash
changed. That row's compile/load/bind refresh passed; the other 206 checker
hashes are unchanged. The repository CMake focused test passes **52 checks**
with an empty build type and `-UNDEBUG` (verified in its compile command).

Recall provenance: Dunking Sheep session
`281c8700-5acc-438b-b93d-c061e28437eb` located the prior 143-case audit. Its
summary was a discovery lead only; the results above were measured afresh.

## Review record

- `target_function`: OVL_03C6_10A7, including its local entry-15 expansion.
- `source_candidates`: unchanged raw-CPU model; no new body or identity is proposed.
- `signals`: two pinned original spans, complete arithmetic-writer inventory,
  observer/overwrite paths, two-site live-SHL FLAGS trace, full-state and native
  regression checks.
- `score`: 4/4 evidence checks passed (pinned spans/inventory, shared liveness,
  full-state plus negative controls, native plus focused C test); not a probability or credit.
- `verdict`: folds are sound under the stated contract; both exposed shifts
  are checked fail-closed. Independent review remains the verifying lane's call.
- `review_notes`: immutable-code contract, declared host stops, pinned AF=1 requirement at both live SHLs, and separate callee scope remain explicit.
